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

using namespace std;

#include "control_surface_integrator_Reaper.h"

const string LogicalCSurf = "LogicalCSurf";

const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";

struct WidgetAddress
{
    string surfaceName;
    string widgetName;
    
    WidgetAddress(string aSurfaceName, string aWidgetName) : surfaceName(aSurfaceName), widgetName(aWidgetName) {}
};

struct MapEntry
{
    string widgetName;
    string paramName;
    
    MapEntry(string aWidgetName, string aParamName) : widgetName(aWidgetName), paramName(aParamName) {}
};

struct FXMap
{
private:
    string name;
    vector<MapEntry> entries_;
    
public:
    FXMap(string aName) : name(aName) {}
    
    string GetName() { return name; }
    
    void AddEntry(string widgetName, string paramName) { entries_.push_back(MapEntry(widgetName, paramName));   }
    
    vector<MapEntry>& GetMapEntries() { return entries_; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager;
class LogicalSurface;
class Interactor;
class Action;
class RealCSurf;
class CSurfChannel;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_= "";
    string GUID_ = "";
    RealCSurf* surface_ = nullptr;
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
public:
    virtual ~MidiWidget() {};
    
    MidiWidget(string name, RealCSurf* surface, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : name_(name), surface_(surface), midiPressMessage_(press), midiReleaseMessage_(release) {}
    
    string GetName() { return name_; }
    
    string GetGUID() { return GUID_; }
    
    RealCSurf* GetSurface() { return surface_; }

    virtual double GetMinDB() { return 0.0; }
    
    virtual double GetMaxDB() { return 1.0; }
    
    void SetGUID(string GUID) { GUID_ = GUID; }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}

    void Update();
    void ForceUpdate();
    virtual void SetValue(double value) {}
    virtual void SetValue(double value, int displaymode) {}
    virtual void SetValue(string value) {}
    virtual void SetValueToZero() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SubChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string subGUID_ = "";
    vector<string> widgetNames_;
    
public:
    SubChannel(string subGUID) : subGUID_(subGUID) {}
    
    string GetSubGUID() { return subGUID_; }
    
    vector<string> GetWidgetNames() { return widgetNames_; }
    
    void AddWidgetName(string name)
    {
        widgetNames_.push_back(name);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string GUID_ = "";
    RealCSurf* surface_= nullptr;
    bool isMovable_ = true;
    vector<MidiWidget*> widgets_;
    bool shouldMapSubChannels_ = false;
    vector<SubChannel*> subChannels_;

public:
    virtual ~CSurfChannel() {}
    
    CSurfChannel(string GUID, RealCSurf* surface, bool isMovable, bool shouldMapSubChannels) : GUID_(GUID), surface_(surface), isMovable_(isMovable), shouldMapSubChannels_(shouldMapSubChannels) {}
    
    string GetGUID() { return GUID_; }

    RealCSurf* GetSurface() { return surface_; }
    
    bool GetIsMovable() { return isMovable_; }
    
    vector<MidiWidget*> GetWidgets() { return widgets_; }
    
    vector<SubChannel*> GetSubChannels() { return subChannels_; }

    void SetIsMovable(bool isMovable) { isMovable_ = isMovable; }
    
    void ClearSubChannels() {  subChannels_.clear(); }
    
    virtual void OnTrackSelection(MediaTrack *trackid);
    
    void AddWidget(MidiWidget* widget)
    {
        widgets_.push_back(widget);
    }
    
    void AddSubChannel(SubChannel* subChannel)
    {
        subChannels_.push_back(subChannel);
    }

    void SetGUID(string GUID)
    {
        GUID_ = GUID;
    
        if(GUID_ == "")
        {
           for(auto* widget : widgets_)
                widget->SetValueToZero();
        }
        /* GAW TBD Don't know if this is necessary
        else if(shouldMapSubChannels_ && DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(GetGUID()), "I_SELECTED") && DAW::CountSelectedTracks(nullptr) == 1  )
        {
            MapFX(DAW::GetTrackFromGUID(GUID));
        }
         */
    }
    
    void MapFX(MediaTrack *trackid);

    void ProcessMidiMessage(const MIDI_event_ex_t* evt);
    
    // to Widgets ->
    virtual void UpdateWidgets();
    virtual void ForceUpdateWidgets();
    
    // to Widgets ->
    virtual void SetWidgetValue(string name, double value);
    virtual void SetWidgetValue(string name, double value, int mode);
    virtual void SetWidgetValue(string name, string value);
    
    void SetWidgetValue(string subGUID, string name, double value);
    void SetWidgetValue(string subGUID, string name, double value, int mode);
    void SetWidgetValue(string subGUID, string name, string value);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealCSurf
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    const string name_ = "";
    LogicalSurface* logicalSurface_ = nullptr;
    string bankGroup_ = "";
    int numBankableChannels_ = 0;
    vector<CSurfChannel*> channels_;
    
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
    
    string ActionAddressFor(string name)
    {
        if(name == Shift || name == Option || name == Control || name == Alt)
            return name;
        
        if(CurrentModifers() != "")
            return CurrentModifers() + "#" + name;
        else return name;
    }
    
    string ActionAddressFor(string GUID, string name)
    {
        if(name == Shift || name == Option || name == Control || name == Alt)
            return name;
        else
            return GUID + GetName() + CurrentModifers() + name;
    }
    
public:
    virtual ~RealCSurf() {};
    
    RealCSurf(const string name, LogicalSurface* logicalSurface, string bankGroup, int numBankableChannels) : name_(name), logicalSurface_(logicalSurface), bankGroup_(bankGroup), numBankableChannels_(numBankableChannels) {}
    
    const string GetName() const { return name_; }

    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }

    vector<CSurfChannel*> & GetChannels() { return channels_; }

    int GetNumBankableChannels() { return numBankableChannels_; }
    
    double GetCurrentNormalizedValue(string GUID, string name);
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    
    virtual void SendMidiMessage(int first, int second, int third) {}

    virtual void OnTrackSelection(MediaTrack *track);
    
    virtual void RunAndUpdate() {}
    
    virtual void AddChannel(CSurfChannel*  channel)
    {
        channels_.push_back(channel);
    }
       
    void SetShift(bool value) { shift_ = value; ForceUpdateWidgets(); }
    void SetOption(bool value) { option_ = value; ForceUpdateWidgets(); }
    void SetControl(bool value) { control_ = value; ForceUpdateWidgets(); }
    void SetAlt(bool value) { alt_ = value; ForceUpdateWidgets(); }
    void SetZoom(bool value) { zoom_ = value; ForceUpdateWidgets(); }
    void SetScrub(bool value) { scrub_ = value; ForceUpdateWidgets(); }
    
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }

    // to Widgets ->
    virtual void UpdateWidgets();
    virtual void ForceUpdateWidgets();

    // to Actions ->
    void UpdateAction(string GUID, string name);
    void ForceUpdateAction(string GUID, string name);
    void CycleAction(string trackGUID, string name);
    void RunAction(string GUID, string name, double value);
    
    void UpdateAction(string GUID, string subGUID, string name);
    void ForceUpdateAction(string GUID, string subGUID, string name);
    void CycleAction(string trackGUID, string subGUID, string name);
    void RunAction(string GUID, string subGUID, string name, double value);

    // to Widgets ->
    void SetWidgetValue(string GUID, string name, double value);
    void SetWidgetValue(string GUID, string name, double value, int mode);
    void SetWidgetValue(string GUID, string name, string value);
    
    void SetWidgetValue(string GUID, string subGUID, string name, double value);
    void SetWidgetValue(string GUID, string subGUID, string name, double value, int mode);
    void SetWidgetValue(string GUID, string subGUID, string name, string value);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";
    Interactor* interactor_ = nullptr;

protected:
    virtual void SetWidgetValue(string surfaceName, double value) {}
    virtual void SetWidgetValue(string surfaceName, string value) {}

    Action(string name, Interactor* interactor) : name_(name), interactor_(interactor) {}
    
public:
    virtual ~Action() {}
    
    string GetName() { return name_; }

    Interactor* GetInteractor() { return interactor_; }
    
    virtual string GetAlias() { return name_; }
    
    virtual int GetDisplayMode() { return 0; }
    
    virtual double GetCurrentNormalizedValue () { return 0.0; }

    virtual void Add(Action* action) {}
    
    virtual void Update(string surfaceName) {}
    virtual void ForceUpdate(string surfaceName) {}
    virtual void Cycle(string surfaceName) {}
    virtual void Run(string surfaceName, double value) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SubInteractor;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Interactor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string GUID_ = "";
    LogicalSurface* logicalSurface_ = nullptr;
    map <string, vector<Action*>> actions_;
    vector<SubInteractor*> fxSubInteractors_;
    vector<SubInteractor*> sendSubInteractors_;

public:
    virtual ~Interactor() {}
    
    Interactor(string GUID, LogicalSurface* logicalSurface) : GUID_(GUID), logicalSurface_(logicalSurface) {}

    virtual string GetGUID() { return GUID_; }

    virtual LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    
    virtual MediaTrack* GetTrack() { return DAW::GetTrackFromGUID(GetGUID()); }
    
    vector<SubInteractor*> GetFXSubInteractors() { return fxSubInteractors_; }
    
    void ClearFXSubInteractors() { fxSubInteractors_.clear(); }
    
    vector<SubInteractor*> GetSendSubInteractors() { return sendSubInteractors_; }

    void ClearSendSubInteractors() { sendSubInteractors_.clear(); }
    
    virtual int GetIndex() { return 0; }
    
    double GetCurrentNormalizedValue(string name)
    {
        if(actions_[name].size() > 0)
            return (actions_[name])[0]->GetCurrentNormalizedValue();
        else
            return 0.0;
    }
    
    void AddAction(Action* action)
    {
        actions_[action->GetName()].push_back(action);
    }
    
    void AddAliasedAction(Action* action)
    {
        actions_[action->GetAlias()].push_back(action);
    }
    
    void AddFXSubInteractor(SubInteractor* subInteractor)
    {
        fxSubInteractors_.push_back(subInteractor);
    }
    
    void AddSendSubInteractor(SubInteractor* subInteractor)
    {
        sendSubInteractors_.push_back(subInteractor);
    }
    
    // to Actions ->
    void UpdateAction(string surfaceName, string name);
    void ForceUpdateAction(string surfaceName, string name);
    void CycleAction(string surfaceName, string name);
    void RunAction(string surfaceName, string name, double value);
    
    void UpdateAction(string surfaceName, string subGUID, string name);
    void ForceUpdateAction(string surfaceName, string subGUID, string name);
    void CycleAction(string surfaceName, string subGUID, string name);
    void RunAction(string surfaceName, string subGUID, string name, double value);
    
    // to Widgets ->
    virtual void SetWidgetValue(string surfaceName, string name, double value);
    virtual void SetWidgetValue(string surfaceName, string name, double value, int mode);
    virtual void SetWidgetValue(string surfaceName, string name, string value);
    
    void SetWidgetValue(string surfaceName, string SubGUID, string name, double value);
    void SetWidgetValue(string surfaceName, string SubGUID, string name, double value, int mode);
    void SetWidgetValue(string surfaceName, string SubGUID, string name, string value);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SubInteractor : public Interactor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string subGUID_ = "";
    int index_ = 0;
    Interactor* interactor_ = nullptr;
    
    Interactor* GetInteractor() { return interactor_; }
    
public:
    virtual ~SubInteractor() {}

    SubInteractor(string subGUID, int index, Interactor* interactor) : Interactor(interactor->GetGUID(), interactor->GetLogicalSurface()), subGUID_(subGUID), index_(index), interactor_(interactor) {}
    
    virtual string GetSubGUID() { return subGUID_; }
    virtual int GetIndex() override { return index_; }
    
    virtual void SetWidgetValue(string surfaceName, string name, double value) override;
    virtual void SetWidgetValue(string surfaceName, string name, double value, int mode) override;
    virtual void SetWidgetValue(string surfaceName, string name, string value) override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LogicalSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfManager* manager_ = nullptr;
    map<string, FXMap *> fxMaps_;
    vector<RealCSurf*> surfaces_;
    vector<Interactor*> trackInteractors_;
    Interactor* logicalSurfaceInteractor_ = nullptr;
    int numLogicalChannels_ = 0;
    int trackOffset_ = 0;
    bool VSTMonitor_ = false;

    void BuildTrackInteractors();
    void BuildTrackInteractors2();
    void BuildCSurfWidgets();

    int GetNumLogicalChannels() { return numLogicalChannels_; }
    
    bool DidTrackListChange();
    
    void AddFXMap(FXMap* fxMap)
    {
        fxMaps_[fxMap->GetName()] = fxMap;
    }
    
    void AddTrackInteractor(Interactor* interactor)
    {
        trackInteractors_.push_back(interactor);
    }
    
    void AddSurface(RealCSurf* surface)
    {
        surfaces_.push_back(surface);
    }
    
    void RebuildTrackInteractors()
    {
        trackInteractors_.clear();
        BuildTrackInteractors();
        RefreshLayout();
    }
    
public:
    LogicalSurface(CSurfManager* manager) : manager_(manager) {}

    CSurfManager* GetManager() { return manager_; }
    
    map<string, FXMap *> GetFXMaps() { return fxMaps_; }
    
    bool GetVSTMonitor() { return VSTMonitor_; }

    RealCSurf* GetSurface(string name)
    {
        for(auto* surface : surfaces_)
            if(surface->GetName() == name)
                return surface;
        
        return nullptr;
    }
    
    void OnTrackSelection(MediaTrack* track)
    {
        for(auto& surface : surfaces_)
            surface->OnTrackSelection(track);
    }

    void RunAndUpdate()
    {
        for(auto & surface : surfaces_)
            surface->RunAndUpdate();
    }

    void TrackListChanged()
    {
        if(DidTrackListChange())
            RebuildTrackInteractors();
    }

    void Initialize();
    void Initialize2();
    void InitializeFXMaps();
    void InitializeSurfaces();
    void InitializeLogicalCSurfInteractor();
    
    void RefreshLayout();

    void SetShift(string surfaceName, bool value);
    void SetOption(string surfaceName, bool value);
    void SetControl(string surfaceName, bool value);
    void SetAlt(string surfaceName, bool value);
    
    void SetZoom(string surfaceName, bool value);
    void SetScrub(string surfaceName, bool value);
    
    void AdjustTrackBank(int stride);
    void ImmobilizeSelectedTracks();
    void MobilizeSelectedTracks();
    
    void TrackFXListChanged(MediaTrack* track);
    void MapFX(MediaTrack* track);

    // to Widgets ->
    void ForceUpdate();
    
    // to Actions ->
    double GetCurrentNormalizedValue(string trackGUID, string name);

    void UpdateAction(string surfaceName, string GUID, string name);
    void ForceUpdateAction(string surfaceName, string GUID, string name);
    void CycleAction(string surfaceName, string trackGUID, string name);
    void RunAction(string actionAddress, double value, WidgetAddress widgetAddress);
    
    void UpdateAction(string surfaceName, string GUID, string subGUID, string name);
    void ForceUpdateAction(string surfaceName, string GUID, string subGUID, string name);
    void CycleAction(string surfaceName, string trackGUID, string subGUID, string name);
    void RunAction(string surfaceName, string trackGUID, string subGUID, string name, double value);
    
    // to Widgets ->
    void SetWidgetValue(string surfaceName, string GUID, string name, double value);
    void SetWidgetValue(string surfaceName, string GUID, string name, double value, int mode);
    void SetWidgetValue(string surfaceName, string GUID, string name, string value);
    
    void SetWidgetValue(string surfaceName, string GUID, string subGUID, string name, double value);
    void SetWidgetValue(string surfaceName, string GUID, string subGUID, string name, double value, int mode);
    void SetWidgetValue(string surfaceName, string GUID, string subGUID, string name, string value);
    
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
            // init the maps here
            
            LogicalSurface* logicalSurface = new LogicalSurface(this);
            logicalSurface->Initialize();
            logicalSurfaces_.push_back(logicalSurface);
            
            logicalSurface = new LogicalSurface(this);
            logicalSurface->Initialize2();
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
    
    bool GetLazyInitialized() { return lazyInitialized_; }
    
    double GetFaderMaxDB()
    {
        return GetPrivateProfileDouble("slidermaxv");
    }
    
    double GetFaderMinDB()
    {
        return GetPrivateProfileDouble("sliderminv");
    }
    
    double GetVUMaxDB()
    {
        return GetPrivateProfileDouble("vumaxvol");
    }
    
    double GetVUMinDB()
    {
        return GetPrivateProfileDouble("vuminvol");
    }
    
    void OnTrackSelection(MediaTrack *trackid)
    {
        logicalSurfaces_[currentLogicalSurfaceIndex_]->OnTrackSelection(trackid);
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

    void TrackListChanged() // tell current map
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
class OSCCSurf : public RealCSurf
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~OSCCSurf() {};
    
    OSCCSurf(const string name, LogicalSurface* surface)
    : RealCSurf(name, surface, "OSC", 8) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WebCSurf : public RealCSurf
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~WebCSurf() {};
    
    WebCSurf(const string name, LogicalSurface* surface)
    : RealCSurf(name, surface, "Web", 8) {};
};

#endif /* control_surface_integrator.h */
