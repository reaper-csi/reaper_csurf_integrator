//
//  control_surface_integrator_ui.cpp
//  reaper_csurf_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_integrator_ui.h"

extern REAPER_PLUGIN_HINSTANCE g_hInst;

////////////////////////////////////////////////////////////////////////////////////////////////////////
// structs
////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RealSurfaceLine
{
    string name = "";
    int numChannels = 0;
    int numBankableChannels = 0;
    int midiIn = 0;
    int midiOut = 0;
    string templateFilename = "";
};

struct SurfaceLine
{
    string realSurfaceName = "";
    string actionTemplateFolder = "";
    string FXTemplateFolder = "";
};

struct SurfaceGroupLine
{
    string name = "";
    vector<SurfaceLine*> surfaces;
};

struct LogicalSurfaceLine
{
    string name = "";
    vector<SurfaceGroupLine*> surfaceGroups;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfIntegrator
////////////////////////////////////////////////////////////////////////////////////////////////////////
CSurfIntegrator::CSurfIntegrator()
{
    manager_ = new CSurfManager();
}

CSurfIntegrator::~CSurfIntegrator()
{
    delete manager_;
}

void CSurfIntegrator::OnTrackSelection(MediaTrack *trackid)
{
    manager_->OnTrackSelection(trackid);
}

int CSurfIntegrator::Extended(int call, void *parm1, void *parm2, void *parm3)
{
    if(call == CSURF_EXT_SUPPORTS_EXTENDED_TOUCH)
    {
        return 1;
    }
    
    if(call == CSURF_EXT_RESET)
    {
        //ClearAllTouchStates();
        //Initialize();
    }
    
    if(call == CSURF_EXT_SETFXCHANGE)
    {
        // parm1=(MediaTrack*)track, whenever FX are added, deleted, or change order
        manager_->TrackFXListChanged((MediaTrack*)parm1);
    }

    return 1;
}

bool CSurfIntegrator::GetTouchState(MediaTrack *track, int touchedControl)
{
    return manager_->GetTouchState(DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false)), touchedControl);
}

void CSurfIntegrator::Run()
{
    manager_->Run();
}

void CSurfIntegrator::SetTrackListChange()
{
    manager_->TrackListChanged();
}

const char *CSurfIntegrator::GetTypeString()
{
    return "CSI";
}

const char *CSurfIntegrator::GetDescString()
{
    descspace.Set(Control_Surface_Integrator.c_str());
    return descspace.Get();
}

const char *CSurfIntegrator::GetConfigString() // string of configuration data
{
    sprintf(configtmp,"0 0");
    return configtmp;
}

static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
    return new CSurfIntegrator();
}

void AddComboEntry(HWND hwndDlg, int x, char * buf, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)buf);
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
}

void AddListEntry(HWND hwndDlg, string buf, int comboId)
{
    SendDlgItemMessage(hwndDlg, comboId, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
}

vector<RealSurfaceLine*> realSurfaces;
vector<LogicalSurfaceLine*> logicalSurfaces;

bool editMode = false;
static int dlgResult = 0;

static char name[BUFSZ];
static int numChannels = 0;
static int numBankableChannels = 0;
static int midiIn = 0;
static int midiOut = 0;
static char templateFilename[BUFSZ];
static char realSurfaceName[BUFSZ];
static char actionTemplateFolder[BUFSZ];
static char FXTemplateFolder[BUFSZ];

static WDL_DLGRET dlgProcLogicalSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_LogicalSurfaceName, name);
            }
        }
            
        case WM_COMMAND:
            {
                switch(LOWORD(wParam))
                {
                    case IDOK:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            GetDlgItemText(hwndDlg, IDC_EDIT_LogicalSurfaceName, name, sizeof(name));
                            dlgResult = IDOK;
                            EndDialog(hwndDlg, 0);
                        }
                        break ;
                        
                    case IDCANCEL:
                        if (HIWORD(wParam) == BN_CLICKED)
                            EndDialog(hwndDlg, 0);
                        break ;
                }
            }
            break ;
            
        case WM_CLOSE:
            DestroyWindow(hwndDlg);
            break ;
            
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            break;
            
        default:
            return DefWindowProc(hwndDlg, uMsg, wParam, lParam) ;
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcRealSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            string path(DAW::GetResourcePath());
            path += "/CSI/rst/";
            int i = 0;
            for(auto filename : FileSystem::GetDirectoryFilenames(path))
            {
                int length = filename.length();
                if(length > 4 && filename[0] != '.' && filename[length - 4] == '.' && filename[length - 3] == 'r' && filename[length - 2] == 's' &&filename[length - 1] == 't')
                    AddComboEntry(hwndDlg, i++, (char*)filename.c_str(), IDC_COMBO_SurfaceTemplate);
            }

            char buf[BUFSZ];
            
            int n = GetNumMIDIInputs();
            for (int i = 0; i < n; i++)
                if (GetMIDIInputName(i, buf, sizeof(buf)))
                    AddComboEntry(hwndDlg, i, buf, IDC_COMBO_MidiIn);
            
            n = GetNumMIDIOutputs();
            for (int i = 0; i < n; i++)
                if (GetMIDIOutputName(i, buf, sizeof(buf)))
                    AddComboEntry(hwndDlg, i, buf, IDC_COMBO_MidiOut);
            
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_RealSurfaceName, name);
            }
            else
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiIn), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiOut), CB_SETCURSEL, 0, 0);
            }
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_RealSurfaceName, name, sizeof(name));
                        dlgResult = IDOK;
                        EndDialog(hwndDlg, 0);
                    }
                    break ;
                    
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EndDialog(hwndDlg, 0);
                    break ;
            }
        }
            break ;
            
        case WM_CLOSE:
            DestroyWindow(hwndDlg) ;
            break ;
            
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            break;
            
        default:
            return DefWindowProc(hwndDlg, uMsg, wParam, lParam) ;
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcSurfaceGroup(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_LogicalSurfaceGroup, name);
            }
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_LogicalSurfaceGroup , name, sizeof(name));
                        dlgResult = IDOK;
                        EndDialog(hwndDlg, 0);
                    }
                    break ;
                    
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EndDialog(hwndDlg, 0);
                    break ;
            }
        }
            break ;
            
        case WM_CLOSE:
            DestroyWindow(hwndDlg) ;
            break ;
            
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            break;
            
        default:
            return DefWindowProc(hwndDlg, uMsg, wParam, lParam) ;
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            for(auto* surface :  realSurfaces)
                AddComboEntry(hwndDlg, 0, (char *)surface->name.c_str(), IDC_COMBO_RealSurface);
            
            string resourcePath(DAW::GetResourcePath());
            resourcePath += "/CSI/";
            
            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "axt/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_ActionTemplates);

            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "fxt/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_FXTemplates);
            
            if(editMode)
            {
                editMode = false;
                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_RealSurface), CB_FINDSTRING, -1, (LPARAM)name);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_RealSurface), CB_SETCURSEL, index, 0);
                
                index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ActionTemplates), CB_FINDSTRING, -1, (LPARAM)actionTemplateFolder);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ActionTemplates), CB_SETCURSEL, index, 0);
                
                index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FXTemplates), CB_FINDSTRING, -1, (LPARAM)FXTemplateFolder);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FXTemplates), CB_SETCURSEL, index, 0);
            }
            else
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_RealSurface), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ActionTemplates), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FXTemplates), CB_SETCURSEL, 0, 0);
            }
        }
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_COMBO_RealSurface , name, sizeof(name));
                        GetDlgItemText(hwndDlg, IDC_COMBO_ActionTemplates , actionTemplateFolder, sizeof(actionTemplateFolder));
                        GetDlgItemText(hwndDlg, IDC_COMBO_FXTemplates , FXTemplateFolder, sizeof(FXTemplateFolder));
                        dlgResult = IDOK;
                        EndDialog(hwndDlg, 0);
                    }
                    break ;
                    
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EndDialog(hwndDlg, 0);
                    break ;
            }
        }
            break ;
            
        case WM_CLOSE:
            DestroyWindow(hwndDlg) ;
            break ;
            
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            break;
            
        default:
            return DefWindowProc(hwndDlg, uMsg, wParam, lParam) ;
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcMainConfig(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            {
                switch(LOWORD(wParam))
                {
                    case IDC_LIST_LogicalSurfaces:
                        if (HIWORD(wParam) == LBN_SELCHANGE)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                for(auto* surfaceGroup : logicalSurfaces[index]->surfaceGroups)
                                    AddListEntry(hwndDlg, surfaceGroup->name, IDC_LIST_SurfaceGroups);
                            }
                            else
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_SurfaceGroups), LB_RESETCONTENT, 0, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                            }
                        }
                        break;
                        
                    case IDC_LIST_SurfaceGroups:
                        if (HIWORD(wParam) == LBN_SELCHANGE)
                        {
                            int logicalSurfaceIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_GETCURSEL, 0, 0);
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_SurfaceGroups, LB_GETCURSEL, 0, 0);
                            if (logicalSurfaceIndex >= 0 && index >= 0)
                            {
                                for(auto* surface: logicalSurfaces[logicalSurfaceIndex]->surfaceGroups[index]->surfaces)
                                    AddListEntry(hwndDlg, surface->realSurfaceName, IDC_LIST_Surfaces);
                            }
                            else
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                            }
                        }
                        break;
                        
                    case IDC_BUTTON_AddRealSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_RealSurface), hwndDlg, dlgProcRealSurface);
                            if(dlgResult == IDOK)
                                AddListEntry(hwndDlg, name, IDC_LIST_RealSurfaces);
                        }
                        break ;
                        
                    case IDC_BUTTON_AddSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Surface), hwndDlg, dlgProcSurface);
                            if(dlgResult == IDOK)
                                AddListEntry(hwndDlg, name, IDC_LIST_Surfaces);
                        }
                        break ;
                        
                    case IDC_BUTTON_AddSurfaceGroup:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_SurfaceGroup), hwndDlg, dlgProcSurfaceGroup);
                            if(dlgResult == IDOK)
                                AddListEntry(hwndDlg, name, IDC_LIST_SurfaceGroups);
                        }
                        break ;
                        
                    case IDC_BUTTON_AddLogicalSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_LogicalSurface), hwndDlg, dlgProcLogicalSurface);
                            if(dlgResult == IDOK)
                                AddListEntry(hwndDlg, name, IDC_LIST_LogicalSurfaces);
                        }
                        break ;
                        
                    case IDC_BUTTON_EditRealSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_RealSurfaces, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_RealSurfaces), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(name));
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_RealSurface), hwndDlg, dlgProcRealSurface);
                                if(dlgResult == IDOK)
                                {
                                    //logicalSurfaces[logicalSurfaceIndex]->surfaceGroups[index]->name = name;
                                    //SendMessage(GetDlgItem(hwndDlg, IDC_LIST_SurfaceGroups), LB_RESETCONTENT, 0, 0);
                                    //for(auto* surfaceGroup: logicalSurfaces[logicalSurfaceIndex]->surfaceGroups)
                                    //AddListEntry(hwndDlg, surfaceGroup->name, IDC_LIST_SurfaceGroups);
                                }
                            }
                        }
                        break ;
                        
                    case IDC_BUTTON_EditSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Surfaces, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(name));
                                
                                int logicalSurfaceIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_GETCURSEL, 0, 0);
                                int surfaceGroupIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_SurfaceGroups, LB_GETCURSEL, 0, 0);

                                SurfaceLine* surfaceLine = nullptr;
                                
                                if(logicalSurfaceIndex >= 0 && surfaceGroupIndex >= 0)
                                {
                                    for(auto* surface : logicalSurfaces[logicalSurfaceIndex]->surfaceGroups[surfaceGroupIndex]->surfaces)
                                        if(surface->realSurfaceName == name)
                                        {
                                            surfaceLine = surface;
                                            
                                            strcpy(actionTemplateFolder, surface->actionTemplateFolder.c_str());
                                            strcpy(FXTemplateFolder, surface->FXTemplateFolder.c_str());
                                        }
                                
                                    dlgResult = false;
                                    editMode = true;
                                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Surface), hwndDlg, dlgProcSurface);
                                    if(dlgResult == IDOK)
                                    {
                                        surfaceLine->realSurfaceName = name;
                                        surfaceLine->actionTemplateFolder = actionTemplateFolder;
                                        surfaceLine->FXTemplateFolder = FXTemplateFolder;
                                        
                                        int logicalSurfaceIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_GETCURSEL, 0, 0);
                                        int index = SendDlgItemMessage(hwndDlg, IDC_LIST_SurfaceGroups, LB_GETCURSEL, 0, 0);
                                        if (logicalSurfaceIndex >= 0 && index >= 0)
                                        {
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                                            for(auto* surface: logicalSurfaces[logicalSurfaceIndex]->surfaceGroups[index]->surfaces)
                                                AddListEntry(hwndDlg, surface->realSurfaceName, IDC_LIST_Surfaces);
                                        }
                                    }
                                }
                            }
                        }
                        break ;

                    case IDC_BUTTON_EditSurfaceGroup:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int logicalSurfaceIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_GETCURSEL, 0, 0);
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_SurfaceGroups, LB_GETCURSEL, 0, 0);
                            if(logicalSurfaceIndex >= 0 && index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_SurfaceGroups), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(name));
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_SurfaceGroup), hwndDlg, dlgProcSurfaceGroup);
                                if(dlgResult == IDOK)
                                {
                                    logicalSurfaces[logicalSurfaceIndex]->surfaceGroups[index]->name = name;
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_SurfaceGroups), LB_RESETCONTENT, 0, 0);
                                    for(auto* surfaceGroup: logicalSurfaces[logicalSurfaceIndex]->surfaceGroups)
                                        AddListEntry(hwndDlg, surfaceGroup->name, IDC_LIST_SurfaceGroups);
                                }
                            }
                        }
                        break ;

                    case IDC_BUTTON_EditLogicalSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_LogicalSurfaces), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(name));
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_LogicalSurface), hwndDlg, dlgProcLogicalSurface);
                                if(dlgResult == IDOK)
                                {
                                    logicalSurfaces[index]->name = name;
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_LogicalSurfaces), LB_RESETCONTENT, 0, 0);
                                    for(auto* surface: logicalSurfaces)
                                        AddListEntry(hwndDlg, surface->name, IDC_LIST_LogicalSurfaces);
                                }
                            }
                        }
                        break ;
                }
            }
            break ;
            
        case WM_INITDIALOG:
        {
            ifstream iniFile(string(DAW::GetResourcePath()) + "/CSI/CSI.ini");
            
            for (string line; getline(iniFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens[0] == MidiInMonitor)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        if(tokens[1] == "On")
                            CheckDlgButton(hwndDlg, IDC_CHECK_MidiInMon, BST_CHECKED);
                    }
                    else if(tokens[0] == MidiOutMonitor)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        if(tokens[1] == "On")
                            CheckDlgButton(hwndDlg, IDC_CHECK_MidiOutMon, BST_CHECKED);
                    }
                    else if(tokens[0] == VSTMonitor)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        if(tokens[1] == "On")
                            CheckDlgButton(hwndDlg, IDC_CHECK_VSTParamMon, BST_CHECKED);
                    }
                    else if(tokens[0] == RealSurface_)
                    {
                        if(tokens.size() != 7)
                            continue;
                    
                        RealSurfaceLine* surface = new RealSurfaceLine();
                        surface->name = tokens[1];
                        surface->numChannels = atoi(tokens[2].c_str());
                        surface->numBankableChannels = atoi(tokens[3].c_str());
                        surface->midiIn = atoi(tokens[4].c_str());
                        surface->midiOut = atoi(tokens[5].c_str());
                        surface->templateFilename = tokens[6];
                        realSurfaces.push_back(surface);
                        
                        AddListEntry(hwndDlg, surface->name, IDC_LIST_RealSurfaces);
                        
                    }
                    else if(tokens[0] == LogicalSurface_)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        LogicalSurfaceLine* logicalSurface = new LogicalSurfaceLine();
                        logicalSurface->name = tokens[1];
                        logicalSurfaces.push_back(logicalSurface);
                        
                        AddListEntry(hwndDlg, logicalSurface->name, IDC_LIST_LogicalSurfaces);

                    }
                    else if(tokens[0] == SurfaceGroup_)
                    {
                        if(tokens.size() != 2)
                            continue;
 
                        SurfaceGroupLine* surfaceGroup = new SurfaceGroupLine();
                        surfaceGroup->name = tokens[1];
                        logicalSurfaces.back()->surfaceGroups.push_back(surfaceGroup);
                    }
                    else if(tokens[0] == Surface_)
                    {
                        if(tokens.size() != 4)
                            continue;
                        
                        SurfaceLine* surface = new SurfaceLine();
                        surface->realSurfaceName = tokens[1];
                        surface->actionTemplateFolder = tokens[2];
                        surface->FXTemplateFolder = tokens[3];
                        logicalSurfaces.back()->surfaceGroups.back()->surfaces.push_back(surface);
                    }
                }
            }
            
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_RealSurfaces), LB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_LogicalSurfaces), LB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_SurfaceGroups), LB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL, 0, 0);

        }
        break;
        
        case WM_USER+1024:
        {
            // GAW TBD -- write the file
            
            //char tmp[512];
            
            //int indev=-1, outdev=-1, offs=0, size=9;
            //int r=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
            //if (r != CB_ERR) indev = SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETITEMDATA,r,0);
            //r=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
            //if (r != CB_ERR)  outdev = SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETITEMDATA,r,0);
            
            //BOOL t;
            //r=GetDlgItemInt(hwndDlg,IDC_EDIT1,&t,TRUE);
            //if (t) offs=r;
            //r=GetDlgItemInt(hwndDlg,IDC_EDIT2,&t,FALSE);
            //if (t)
            //{
                //if (r<1)r=1;
                //else if(r>256)r=256;
                //size=r;
            //}
            //int cflags=0;
            
            
            //if (IsDlgButtonChecked(hwndDlg,IDC_CHECK1))
            //cflags|=CONFIG_FLAG_FADER_TOUCH_MODE;
            
            //if (IsDlgButtonChecked(hwndDlg,IDC_CHECK2))
            //{
            //cflags|=CONFIG_FLAG_MAPF1F8TOMARKERS;
            //}
            
            //sprintf(tmp,"%d %d %d %d %d",offs,size,indev,outdev,cflags);
            //lstrcpyn((char *)lParam, tmp,wParam);
            
        }
        break;
    }
    
    return 0;
}

static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
    return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_SURFACEEDIT_CSI),parent,dlgProcMainConfig,(LPARAM)initConfigString);
}

reaper_csurf_reg_t csurf_integrator_reg =
{
    "CSI",
    Control_Surface_Integrator.c_str(),
    createFunc,
    configFunc,
};
