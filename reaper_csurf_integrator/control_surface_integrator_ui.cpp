//
//  control_surface_integrator_ui.cpp
//  reaper_csurf_integrator
//
//

#include "control_surface_integrator_ui.h"


#ifndef _WIN32
#include <dirent.h>

vector<string> GetDirectoryFiles(const string& dir)
{
    vector<string> files;
    shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
    struct dirent *dirent_ptr;
    
    while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
        files.push_back(string(dirent_ptr->d_name));
    
    return files;
}

vector<string> GetDirectoryFolders(const string& dir)
{
    vector<string> folders;
    shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
    struct dirent *dirent_ptr;
    
    while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
        if(dirent_ptr->d_type == DT_DIR)
            folders.push_back(string(dirent_ptr->d_name));
    
    return folders;
}


#else
#include <Windows.h>
#include <vector>

vector<string> GetDirectoryFiles(const string& directory)
{
    vector<string> files;
    WIN32_FIND_DATA fileData;
    HANDLE hFind;
    
    string fullPath = directory + "*.*";
    
    hFind = FindFirstFile(fullPath.c_str(), &fileData);

    if ( hFind != INVALID_HANDLE_VALUE)
    {
        files.push_back(fileData.cFileName);
        
        while (FindNextFile(hFind, &fileData))
            files.push_back(fileData.cFileName);
    }
    
    FindClose(hFind);
    return files;
}

vector<string> GetDirectoryFolders(const string& directory)
{
    vector<string> files;
    WIN32_FIND_DATA fileData;
    HANDLE hFind;
    
    string fullPath = directory + "*.*";
    
    hFind = FindFirstFile(fullPath.c_str(), &fileData);
    
    if ( hFind != INVALID_HANDLE_VALUE)
    {
        if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            files.push_back(fileData.cFileName);
        
        while (FindNextFile(hFind, &fileData))
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                files.push_back(fileData.cFileName);
    }
    
    FindClose(hFind);
    return files;
}
#endif


extern REAPER_PLUGIN_HINSTANCE g_hInst; 

static CSurfIntegrator* integrator = nullptr;

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
    descspace.Set("Control Surface Integrator");
    return descspace.Get();
}

const char *CSurfIntegrator::GetConfigString() // string of configuration data
{
    sprintf(configtmp,"0 0");
    return configtmp;
}

static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
    integrator = new CSurfIntegrator();
    return integrator;
}

static void parseParms(const char *str, int parms[5])
{
    parms[0]=0;
    parms[1]=9;
    parms[2]=parms[3]=-1;
    parms[4]=0;
    
    const char *p=str;
    if (p)
    {
        int x=0;
        while (x<5)
        {
            while (*p == ' ') p++;
            if ((*p < '0' || *p > '9') && *p != '-') break;
            parms[x++]=atoi(p);
            while (*p && *p != ' ') p++;
        }
    }
}


void FillCombo(HWND hwndDlg, int x, char * buf, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)buf);
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
}

void FillCombo(HWND hwndDlg, string buf, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)buf.c_str());
    //SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
}

void AddNoneToMIDIList(HWND hwndDlg, int comboId)
{
    int x=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)"None");
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,x,-1);
}

static int dlgResult = 0;
static char name[BUFSZ];

static WDL_DLGRET dlgProcLogicalSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
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
            char buf[BUFSZ];
            
            AddNoneToMIDIList(hwndDlg, IDC_COMBO_MidiIn);
            int n = GetNumMIDIInputs();
            for (int i = 0; i < n; i++)
                if (GetMIDIInputName(i, buf, sizeof(buf)))
                    FillCombo(hwndDlg, i, buf, IDC_COMBO_MidiIn);
            
            AddNoneToMIDIList(hwndDlg, IDC_COMBO_MidiOut);
            n = GetNumMIDIOutputs();
            for (int i = 0; i < n; i++)
                if (GetMIDIOutputName(i, buf, sizeof(buf)))
                    FillCombo(hwndDlg, i, buf, IDC_COMBO_MidiOut);
            
            string path(DAW::GetResourcePath());
            path += "/CSI/rst/";
            int i = 0;
            for(auto filename : GetDirectoryFiles(path))
            {
                if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'r' && filename[filename.length() - 2] == 's' &&filename[filename.length() - 1] == 't')
                {
                    strcpy(buf, filename.c_str());
                    FillCombo(hwndDlg, i++, buf, IDC_COMBO_SurfaceTemplate);
                }
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
            char buf[BUFSZ];
            
            for(auto* realSurface :  integrator->GetManager()->GetRealSurfaces())
                FillCombo(hwndDlg, realSurface->GetName(), IDC_COMBO_RealSurface);
            
            string resourcePath(DAW::GetResourcePath());
            resourcePath += "/CSI/";
            
            for(auto filename : GetDirectoryFolders(resourcePath + "axt/"))
            {
                if(filename[0] != '.')
                {
                    strcpy(buf, filename.c_str());
                    FillCombo(hwndDlg, buf, IDC_COMBO_ActionTemplates);
                }
            }

            for(auto filename : GetDirectoryFolders(resourcePath + "fxt/"))
            {
                if(filename[0] != '.')
                {
                    strcpy(buf, filename.c_str());
                    FillCombo(hwndDlg, buf, IDC_COMBO_FXTemplates);
                }
            }
        }
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int index = SendDlgItemMessage(hwndDlg, IDC_COMBO_RealSurface, CB_GETCURSEL, 0, 0);
                        if (index != CB_ERR)
                            strcpy(name, integrator->GetManager()->GetRealSurfaces()[index]->GetName().c_str());
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
                    case IDC_BUTTON_AddRealSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_RealSurface), hwndDlg, dlgProcRealSurface);
                            if(dlgResult == IDOK)
                                SendDlgItemMessage(hwndDlg, IDC_LIST_RealSurfaces, LB_ADDSTRING, 0, (LPARAM)name);
                        }
                        break ;
                        
                    case IDC_BUTTON_AddSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Surface), hwndDlg, dlgProcSurface);
                            if(dlgResult == IDOK)
                                SendDlgItemMessage(hwndDlg, IDC_LIST_Surfaces, LB_ADDSTRING, 0, (LPARAM)name);
                        }
                        break ;
                        
                    case IDC_BUTTON_AddSurfaceGroup:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_SurfaceGroup), hwndDlg, dlgProcSurfaceGroup);
                            if(dlgResult == IDOK)
                                SendDlgItemMessage(hwndDlg, IDC_LIST_SurfaceGroups, LB_ADDSTRING, 0, (LPARAM)name);
                        }
                        break ;
                        
                    case IDC_BUTTON_AddLogicalSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_LogicalSurface), hwndDlg, dlgProcLogicalSurface);
                            if(dlgResult == IDOK)
                                SendDlgItemMessage(hwndDlg, IDC_LIST_LogicalSurfaces, LB_ADDSTRING, 0, (LPARAM)name);
                        }
                        
                        break ;
                }
            }
            break ;
            

            
            
            
            
            
            
            
        case WM_INITDIALOG:
        {
            
        }
            
            break;
        
        case WM_USER+1024:
            if (wParam > 1 && lParam)
            {
                char tmp[512];
                
                int indev=-1, outdev=-1, offs=0, size=9;
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
                int cflags=0;
                
                
                //if (IsDlgButtonChecked(hwndDlg,IDC_CHECK1))
                //cflags|=CONFIG_FLAG_FADER_TOUCH_MODE;
                
                //if (IsDlgButtonChecked(hwndDlg,IDC_CHECK2))
                //{
                //cflags|=CONFIG_FLAG_MAPF1F8TOMARKERS;
                //}
                
                sprintf(tmp,"%d %d %d %d %d",offs,size,indev,outdev,cflags);
                lstrcpyn((char *)lParam, tmp,wParam);
                
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
    "Control Surface Integrator",
    createFunc,
    configFunc,
};
