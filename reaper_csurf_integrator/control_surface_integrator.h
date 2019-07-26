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
class TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channelNum_ = 0;
    bool isChannelTouched_ = false;
    
protected:
    ControlSurface* surface_ = nullptr;   
    TrackNavigator(ControlSurface* surface) : surface_(surface) {}
    
public:
    TrackNavigator(ControlSurface* surface, int channelNum) : surface_(surface), channelNum_(channelNum) {}
    virtual ~TrackNavigator() {}
    
    virtual void SetTouchState(bool isChannelTouched) { isChannelTouched_ = isChannelTouched; }
    bool GetIsChannelTouched() { return isChannelTouched_; }
    
    virtual MediaTrack* GetTrack();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectedTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectedTrackNavigator(ControlSurface* surface) : TrackNavigator(surface) {}
    virtual ~SelectedTrackNavigator() {}
   
    virtual void SetTouchState(bool isChannelTouched) override {}
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXTrackNavigator(ControlSurface* surface) : TrackNavigator(surface) {}
    virtual ~FocusedFXTrackNavigator() {}

    virtual void SetTouchState(bool isChannelTouched) override {}
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SendsActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    bool shouldMapSends_ = false;
    int numSendSlots_ = 0;
    
    vector<Zone*> activeSendZones_;
        
public:
    SendsActivationManager(ControlSurface* surface) : surface_(surface) {}
    
    void SetNumSendSlots(int numSendSlots) { numSendSlots_ = numSendSlots; }
    
    void MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones);
    void ClearAll();
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
    vector<Zone*> activeFXZones_;
    
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
    
    bool GetShowFXWindows() { return showFXWindows_; }
    
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
    
    void TrackFXListChanged(MediaTrack* track, bool VSTMonitor)
    {
        char fxName[BUFSZ];
        char fxParamName[BUFSZ];
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            
            if(VSTMonitor)
            {
                DAW::ShowConsoleMsg(("\n\n" + string(fxName) + "\n").c_str());
                
                for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                {
                    DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                    DAW::ShowConsoleMsg((string(fxParamName) + "\n").c_str());
                }
            }
        }
        
        // GAW TBD -- clear all fx items and rebuild
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
    int surfaceChannelOffset_ = 0;
    vector<Widget*> widgets_;
    vector<TrackNavigator*> trackNavigators_;

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
    int GetSurfacChannelOffset() { return surfaceChannelOffset_; }
    bool GetUseZoneLink() { return useZoneLink_; }
    bool GetShowFXWindows() { return FXActivationManager_->GetShowFXWindows(); }
    void SetNumSendSlots(int numSendSlots) { sendsActivationManager_->SetNumSendSlots(numSendSlots); }
    virtual void TurnOffMonitoring() {}
    
    void GoZone(string zoneName);
    TrackNavigator* AddTrackNavigator();
    
    void MapSelectedTrackFXToWidgets()
    {
        FXActivationManager_->MapSelectedTrackFXToWidgets(zones_);
    }
    
    void MapFocusedTrackFXToWidgets()
    {
        FXActivationManager_->MapFocusedTrackFXToWidgets(zones_);
    }
    
    void MapSelectedTrackSendsToWidgets()
    {
        sendsActivationManager_->MapSelectedTrackSendsToWidgets(zones_);
    }

    bool AddZone(Zone* zone);
    void ActivateZone(string zoneName);
    
    bool IsTrackTouched(MediaTrack* track)
    {
        for(auto navigator : trackNavigators_)
            if(navigator->GetTrack() == track && navigator->GetIsChannelTouched())
                return true;
        
        return false;
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
    virtual void SetTrack(MediaTrack* track) {}
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

    virtual void RequestUpdate() { widget_->Reset(); }
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
    
    void Activate(int actionIndex)
    {
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
            {
                action->SetIndex(actionIndex);
                action->Activate(this);
            }
    }
    
    void Activate(MediaTrack* track, int actionIndex)
    {
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
            {
                action->SetTrack(track);
                action->SetIndex(actionIndex);
                action->Activate(this);
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<WidgetActionManager*> widgetActionManagers_;
    vector<Zone*> includedZones_;
    
protected:
    ControlSurface* surface_ = nullptr;
    string name_ = "";
    string sourceFilePath_ = "";
    
public:
    Zone(ControlSurface* surface, string name, string sourceFilePath) : surface_(surface), name_(name), sourceFilePath_(sourceFilePath) {}
    virtual ~Zone() {}
    
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
        
    void Deactivate()
    {
        for(auto widgetActionManager : widgetActionManagers_)
        {
            Widget* widget =  widgetActionManager->GetWidget();
            widget->SetWidgetActionManager(nullptr);
            widget->Reset();
        }
        
        for(auto zone : includedZones_)
            zone->Deactivate();
    }
    
    void Activate()
    {
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->Activate();
        
        for(auto zone : includedZones_)
            zone->Activate();
    }
    
    void Activate(int fxIndex)
    {
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->Activate(fxIndex);
        
        for(auto zone : includedZones_)
            zone->Activate(fxIndex);
    }
    
    void Activate(MediaTrack* track, int sendsIndex)
    {
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->Activate(track, sendsIndex);
        
        for(auto zone : includedZones_)
            zone->Activate(track, sendsIndex);
    }
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Page* page_ = nullptr;
    bool followMCP_ = true;
    bool synchPages_ = false;
    bool scrollLink_ = true;
    bool colourTracks_ = false;
    int trackColourRedValue_ = 0;
    int trackColourGreenValue_ = 0;
    int trackColourBlueValue_ = 0;
    map<string, int> trackColours_;
    int trackOffset_ = 0;
    int folderTrackOffset_ = 0;
    int totalNumChannels_ = 0;
    vector<MediaTrack*> tracks_;
    vector<MediaTrack*> folderTracks_;
    
public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages, bool colourTracks, int red, int green, int blue) : page_(page), followMCP_(followMCP), synchPages_(synchPages), colourTracks_(colourTracks), trackColourRedValue_(red), trackColourGreenValue_(green), trackColourBlueValue_(blue) {}
    
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    int  GetNumTracks() { return tracks_.size(); }
    int  GetNumFolderTracks() { return folderTracks_.size(); }
    
    void Init();
    void OnTrackSelection();
    void TrackListChanged();
    void AdjustTrackBank(int stride);

    MediaTrack* GetTrackFromChannel(int channelNumber)
    {
        if(tracks_.size() > channelNumber + trackOffset_)
            return tracks_[channelNumber + trackOffset_];
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
    
    int GetTotalNumChannels()
    {
        return totalNumChannels_;
    }

    void Run()
    {
        int flags;
        MediaTrack* track;
        
        tracks_.clear();
        folderTracks_.clear();
        
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
        
        int top = GetNumTracks() - totalNumChannels_;
        if(trackOffset_ >  top)
            trackOffset_ = top;
        
        top = GetNumFolderTracks();
        if(folderTrackOffset_ >  top)
            folderTrackOffset_ = top;
    }
    
    void AddTrackNavigator()
    {
        totalNumChannels_++;
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
    
    void SetScrollLink(bool value)
    {
        scrollLink_ = value;
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
        for(auto surface : surfaces_)
            if(surface->IsTrackTouched(track))
                return true;
        
        return false;
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
    
    void TrackHasBeenRemoved(MediaTrack* removedTrack)
    {
        // GAW TBD -- this matters for Pinned Tracks
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
    
    void MapSelectedTrackSendsToWidgets()
    {
        for(auto surface : surfaces_)
            surface->MapSelectedTrackSendsToWidgets();
    }
        
    /// GAW -- start TrackNavigationManager facade
    
    bool GetSynchPages() { return trackNavigationManager_->GetSynchPages(); }
    bool GetScrollLink() { return trackNavigationManager_->GetScrollLink(); }
    int  GetNumTracks() { return trackNavigationManager_->GetNumTracks(); }
    int  GetTotalNumChannels() { return trackNavigationManager_->GetTotalNumChannels(); }

    MediaTrack* GetTrackFromChannel(int channelNumber) { return trackNavigationManager_->GetTrackFromChannel(channelNumber); }
    MediaTrack* GetTrackFromId(int trackNumber) { return trackNavigationManager_->GetTrackFromId(trackNumber); }

    void AddTrackNavigator()
    {
        trackNavigationManager_->AddTrackNavigator();
    }
    
    void AdjustTrackBank(int stride)
    {
        trackNavigationManager_->AdjustTrackBank(stride);
    }

    void Init()
    {
        trackNavigationManager_->Init();
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
    
    void SetScrollLink(bool value)
    {
        trackNavigationManager_->SetScrollLink(value);
    }

    void TrackListChanged()
    {
        trackNavigationManager_->TrackListChanged();
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
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->TrackListChanged();
    }
};
#endif /* control_surface_integrator.h */
