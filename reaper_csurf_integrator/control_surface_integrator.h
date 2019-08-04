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
#include <iomanip>
#include <fstream>
#include <regex>

#ifdef _WIN32
#include "oscpkt.hh"
#include "udp.hh"
#endif

#include "control_surface_integrator_Reaper.h"

#ifdef _WIN32
#include <memory>
#include "direntWin.h"
#include <functional>
#else
#include <dirent.h>
#include "oscpkt.hh"
#include "udp.hh"
#endif

const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";

// CSI.ini tokens used by GUI and initialization
const string MidiInMonitorToken = "MidiInMonitor";
const string MidiOutMonitorToken = "MidiOutMonitor";
const string VSTMonitorToken = "VSTMonitor";
const string FollowMCPToken = "FollowMCP";
const string MidiSurfaceToken = "MidiSurface";
const string PageToken = "Page";
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
const string NoModifiers = "NoModifiers";

extern int __g_projectconfig_timemode2, __g_projectconfig_timemode;

class Manager;
extern Manager* TheManager;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSurface;
class FeedbackProcessor;
class WidgetActionManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    string name_ = "";

    WidgetActionManager* widgetActionManager_ = nullptr;
    vector<FeedbackProcessor*> feedbackProcessors_;

    bool isModifier_ = false;
    double lastValue_ = 0.0;
    string lastStringValue_ = "";
    
public:
    Widget(ControlSurface* surface, string name) : surface_(surface), name_(name) {}
    virtual ~Widget() {};
    
    ControlSurface* GetSurface() { return surface_; }
    string GetName() { return name_; }
    void SetWidgetActionManager(WidgetActionManager* widgetActionManager) { widgetActionManager_ = widgetActionManager;  }
    void AddFeedbackProcessor(FeedbackProcessor* feedbackProcessor) { feedbackProcessors_.push_back(feedbackProcessor); }
    void SetIsModifier() { isModifier_ = true; }
    bool GetIsModifier() { return isModifier_; }

    void Reset()
    {
        SetValue(0.0);
        SetValue(0, 0.0);
        SetValue("");
        ClearCache();
    }

    MediaTrack* GetTrack();
    void RequestUpdate();
    void DoAction(double value);
    void DoRelativeAction(double value);
    void SetIsTouched(bool isTouched);
    void SetValue(double value);
    void SetValue(int mode, double value);
    void SetValue(string value);
    void ClearCache();
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
protected:
    Midi_ControlSignalGenerator(Widget* widget) : ControlSignalGenerator(widget) {}

public:
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
    Midi_ControlSurface* surface_;
    MIDI_event_ex_t* lastMessageSent_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiFeedbackMessage1_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiFeedbackMessage2_ = new MIDI_event_ex_t(0, 0, 0);
    
    Midi_FeedbackProcessor(Midi_ControlSurface* surface) : surface_(surface) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : surface_(surface), midiFeedbackMessage1_(feedback1) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1, MIDI_event_ex_t* feedback2) : surface_(surface), midiFeedbackMessage1_(feedback1), midiFeedbackMessage2_(feedback2) {}

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
class Zone;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SendsActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    int numSendSlots_ = 0;
    bool shouldMapSends_ = false;
    vector<Zone*> activeSendZones_;
        
public:
    SendsActivationManager(ControlSurface* surface) : surface_(surface) {}
    
    bool GetShouldMapSends() { return shouldMapSends_; }
    void SetNumSendSlots(int numSendSlots) { numSendSlots_ = numSendSlots; }
    vector<Zone*> &GetActiveZones() { return activeSendZones_; }
    
    void ToggleMapSends(map<string, Zone*> &zones)
    {
        shouldMapSends_ = ! shouldMapSends_;
        MapSelectedTrackSendsToWidgets(zones);
    }
    
    void MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXWindow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string fxName = "";
    MediaTrack* track = nullptr;;
    int fxIndex = 0;
    
    FXWindow(string anFxName, MediaTrack* aTrack, int anFxIndex) : fxName(anFxName), track(aTrack), fxIndex(anFxIndex) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    bool shouldMapFX_ = false;
    vector<Zone*> activeFXZones_;
    
    bool mapsSelectedTrackFXToWidgets_ = false;
    bool mapsFocusedTrackFXToWidgets_ = false;

    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;
    
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
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, fxWindow.fxIndex, 2);
        openFXWindows_.clear();
    }
    
public:
    FXActivationManager(ControlSurface* surface) : surface_(surface) {}
    
    bool GetShouldMapFX() { return shouldMapFX_; }
    bool GetShowFXWindows() { return showFXWindows_; }
    vector<Zone*> &GetActiveZones() { return activeFXZones_; }
    
    void MapSelectedTrackFXToWidgets(map<string, Zone*> &zones);
    void MapFocusedTrackFXToWidgets(map<string, Zone*> &zones);
    
    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
    }
    
    void ToggleMapFX(map<string, Zone*> &zones)
    {
        shouldMapFX_ = ! shouldMapFX_;
        MapSelectedTrackFXToWidgets(zones);
    }
    
    void TrackFXListChanged(map<string, Zone*> &zones)
    {
        if(mapsSelectedTrackFXToWidgets_)
            MapSelectedTrackFXToWidgets(zones);
        else if(mapsFocusedTrackFXToWidgets_)
            MapFocusedTrackFXToWidgets(zones);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface(Page* page, const string name, bool useZoneLink);
    Page* page_ = nullptr;
    const string name_ = "";
    vector<Widget*> widgets_;

    FXActivationManager* FXActivationManager_ = nullptr;
    SendsActivationManager* sendsActivationManager_ = nullptr;

    map<string, Zone*> zones_;
    bool useZoneLink_ = false;

    virtual void InitWidgets(string templateFilename) {}

    void InitZones(string zoneFolder);
    
    void RequestUpdate()
    {
        for(auto widget : widgets_)
            widget->RequestUpdate();
    }
    
public:
    virtual ~ControlSurface() {};
    
    Page* GetPage() { return page_; }
    string GetName() { return name_; }
    bool GetUseZoneLink() { return useZoneLink_; }
    bool GetShowFXWindows() { return FXActivationManager_->GetShowFXWindows(); }
    void SetNumSendSlots(int numSendSlots) { sendsActivationManager_->SetNumSendSlots(numSendSlots); }
    virtual void TurnOffMonitoring() {}
    
    WidgetActionManager* GetHomeWidgetActionManagerForWidget(Widget* widget);
    int GetParentZoneIndex(Zone* childZone);
    void GoZone(string zoneName);
    void ToggleMapSends();
    void ToggleMapFX();
    void MapSelectedTrackSendsToWidgets();
    void MapSelectedTrackFXToWidgets();
    void MapFocusedTrackFXToWidgets();
    bool AddZone(Zone* zone);
    void ActivateZone(string zoneName);

    bool GetShouldMapSends() { return sendsActivationManager_->GetShouldMapSends(); }
    
    void ActivateToggleMapSends()
    {
        sendsActivationManager_->ToggleMapSends(zones_);
    }
    
    bool GetShouldMapFX() { return FXActivationManager_->GetShouldMapFX(); }
    
    void ActivateToggleMapFX()
    {
        FXActivationManager_->ToggleMapFX(zones_);
    }
    
    void ActivateSelectedTrackSends()
    {
        sendsActivationManager_->MapSelectedTrackSendsToWidgets(zones_);
    }

    void ActivateSelectedTrackFX()
    {
        FXActivationManager_->MapSelectedTrackFXToWidgets(zones_);
    }
    
    void ActivateFocusedTrackFX()
    {
        FXActivationManager_->MapFocusedTrackFXToWidgets(zones_);
    }
    
    virtual void Run()
    {
        RequestUpdate(); // this should always be last so that state changes are complete
    }
    
    void ResetAllWidgets()
    {
        for(auto widget : widgets_)
            widget->Reset();
    }
    
    void ClearCache()
    {
        for(auto widget : widgets_)
            widget->ClearCache();
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
    }

    void SetShowFXWindows(bool value)
    {
        FXActivationManager_->SetShowFXWindows(value);
    }

    void OnTrackSelection()
    {
        for(auto widget : widgets_)
            if(widget->GetName() == "OnTrackSelection")
                widget->DoAction(1.0);
    }
    
    void OnFXFocus(MediaTrack* track, int fxIndex)
    {
        for(auto widget : widgets_)
            if(widget->GetName() == "OnFXFocus")
                widget->DoAction(1.0);
    }
    
    void TrackFXListChanged(MediaTrack* track)
    {
        FXActivationManager_->TrackFXListChanged(zones_);
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
        if(controlGeneratorsByMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]) > 0)
            controlGeneratorsByMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]]->ProcessMidiMessage(evt);
        else if(controlGeneratorsByMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100) > 0)
            controlGeneratorsByMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100]->ProcessMidiMessage(evt);
        else if(controlGeneratorsByMessage_.count(evt->midi_message[0] * 0x10000) > 0)
            controlGeneratorsByMessage_[evt->midi_message[0] * 0x10000]->ProcessMidiMessage(evt);
    }
    
protected:
    virtual void InitWidgets(string templateFilename) override;

public:
    Midi_ControlSurface(Page* page, const string name, string templateFilename, string zoneFolder, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor, bool useZoneLink)
    : ControlSurface(page, name, useZoneLink), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor)
    {
        InitWidgets(templateFilename);
        
        ResetAllWidgets();
        
        // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
        InitZones(zoneFolder);
        
        ActivateZone("Home");
    }

    virtual ~Midi_ControlSurface() {}
    

    virtual void TurnOffMonitoring() override
    {
        midiInMonitor_ = false;
        midiOutMonitor_ = false;
    }

    virtual void Run() override
    {
        HandleMidiInput();
        ControlSurface::Run();
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
class TrackNavigationManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channelNum_ = 0;
    int bias_ = 0;
    bool isChannelTouched_ = false;
    MediaTrack* pinnedTrack_ = nullptr;
    bool isChannelPinned_ = false;
    bool isChannelPinnedToSelectedTrack_ = false;
    
protected:
    TrackNavigationManager* manager_ = nullptr;
    TrackNavigator(TrackNavigationManager* manager) : manager_(manager) {}
    
public:
    TrackNavigator(TrackNavigationManager* manager, int channelNum) : manager_(manager), channelNum_(channelNum) {}
    virtual ~TrackNavigator() {}
    
    virtual void SetTouchState(bool isChannelTouched) { isChannelTouched_ = isChannelTouched; }
    bool GetIsChannelTouched() { return isChannelTouched_; }
    MediaTrack* GetPinnedTrack() { return pinnedTrack_; }
    bool GetIsChannelPinned() { return isChannelPinned_; }
    bool GetIsChannelPinnedToSelectedTrack() { return isChannelPinnedToSelectedTrack_; }
    void IncBias() { bias_++; }
    void DecBias() { bias_--; }
    
    virtual void PinToTrack();
    virtual void PinToSelectedTrack();
    virtual void Unpin();
    
    virtual MediaTrack* GetTrack();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectedTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectedTrackNavigator(TrackNavigationManager* manager) : TrackNavigator(manager) {}
    virtual ~SelectedTrackNavigator() {}
    
    virtual void SetTouchState(bool isChannelTouched) override {}
    virtual void PinToTrack() override {}
    virtual void PinToSelectedTrack() override {}
    virtual void Unpin() override {}

    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXTrackNavigator(TrackNavigationManager* manager) : TrackNavigator(manager) {}
    virtual ~FocusedFXTrackNavigator() {}
    
    virtual void SetTouchState(bool isChannelTouched) override {}
    virtual void PinToTrack() override {}
    virtual void PinToSelectedTrack() override {}
    virtual void Unpin() override {}
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetActionManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* page_ = nullptr;
    Widget* widget_ = nullptr;
    
    WidgetActionManager* widgetActionManager_ = nullptr;
    bool isInverted_ = false;
    bool shouldToggle_ = false;
    bool shouldExecute_ = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    
public:
    Action(WidgetActionManager* widgetActionManager);
    virtual ~Action() {}
    
    WidgetActionManager* GetWidgetActionManager() { return widgetActionManager_; }
    
    void SetIsInverted() { isInverted_ = true; }
    void SetShouldToggle() { shouldToggle_ = true; }
    void SetDelayAmount(double delayAmount) { delayAmount_ = delayAmount; }
    
    virtual void AddAction(Action* action) {}
    virtual void SetIndex(int index) {}
    virtual void SetAlias(string alias) {}
    virtual string GetAlias() { return ""; }
    
    virtual void DoAction(double value)
    {
        value = isInverted_ == false ? value : 1.0 - value;
        
        if(shouldToggle_)
            DoToggle(value);
        else
            Do(value);
    }

    virtual void RequestUpdate() {}
    virtual void RequestTrackUpdate(MediaTrack* track) {}
    
    virtual void Do(string value) {}
    virtual void Do(double value) {}
    virtual void DoToggle(double value) {}
    
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
    
    void Activate(WidgetActionManager* widgetActionManager)
    {
        widget_->SetWidgetActionManager(widgetActionManager);
    }
    
    void ActivateNoAction()
    {
        widget_->SetWidgetActionManager(nullptr);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetActionManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Widget* widget_ = nullptr;
    TrackNavigator* trackNavigator_ = nullptr;
    map<string, vector <Action*>> actions_;
    
    vector <Action*> trackTouchedActions_;
    
    string GetModifiers();

public:
    WidgetActionManager(Widget* widget, TrackNavigator* trackNavigator) : widget_(widget), trackNavigator_(trackNavigator) {}
    
    Widget* GetWidget() { return widget_; }
    
    MediaTrack* GetTrack()
    {
        if(trackNavigator_ == nullptr)
            return nullptr;
        else
            return trackNavigator_->GetTrack();
    }
    
    void RequestUpdate()
    {
        if(trackTouchedActions_.size() > 0 && trackNavigator_ != nullptr && trackNavigator_->GetIsChannelTouched())
        {
            for(auto action : trackTouchedActions_)
                action->RequestUpdate();
        }
        else
        {
            if(actions_.count(GetModifiers()) > 0)
                for(auto action : actions_[GetModifiers()])
                    action->RequestUpdate();
        }
    }
    
    void DoAction(double value)
    {
        string mods = GetModifiers();
        
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
                action->DoAction(value);
    }
    
    void Activate()
    {
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
                action->Activate(this);
    }
    
    void Activate(int index)
    {
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
            {
                action->SetIndex(index);
                action->Activate(this);
            }
    }
    
    void ActivateNoAction(int index)
    {
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
            {
                action->SetIndex(index);
                action->ActivateNoAction();
            }
    }
    
    void SetIsTouched(bool isTouched)
    {
        if(trackNavigator_ != nullptr)
            trackNavigator_->SetTouchState((isTouched));
    }
    
    void AddAction(string modifiers, Action* action)
    {
        actions_[modifiers].push_back(action);
    }
    
    void AddTrackTouchedAction(Action* action)
    {
        trackTouchedActions_.push_back(action);
    }
};

// substracts b<T> from a<T>
template <typename T>
void
subtract_vector(std::vector<T>& a, const std::vector<T>& b)
{
    typename std::vector<T>::iterator       it = a.begin();
    typename std::vector<T>::const_iterator it2 = b.begin();
    typename std::vector<T>::iterator       end = a.end();
    typename std::vector<T>::const_iterator end2 = b.end();
    
    while (it != end)
    {
        while (it2 != end2)
        {
            if (*it == *it2)
            {
                it = a.erase(it);
                end = a.end();
                it2 = b.begin();
            }
            else
                ++it2;
        }
        ++it;
        it2 = b.begin();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Page* page_ = nullptr;
    bool followMCP_ = true;
    bool synchPages_ = false;
    bool scrollLink_ = false;
    bool colourTracks_ = false;
    int trackColourRedValue_ = 0;
    int trackColourGreenValue_ = 0;
    int trackColourBlueValue_ = 0;
    map<string, int> trackColours_;
    int trackOffset_ = 0;
    int folderTrackOffset_ = 0;
    vector<MediaTrack*> tracks_;
    vector<MediaTrack*> pinnedTracks_;
    vector<MediaTrack*> unpinnedTracks_;
    vector<MediaTrack*> folderTracks_;
    vector<TrackNavigator*> trackNavigators_;
    
public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages, bool colourTracks, int red, int green, int blue) : page_(page), followMCP_(followMCP), synchPages_(synchPages), colourTracks_(colourTracks), trackColourRedValue_(red), trackColourGreenValue_(green), trackColourBlueValue_(blue) {}
    
    void PinTrackToChannel(MediaTrack* track, int channelNum)
    {
        pinnedTracks_.push_back(track);
        
        for(int i = channelNum + 1; i < trackNavigators_.size(); i++)
            trackNavigators_[i]->IncBias();
    }
    
    void UnpinTrackFromChannel(MediaTrack* track, int channelNum)
    {
        vector<MediaTrack*>::iterator it = find(pinnedTracks_.begin(), pinnedTracks_.end(), track);
        
        if(it != pinnedTracks_.end())
            pinnedTracks_.erase(it);
        
        for(int i = channelNum + 1; i < trackNavigators_.size(); i++)
            trackNavigators_[i]->DecBias();
    }
    
    void TogglePin(MediaTrack* track)
    {
        if(track == tracks_[tracks_.size() - 1]) // GAW TDB -- prevent Pinning last Track -- this is a hack because of a bug in subtract_vectors
            return;
        
        for(auto navigator : trackNavigators_)
        {
            if(track == navigator->GetTrack())
            {
                if(navigator->GetIsChannelPinned())
                    navigator->Unpin();
                else
                    navigator->PinToTrack();
                
                break;
            }
        }
    }

    
    Page* GetPage() { return page_; }
    bool GetFollowMCP() { return followMCP_; }
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    int  GetNumTracks() { return tracks_.size(); }
    int  GetNumFolderTracks() { return folderTracks_.size(); }
    
    TrackNavigator* AddTrackNavigator();
    void OnTrackSelection();
    void TrackListChanged();
    void AdjustTrackBank(int amount);

    MediaTrack* GetTrackFromChannel(int channelNumber)
    {
        if(unpinnedTracks_.size() > channelNumber + trackOffset_)
            return unpinnedTracks_[channelNumber + trackOffset_];
        else
            return nullptr;
    }
    
    MediaTrack* GetTrackFromId(int trackNumber)
    {
        if(tracks_.size() > trackNumber)
            return tracks_[trackNumber];
        else
            return nullptr;
    }
    
    
    int cycles = 0;

    void Run()
    {
        /*
        cycles++;
        

        if(cycles < 60)
            return;
        
        cycles = 0;

        int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        */
        
        
        
        int flags;
        MediaTrack* track;
        
        tracks_.clear();
        folderTracks_.clear();
        
        // Get Visible Tracks
        for (int i = 1; i <= DAW::CSurf_NumTracks(followMCP_); i++)
        {
            track = DAW::CSurf_TrackFromID(i, followMCP_);
            
            if(DAW::IsTrackVisible(track, followMCP_))
            {
                tracks_.push_back(track);
                
                DAW::GetTrackInfo(track, &flags);
                
                if(flags & 0x01) // folder Track
                    folderTracks_.push_back(track);
            }
        }
        
        // find pinnedTracks_ that are no longer in tracks_
        vector<MediaTrack*> tracksToRemove;
        for(auto track : pinnedTracks_)
        {
            if(find(tracks_.begin(), tracks_.end(), track) == tracks_.end())
            {
                tracksToRemove.push_back(track);
                break;
            }
        }
        
        // Unpin any removed tracks
        for(auto track : tracksToRemove)
        {
            for(auto navigator : trackNavigators_)
            {
                if(track == navigator->GetTrack())
                {
                    navigator->Unpin();
                    break;
                }
            }
        }

        // remove removed tracks from pinnedTracks_
        subtract_vector(pinnedTracks_, tracksToRemove);
        
        // clone tracks_ and remove pinnedTracks_
        unpinnedTracks_.assign(tracks_.begin(), tracks_.end());
        subtract_vector(unpinnedTracks_, pinnedTracks_);
        
        int top = GetNumTracks() - trackNavigators_.size();
        
        if(top < 0)
            trackOffset_ = 0;
        else if(trackOffset_ >  top)
            trackOffset_ = top;
        
        top = GetNumFolderTracks();
        if(folderTrackOffset_ >  top)
            folderTrackOffset_ = top;
        
        /*
        int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
        
        
        char msgBuffer[250];
        
        sprintf(msgBuffer, "%d microseconds\n", duration);
        DAW::ShowConsoleMsg(msgBuffer);
        */
    }
    
    bool GetIsTrackTouched(MediaTrack* track)
    {
        for(auto navigator : trackNavigators_)
            if(navigator->GetTrack() == track && navigator->GetIsChannelTouched())
                return true;
        
        return false;
    }
    
    void EnterPage()
    {
        /*
         if(colourTracks_)
         {
         // capture track colors
         for(auto* navigator : trackNavigators_)
         if(MediaTrack* track = DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_))
         trackColours_[navigator->GetTrackGUID()] = DAW::GetTrackColor(track);
         }
         */
    }
    
    void LeavePage()
    {
        /*
         if(colourTracks_)
         {
         DAW::PreventUIRefresh(1);
         // reset track colors
         for(auto* navigator : trackNavigators_)
         if(MediaTrack* track = DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_))
         if(trackColours_.count(navigator->GetTrackGUID()) > 0)
         DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", &trackColours_[navigator->GetTrackGUID()]);
         DAW::PreventUIRefresh(-1);
         }
         */
    }
    
    void ToggleScrollLink(int targetChannel)
    {
        scrollLink_ = ! scrollLink_;
        
        MediaTrack* selectedTrack = GetSelectedTrack();
        
        if(scrollLink_ && selectedTrack != nullptr)
        {
            // GAW TBD -- set the trackOffset_ such that the Selected Track is as close as possible to the targetChannel  
            
        }
    }
    
    MediaTrack* GetSelectedTrack()
    {
        if(DAW::CountSelectedTracks(NULL) != 1)
            return nullptr;
        
        MediaTrack* track = nullptr;
        
        for(int i = 0; i < GetNumTracks(); i++)
        {
            if(DAW::GetMediaTrackInfo_Value(GetTrackFromId(i), "I_SELECTED"))
            {
                track = GetTrackFromId(i);
                break;
            }
        }
        
        return track;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<WidgetActionManager*> widgetActionManagers_;
    vector<Zone*> includedZones_;
    int zoneIndex_ = 0;
    string parentZoneName_ = "";

    ControlSurface* surface_ = nullptr;
    string name_ = "";
    string sourceFilePath_ = "";
    
public:
    Zone(ControlSurface* surface, string name, string sourceFilePath) : surface_(surface), name_(name), sourceFilePath_(sourceFilePath) {}
    virtual ~Zone() {}
    
    int GetZoneIndex() { return zoneIndex_; }
    string GetParentZoneName() { return parentZoneName_; }
    void SetParentZoneName(string parentZoneName) { parentZoneName_ = parentZoneName; }
    string GetName() { return name_ ;}
    string GetSourceFilePath() { return sourceFilePath_; }
    
    virtual void AddWidgetActionManager(WidgetActionManager* manager)
    {
        widgetActionManagers_.push_back(manager);
    }
    
    void AddZone(Zone* zone)
    {
        includedZones_.push_back(zone);
    }
    
    WidgetActionManager* GetHomeWidgetActionManagerForWidget(Widget* widget)
    {
        for(auto manager : widgetActionManagers_)
            if(manager->GetWidget() == widget)
                return manager;
        
        for(auto zone : includedZones_)
            if(WidgetActionManager* manager = zone->GetHomeWidgetActionManagerForWidget(widget))
                return manager;
        
        return nullptr;
    }

    void Deactivate()
    {
        for(auto widgetActionManager : widgetActionManagers_)
        {
            Widget* widget =  widgetActionManager->GetWidget();
            WidgetActionManager* manager = surface_->GetHomeWidgetActionManagerForWidget(widget);
            if(manager == nullptr)
                widget->Reset();
            widget->SetWidgetActionManager(manager);
        }
        
        for(auto zone : includedZones_)
            zone->Deactivate();
    }
    
    void SetWidgetsToZero()
    {
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->GetWidget()->Reset();
    }
    
    void Activate()
    {
        if(parentZoneName_ != "")
        {
            int index = surface_->GetParentZoneIndex(this);
            
            for(auto widgetActionManager : widgetActionManagers_)
                widgetActionManager->Activate(index);
            
            for(auto zone : includedZones_)
                zone->Activate(index);
        }
        else
        {
            for(auto widgetActionManager : widgetActionManagers_)
                widgetActionManager->Activate();
            
            for(auto zone : includedZones_)
                zone->Activate();
        }
    }
    
    void ActivateNoAction(int zoneIndex)
    {
        zoneIndex_ = zoneIndex;
        
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->ActivateNoAction(zoneIndex);
        
        for(auto zone : includedZones_)
            zone->ActivateNoAction(zoneIndex);
    }
    
    void Activate(int zoneIndex)
    {
        zoneIndex_ = zoneIndex;
        
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->Activate(zoneIndex);
        
        for(auto zone : includedZones_)
            zone->Activate(zoneIndex);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";
    vector<ControlSurface*> surfaces_;
    
    bool isShift_ = false;
    bool isOption_ = false;
    bool isControl_ = false;
    bool isAlt_ = false;

    TrackNavigationManager* trackNavigationManager_ = nullptr;

public:
    Page(string name, bool followMCP, bool synchPages, bool colourTracks, int red, int green, int blue) : name_(name)
    {
        trackNavigationManager_ = new TrackNavigationManager(this, followMCP, synchPages, colourTracks, red, green, blue);
    }
    
    string GetName() { return name_; }
    
    vector<ControlSurface*> &GetSurfaces() { return surfaces_; }
    
    void Run()
    {
        trackNavigationManager_->Run();
        
        for(auto surface : surfaces_)
            surface->Run();
    }
    
    void TurnOffMonitoring()
    {
        for(auto surface : surfaces_)
            surface->TurnOffMonitoring();
    }

    void ResetAllWidgets()
    {
        for(auto surface : surfaces_)
            surface->ResetAllWidgets();
    }
    
    void AddSurface(ControlSurface* surface)
    {
        surfaces_.push_back(surface);
    }
    
    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        return trackNavigationManager_->GetIsTrackTouched(track);
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
    
    string GetModifiers()
    {
        string modifiers = "";
        
        if(isShift_)
            modifiers += Shift;
        if(isOption_)
            modifiers += Option;
        if(isControl_)
            modifiers +=  Control;
        if(isAlt_)
            modifiers += Alt;
        
        if(modifiers == "")
            modifiers = NoModifiers;
        
        return modifiers;
    }
    
    void OnTrackSelection()
    {
        trackNavigationManager_->OnTrackSelection();
        
        for(auto surface : surfaces_)
            surface->OnTrackSelection();
    }
 
    MediaTrack* GetSelectedTrack() { return trackNavigationManager_->GetSelectedTrack(); }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        for(auto surface : surfaces_)
            surface->OnFXFocus(track, fxIndex);
    }
    
    void GoZone(string zoneName)
    {
        for(auto surface : surfaces_)
            if(surface->GetUseZoneLink())
                surface->ActivateZone(zoneName);
    }

    void ToggleMapSends()
    {
        for(auto surface : surfaces_)
            if(surface->GetUseZoneLink())
                surface->ActivateToggleMapSends();
    }
    
    void ToggleMapFX()
    {
        for(auto surface : surfaces_)
            if(surface->GetUseZoneLink())
                surface->ActivateToggleMapFX();
    }
    
    void MapSelectedTrackSendsToWidgets()
    {
        for(auto surface : surfaces_)
            if(surface->GetUseZoneLink())
                surface->ActivateSelectedTrackSends();
    }
    
    void MapSelectedTrackFXToWidgets()
    {
        for(auto surface : surfaces_)
            if(surface->GetUseZoneLink())
                surface->ActivateSelectedTrackFX();
    }
    
    void MapFocusedTrackFXToWidgets()
    {
        for(auto surface : surfaces_)
            if(surface->GetUseZoneLink())
                surface->ActivateFocusedTrackFX();
    }
    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto surface : surfaces_)
            surface->TrackFXListChanged(track);
    }

    /// GAW -- start TrackNavigationManager facade
    bool GetFollowMCP() { return trackNavigationManager_->GetFollowMCP(); }
    bool GetSynchPages() { return trackNavigationManager_->GetSynchPages(); }
    bool GetScrollLink() { return trackNavigationManager_->GetScrollLink(); }
    int  GetNumTracks() { return trackNavigationManager_->GetNumTracks(); }
    MediaTrack* GetTrackFromId(int trackNumber) { return trackNavigationManager_->GetTrackFromId(trackNumber); }

    TrackNavigationManager* GetTrackNavigationManager() { return trackNavigationManager_; }

    TrackNavigator* AddTrackNavigator()
    {
        return trackNavigationManager_->AddTrackNavigator();
    }
    
    void TogglePin(MediaTrack* track)
    {
        trackNavigationManager_->TogglePin(track);
    }
    
    void AdjustTrackBank(int amount)
    {
        trackNavigationManager_->AdjustTrackBank(amount);
    }

    void EnterPage()
    {
        trackNavigationManager_->EnterPage();
        
        for(auto surface : surfaces_)
            surface->ClearCache();
    }
    
    void LeavePage()
    {
        trackNavigationManager_->LeavePage();
    }
    
    void ToggleScrollLink(int targetChannel)
    {
        trackNavigationManager_->ToggleScrollLink(targetChannel);
    }
    
    /// GAW -- end TrackNavigationManager facade
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<string, function<Action*(WidgetActionManager* manager, vector<string>)>> actions_;
    vector <Page*> pages_;
    
    map<string, map<string, int>> fxParamIndices_;
    
    int currentPageIndex_ = 0;
    bool VSTMonitor_ = false;
    
    void InitActionDictionary();

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
        InitActionDictionary();
    }
    
    void ResetAllWidgets()
    {
        VSTMonitor_ = false;
        
        if(pages_.size() > 0)
        {
            pages_[currentPageIndex_]->TurnOffMonitoring();
            pages_[currentPageIndex_]->ResetAllWidgets();
        }
    }
    
    void Init();

    bool GetVSTMonitor() { return VSTMonitor_; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    Page* GetCurrentPage()
    {
        if(pages_.size() > 0)
            return pages_[currentPageIndex_];
        else
            return nullptr;
    }
    
    Action* GetAction(WidgetActionManager* manager, vector<string> params)
    {
        if(actions_.count(params[0]) > 0)
            return actions_[params[0]](manager, params);
        else
            return nullptr;
    }

    bool IsActionAvailable(string actionName)
    {
        if(actions_.count(actionName) > 0)
            return true;
        else
            return false;
    }
    
    void OnTrackSelection(MediaTrack *track)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnTrackSelection();
    }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnFXFocus(track, fxIndex);
    }

    int GetFXParamIndex(MediaTrack* track, Widget* widget, int fxIndex, string fxParamName)
    {
        char fxName[BUFSZ];
        
        DAW::TrackFX_GetFXName(track, fxIndex, fxName, sizeof(fxName));
        
        if(fxParamIndices_.count(fxName) > 0 && fxParamIndices_[fxName].count(fxParamName) > 0)
            return fxParamIndices_[fxName][fxParamName];
        
        char paramName[BUFSZ];
        
        for(int i = 0; i < DAW::TrackFX_GetNumParams(track, fxIndex); i++)
        {
            DAW::TrackFX_GetParamName(track, fxIndex, i, paramName, sizeof(paramName));
            
            if(paramName == fxParamName)
            {
                fxParamIndices_[fxName][fxParamName] = i;
                return i;
            }
        }
        
        return 0;
    }
    
    void Run()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->Run();
    }
    
    void AdjustTrackBank(Page* sendingPage, int amount)
    {
        if(! sendingPage->GetSynchPages())
            sendingPage->AdjustTrackBank(amount);
        else
            for(auto page: pages_)
                if(page->GetSynchPages())
                    page->AdjustTrackBank(amount);
    }
    
    void NextPage()
    {
        if(pages_.size() > 0)
        {
            pages_[currentPageIndex_]->LeavePage();
            currentPageIndex_ = currentPageIndex_ == pages_.size() - 1 ? 0 : ++currentPageIndex_;
            pages_[currentPageIndex_]->EnterPage();
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
                pages_[currentPageIndex_]->EnterPage();

                break;
            }
        }
    }
    
    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        if(pages_.size() > 0)
            return pages_[currentPageIndex_]->GetTouchState(track, touchedControl);
        else
            return false;
    }
        
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto & page : pages_)
            page->TrackFXListChanged(track);
        
        if(VSTMonitor_)
        {
            char fxName[BUFSZ];
            char fxParamName[BUFSZ];
            
            for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
            {
                DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
                
                DAW::ShowConsoleMsg(("\n\n" + string(fxName)).c_str());

                string filename(fxName);
                filename = regex_replace(filename, regex("[\\:*?<>|.,()]"), "_");
                filename += ".txt";

                ofstream rawFXFile(string(DAW::GetResourcePath()) + "/CSI/Zones/ZoneRawFXFiles/" + filename);
                
                if(rawFXFile.is_open())
                {
                    rawFXFile << string(fxName);
                    
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                    {
                        DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                        DAW::ShowConsoleMsg(("\n" + string(fxParamName)).c_str());
                        rawFXFile << "\n" + string(fxParamName);
                    }
                }
                
                rawFXFile.close();
            }
        }
    }
};




/*
 int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
 
 
 // code you wish to time goes here
 // code you wish to time goes here
 // code you wish to time goes here
 // code you wish to time goes here
 // code you wish to time goes here
 // code you wish to time goes here
 
 
 
 int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
 
 char msgBuffer[250];
 
 sprintf(msgBuffer, "%d microseconds\n", duration);
 DAW::ShowConsoleMsg(msgBuffer);
 
 */

#endif /* control_surface_integrator.h */
