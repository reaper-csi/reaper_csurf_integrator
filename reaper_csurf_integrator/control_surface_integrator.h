//
//  control_surface_integrator.h
//  reaper_control_surface_integrator
//
//

#ifndef control_surface_integrator
#define control_surface_integrator

#include <sstream>
#include <vector>
#include <map>

#include "control_surface_integrator_Reaper.h"

const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";

// Note for Windows environments
// use std::byte for C++17 byte
// use ::byte for Windows byte

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                        THE RULES
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following are all reserved words in the map vocabulary
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const string ReaperLogicalControlSurface = "ReaperLogicalControlSurface";
const string ReaperGainReduction_dB = "ReaperGainReduction_dB";

//
// An ActionAddress allows a widget to access a particular action - e.g. "{ GUID }Mixer1Fader"
// ActionAddress format = GUID + realSurfaceName + modifiers + widgetActionName
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
// Please do not modify RealSurface::CurrentModifiers()
//
// Allowed -- ShiftControl -- OK
// Disallowed -- ControlShift -- no good
//
// Modifier Order matters !!
// Please do not modify RealSurface::CurrentModifiers()
//
// The modifiers, if present:
//  must be contained in the modifier part of the action address
//  must be contained only in the modifier part of the action address
//  in the case of combos, must be in the same order as listed above -- e.g. ShiftOptionControlAlt for the full meal deal
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
    // GAW TBD this should really use FXGUID instead of index, as the order might be rearranged whilst the window is open
    MediaTrack* track = nullptr;;
    int index = 0;
    
    FXWindow(MediaTrack* aTrack, int anIndex) : track(aTrack), index(anIndex) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MapEntry
{
    string widgetName;
    string paramName;
    
    MapEntry(string aWidgetName, string aParamName) : widgetName(aWidgetName), paramName(aParamName) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXMap
{
private:
    string name;
    vector<MapEntry> entries_;
    
public:
    FXMap(string aName) : name(aName) {}
    
    string GetName() { return name; }
    vector<MapEntry>& GetMapEntries() { return entries_; }
    
    void AddEntry(string widgetName, string paramName)
    {
        entries_.push_back(MapEntry(widgetName, paramName));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurface;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string GUID_ = "";
    RealSurface* realSsurface_ = nullptr;
    string name_= "";
    string actionName_ = "";
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    string GetGUID() { return GUID_; }
    RealSurface* GetRealSurface() { return realSsurface_; }
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
    MidiWidget(string GUID, RealSurface* surface, string actionName, string name, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : GUID_(GUID), realSsurface_(surface), actionName_(actionName), name_(name),  midiPressMessage_(press), midiReleaseMessage_(release) {}

public:
    virtual ~MidiWidget() {};
    
    virtual double GetMinDB() { return -72.0; }
    virtual double GetMaxDB() { return 12.0; }
    string GetName() { return name_; }
    string GetActionName() { return actionName_; }

    void SetGUID(string GUID)
    {
        GUID_ = GUID;
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
class LogicalSurface;
class RealSurfaceChannel;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    const string name_ = "";
    LogicalSurface* logicalSurface_ = nullptr;
    string surfaceGroupName_ = "";
    int numBankableChannels_ = 0;
    vector<RealSurfaceChannel*> channels_;
    map<string, MidiWidget*> widgetsByName_;
    map<string, MidiWidget*> widgetsByMessage_;
    vector<FXWindow> openFXWindows_;
    
    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
    bool zoom_ = false;
    bool scrub_ = false;
    
    bool showFXWindows_ = false;
    
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
    
    string ActionAddressFor(string GUID, string actionName)
    {
        string currentModifiers = "";
        
        if(actionName != Shift && actionName != Option && actionName != Control && actionName != Alt)
            currentModifiers = CurrentModifers();
        
        return GUID + GetName() + currentModifiers + actionName;
    }
    
    RealSurface(LogicalSurface* logicalSurface, const string name, int numBankableChannels) : logicalSurface_(logicalSurface), name_(name),  numBankableChannels_(numBankableChannels) {}
    
public:
    virtual ~RealSurface() {};
    
    const string GetName() const { return name_; }
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    string GetSurfaceGroupName() { return surfaceGroupName_; }
    vector<RealSurfaceChannel*> & GetChannels() { return channels_; }
    int GetNumBankableChannels() { return numBankableChannels_; }
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }
    bool IsShowFXWindows() { return showFXWindows_; }
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    virtual void SendMidiMessage(int first, int second, int third) {}
    
    virtual void RunAndUpdate() {}
    
    void MapTrackActions(string GUID);
    
    void SetGroupName(string groupName)
    {
        surfaceGroupName_ = groupName;
    }
    
    void AddChannel(RealSurfaceChannel*  channel)
    {
        channels_.push_back(channel);
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
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetGUID(GUID);
    }

    void SetShift(bool value)
    {
        shift_ = value;
        ForceUpdateWidgets();
    }
    
    void SetOption(bool value)
    {
        option_ = value;
        ForceUpdateWidgets();
    }
    
    void SetControl(bool value)
    {
        control_ = value;
        ForceUpdateWidgets();
    }
    
    void SetAlt(bool value)
    {
        alt_ = value;
        ForceUpdateWidgets();
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
    
    void ClearFXWindows()
    {
        openFXWindows_.clear();
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
                DAW::TrackFX_Show(fxWindow.track, fxWindow.index, 3);
    }
    
    void CloseFXWindows()
    {
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, fxWindow.index, 2);
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
class RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string GUID_ = "";
    RealSurface* realSurface_= nullptr;
    bool isMovable_ = true;
    bool shouldMapFXTrackToChannel_ = false;
    vector<string> widgetNames_;

public:
    virtual ~RealSurfaceChannel() {}
    
    RealSurfaceChannel(string GUID, RealSurface* surface, bool isMovable) : GUID_(GUID), realSurface_(surface), isMovable_(isMovable) {}
    
    RealSurfaceChannel(string GUID, RealSurface* surface, bool isMovable, bool shouldMapFXTrackToChannel) : GUID_(GUID), realSurface_(surface), isMovable_(isMovable), shouldMapFXTrackToChannel_(shouldMapFXTrackToChannel) {}
    
    string GetGUID() { return GUID_; }
    bool GetIsMovable() { return isMovable_; }
    bool GetShouldMapFXTrackToChannel() { return shouldMapFXTrackToChannel_; }
    
    void MapTrackActions(string GUID);

    void SetIsMovable(bool isMovable)
    {
        isMovable_ = isMovable;
    }
    
    void AddWidget(MidiWidget* widget)
    {
        widgetNames_.push_back(widget->GetName());
        realSurface_->AddWidget(widget);
    }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
        
        realSurface_->MapTrackActions(GUID_);
        
        for (auto widgetName : widgetNames_)
        {
            realSurface_->SetWidgetGUID(widgetName, GUID);
            
            if(GUID_ == "")
                realSurface_->SetWidgetValueToZero(widgetName);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    LogicalSurface* logicalSurface_ = nullptr;
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) {}
    virtual void SetWidgetValue(string surfaceName, string widgetName, string value) {}

    Action(LogicalSurface* logicalSurface) : logicalSurface_(logicalSurface) {}
    
public:
    virtual ~Action() {}
    
    virtual int GetDisplayMode() { return 0; }
    virtual double GetCurrentNormalizedValue () { return 0.0; }

    virtual void Add(Action* action) {}
    virtual void Update(string surfaceName, string widgetName) {}
    virtual void ForceUpdate(string surfaceName, string widgetName) {}
    virtual void Cycle(string surfaceName, string widgetName) {}
    virtual void Do(double value, string surfaceName, string widgetName) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceGroup
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string groupName_ = "";
    int numLogicalChannels_ = 0;
    int trackOffset_ = 0;
    vector<RealSurface*> realSurfaces_;
    
public:
    SurfaceGroup(string groupName, int numLogicalChannels) : groupName_(groupName), numLogicalChannels_(numLogicalChannels) {}
    
    string GetGroupName() { return groupName_; }
    
    int GetNumLogicalChannels() { return numLogicalChannels_; }
    
    void AddSurface(RealSurface* surface)
    {
        realSurfaces_.push_back(surface);
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
        
        if(trackOffset_ < 1 - GetNumLogicalChannels())
            trackOffset_ = 1 - GetNumLogicalChannels();
        
        if(trackOffset_ > DAW::GetNumTracks() - 1)
            trackOffset_ = DAW::GetNumTracks() - 1;
        
        if(trackOffset_ != previousTrackOffset)
            RefreshLayout();
    }

    void RefreshLayout()
    {
        auto currentOffset = trackOffset_;
        
        vector<string> immovableTracks;
        
        for(auto* surface : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == false)
                    immovableTracks.push_back(channel->GetGUID());
        
        vector<string> movableTracks;
        
        for(int i = 0; i < GetNumLogicalChannels(); i++)
        {
            if(currentOffset < 0)
            {
                movableTracks.push_back("");
                currentOffset++;
            }
            else if(currentOffset >= DAW::GetNumTracks())
                movableTracks.push_back("");
            else if(find(immovableTracks.begin(), immovableTracks.end(), DAW::GetTrackGUIDAsString(currentOffset)) == immovableTracks.end())
                movableTracks.push_back(DAW::GetTrackGUIDAsString(currentOffset++));
            else
                currentOffset++;
        }
        
        currentOffset = 0;
        
        // Apply new layout
        for(auto* surface : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == true)
                    channel->SetGUID(movableTracks[currentOffset++]);
        
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetShift(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetShift(value);
    }
    
    void SetOption(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetOption(value);
    }
    
    void SetControl(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetControl(value);
    }
    
    void SetAlt(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetAlt(value);
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
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LogicalSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfManager* manager_ = nullptr;
    map<string, FXMap *> fxMaps_;
    vector<RealSurface*> realSurfaces_;
    map<string, SurfaceGroup*> surfaceGroups_;
    map<string, Action*> actions_;
    vector<string> mappedTrackActionGUIDs_;
    
    bool VSTMonitor_ = false;

    void InitRealSurfaces();
    void InitFXMaps(RealSurface* surface);
    void MapReaperLogicalControlSurfaceActions(RealSurface* surface);
    void InitCSurfWidgets(RealSurface* surface);
    void SetImmobilizedChannels();

    RealSurface* GetRealSurfaceFor(string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface;

        return nullptr;
    }
    
    void AddFXMap(FXMap* fxMap)
    {
        fxMaps_[fxMap->GetName()] = fxMap;
    }
    
    void AddRealSurface(RealSurface* realSurface)
    {
        realSurfaces_.push_back(realSurface);
    }
 
    void AddAction(string actionAddress, Action* action)
    {
        actions_[actionAddress] = action;
    }
    
public:
    LogicalSurface(CSurfManager* manager) : manager_(manager) {}

    CSurfManager* GetManager() { return manager_; }
    map<string, FXMap *> GetFXMaps() { return fxMaps_; }
    bool GetVSTMonitor() { return VSTMonitor_; }

    bool IsShowFXWindows(string surfaceName)
    {
        for(auto & surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface->IsShowFXWindows();
        
        return false;
    }
    
    void Init()
    {
        // GAW TBD temp hardwiring -- this will be replaced with load from map file //////////////////////////////////////////
        InitRealSurfaces();
        surfaceGroups_[ReaperLogicalControlSurface] = new SurfaceGroup(ReaperLogicalControlSurface, 21);

        for(auto* surface : realSurfaces_)
        {
            surface->SetGroupName(ReaperLogicalControlSurface);
            surfaceGroups_[ReaperLogicalControlSurface]->AddSurface(surface);
            
            InitFXMaps(surface);
            MapReaperLogicalControlSurfaceActions(surface);
            InitCSurfWidgets(surface);
        }
        // ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        SetImmobilizedChannels();
        RefreshLayout();
    }
    
    void MapTrackActions(string trackGUID);
    void MapFX(MediaTrack* track);
    void MapWidgetsToFX(MediaTrack *trackid);
    
    void AdjustTrackBank(string surfaceName, int stride)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->AdjustTrackBank(stride);
    }
    
    void TrackListChanged()
    {
        for(auto const& [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->TrackListChanged();
    }
    
    void RefreshLayout()
    {
        for(auto const& [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->RefreshLayout();
    }
    
    void OnTrackSelection(MediaTrack* track)
    {
        for(auto & surface : realSurfaces_)
            surface->CloseFXWindows();
        
        if(DAW::CountSelectedTracks(nullptr) == 1)
            MapWidgetsToFX(track);
    }

    void RunAndUpdate()
    {
        for(auto & surface : realSurfaces_)
            surface->RunAndUpdate();
    }

    void SetShift(string surfaceName, bool value)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->SetShift(value);
    }
    
    void SetOption(string surfaceName, bool value)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->SetOption(value);
    }
    
    void SetControl(string surfaceName, bool value)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->SetControl(value);
    }
    
    void SetAlt(string surfaceName, bool value)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->SetAlt(value);
    }
    
    void SetZoom(string surfaceName, bool value)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->SetZoom(value);
    }
    
    void SetScrub(string surfaceName, bool value)
    {
        surfaceGroups_[GetRealSurfaceFor(surfaceName)->GetSurfaceGroupName()]->SetScrub(value);
    }
    
    void SetShowOpenFXWindows(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetShowFXWindows(value);
    }
    
    void OpenFXWindows(RealSurface* surface)
    {
        for(auto* surface : realSurfaces_)
            surface->OpenFXWindows();
    }
    
    void CloseFXWindows(RealSurface* surface)
    {
        for(auto* surface : realSurfaces_)
            surface->CloseFXWindows();
    }
    
    void ImmobilizeSelectedTracks()
    {
        for(auto * surface : realSurfaces_)
            for(int i = 0; i < surface->GetChannels().size(); i++)
                if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(surface->GetChannels()[i]->GetGUID()), "I_SELECTED"))
                {
                    surface->GetChannels()[i]->SetIsMovable(false);
                    DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  to_string(i - 1)).c_str(), surface->GetChannels()[i]->GetGUID().c_str());
                    DAW::MarkProjectDirty(nullptr);
                }
    }
    
    void MobilizeSelectedTracks()
    {
        char buffer[256];

        for(auto * surface : realSurfaces_)
            for(int i = 0; i < surface->GetChannels().size(); i++)
                if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(surface->GetChannels()[i]->GetGUID()), "I_SELECTED"))
                {
                    surface->GetChannels()[i]->SetIsMovable(true);
                    if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  to_string(i - 1)).c_str(), buffer, sizeof(buffer)))
                    {
                        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  to_string(i - 1)).c_str(), "");
                        DAW::MarkProjectDirty(nullptr);
                    }
                }
    }

    void TrackFXListChanged(MediaTrack* track)
    {
        MapFX(track);
    }
    
    // to Widgets ->
    void ForceUpdate()
    {
        for(auto& surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string actionAddress, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            return actions_[actionAddress]->GetCurrentNormalizedValue();
        else
            return 0.0;
    }

    void UpdateAction(string actionAddress, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            actions_[actionAddress]->Update(surfaceName, widgetName);
    }
    
    void ForceUpdateAction(string actionAddress, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            actions_[actionAddress]->ForceUpdate(surfaceName, widgetName);
    }

    void CycleAction(string actionAddress, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            actions_[actionAddress]->Cycle(surfaceName, widgetName);
    }
    
    void DoAction(string actionAddress, double value, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            actions_[actionAddress]->Do(value, surfaceName, widgetName);
    }
    
    // to Widgets ->
    void SetWidgetValue(string surfaceName, string widgetName, double value)
    {
        for(auto & surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value, int mode)
    {
        for(auto & surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value, mode);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, string value)
    {
        for(auto & surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiIOManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
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
private:
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
    vector <LogicalSurface*> logicalSurfaces_;
    bool lazyInitialized_ = false;
    int currentLogicalSurfaceIndex_ = 0;;

    void RunAndUpdate()
    {
        if(!lazyInitialized_)
        {
            LogicalSurface* logicalSurface = new LogicalSurface(this);
            logicalSurface->Init();
            logicalSurfaces_.push_back(logicalSurface);
            
            lazyInitialized_ = true;
        }
        
        logicalSurfaces_[currentLogicalSurfaceIndex_]->RunAndUpdate();
    }
    
    double GetPrivateProfileDouble(string key)
    {
        char tmp[512];
        memset(tmp, 0, sizeof(tmp));
        
        DAW::GetPrivateProfileString("REAPER", key.c_str() , "", tmp, sizeof(tmp), get_ini_file());

        return strtod (tmp, NULL);
    }

public:
    virtual ~CSurfManager() {};
    
    CSurfManager() { midiIOManager_ = new MidiIOManager(); }
    
    MidiIOManager* MidiManager() { return midiIOManager_; }
    bool GetLazyIsInitialized() { return lazyInitialized_; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    void OnTrackSelection(MediaTrack *track)
    {
        logicalSurfaces_[currentLogicalSurfaceIndex_]->OnTrackSelection(track);
    }
    
    void Run()
    {
        RunAndUpdate();
    }
    
    void NextLogicalSurface()
    {
        currentLogicalSurfaceIndex_ = currentLogicalSurfaceIndex_ == logicalSurfaces_.size() - 1 ? 0 : ++currentLogicalSurfaceIndex_;

        logicalSurfaces_[currentLogicalSurfaceIndex_]->RefreshLayout();
    }

    void TrackListChanged()
    {
        for(auto & surface : logicalSurfaces_)
            surface->TrackListChanged();
    }

    void TrackFXListChanged(MediaTrack* trackid)
    {
        for(auto & surface : logicalSurfaces_)
            surface->TrackFXListChanged(trackid);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSCCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~OSCCSurf() {};
    
    OSCCSurf(const string name, LogicalSurface* surface)
    : RealSurface(surface, "OSC", 8) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WebCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~WebCSurf() {};
    
    WebCSurf(const string name, LogicalSurface* surface)
    : RealSurface(surface, "Web", 8) {};
};

#endif /* control_surface_integrator.h */
