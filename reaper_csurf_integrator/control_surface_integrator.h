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
const string TrackTouch = "TrackTouch";
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
const string NoModifiers = "NoModifiers";

extern int __g_projectconfig_timemode2, __g_projectconfig_timemode;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSurface;
class FeedbackProcessor;
class WidgetActionContextManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    string name_ = "";

    WidgetActionContextManager* widgetActionContextManager_ = nullptr;
    vector<FeedbackProcessor*> feedbackProcessors_;
    
    double lastValue_ = 0.0;
    string lastStringValue_ = "";

    bool shouldRefresh_ = false;
    double refreshInterval_ = 0.0;
    double lastRefreshed_ = 0.0;
    
    bool isModifier_ = false;
    
public:
    Widget(ControlSurface* surface, string name) : surface_(surface), name_(name) {}
    virtual ~Widget() {};
    
    ControlSurface* GetSurface() { return surface_; }
    string GetName() { return name_; }
    MediaTrack* GetTrack();
    void RequestUpdate();
    void DoAction(double value);
    void DoRelativeAction(double value);

    void SetWidgetActionContextManager(WidgetActionContextManager* widgetActionContextManager) { widgetActionContextManager_ = widgetActionContextManager;  }
    void AddFeedbackProcessor(FeedbackProcessor* feedbackProcessor) { feedbackProcessors_.push_back(feedbackProcessor); }
    void SetRefreshInterval(double refreshInterval) { shouldRefresh_ = true; refreshInterval_ = refreshInterval * 1000.0; }

    void SetIsModifier() { isModifier_ = true; }
    bool GetIsModifier() { return isModifier_; }
    
    void SetValue(double value);
    void SetValue(int mode, double value);
    void SetValue(string value);
    void ClearCache();

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
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<WidgetActionContextManager*> actionContextManagers_;
    vector<Zone*> includedZones_;

protected:
    ControlSurface* surface_ = nullptr;
    string name_ = "";
    string sourceFilePath_ = "";
    
public:
    Zone(ControlSurface* surface, string name, string sourceFilePath) : surface_(surface), name_(name), sourceFilePath_(sourceFilePath) {}
    virtual ~Zone() {}
    
    void Deactivate();
    void Activate();
    void Activate(int contextIndex);
    void Activate(MediaTrack* track, int contextIndex);

    string GetName() { return name_ ;}
    string GetSourceFilePath() { return sourceFilePath_; }
       
    virtual void AddActionContextManager(WidgetActionContextManager* manager)
    {
        actionContextManagers_.push_back(manager);
    }
    
    void AddZone(Zone* zone)
    {
        includedZones_.push_back(zone);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int offset_ = 0;
    bool trackIsTouched_ = false;
    
protected:
    Page* page_ = nullptr;
    TrackNavigator(Page* page) : page_(page) {}
    
public:
    TrackNavigator(Page* page, int offset) : page_(page), offset_(offset) {}
    virtual ~TrackNavigator() {}
    
    virtual MediaTrack* GetTrack();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectedTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectedTrackNavigator(Page* page) : TrackNavigator(page) {}
    virtual ~SelectedTrackNavigator() {}
   
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXTrackNavigator(Page* page) : TrackNavigator(page) {}
    virtual ~FocusedFXTrackNavigator() {}
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    char prjFn[BUFSZ * 10] = "";

protected:
    Page* page_ = nullptr;
    const string name_ = "";

    bool useZoneLink_ = false;
    
    vector<Widget*> widgets_;
    map<string, Zone*> zones_;
    map<string, vector<Zone*>> activeSubZones_;
    vector<Zone*> activeZones_;
    
    virtual void InitWidgets(string templateFilename) {}
    void InitZones(string zoneFolder);
    
    ControlSurface(Page* page, const string name, bool useZoneLink) : page_(page), name_(name), useZoneLink_(useZoneLink) {}

    void RequestUpdate()
    {
        for(auto widget : widgets_)
            widget->RequestUpdate();
    }
    
    bool HasActiveZone(string zoneName)
    {
        if(find(activeZones_.begin(), activeZones_.end(), zones_[zoneName]) != activeZones_.end())
            return true;
        else
            return false;
    }

    void ActivateZone(string zoneName)
    {
        if(zones_.count(zoneName) > 0)
            AddActiveZone(zoneName);
    }

    void ReactivateZoneStack()
    {
        for(auto zone : activeZones_)
            zone->Activate();
    }
    
    void AddActiveZone(string zoneName)
    {
        zones_[zoneName]->Activate();
        activeZones_.push_back(zones_[zoneName]);
        ReactivateZoneStack();
    }
    
    void AddActiveFXZone(string zoneName, int fxIndex)
    {
        zones_[zoneName]->Activate(fxIndex);
        activeZones_.push_back(zones_[zoneName]);
        ReactivateZoneStack();
    }
    
    void AddActiveSendsZone(string zoneName, MediaTrack* track, int fxIndex)
    {
        zones_[zoneName]->Activate(track, fxIndex);
        activeZones_.push_back(zones_[zoneName]);
        ReactivateZoneStack();
    }
    
    void RemoveActiveZone(string zoneName)
    {
        if(activeSubZones_.count(zoneName) > 0)
        {
            for(auto subZone : activeSubZones_[zoneName])
            {
                subZone->Deactivate();
                activeZones_.erase(find(activeZones_.begin(), activeZones_.end(), subZone));
            }
            
            activeSubZones_.erase(zoneName);
        }
        
        zones_[zoneName]->Deactivate();
        activeZones_.erase(find(activeZones_.begin(), activeZones_.end(), zones_[zoneName]));
        
        ReactivateZoneStack();
    }
    
public:
    virtual ~ControlSurface() {};
    
    Page* GetPage() { return page_; }
    string GetName() { return name_; }
    
    bool GetUseZoneLink() { return useZoneLink_; }
    
    virtual void TurnOffMonitoring() {}
    void GoZone(string zoneName);

    virtual void Run()
    {
        /*
        DAW::EnumProjects(-1, prjFn, sizeof(prjFn));
        if (! *prjFn) // No projects open
            for(auto widget : widgets_)
                widget->Reset();
         */

        RequestUpdate();
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
    
    bool AddZone(Zone* zone)
    {
        if(zones_.count(zone->GetName()) > 0)
        {
            char buffer[5000];
            sprintf(buffer, "The Zone named \"%s\" is already defined in file\n %s\n\n The new Zone named \"%s\" defined in file\n %s\n will not be added\n\n\n\n",
                    zone->GetName().c_str(), zones_[zone->GetName()]->GetSourceFilePath().c_str(), zone->GetName().c_str(), zone->GetSourceFilePath().c_str());
            DAW::ShowConsoleMsg(buffer);
            return false;
        }
        else
        {
            zones_[zone->GetName()] = zone;
            return true;
        }
    }

    void OnTrackSelection(MediaTrack* track)
    {
        for(auto widget : widgets_)
            if(widget->GetName() == "OnTrackSelection")
                widget->DoAction(0.0);
    }
    
    void OnFXFocus(MediaTrack* track, int fxIndex)
    {
        for(auto widget : widgets_)
            if(widget->GetName() == "OnFXFocus")
                widget->DoAction(0.0);
    }

    bool ActivateFXZone(string zoneName, int fxIndex)
    {
        if((activeZones_.back())->GetName() == zoneName)
            return false;

        if(zones_.count(zoneName) > 0)
        {
            AddActiveFXZone(zoneName, fxIndex);
            return true;
        }
        
        return false;
    }
    
    bool ActivateSendsZone(string zoneName, MediaTrack* track, int sendsIndex)
    {
        if((activeZones_.back())->GetName() == zoneName)
            return false;

        if(zones_.count(zoneName) > 0)
        {
            AddActiveSendsZone(zoneName, track, sendsIndex);
            return true;
        }
        
        return false;
    }
    
    void DeactivateZone(string zoneName)
    {
        if(zones_.count(zoneName) > 0)
            RemoveActiveZone(zoneName);
    }

    void ToggleZone(string zoneName)
    {
        if(HasActiveZone(zoneName))
            DeactivateZone(zoneName);
        else if(zones_.count(zoneName) > 0)
            ActivateZone(zoneName);
    }
    
    void GoSubZone(string zoneName, string parentZoneName)
    {
        if((activeZones_.back())->GetName() == zoneName)
            return;
        
        if(zones_.count(parentZoneName) > 0 && zones_.count(zoneName) > 0)
        {
            if(HasActiveZone(zoneName))
                DeactivateZone(zoneName);
            
            activeSubZones_[parentZoneName].push_back(zones_[zoneName]);

            ActivateZone(zoneName);
        }
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
    
    virtual void InitWidgets(string templateFilename) override;

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
class ActionContext;
class WidgetActionContextManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Action() {}
    
    virtual void RequestUpdate(ActionContext* context);
    virtual void RequestUpdate(ActionContext* context, int commandId) { RequestUpdate(context); }
    virtual void RequestUpdate(ActionContext* context, string param) { RequestUpdate(context); }
    virtual void RequestUpdate(ActionContext* context, MediaTrack* track) { RequestUpdate(context); }
    virtual void RequestUpdate(ActionContext* context, MediaTrack* track, int param) { RequestUpdate(context); }
    virtual void RequestUpdate(ActionContext* context, MediaTrack* track, int fxIndex, int paramIndex) { RequestUpdate(context); }

    virtual void Do(double value) {}
    virtual void Do(Page* page, double value) {}
    virtual void Do(Page* page, ControlSurface* surface) {}
    virtual void Do(ControlSurface* surface, string value) {}
    virtual void Do(ControlSurface* surface, string value1, string value2) {}
    virtual void Do(Page* page, string value) {}
    virtual void Do(Widget* widget, MediaTrack* track, double value) {}
    virtual void Do(Widget* widget, MediaTrack* track, int sendIndex, double value) {}
    virtual void Do(Widget* widget, MediaTrack* track, WidgetActionContextManager* manager, string stringParam2, double value) {}
    virtual void Do(MediaTrack* track, int fxIndex, int paramIndex, double value) {}
    virtual void DoToggle(MediaTrack* track, int fxIndex, int paramIndex, double value) {}                 
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* page_ = nullptr;
    WidgetActionContextManager* widgetActionContextManager_ = nullptr;
    Action* action_ = nullptr;
    bool isInverted_ = false;
    bool shouldToggle_ = false;
    bool shouldExecute_ = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    
public:
    ActionContext(WidgetActionContextManager* widgetActionContextManager, Action* action) : widgetActionContextManager_(widgetActionContextManager), action_(action)
    {
        page_ = GetWidget()->GetSurface()->GetPage();
    }
    virtual ~ActionContext() {}
    
    WidgetActionContextManager* GetWidgetActionContextManager() { return widgetActionContextManager_; }
    
    void SetIsInverted() { isInverted_ = true; }
    void SetShouldToggle() { shouldToggle_ = true; }
    void SetDelayAmount(double delayAmount) { delayAmount_ = delayAmount; }
    
    Widget* GetWidget();
    Page* GetPage() { return page_; }
    
    virtual void AddActionContext(ActionContext* actionContext) {}
    virtual void SetIndex(int index) {}
    virtual void SetTrack(MediaTrack* track) {}
    virtual void SetAlias(string alias) {}
    virtual string GetAlias() { return ""; }
    virtual void RequestUpdate() {}
    virtual void DoAction(double value) {}
 
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
    
    void Activate(WidgetActionContextManager* modifierActionContextManager)
    {
        GetWidget()->SetWidgetActionContextManager(modifierActionContextManager);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetActionContextManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Widget* widget_ = nullptr;
    TrackNavigator* trackNavigator_ = nullptr;
    map<string, vector <ActionContext*>> widgetActionContexts_;
    
    string GetModifiers();
    
public:
    WidgetActionContextManager(Widget* widget) : widget_(widget) {}
    
    Widget* GetWidget() { return widget_; }
    MediaTrack* GetTrack();
    
    void SetTrackNavigator(TrackNavigator* trackNavigator) { trackNavigator_ = trackNavigator; }
    
    void RequestUpdate();
    void DoAction(double value);

    void Activate();
    void Activate(int contextIndex);
    void Activate(MediaTrack* track, int contextIndex);
    
    void AddActionContext(string modifiers, ActionContext* context)
    {
        widgetActionContexts_[modifiers].push_back(context);
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
    vector<TrackNavigator*> trackNavigators_;
    vector<MediaTrack*> tracks_;
    vector<MediaTrack*> folderTracks_;
    
public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages, bool colourTracks, int red, int green, int blue) : page_(page), followMCP_(followMCP), synchPages_(synchPages), colourTracks_(colourTracks), trackColourRedValue_(red), trackColourGreenValue_(green), trackColourBlueValue_(blue) {}
    
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    int  GetNumTracks() { return tracks_.size(); }
    int  GetNumFolderTracks() { return folderTracks_.size(); }

    MediaTrack* GetTrack(int channelNumber)
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
    
    void Init();
    void AdjustTrackBank(int stride);
    void OnTrackSelection(MediaTrack* track);
    void OnTrackSelectionBySurface(MediaTrack* track);
    void TrackListChanged();

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
        
        int top = GetNumTracks() - trackNavigators_.size();
        if(trackOffset_ >  top)
            trackOffset_ = top;
        
        top = GetNumFolderTracks();
        if(folderTrackOffset_ >  top)
            folderTrackOffset_ = top;
    }
    
    TrackNavigator* AddTrackNavigator()
    {
        int offset = trackNavigators_.size();
        
        trackNavigators_.push_back(new TrackNavigator(page_, offset));
        
        return trackNavigators_[offset];
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
class SendsNavigationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Page* page_ = nullptr;
    int sendsOffset_ = 0;
   
    int GetMaxSends();

public:
    SendsNavigationManager(Page* page) : page_(page) {}
    
    int GetSendsOffset() { return sendsOffset_; }
    
    
    void AdjustTrackSendBank(int stride)
    {
        int maxOffset = GetMaxSends() - 1;
        
        sendsOffset_ += stride;
        
        if(sendsOffset_ < 0)
            sendsOffset_ = 0;
        else if(sendsOffset_ > maxOffset)
            sendsOffset_ = maxOffset;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<string, map<string, int>> fxParamIndices_;

    Page* page_ = nullptr;
    map<ControlSurface*, vector<string>> activeFXZoneNames_;
    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;
    
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
    
public:
    FXActivationManager(Page* page) : page_(page) {}
    
    bool GetShowFXWindows() { return showFXWindows_; }
    
    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
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

    void ClearActiveFXZones(ControlSurface* surface)
    {
        if(activeFXZoneNames_.count(surface) > 0)
            activeFXZoneNames_[surface].clear();
    }
    
    void MapSelectedTrackFXToWidgets(ControlSurface* surface, MediaTrack* selectedTrack)
    {
        DeleteFXWindows();
        
        if(activeFXZoneNames_.count(surface) > 0)
        {
            for(auto zoneName :activeFXZoneNames_[surface])
                surface->DeactivateZone(zoneName);
            
            activeFXZoneNames_[surface].clear();
        }
        
        int flags;
        
        DAW::GetTrackInfo(selectedTrack, &flags);
        
        if(flags & 0x02) // track is selected -- not deselected
        {
            for(int i = 0; i < DAW::TrackFX_GetCount(selectedTrack); i++)
            {
                char FXName[BUFSZ];
                
                DAW::TrackFX_GetFXName(selectedTrack, i, FXName, sizeof(FXName));
                
                if(surface->ActivateFXZone(FXName, i))
                {
                    AddFXWindow(FXWindow(selectedTrack, i));
                    activeFXZoneNames_[surface].push_back(FXName);
                }
            }
            
            OpenFXWindows();
        }
    }
    
    void MapFocusedTrackFXToWidgets(ControlSurface* surface, MediaTrack* selectedTrack, int fxIndex)
    {
        if(activeFXZoneNames_.count(surface) > 0)
        {
            for(auto zoneName : activeFXZoneNames_[surface])
                surface->DeactivateZone(zoneName);
            
            activeFXZoneNames_[surface].clear();
        }
        
        char FXName[BUFSZ];
        
        DAW::TrackFX_GetFXName(selectedTrack, fxIndex, FXName, sizeof(FXName));
        
        if(surface->ActivateFXZone(FXName, fxIndex))
            activeFXZoneNames_[surface].push_back(FXName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SendsActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Page* page_ = nullptr;
    bool shouldMapSends_ = false;
    
    map<ControlSurface*, vector<string>> activeSendZoneNames_;

    void DeactivateSendsZones(ControlSurface* surface);
    void ActivateSendsZones(ControlSurface* surface, MediaTrack* selectedTrack);
    void ActivateSendsZone(ControlSurface* surface, MediaTrack* selectedTrack, int sendsIndex, string zoneName);

public:
    SendsActivationManager(Page* page) : page_(page) {}
    
    void ToggleMapSends(ControlSurface* surface);
    
    void MapSelectedTrackSendsToWidgets(ControlSurface* surface, MediaTrack* selectedTrack);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSI_TrackInfo
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int index = 0;
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
    
    vector<MediaTrack*> touchedTracks_;

    map<string, map<MediaTrack*, CSI_TrackInfo>> CSITrackSlotInfo_;

    TrackNavigationManager* trackNavigationManager_ = nullptr;
    SendsNavigationManager* sendsNavigationManager_ = nullptr;
    FXActivationManager* FXActivationManager_ = nullptr;
    SendsActivationManager* sendsActivationManager_ = nullptr;

public:
    Page(string name, bool followMCP, bool synchPages, bool colourTracks, int red, int green, int blue) : name_(name)
    {
        trackNavigationManager_ = new TrackNavigationManager(this, followMCP, synchPages, colourTracks, red, green, blue);
        sendsNavigationManager_ = new SendsNavigationManager(this);
        FXActivationManager_ = new FXActivationManager(this);
        sendsActivationManager_ = new SendsActivationManager(this);
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
        for(auto touchedTrack : touchedTracks_)
            if(track == touchedTrack)
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
    
    int GetTrackSlotIndex(string slotName, MediaTrack* track)
    {
        if(CSITrackSlotInfo_.count(slotName) < 1)
            return 0;
        else if(CSITrackSlotInfo_[slotName].count(track) < 1)
            return 0;
        else
            return CSITrackSlotInfo_[slotName][track].index;
    }
    
    void CycleTrackSlotIndex(string slotName, MediaTrack* track, int maxSize)
    {
        if(CSITrackSlotInfo_.count(slotName) < 1 || CSITrackSlotInfo_[slotName].count(track) < 1)
                CSITrackSlotInfo_[slotName][track] = CSI_TrackInfo();
        
        CSITrackSlotInfo_[slotName][track].index++;
        
        if(CSITrackSlotInfo_[slotName][track].index > maxSize - 1)
            CSITrackSlotInfo_[slotName][track].index = 0;
        
        // GAW could save to rpp file here for recall after project reload
        // could get VERY verbose
        // big downside - ties project to certain CSI config
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
    
    string GetModifiers(MediaTrack* track)
    {
        string modifiers = "";
        
        if(GetTouchState(track, 0))
            modifiers += TrackTouch;
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
        touchedTracks_.erase(remove(touchedTracks_.begin(), touchedTracks_.end(), removedTrack), touchedTracks_.end());

            for(auto [customModifierName, trackInfo] : CSITrackSlotInfo_)
                if(CSITrackSlotInfo_[customModifierName].count(removedTrack) > 0)
                {
                    CSITrackSlotInfo_[customModifierName].erase(removedTrack);
                    break;
                }

            
            

        
        
    }
    
    void OnTrackSelection(MediaTrack* track)
    {
        trackNavigationManager_->OnTrackSelection(track);
        
        for(auto surface : surfaces_)
            surface->OnTrackSelection(track);
    }
 
    /// GAW -- start ZoneActivation facade

    void GoSubZone(ControlSurface* surface, string zoneName, string parentZoneName)
    {
        if( ! surface->GetUseZoneLink())
            surface->GoSubZone(zoneName, parentZoneName);
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GoSubZone(zoneName, parentZoneName);
    }
    
    void GoZone(ControlSurface* surface, string zoneName)
    {
        if( ! surface->GetUseZoneLink())
            surface->GoZone(zoneName);
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GoZone(zoneName);
    }
    
    void ToggleZone(ControlSurface* surface, string zoneName)
    {
        if( ! surface->GetUseZoneLink())
            surface->ToggleZone(zoneName);
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->ToggleZone(zoneName);
    }

    /// GAW -- start TrackNavigationManager facade
    
    bool GetSynchPages() { return trackNavigationManager_->GetSynchPages(); }
    bool GetScrollLink() { return trackNavigationManager_->GetScrollLink(); }
    int  GetNumTracks() { return trackNavigationManager_->GetNumTracks(); }
    MediaTrack* GetTrack(int channelNumber) { return trackNavigationManager_->GetTrack(channelNumber); }
    MediaTrack* GetTrackFromId(int trackNumber) { return trackNavigationManager_->GetTrackFromId(trackNumber); }

    TrackNavigator* AddTrackNavigator()
    {
        return trackNavigationManager_->AddTrackNavigator();
    }
    
    void AdjustTrackBank(int stride)
    {
        if(touchedTracks_.size() == 0)
            trackNavigationManager_->AdjustTrackBank(stride);
    }
    
    void OnTrackSelectionBySurface(ControlSurface* surface, MediaTrack* track)
    {
        trackNavigationManager_->OnTrackSelectionBySurface(track);
        
        if(surface->GetUseZoneLink())
        {
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->OnTrackSelection(track);
        }
        else
            surface->OnTrackSelection(track);
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
    
    
    /// GAW -- start SendsNavigationManager facade
    
    int GetSendsOffset() { return sendsNavigationManager_->GetSendsOffset(); }
    
    
    void AdjustTrackSendBank(int stride)
    {
        sendsNavigationManager_->AdjustTrackSendBank(stride);
    }
    
    /// GAW -- end SendsNavigationManager facade


    /// GAW -- start FXActivationManager facade
    
    int GetFXParamIndex(MediaTrack* track, Widget* widget, int fxIndex, string fxParamName) { return FXActivationManager_->GetFXParamIndex(track, widget, fxIndex, fxParamName); }
    bool GetShowFXWindows() { return FXActivationManager_->GetShowFXWindows(); }
    
    void MapSelectedTrackSendsToWidgets()
    {
        if(MediaTrack* track = trackNavigationManager_->GetSelectedTrack())
            for(auto surface : surfaces_)
                sendsActivationManager_->MapSelectedTrackSendsToWidgets(surface, track);
    }
    
    void MapSelectedTrackFXToWidgets()
    {
        if(MediaTrack* track = trackNavigationManager_->GetSelectedTrack())
            for(auto surface : surfaces_)
                FXActivationManager_->MapSelectedTrackFXToWidgets(surface, track);
    }

    void MapFocusedTrackFXToWidgets()
    {
        int tracknumberOut = 0;
        int itemnumberOut = 0;
        int fxnumberOut = 0;
        
        if( DAW::GetFocusedFX(&tracknumberOut, &itemnumberOut, &fxnumberOut) == 1)
            for(auto surface : surfaces_)
                FXActivationManager_->MapFocusedTrackFXToWidgets(surface, GetTrackFromId(tracknumberOut), fxnumberOut);
    }
    
    void TrackFXListChanged(MediaTrack* track, bool VSTMonitor)
    {
        FXActivationManager_->TrackFXListChanged(track, VSTMonitor);
    }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        for(auto surface : surfaces_)
            surface->OnFXFocus(track, fxIndex);
    }
    
    void SetShowFXWindows(bool value)
    {
        FXActivationManager_->SetShowFXWindows(value);
    }

    void ClearActiveFXZones(ControlSurface* surface)
    {
        FXActivationManager_->ClearActiveFXZones(surface);
    }

    /// GAW -- end SendsActivationManager facade

    void ToggleMapSends(ControlSurface* surface) { sendsActivationManager_->ToggleMapSends(surface); }

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<string, Action*> actions_;
    map<string , function<ActionContext*(WidgetActionContextManager* manager, vector<string>)>> actionContexts_;
    vector <Page*> pages_;
    
    time_t lastTimeCacheCleared = 0;
    
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
    
    ActionContext* GetActionContext(WidgetActionContextManager* manager, vector<string> params)
    {
        if(actionContexts_.count(params[0]) > 0)
            return actionContexts_[params[0]](manager, params);
        
        return nullptr;
    }

    bool IsActionContextAvailable(string contextName)
    {
        if(actionContexts_.count(contextName) > 0)
            return true;
        else return false;
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
        /*
        time_t timeNow = DAW::GetCurrentNumberOfMilliseconds();
        
        if(timeNow - lastTimeCacheCleared > 30000) // every 30 seconds
        {
            lastTimeCacheCleared = timeNow;
            DAW::ClearCache();
        }
         */
        
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
    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto & page : pages_)
            page->TrackFXListChanged(track, VSTMonitor_);
    }
};

#endif /* control_surface_integrator.h */
