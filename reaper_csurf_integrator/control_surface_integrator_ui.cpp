//
//  control_surface_integrator_ui.cpp
//  reaper_csurf_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_integrator_ui.h"

extern REAPER_PLUGIN_HINSTANCE g_hInst;
Manager* TheManager = nullptr;

const string Control_Surface_Integrator = "Control Surface Integrator";

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
struct MidiSurfaceLine
{
    string name = "";
    int midiIn = 0;
    int midiOut = 0;
    string templateFilename = "";
    string zoneTemplateFolder = "";
    bool useZoneLink = false;
};

struct PageLine
{
    string name = "";
    bool followMCP = true;
    bool synchPages = false;
    bool trackColouring = false;
    int red = 0;
    int green = 0;
    int blue = 0;
    vector<MidiSurfaceLine*> midiSurfaces;
};

// Scratch pad to get in and out of dialogs easily
static bool editMode = false;
static int dlgResult = 0;

static int pageIndex = 0;

static char name[BUFSZ];
static int midiIn = 0;
static int midiOut = 0;
static char templateFilename[BUFSZ];
static char zoneTemplateFolder[BUFSZ];

static bool followMCP = true;
static bool synchPages = false;
static bool useZoneLink = false;
static bool trackColouring = false;

static vector<PageLine*> pages;

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
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SynchPages))
                            synchPages = true;
                        else
                            synchPages = false;
                        
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
            }
            else
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiIn), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_MidiOut), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceTemplate), CB_SETCURSEL, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ZoneTemplates), CB_SETCURSEL, 0, 0);
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

                        int currentSelection = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiIn, CB_GETCURSEL, 0, 0);
                        if (currentSelection >= 0)
                            midiIn = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiIn, CB_GETITEMDATA, currentSelection, 0);
                        currentSelection = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiOut, CB_GETCURSEL, 0, 0);
                        if (currentSelection >= 0)
                            midiOut = SendDlgItemMessage(hwndDlg, IDC_COMBO_MidiOut, CB_GETITEMDATA, currentSelection, 0);

                        GetDlgItemText(hwndDlg, IDC_COMBO_SurfaceTemplate, templateFilename, sizeof(templateFilename));
                        GetDlgItemText(hwndDlg, IDC_COMBO_ZoneTemplates, zoneTemplateFolder, sizeof(zoneTemplateFolder));

                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ZoneLink))
                            useZoneLink = true;
                        else
                            useZoneLink = false;
                       
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
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);

                                pageIndex = index;
                                
                                for(auto* surface : pages[index]->midiSurfaces)
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
                                    MidiSurfaceLine* surface = new MidiSurfaceLine();
                                    surface->name = name;
                                    surface->midiIn = midiIn;
                                    surface->midiOut = midiOut;
                                    surface->templateFilename = templateFilename;
                                    surface->zoneTemplateFolder = zoneTemplateFolder;
                                    surface->useZoneLink = useZoneLink;
                                    
                                    pages[pageIndex]->midiSurfaces.push_back(surface);
                                    
                                    AddListEntry(hwndDlg, name, IDC_LIST_Surfaces);
                                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_SETCURSEL,  pages[pageIndex]->midiSurfaces.size() - 1, 0);
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
                                trackColouring = pages[index]->trackColouring;
                                dlgResult = false;
                                editMode = true;
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Page), hwndDlg, dlgProcPage);
                                if(dlgResult == IDOK)
                                {
                                    pages[index]->name = name;
                                    pages[index]->followMCP = followMCP;
                                    pages[index]->synchPages = synchPages;
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
                                midiIn = pages[pageIndex]->midiSurfaces[index]->midiIn;
                                midiOut = pages[pageIndex]->midiSurfaces[index]->midiOut;
                                strcpy(templateFilename, pages[pageIndex]->midiSurfaces[index]->templateFilename.c_str());
                                strcpy(zoneTemplateFolder, pages[pageIndex]->midiSurfaces[index]->zoneTemplateFolder.c_str());
                                useZoneLink = pages[pageIndex]->midiSurfaces[index]->useZoneLink;
                                dlgResult = false;
                                editMode = true;
                                
                                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_MidiSurface), hwndDlg, dlgProcMidiSurface);
                                if(dlgResult == IDOK)
                                {
                                    pages[pageIndex]->midiSurfaces[index]->name = name;
                                    pages[pageIndex]->midiSurfaces[index]->midiIn = midiIn;
                                    pages[pageIndex]->midiSurfaces[index]->midiOut = midiOut;
                                    pages[pageIndex]->midiSurfaces[index]->templateFilename = templateFilename;
                                    pages[pageIndex]->midiSurfaces[index]->zoneTemplateFolder = zoneTemplateFolder;
                                    pages[pageIndex]->midiSurfaces[index]->useZoneLink = useZoneLink;
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
                                pages[pageIndex]->midiSurfaces.erase(pages[pageIndex]->midiSurfaces.begin() + index);
                                
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Surfaces), LB_RESETCONTENT, 0, 0);
                                for(auto* surface: pages[pageIndex]->midiSurfaces)
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
                if(line[0] != '\r' && line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens[0] == MidiInMonitorToken)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        if(tokens[1] == "On")
                            CheckDlgButton(hwndDlg, IDC_CHECK_MidiInMon, BST_CHECKED);
                    }
                    else if(tokens[0] == MidiOutMonitorToken)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        if(tokens[1] == "On")
                            CheckDlgButton(hwndDlg, IDC_CHECK_MidiOutMon, BST_CHECKED);
                    }
                    else if(tokens[0] == VSTMonitorToken)
                    {
                        if(tokens.size() != 2)
                            continue;
                        
                        if(tokens[1] == "On")
                            CheckDlgButton(hwndDlg, IDC_CHECK_VSTParamMon, BST_CHECKED);
                    }
                    else if(tokens[0] == PageToken)
                    {
                        if(tokens.size() != 8)
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
                        
                        if(tokens[4] == "UseTrackColoring")
                            page->trackColouring = true;
                        else
                            page->trackColouring = false;
                        
                        page->red = atoi(tokens[5].c_str());
                        page->green = atoi(tokens[6].c_str());
                        page->blue = atoi(tokens[7].c_str());
                        
                        pages.push_back(page);
                        
                        AddListEntry(hwndDlg, page->name, IDC_LIST_Pages);
                    }
                    else if(tokens[0] == MidiSurfaceToken)
                    {
                        if(tokens.size() != 7)
                            continue;
                        
                        MidiSurfaceLine* surface = new MidiSurfaceLine();
                        surface->name = tokens[1];
                        surface->midiIn = atoi(tokens[2].c_str());
                        surface->midiOut = atoi(tokens[3].c_str());
                        surface->templateFilename = tokens[4];
                        surface->zoneTemplateFolder = tokens[5];
                        surface->useZoneLink = tokens[6] == "UseZoneLink" ? true : false;
                        
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
                string line = MidiInMonitorToken + " ";
                if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_MidiInMon))
                    line += "On";
                else
                    line += "Off";
                iniFile << line + "\n";
                
                line = MidiOutMonitorToken + " ";
                if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_MidiOutMon))
                    line += "On";
                else
                    line += "Off";
                iniFile << line + "\n";
                
                line = VSTMonitorToken + " ";
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
                    
                    if(page->synchPages)
                        line += "SynchPages ";
                    else
                        line += "NoSynchPages ";
                    
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
                        line = MidiSurfaceToken + " ";
                        line += surface->name + " ";
                        line += to_string(surface->midiIn) + " " ;
                        line += to_string(surface->midiOut) + " " ;
                        line += surface->templateFilename + " ";
                        line += surface->zoneTemplateFolder + " " ;
                        line += surface->useZoneLink == true ? "UseZoneLink\n" : "NoZoneLink\n";
                        
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
