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

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "time.h"
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <functional>
#include <regex>

#ifdef _WIN32
#include "oscpkt.hh"
#include "udp.hh"
#endif

#include "control_surface_integrator_Reaper.h"

#ifdef _WIN32
#include <memory>
#include "direntWin.h"
#else
#include <dirent.h>
#include "oscpkt.hh"
#include "udp.hh"
#endif

const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";
//const string Shift = "Shift";
//const string Option = "Option";
//const string Control = "Control";
//const string Alt = "Alt";

// CSI.ini tokens used by GUI and initialization
const string MidiInMonitorToken = "MidiInMonitor";
const string MidiOutMonitorToken = "MidiOutMonitor";
const string VSTMonitorToken = "VSTMonitor";
const string FollowMCPToken = "FollowMCP";
const string MidiSurfaceToken = "MidiSurface";
const string PageToken = "Page";

extern int __g_projectconfig_timemode2, __g_projectconfig_timemode;

// subtracts b<T> from a<T>
template <typename T>
void subtract_vector(std::vector<T>& a, const std::vector<T>& b)
{
    typename std::vector<T>::iterator       ita = a.begin();
    typename std::vector<T>::const_iterator itb = b.begin();
    typename std::vector<T>::iterator       enda = a.end();
    typename std::vector<T>::const_iterator endb = b.end();
    
    while (ita != enda)
    {
        while (itb != endb)
        {
            if (*ita == *itb)
            {
                ita = a.erase(ita);
                enda = a.end();
                itb = b.begin();
            }
            else
                ++itb;
        }
        ++ita;
        
        itb = b.begin();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiChannelInput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int channel_ = 0;
    midi_Input* midiInput_ = nullptr;
    
    MidiChannelInput(int channel, midi_Input* midiInput)
    : channel_(channel), midiInput_(midiInput) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiChannelOutput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int channel_ = 0;
    midi_Output* midiOutput_ = nullptr;
    
    MidiChannelOutput(int channel, midi_Output* midiOutput)
    : channel_(channel), midiOutput_(midiOutput) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiIOManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
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
class ControlSurface;
class ActionContext;
class Page;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";

protected:
    Widget(string name, bool wantsFeedback) : name_(name), wantsFeedback_(wantsFeedback) {}
    ActionContext* actionContext_ = nullptr;
    bool wantsFeedback_ = false;
    bool shouldRefresh_ = false;
    double refreshInterval_ = 0.0;
    double lastRefreshed_ = 0.0;
    
public:
    virtual ~Widget() {};
    
    string GetName() { return name_; }
    void RequestUpdate();
    void DoAction(double value);
    void DoRelativeAction(double value);

    void SetRefreshInterval(double refreshInterval) { shouldRefresh_ = true; refreshInterval_ = refreshInterval * 1000.0; }
    void SetActionContext(ActionContext* actionContext) { actionContext_ = actionContext;  }

    virtual void SetValue(double value) {}
    virtual void SetValue(int mode, double value) {}
    virtual void SetValue(string value) {}
    virtual void ClearCache() {}
    
    void Reset()
    {
        SetValue(0.0);
        SetValue(0, 0.0);
        SetValue("");
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSignalGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Widget* widget_ = nullptr;
    ControlSignalGenerator(Widget* widget) : widget_(widget) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_ControlSignalGenerator : public ControlSignalGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Midi_ControlSignalGenerator(Widget* widget) : ControlSignalGenerator(widget) {}
    virtual ~Midi_ControlSignalGenerator() {}
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    bool shouldRefresh_ = false;
    double refreshInterval_ = 0.0;
    double lastRefreshed_ = 0.0;
    
public:
    virtual ~FeedbackProcessor() {}
    void SetRefreshInterval(double refreshInterval) { shouldRefresh_ = true; refreshInterval_ = refreshInterval * 1000.0; }
    virtual void SetValue(double value) {}
    virtual void SetValue(int param, double value) {}
    virtual void SetValue(string value) {}
    virtual void ClearCache() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_ControlSurface;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_FeedbackProcessor : public FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* press) : surface_(surface), midiPressMessage_(press) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : surface_(surface), midiPressMessage_(press), midiReleaseMessage_(release) {}

    Midi_ControlSurface* surface_;
    MIDI_event_ex_t* lastMessageSent_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiPressMessage_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiReleaseMessage_ = new MIDI_event_ex_t(0, 0, 0);
    
    void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(int first, int second, int third);
    
public:
    virtual void ClearCache() override
    {
        lastMessageSent_->midi_message[0] = 0;
        lastMessageSent_->midi_message[1] = 0;
        lastMessageSent_->midi_message[2] = 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_Widget : public Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Midi_ControlSurface* surface_ = nullptr;
    
protected:
    MIDI_event_ex_t* lastMessageSent_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiPressMessage_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiReleaseMessage_ = new MIDI_event_ex_t(0, 0, 0);

    void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(int first, int second, int third);

public:
    Midi_Widget(Midi_ControlSurface* surface, string name, bool wantsFeedback) : Widget(name, wantsFeedback), surface_(surface) {}
    Midi_Widget(Midi_ControlSurface* surface, string name, bool wantsFeedback, MIDI_event_ex_t* press) : Widget(name, wantsFeedback), surface_(surface), midiPressMessage_(press) {}
    Midi_Widget(Midi_ControlSurface* surface, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Widget(name, wantsFeedback), surface_(surface), midiPressMessage_(press), midiReleaseMessage_(release) {}
    virtual ~Midi_Widget() {};
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
    
    virtual void ClearCache() override
    {
        lastMessageSent_->midi_message[0] = 0;
        lastMessageSent_->midi_message[1] = 0;
        lastMessageSent_->midi_message[2] = 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<Widget*, ActionContext*> actionContextForWidget_;

protected:
    ControlSurface* surface_ = nullptr;
    string name_ = "";
    
public:
    Zone(ControlSurface* surface, string name) : surface_(surface), name_(name) {}
    virtual ~Zone() {}
    
    string GetName() { return name_ ;}
    
    virtual void AddActionContextForWidget(Widget* widget, ActionContext* context)
    {
        actionContextForWidget_[widget] = context;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CompositeZone : public Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<string, Zone*> zones_;
    
public:
    CompositeZone(ControlSurface* surface, string name) : Zone(surface, name) {}
    
    virtual ~CompositeZone() {}
    
    virtual void AddActionContextForWidget(Widget* widget, ActionContext* context) override {}
    
    void AddZone(Zone* zone)
    {
        zones_[zone->GetName()] = zone;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionZone : public Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ActionZone(ControlSurface* surface, string name) : Zone(surface, name) {}
    
    virtual ~ActionZone() {}
    
    virtual void AddActionContextForWidget(Widget* widget, ActionContext* context) override {}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigator : public Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool isPinned_ = false;
    string trackGUID_ = "";
    vector<Widget*> widgets_;
    
public:
    TrackNavigator() {}
    
    void AddWidget(Widget* widget) { widgets_.push_back(widget); }
    bool GetIsPinned() { return isPinned_; }
    string GetTrackGUID() { return trackGUID_; }
    void SetTrackGUID(Page* page, string trackGUID);
    
    void SetIsPinned(bool pinned)
    {
        isPinned_ = pinned;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* page_ = nullptr;
    const string name_ = "";

    vector<Widget*> widgets_;
    map<string, Zone*> zones_;
    vector<Zone*> activeZones_;
    void InitZones(string templateFilename);
    void ProcessCompositeZone(ifstream &zoneFile, vector<string> tokens, map<string, vector<CompositeZone*>> &compositeZoneMembers);
    void ProcessZone(ifstream &zoneFile, vector<string> tokens);
    void ProcessActionZone(ifstream &zoneFile, vector<string> tokens);

    
    ControlSurface(Page* page, const string name) : page_(page), name_(name) {}

    void RequestUpdate()
    {
        for(auto widget : widgets_)
            widget->RequestUpdate();
    }

public:
    virtual ~ControlSurface() {};
    
    vector<Widget*> & GetAllWidgets()
    {
        return widgets_;
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
    }
    
    virtual void Run()
    {
        RequestUpdate();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    midi_Input* midiInput_ = nullptr;
    midi_Output* midiOutput_ = nullptr;
    bool midiInMonitor_ = false;
    bool midiOutMonitor_ = false;
    map<int, Midi_Widget*> widgetsByMessage_;
    map<int, Midi_ControlSignalGenerator*> controlGeneratorsByMessage_;
    
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
        if(midiInMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "IN -> %s %02x  %02x  %02x \n", name_.c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
            DAW::ShowConsoleMsg(buffer);
        }
        
        // At this point we don't know how much of the message comprises the key, so try all three
        if(widgetsByMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]) > 0)
            widgetsByMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100) > 0)
            widgetsByMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(evt->midi_message[0] * 0x10000) > 0)
            widgetsByMessage_[evt->midi_message[0] * 0x10000]->ProcessMidiMessage(evt);
    }
    
public:
    Midi_ControlSurface(Page* page, const string name, string templateFilename, string zoneFolder, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor);

    virtual ~Midi_ControlSurface()
    {
        // GAW TBD -- removing this temporarily to see what happens with windows users crash when loading other projects
        /*
        if (midiInput_) delete midiInput_;
        if(midiOutput_) delete midiOutput_;
         */
    }
    
    virtual void Run() override
    {
        HandleMidiInput();
        RequestUpdate();
    }
    
    void AddWidgetToMessageMap(int message, Midi_Widget* widget)
    {
        widgetsByMessage_[message] = widget;
    }
    
    void AddControlGenerator(int message, Midi_ControlSignalGenerator* controlGenerator)
    {
        controlGeneratorsByMessage_[message] = controlGenerator;
    }
    
    void SendMidiMessage(MIDI_event_ex_t* midiMessage)
    {
        if(midiOutput_)
            midiOutput_->SendMsg(midiMessage, -1);
        
        if(midiOutMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "OUT -> %s SysEx \n", name_.c_str());
            DAW::ShowConsoleMsg(buffer);
        }
    }
    
    void SendMidiMessage(int first, int second, int third)
    {
        if(midiOutput_)
            midiOutput_->Send(first, second, third, -1);
        
        if(midiOutMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "OUT -> %s %02x  %02x  %02x \n", name_.c_str(), first, second, third);
            DAW::ShowConsoleMsg(buffer);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Action() {}
    
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget) {}                                     // GlobalContext
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, int commandId) {}                      // ReaperActionContext
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, string param) {}                       // string param
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, MediaTrack* track) {}                  // TrackContext
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, MediaTrack* track, int param) {}       // TrackParamContext
    virtual void RequestUpdate(ActionContext* actionContext, Widget* widget, MediaTrack* track, int fxIndex, int paramIndex) {} // FXContext

    virtual void Do(Page* page, double value) {}                                                                                // GlobalContext / ReaperActionContext
    virtual void Do(Page* page, string value) {}                                                                                // GlobalContext / ReaperActionContext
    virtual void Do(Page* page, Widget* widget, MediaTrack* track, double value) {}                                             // TrackContext / TrackParamContext
    virtual void Do(Page* page, Widget* widget, MediaTrack* track, int sendIndex, double value) {}                              // Sends
    virtual void Do(MediaTrack* track, int fxIndex, int paramIndex, double value) {}                                            // FXContext
    virtual void DoToggle(MediaTrack* track, int fxIndex, int paramIndex, double value) {}                                      // FXContext
    virtual void Do(Page* page, ControlSurface* surface) {}
    virtual void Do(Page* page, ControlSurface* surface, MediaTrack* track) {}
    virtual void Do(Page* page, ControlSurface* surface, MediaTrack* track, int fxIndex) {}
    virtual void Do(Page* page, ControlSurface* surface, double value) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* page_ = nullptr;
    ControlSurface* surface_ = nullptr;
    Action * action_ = nullptr;
    bool isInverted_ = false;
    bool shouldToggle_ = false;
    bool shouldExecute_ = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;

    ActionContext(Page* page, ControlSurface* surface, Action* action) : page_(page), surface_(surface), action_(action) {}
    
public:
    virtual ~ActionContext() {}
    
    void SetIsInverted() { isInverted_ = true; }
    void SetShouldToggle() { shouldToggle_ = true; }
    void SetDelayAmount(double delayAmount) { delayAmount_ = delayAmount; }

    virtual void SetTrack(string trackGUID) {}
    virtual void SetIndex(int index) {}
    virtual void SetAlias(string alias) {}
    virtual string GetAlias() { return ""; }
    virtual void SetCyclerWidget(Widget* cyclerWidget) {}
    virtual void RequestUpdate(Widget* widget) {}
    virtual void DoAction(Widget* widget) {}
    virtual void DoAction(Widget* widget, double value) {}
    virtual void DoRelativeAction(Widget* widget, double value) {}
    virtual void DoAction(Widget* widget, MediaTrack* track) {}
    virtual void DoAction(Widget* widget, MediaTrack* track, int fxIndex) {}

    void SetWidgetValue(Widget* widget, double value)
    {
        isInverted_ == false ? widget->SetValue(value) : widget->SetValue(1.0 - value);
    }
    
    void SetWidgetValue(Widget* widget, int param, double value)
    {
        isInverted_ == false ? widget->SetValue(param, value) : widget->SetValue(param, 1.0 - value);
    }
    
    void SetWidgetValue(Widget* widget, string value)
    {
        widget->SetValue(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXWindow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    MediaTrack* track = nullptr;;
    int fxIndex = 0;
    
    FXWindow(MediaTrack* aTrack, int anFxIndex) : track(aTrack), fxIndex(anFxIndex) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";
    int number_ = 0;
    bool followMCP_ = true;
    bool hasMasterChannel_ = false;
    bool synchPages_ = false;
    bool scrollLink_ = true;
    bool colourTracks_ = false;
    int trackColourRedValue_ = 0;
    int trackColourGreenValue_ = 0;
    int trackColourBlueValue_ = 0;
    map<string, int> trackColours_;
    int trackOffset_ = 0;
    int sendsOffset_ = 0;
    MediaTrack **previousTrackList_ = nullptr;
    int previousNumVisibleTracks_ = 0;
    vector<ControlSurface*> realSurfaces_;
    vector<TrackNavigator*> bankableChannels_;
    vector<MediaTrack*> touchedTracks_;
    map <string, vector<Widget*>> fxWidgets_;
    bool currentlyRefreshingLayout_ = false;
    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;

    void InitActionContexts(ControlSurface* surface, string templateFilename);
    void InitFXContexts(ControlSurface* surface, string templateDirectory);


    
    bool isShift_ = false;
    bool isOption_ = false;
    bool isControl_ = false;
    bool isAlt_ = false;
    
    string GetCurrentModifiers()
    {
        string modifiers = "";
        
        if(isShift_)
            modifiers += "Shift";
        if(isOption_)
            modifiers += "Option";
        if(isControl_)
            modifiers +=  "Control";
        if(isAlt_)
            modifiers += "Alt";
        
        return modifiers;
    }


    void SetShift(bool value)
    {
        isShift_ = value;
    }
    
    void SetOption(bool value)
    {
        isOption_ = value;
    }
    
    void SetControl(bool value)
    {
        isControl_ = value;
    }
    
    void SetAlt(bool value)
    {
        isAlt_ = value;
    }
    
    void SetPinnedTracks()
    {
        char buffer[BUFSZ];
        
        for(int i = 0; i < bankableChannels_.size(); i++)
        {
            if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
            {
                bankableChannels_[i]->SetTrackGUID(this, buffer);
                bankableChannels_[i]->SetIsPinned(true);
            }
        }
    }

    void GetPinnedChannelGUIDs(vector<string> & pinnedChannels)
    {
        string masterTrackGUID = DAW::GetTrackGUIDAsString(0, followMCP_);
        
        if(hasMasterChannel_)
            pinnedChannels.push_back(masterTrackGUID);
        else if( ! (followMCP_ && (DAW::GetMasterTrackVisibility() & 0x02)))
            pinnedChannels.push_back(masterTrackGUID);
        else if( ! (! followMCP_ && (DAW::GetMasterTrackVisibility() & 0x01)))
            pinnedChannels.push_back(masterTrackGUID);
        
        for(auto* channel : bankableChannels_)
            if(channel->GetIsPinned())
                pinnedChannels.push_back(channel->GetTrackGUID());
    }

    void AddFXWindow(FXWindow fxWindow)
    {
        openFXWindows_.push_back(fxWindow);
    }
    
    void OpenFXWindows()
    {
        if(showFXWindows_)
            for(auto fxWindow : openFXWindows_)
                DAW::TrackFX_Show(fxWindow.track, fxWindow.fxIndex, 3);
    }
    
    void CloseFXWindows()
    {
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, fxWindow.fxIndex, 2);
    }
    
    void DeleteFXWindows()
    {
        CloseFXWindows();
        openFXWindows_.clear();
    }
    
    int GetNumPinnedTracks()
    {
        int numPinnedTracks = 0;
        
        for(auto* channel : bankableChannels_)
            if(channel->GetIsPinned())
                numPinnedTracks++;
        
        return numPinnedTracks;
    }
    
    bool IsTrackVisible(MediaTrack* track)
    {
        if(DAW::GetMediaTrackInfo_Value(track, "IP_TRACKNUMBER") == -1) // Master
        {
            if(hasMasterChannel_)
                return false;
            else if(followMCP_ && (DAW::GetMasterTrackVisibility() & 0x02))
                return true;
            else if( ! followMCP_ && (DAW::GetMasterTrackVisibility() & 0x01))
                return true;
            else
                return false;
        }
        else
        {
            if(followMCP_ && DAW::GetMediaTrackInfo_Value(track, "B_SHOWINMIXER"))
                return true;
            else if( ! followMCP_ && DAW::GetMediaTrackInfo_Value(track, "B_SHOWINTCP"))
                return true;
            else
                return false;
        }
    }

public:
    Page(string name, int number, bool followMCP, bool synchPages, bool colourTracks, int red, int green, int blue) : name_(name), number_(number), followMCP_(followMCP), synchPages_(synchPages), colourTracks_(colourTracks), trackColourRedValue_(red), trackColourGreenValue_(green), trackColourBlueValue_(blue) {}
    
    string GetName() { return name_; }
    int GetNumber() { return number_; }
    string GetNumberString() { return "Page" + to_string(number_); }
    int GetSendsOffset() { return sendsOffset_; }
    int GetFXParamIndex(MediaTrack* track, Widget* widget, int fxIndex, string fxParamName);
    bool GetShowFXWindows() { return showFXWindows_; }
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    void AdjustTrackBank(int stride);
    void RefreshLayout();
    void TrackFXListChanged(MediaTrack* track);
    void OnGlobalMapTrackAndFxToWidgetsForTrack(MediaTrack* track);
    void OnFXFocus(MediaTrack* track, int fxIndex);
    void OnTrackSelection(MediaTrack* track);
    void OnTrackSelectionBySurface(MediaTrack* track);
    
    void SetHasMasterChannel(bool hasMasterChannel) { hasMasterChannel_ = hasMasterChannel; }
    
    void LeavePage()
    {
        if(colourTracks_)
        {
            DAW::PreventUIRefresh(1);
            // reset track colors
            for(auto* channel : bankableChannels_)
                if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                    if(trackColours_.count(channel->GetTrackGUID()) > 0)
                        DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", &trackColours_[channel->GetTrackGUID()]);
            DAW::PreventUIRefresh(-1);
        }
    }
    
    void EnterPage()
    {
        if(colourTracks_)
        {
            // capture track colors
            for(auto* channel : bankableChannels_)
                if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                    trackColours_[channel->GetTrackGUID()] = DAW::GetTrackColor(track);
        }
        
        for(auto surface : realSurfaces_)
            for(auto widget : surface->GetAllWidgets())
                widget->ClearCache();
    }

    void Run()
    {
        for(auto surface : realSurfaces_)
            surface->Run();
    }
    
    void ResetAllWidgets()
    {
        for(auto surface : realSurfaces_)
            for(auto widget : surface->GetAllWidgets())
                widget->Reset();
    }

    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
    }
    
    int GetMaxSends()
    {
        int maxSends = 0;
        
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_); i++)
        {
            MediaTrack* track = DAW::CSurf_TrackFromID(i, followMCP_);
            
            int numSends = DAW::GetTrackNumSends(track, 0);
            
            if(numSends > maxSends)
                maxSends = numSends;
        }

        return maxSends;
    }
    
    void AdjustTrackSendBank(int stride)
    {
        int maxOffset = GetMaxSends() - 1;

        sendsOffset_ += stride;
        
        if(sendsOffset_ < 0)
            sendsOffset_ = 0;
        else if(sendsOffset_ > maxOffset)
            sendsOffset_ = maxOffset;
    }
    
    void AddSurface(ControlSurface* surface, string actionTemplateFile, string fxTemplateDirectory)
    {
        realSurfaces_.push_back(surface);
        
        string resourcePath(DAW::GetResourcePath());
        resourcePath += "/CSI/";
   
        //InitActionContexts(surface, resourcePath + "axt/" + actionTemplateFile);
        //InitFXContexts(surface, resourcePath + "fxt/" + fxTemplateDirectory);
    }
    
    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        for(MediaTrack* touchedTrack : touchedTracks_)
            if(touchedTrack == track)
                return true;
        
        return false;
    }
    
    void SetTouchState(MediaTrack* track,  bool touched)
    {
        if(touched)
            touchedTracks_.push_back(track);
        else
            touchedTracks_.erase(remove(touchedTracks_.begin(), touchedTracks_.end(), track), touchedTracks_.end());
    }
    
    bool GetFollowMCP()
    {
        return followMCP_;
    }
    
    void SetScrollLink(bool value)
    {
        scrollLink_ = value;
    }
        
    void MapTrackAndFXToWidgets(ControlSurface* surface, MediaTrack* track)
    {
        MapTrackToWidgets(surface, track);
        MapFXToWidgets(track);
    }

    void MapTrackToWidgets(ControlSurface* surface, MediaTrack* track)
    {
        //widgetContextsMappedToTracks_.clear();
        /*
        for(auto channel : surface->GetChannels())
            for(auto widget : channel)
                if(widgetContexts_.count(widget) > 0)
                {
                    widgetContexts_[widget]->SetComponentTrackContext(Track, DAW::GetTrackGUIDAsString(track, followMCP_));
                    widgetContextsMappedToTracks_.push_back(widgetContexts_[widget]);
                }
         */
    }
    
    void MapFXToWidgets(MediaTrack* track)
    {
        //widgetContextsMappedToFX_.clear();
        
        char fxName[BUFSZ];
        
        DeleteFXWindows();
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            
            if(fxWidgets_.count(fxName) > 0)
            {
                for(auto widget : fxWidgets_[fxName])
                {/*
                    if(widgetContexts_.count(widget) > 0)
                    {
                        widgetContexts_[widget]->SetComponentTrackContext(fxName, DAW::GetTrackGUIDAsString(track, followMCP_));
                        widgetContexts_[widget]->SetComponent(fxName);
                        widgetContexts_[widget]->SetIndex(i);
                        widgetContextsMappedToFX_.push_back(widgetContexts_[widget]);
                    } */
                }
                
                AddFXWindow(FXWindow(track, i));
            }
        }
        
        OpenFXWindows();
    }
    
    void MapSingleFXToWidgets(MediaTrack* track, int fxIndex)
    {
        /*
        for(auto widgetContext : widgetContextsMappedToFX_)
        {
            widgetContext->GetWidget()->SetValue(0, 0.0);
            widgetContext->ClearAllButTrackContexts();
            widgetContext->SetComponent(Track);
        }
         */

        char fxName[BUFSZ];

        DAW::TrackFX_GetFXName(track, fxIndex, fxName, sizeof(fxName));
        
        if(fxWidgets_.count(fxName) > 0)
        {
            for(auto widget : fxWidgets_[fxName])
            {/*
                if(widgetContexts_.count(widget) > 0)
                {
                    widgetContexts_[widget]->SetComponentTrackContext(fxName, DAW::GetTrackGUIDAsString(track, followMCP_));
                    widgetContexts_[widget]->SetComponent(fxName);
                    widgetContexts_[widget]->SetIndex(fxIndex);
                    widgetContextsMappedToFX_.push_back(widgetContexts_[widget]);
                }*/
            }
            
            AddFXWindow(FXWindow(track, fxIndex));
        }
        
        OpenFXWindows();
    }
    
    void ToggleMapTrackAndFXToWidgets(ControlSurface* surface, MediaTrack* track)
    {
        ToggleMapTrackToWidgets(surface, track);
        ToggleMapFXToWidgets(surface, track);
    }
    
    void ToggleMapTrackToWidgets(ControlSurface* surface, MediaTrack* track)
    {
        /*
        if(widgetContextsMappedToTracks_.size() > 0)
            UnmapWidgetsFromTrack();
        else
            MapTrackToWidgets(surface, track);
         */
    }

    void ToggleMapFXToWidgets(ControlSurface* surface, MediaTrack* track)
    {
        /*
        if(widgetContextsMappedToFX_.size() > 0)
            UnmapWidgetsFromFX();
        else MapFXToWidgets(track);
         */
    }
    
    void ToggleMapSingleFXToWidgets(ControlSurface* surface, MediaTrack* track, int fxIndex)
    {
        if(track == nullptr)
        {/*
            for(auto widgetContext : widgetContextsMappedToFX_)
            {
                widgetContext->GetWidget()->SetValue(0, 0.0);
                widgetContext->ClearAllButTrackContexts();
                widgetContext->SetComponent(Track);
            }*/
        }
        else
            MapSingleFXToWidgets(track, fxIndex);
    }
    
    void UnmapWidgetsFromTrackAndFX()
    {
        UnmapWidgetsFromTrack();
        UnmapWidgetsFromFX();
    }
    
    void UnmapWidgetsFromTrack()
    {/*
        for(auto widgetContext : widgetContextsMappedToTracks_)
        {
            widgetContext->GetWidget()->SetValue(0, 0.0);
            widgetContext->SetComponentTrackContext(Track, "");
        }
        
        widgetContextsMappedToTracks_.clear();*/
    }
    
    void UnmapWidgetsFromFX()
    {
        DeleteFXWindows();
        /*
        for(auto widgetContext : widgetContextsMappedToFX_)
        {
            widgetContext->GetWidget()->SetValue(0, 0.0);
            widgetContext->ClearAllButTrackContexts();
            widgetContext->SetComponent(Track);
        }
        
        widgetContextsMappedToFX_.clear();*/
    }
    
    void CycleTimeDisplayModes()
    {
        int *tmodeptr = &__g_projectconfig_timemode2;
        if (tmodeptr && *tmodeptr>=0)
        {
            (*tmodeptr)++;
            if ((*tmodeptr)>5)
                (*tmodeptr)=0;
        }
        else
        {
            tmodeptr = &__g_projectconfig_timemode;
            
            if (tmodeptr)
            {
                (*tmodeptr)++;
                if ((*tmodeptr)>5)
                    (*tmodeptr)=0;
            }
        }
    }
    
    void Init()
    {
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_) && i < bankableChannels_.size(); i++)
            bankableChannels_[i]->SetTrackGUID(this, DAW::GetTrackGUIDAsString(i, followMCP_));

        SetPinnedTracks();
        
        char buffer[BUFSZ];
        if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "BankOffset").c_str(), buffer, sizeof(buffer)))
            trackOffset_ = atol(buffer);
    }
    
    void PinSelectedTracks()
    {
        TrackNavigator* channel = nullptr;
        
        for(int i = 0; i < bankableChannels_.size(); i++)
        {
            channel = bankableChannels_[i];
            
            MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_);
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsPinned(true);
                DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "Channel" + to_string(i + 1)).c_str(), channel->GetTrackGUID().c_str());
                DAW::MarkProjectDirty(nullptr);
            }
        }
    }
    
    void UnpinSelectedTracks()
    {
        TrackNavigator* channel = nullptr;
        char buffer[BUFSZ];
        
        for(int i = 0; i < bankableChannels_.size(); i++)
        {
            channel = bankableChannels_[i];
            
            MediaTrack* track =  DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_);
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsPinned(false);
                if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
                {
                    DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "Channel" + to_string(i + 1)).c_str(), "");
                    DAW::MarkProjectDirty(nullptr);
                }
            }
        }
        
        RefreshLayout();
    }
    
    bool TrackListChanged()
    {
        if(currentlyRefreshingLayout_)
            return false;
        
        int currentNumVisibleTracks = 0;
        
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_); i++)
            if(IsTrackVisible(DAW::CSurf_TrackFromID(i, followMCP_)))
                currentNumVisibleTracks++;

        if(currentNumVisibleTracks != previousNumVisibleTracks_)
        {
            if(previousTrackList_ != nullptr)
                delete[] previousTrackList_;

            previousNumVisibleTracks_ = currentNumVisibleTracks;
            previousTrackList_ = new MediaTrack* [currentNumVisibleTracks];
            
            for(int i = 0; i < currentNumVisibleTracks; i++)
                previousTrackList_[i] = DAW::CSurf_TrackFromID(i, followMCP_);
            
            DAW::ClearGUIDTracksCache();
            
            TrackNavigator* channel = nullptr;
            char buffer[BUFSZ];
            for(int i = 0; i < bankableChannels_.size(); i++)
            {
                channel = bankableChannels_[i];

                if(channel->GetIsPinned())
                {
                    if(DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_) == nullptr) // track has been removed
                    {
                        channel->SetIsPinned(false);
                        channel->SetTrackGUID(this, "");

                        // GAW remove this from pinned tracks list in project
                        if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
                        {
                            DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "Channel" + to_string(i + 1)).c_str(), "");
                            DAW::MarkProjectDirty(nullptr);
                        }
                    }
                }
            }
            
            return true;
        }
        else if(currentNumVisibleTracks == previousNumVisibleTracks_)
        {
            MediaTrack **currentTrackList = new MediaTrack* [currentNumVisibleTracks];
            for(int i = 0; i < currentNumVisibleTracks; i++)
                currentTrackList[i] = DAW::CSurf_TrackFromID(i, followMCP_);

            if(memcmp(previousTrackList_, currentTrackList, currentNumVisibleTracks * sizeof(MediaTrack*)))
            {
                if(previousTrackList_ != nullptr)
                    delete[] previousTrackList_;
                previousTrackList_ = currentTrackList;
                
                DAW::ClearGUIDTracksCache();
                return true;
            }
            else
            {
                delete[]currentTrackList;
                return false;
            }
        }

        return false;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MidiIOManager* midiIOManager_ = nullptr;
    map<string, Action*> actions_;
    map<string , function<ActionContext*(Page*, ControlSurface*, vector<string>)>> actionContexts_;
    vector <Page*> pages_;
    map<string, map<string, int>> fxParamIndices_;
    
    int currentPageIndex_ = 0;
    bool VSTMonitor_ = false;
    
    void InitActionDictionary();
    void InitActionContextDictionary();

    double GetPrivateProfileDouble(string key)
    {
        char tmp[512];
        memset(tmp, 0, sizeof(tmp));
        
        DAW::GetPrivateProfileString("REAPER", key.c_str() , "", tmp, sizeof(tmp), DAW::get_ini_file());
        
        return strtod (tmp, NULL);
    }
    
public:
    ~Manager() {};
    Manager()
    {
        InitActionContextDictionary();
        midiIOManager_ = new MidiIOManager();
    }
    
    void ResetAllWidgets()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ResetAllWidgets();
    }
    
    void Init();

    bool GetVSTMonitor() { return VSTMonitor_; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    map<string, map<string, int>> & GetFXParamIndices() { return fxParamIndices_; }
    
    Page* GetCurrentPage()
    {
        if(pages_.size() > 0)
            return pages_[currentPageIndex_];
        else
            return nullptr;
    }
    
    ActionContext* GetActionContext(Page* page, ControlSurface* surface, vector<string> params)
    {
        if(actionContexts_.count(params[0]) > 0)
            return actionContexts_[params[0]](page, surface, params);
        
        return nullptr;
    }
    
    ActionContext* GetFXActionContext(Page* page, ControlSurface* surface, Widget* widget, vector<string> params, string alias)
    {
        if(actionContexts_.count(params[0]) > 0)
        {
            ActionContext* context = actionContexts_[params[0]](page, surface, params);
            context->SetAlias(alias);
            return context;
        }
        
        return nullptr;
    }
    
    void OnTrackSelection(MediaTrack *track)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnTrackSelection(track);
    }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnFXFocus(track, fxIndex);
    }

    void Run()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->Run();
    }
    
    void AdjustTrackBank(Page* sendingPage, int stride)
    {
        if(! sendingPage->GetSynchPages())
            sendingPage->AdjustTrackBank(stride);
        else
            for(auto page: pages_)
                if(page->GetSynchPages())
                    page->AdjustTrackBank(stride);
    }
    
    void NextPage()
    {
        if(pages_.size() > 0)
        {
            pages_[currentPageIndex_]->LeavePage();
            currentPageIndex_ = currentPageIndex_ == pages_.size() - 1 ? 0 : ++currentPageIndex_;
            SavePageIndexToProjectFile();
            pages_[currentPageIndex_]->EnterPage();
            pages_[currentPageIndex_]->RefreshLayout();
        }
    }
    
    void GoPage(string pageName)
    {
        for(int i = 0; i < pages_.size(); i++)
        {
            if(pages_[i]->GetName() == pageName)
            {
                pages_[currentPageIndex_]->LeavePage();
                currentPageIndex_ = i;
                SavePageIndexToProjectFile();
                pages_[currentPageIndex_]->EnterPage();
                pages_[currentPageIndex_]->RefreshLayout();

                break;
            }
        }
    }
    
    void SavePageIndexToProjectFile()
    {
        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), "PageIndex", to_string(currentPageIndex_).c_str());
        DAW::MarkProjectDirty(nullptr);
    }
    
    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        if(pages_.size() > 0)
            return pages_[currentPageIndex_]->GetTouchState(track, touchedControl);
        else
            return false;
    }
    
    void TrackListChanged()
    {
        for(auto & page : pages_)
            if(page->TrackListChanged() && page == pages_[currentPageIndex_])
                page->RefreshLayout();
    }
    
    void TrackFXListChanged(MediaTrack* trackid)
    {
        for(auto & page : pages_)
            page->TrackFXListChanged(trackid);
    }
};

#endif /* control_surface_integrator.h */
