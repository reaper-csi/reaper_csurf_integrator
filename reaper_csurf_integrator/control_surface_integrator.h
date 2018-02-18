//
//  control_surface_integrator.h
//  reaper_control_surface_integrator
//
//

// Note for Windows environments:
//  use std::byte for C++17 byte
//  use ::byte for Windows byte

#ifndef control_surface_integrator
#define control_surface_integrator

#include "time.h"
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <fstream>

#include "control_surface_integrator_Reaper.h"

#ifndef _WIN32
#include <dirent.h>
#else
#include <memory>
#include "direntWin.h"
#endif

const string Control_Surface_Integrator = "Control Surface Integrator";
const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";
const string DefaultGUID = "Control Surface Integrator GUID";
const string GainReductionDB = "GainReductionDB";
const string TrackOnSelection = "TrackOnSelection";
const string Channel = "Channel";
const string ChannelEnd = "ChannelEnd";
const string MidiInMonitor = "MidiInMonitor";
const string MidiOutMonitor = "MidiOutMonitor";
const string VSTMonitor = "VSTMonitor";
const string RealSurface_ = "RealSurface";
const string Layout_ = "Layout";
const string Zone_ = "Zone";
const string VirtualSurface_ = "VirtualSurface";

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
        
        while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
            filenames.push_back(string(dirent_ptr->d_name));
        
        return filenames;
    }

    static vector<string> GetDirectoryFolderNames(const string& dir)
    {
        vector<string> folderNames;
        shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
        struct dirent *dirent_ptr;
        
        while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
            if(dirent_ptr->d_type == DT_DIR)
                folderNames.push_back(string(dirent_ptr->d_name));
        
        return folderNames;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                        THE RULES
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// An ActionAddress allows a widget to access a particular action - e.g. "{ GUID }Zone1Mixer1Fader"
// ActionAddress format = GUID + zoneName + realSurfaceName + modifiers + widgetName
// Modifiers can be ""
//

// Modifiers
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
// Combos allowed -- ShiftControl -- OK
// Dups disallowed -- ShiftShift -- no good
//
// Modifier Order matters !!
// Please do not modify Zone::CurrentModifiers()
//
// Allowed -- ShiftControl -- OK
// Disallowed -- ControlShift -- no good
//
// Modifier Order matters !!
// Please do not modify Zone::CurrentModifiers()
//
// The modifiers, if present:
//  must be contained in the modifier part of the action address
//  must be contained only in the modifier part of the action address
//  in the case of combos, must be in the same order as listed above -- e.g. "{ GUID }Zone1Mixer1ShiftOptionControlAltFader" for the full meal deal
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                        THE RULES
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structs
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXWindow
{
    MediaTrack* track = nullptr;;
    string fxGUID = "";
    
    FXWindow(MediaTrack* aTrack, string anFxGUID) : track(aTrack), fxGUID(anFxGUID) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXTemplateEntry
{
    string widgetName;
    string paramName;
    
    FXTemplateEntry(string aWidgetName, string aParamName) : widgetName(aWidgetName), paramName(aParamName) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXTemplate
{
private:
    string name;
    vector<FXTemplateEntry> entries_;
    
public:
    FXTemplate(string aName) : name(aName) {}
    
    string GetName() { return name; }
    vector<FXTemplateEntry>& GetTemplateEntries() { return entries_; }
    
    void AddEntry(string widgetName, string paramName)
    {
        entries_.push_back(FXTemplateEntry(widgetName, paramName));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Layout;
class Zone;
class RealSurface;
class RealSurfaceChannel;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Layout* layout_ = nullptr;
    Layout* GetLayout() { return layout_; }
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) {}
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, string value) {}
    
public:
    Action(Layout* layout) : layout_(layout) {}
    virtual ~Action() {}
    
    virtual int GetDisplayMode() { return 0; }
    virtual double GetCurrentNormalizedValue (string groupName, string surfaceName, string widgetName) { return 0.0; }
    
    virtual void AddAction(Action* action) {}
    virtual void Update(string zoneName, string surfaceName, string widgetName) {}
    virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) {}
    virtual void Cycle(string zoneName, string surfaceName, string widgetName) {}
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string GUID_ = DefaultGUID;
    RealSurface* realSurface_ = nullptr;
    string suffix_= "";
    string name_ = "";
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    RealSurface* GetRealSurface() { return realSurface_; }
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
public:
    MidiWidget(RealSurface* surface, string name, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : realSurface_(surface), name_(name),  midiPressMessage_(press), midiReleaseMessage_(release) {}
    virtual ~MidiWidget() {};
    
    string GetGUID() { return GUID_; }
    virtual double GetMinDB() { return 0.0; }
    virtual double GetMaxDB() { return 0.0; }
    string GetName() { return name_ + suffix_; }
    string GetActionName() { return name_; }

    void SetSuffix(string suffix)
    {
        suffix_ = suffix;
    }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
        
        if(GUID_ == "")
        {
            SetValueToZero();
            ForceUpdate();
        }
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
    virtual void AddToRealSurface(RealSurface* surface);
    void Update();
    void ForceUpdate();
    virtual void SetValue(double value) {}
    virtual void SetValue(double value, int displaymode) {}
    virtual void SetValue(string value) {}
    virtual void SetValueToZero() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    const string name_ = "";
    string templateFilename_ = "";
    Zone* zone_ = nullptr;
    bool isBankable_ = true;
    
    vector<RealSurfaceChannel*> channels_;
    vector<RealSurfaceChannel*> emptyChannels_;
    map<string, MidiWidget*> widgetsByName_;
    map<string, MidiWidget*> widgetsByMessage_;
    map<string, string> remappedFXWidgets_;
    
    bool zoom_ = false;
    bool scrub_ = false;
    
    RealSurface(const string name, string templateFilename, int numChannels, int numBankableChannels); 

public:
    virtual ~RealSurface() {};
    
    const string GetName() const { return name_; }
    string GetTemplateFilename() const { return templateFilename_; }
    Zone* GetZone() { return zone_; }
    vector<RealSurfaceChannel*> & GetChannels() { return channels_; }
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }
    
    string GetWidgetGUID(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetGUID();
        
        return "";
    }
    
    vector<RealSurfaceChannel*> & GetBankableChannels()
    {
        if(isBankable_)
            return GetChannels();
        else
            return emptyChannels_;
    }
    
    void AddAction(string actionAddress, Action* action);
    void MapTrackToWidgets(MediaTrack *track);
    void UnmapWidgetsFromTrack(MediaTrack *track);
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    virtual void SendMidiMessage(int first, int second, int third) {}
    
    virtual void RunAndUpdate() {}

    void UnmapFXFromWidgets(MediaTrack *track)
    {
        for(auto [widgetName, GUID] : remappedFXWidgets_)
            if(widgetsByName_.count(widgetName) > 0)
                widgetsByName_[widgetName]->SetGUID(GUID);
        
        remappedFXWidgets_.clear();
    }
    
    void SetZone(Zone* zone)
    {
        zone_ = zone;
    }
    
    void AddWidget(MidiWidget* widget)
    {
        widget->AddToRealSurface(this);
    }
    
    void AddWidgetToNameMap(string name, MidiWidget* widget)
    {
        widgetsByName_[name] = widget;
    }
    
    void AddWidgetToMessageMap(string message, MidiWidget* widget)
    {
        widgetsByMessage_[message] = widget;
    }

    void SetWidgetGUID(string widgetName, string GUID)
    {
        if(remappedFXWidgets_.count(widgetName) > 0)
            remappedFXWidgets_[widgetName] = GUID;
        else if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetGUID(GUID);
    }
    
    void SetWidgetFXGUID(string widgetName, string GUID)
    {
        if(widgetsByName_.count(widgetName) > 0)
        {
            remappedFXWidgets_[widgetName] = widgetsByName_[widgetName]->GetGUID();
            widgetsByName_[widgetName]->SetGUID(GUID);
        }
    }
    
    void SetZoom(bool value)
    {
        zoom_ = value;
        ForceUpdateWidgets();
    }
    
    void SetScrub(bool value)
    {
        scrub_ = value;
        ForceUpdateWidgets();
    }
    
    // to Widgets ->
    virtual void UpdateWidgets()
    {
        for(auto const& [name, widget] : widgetsByName_ )
            widget->Update();
    }

    virtual void ForceUpdateWidgets()
    {
        for(auto const& [name, widget] : widgetsByName_ )
            widget->ForceUpdate();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string surfaceName, string actionName, string widgetName);
    void UpdateAction(string surfaceName, string actionName, string widgetName);
    void ForceUpdateAction(string surfaceName, string actionName, string widgetName);
    void CycleAction(string surfaceName, string actionName, string widgetName);
    void DoAction(string surfaceName, string actionName, string widgetName, double value);
    
    // to Widgets ->
    double GetWidgetMaxDB(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetMaxDB();
        
        return 0.0;
    }
    
    double GetWidgetMinDB(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetMinDB();
        
        return 0.0;
    }
    
    void SetWidgetValue(string widgetName, double value)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValue(value);
    }
 
    void SetWidgetValue(string widgetName, double value, int mode)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValue(value, mode);
    }

    void SetWidgetValue(string widgetName, string value)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValue(value);
    }
    
    void SetWidgetValueToZero(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValueToZero();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    midi_Input* midiInput_ = nullptr;
    midi_Output* midiOutput_ = nullptr;
    bool midiInMonitor_ = false;
    bool midiOutMonitor_ = false;
    
    void HandleMidiInput()
    {
        if(midiInput_)
        {
            DAW::SwapBufsPrecise(midiInput_);
            MIDI_eventlist* list = midiInput_->GetReadBuf();
            int bpos = 0;
            MIDI_event_t* evt;
            while ((evt = list->EnumItems(&bpos)))
                ProcessMidiMessage((MIDI_event_ex_t*)evt);
        }
    }
    
    void ProcessMidiMessage(const MIDI_event_ex_t* evt)
    {
        // At this point we don't know how much of the message comprises the key, so try all three
        if(widgetsByMessage_.count(to_string(evt->midi_message[0])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0])]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(to_string(evt->midi_message[0]) + to_string(evt->midi_message[1])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0]) + to_string(evt->midi_message[1])]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(to_string(evt->midi_message[0]) + to_string(evt->midi_message[1]) + to_string(evt->midi_message[2])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0]) + to_string(evt->midi_message[1]) + to_string(evt->midi_message[2])]->ProcessMidiMessage(evt);
        
        if(midiInMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "IN -> %s %02x  %02x  %02x \n", GetName().c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
            DAW::ShowConsoleMsg(buffer);
        }
    }
    
public:
    virtual ~MidiCSurf()
    {
        if (midiInput_) delete midiInput_;
        if(midiOutput_) delete midiOutput_;
    }
    
    MidiCSurf(const string name, string templateFilename, int numChannels, bool isBankable, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor)
    : RealSurface(name, templateFilename, numChannels, isBankable), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor) {}
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) override
    {
        if(midiOutput_)
            midiOutput_->SendMsg(midiMessage, -1);
    }
    
    virtual void SendMidiMessage(int first, int second, int third) override
    {
        if(midiOutput_)
            midiOutput_->Send(first, second, third, -1);
        
        if(midiOutMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "OUT -> %s %02x  %02x  %02x \n", GetName().c_str(), first, second, third);
            DAW::ShowConsoleMsg(buffer);
        }
    }
    
    virtual void RunAndUpdate() override
    {
        HandleMidiInput();
        RealSurface::UpdateWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string suffix_= "";
    string GUID_ = DefaultGUID;
    RealSurface* realSurface_= nullptr;
    bool isMovable_ = true;
    vector<string> widgetNames_;
    
public:
    RealSurfaceChannel(string suffix, RealSurface* surface) : suffix_(suffix), realSurface_(surface) {}
   
    string GetSuffix() { return suffix_; }
    string GetGUID() { return GUID_; }
    bool GetIsMovable() { return isMovable_; }
    
    void SetGUID(string GUID);

    void SetIsMovable(bool isMovable)
    {
        isMovable_ = isMovable;
    }
    
    void AddWidget(MidiWidget* widget)
    {
        widget->SetSuffix(suffix_);
        widgetNames_.push_back(widget->GetName());
        realSurface_->AddWidget(widget);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string name_ = "";
    Layout* layout_= nullptr;
    int numBankableChannels_ = 0;
    int trackOffset_ = 0;
    vector<RealSurface*> realSurfaces_;
    map<string, string> actionTemplateDirectory_;
    map<string, string> fxTemplateDirectory_;
    map<string, map<string, FXTemplate *>> fxTemplates_;
    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;

    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
    void AddAction(string actionAddress, Action* action);
    void MapRealSurfaceActions(RealSurface* surface);
    void MapTrackActions(string trackGUID, RealSurface* surface);

    string CurrentModifers()
    {
        string modifiers = "";
        
        if(shift_)
            modifiers += Shift;
        if(option_)
            modifiers += Option;
        if(control_)
            modifiers +=  Control;
        if(alt_)
            modifiers += Alt;
        
        return modifiers;
    }
    
    string ActionAddressFor(string GUID, string surfaceName, string actionName)
    {
        string currentModifiers = "";
        
        if(actionName != Shift && actionName != Option && actionName != Control && actionName != Alt)
            currentModifiers = CurrentModifers();
        
        return GUID + GetName() + surfaceName + currentModifiers + actionName;
    }
    
    void InitFXMaps(RealSurface* surface)
    {
        FXTemplate* fxTemplate = nullptr;
        string templateDirectory = fxTemplateDirectory_[surface->GetName()];
        
        for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
        {
            if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'f' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
            {
                ifstream fxTemplateFile(string(templateDirectory + "/" + filename));
                
                string firstLine;
                getline(fxTemplateFile, firstLine);
                fxTemplate = new FXTemplate(firstLine);
                
                for (string line; getline(fxTemplateFile, line) ; )
                {
                    if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                    {
                        istringstream iss(line);
                        vector<string> tokens;
                        string token;
                        while (iss >> quoted(token))
                            tokens.push_back(token);
                        
                        if(tokens.size() == 2)
                        {
                            if(fxTemplate != nullptr)
                            {
                                replace(tokens[1].begin(), tokens[1].end(), '_', ' ');
                                fxTemplate->AddEntry(tokens[0], tokens[1]);
                            }
                        }
                    }
                }

                fxTemplates_[surface->GetName()][fxTemplate->GetName()] = fxTemplate;
            }
        }
    }

public:
    Zone(string name, Layout* layout) : name_(name), layout_(layout) {}
    
    string GetName() { return name_; }
    Layout* GetLayout() { return layout_; }
    bool IsShowFXWindows() { return showFXWindows_; }
    void MapFXActions(string trackGUID, RealSurface* surface);
    void TrackFXListChanged(MediaTrack* track);

    void Init()
    {
        for(auto* surface : realSurfaces_)
        {
            InitFXMaps(surface);
            MapRealSurfaceActions(surface);
        }
    }
    
    void AddSurface(RealSurface* surface, string actionTemplateDirectory, string fxTemplateDirectory)
    {
        string resourcePath(DAW::GetResourcePath());
        resourcePath += "/CSI/";
        
        actionTemplateDirectory_[surface->GetName()] = resourcePath + "axt/" + actionTemplateDirectory;
        fxTemplateDirectory_[surface->GetName()] = resourcePath + "fxt/" + fxTemplateDirectory;

        numBankableChannels_ += surface->GetBankableChannels().size();
        surface->SetZone(this);
        realSurfaces_.push_back(surface);
    }
    
    void AddFXWindow(FXWindow fxWindow)
    {
        openFXWindows_.push_back(fxWindow);
    }
    
    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
    }
    
    void OpenFXWindows()
    {
        if(showFXWindows_)
            for(auto fxWindow : openFXWindows_)
                DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 3);
    }
    
    void CloseFXWindows()
    {
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 2);
    }
    
    void DeleteFXWindows()
    {
        CloseFXWindows();
        openFXWindows_.clear();
    }

    void OnTrackSelection(MediaTrack* track)
    {
        for(auto* surface : realSurfaces_)
            DoAction(1.0, DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false)), surface->GetName(), TrackOnSelection, TrackOnSelection);
    }
    
    void MapTrackToWidgets(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->MapTrackToWidgets(track);
    }
    
    void UnmapWidgetsFromTrack(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->UnmapWidgetsFromTrack(track);
    }
    
    void MapFXToWidgets(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                MapFXToWidgets(track, surface);
    }
    
    void MapFXToWidgets(MediaTrack *track, RealSurface* surface)
    {
        char fxName[BUFSZ];
        char fxGUID[BUFSZ];
        char fxParamName[BUFSZ];
        
        DeleteFXWindows();
        surface->UnmapFXFromWidgets(track);
        
        string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            
            if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
            {
                FXTemplate* map = fxTemplates_[surface->GetName()][fxName];
                
                for(auto mapEntry : map->GetTemplateEntries())
                {
                    if(mapEntry.paramName == GainReductionDB)
                        surface->SetWidgetFXGUID(mapEntry.widgetName, trackGUID + fxGUID);
                    else
                        for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); DAW::TrackFX_GetParamName(track, i, j++, fxParamName, sizeof(fxParamName)))
                            if(mapEntry.paramName == fxParamName)
                                surface->SetWidgetFXGUID(mapEntry.widgetName, trackGUID + fxGUID);
                }
                
                AddFXWindow(FXWindow(track, fxGUID));
            }
        }
        
        OpenFXWindows();
        
        surface->ForceUpdateWidgets();
    }
    
    void UnmapWidgetsFromFX(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->UnmapFXFromWidgets(track);
    }
    
    void MapTrackAndFXActions(string trackGUID)
    {
        for(auto* surface : realSurfaces_)
            MapTrackActions(trackGUID, surface);
        
        for(auto* surface : realSurfaces_)
            MapFXActions(trackGUID, surface);
    }

    void TrackListChanged()
    {
        vector<RealSurfaceChannel*> channels;
       
        for(auto* surface : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                channels.push_back(channel);

        int currentOffset = 0;
        bool shouldRefreshLayout = false;

        for(int i = trackOffset_; i < DAW::GetNumTracks() + 1 && currentOffset < channels.size(); i++)
        {
            if(channels[currentOffset]->GetIsMovable() == false)
            {
                if(DAW::GetTrackFromGUID(channels[currentOffset]->GetGUID()) == nullptr) // track has been removed
                {
                    channels[currentOffset]->SetIsMovable(true); // unlock this, sinvce there is no longer a track to lock to
                    shouldRefreshLayout = true;
                    break;
                }
                else
                {
                    currentOffset++; // track exists, move on
                }
            }
            else if(channels[currentOffset]->GetGUID() == DAW::GetTrackGUIDAsString(i))
            {
                currentOffset++; // track exists and positions are in synch
            }
            else
            {
                shouldRefreshLayout = true;
                break;
            }
        }
        
        if(shouldRefreshLayout)
            RefreshLayout();
    }
    
    void AdjustTrackBank(int stride)
    {
        int previousTrackOffset = trackOffset_;
        
        trackOffset_ += stride;
        
        if(trackOffset_ < 1 - numBankableChannels_)
            trackOffset_ = 1 - numBankableChannels_;
        
        if(trackOffset_ > DAW::GetNumTracks() - 1)
            trackOffset_ = DAW::GetNumTracks() - 1;
        
        if(trackOffset_ != previousTrackOffset)
            RefreshLayout();
    }

    void RefreshLayout()
    {
        vector<string> trackLayout;
        vector<string> lockedChannels;
       
        // Place locked channel GUIDs
        for(auto surface : realSurfaces_)
            for(auto* channel : surface->GetBankableChannels())
                if(channel->GetIsMovable() == false)
                {
                    trackLayout.push_back(channel->GetGUID());
                    lockedChannels.push_back(channel->GetGUID());
                }
                else
                    trackLayout.push_back("");

        // Fill, in the rest of the GUID slots
        int layoutStartIndex = 0;
        int offset = trackOffset_;
        
        while(offset < 0)
        {
            offset++;
            layoutStartIndex++;
        }
        
        for(int i = layoutStartIndex; i < trackLayout.size() && offset < DAW::GetNumTracks() ; )
        {
            if(find(lockedChannels.begin(), lockedChannels.end(), DAW::GetTrackGUIDAsString(offset)) != lockedChannels.end())
            {
                offset++;
                continue;
            }
            else if(trackLayout[i] == "")
                trackLayout[i++] = DAW::GetTrackGUIDAsString(offset++);
            else
                i++;
        }
            
        // Apply new layout
        offset = 0;
        for(auto* surface : realSurfaces_)
            for(auto* channel : surface->GetBankableChannels())
                 channel->SetGUID(trackLayout[offset++]);

        
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();

        
        
        
        /*
        vector<string> immovableTracks;
        
        for(auto surface : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == false)
                    immovableTracks.push_back(channel->GetGUID());
        
        vector<string> movableTracks;
        
        for(int i = 0; i < GetNumBankableChannels(); i++)
        {
            if(offset < 0)
            {
                movableTracks.push_back("");
                offset++;
            }
            else if(offset >= DAW::GetNumTracks())
                movableTracks.push_back("");
            else if(find(immovableTracks.begin(), immovableTracks.end(), DAW::GetTrackGUIDAsString(offset)) == immovableTracks.end())
                movableTracks.push_back(DAW::GetTrackGUIDAsString(offset++));
            else
                offset++;
        }
        
        offset = 0;
        
        // Apply new layout
        for(auto* surface : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == true)
                    //if(movableTracks.size() < offset)
                        channel->SetGUID(movableTracks[offset++]);
        
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
         */
    }
    
    void RunAndUpdate()
    {
        for(auto* surface : realSurfaces_)
            surface->RunAndUpdate();
    }

    void SetShift(bool value)
    {
        shift_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetOption(bool value)
    {
        option_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetControl(bool value)
    {
        control_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetAlt(bool value)
    {
        alt_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetZoom(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetZoom(value);
    }
    
    void SetScrub(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetScrub(value);
    }
    
    void SetImmobilizedTracks()
    {
        char buffer[BUFSZ];
        RealSurfaceChannel* channel = nullptr;
        
        for(auto* surface : realSurfaces_)
        {
            for(int i = 0; i < surface->GetChannels().size(); i++)
            {
                channel = surface->GetChannels()[i];
                
                if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  channel->GetSuffix()).c_str(), buffer, sizeof(buffer)))
                {
                    channel->SetGUID(buffer);
                    channel->SetIsMovable(false);
                }
            }
        }
    }

    void ImmobilizeSelectedTracks()
    {
        RealSurfaceChannel* channel = nullptr;
        
        for(auto* surface : realSurfaces_)
        {
            for(int i = 0; i < surface->GetChannels().size(); i++)
            {
                channel = surface->GetChannels()[i];
                
                if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(channel->GetGUID()), "I_SELECTED"))
                {
                    channel->SetIsMovable(false);
                    DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  channel->GetSuffix()).c_str(), channel->GetGUID().c_str());
                    DAW::MarkProjectDirty(nullptr);
                }
            }
        }
    }
    
    void MobilizeSelectedTracks()
    {
        char buffer[BUFSZ];
        RealSurfaceChannel* channel = nullptr;
        
        for(auto* surface : realSurfaces_)
        {
            for(int i = 0; i < surface->GetChannels().size(); i++)
            {
                channel = surface->GetChannels()[i];
                
                if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(channel->GetGUID()), "I_SELECTED"))
                {
                    channel->SetIsMovable(true);
                    if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  channel->GetSuffix()).c_str(), buffer, sizeof(buffer)))
                    {
                        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  channel->GetSuffix()).c_str(), "");
                        DAW::MarkProjectDirty(nullptr);
                    }
                }
            }
        }
    }

    // to Widgets ->
    void ForceUpdateWidgets()
    {
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    // to Actions ->
    double GetActionCurrentNormalizedValue(string GUID, string surfaceName, string actionName, string widgetName);
    void UpdateAction(string GUID, string surfaceName, string actionName, string widgetName);
    void ForceUpdateAction(string GUID, string surfaceName, string actionName, string widgetName);
    void CycleAction(string GUID, string surfaceName, string actionName, string widgetName);
    void DoAction(double value, string GUID, string surfaceName, string actionName, string widgetName);
    
    // to Widgets ->
    double GetWidgetMaxDB(string surfaceName, string widgetName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface->GetWidgetMaxDB(widgetName);
        
        return 0.0;
    }
    
    double GetWidgetMinDB(string surfaceName, string widgetName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface->GetWidgetMinDB(widgetName);
        
        return 0.0;
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value, int mode)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value, mode);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, string value)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Layout
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";
    CSurfManager* manager_ = nullptr;
    map<string, Zone*> zones_;
    map<string, vector<Action*>> actions_;
    vector<string> mappedTrackGUIDs_;
    vector<string> touchedTracks_;
    
    void SetImmobilizedTracks()
    {
        for(auto [name, zone] : zones_)
            zone->SetImmobilizedTracks();
    }
    
public:
    Layout(string name, CSurfManager* manager) : name_(name), manager_(manager) {}

    string GetName() { return name_; }
    CSurfManager* GetManager() { return manager_; }
   
    bool GetTouchState(string trackGUID, int touchedControl)
    {
        for(string touchedGUID : touchedTracks_)
            if(touchedGUID == trackGUID)
                return true;
        
        return false;
    }
   
    void SetTouchState(string trackGUID,  bool state)
    {
        if(state)
            touchedTracks_.push_back(trackGUID);
        else
            touchedTracks_.erase(remove(touchedTracks_.begin(), touchedTracks_.end(), trackGUID), touchedTracks_.end());
    }
    
    void AddAction(string actionAddress, Action* action)
    {
        actions_[actionAddress].push_back(action);
    }
   
    void MapTrackAndFXToWidgets(MediaTrack* track, string zoneName, string surfaceName)
    {
        MapTrackToWidgets(track, zoneName, surfaceName);
        MapFXToWidgets(track, zoneName, surfaceName);
    }

    void MapTrackToWidgets(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->MapTrackToWidgets(track, surfaceName);
    }
    
    void UnmapWidgetsFromTrack(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->UnmapWidgetsFromTrack(track, surfaceName);
    }
    
    void MapFXToWidgets(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->MapFXToWidgets(track, surfaceName);
    }
    
    void UnmapWidgetsFromFX(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->UnmapWidgetsFromFX(track, surfaceName);
    }
    
    void OnTrackSelection(MediaTrack* track)
    {
        MapTrack(DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false)));
        
        for(auto const& [name, zone] : zones_)
            zone->OnTrackSelection(track);
    }
    
    void SetShowFXWindows(string zoneName, string surfaceName, bool value)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetShowFXWindows(value);
    }
    
    bool IsShowFXWindows(string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->IsShowFXWindows();
        
        return false;
    }
    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto [name, zone] : zones_)
            zone->TrackFXListChanged(track);
    }
    
    void AddZone(Zone* zone)
    {
        zones_[zone->GetName()] = zone;
    }
    
    void Init()
    {
        for(auto [name, zone] : zones_)
            zone->Init();
        
        SetImmobilizedTracks();
        RefreshLayout();
    }
 
    void MapTrack(string trackGUID)
    {
        if(trackGUID == "")
            return; // Nothing to map
        
        if(find(mappedTrackGUIDs_.begin(), mappedTrackGUIDs_.end(), trackGUID) != mappedTrackGUIDs_.end())
            return; // Already did this track
        
        for(auto [name, zone] : zones_)
            zone->MapTrackAndFXActions(trackGUID);
        
        mappedTrackGUIDs_.push_back(trackGUID);
    }
    
    void AdjustTrackBank(string zoneName, string surfaceName, int stride)
    {
        touchedTracks_.clear(); // GAW TBD -- in case anyone is touching a fader -- this is slightly pessimistic, if a fader from another zone is being touched it gets cleared too
        zones_[zoneName]->AdjustTrackBank(stride);
    }
    
    void TrackListChanged()
    {
        for(auto const& [name, zone] : zones_)
            zone->TrackListChanged();
    }
    
    void RefreshLayout()
    {
        for(auto const& [name, zone] : zones_)
            zone->RefreshLayout();
    }
    
    void RunAndUpdate()
    {
        for(auto const& [name, zone] : zones_)
            zone->RunAndUpdate();
    }

    void SetShift(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetShift(value);
    }
    
    void SetOption(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetOption(value);
    }
    
    void SetControl(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetControl(value);
    }
    
    void SetAlt(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetAlt(value);
    }
    
    void SetZoom(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetZoom(value);
    }
    
    void SetScrub(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetScrub(value);
    }

    void ImmobilizeSelectedTracks()
    {
        for(auto [name, zone] : zones_)
            zone->ImmobilizeSelectedTracks();
    }
    
    void MobilizeSelectedTracks()
    {
        for(auto [name, zone] : zones_)
            zone->MobilizeSelectedTracks();
    }
    
    // to Widgets ->
    void ForceUpdateWidgets()
    {
        for(auto [name, zone] : zones_)
            zone->ForceUpdateWidgets();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0 && actions_[actionAddress].size() > 0)
            return actions_[actionAddress][0]->GetCurrentNormalizedValue(zoneName, surfaceName, widgetName);
        else
            return 0.0;
    }

    void UpdateAction(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Update(zoneName, surfaceName, widgetName);
    }
    
    void ForceUpdateAction(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->ForceUpdate(zoneName, surfaceName, widgetName);
    }

    void CycleAction(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Cycle(zoneName, surfaceName, widgetName);
    }
    
    void DoAction(string actionAddress, double value, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Do(value, zoneName, surfaceName, widgetName);
    }
    
    // to Widgets ->
    double GetWidgetMaxDB(string zoneName, string surfaceName, string widgetName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->GetWidgetMaxDB(surfaceName, widgetName);
        
        return 0.0;
    }
    
    double GetWidgetMinDB(string zoneName, string surfaceName, string widgetName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->GetWidgetMinDB(surfaceName, widgetName);
        
        return 0.0;
    }
    
    void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetWidgetValue(surfaceName, widgetName, value);
    }
    
    void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value, int mode)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetWidgetValue(surfaceName, widgetName, value, mode);
    }
    
    void SetWidgetValue(string zoneName, string surfaceName, string widgetName, string value)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetWidgetValue(surfaceName, widgetName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiIOManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct MidiChannelInput // inner struct
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    {
        int channel_ = 0;
        midi_Input* midiInput_ = nullptr;
        
        MidiChannelInput(int channel, midi_Input* midiInput)
        : channel_(channel), midiInput_(midiInput) {}
    };
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct MidiChannelOutput // inner struct
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    {
        int channel_ = 0;
        midi_Output* midiOutput_ = nullptr;
        
        MidiChannelOutput(int channel, midi_Output* midiOutput)
        : channel_(channel), midiOutput_(midiOutput) {}
    };
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // class MidiIOManager starts here
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    vector<MidiChannelInput> inputs_;
    vector<MidiChannelOutput> outputs_;
    
public:
    MidiIOManager() {}
    
    midi_Input* GetMidiInputForChannel(int inputChannel)
    {
        for(auto input : inputs_)
            if(input.channel_ == inputChannel)
                return input.midiInput_; // return existing
        
        // make new
        midi_Input* newInput = DAW::CreateMIDIInput(inputChannel);
        
        if(newInput)
        {
            newInput->start();
            inputs_.push_back(MidiChannelInput(inputChannel, newInput));
            return newInput;
        }
        
        return nullptr;
    }
    
    midi_Output* GetMidiOutputForChannel(int outputChannel)
    {
        for(auto output : outputs_)
            if(output.channel_ == outputChannel)
                return output.midiOutput_; // return existing
        
        // make new
        midi_Output* newOutput = DAW::CreateMIDIOutput(outputChannel, false, NULL );
        
        if(newOutput)
        {
            outputs_.push_back(MidiChannelOutput(outputChannel, newOutput));
            return newOutput;
        }
        
        return nullptr;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MidiIOManager* midiIOManager_ = nullptr;
    vector <Layout*> layouts_;
    vector<RealSurface*> realSurfaces_;
    bool isInitialized_ = false;
    int currentLayoutIndex_ = 0; 
    bool VSTMonitor_ = false;
    
    void InitRealSurface(RealSurface* surface);

    void Init()
    {
        bool midiInMonitor = false;
        bool midiOutMonitor = false;
        VSTMonitor_ = false;

        Layout* currentLayout = nullptr;
        Zone* currentZone = nullptr;

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
                        midiInMonitor = true;
                }
                else if(tokens[0] == MidiOutMonitor)
                {
                    if(tokens.size() != 2)
                        continue;

                    if(tokens[1] == "On")
                        midiOutMonitor = true;
                }
                else if(tokens[0] == VSTMonitor)
                {
                    if(tokens.size() != 2)
                        continue;

                    if(tokens[1] == "On")
                        VSTMonitor_ = true;
                }
                else if(tokens[0] == RealSurface_)
                {
                    if(tokens.size() != 7)
                        continue;
                    
                    int numChannels = atoi(tokens[2].c_str());
                    bool isBankable = tokens[3] == "1" ? true : false;
                    int channelIn = atoi(tokens[4].c_str());
                    int channelOut = atoi(tokens[5].c_str());
        
                    AddRealSurface(new MidiCSurf(tokens[1], string(DAW::GetResourcePath()) + "/CSI/rst/" + tokens[6], numChannels, isBankable, GetMidiIOManager()->GetMidiInputForChannel(channelIn), GetMidiIOManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
                }
                else if(tokens[0] == Layout_)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    currentLayout = new Layout(tokens[1], this);
                    layouts_.push_back(currentLayout);
                    
                }
                else if(tokens[0] == Zone_ && currentLayout != nullptr)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    currentZone = new Zone(tokens[1], currentLayout);
                    currentLayout->AddZone(currentZone);
                }
                else if(tokens[0] == VirtualSurface_ && currentZone != nullptr)
                {
                    if(tokens.size() != 4)
                        continue;

                    for(auto surface : realSurfaces_)
                        if(surface->GetName() == tokens[1])
                            currentZone->AddSurface(surface, tokens[2], tokens[3]);
                }
            }
        }
        
        for(auto layout : layouts_)
            layout->Init();
    }

    void AddRealSurface(RealSurface* realSurface)
    {
        InitRealSurface(realSurface);
        realSurfaces_.push_back(realSurface);
    }

    void RunAndUpdate()
    {
        if(!isInitialized_)
        {
            Init();
            isInitialized_ = true;
        }
        
        if(layouts_.size() > 0)
            layouts_[currentLayoutIndex_]->RunAndUpdate();
    }
    
    double GetPrivateProfileDouble(string key)
    {
        char tmp[512];
        memset(tmp, 0, sizeof(tmp));
        
        DAW::GetPrivateProfileString("REAPER", key.c_str() , "", tmp, sizeof(tmp), DAW::get_ini_file());

        return strtod (tmp, NULL);
    }

public:
    virtual ~CSurfManager() {};
    
    CSurfManager() { midiIOManager_ = new MidiIOManager(); }
    
    MidiIOManager* GetMidiIOManager() { return midiIOManager_; }
    bool GetVSTMonitor() { return isInitialized_ ? VSTMonitor_ : false; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    void OnTrackSelection(MediaTrack *track)
    {
        if(layouts_.size() > 0)
            layouts_[currentLayoutIndex_]->OnTrackSelection(track);
    }
    
    void Run()
    {
        RunAndUpdate();
    }
    
    void ReInit()
    {
        layouts_.clear();
        realSurfaces_.clear();
        isInitialized_ = false;
        Init();
        isInitialized_ = true;

        if(layouts_.size() > 0)
            layouts_[currentLayoutIndex_]->RefreshLayout();
    }
    
    void NextLayout()
    {
        if(layouts_.size() > 0)
        {
            currentLayoutIndex_ = currentLayoutIndex_ == layouts_.size() - 1 ? 0 : ++currentLayoutIndex_;
            layouts_[currentLayoutIndex_]->RefreshLayout();
        }
    }

    bool GetTouchState(string trackGUID, int touchedControl)
    {
        if(layouts_.size() > 0)
            return layouts_[currentLayoutIndex_]->GetTouchState(trackGUID, touchedControl);
        else
            return false;
    }
    
    void TrackListChanged()
    {
        for(auto & layout : layouts_)
            layout->TrackListChanged();
    }

    void TrackFXListChanged(MediaTrack* trackid)
    {
        for(auto & layout : layouts_)
            layout->TrackFXListChanged(trackid);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSCCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~OSCCSurf() {};
    
    OSCCSurf(const string name, string templateFilename, Layout* layout)
    : RealSurface("OSC", templateFilename, 8, 8) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WebCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~WebCSurf() {};
    
    WebCSurf(const string name, string templateFilename, Layout* layout)
    : RealSurface("Web", templateFilename, 8, 8) {};
};

#endif /* control_surface_integrator.h */
