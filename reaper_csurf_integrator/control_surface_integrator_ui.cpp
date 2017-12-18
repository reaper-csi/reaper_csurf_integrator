//
//  control_surface_integrator_ui.cpp
//  reaper_csurf_integrator
//
//

#include "control_surface_integrator_ui.h"

extern REAPER_PLUGIN_HINSTANCE g_hInst; 

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

bool CSurfIntegrator::GetTouchState(MediaTrack *trackid, int touchedControl)
{
    // GAW TBD
    
    return true;
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
    return new CSurfIntegrator();
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

static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            int parms[5];
            parseParms((const char *)lParam,parms);
            
            int n=GetNumMIDIInputs();
            int x=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"None");
            SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,x,-1);
            x=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)"None");
            SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,x,-1);
            for (x = 0; x < n; x ++)
            {
                char buf[512];
                if (GetMIDIInputName(x,buf,sizeof(buf)))
                {
                    int a=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)buf);
                    SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,a,x);
                    if (x == parms[2]) SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,a,0);
                }
            }
            n=GetNumMIDIOutputs();
            for (x = 0; x < n; x ++)
            {
                char buf[512];
                if (GetMIDIOutputName(x,buf,sizeof(buf)))
                {
                    int a=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)buf);
                    SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,a,x);
                    if (x == parms[3]) SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETCURSEL,a,0);
                }
            }
            SetDlgItemInt(hwndDlg,IDC_EDIT1,parms[0],TRUE);
            SetDlgItemInt(hwndDlg,IDC_EDIT2,parms[1],FALSE);
            
            
            //if (parms[4]&CONFIG_FLAG_FADER_TOUCH_MODE)
            //CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
            //if (parms[4]&CONFIG_FLAG_MAPF1F8TOMARKERS)
            //CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);
            
        }
            break;
        case WM_USER+1024:
            if (wParam > 1 && lParam)
            {
                char tmp[512];
                
                int indev=-1, outdev=-1, offs=0, size=9;
                int r=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
                if (r != CB_ERR) indev = SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETITEMDATA,r,0);
                r=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
                if (r != CB_ERR)  outdev = SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETITEMDATA,r,0);
                
                BOOL t;
                r=GetDlgItemInt(hwndDlg,IDC_EDIT1,&t,TRUE);
                if (t) offs=r;
                r=GetDlgItemInt(hwndDlg,IDC_EDIT2,&t,FALSE);
                if (t)
                {
                    if (r<1)r=1;
                    else if(r>256)r=256;
                    size=r;
                }
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
    //ShowMessageBox("Here ya go", "This Title", 0);
    
    //MessageBox(parent, "Here ya go", "New Title", 0);
    
   // return 0;
    
    
    //return CreateDialogParam(g_hInst,0,parent,dlgProc,(LPARAM)initConfigString);
    
    
    return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_SURFACEEDIT_MCU1),parent,dlgProc,(LPARAM)initConfigString);
}

reaper_csurf_reg_t csurf_integrator_reg =
{
    "CSI",
    "Control Surface Integrator",
    createFunc,
    configFunc,
};
