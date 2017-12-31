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


void FillCombo(HWND hwndDlg, int parms, int x, char * buf, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)buf);
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
    if (x == parms) SendDlgItemMessage(hwndDlg,comboId,CB_SETCURSEL,a,0);
}

void AddNoneToMIDIList(HWND hwndDlg, int comboId)
{
    int x=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)"None");
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,x,-1);
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
            
            AddNoneToMIDIList(hwndDlg, IDC_COMBO1);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO2);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO3);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO4);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO5);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO6);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO7);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO8);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO9);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO10);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO11);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO12);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO13);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO14);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO15);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO16);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO17);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO18);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO19);
            AddNoneToMIDIList(hwndDlg, IDC_COMBO20);

            for (int x = 0; x < n; x ++)
            {
                char buf[512];
                if (GetMIDIInputName(x,buf,sizeof(buf)))
                {
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO1);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO2);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO3);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO4);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO5);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO6);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO7);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO8);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO9);
                    FillCombo(hwndDlg, parms[2], x, buf, IDC_COMBO10);
                }
            }
            
            n=GetNumMIDIOutputs();
            
            for (int x = 0; x < n; x ++)
            {
                char buf[512];
                if (GetMIDIOutputName(x,buf,sizeof(buf)))
                {
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO11);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO12);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO13);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO14);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO15);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO16);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO17);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO18);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO19);
                    FillCombo(hwndDlg, parms[3], x, buf, IDC_COMBO20);
                }
            }
            
            //SetDlgItemInt(hwndDlg,IDC_EDIT1,parms[0],TRUE);
            //SetDlgItemInt(hwndDlg,IDC_EDIT2,parms[1],FALSE);
            
            
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
    return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_SURFACEEDIT_CSI),parent,dlgProc,(LPARAM)initConfigString);
}

reaper_csurf_reg_t csurf_integrator_reg =
{
    "CSI",
    "Control Surface Integrator",
    createFunc,
    configFunc,
};
