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

// Note for Windows environments
// use std::byte for C++17 byte
// use ::byte for Windows byte

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                        THE RULES
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following are all reserved words in the map vocabulary
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const string Master = "Master";
const string LogicalControlSurface = "LogicalControlSurface";
const string GainReduction_dB = "GainReduction_dB";

//
// An ActionAddress allows a widget to access a particular action == "{ GUID }Mixer1Fader1"
// ActionAddress format - GUID + realSurfaceName + modifiers + widgetName
// GUID and/or modifiers can be ""
// realSurfaceName + widgetName must be present and unique within a given logical surface
//
//
//
// Modifiers
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
// The following modifiers, if present:
//  must be contained in the modifier part of the action address
//  must be contained only in the modifier part of the action address
//  in the case of combos, must be in the same order as listed below -- e.g. ShiftOptionControlAlt for the full meal deal
//

const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                        THE RULES
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FXWindow
{
    MediaTrack* track = nullptr;;
    int index = 0;
    
    FXWindow(MediaTrack* aTrack, int anIndex) : track(aTrack), index(anIndex) {}
};

struct MapEntry
{
    string surfaceName;
    string widgetName;
    string paramName;
    
    MapEntry(string aSurfaceName, string aWidgetName, string aParamName) : surfaceName(aSurfaceName), widgetName(aWidgetName), paramName(aParamName) {}
};

struct FXMap
{
private:
    string name;
    vector<MapEntry> entries_;
    
public:
    FXMap(string aName) : name(aName) {}
    
    string GetName() { return name; }
    vector<MapEntry>& GetMapEntries() { return entries_; }
    
    void AddEntry(string surfaceName, string widgetName, string paramName)
    {
        entries_.push_back(MapEntry(surfaceName, widgetName, paramName));
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
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    string GetGUID() { return GUID_; }
    RealSurface* GetRealSurface() { return realSsurface_; }
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
    MidiWidget(string GUID, RealSurface* surface, string name, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : GUID_(GUID), realSsurface_(surface), name_(name), midiPressMessage_(press), midiReleaseMessage_(release) {}

public:
    virtual ~MidiWidget() {};
    
    virtual double GetMinDB() { return -72.0; }
    virtual double GetMaxDB() { return 12.0; }
    string GetName() { return name_; }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
    }
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
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
    string bankGroup_ = "";
    int numBankableChannels_ = 0;
    vector<RealSurfaceChannel*> channels_;
    map<string, MidiWidget*> widgets_;
    
    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
    bool zoom_ = false;
    bool scrub_ = false;
    
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
    
    string ActionAddressFor(string GUID, string widgetName)
    {
        string currentModifiers = "";
        
        if(widgetName != Shift && widgetName != Option && widgetName != Control && widgetName != Alt)
            currentModifiers = CurrentModifers();
        
        return GUID + GetName() + currentModifiers + widgetName;
    }
    
    RealSurface(LogicalSurface* logicalSurface, string bankGroup, const string name, int numBankableChannels) : logicalSurface_(logicalSurface), bankGroup_(bankGroup), name_(name),  numBankableChannels_(numBankableChannels) {}
    
public:
    virtual ~RealSurface() {};
    
    const string GetName() const { return name_; }
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    string GetBankGroup() { return bankGroup_; }
    vector<RealSurfaceChannel*> & GetChannels() { return channels_; }
    int GetNumBankableChannels() { return numBankableChannels_; }
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }
        
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    virtual void SendMidiMessage(int first, int second, int third) {}
    
    virtual void RunAndUpdate() {}
    
    void ClearChannels()
    {
        channels_.clear();
    }

    void AddChannel(RealSurfaceChannel*  channel)
    {
        channels_.push_back(channel);
    }
    
    void ClearWidgets()
    {
        widgets_.clear();
    }

    void AddWidget(MidiWidget* widget)
    {
        widgets_[widget->GetName()] = widget;
    }
    
    void SetWidgetGUID(string widgetName, string GUID)
    {
        if(widgets_.count(widgetName) > 0)
            widgets_[widgetName]->SetGUID(GUID);
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
        control_ = value; ForceUpdateWidgets();
    }
    
    void SetAlt(bool value)
    {
        alt_ = value; ForceUpdateWidgets();
    }
    
    void SetZoom(bool value)
    {
        zoom_ = value; ForceUpdateWidgets();
    }
    
    void SetScrub(bool value)
    {
        scrub_ = value; ForceUpdateWidgets();
    }

    // to Widgets ->
    virtual void UpdateWidgets()
    {
        for(auto const& [name, widget] : widgets_ )
            widget->Update();
    }

    virtual void ForceUpdateWidgets()
    {
        for(auto const& [name, widget] : widgets_ )
            widget->ForceUpdate();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string surfaceName, string widgetName);
    void UpdateAction(string surfaceName, string widgetName);
    void ForceUpdateAction(string surfaceName, string widgetName);
    void CycleAction(string surfaceName, string widgetName);
    void RunAction(string surfaceName, string widgetName, double value);
    
    // to Widgets ->
    void SetWidgetValue(string widgetName, double value)
    {
        if(widgets_.count(widgetName) > 0)
            widgets_[widgetName]->SetValue(value);
    }
 
    void SetWidgetValue(string widgetName, double value, int mode)
    {
        if(widgets_.count(widgetName) > 0)
            widgets_[widgetName]->SetValue(value, mode);
    }

    void SetWidgetValue(string widgetName, string value)
    {
        if(widgets_.count(widgetName) > 0)
            widgets_[widgetName]->SetValue(value);
    }
    
    void SetWidgetValueToZero(string widgetName)
    {
        if(widgets_.count(widgetName) > 0)
            widgets_[widgetName]->SetValueToZero();
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
    vector<string> widgetNames_;

    RealSurface* GetRealSurface() { return realSurface_; }

public:
    virtual ~RealSurfaceChannel() {}
    
    RealSurfaceChannel(string GUID, RealSurface* surface, bool isMovable) : GUID_(GUID), realSurface_(surface), isMovable_(isMovable) {}
    
    string GetGUID() { return GUID_; }
    bool GetIsMovable() { return isMovable_; }
    
    void SetIsMovable(bool isMovable)
    {
        isMovable_ = isMovable;
    }
    
    void AddWidget(MidiWidget* widget)
    {
        widgetNames_.push_back(widget->GetName());
        GetRealSurface()->AddWidget(widget);
    }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
        
        for (auto widgetName : widgetNames_)
        {
            GetRealSurface()->SetWidgetGUID(widgetName, GUID);

            if(GUID_ == "")
                GetRealSurface()->SetWidgetValueToZero(widgetName);
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
    virtual void Run(double value, string surfaceName, string widgetName) {}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager;
class TrackGUIDAssociation;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LogicalSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfManager* manager_ = nullptr;
    map<string, FXMap *> fxMaps_;
    vector<RealSurface*> realSurfaces_;
    map<string, Action*> actions_;
    vector<TrackGUIDAssociation*> trackGUIDAssociations_;
    vector<string> trackGUIDs_;
    vector<FXWindow> openFXWindows_;
    
    int numLogicalChannels_ = 0;
    int trackOffset_ = 0;
    bool VSTMonitor_ = false;

    void InitializeFXMaps();
    void InitializeSurfaces();
    void BuildActions();
    void BuildCSurfWidgets();

    int GetNumLogicalChannels() { return numLogicalChannels_; }
    
    bool DidTrackListChange()
    {
        if(trackGUIDs_.size() == 0)
            return false;               // We have no idea if track list changed, we have been called way too early, there's nothing to compare, just return false
        
        if(trackGUIDs_.size() != DAW::GetNumTracks() + 1) // +1 is for Master
            return true; // list sizes disagree
        
        for(int i = 0; i < trackGUIDs_.size(); i++)
            if(trackGUIDs_[i] != DAW::GetTrackGUIDAsString(i))
                return true;

        return false;
    }
    
    void AddFXMap(FXMap* fxMap)
    {
        fxMaps_[fxMap->GetName()] = fxMap;
    }
    
    void AddRealSurface(RealSurface* realSurface)
    {
        realSurfaces_.push_back(realSurface);
    }
 
    void AddTrackGUIDAssociation(TrackGUIDAssociation* trackGUIDAssociation)
    {
        trackGUIDAssociations_.push_back(trackGUIDAssociation);
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

    void MapWidgetsToFX(MediaTrack *trackid);
    void MapFX(MediaTrack* track);

    void Initialize()
    {
        InitializeSurfaces();
        InitializeFXMaps();
        BuildActions();
        BuildCSurfWidgets();
    }
    
    void RemoveAction(string actionAddress)
    {
        if(actions_.count(actionAddress) > 0)
            actions_.erase(actionAddress);
    }

    void OnTrackSelection(MediaTrack* track)
    {
        if(DAW::CountSelectedTracks(nullptr) == 1)
            MapWidgetsToFX(track);
    }

    void RunAndUpdate()
    {
        for(auto & surface : realSurfaces_)
            surface->RunAndUpdate();
    }

    void TrackListChanged()
    {
        if(DidTrackListChange())
            BuildActions();
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
            {
                movableTracks.push_back("");
            }
            else if(find(immovableTracks.begin(), immovableTracks.end(), DAW::GetTrackGUIDAsString(currentOffset)) == immovableTracks.end())
            {
                movableTracks.push_back(DAW::GetTrackGUIDAsString(currentOffset++));
            }
            else
            {
                currentOffset++;
            }
        }
        
        currentOffset = 0;
        
        for(auto* surface : realSurfaces_)
            for(auto* channel : surface->GetChannels())
                if(channel->GetIsMovable() == true)
                    channel->SetGUID(movableTracks[currentOffset++]);
        
        ForceUpdate();
    }

    void SetShift(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetShift(value);
    }
    
    void SetOption(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetOption(value);
    }
    
    void SetControl(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetControl(value);
    }
    
    void SetAlt(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetAlt(value);
    }
    
    void SetZoom(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetZoom(value);
    }
    
    void SetScrub(string surfaceName, bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetScrub(value);
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
    
    void ImmobilizeSelectedTracks()
    {
        for(auto * surface : realSurfaces_)
            for(auto * channel : surface->GetChannels())
                if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(channel->GetGUID()), "I_SELECTED"))
                    channel->SetIsMovable(false);
    }
    
    void MobilizeSelectedTracks()
    {
        for(auto * surface : realSurfaces_)
            for(auto * channel : surface->GetChannels())
                if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(channel->GetGUID()), "I_SELECTED"))
                    channel->SetIsMovable(true);
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
    
    void RunAction(string actionAddress, double value, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            actions_[actionAddress]->Run(value, surfaceName, widgetName);
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
class TrackGUIDAssociation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string GUID_ = "";
    LogicalSurface* logicalSurface_= nullptr;
    vector<string> FXActionAddresses_;
    vector<string> sendActionAddresses_;
    
public:
    TrackGUIDAssociation(string GUID, LogicalSurface* logicalSurface) : GUID_(GUID), logicalSurface_(logicalSurface) {}
    
    string GetGUID() { return GUID_; }
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
   
    void ClearSendActionAddresses()
    {
        sendActionAddresses_.clear();
    }
   
    void AddSendActionAddress(string actionAddress)
    {
        sendActionAddresses_.push_back(actionAddress);
    }
    
    void ClearFXActionAddresses()
    {
        // Remove any existing actionAddress entries for this trackGUID
        for(auto actionAddresss : FXActionAddresses_)
            GetLogicalSurface()->RemoveAction(actionAddresss);
        
        FXActionAddresses_.clear();
    }
    
    void AddFXActionAddress(string actionAddress)
    {
        FXActionAddresses_.push_back(actionAddress);
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
            // init the logical surface maps here
            
            LogicalSurface* logicalSurface = new LogicalSurface(this);
            logicalSurface->Initialize();
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
        if(logicalSurfaces_.size() != 0) // seems we need to protect against prematurely early calls
            for(auto & surface : logicalSurfaces_)
                surface->TrackListChanged();
    }

    void TrackFXListChanged(MediaTrack* trackid)
    {
        if(logicalSurfaces_.size() != 0) // seems we need to protect against prematurely early calls
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
    : RealSurface(surface, "", "OSC", 8) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WebCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~WebCSurf() {};
    
    WebCSurf(const string name, LogicalSurface* surface)
    : RealSurface(surface, "", "Web", 8) {};
};

#endif /* control_surface_integrator.h */
