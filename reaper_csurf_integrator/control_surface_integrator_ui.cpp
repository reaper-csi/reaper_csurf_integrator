//
//  control_surface_integrator_ui.cpp
//  reaper_csurf_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_integrator_ui.h"

Manager* TheManager = nullptr;
extern string GetLineEnding();

const string Control_Surface_Integrator = "Control Surface Integrator";

extern int g_registered_command;

bool hookCommandProc(int command, int flag)
{
    if (g_registered_command && command == g_registered_command && TheManager != nullptr)
    {
        TheManager->OpenLearnModeWindow();
        return true;
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfIntegrator
////////////////////////////////////////////////////////////////////////////////////////////////////////
CSurfIntegrator::CSurfIntegrator()
{
    TheManager = new Manager(this);
}

CSurfIntegrator::~CSurfIntegrator()
{
    if(TheManager)
    {
        TheManager->Shutdown();
        delete TheManager;
        TheManager = nullptr;
    }
}

void CSurfIntegrator::OnTrackSelection(MediaTrack *trackid)
{
    if(TheManager)
        TheManager->OnTrackSelection(trackid);
}

void CSurfIntegrator::SetTrackListChange()
{
    if(TheManager)
        TheManager->OnTrackListChange();
}

int CSurfIntegrator::Extended(int call, void *parm1, void *parm2, void *parm3)
{
    if(call == CSURF_EXT_SUPPORTS_EXTENDED_TOUCH)
    {
        return 1;
    }
    
    if(call == CSURF_EXT_RESET)
    {
       if(TheManager)
           TheManager->Init();
    }
    
    if(call == CSURF_EXT_SETFXCHANGE)
    {
        // parm1=(MediaTrack*)track, whenever FX are added, deleted, or change order
        if(TheManager)
            TheManager->TrackFXListChanged((MediaTrack*)parm1);
    }
    
    if(call == CSURF_EXT_SETFOCUSEDFX)
    {
        // GAW TBD -- need to implement take FX and clear focus
        
        // Track FX focused
        if(parm2 == nullptr)
        {
            MediaTrack* track = (MediaTrack*)parm1;
            int fxIndex = parm3 == nullptr ? 0 : *(int*)parm3;
            TheManager->OnFXFocus(track, fxIndex);
        }
    }
    
    return 1;
}

bool CSurfIntegrator::GetTouchState(MediaTrack *track, int touchedControl)
{
    return TheManager->GetTouchState(track, touchedControl);
}

void CSurfIntegrator::Run()
{
    if(TheManager)
        TheManager->Run();
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
    snprintf(configtmp, sizeof(configtmp),"0 0");
    return configtmp;
}

static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
    return new CSurfIntegrator();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FileSystem
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    static vector<string> GetDirectoryFilenames(const string& dir)
    {
        vector<string> filenames;
        shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
        struct dirent *dirent_ptr;
        
        if(directory_ptr == nullptr)
            return filenames;
        
        while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
            filenames.push_back(string(dirent_ptr->d_name));
        
        return filenames;
    }
    
    static vector<string> GetDirectoryFolderNames(const string& dir)
    {
        vector<string> folderNames;
        shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
        struct dirent *dirent_ptr;
        
        if(directory_ptr == nullptr)
            return folderNames;
        
        while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
            if(dirent_ptr->d_type == DT_DIR)
                folderNames.push_back(string(dirent_ptr->d_name));
        
        return folderNames;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// structs
////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SurfaceLine
{
    string type = "";
    string name = "";
    int inPort = 0;
    int outPort = 0;
    string templateFilename = "";
    string zoneTemplateFolder = "";
    bool useZoneLink = false;
    bool autoMapSends = false;
    bool autoMapFX = false;
    bool autoMapFXMenu = false;
    bool autoMapFocusedFX = false;
    
    // for OSC
    string remoteDeviceIP = "";

    // for EuCon
    int numChannels = 0;
};

struct PageLine
{
    string name = "";
    bool followMCP = true;
    bool synchPages = false;
    bool useScrollLink = false;
    bool trackColouring = false;
    int red = 0;
    int green = 0;
    int blue = 0;
    vector<SurfaceLine*> surfaces;
};

// Scratch pad to get in and out of dialogs easily
static bool editMode = false;
static int dlgResult = 0;

static int pageIndex = 0;

static string type = "";
static char name[BUFSZ];
static int inPort = 0;
static int outPort = 0;
static char remoteDeviceIP[BUFSZ];
static char templateFilename[BUFSZ];
static char zoneTemplateFolder[BUFSZ];
// for EuCon
int numChannels = 0;

static bool followMCP = true;
static bool synchPages = false;
static bool trackColouring = false;
static bool useScrollLink = false;

static bool useZoneLink = false;
static bool autoMapSends = false;
static bool autoMapFX = false;
static bool autoMapFXMenu = false;
static bool autoMapFocusedFX = false;

static vector<PageLine*> pages;

static void ModifyEuConZoneFile(int numChannels)
{
    string channelStr = "1-" + to_string(numChannels);
    
    vector<string> inputLines;
    vector<string> outputLines;

    ifstream euconInputZoneFile(string(DAW::GetResourcePath()) + "/CSI/Zones/EuCon/EuCon.zon");

    for (string line; getline(euconInputZoneFile, line) ; )
        inputLines.push_back(line);
    
    for(auto inputLine : inputLines)
    {
        size_t zonePos = inputLine.find("Zone");
        size_t channelPos = inputLine.find("Channel|");

        string outputLine;
        
        if(channelPos != string::npos && zonePos == string::npos)
            outputLine = inputLine.substr(0, channelPos + strlen("Channel|")) + channelStr + "\"";
        else
            outputLine = inputLine;

        outputLines.push_back(outputLine);
    }
    
    euconInputZoneFile.close();
    
    ofstream euconOutputZoneFile(string(DAW::GetResourcePath()) + "/CSI/Zones/EuCon/EuCon.zon");
    
    for(auto outputLine : outputLines)
        euconOutputZoneFile << outputLine + "\n";
    
    euconOutputZoneFile.close();
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

static WDL_DLGRET dlgProcPage(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if(editMode)
            {
                editMode = false;
                
                SetDlgItemText(hwndDlg, IDC_EDIT_PageName, name);
                
                if(followMCP)
                    CheckDlgButton(hwndDlg, IDC_RADIO_MCP, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_RADIO_TCP, BST_CHECKED);
                
                if(synchPages)
                    CheckDlgButton(hwndDlg, IDC_CHECK_SynchPages, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_SynchPages, BST_UNCHECKED);
                
                if(useScrollLink)
                    CheckDlgButton(hwndDlg, IDC_CHECK_ScrollLink, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_ScrollLink, BST_UNCHECKED);
                
                if(trackColouring)
                    CheckDlgButton(hwndDlg, IDC_CHECK_ColourTracks, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_ColourTracks, BST_UNCHECKED);
            }
            else
            {
                CheckDlgButton(hwndDlg, IDC_RADIO_TCP, BST_CHECKED);
            }
        }
            break;
            
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
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SynchPages))
                            synchPages = true;
                        else
                            synchPages = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ScrollLink))
                            useScrollLink = true;
                        else
                            useScrollLink = false;
                        
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
                    if(editMode && inPort == i)
                        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiIn), CB_SETCURSEL, currentIndex, 0);
                    currentIndex++;
                }
            
            currentIndex = 0;
            
            for (int i = 0; i < GetNumMIDIOutputs(); i++)
                if (GetMIDIOutputName(i, buf, sizeof(buf)))
                {
                    AddComboEntry(hwndDlg, i, buf, IDC_COMBO_MidiOut);
                    if(editMode && outPort == i)
                        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiOut), CB_SETCURSEL, currentIndex, 0);
                    currentIndex++;
                }
            
            string resourcePath(DAW::GetResourcePath());
            
            int i = 0;
            for(auto filename : FileSystem::GetDirectoryFilenames(resourcePath + "/CSI/Surfaces/Midi/"))
            {
                int length = filename.length();
                if(length > 4 && filename[0] != '.' && filename[length - 4] == '.' && filename[length - 3] == 'm' && filename[length - 2] == 's' &&filename[length - 1] == 't')
                    AddComboEntry(hwndDlg, i++, (char*)filename.c_str(), IDC_COMBO_SurfaceTemplate);
            }
            
            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "/CSI/Zones/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_ZoneTemplates);
            
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceName, name);
                
                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_FINDSTRING, -1, (LPARAM)templateFilename);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, index, 0);
                
                index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_FINDSTRING, -1, (LPARAM)zoneTemplateFolder);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, index, 0);
                
                if(useZoneLink)
                    CheckDlgButton(hwndDlg, IDC_CHECK_ZoneLink, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_ZoneLink, BST_UNCHECKED);
                
                if(autoMapSends)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapSends, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapSends, BST_UNCHECKED);
                
                if(autoMapFX)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFX, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFX, BST_UNCHECKED);
                
                if(autoMapFXMenu)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFXMenu, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFXMenu, BST_UNCHECKED);
                
                if(autoMapFocusedFX)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFocusedFX, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFocusedFX, BST_UNCHECKED);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceName, "");
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiIn), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiOut), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, 0, 0);
            }
        }
            break;
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_COMBO_SurfaceTemplate:
                {
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                char buffer[BUFSZ];
                                
                                GetDlgItemText(hwndDlg, IDC_COMBO_SurfaceTemplate, buffer, sizeof(buffer));

                                for(int i = 0; i < sizeof(buffer); i++)
                                {
                                    if(buffer[i] == '.')
                                    {
                                        buffer[i] = 0;
                                        break;
                                    }
                                }
                                
                                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_FINDSTRINGEXACT, -1, (LPARAM)buffer);
                                if(index >= 0)
                                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, index, 0);
                            }
                        }
                    }
                    
                    break;
                }

                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_MidiSurfaceName, name, sizeof(name));
                        
                        int currentSelection = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiIn, CB_GETCURSEL, 0, 0);
                        if (currentSelection >= 0)
                            inPort = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiIn, CB_GETITEMDATA, currentSelection, 0);
                        currentSelection = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiOut, CB_GETCURSEL, 0, 0);
                        if (currentSelection >= 0)
                            outPort = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiOut, CB_GETITEMDATA, currentSelection, 0);
                        
                        GetDlgItemText(hwndDlg, IDC_COMBO_SurfaceTemplate, templateFilename, sizeof(templateFilename));
                        GetDlgItemText(hwndDlg, IDC_COMBO_ZoneTemplates, zoneTemplateFolder, sizeof(zoneTemplateFolder));
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ZoneLink))
                            useZoneLink = true;
                        else
                            useZoneLink = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapSends))
                            autoMapSends = true;
                        else
                            autoMapSends = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapFX))
                            autoMapFX = true;
                        else
                            autoMapFX = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapFXMenu))
                            autoMapFXMenu = true;
                        else
                            autoMapFXMenu = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapFocusedFX))
                            autoMapFocusedFX = true;
                        else
                            autoMapFocusedFX = false;
                        
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
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcOSCSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            string resourcePath(DAW::GetResourcePath());
            
            int i = 0;
            for(auto filename : FileSystem::GetDirectoryFilenames(resourcePath + "/CSI/Surfaces/OSC/"))
            {
                int length = filename.length();
                if(length > 4 && filename[0] != '.' && filename[length - 4] == '.' && filename[length - 3] == 'o' && filename[length - 2] == 's' &&filename[length - 1] == 't')
                    AddComboEntry(hwndDlg, i++, (char*)filename.c_str(), IDC_COMBO_SurfaceTemplate);
            }
            
            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "/CSI/Zones/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_ZoneTemplates);
            
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCSurfaceName, name);
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCRemoteDeviceIP, remoteDeviceIP);
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCInPort, to_string(inPort).c_str());
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCOutPort, to_string(outPort).c_str());
                
                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_FINDSTRING, -1, (LPARAM)templateFilename);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, index, 0);
                
                index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_FINDSTRING, -1, (LPARAM)zoneTemplateFolder);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, index, 0);
                
                if(useZoneLink)
                    CheckDlgButton(hwndDlg, IDC_CHECK_ZoneLink, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_ZoneLink, BST_UNCHECKED);
                
                if(autoMapSends)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapSends, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapSends, BST_UNCHECKED);
                
                if(autoMapFX)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFX, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFX, BST_UNCHECKED);
                
                if(autoMapFXMenu)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFXMenu, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFXMenu, BST_UNCHECKED);
                
                if(autoMapFocusedFX)
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFocusedFX, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_AutoMapFocusedFX, BST_UNCHECKED);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCSurfaceName, "");
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCRemoteDeviceIP, "");
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCInPort, "");
                SetDlgItemText(hwndDlg, IDC_EDIT_OSCOutPort, "");
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, 0, 0);
            }
        }
            break;
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_COMBO_SurfaceTemplate:
                {
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                char buffer[BUFSZ];
                                
                                GetDlgItemText(hwndDlg, IDC_COMBO_SurfaceTemplate, buffer, sizeof(buffer));
                                
                                for(int i = 0; i < sizeof(buffer); i++)
                                {
                                    if(buffer[i] == '.')
                                    {
                                        buffer[i] = 0;
                                        break;
                                    }
                                }
                                
                                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_FINDSTRINGEXACT, -1, (LPARAM)buffer);
                                if(index >= 0)
                                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, index, 0);
                            }
                        }
                    }
                    
                    break;
                }
                    
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_OSCSurfaceName, name, sizeof(name));
                        GetDlgItemText(hwndDlg, IDC_EDIT_OSCRemoteDeviceIP, remoteDeviceIP, sizeof(remoteDeviceIP));
                        
                        char buf[BUFSZ];
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_OSCInPort, buf, sizeof(buf));
                        inPort = atoi(buf);
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_OSCOutPort, buf, sizeof(buf));
                        outPort = atoi(buf);
                        
                        GetDlgItemText(hwndDlg, IDC_COMBO_SurfaceTemplate, templateFilename, sizeof(templateFilename));
                        GetDlgItemText(hwndDlg, IDC_COMBO_ZoneTemplates, zoneTemplateFolder, sizeof(zoneTemplateFolder));
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ZoneLink))
                            useZoneLink = true;
                        else
                            useZoneLink = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapSends))
                            autoMapSends = true;
                        else
                            autoMapSends = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapFX))
                            autoMapFX = true;
                        else
                            autoMapFX = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapFXMenu))
                            autoMapFXMenu = true;
                        else
                            autoMapFXMenu = false;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_AutoMapFocusedFX))
                            autoMapFocusedFX = true;
                        else
                            autoMapFocusedFX = false;
                        
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
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcEuConSurface(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            string resourcePath(DAW::GetResourcePath());
            for(auto foldername : FileSystem::GetDirectoryFolderNames(resourcePath + "/CSI/Zones/"))
                if(foldername[0] != '.')
                    AddComboEntry(hwndDlg, 0, (char *)foldername.c_str(), IDC_COMBO_ZoneTemplates);
            
            if(editMode)
            {
                editMode = false;
                SetDlgItemText(hwndDlg, IDC_EDIT_EuConSurfaceName, name);
                SetDlgItemText(hwndDlg, IDC_EDIT_NumChannels, to_string(numChannels).c_str());

                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_FINDSTRING, -1, (LPARAM)zoneTemplateFolder);
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, index, 0);
                
                if(useZoneLink)
                    CheckDlgButton(hwndDlg, IDC_CHECK_ZoneLink, BST_CHECKED);
                else
                    CheckDlgButton(hwndDlg, IDC_CHECK_ZoneLink, BST_UNCHECKED);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_EDIT_EuConSurfaceName, "EuCon");
                SetDlgItemText(hwndDlg, IDC_EDIT_NumChannels, "8");
                
                int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_FINDSTRING, -1, (LPARAM)"EuCon");
                if(index >= 0)
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, index, 0);
            }
        }
            break;
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_EuConSurfaceName, name, sizeof(name));

                        char buf[BUFSZ];
                        
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_NumChannels, buf, sizeof(buf));
                        numChannels = atoi(buf);
                        
                       
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ZoneLink))
                            useZoneLink = true;
                        else
                            useZoneLink = false;
                        
                        GetDlgItemText(hwndDlg, IDC_COMBO_ZoneTemplates, zoneTemplateFolder, sizeof(zoneTemplateFolder));

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
                    case IDC_LIST_Pages:
                        if (HIWORD(wParam) == LBN_SELCHANGE)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);

                                pageIndex = index;
                                
                                for(auto* surface : pages[index]->surfaces)
                                    AddListEntry(hwndDlg, surface->name, IDC_LIST_Surfaces);
                            }
                            else
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                            }
                        }
                        break;
                        
                    case IDC_BUTTON_AddPage:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            dlgResult = false;
                            followMCP = true;
                            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Page), hwndDlg, dlgProcPage);
                            if(dlgResult == IDOK)
                            {
                                PageLine* page = new PageLine();
                                page->name = name;
                                page->followMCP = followMCP;
                                page->synchPages = synchPages;
                                page->useScrollLink = useScrollLink;
                                page->trackColouring = trackColouring;
                                pages.push_back(page);
                                AddListEntry(hwndDlg, name, IDC_LIST_Pages);
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_SETCURSEL, pages.size() - 1, 0);
                            }
                        }
                        break ;
                        
                    case IDC_BUTTON_AddMidiSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                dlgResult = false;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface), hwndDlg, dlgProcMidiSurface);
                                if(dlgResult == IDOK)
                                {
                                    SurfaceLine* surface = new SurfaceLine();
                                    surface->type = MidiSurfaceToken;
                                    surface->name = name;
                                    surface->inPort = inPort;
                                    surface->outPort = outPort;
                                    surface->templateFilename = templateFilename;
                                    surface->zoneTemplateFolder = zoneTemplateFolder;
                                    surface->useZoneLink = useZoneLink;
                                    surface->autoMapSends = autoMapSends;
                                    surface->autoMapFX = autoMapFX;
                                    surface->autoMapFXMenu = autoMapFXMenu;
                                    surface->autoMapFocusedFX = autoMapFocusedFX;

                                    pages[pageIndex]->surfaces.push_back(surface);
                                    
                                    AddListEntry(hwndDlg, name, IDC_LIST_Surfaces);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL,  pages[pageIndex]->surfaces.size() - 1, 0);
                                }
                            }
                        }
                        break ;
                        
                    case IDC_BUTTON_AddOSCSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                dlgResult = false;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface1), hwndDlg, dlgProcOSCSurface);
                                if(dlgResult == IDOK)
                                {
                                    SurfaceLine* surface = new SurfaceLine();
                                    surface->type = OSCSurfaceToken;
                                    surface->name = name;
                                    surface->remoteDeviceIP = remoteDeviceIP;
                                    surface->inPort = inPort;
                                    surface->outPort = outPort;
                                    surface->templateFilename = templateFilename;
                                    surface->zoneTemplateFolder = zoneTemplateFolder;
                                    surface->useZoneLink = useZoneLink;
                                    surface->autoMapSends = autoMapSends;
                                    surface->autoMapFX = autoMapFX;
                                    surface->autoMapFXMenu = autoMapFXMenu;
                                    surface->autoMapFocusedFX = autoMapFocusedFX;
                                    
                                    pages[pageIndex]->surfaces.push_back(surface);
                                    
                                    AddListEntry(hwndDlg, name, IDC_LIST_Surfaces);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL,  pages[pageIndex]->surfaces.size() - 1, 0);
                                }
                            }
                        }
                        break ;
                        
                    case IDC_BUTTON_AddEuConSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Pages, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                dlgResult = false;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface2), hwndDlg, dlgProcEuConSurface);
                                if(dlgResult == IDOK)
                                {
                                    SurfaceLine* surface = new SurfaceLine();
                                    surface->type = EuConSurfaceToken;
                                    surface->name = name;
                                    surface->zoneTemplateFolder = zoneTemplateFolder;
                                    surface->numChannels = numChannels;
                                    surface->useZoneLink = useZoneLink;

                                    pages[pageIndex]->surfaces.push_back(surface);
                                    
                                    AddListEntry(hwndDlg, name, IDC_LIST_Surfaces);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL,  pages[pageIndex]->surfaces.size() - 1, 0);
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
                                synchPages = pages[index]->synchPages;
                                useScrollLink = pages[index]->useScrollLink;
                                trackColouring = pages[index]->trackColouring;
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Page), hwndDlg, dlgProcPage);
                                if(dlgResult == IDOK)
                                {
                                    pages[index]->name = name;
                                    pages[index]->followMCP = followMCP;
                                    pages[index]->synchPages = synchPages;
                                    pages[index]->useScrollLink = useScrollLink;
                                    pages[index]->trackColouring = trackColouring;
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_RESETCONTENT, 0, 0);
                                    for(auto* page :  pages)
                                        AddListEntry(hwndDlg, page->name, IDC_LIST_Pages);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Pages), LB_SETCURSEL, index, 0);
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
                                inPort = pages[pageIndex]->surfaces[index]->inPort;
                                outPort = pages[pageIndex]->surfaces[index]->outPort;
                                strcpy(remoteDeviceIP, pages[pageIndex]->surfaces[index]->remoteDeviceIP.c_str());
                                strcpy(templateFilename, pages[pageIndex]->surfaces[index]->templateFilename.c_str());
                                strcpy(zoneTemplateFolder, pages[pageIndex]->surfaces[index]->zoneTemplateFolder.c_str());
                                useZoneLink = pages[pageIndex]->surfaces[index]->useZoneLink;
                                autoMapSends = pages[pageIndex]->surfaces[index]->autoMapSends;
                                autoMapFX = pages[pageIndex]->surfaces[index]->autoMapFX;
                                autoMapFXMenu = pages[pageIndex]->surfaces[index]->autoMapFXMenu;
                                autoMapFocusedFX = pages[pageIndex]->surfaces[index]->autoMapFocusedFX;
                                
                                // for EuCon
                                numChannels = pages[pageIndex]->surfaces[index]->numChannels;
                                
                                dlgResult = false;
                                editMode = true;
                                
                                if(pages[pageIndex]->surfaces[index]->type == MidiSurfaceToken)
                                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface), hwndDlg, dlgProcMidiSurface);
                                else if(pages[pageIndex]->surfaces[index]->type == OSCSurfaceToken)
                                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface1), hwndDlg, dlgProcOSCSurface);
                                else if(pages[pageIndex]->surfaces[index]->type == EuConSurfaceToken)
                                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface2), hwndDlg, dlgProcEuConSurface);
                                
                                if(dlgResult == IDOK)
                                {
                                    pages[pageIndex]->surfaces[index]->name = name;
                                    pages[pageIndex]->surfaces[index]->remoteDeviceIP = remoteDeviceIP;
                                    pages[pageIndex]->surfaces[index]->inPort = inPort;
                                    pages[pageIndex]->surfaces[index]->outPort = outPort;
                                    pages[pageIndex]->surfaces[index]->templateFilename = templateFilename;
                                    pages[pageIndex]->surfaces[index]->zoneTemplateFolder = zoneTemplateFolder;
                                    pages[pageIndex]->surfaces[index]->useZoneLink = useZoneLink;
                                    pages[pageIndex]->surfaces[index]->autoMapSends = autoMapSends;
                                    pages[pageIndex]->surfaces[index]->autoMapFX =autoMapFX;
                                    pages[pageIndex]->surfaces[index]->autoMapFXMenu = autoMapFXMenu;
                                    pages[pageIndex]->surfaces[index]->autoMapFocusedFX = autoMapFocusedFX;
                                    
                                    // for EuCon
                                    pages[pageIndex]->surfaces[index]->numChannels = numChannels;
                                }
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

                    case IDC_BUTTON_RemoveSurface:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Surfaces, LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                pages[pageIndex]->surfaces.erase(pages[pageIndex]->surfaces.begin() + index);
                                
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                                for(auto* surface: pages[pageIndex]->surfaces)
                                    AddListEntry(hwndDlg, surface->name, IDC_LIST_Surfaces);
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL, index, 0);
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
                line = regex_replace(line, regex(TabChars), " ");
                line = regex_replace(line, regex(CRLFChars), "");
                                     
                if(line[0] != '\r' && line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens[0] == PageToken)
                    {
                        if(tokens.size() != 11)
                            continue;
 
                        PageLine* page = new PageLine();
                        page->name = tokens[1];
                        
                        if(tokens[2] == "FollowMCP")
                            page->followMCP = true;
                        else
                            page->followMCP = false;
                        
                        if(tokens[3] == "SynchPages")
                            page->synchPages = true;
                        else
                            page->synchPages = false;
                        
                        if(tokens[4] == "UseScrollLink")
                            page->useScrollLink = true;
                        else
                            page->useScrollLink = false;
                        
                        if(tokens[5] == "UseTrackColoring")
                            page->trackColouring = true;
                        else
                            page->trackColouring = false;
                        
                        page->red = atoi(tokens[7].c_str());
                        page->green = atoi(tokens[8].c_str());
                        page->blue = atoi(tokens[9].c_str());
                        
                        pages.push_back(page);
                        
                        AddListEntry(hwndDlg, page->name, IDC_LIST_Pages);
                    }
                    else if(tokens[0] == MidiSurfaceToken || tokens[0] == OSCSurfaceToken || tokens[0] == EuConSurfaceToken)
                    {
                        SurfaceLine* surface = new SurfaceLine();
                        surface->type = tokens[0];
                        surface->name = tokens[1];
                        
                        if((surface->type == MidiSurfaceToken || surface->type == OSCSurfaceToken) && (tokens.size() == 11 || tokens.size() == 12))
                        {
                            surface->inPort = atoi(tokens[2].c_str());
                            surface->outPort = atoi(tokens[3].c_str());
                            surface->templateFilename = tokens[4];
                            surface->zoneTemplateFolder = tokens[5];
                            surface->useZoneLink = tokens[6] == "UseZoneLink" ? true : false;
                            surface->autoMapSends = tokens[7] == "AutoMapSends" ? true : false;
                            surface->autoMapFX = tokens[8] == "AutoMapFX" ? true : false;
                            surface->autoMapFXMenu = tokens[9] == "AutoMapFXMenu" ? true : false;
                            surface->autoMapFocusedFX = tokens[10] == "AutoMapFocusedFX" ? true : false;

                            if(tokens[0] == OSCSurfaceToken && tokens.size() == 12)
                                surface->remoteDeviceIP = tokens[11];
                        }
                        else if(surface->type == EuConSurfaceToken && tokens.size() == 5 )
                        {
                            surface->numChannels = atoi(tokens[2].c_str());
                            surface->zoneTemplateFolder = tokens[3];
                            surface->useZoneLink = tokens[4] == "UseZoneLink" ? true : false;
                         }
                        
                        if(pages.size() > 0)
                            pages[pages.size() - 1]->surfaces.push_back(surface);
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
                string line = "";
                
                for(auto page : pages)
                {
                    line = PageToken + " ";
                    line += "\"" + page->name + "\" ";
                    if(page->followMCP)
                        line += "FollowMCP ";
                    else
                        line += "FollowTCP ";
                    
                    if(page->synchPages)
                        line += "SynchPages ";
                    else
                        line += "NoSynchPages ";
                    
                    if(page->useScrollLink)
                        line += "UseScrollLink ";
                    else
                        line += "NoScrollLink ";
                    
                    if(page->trackColouring)
                        line += "UseTrackColoring ";
                    else
                        line += "NoTrackColoring ";
                    
                    line += "{ ";
                    
                    line += to_string(page->red) + " ";
                    line += to_string(page->green) + " ";
                    line += to_string(page->blue);

                    line += " }" + GetLineEnding();

                    iniFile << line;

                    for(auto surface : page->surfaces)
                    {
                        line = surface->type + " ";
                        line += "\"" + surface->name + "\" ";
                        
                        if(surface->type == MidiSurfaceToken || surface->type == OSCSurfaceToken)
                        {
                            line += to_string(surface->inPort) + " " ;
                            line += to_string(surface->outPort) + " " ;
                            line += "\"" + surface->templateFilename + "\" ";
                            line += "\"" + surface->zoneTemplateFolder + "\" ";
                            line += surface->useZoneLink == true ? "UseZoneLink " : "NoZoneLink ";
                            line += surface->autoMapSends == true ? "AutoMapSends " : "NoAutoMapSends ";
                            line += surface->autoMapFX == true ? "AutoMapFX " : "NoAutoMapFX ";
                            line += surface->autoMapFXMenu == true ? "AutoMapFXMenu " : "NoAutoMapFXMenu ";
                            line += surface->autoMapFocusedFX == true ? "AutoMapFocusedFX " : "NoAutoMapFocusedFX ";
                            
                            if(surface->type == OSCSurfaceToken)
                                line += " " + surface->remoteDeviceIP;
                        }
                        else if(surface->type == EuConSurfaceToken)
                        {
                            line += to_string(surface->numChannels) + " " ;
                            line += "\"" + surface->zoneTemplateFolder + "\" ";
                            line += surface->useZoneLink == true ? "UseZoneLink " : "NoZoneLink ";
                            
                            ModifyEuConZoneFile( surface->numChannels);
                        }

                        line += GetLineEnding();
                        
                        iniFile << line;
                    }
                    
                    iniFile << GetLineEnding();
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
