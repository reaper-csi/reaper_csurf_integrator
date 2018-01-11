//
//  control_surface_integrator.h
//  reaper_control_surface_integrator
//
//

#ifndef control_surface_integrator
#define control_surface_integrator

#include "time.h"
#include <sstream>
#include <vector>
#include <map>

#include "control_surface_integrator_Reaper.h"

const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";

const double System_MaxDB = 12.0;
const double System_MinDB = -72.0;

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
const string GainReduction_dB = "GainReduction_dB";
const string TrackOnSelection = "TrackOnSelection";

//
// An ActionAddress allows a widget to access a particular action - e.g. "{ GUID }Mixer1Fader"
// ActionAddress format = GUID + realSurfaceName + modifiers + widgetName
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
//  in the case of combos, must be in the same order as listed above -- e.g. "{ GUID }Mixer1ShiftOptionControlAltFader" for the full meal deal
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
    string suffix_= "";
    string name_ = "";
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    RealSurface* GetRealSurface() { return realSsurface_; }
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
    MidiWidget(string GUID, RealSurface* surface, string name, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : GUID_(GUID), realSsurface_(surface), name_(name),  midiPressMessage_(press), midiReleaseMessage_(release) {}

public:
    virtual ~MidiWidget() {};
    
    string GetGUID() { return GUID_; }
    virtual double GetMinDB() { return System_MinDB; }
    virtual double GetMaxDB() { return System_MaxDB; }
    string GetName() { return name_ + suffix_; }
    string GetActionName() { return name_; }

    void SetSuffix(string suffix)
    {
        suffix_ = suffix;
    }
    
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
class SurfaceGroup;
class RealSurfaceChannel;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    const string name_ = "";
    SurfaceGroup* surfaceGroup_ = nullptr;
    int numBankableChannels_ = 0;
    vector<RealSurfaceChannel*> channels_;
    map<string, MidiWidget*> widgetsByName_;
    map<string, MidiWidget*> widgetsByMessage_;
    
    bool zoom_ = false;
    bool scrub_ = false;
    
    RealSurface(const string name, int numBankableChannels) : name_(name),  numBankableChannels_(numBankableChannels) {}
    
public:

    
    
    
    
    
    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;
    
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
                DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 3);
    }
    
    void CloseFXWindows()
    {
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 2);
    }
    

    
    
    
    
    
    
    
    
    virtual ~RealSurface() {};
    
    const string GetName() const { return name_; }
    SurfaceGroup* GetSurfaceGroup() { return surfaceGroup_; }
    vector<RealSurfaceChannel*> & GetChannels() { return channels_; }
    int GetNumBankableChannels() { return numBankableChannels_; }
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }
    bool IsShowFXWindows() { return showFXWindows_; }
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    virtual void SendMidiMessage(int first, int second, int third) {}
    
    virtual void RunAndUpdate() {}
    
    void SetSurfaceGroup(SurfaceGroup* surfaceGroup)
    {
        surfaceGroup_ = surfaceGroup;
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
        
        return System_MaxDB;
    }
    
    double GetWidgetMinDB(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetMinDB();
        
        return System_MinDB;
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
class RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string suffix_= "";
    int index_ = 0;
    string GUID_ = "";
    RealSurface* realSurface_= nullptr;
    bool isMovable_ = true;
    bool shouldMapFXTrackToChannel_ = false;
    vector<string> widgetNames_;
    
public:
    RealSurfaceChannel(string suffix, int index, RealSurface* surface) : suffix_(suffix), index_(index), realSurface_(surface) {}
   
    string GetSuffix() { return suffix_; }
    string GetGUID() { return GUID_; }
    bool GetIsMovable() { return isMovable_; }
    bool GetShouldMapFXTrackToChannel() { return shouldMapFXTrackToChannel_; }
    
    void SetGUID(string GUID);

    void SetIsMovable(bool isMovable)
    {
        isMovable_ = isMovable;
    }
    
    void SetShouldMapFXTrackToChannel(bool shouldMapFXTrackToChannel)
    {
        shouldMapFXTrackToChannel_ = shouldMapFXTrackToChannel;
    }
    
    void AddWidget(MidiWidget* widget)
    {
        widget->SetSuffix(suffix_);
        widgetNames_.push_back(widget->GetName());
        realSurface_->AddWidget(widget);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    LogicalSurface* logicalSurface_ = nullptr;
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    
    virtual void SetWidgetValue(string groupName, string surfaceName, string widgetName, double value) {}
    virtual void SetWidgetValue(string groupName, string surfaceName, string widgetName, string value) {}

    Action(LogicalSurface* logicalSurface) : logicalSurface_(logicalSurface) {}
    
public:
    virtual ~Action() {}
    
    virtual int GetDisplayMode() { return 0; }
    virtual double GetCurrentNormalizedValue (string groupName, string surfaceName, string widgetName) { return 0.0; }

    virtual void AddAction(Action* action) {}
    virtual void Update(string groupName, string surfaceName, string widgetName) {}
    virtual void ForceUpdate(string groupName, string surfaceName, string widgetName) {}
    virtual void Cycle(string groupName, string surfaceName, string widgetName) {}
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceGroup
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string name_ = "";
    LogicalSurface* logicalSurface_= nullptr;
    int numLogicalChannels_ = 0;
    int trackOffset_ = 0;
    map<string, RealSurface*> realSurfaces_;
    
    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
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

   
public:
    SurfaceGroup(string name, LogicalSurface* logicalSurface, int numLogicalChannels) : name_(name), logicalSurface_(logicalSurface), numLogicalChannels_(numLogicalChannels) {}
    
    string GetName() { return name_; }
    
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    
    int GetNumLogicalChannels() { return numLogicalChannels_; }

    void AddSurface(RealSurface* surface)
    {
        realSurfaces_[surface->GetName()] = surface;
    }
    
    void TrackListChanged()
    {
        vector<RealSurfaceChannel*> channels;
       
        for(auto [name, surface] : realSurfaces_)
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
        auto offset = trackOffset_;
        
        vector<string> immovableTracks;
        
        for(auto [name, surface] : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == false)
                    immovableTracks.push_back(channel->GetGUID());
        
        vector<string> movableTracks;
        
        for(int i = 0; i < GetNumLogicalChannels(); i++)
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
        for(auto [name, surface] : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == true)
                    channel->SetGUID(movableTracks[offset++]);
        
        for(auto [name, surface] : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void RunAndUpdate()
    {
        for(auto [name, surface] : realSurfaces_)
            surface->RunAndUpdate();
    }

    void SetShift(bool value)
    {
        shift_ = value;
        for(auto [name, surface] : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetOption(bool value)
    {
        option_ = value;
        for(auto [name, surface] : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetControl(bool value)
    {
        control_ = value;
        for(auto [name, surface] : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetAlt(bool value)
    {
        alt_ = value;
        for(auto [name, surface] : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetZoom(bool value)
    {
        for(auto [name, surface] : realSurfaces_)
            surface->SetZoom(value);
    }
    
    void SetScrub(bool value)
    {
        for(auto [name, surface] : realSurfaces_)
            surface->SetScrub(value);
    }
    
    void SetImmobilizedTracks()
    {
        char buffer[BUFSZ];
        RealSurfaceChannel* channel = nullptr;
        
        for(auto [name, surface] : realSurfaces_)
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
        
        for(auto [name, surface] : realSurfaces_)
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
        
        for(auto [name, surface] : realSurfaces_)
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
        for(auto [name, surface] : realSurfaces_)
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
        if(realSurfaces_.count(surfaceName) > 0)
            return realSurfaces_[surfaceName]->GetWidgetMaxDB(widgetName);
        
        return System_MaxDB;
    }
    
    double GetWidgetMinDB(string surfaceName, string widgetName)
    {
        if(realSurfaces_.count(surfaceName) > 0)
            return realSurfaces_[surfaceName]->GetWidgetMinDB(widgetName);
        
        return System_MinDB;
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value)
    {
        if(realSurfaces_.count(surfaceName) > 0)
            realSurfaces_[surfaceName]->SetWidgetValue(widgetName, value);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value, int mode)
    {
        if(realSurfaces_.count(surfaceName) > 0)
            realSurfaces_[surfaceName]->SetWidgetValue(widgetName, value, mode);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, string value)
    {
        if(realSurfaces_.count(surfaceName) > 0)
            realSurfaces_[surfaceName]->SetWidgetValue(widgetName, value);
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
    map<string, vector<Action*>> actions_;
    vector<string> mappedTrackGUIDs_;
    vector<string> touchedTracks_;
    
    void InitFXMaps(RealSurface* surface);
    void MapReaperLogicalControlSurfaceActions(string groupName, string surfaceName);

    void SetImmobilizedTracks()
    {
        for(auto [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->SetImmobilizedTracks();
    }
    
    void AddFXMap(FXMap* fxMap)
    {
        fxMaps_[fxMap->GetName()] = fxMap;
    }

    void AddAction(string actionAddress, Action* action)
    {
        actions_[actionAddress].push_back(action);
    }
    
public:
    LogicalSurface(CSurfManager* manager) : manager_(manager) {}

    CSurfManager* GetManager() { return manager_; }
    map<string, FXMap *> GetFXMaps() { return fxMaps_; }
   
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
    
    void Init();
    
    void MapTrackAndFXActions(string trackGUID, string groupName, string surfaceName);
    void MapFXActions(MediaTrack* track, string groupName, string surfaceName);
    
    void MapTrack(string trackGUID)
    {
        if(trackGUID == "")
            return; // Nothing to map
        
        for(string mappedTrackActionGUID : mappedTrackGUIDs_)
            if(mappedTrackActionGUID == trackGUID)
                return; // Already did this track
        
        for(auto * surface : realSurfaces_)
            MapTrackAndFXActions(trackGUID, surface->GetSurfaceGroup()->GetName(), surface->GetName());
        
        mappedTrackGUIDs_.push_back(trackGUID);
    }
    
    void AdjustTrackBank(string groupName, string surfaceName, int stride)
    {
        touchedTracks_.clear(); // GAW -- in case anyone is touching a fader -- this is slightly pessimistic, if a fader from another durface group is being touched it gets cleared too
        surfaceGroups_[groupName]->AdjustTrackBank(stride);
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
    
    void RunAndUpdate()
    {
        for(auto const& [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->RunAndUpdate();
    }

    void SetShift(string groupName, string surfaceName, bool value)
    {
        surfaceGroups_[groupName]->SetShift(value);
    }
    
    void SetOption(string groupName, string surfaceName, bool value)
    {
        surfaceGroups_[groupName]->SetOption(value);
    }
    
    void SetControl(string groupName, string surfaceName, bool value)
    {
        surfaceGroups_[groupName]->SetControl(value);
    }
    
    void SetAlt(string groupName, string surfaceName, bool value)
    {
        surfaceGroups_[groupName]->SetAlt(value);
    }
    
    void SetZoom(string groupName, string surfaceName, bool value)
    {
        surfaceGroups_[groupName]->SetZoom(value);
    }
    
    void SetScrub(string groupName, string surfaceName, bool value)
    {
        surfaceGroups_[groupName]->SetScrub(value);
    }

    
    
    
    
    
    
    void OnTrackSelection(MediaTrack* track)
    {
        for(auto* surface : realSurfaces_)
        {
            surface->CloseFXWindows();
            surface->ClearFXWindows();
            
            if(1 == DAW::CountSelectedTracks(nullptr))
                MapFXToWidgets(track, surface);
        }
        
        for(auto* surface : realSurfaces_)
            OpenFXWindows(surface);
        
        ForceUpdateWidgets();
    }
    
    void MapFXToWidgets(MediaTrack *track, RealSurface* surface)
    {
        string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
        
        // Map Track to any Channels interested
        for(auto* channel : surface->GetChannels())
            if(channel->GetShouldMapFXTrackToChannel())
                channel->SetGUID(trackGUID);
        
        char fxName[BUFSZ];
        char fxGUID[BUFSZ];
        char fxParamName[BUFSZ];
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            
            if(fxMaps_.count(surface->GetName() + fxName) > 0)
            {
                FXMap* map = fxMaps_[surface->GetName() + fxName];
                
                for(auto mapEntry : map->GetMapEntries())
                {
                    if(mapEntry.paramName == GainReduction_dB)
                        surface->SetWidgetGUID(mapEntry.widgetName, trackGUID + fxGUID);
                    else
                        for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); DAW::TrackFX_GetParamName(track, i, j++, fxParamName, sizeof(fxParamName)))
                            if(mapEntry.paramName == fxParamName)
                                surface->SetWidgetGUID(mapEntry.widgetName, trackGUID + fxGUID);
                }
                
                surface->AddFXWindow(FXWindow(track, fxGUID));
            }
        }
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
    
    void SetShowOpenFXWindows(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetShowFXWindows(value);
    }
    
    
    bool IsShowFXWindows(string surfaceName)
    {
        for(auto & surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface->IsShowFXWindows();
        
        return false;
    }

    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto* surface : realSurfaces_)
            MapFXActions(track, surface->GetSurfaceGroup()->GetName(), surface->GetName());
    }

    
    
    
    
    
    
   
    void ImmobilizeSelectedTracks()
    {
        for(auto [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->ImmobilizeSelectedTracks();
    }
    
    void MobilizeSelectedTracks()
    {
        for(auto [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->MobilizeSelectedTracks();
    }
    
    // to Widgets ->
    void ForceUpdateWidgets()
    {
        for(auto [name, surfaceGroup] : surfaceGroups_)
            surfaceGroup->ForceUpdateWidgets();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string actionAddress, string groupName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0 && actions_[actionAddress].size() > 0)
            return actions_[actionAddress][0]->GetCurrentNormalizedValue(groupName, surfaceName, widgetName);
        else
            return 0.0;
    }

    void UpdateAction(string actionAddress, string groupName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Update(groupName, surfaceName, widgetName);
    }
    
    void ForceUpdateAction(string actionAddress, string groupName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->ForceUpdate(groupName, surfaceName, widgetName);
    }

    void CycleAction(string actionAddress, string groupName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Cycle(groupName, surfaceName, widgetName);
    }
    
    void DoAction(string actionAddress, double value, string groupName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Do(value, groupName, surfaceName, widgetName);
    }
    
    // to Widgets ->
    double GetWidgetMaxDB(string groupName, string surfaceName, string widgetName)
    {
        if(surfaceGroups_.count(groupName) > 0)
            return surfaceGroups_[groupName]->GetWidgetMaxDB(surfaceName, widgetName);
        
        return System_MaxDB;
    }
    
    double GetWidgetMinDB(string groupName, string surfaceName, string widgetName)
    {
        if(surfaceGroups_.count(groupName) > 0)
            return surfaceGroups_[groupName]->GetWidgetMinDB(surfaceName, widgetName);
        
        return System_MinDB;
    }
    
    void SetWidgetValue(string groupName, string surfaceName, string widgetName, double value)
    {
        if(surfaceGroups_.count(groupName) > 0)
            surfaceGroups_[groupName]->SetWidgetValue(surfaceName, widgetName, value);
    }
    
    void SetWidgetValue(string groupName, string surfaceName, string widgetName, double value, int mode)
    {
        if(surfaceGroups_.count(groupName) > 0)
            surfaceGroups_[groupName]->SetWidgetValue(surfaceName, widgetName, value, mode);
    }
    
    void SetWidgetValue(string groupName, string surfaceName, string widgetName, string value)
    {
        if(surfaceGroups_.count(groupName) > 0)
            surfaceGroups_[groupName]->SetWidgetValue(surfaceName, widgetName, value);
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
    vector<RealSurface*> realSurfaces_;
    bool isInitialized_ = false;
    int currentLogicalSurfaceIndex_ = 0;;
    
    bool VSTMonitor_ = false;
    
    void InitRealSurfaces();
    void InitRealSurfaceMap(RealSurface* surface);
    
    void AddRealSurface(RealSurface* realSurface)
    {
        InitRealSurfaceMap(realSurface);
        realSurfaces_.push_back(realSurface);
    }

    void RunAndUpdate()
    {
        if(!isInitialized_)
        {
            InitRealSurfaces();
          
            LogicalSurface* logicalSurface = new LogicalSurface(this);

            logicalSurface->Init();
            logicalSurfaces_.push_back(logicalSurface);
            
            isInitialized_ = true;
        }
        
        logicalSurfaces_[currentLogicalSurfaceIndex_]->RunAndUpdate();
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
    vector<RealSurface*> GetRealSurfaces() { return realSurfaces_; }
    bool GetIsInitialized() { return isInitialized_; }
    bool GetVSTMonitor() { return VSTMonitor_; }
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

    bool GetTouchState(string trackGUID, int touchedControl)
    {
        return logicalSurfaces_[currentLogicalSurfaceIndex_]->GetTouchState(trackGUID, touchedControl);
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
    : RealSurface("OSC", 8) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WebCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~WebCSurf() {};
    
    WebCSurf(const string name, LogicalSurface* surface)
    : RealSurface("Web", 8) {};
};

#endif /* control_surface_integrator.h */
