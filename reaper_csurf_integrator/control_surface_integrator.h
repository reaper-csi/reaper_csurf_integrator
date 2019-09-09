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

extern REAPER_PLUGIN_HINSTANCE g_hInst;

const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";

const string FollowMCPToken = "FollowMCP";
const string MidiSurfaceToken = "MidiSurface";
const string OSCSurfaceToken = "OSCSurface";
const string PageToken = "Page";
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
const string NoModifiers = "NoModifiers";
const string BadFileChars = "[ \\:*?<>|.,()/]";

extern int __g_projectconfig_timemode2, __g_projectconfig_timemode;

class Manager;
extern Manager* TheManager;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfIntegrator;
class Page;
class ControlSurface;
class Midi_ControlSurface;
class OSC_ControlSurface;
class Zone;
class WidgetActionManager;
class TrackNavigator;
class TrackNavigationManager;
class FXActivationManager;
class FeedbackProcessor;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
class CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Widget* widget_ = nullptr;
    CSIMessageGenerator(Widget* widget) : widget_(widget) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_CSIMessageGenerator : public CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Midi_CSIMessageGenerator(Widget* widget) : CSIMessageGenerator(widget) {}
    
public:
    virtual ~Midi_CSIMessageGenerator() {}
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSC_CSIMessageGenerator : public CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    OSC_CSIMessageGenerator(OSC_ControlSurface* surface, Widget* widget, string message);
    virtual ~OSC_CSIMessageGenerator() {}
    
    virtual void ProcessOSCMessage(string message, double value)
    {
        widget_->DoAction(value);
    }
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
class Midi_FeedbackProcessor : public FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Midi_ControlSurface* surface_ = nullptr;
    
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
class OSC_FeedbackProcessor : public FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    OSC_ControlSurface* surface_ = nullptr;
    string oscAddress_ = "";
    float lastFloatValue_ = 0.0;
    string lastStringValue_ = "";
    
public:
    
    OSC_FeedbackProcessor(OSC_ControlSurface* surface, string oscAddress) : surface_(surface), oscAddress_(oscAddress) {}
    ~OSC_FeedbackProcessor() {}
    
    virtual void SetValue(double value) override;
    virtual void SetValue(int param, double value) override;
    virtual void SetValue(string value) override;

    virtual void ClearCache() override
    {
        lastFloatValue_ = 0.0;
        lastStringValue_ = "";
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Action(string name, WidgetActionManager* widgetActionManager);
    
    string name_ = "";
    Page* page_ = nullptr;
    Widget* widget_ = nullptr;
    
    WidgetActionManager* widgetActionManager_ = nullptr;
    bool isInverted_ = false;
    bool shouldToggle_ = false;
    bool shouldIgnoreRelease_ = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    
public:
    virtual ~Action() {}
    
    WidgetActionManager* GetWidgetActionManager() { return widgetActionManager_; }
    
    virtual string GetDisplayName() { return ""; }
    string GetName() { return name_; }
    
    virtual string GetParamAsString() { return ""; }
    virtual int GetParam() { return 0; }
    virtual string GetAlias() { return ""; }
    
    void SetIsInverted() { isInverted_ = true; }
    void SetShouldToggle() { shouldToggle_ = true; }
    void SetShouldIgnoreRelease() { shouldIgnoreRelease_ = true; }
    void SetDelayAmount(double delayAmount) { delayAmount_ = delayAmount; }
    
    virtual void AddAction(Action* action) {}
    virtual void SetIndex(int index) {}
    
    virtual void DoAction(double value);
    
    virtual double GetCurrentValue() { return 0.0; }
    virtual void RequestUpdate() {}
    virtual void RequestTrackUpdate(MediaTrack* track) {}
    
    virtual void Do(string value) {}
    virtual void Do(double value) {}
    
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
class NoAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NoAction(string name, WidgetActionManager* widgetActionManager) : Action(name, widgetActionManager) {}
    virtual ~NoAction() {}
    
    virtual void RequestUpdate() { widget_->Reset(); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetActionManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Widget* widget_ = nullptr;
    Zone* zone_ = nullptr;
    TrackNavigator* trackNavigator_ = nullptr;
    map<string, vector <Action*>> actions_;
    
    vector <Action*> trackTouchedActions_;
    
    string GetModifiers();
    
public:
    WidgetActionManager(Widget* widget, Zone* zone, TrackNavigator* trackNavigator) : widget_(widget), zone_(zone), trackNavigator_(trackNavigator) {}
    
    Widget* GetWidget() { return widget_; }
    Zone* GetZone() { return zone_; }
    bool GetHasFocusedFXNavigator();
    string GetNavigatorName();
    MediaTrack* GetTrack();
    void RequestUpdate();
    void SetIsTouched(bool isTouched);
    
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
    int zoneIndex_ = 0;
    string parentZoneName_ = "";
    
    ControlSurface* surface_ = nullptr;
    string name_ = "";
    string alias_ = "";
    string sourceFilePath_ = "";
    
public:
    Zone(ControlSurface* surface, string name, string sourceFilePath, string alias) : surface_(surface), name_(name), sourceFilePath_(sourceFilePath), alias_(alias) {}
    virtual ~Zone() {}
    
    int GetZoneIndex() { return zoneIndex_; }
    string GetParentZoneName() { return parentZoneName_; }
    void SetParentZoneName(string parentZoneName) { parentZoneName_ = parentZoneName; }
    string GetName() { return name_ ;}
    string GetAlias() { return alias_;}
    string GetSourceFilePath() { return sourceFilePath_; }
    
    void Activate();
    void Deactivate();
    
    bool ContainsWidgetActionManager(WidgetActionManager* widgetActionManager)
    {
        for(auto manager : widgetActionManagers_)
            if(widgetActionManager == manager)
                return true;
        
        for(auto zone : includedZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return true;
        
        return false;
    }
    
    bool GetHasFocusedFXTrackNavigator()
    {
        if(widgetActionManagers_.size() > 0)
            return widgetActionManagers_[0]->GetHasFocusedFXNavigator(); // GAW -- Kinda hokey, but Zone members all get the same Navigator
        else
            return false;
    }
    
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
    
    void SetWidgetsToZero()
    {
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->GetWidget()->Reset();
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
    void SetShouldMapSends(bool shouldMapSends) { shouldMapSends_ = shouldMapSends;  }
    int GetNumSendSlots() { return numSendSlots_; }
    void SetNumSendSlots(int numSendSlots) { numSendSlots_ = numSendSlots; }
    vector<Zone*> GetActiveZones() { return activeSendZones_; }
    
    void ToggleMapSends();
    void MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones);

    Zone* GetActiveZone(WidgetActionManager* widgetActionManager)
    {
        for(auto zone : activeSendZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;

        return nullptr;
    }
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
    int numFXlots_ = 0;
    bool shouldMapSelectedTrackFX_ = false;
    bool shouldMapSelectedTrackFXMenus_ = false;
    bool shouldMapFocusedFX_ = false;
    vector<Zone*> activeSelectedTrackFXZones_;
    vector<Zone*> activeSelectedTrackFXMenuZones_;
    vector<Zone*> activeSelectedTrackFXMenuFXZones_;
    vector<Zone*> activeFocusedFXZones_;
    
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
    
    bool GetShouldMapSelectedTrackFXMenus() { return shouldMapSelectedTrackFXMenus_; }
    bool GetShouldMapSelectedTrackFX() { return shouldMapSelectedTrackFX_; }
    bool GetShouldMapFocusedFX() { return shouldMapFocusedFX_; }
    int GetNumFXSlots() { return numFXlots_; }
    void SetNumFXSlots(int numFXSlots) { numFXlots_ = numFXSlots; }
    bool GetShowFXWindows() { return showFXWindows_; }
    
    void SetShouldMapSelectedTrackFX(bool shouldMapSelectedTrackFX) { shouldMapSelectedTrackFX_ = shouldMapSelectedTrackFX; }
    void SetShouldMapSelectedTrackFXMenus(bool shouldMapSelectedTrackFXMenus) { shouldMapSelectedTrackFXMenus_ = shouldMapSelectedTrackFXMenus; }
    void SetShouldMapFocusedFX(bool shouldMapFocusedFX) { shouldMapFocusedFX_ = shouldMapFocusedFX; }
    void ToggleMapSelectedTrackFX();
    void ToggleMapFocusedFX();
    void ToggleMapSelectedTrackFXMenu();
    void MapSelectedTrackFXToWidgets();
    void MapSelectedTrackFXToMenu();
    void MapSelectedTrackFXSlotToWidgets(int slot);
    void MapFocusedFXToWidgets();
    
    Zone* GetActiveZone(WidgetActionManager* widgetActionManager)
    {
        for(auto zone : activeSelectedTrackFXZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;
        
        for(auto zone : activeSelectedTrackFXMenuFXZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;
        
        for(auto zone : activeFocusedFXZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;

        return nullptr;
    }
    
    vector<Zone*> GetActiveZones()
    {
        vector<Zone*> activeFXZones;
        
        for(auto zone : activeSelectedTrackFXZones_)
            activeFXZones.push_back(zone);
        
        for(auto zone : activeFocusedFXZones_)
            activeFXZones.push_back(zone);
        
        return activeFXZones;
    }

    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
    }
    
    void TrackFXListChanged()
    {
        if(shouldMapSelectedTrackFX_)
            MapSelectedTrackFXToWidgets();
        
        if(shouldMapFocusedFX_)
            MapFocusedFXToWidgets();
        
        if(shouldMapSelectedTrackFXMenus_)
            MapSelectedTrackFXToMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, bool useZoneLink);
    CSurfIntegrator* CSurfIntegrator_ = nullptr;
    Page* page_ = nullptr;
    const string name_ = "";
    vector<Widget*> widgets_;

    FXActivationManager* fxActivationManager_ = nullptr;
    SendsActivationManager* sendsActivationManager_ = nullptr;

    map<string, Zone*> zones_;
    Zone* activeZone_ = nullptr;
    bool useZoneLink_ = false;

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
    vector<Widget*> &GetWidgets() { return widgets_; }
    map<string, Zone*> &GetZones() { return zones_;}
    FXActivationManager* GetFXActivationManager() { return fxActivationManager_; }
    bool GetUseZoneLink() { return useZoneLink_; }
    bool GetShouldMapSends() { return sendsActivationManager_->GetShouldMapSends(); }
    void SetShouldMapSends(bool shouldMapSends) { sendsActivationManager_->SetShouldMapSends(shouldMapSends); }
    int GetNumSendSlots() { return sendsActivationManager_->GetNumSendSlots(); }
    void SetNumSendSlots(int numSendSlots) { sendsActivationManager_->SetNumSendSlots(numSendSlots); }
    virtual void ResetAll() {}
    virtual void LoadingZone(string zoneName) {}
    virtual void HandleOSCInput() {}

    WidgetActionManager* GetHomeWidgetActionManagerForWidget(Widget* widget);
    string GetZoneAlias(string ZoneName);
    string GetLocalZoneAlias(string ZoneName);
    int GetParentZoneIndex(Zone* childZone);
    bool AddZone(Zone* zone);
    void GoZone(string zoneName);
   
    void ToggleMapSends()
    {
        sendsActivationManager_->ToggleMapSends();
    }
    
    void MapSelectedTrackSendsToWidgets()
    {
        sendsActivationManager_->MapSelectedTrackSendsToWidgets(zones_);
    }
    
    virtual void Run()
    {
        RequestUpdate(); // if you add any more code here RequestUpdate() should always be last so that state changes are complete
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
    map<int, Midi_CSIMessageGenerator*> CSIMessageGeneratorsByMidiMessage_;
    
    void InitWidgets(string templateFilename);
    void ProcessMidiMessage(const MIDI_event_ex_t* evt);

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

public:
    Midi_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, midi_Input* midiInput, midi_Output* midiOutput, bool useZoneLink)
    : ControlSurface(CSurfIntegrator, page, name, useZoneLink), midiInput_(midiInput), midiOutput_(midiOutput)
    {
        InitWidgets(templateFilename);
        
        ResetAllWidgets();
        
        // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
        InitZones(zoneFolder);
        
        GoZone("Home");
    }
    
    virtual ~Midi_ControlSurface() {}
    
    void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(int first, int second, int third);
    
    virtual void Run() override
    {
        HandleMidiInput();
        ControlSurface::Run(); // this should always be last so that state changes caused by handling input are complete
    }
    
    void AddCSIMessageGenerator(int message, Midi_CSIMessageGenerator* messageGenerator)
    {
        CSIMessageGeneratorsByMidiMessage_[message] = messageGenerator;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSC_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string remoteDeviceIP_ = "";
    int inPort_ = 0;
    int outPort_ = 0;
    oscpkt::UdpSocket inSocket_;
    oscpkt::UdpSocket outSocket_;
    oscpkt::PacketReader packetReader_;
    oscpkt::PacketWriter packetWriter_;
    map<string, OSC_CSIMessageGenerator*> CSIMessageGeneratorsByOSCMessage_;
    
    void InitWidgets(string templateFilename);
    void ProcessOSCMessage(string message, double value);
    
    void runServer()
    {
        inSocket_.bindTo(inPort_);

        if (!inSocket_.isOk())
        {
            //cerr << "Error opening port " << PORT_NUM << ": " << inSocket_.errorMessage() << "\n";
            return;
        }
        
        if( ! outSocket_.connectTo(remoteDeviceIP_, outPort_))
        {
            //cerr << "Error connecting " << remoteDeviceIP_ << ": " << outSocket_.errorMessage() << "\n";
            return;
        }
        
        outSocket_.bindTo(outPort_);
       
        if ( ! outSocket_.isOk())
        {
            //cerr << "Error opening port " << outPort_ << ": " << outSocket_.errorMessage() << "\n";
            return;
        }
    }
   
    void HandleOSCInput() override
    {
        if(inSocket_.isOk())
        {
            while (inSocket_.receiveNextPacket(0))  // timeout, in ms
            {
                packetReader_.init(inSocket_.packetData(), inSocket_.packetSize());
                oscpkt::Message *message;
                
                while (packetReader_.isOk() && (message = packetReader_.popMessage()) != 0)
                {
                    float value = 0;
                    
                    if(message->arg().isFloat())
                    {
                        message->arg().popFloat(value);
                        ProcessOSCMessage(message->addressPattern(), value);
                    }
                }
            }
        }
    }
    
public:
    OSC_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int inPort, int outPort, bool useZoneLink, string remoteDeviceIP);
    virtual ~OSC_ControlSurface() {}
    
    virtual void LoadingZone(string zoneName) override;
    void SendOSCMessage(string oscAddress, double value);
    void SendOSCMessage(string oscAddress, string value);
    
    virtual void ResetAll() override
    {
        LoadingZone("Home");
    }
    
    virtual void Run() override
    {
        ControlSurface::Run(); // this should always be last so that state changes caused by handling input are complete
    }
    
    void AddCSIMessageGenerator(string message, OSC_CSIMessageGenerator* messageGenerator)
    {
        CSIMessageGeneratorsByOSCMessage_[message] = messageGenerator;
    }
};

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
    
protected:
    TrackNavigationManager* manager_ = nullptr;
    TrackNavigator(TrackNavigationManager* manager) : manager_(manager) {}
    
public:
    TrackNavigator(TrackNavigationManager* manager, int channelNum) : manager_(manager), channelNum_(channelNum) {}
    virtual ~TrackNavigator() {}
    
    virtual void SetTouchState(bool isChannelTouched) { isChannelTouched_ = isChannelTouched; }
    bool GetIsChannelTouched() { return isChannelTouched_; }
    bool GetIsChannelPinned() { return isChannelPinned_; }
    virtual bool GetIsFocusedFXNavigator() { return false; }
    void IncBias() { bias_++; }
    void DecBias() { bias_--; }
    
    virtual void Pin();
    virtual void Unpin();
    
    virtual string GetName() { return "TrackNavigator"; }
    
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
    virtual void Pin() override {}
    virtual void Unpin() override {}
    
    virtual string GetName() override { return "SelectedTrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXNavigator(TrackNavigationManager* manager) : TrackNavigator(manager) {}
    virtual ~FocusedFXNavigator() {}
    
    virtual bool GetIsFocusedFXNavigator() override { return true; }
    
    virtual void SetTouchState(bool isChannelTouched) override {}
    virtual void Pin() override {}
    virtual void Unpin() override {}
    
    virtual string GetName() override { return "FocusedFXNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Page* page_ = nullptr;
    bool followMCP_ = true;
    bool synchPages_ = false;
    bool scrollLink_ = false;
    int targetScrollLinkChannel_ = 0;
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
        if(track == tracks_[tracks_.size() - 1]) // GAW TBD -- prevent Pinning last Track -- this is a hack because of a bug in subtract_vectors, or maybe ny usage :)
            return;
        
        for(auto navigator : trackNavigators_)
        {
            if(track == navigator->GetTrack())
            {
                if(navigator->GetIsChannelPinned())
                    navigator->Unpin();
                else
                    navigator->Pin();
                
                break;
            }
        }
    }
    
    Page* GetPage() { return page_; }
    bool GetFollowMCP() { return followMCP_; }
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    int  GetNumTracks() { return tracks_.size(); }
    
    void SetScrollLink(bool scrollLink) { scrollLink_ = scrollLink; }
    
    TrackNavigator* AddTrackNavigator();
    void OnTrackSelection();
    void OnTrackSelectionBySurface(MediaTrack* track);
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
        //int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        tracks_.clear();
        
        // Get Visible Tracks
        for (int i = 1; i <= DAW::CSurf_NumTracks(followMCP_); i++)
        {
            MediaTrack* track = DAW::CSurf_TrackFromID(i, followMCP_);
            
            if(DAW::IsTrackVisible(track, followMCP_))
                tracks_.push_back(track);
             
                // I_FOLDERDEPTH : int * : folder depth change (0=normal, 1=track is a folder parent, -1=track is the last in the innermost folder, -2=track is the last in the innermost and next-innermost folders, etc

                //int retVal = DAW::GetMediaTrackInfo_Value(track, "I_FOLDERDEPTH");
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
        
        /*
         cycles++;
         
         if(cycles > 60)
         {
         cycles = 0;
         
         int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
         
         char msgBuffer[250];
         
         sprintf(msgBuffer, "%d microseconds for TrackNavigator run method\n", duration);
         DAW::ShowConsoleMsg(msgBuffer);
         }
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
        targetScrollLinkChannel_ = targetChannel - 1 < 0 ? 0 : targetChannel - 1;
        
        scrollLink_ = ! scrollLink_;
        
        OnTrackSelection();
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
    TrackNavigationManager* GetTrackNavigationManager() { return trackNavigationManager_; }
    
    vector<ControlSurface*> &GetSurfaces() { return surfaces_; }
    
    void OpenLearnModeWindow();
    void CloseLearnModeWindow();
    void ToggleLearnMode();
    void InputReceived(Widget* widget);
    void ActionPerformed(WidgetActionManager* widgetActionManager, Action* action);

    
    void HandleOSCInput()
    {
        for(auto surface : surfaces_)
            surface->HandleOSCInput();
    }
    
    void Run()
    {
        trackNavigationManager_->Run();
        
        for(auto surface : surfaces_)
            surface->Run();
    }
    
    void ResetAll()
    {
        for(auto surface : surfaces_)
            surface->ResetAll();
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
        
        if(isShift_ /*|| GetAsyncKeyState(VK_SHIFT)*/)
            modifiers += Shift;
        if(isOption_)
            modifiers += Option;
        if(isControl_  /*|| GetAsyncKeyState(VK_CONTROL)*/)
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
    
    void OnTrackSelectionBySurface(MediaTrack* track)
    {
        trackNavigationManager_->OnTrackSelectionBySurface(track);
        
        for(auto surface : surfaces_)
            surface->OnTrackSelection();
    }
 
    MediaTrack* GetSelectedTrack() { return trackNavigationManager_->GetSelectedTrack(); }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        for(auto surface : surfaces_)
            surface->OnFXFocus(track, fxIndex);
    }

    string GetZoneAlias(string zoneName)
    {
        for(auto surface : surfaces_)
            if(surface->GetZoneAlias(zoneName) != "")
                return surface->GetZoneAlias(zoneName);
            
        return "";
    }
    
    void GoZone(ControlSurface* surface, string zoneName)
    {
        if(! surface->GetUseZoneLink())
          surface->GoZone(zoneName);
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GoZone(zoneName);
    }

    void ToggleMapSends(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->ToggleMapSends();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->ToggleMapSends();
    }
    
    void ToggleMapSelectedFX(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->ToggleMapSelectedTrackFX();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->ToggleMapSelectedTrackFX();
    }
    
    void ToggleMapFXMenu(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->ToggleMapSelectedTrackFXMenu();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->ToggleMapSelectedTrackFXMenu();
    }
    
    void ToggleMapFocusedTrackFX(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->ToggleMapFocusedFX();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->ToggleMapFocusedFX();
    }
    
    void MapSelectedTrackSendsToWidgets(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->MapSelectedTrackSendsToWidgets();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->MapSelectedTrackSendsToWidgets();
    }
    
    void MapSelectedTrackFXToWidgets(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->MapSelectedTrackFXToWidgets();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->MapSelectedTrackFXToWidgets();
    }
    
    void MapSelectedTrackFXToMenu(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->MapSelectedTrackFXToMenu();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->MapSelectedTrackFXToMenu();
    }
    
    void MapFocusedTrackFXToWidgets(ControlSurface* surface)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->MapFocusedFXToWidgets();
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->MapFocusedFXToWidgets();
    }
    
    void MapSelectedTrackFXSlotToWidgets(ControlSurface* surface, int fxIndex)
    {
        if(! surface->GetUseZoneLink())
            surface->GetFXActivationManager()->MapSelectedTrackFXSlotToWidgets(fxIndex);
        else
            for(auto surface : surfaces_)
                if(surface->GetUseZoneLink())
                    surface->GetFXActivationManager()->MapSelectedTrackFXSlotToWidgets(fxIndex);
    }
    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto surface : surfaces_)
            surface->GetFXActivationManager()->TrackFXListChanged();
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
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfIntegrator* CSurfIntegrator_ = nullptr;
    map<string, function<Action*(string name, WidgetActionManager* manager, vector<string>)>> actions_;
    vector <Page*> pages_;
    
    map<string, map<string, int>> fxParamIndices_;
    
    int currentPageIndex_ = 0;
    bool surfaceInMonitor_ = false;
    bool surfaceOutMonitor_ = false;
    bool fxMonitor_ = false;

    
    
    bool oscInMonitor_ = false;
    bool oscOutMonitor_ = false;
    
    
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
    Manager(CSurfIntegrator* CSurfIntegrator) : CSurfIntegrator_(CSurfIntegrator)
    {
        InitActionDictionary();
    }
    
    void ResetAllWidgets()
    {
        fxMonitor_ = false;
        surfaceInMonitor_ = false;
        surfaceOutMonitor_ = false;
        oscInMonitor_ = false;
        oscOutMonitor_ = false;

        if(pages_.size() > 0)
        {
            pages_[currentPageIndex_]->ResetAll();
            pages_[currentPageIndex_]->ResetAllWidgets();
        }
    }
    
    void Init();

    void SetFXMonitor(bool value) { fxMonitor_ = value;  }
    void SetSurfaceInMonitor(bool value) { surfaceInMonitor_ = value;  }
    void SetSurfaceOutMonitor(bool value) { surfaceOutMonitor_ = value;  }
    
    bool GetFXMonitor() { return fxMonitor_;  }
    bool GetSurfaceInMonitor() { return surfaceInMonitor_;  }
    bool GetSurfaceOutMonitor() { return surfaceOutMonitor_;  }
    
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    vector<string> GetActionNames()
    {
        vector<string> actionNames;
        
        for(auto [key, value] : actions_)
            actionNames.push_back(key);
        
        return actionNames;
    }
    
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
            return actions_[params[0]](params[0], manager, params);
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
    
    void HandleOSCInput()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->HandleOSCInput();
    }
    
    void Run()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->Run();
    }
    
    void AdjustTrackBank(Page* sendingPage, int amount)
    {
        if(! sendingPage->GetTrackNavigationManager()->GetSynchPages())
            sendingPage->GetTrackNavigationManager()->AdjustTrackBank(amount);
        else
            for(auto page: pages_)
                if(page->GetTrackNavigationManager()->GetSynchPages())
                    page->GetTrackNavigationManager()->AdjustTrackBank(amount);
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
    
    void OpenLearnModeWindow()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OpenLearnModeWindow();
    }
    
    void CloseLearnModeWindow()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->CloseLearnModeWindow();
    }
        
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto & page : pages_)
            page->TrackFXListChanged(track);
        
        if(fxMonitor_)
        {
            char fxName[BUFSZ];
            char fxParamName[BUFSZ];
            
            for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
            {
                DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
                DAW::ShowConsoleMsg(("\n\n" + string(fxName)).c_str());
            
                for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                {
                    DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                    
                    double stepOut = 0;
                    double smallstepOut = 0;
                    double largestepOut = 0;
                    bool istoggleOut = false;
                    
                    DAW::ShowConsoleMsg(("\n\n" + string(fxParamName)).c_str());
                    
                    TrackFX_GetParameterStepSizes(track, i, j, &stepOut, &smallstepOut, &largestepOut, &istoggleOut);
                    
                    DAW::ShowConsoleMsg(("\n" + to_string(stepOut) + " " +  to_string(smallstepOut) + " " + to_string(largestepOut) + " " + to_string(istoggleOut) + " ").c_str());
                }
            }
        }
    }
};

// GetAsyncKeyState(VK_SHIFT)

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
