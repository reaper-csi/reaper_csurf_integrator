//
//  control_surface_integrator_ui.cpp
//  reaper_csurf_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_integrator_ui.h"

extern REAPER_PLUGIN_HINSTANCE g_hInst;
Manager* TheManager = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfIntegrator
////////////////////////////////////////////////////////////////////////////////////////////////////////
CSurfIntegrator::CSurfIntegrator()
{
    TheManager = new Manager();
}

CSurfIntegrator::~CSurfIntegrator()
{
    if(TheManager)
        TheManager->ResetAllWidgets();
}

void CSurfIntegrator::OnTrackSelection(MediaTrack *trackid)
{
    TheManager->OnTrackSelection(trackid);
}

int CSurfIntegrator::Extended(int call, void *parm1, void *parm2, void *parm3)
{
    if(call == CSURF_EXT_SUPPORTS_EXTENDED_TOUCH)
    {
        return 1;
    }
    
    if(call == CSURF_EXT_RESET)
    {
       TheManager->Init();
    }
    
    if(call == CSURF_EXT_SETFXCHANGE)
    {
        // parm1=(MediaTrack*)track, whenever FX are added, deleted, or change order
        TheManager->TrackFXListChanged((MediaTrack*)parm1);
    }

    return 1;
}

bool CSurfIntegrator::GetTouchState(MediaTrack *track, int touchedControl)
{
    return TheManager->GetTouchState(track, touchedControl);
}

void CSurfIntegrator::Run()
{
    TheManager->Run();
}

void CSurfIntegrator::SetTrackListChange()
{
    TheManager->TrackListChanged();
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// structs
////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiSurfaceLine
{
    string name = "";
    int numChannels = 0;
    bool isBankable = true;
    int midiIn = 0;
    int midiOut = 0;
    string templateFilename = "";
    string actionTemplateFolder = "";
    string FXTemplateFolder = "";

};

struct PageLine
{
    string name = "";
    bool followMCP = true;
    bool trackColouring = false;
    int red = 0;
    int green = 0;
    int blue = 0;
    vector<MidiSurfaceLine*> midiSurfaces;
};

// Scratch pad to get in and out of dialogs easily
static bool editMode = false;
static int dlgResult = 0;
static char name[BUFSZ];
static int numChannels = 0;
static bool isBankable = true;
static int midiIn = 0;
static int midiOut = 0;
static char templateFilename[BUFSZ];
static char actionTemplateFolder[BUFSZ];
static char FXTemplateFolder[BUFSZ];

static bool followMCP = true;
static bool trackColouring = false;

static vector<PageLine*> pages;










static WDL_DLGRET dlgProcVirtualSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            //for(auto* surface :  realSurfaces)
                //AddComboEntry(hwndDlg, 0, (char *)surface->name.c_str(), IDC_COMBO_RealSurface);
            
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
                //int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_RealSurface), CB_FINDSTRING, -1, (LPARAM)name);
                //if(index >= 0)
                    //SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_RealSurface), CB_SETCURSEL, index, 0);
                
                //index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ActionTemplates), CB_FINDSTRING, -1, (LPARAM)actionTemplateFolder);
                //if(index >= 0)
                    //SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ActionTemplates), CB_SETCURSEL, index, 0);
                
                //index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FXTemplates), CB_FINDSTRING, -1, (LPARAM)FXTemplateFolder);
                //if(index >= 0)
                    //SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FXTemplates), CB_SETCURSEL, index, 0);
                
                
                if(isBankable)
                    CheckDlgButton(hwndDlg, IDC_CHECK_IsBankable, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_IsBankable, BST_UNCHECKED);
            }
            else
            {
                CheckDlgButton(hwndDlg, IDC_CHECK_IsBankable, BST_CHECKED);
                //SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_RealSurface), CB_SETCURSEL, 0, 0);
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
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_IsBankable))
                            isBankable = true;
                        else
                            isBankable = false;
                        //GetDlgItemText(hwndDlg, IDC_COMBO_RealSurface , name, sizeof(name));
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











static WDL_DLGRET dlgProcPage(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if(followMCP)
                CheckDlgButton(hwndDlg, IDC_RADIO_MCP, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_RADIO_TCP, BST_CHECKED);
            
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_PageName, name);
                
                if(trackColouring)
                    CheckDlgButton(hwndDlg, IDC_CHECK_ColourTracks, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_ColourTracks, BST_UNCHECKED);
            }
        }
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_RADIO_MCP:
                    CheckDlgButton(hwndDlg, IDC_RADIO_TCP, BST_UNCHECKED);
                    break;
                    
                case IDC_RADIO_TCP:
                    CheckDlgButton(hwndDlg, IDC_RADIO_MCP, BST_UNCHECKED);
                    break;
                    
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_PageName , name, sizeof(name));
                        if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_MCP))
                            followMCP = true;
                        else
                            followMCP = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ColourTracks))
                            trackColouring = true;
                        else
                            trackColouring = false;
                        
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

static WDL_DLGRET dlgProcMidiSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            char buf[BUFSZ];
            int currentIndex = 0;

            for (int i = 0; i < GetNumMIDIInputs(); i++)
                if (GetMIDIInputName(i, buf, sizeof(buf)))
                {
                    AddComboEntry(hwndDlg, i, buf, IDC_COMBO_MidiIn);
                    if(editMode && midiIn == i)
                        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiIn), CB_SETCURSEL, currentIndex, 0);
                    currentIndex++;
                }
            
            currentIndex = 0;
            
            for (int i = 0; i < GetNumMIDIOutputs(); i++)
                if (GetMIDIOutputName(i, buf, sizeof(buf)))
                {
                    AddComboEntry(hwndDlg, i, buf, IDC_COMBO_MidiOut);
                    if(editMode && midiOut == i)
                        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiOut), CB_SETCURSEL, currentIndex, 0);
                    currentIndex++;
                }
            
            string resourcePath(DAW::GetResourcePath());
            int i = 0;
            for(auto filename : FileSystem::GetDirectoryFilenames(resourcePath + "/CSI/rst/"))
            {
                int length = filename.length();
                if(length > 4 && filename[0] != '.' && filename[length - 4] == '.' && filename[length - 3] == 'r' && filename[length - 2] == 's' &&filename[length - 1] == 't')
                    AddComboEntry(hwndDlg, i++, (char*)filename.c_str(), IDC_COMBO_SurfaceTemplate);
            }
            
            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "/CSI/axt/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_ActionTemplates);
            
            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "/CSI/fxt/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_FXTemplates);

            
            
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceName, name);
                SetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceNumChannels, to_string(numChannels).c_str());

                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_FINDSTRING, -1, (LPARAM)templateFilename);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, index, 0);
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
                        GetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceName, name, sizeof(name));
                        char tempBuf[BUFSZ];
                        GetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceNumChannels, tempBuf, sizeof(tempBuf));
                        numChannels = atoi(tempBuf);
                        GetDlgItemText(hwndDlg, IDC_COMBO_SurfaceTemplate, templateFilename, sizeof(templateFilename));
                        
                        int currentSelection = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiIn, CB_GETCURSEL, 0, 0);
                        if (currentSelection >= 0)
                            midiIn = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiIn, CB_GETITEMDATA, currentSelection, 0);
                        currentSelection = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiOut, CB_GETCURSEL, 0, 0);
                        if (currentSelection >= 0)
                            midiOut = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiOut, CB_GETITEMDATA, currentSelection, 0);

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
                    case IDCHOOSECOLOUR:
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                int colorOut = DAW::ColorToNative(pages[index]->red, pages[index]->green, pages[index]->blue);
                                
                                if(DAW::GR_SelectColor(hwndDlg, &colorOut))
                                    DAW::ColorFromNative(colorOut, &pages[index]->red, &pages[index]->green, &pages[index]->blue);
                            }
                        }
                        break;
                        
                    case IDC_LIST_Pages:
                        if (HIWORD(wParam) == LBN_SELCHANGE)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                //SendMessage(GetDlgItem(hwndDlg, IDC_LIST_VirtualSurfaces), LB_RESETCONTENT, 0, 0);

                                //for(auto* surface : pages[index]->virtualSurfaces)
                                    //AddListEntry(hwndDlg, surface->realSurfaceName, IDC_LIST_VirtualSurfaces);
                            }
                            else
                            {
                                //SendMessage(GetDlgItem(hwndDlg, IDC_LIST_VirtualSurfaces), LB_RESETCONTENT, 0, 0);
                            }
                        }
                        break;
                        
                    case IDC_BUTTON_AddMidiSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface), hwndDlg, dlgProcMidiSurface);
                            if(dlgResult == IDOK)
                            {
                                MidiSurfaceLine* surface = new MidiSurfaceLine();
                                surface->name = name;
                                surface->numChannels = numChannels;
                                surface->midiIn = midiIn;
                                surface->midiOut = midiOut;
                                surface->templateFilename = templateFilename;
                                //midiSurfaces.push_back(surface);
                                //AddListEntry(hwndDlg, name, IDC_LIST_Surfaces);
                                //SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL, midiSurfaces.size() - 1, 0);
                            }
                        }
                        break ;
                        
                        
                    case IDC_BUTTON_AddPage:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                dlgResult = false;
                                followMCP = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Page), hwndDlg, dlgProcPage);
                                if(dlgResult == IDOK)
                                {
                                    PageLine* page = new PageLine();
                                    page->name = name;
                                    page->followMCP = followMCP;
                                    pages.push_back(page);
                                    AddListEntry(hwndDlg, name, IDC_LIST_Pages);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_SETCURSEL, pages.size() - 1, 0);
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
                                //numChannels = midiSurfaces[index]->numChannels;
                                //midiIn = midiSurfaces[index]->midiIn;
                                //midiOut = midiSurfaces[index]->midiOut;
                                //strcpy(templateFilename, midiSurfaces[index]->templateFilename.c_str());
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface), hwndDlg, dlgProcMidiSurface);
                                if(dlgResult == IDOK)
                                {
                                    //midiSurfaces[index]->name = name;
                                    //midiSurfaces[index]->numChannels = numChannels;
                                    //midiSurfaces[index]->midiIn = midiIn;
                                    //midiSurfaces[index]->midiOut = midiOut;
                                    //midiSurfaces[index]->templateFilename = templateFilename;
                                    
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                                    //for(auto* surface: midiSurfaces)
                                        //AddListEntry(hwndDlg, surface->name, IDC_LIST_Surfaces);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL, index, 0);
                                }
                            }
                        }
                        break ;
                    

                    case IDC_BUTTON_EditPage:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(name));
                                followMCP = pages[index]->followMCP;
                                trackColouring = pages[index]->trackColouring;
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Page), hwndDlg, dlgProcPage);
                                if(dlgResult == IDOK)
                                {
                                    pages[index]->name = name;
                                    pages[index]->followMCP = followMCP;
                                    pages[index]->trackColouring = trackColouring;
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_RESETCONTENT, 0, 0);
                                    for(auto* page :  pages)
                                        AddListEntry(hwndDlg, page->name, IDC_LIST_Pages);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_SETCURSEL, index, 0);
                                }
                            }
                        }
                        break ;

                    case IDC_BUTTON_RemoveSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Surfaces, LB_GETCURSEL, 0, 0);
                            
                            if(index >= 0)
                            {
                                //midiSurfaces.erase(midiSurfaces.begin() + index);
                                
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                                //for(auto* surface: midiSurfaces)
                                    //AddListEntry(hwndDlg, surface->name, IDC_LIST_Surfaces);
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL, index, 0);
                            }
                        }
                        break ;
                        
                    case IDC_BUTTON_RemovePage:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                pages.erase(pages.begin() + index);
                                
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_RESETCONTENT, 0, 0);
                                
                                for(auto* page: pages)
                                    AddListEntry(hwndDlg, page->name, IDC_LIST_Pages);
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_SETCURSEL, index, 0);
                            }
                        }
                        break ;
                }
            }
            break ;
            
        case WM_INITDIALOG:
        {
            pages.clear();
            
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
                    else if(tokens[0] == PageToken)
                    {
                        if(tokens.size() != 7)
                            continue;
 
                        PageLine* page = new PageLine();
                        page->name = tokens[1];
                        
                        if(tokens[2] == "FollowMCP")
                            page->followMCP = true;
                        else
                            page->followMCP = false;
                        
                        if(tokens[3] == "UseTrackColoring")
                            page->trackColouring = true;
                        else
                            page->trackColouring = false;
                        
                        page->red = atoi(tokens[4].c_str());
                        page->green = atoi(tokens[5].c_str());
                        page->blue = atoi(tokens[6].c_str());
                        
                        pages.push_back(page);
                        
                        AddListEntry(hwndDlg, page->name, IDC_LIST_Pages);
                    }
                    else if(tokens[0] == MidiSurface_)
                    {
                        if(tokens.size() != 9)
                            continue;
                        
                        MidiSurfaceLine* surface = new MidiSurfaceLine();
                        surface->name = tokens[1];
                        surface->numChannels = atoi(tokens[2].c_str());
                        surface->isBankable = tokens[3] == "Bankable" ? true : false;;
                        surface->midiIn = atoi(tokens[4].c_str());
                        surface->midiOut = atoi(tokens[5].c_str());
                        surface->templateFilename = tokens[6];
                        surface->actionTemplateFolder = tokens[7];
                        surface->FXTemplateFolder = tokens[8];
                        
                        if(pages.size() > 0)
                            pages[pages.size() - 1]->midiSurfaces.push_back(surface);
                    }
                }
            }
            
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_SETCURSEL, 0, 0);

        }
        break;
        
        case WM_USER+1024:
        {
            ofstream iniFile(string(DAW::GetResourcePath()) + "/CSI/CSI.ini");

            if(iniFile.is_open())
            {
                string line = MidiInMonitor + " ";
                if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_MidiInMon))
                    line += "On";
                else
                    line += "Off";
                iniFile << line + "\n";
                
                line = MidiOutMonitor + " ";
                if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_MidiOutMon))
                    line += "On";
                else
                    line += "Off";
                iniFile << line + "\n";
                
                line = VSTMonitor + " ";
                if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_VSTParamMon))
                    line += "On";
                else
                    line += "Off";
                iniFile << line + "\n";
                
                iniFile << "\n";
                
                for(auto page : pages)
                {
                    line = PageToken + " ";
                    line += page->name + " ";
                    if(page->followMCP)
                        line += "FollowMCP ";
                    else
                        line += "FollowTCP ";
                    
                    if(page->trackColouring)
                        line += "UseTrackColoring ";
                    else
                        line += "NoTrackColoring ";

                    line += to_string(page->red) + " ";
                    line += to_string(page->green) + " ";
                    line += to_string(page->blue) + "\n";

                    iniFile << line;

                    for(auto surface : page->midiSurfaces)
                    {
                        line = MidiSurface_ + " ";
                        line += surface->name + " ";
                        line += to_string(surface->numChannels) + " ";
                        line += surface->isBankable ? "Bankable " : "NonBankable ";
                        line += to_string(surface->midiIn) + " " ;
                        line += to_string(surface->midiOut) + " " ;
                        line += surface->templateFilename + " ";
                        line += surface->actionTemplateFolder + " " ;
                        line += surface->FXTemplateFolder + "\n";
                        
                        iniFile << line;
                    }
                    
                    iniFile << "\n";
                }

                iniFile.close();
            }
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
