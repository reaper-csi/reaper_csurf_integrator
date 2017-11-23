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

const string Volume = "Volume";
const string Pan = "Pan";

const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";

const string Delimiter = "#";

template <class Container>
static void SplitString(string& str, Container& cont, char delim = ' ')
{
    stringstream ss(str);
    string token;
    while (getline(ss, token, delim))
        cont.push_back(token);
};

struct MapEntry
{
    string widget;
    string param;
    
    MapEntry(string aWidget, string aParam) : widget(aWidget), param(aParam) {}
};

struct FXMap
{
private:
    string name_;
    vector<MapEntry> entries_;
    
public:
    FXMap(string name) : name_(name) {}
    
    string GetName() { return name_; }
    
    void AddEntry(string widget, string param) { entries_.push_back(MapEntry(widget, param));   }
    
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
    CSurfChannel* channel_ = nullptr;
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
public:
    virtual ~MidiWidget() {};
    
    MidiWidget(string name, CSurfChannel* channel, string GUID, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : name_(name), channel_(channel), GUID_(GUID), midiPressMessage_(press), midiReleaseMessage_(release) {}
    
    RealCSurf* GetSurface();
    
    virtual CSurfChannel* GetChannel() { return channel_; }

    virtual double GetMinDB() { return 0.0; }
    
    virtual double GetMaxDB() { return 1.0; }
    
    string GetName();

    void SetGUID(string GUID)
    {
        GUID_ = GUID;
        ForceUpdate();
    }
    
    string GetGUID() { return GUID_; }

    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}

    void Update();
    void ForceUpdate();
    virtual void SetValue(double value) {}
    virtual void SetValue(double value, int displaymode) {}
    virtual void SetValue(string value) {}
    virtual void SetValueToZero() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    RealCSurf* surface_= nullptr;
    string GUID_ = "";
    vector<MidiWidget*> widgets_;
    
    void SetWidgetsToGUID(string GUID)
    {
        for(auto & widget : widgets_)
        {
            widget->SetValueToZero(); // We're not sure what the new mappings are, based on the GUID change, so zero everything to avoid spurious lights left on, etc.
            widget->SetGUID(GUID);
        }
    }
    
public:
    virtual ~CSurfChannel() {}
    
    CSurfChannel(RealCSurf* surface, string GUID) : surface_(surface), GUID_(GUID) {}
    
    RealCSurf* GetSurface() { return surface_; }
    
    DAW* GetDAW();
    
    string GetGUID() { return GUID_; }
    
    vector<MidiWidget*> GetWidgets() { return widgets_; }
    
    virtual void OnTrackSelection(MediaTrack *trackid) {};
    
    void AddWidget(MidiWidget* widget)
    {
        widgets_.push_back(widget);
    }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
        
        SetWidgetsToGUID(GUID);
    }
    
    void ProcessMidiMessage(const MIDI_event_ex_t* evt);
    
    virtual void Update();
    virtual void ForceUpdate();
    void SetWidgetValue(string trackGUID, string name, double value);
    void SetWidgetValue(string trackGUID, string name, double value, int mode);
    void SetWidgetValue(string trackGUID, string name, string value);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UniquelySelectedCSurfChannel : public CSurfChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    void MapFX(MediaTrack *trackid);
    
public:
    virtual ~UniquelySelectedCSurfChannel() {}
    
    UniquelySelectedCSurfChannel(RealCSurf* surface, int channelNumber, string trackGUID) :CSurfChannel(surface, trackGUID) {}
    
    virtual void OnTrackSelection(MediaTrack *trackid) override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealCSurf
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    LogicalSurface* logicalSurface_ = nullptr;
    const string name_ = "";
    const int numChannels_ = 0;
    vector<CSurfChannel*> channels_;

public:
    virtual ~RealCSurf() {};
    
    RealCSurf(LogicalSurface* logicalSurface,  const string name, const int numChannels) : logicalSurface_(logicalSurface),  name_(name), numChannels_(numChannels) {}
    
    CSurfManager* GetManager();
    
    LogicalSurface* GetLogicalSurface() { return logicalSurface_; }
    
    vector<CSurfChannel*> & GetChannels() { return channels_; }
    
    const string GetName() const { return name_; }
    
    const int GetNumChannels() const { return numChannels_; }

    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    
    virtual void SendMidiMessage(int first, int second, int third) {}

    virtual void OnTrackSelection(MediaTrack *trackid);
    
    virtual void RunAndUpdate() {}
    
    virtual void AddChannel(CSurfChannel*  channel)
    {
        channels_.push_back(channel);
    }
    
    virtual void Update();
    virtual void ForceUpdate();
    void SetWidgetValue(string trackGUID, string name, double value);
    void SetWidgetValue(string trackGUID, string name, double value, int mode);
    void SetWidgetValue(string trackGUID, string name, string value);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfManager* manager_ = nullptr;
    string name_ = "";
    Interactor* interactor_ = nullptr;

protected:
    Interactor* GetInteractor() { return interactor_; }
    virtual void SetWidgetValue(double value) {}
    virtual void SetWidgetValue(string value) {}

    Action(string name, CSurfManager* manager, Interactor* interactor) : name_(name), manager_(manager), interactor_(interactor) {}
    
public:
    virtual ~Action() {}

    CSurfManager* GetManager() { return manager_; }
    
    DAW* GetDAW();
    
    string GetName() { return name_; }
    
    virtual string GetAlias() { return name_; }
    
    virtual int GetDisplayMode() { return 0; }
    
    virtual double GetCurrentNormalizedValue () { return 0.0; }

    virtual void AddAction(Action* action) {}
    
    virtual void Update() {}
    virtual void ForceUpdate() {}
    virtual void RunAction(double value) {}
    virtual void Cycle() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Interactor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfManager* manager_ = nullptr;
    map <string, vector<Action*>> actions_;
    string GUID_ = "";
    string trackGUID_ = "";
    int index_ = 0;

public:
    Interactor(CSurfManager* manager, string GUID) : manager_(manager), GUID_(GUID), trackGUID_(GUID) {}
    
    Interactor(CSurfManager* manager, string GUID, string trackGUID, int index) : manager_(manager), GUID_(GUID), trackGUID_(trackGUID), index_(index) {}
    
    CSurfManager* GetManager() { return manager_; }
    
    string GetGUID() { return GUID_; }
    
    string GetTrackGUID() { return trackGUID_; }

    MediaTrack* GetTrack();
    
    int GetIndex() { return index_; }
    
    DAW* GetDAW();
    
    double GetCurrentNormalizedValue(string name);
    
    void AddAction(Action* action)
    {
        actions_[action->GetName()].push_back(action);
    }
    
    void AddAliasedAction(Action* action)
    {
        actions_[action->GetAlias()].push_back(action);
    }
    
    void Update(string name);
    void ForceUpdate(string name);
    void RunAction(string name, double value);
    void CycleAction(string name);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LogicalSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfManager* manager_ = nullptr;
    map<string, FXMap *> fxMaps_;
    vector<RealCSurf*> surfaces_;
    vector<Interactor*> interactors_;
    vector<string> immovableTrackGUIDs_;
    bool isSynchronized_ = false;
    bool isFlipped_ = false;
    int trackOffset_ = 0;
    
    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
    bool zoom_ = false;
    bool scrub_ = false;

    void BuildTrackInteractors();
    void BuildTrackInteractors2();
    void BuildCSurfWidgets();

    // There is an immovable GUID slot for each channel, so by definition immovableTrackGUIDs_.size is number of channels
    int NumChannels() { return (int)immovableTrackGUIDs_.size(); }
    
    bool DidTrackListChange();

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
    
    string FlipNameFor(string name)
    {
        if(name == Volume && isFlipped_)
            return Pan;
        else if(name == Pan && isFlipped_)
            return Volume;
        else
            return name;
    }
    
    string ModifiedNameFor(string name)
    {
        if(name == Shift || name == Option || name == Control || name == Alt)
            return name;
        
        if(CurrentModifers() != "")
            return CurrentModifers() + "#" + name;
        else return name;
    }
    
    string UnmodifiedNameFor(string name)
    {
        vector<string> tokens;
        
        SplitString(name, tokens, '#');
        
        if(tokens.size() == 1)
            return name;
        else
            return tokens[1];
    }
    
    bool IsOKToSetWidget(string name)
    {
        vector<string> tokens;
        
        SplitString(name, tokens, '#');
        
        if((tokens.size() == 1 && CurrentModifers() == "") || (tokens.size() > 0 && tokens[0] == CurrentModifers()))
            return true;
        else
            return false;
    }
    
    CSurfChannel* Channel(int wantChannel)
    {
        int offset = 0;
        
        for(auto & surface : surfaces_)
        {
            if(surface->GetNumChannels() + offset - 1  < wantChannel)
                offset += surface->GetNumChannels();
            else
                return surface->GetChannels()[wantChannel - offset];
        }
        
        return nullptr;
    }
    
    bool isInImmovableTrackGUIDS(string trackGUID)
    {
        for(auto & immovableTrackGUID : immovableTrackGUIDs_)
            if(immovableTrackGUID == trackGUID)
                return true;
        
        return false;
    }
    
    void SetSynchronized(bool isSynchronized)
    {
        isSynchronized_ = isSynchronized;
    }
    
    void AddFXMap(FXMap* fxMap)
    {
        fxMaps_[fxMap->GetName()] = fxMap;
    }
    
    void AddInteractor(Interactor* interactor)
    {
        interactors_.push_back(interactor);
    }
    
    void AddSurface(RealCSurf* surface)
    {
        surfaces_.push_back(surface);
    }
    
    void RebuildInteractors()
    {
        interactors_.clear();
        BuildTrackInteractors();
        RefreshLayout();
    }
    
public:
    LogicalSurface(CSurfManager* manager) : manager_(manager) {}

    CSurfManager* GetManager() { return manager_; }
    
    DAW* GetDAW();
    
    map<string, FXMap *> GetFXMaps() { return fxMaps_; }
    
    bool IsZoom() { return zoom_; }
    
    bool IsScrub() { return scrub_; }
    
    bool IsFlipped() { return isFlipped_; }
    
    void OnTrackSelection(MediaTrack* trackid)
    {
        for(auto& surface : surfaces_)
            surface->OnTrackSelection(trackid);
    }

    void RunAndUpdate()
    {
        for(auto & surface : surfaces_)
            surface->RunAndUpdate();
    }

    void TrackListChanged()
    {
        if(DidTrackListChange())
            RebuildInteractors();
    }
    
    void ToggleFlipped(string name)
    {
        isFlipped_ = ! isFlipped_;
        
        SetWidgetValue("", name, isFlipped_);
    }

    void Initialize();
    void Initialize2();
    void InitializeFXMaps();
    void InitializeSurfaces();
    void InitializeLogicalCSurfInteractors();
    
    double GetCurrentNormalizedValue(string trackGUID, string name);
    
    void SetShift(bool value) { shift_ = value; ForceUpdate(); }
    void SetOption(bool value) { option_ = value; ForceUpdate(); }
    void SetControl(bool value) { control_ = value; ForceUpdate(); }
    void SetAlt(bool value) { alt_ = value; ForceUpdate(); }
    void SetZoom(bool value) { zoom_ = value; ForceUpdate(); }
    void SetScrub(bool value) { scrub_ = value; ForceUpdate(); }

    void RefreshLayout();
    
    void Update(string trackGUID, string name);
    void ForceUpdate();
    void ForceUpdate(string trackGUID, string name);
    void RunAction(string trackGUID, string name, double value);
    void CycleAction(string trackGUID, string name);
    
    void SetWidgetValue(string trackGUID, string name, double value);
    void SetWidgetValue(string trackGUID, string name, double value, int mode);
    void SetWidgetValue(string trackGUID, string name, string value);
    
    void AdjustTrackBank(int stride);
    void ImmobilizeSelectedTracks();
    void MobilizeSelectedTracks();
    
    void TrackFXListChanged(MediaTrack* trackid);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiIOManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    DAW* daw_ = new DAW();
    
    MidiIOManager* midiIOManager_ = nullptr;
    
    vector <LogicalSurface*> surfaces_;

    bool lazyInitialized_ = false;
    
    int currentSurfaceIndex_ = 0;;

    void RunAndUpdate()
    {
        if(!lazyInitialized_)
        {
            // init the maps here
            
            LogicalSurface* surface = new LogicalSurface(this);
            surface->Initialize();
            surfaces_.push_back(surface);
            
            surface = new LogicalSurface(this);
            surface->Initialize2();
            surfaces_.push_back(surface);
            
            lazyInitialized_ = true;
        }
        
        surfaces_[currentSurfaceIndex_]->RunAndUpdate();
    }
    
    double GetPrivateProfileDouble(string key)
    {
        char tmp[512];
        memset(tmp, 0, sizeof(tmp));
        
        GetDAW()->GetPrivateProfileString("REAPER", key.c_str() , "", tmp, sizeof(tmp), get_ini_file());

        return strtod (tmp, NULL);
    }

    
public:
    virtual ~CSurfManager() { };
    
    CSurfManager();

    DAW* GetDAW() { return daw_; }
    
    MidiIOManager* MidiManager() { return midiIOManager_; }

    LogicalSurface* GetCurrentLogicalSurface() { return surfaces_[currentSurfaceIndex_]; }
    
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
        surfaces_[currentSurfaceIndex_]->OnTrackSelection(trackid);
    }
    
    void Run()
    {
        RunAndUpdate();
    }
    
    void NextLogicalSurface()
    {
        currentSurfaceIndex_ = currentSurfaceIndex_ == surfaces_.size() - 1 ? 0 : ++currentSurfaceIndex_;

        surfaces_[currentSurfaceIndex_]->RefreshLayout();
    }
    
    void ImmobilizeSelectedTracks(LogicalSurface* logicalCSurf)
    {
        for(auto & surface : surfaces_)
            if(surface != logicalCSurf)
                surface->ImmobilizeSelectedTracks();
    }
    
   void  MobilizeSelectedTracks(LogicalSurface* logicalCSurf)
    {
        for(auto & surface : surfaces_)
            if(surface != logicalCSurf)
                surface->MobilizeSelectedTracks();
    }
    
    void AdjustTrackBank(LogicalSurface* logicalCSurf, int stride)
    {
        for(auto & surface : surfaces_)
            if(surface != logicalCSurf)
                surface->AdjustTrackBank(stride);
    }

    void TrackListChanged() // tell current map
    {
        if(surfaces_.size() != 0) // seems we need to protect against prematurely early calls
            for(auto & surface : surfaces_)
                surface->TrackListChanged();
    }
    
    void TrackFXListChanged(MediaTrack* trackid)
    {
        if(surfaces_.size() != 0) // seems we need to protect against prematurely early calls
            for(auto & surface : surfaces_)
                surface->TrackFXListChanged(trackid);
    }
   
    double GetCurrentNormalizedValue(string trackGUID, string name);
    
    void Update(string trackGUID, string name);
    void ForceUpdate(string trackGUID, string name);
    void RunAction(string trackGUID, string name, double value);
    void CycleAction(string trackGUID, string name);
    void SetWidgetValue(string trackGUID, string name, double value);
    void SetWidgetValue(string trackGUID, string name, double value, int mode);
    void SetWidgetValue(string trackGUID, string name, string value);
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
    CSurfManager* manager_ = nullptr;
    vector<MidiChannelInput> inputs_;
    vector<MidiChannelOutput> outputs_;
    
public:
    MidiIOManager(CSurfManager* manager) : manager_(manager) {}
    
    CSurfManager* GetManager() { return  manager_; }
    
    midi_Input* GetMidiInputForChannel(int inputChannel)
    {
        for(auto input : inputs_)
            if(input.channel_ == inputChannel)
                return input.midiInput_;
        
        midi_Input* newInput = GetManager()->GetDAW()->CreateMIDIInput(inputChannel);
        
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
                return output.midiOutput_;
        
        midi_Output* newOutput = GetManager()->GetDAW()->CreateMIDIOutput(outputChannel, false, NULL );
        
        if(newOutput)
        {
            outputs_.push_back(MidiChannelOutput(outputChannel, newOutput));
            return newOutput;
        }
        
        return nullptr;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSCCSurf : public RealCSurf
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~OSCCSurf() {};
    
    OSCCSurf(LogicalSurface* surface, const string name, const int numChannels)
    : RealCSurf(surface, name, numChannels) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WebCSurf : public RealCSurf
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~WebCSurf() {};
    
    WebCSurf(LogicalSurface* surface, const string name, const int numChannels)
    : RealCSurf(surface, name, numChannels) {};
};

#endif /* control_surface_integrator.h */
