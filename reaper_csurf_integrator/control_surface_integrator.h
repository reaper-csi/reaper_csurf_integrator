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
#include "commctrl.h"
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
const string EuConSurfaceToken = "EuConSurface";
const string PageToken = "Page";
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
const string FXParam = "FXParam";

const string BadFileChars = "[ \\:*?<>|.,()/]";
const string CRLFChars = "[\r\n]";
const string TabChars = "[\t]";

extern int __g_projectconfig_timemode2, __g_projectconfig_timemode;

class Manager;
extern Manager* TheManager;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfIntegrator;
class Page;
class ControlSurface;
class Midi_ControlSurface;
class OSC_ControlSurface;
class EuCon_ControlSurface;
class Zone;
class Widget;
class Action;
class TrackNavigationManager;
class FeedbackProcessor;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool isZoneSet_ = false;
    
protected:
    Zone* const zone_;
    
public:
    Navigator(Zone* zone) : zone_(zone) { }
    virtual ~Navigator() { }

    virtual string GetName() { return ""; }
    virtual MediaTrack* GetTrack() { return nullptr; }
    virtual bool GetIsZoneTouched() { return false; }
    virtual bool GetIsChannelPinned() { return false; }
    virtual void IncBias() { }
    virtual void DecBias() { }
    virtual void Pin() {}
    virtual void Unpin() {}
    virtual bool GetIsFocusedFXNavigator() { return false; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigator : public Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channelNum_ = 0;
    int bias_ = 0;
    MediaTrack* pinnedTrack_ = nullptr;
    bool isChannelPinned_ = false;
    
protected:
    TrackNavigationManager* const manager_;

public:
    TrackNavigator(Zone* zone, TrackNavigationManager* manager, int channelNum) : Navigator(zone), manager_(manager), channelNum_(channelNum) {}
    TrackNavigator(Zone* zone, TrackNavigationManager* manager) : Navigator(zone), manager_(manager) {}
    virtual ~TrackNavigator() {}
    
    virtual bool GetIsChannelPinned() override { return isChannelPinned_; }
    virtual void IncBias() override { bias_++; }
    virtual void DecBias() override { bias_--; }
    
    virtual bool GetIsZoneTouched() override;
    virtual void Pin() override;
    virtual void Unpin() override;
    
    virtual string GetName() override { return "TrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MasterTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MasterTrackNavigator(Zone* zone, TrackNavigationManager* manager) : TrackNavigator(zone, manager) {}
    virtual ~MasterTrackNavigator() {}
    
    virtual string GetName() override { return "MasterTrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectedTrackNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectedTrackNavigator(Zone* zone, TrackNavigationManager* manager) : TrackNavigator(zone, manager) {}
    virtual ~SelectedTrackNavigator() {}
    
    //virtual void SetTouchState(bool isZoneTouched) override {}
    //virtual void Pin() override {}
    //virtual void Unpin() override {}
    
    virtual string GetName() override { return "SelectedTrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXNavigator : public TrackNavigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXNavigator(Zone* zone, TrackNavigationManager* manager) : TrackNavigator(zone, manager) {}
    virtual ~FocusedFXNavigator() {}
    
    virtual bool GetIsFocusedFXNavigator() override { return true; }
    
    //virtual void SetTouchState(bool isZoneTouched) override {}
    //virtual void Pin() override {}
    //virtual void Unpin() override {}
    
    virtual string GetName() override { return "FocusedFXNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_;
    string const name_;
    string activeZoneName = "";
    bool isModifier_ = false;
    double lastValue_ = 0.0;
    string lastStringValue_ = "";

    vector<FeedbackProcessor*> feedbackProcessors_;

    map<string, map<string, vector <Action*>>> actions_;   // vector<Action*> actionList = actions_[zoneName][modifiers];
    map<string, map<string, vector <Action*>>> trackTouchedActions_;
public:
    Widget(ControlSurface* surface, string name) : surface_(surface), name_(name) {}
    virtual ~Widget() {};
    
    ControlSurface* GetSurface() { return surface_; }
    string GetName() { return name_; }
    void AddAction(string zoneName, string modifiers, Action* action)  { actions_[zoneName][modifiers].push_back(action); }
    void AddTrackTouchedAction(string zoneName, string modifiers, Action* action) { trackTouchedActions_[zoneName][modifiers].push_back(action); }
    void AddFeedbackProcessor(FeedbackProcessor* feedbackProcessor) { feedbackProcessors_.push_back(feedbackProcessor); }
    void SetIsModifier() { isModifier_ = true; }
    double GetLastValue() { return lastValue_; }
    
    void GoZone(string zoneName)
    {
        if(actions_.count(zoneName) > 0)
            activeZoneName = zoneName;
    }
    
    void Reset()
    {
        SetValue(0.0);
        SetValue(0, 0.0);
        SetValue("");
        SetRGBValue(0, 0, 0);
    }

    MediaTrack* GetTrack();
    void RequestUpdate();
    void DoAction(double value);
    void DoRelativeAction(double value);
    void SetIsTouched(bool isTouched);
    void SetValue(double value);
    void SetValue(int mode, double value);
    void SetValue(string value);
    void SetRGBValue(int r, int g, int b);
    void ClearCache();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool supportsRGB_ = false;
    vector<rgb_color> RGBValues_;
    
    int currentRGBIndex_ = 0;
    
    void SetRGB(vector<string> params)
    {
        vector<int> rawValues;
        
        auto openCurlyBrace = find(params.begin(), params.end(), "{");
        auto closeCurlyBrace = find(params.begin(), params.end(), "}");
        
        if(openCurlyBrace != params.end() && closeCurlyBrace != params.end())
        {
            for(auto it = openCurlyBrace + 1; it != closeCurlyBrace; ++it)
            {
                string strVal = *(it);
                
                if(regex_match(strVal, regex("[0-9]+")))
                {
                    int value = stoi(strVal);
                    value = value < 0 ? 0 : value;
                    value = value > 255 ? 255 : value;
                    
                    rawValues.push_back(value);
                }
            }
        }
        
        if(rawValues.size() % 3 == 0 && rawValues.size() > 2)
        {
            supportsRGB_ = true;
            
            for(int i = 0; i < rawValues.size(); i += 3)
            {
                rgb_color color;
                
                color.r = rawValues[i];
                color.g = rawValues[i + 1];
                color.b = rawValues[i + 2];
                
                RGBValues_.push_back(color);
            }
        }
    }
    
    void SetSteppedValues(vector<string> params)
    {
        auto openSquareBrace = find(params.begin(), params.end(), "[");
        auto closeCurlyBrace = find(params.begin(), params.end(), "]");
        
        if(openSquareBrace != params.end() && closeCurlyBrace != params.end())
        {
            for(auto it = openSquareBrace + 1; it != closeCurlyBrace; ++it)
            {
                string strVal = *(it);
                
                if(regex_match(strVal, regex("[0-9]+[.][0-9]+")))
                    steppedValues_.push_back(stod(strVal));
            }
        }
    }
    
protected:
    Action(string name, Widget* widget, Zone* zone, vector<string> params);
    
    string const name_;
    Widget* const widget_;
    Zone* const zone_;
    
    vector<double> steppedValues_;
    
    bool isInverted_ = false;
    bool shouldToggle_ = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    
    virtual void RequestTrackUpdate(MediaTrack* track) {}
    
public:
    virtual ~Action() {}
    
    Page* GetPage();
    Widget* GetWidget() { return widget_; }
    Zone* GetZone() { return zone_; }
    ControlSurface* GetSurface();
    
    virtual string GetDisplayName() { return ""; }
    string GetName() { return name_; }
    
    virtual string GetParamNumAsString() { return ""; }
    virtual int GetParamNum() { return 0; }
    virtual string GetAlias() { return ""; }
    bool GetSupportsRGB() { return supportsRGB_; }
    vector<rgb_color> &GetRGBValues() { return  RGBValues_; }
    
    void SetIsInverted() { isInverted_ = true; }
    void SetShouldToggle() { shouldToggle_ = true; }
    void SetDelayAmount(double delayAmount) { delayAmount_ = delayAmount; }
    
    virtual void SetIndex(int index) {}
    
    virtual void DoAction(double value, Widget* sender);
    
    virtual double GetCurrentValue() { return 0.0; }
    virtual void RequestUpdate()
    {
        if(supportsRGB_)
            GetWidget()->SetRGBValue(RGBValues_[0].r, RGBValues_[0].g, RGBValues_[0].b);
    }
    
    virtual void Do(string value, Widget* sender) {}
    virtual void Do(double value, Widget* sender) {}
    
    void SetCurrentRGB(rgb_color newColor)
    {
        supportsRGB_ = true;
        RGBValues_[currentRGBIndex_] = newColor;
    }
    
    rgb_color GetCurrentRGB()
    {
        rgb_color blankColor;
        
        if(RGBValues_.size() > 0 && currentRGBIndex_ < RGBValues_.size())
            return RGBValues_[currentRGBIndex_];
        else return blankColor;
    }
    
    void SetWidgetValue(Widget* widget, double value)
    {
        value = isInverted_ == false ? value : 1.0 - value;
        
        widget->SetValue(value);
        
        currentRGBIndex_ = value == 0 ? 0 : 1;
        
        if(supportsRGB_)
            widget->SetRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
    }
    
    void SetWidgetValue(Widget* widget, int param, double value)
    {
        value = isInverted_ == false ? value : 1.0 - value;
        
        widget->SetValue(param, value);
        
        currentRGBIndex_ = value == 0 ? 0 : 1;
        
        if(supportsRGB_)
            widget->SetRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
    }
    
    void SetWidgetValue(Widget* widget, string value)
    {
        widget->SetValue(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NoAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NoAction(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    virtual ~NoAction() {}
    
    virtual void RequestUpdate() { GetWidget()->Reset(); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string const name_;
    string const alias_;
    string const sourceFilePath_;
    Navigator* navigator_ = nullptr;
    vector<Zone*> includedZones_;
    int index_ = 0;
    bool isTouched_ = false;
    
public:
    Zone(string navigatorName, ControlSurface* surface, Zone* parentZone, int channelNum, string name, string sourceFilePath, string alias);
    virtual ~Zone() {}
    
    int GetIndex() { return index_; }
    string GetName() { return name_ ;}
    string GetAlias() { return alias_;}
    string GetSourceFilePath() { return sourceFilePath_; }
    Navigator* GetNavigator() { return navigator_; }
    void SetIsTouched(bool isTouched) { isTouched_ = isTouched; }
    bool GetIsTouched() { return isTouched_; }
    void Activate(ControlSurface* surface);
    
    // GAW TBD -- remove -- change learn mode editor to file based
    vector<Zone*> &GetIncludedZones() { return includedZones_; }
    
    // GAW TBD -- maybe allow this later after fully debugged
    //void SetTrackNavigator(Navigator* navigator) { navigator_ = navigator; }
    //void SetAlias(string alias) { alias_ = alias;}
    
    string GetNavigatorName()
    {
        if(navigator_ != nullptr)
            return navigator_->GetName();
        else
            return "";
    }
    
    bool GetHasFocusedFXTrackNavigator()
    {
        if(navigator_ == nullptr)
            return false;
        else
            return navigator_->GetIsFocusedFXNavigator();
    }
    
    void AddZone(Zone* zone)
    {
        includedZones_.push_back(zone);
    }
    
    void Activate(ControlSurface* surface, int index)
    {
        index_ = index;
        Activate(surface);
        
        for(auto zone : includedZones_)
            zone->Activate(surface, index);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Widget* const widget_;
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
class EuCon_CSIMessageGenerator : public CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    EuCon_CSIMessageGenerator(EuCon_ControlSurface* surface, Widget* widget, string message);
    virtual ~EuCon_CSIMessageGenerator() {}
    
    virtual void ProcessMessage(string message, double value)
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
    virtual void SetRGBValue(int r, int g, int b) {}
    virtual void ClearCache() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_FeedbackProcessor : public FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Midi_ControlSurface* const surface_ = nullptr;
    
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
    OSC_ControlSurface* const surface_ = nullptr;
    string oscAddress_ = "";
    double lastDoubleValue_ = 0.0;
    string lastStringValue_ = "";
    
public:
    
    OSC_FeedbackProcessor(OSC_ControlSurface* surface, string oscAddress) : surface_(surface), oscAddress_(oscAddress) {}
    ~OSC_FeedbackProcessor() {}
    
    virtual void SetValue(double value) override;
    virtual void SetValue(int param, double value) override;
    virtual void SetValue(string value) override;
    
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
        lastStringValue_ = "";
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuCon_FeedbackProcessor : public FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    EuCon_ControlSurface* const surface_ = nullptr;
    string address_ = "";
    double lastDoubleValue_ = 0.0;
    string lastStringValue_ = "";
    
public:
    
    EuCon_FeedbackProcessor(EuCon_ControlSurface* surface, string address) : surface_(surface), address_(address) {}
    ~EuCon_FeedbackProcessor() {}
    
    virtual void SetValue(double value) override;
    virtual void SetValue(int param, double value) override;
    virtual void SetValue(string value) override;
    
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
        lastStringValue_ = "";
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SendsActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_ = nullptr;
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

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXWindow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string fxName = "";
    MediaTrack* const track = nullptr;;
    int fxIndex = 0;
    
    FXWindow(string anFxName, MediaTrack* aTrack, int anFxIndex) : fxName(anFxName), track(aTrack), fxIndex(anFxIndex) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_ = nullptr;
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
    
    Zone* GetActiveZone(Widget* sender)
    {
        /*
        for(auto zone : activeSelectedTrackFXZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;
        
        for(auto zone : activeSelectedTrackFXMenuFXZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;
        
        for(auto zone : activeFocusedFXZones_)
            if(zone->ContainsWidgetActionManager(widgetActionManager))
                return zone;
         */

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

    void ToggleShowFXWindows()
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
    ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name) : CSurfIntegrator_(CSurfIntegrator), page_(page), name_(name),
    fxActivationManager_(new FXActivationManager(this)), sendsActivationManager_(new SendsActivationManager(this))  { }

    CSurfIntegrator* const CSurfIntegrator_ ;
    Page* const page_;
    string const name_;
    vector<Widget*> widgets_;

    FXActivationManager* const fxActivationManager_;
    SendsActivationManager* const sendsActivationManager_ = nullptr;

    map<string, Zone*> zones_;
    map<string, vector<Zone*>> zonesInZoneFile_;
    bool useZoneLink_ = false;

    void InitZones(string zoneFolder);
    
    void InitHardwiredWidgets()
    {
        // Add the "hardwired" widgets
        widgets_.push_back(new Widget(this, "OnTrackSelection"));
        widgets_.push_back(new Widget(this, "OnFXFocus"));
    }
    
public:
    virtual ~ControlSurface() {};
    
    Page* GetPage() { return page_; }
    string GetName() { return name_; }
    virtual string GetSourceFileName() { return ""; }
    vector<Widget*> &GetWidgets() { return widgets_; }
    map<string, Zone*> &GetZones() { return zones_;}
    map<string, vector<Zone*>> &GetZonesInZoneFile() { return zonesInZoneFile_; }
    FXActivationManager* GetFXActivationManager() { return fxActivationManager_; }
    bool GetUseZoneLink() { return useZoneLink_; }
    void SetUseZoneLink(bool useZoneLink) { useZoneLink_ = useZoneLink; }
    bool GetShouldMapSends() { return sendsActivationManager_->GetShouldMapSends(); }
    void SetShouldMapSends(bool shouldMapSends) { sendsActivationManager_->SetShouldMapSends(shouldMapSends); }
    int GetNumSendSlots() { return sendsActivationManager_->GetNumSendSlots(); }
    void SetNumSendSlots(int numSendSlots) { sendsActivationManager_->SetNumSendSlots(numSendSlots); }
    virtual void ResetAll() {}
    virtual void LoadingZone(string zoneName) {}
    virtual void HandleExternalInput() {}
    virtual void InitializeEuCon() {}
    virtual void InitializeEuConWidget(string name, string control, string FB_Processor) {}
    virtual void EuConInitializationComplete() {}
    virtual void ReceiveEuConMessage(string oscAddress, double value) {}
    virtual void ReceiveEuConMessage(string oscAddress, string value) {}

    string GetZoneAlias(string ZoneName);
    string GetLocalZoneAlias(string ZoneName);
    bool AddZone(Zone* zone);
    void GoZone(string zoneName);
    void GoHome() { GoZone("Home"); }
    
    void WidgetsGoZone(string zoneName)
    {
        for(auto widget : widgets_)
            widget->GoZone(zoneName);
    }
    
    bool GetIsTouched(string zoneName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->GetIsTouched();
        else
            return false;
    }
    
    void SetIsTouched(string zoneName, bool isTouched)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetIsTouched(isTouched);
    }
    
    void ToggleMapSends()
    {
        sendsActivationManager_->ToggleMapSends();
    }
    
    void MapSelectedTrackSendsToWidgets()
    {
        sendsActivationManager_->MapSelectedTrackSendsToWidgets(zones_);
    }
    
    void RequestUpdate()
    {
        for(auto widget : widgets_)
            widget->RequestUpdate();
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
    string templateFilename_ = "";
    midi_Input* midiInput_ = nullptr;
    midi_Output* midiOutput_ = nullptr;
    map<int, vector<Midi_CSIMessageGenerator*>> CSIMessageGeneratorsByMidiMessage_;
    
    void ProcessMidiMessage(const MIDI_event_ex_t* evt);
   
    void InitWidgets(string templateFilename);

public:
    Midi_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, midi_Input* midiInput, midi_Output* midiOutput)
    : ControlSurface(CSurfIntegrator, page, name), templateFilename_(templateFilename), midiInput_(midiInput), midiOutput_(midiOutput)
    {
        InitWidgets(templateFilename);
        
        ResetAllWidgets();
        
        // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
        InitZones(zoneFolder);
        
        GoHome();
    }
    
    virtual ~Midi_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "/CSI/Surfaces/Midi/" + templateFilename_; }
    
    void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(int first, int second, int third);

    bool hasSetGlobalSysEx_ = false;

    virtual void HandleExternalInput() override
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
    
    void AddCSIMessageGenerator(int message, Midi_CSIMessageGenerator* messageGenerator)
    {
        CSIMessageGeneratorsByMidiMessage_[message].push_back(messageGenerator);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSC_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string remoteDeviceIP_ = "";
    string templateFilename_ = "";
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
    
public:
    OSC_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int inPort, int outPort, string remoteDeviceIP)
    : ControlSurface(CSurfIntegrator, page, name), templateFilename_(templateFilename), inPort_(inPort), outPort_(outPort), remoteDeviceIP_(remoteDeviceIP)
    {
        InitWidgets(templateFilename);
        
        ResetAllWidgets();
        
        // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
        InitZones(zoneFolder);
        
        runServer();
        
        GoHome();
    }
    
    virtual ~OSC_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "/CSI/Surfaces/OSC/" + templateFilename_; }
    
    virtual void LoadingZone(string zoneName) override;
    void SendOSCMessage(string oscAddress, double value);
    void SendOSCMessage(string oscAddress, string value);
    
    virtual void ResetAll() override
    {
        LoadingZone("Home");
    }
    
    virtual void HandleExternalInput() override
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

    void AddCSIMessageGenerator(string message, OSC_CSIMessageGenerator* messageGenerator)
    {
        CSIMessageGeneratorsByOSCMessage_[message] = messageGenerator;
    }
};

class MarshalledFunctionCall;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuCon_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string zoneFolder_ = "";
    int lowChannel_ = 0;
    int highChannel_ = 0;
    int numSends_ = 0;
    int numFX_ = 0;
    int options_ = 0;
    
    map<string, EuCon_CSIMessageGenerator*> CSIMessageGeneratorsByMessage_;

    WDL_Mutex mutex_;
    list<MarshalledFunctionCall*> workQueue_;

    virtual void InitializeEuConWidget(string name, string control, string FB_Processor) override;

public:
    EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder,
                                               int lowChannel, int highChannel, int numSends, int numFX, int options);
    virtual ~EuCon_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "EuCon"; }
    
    virtual void InitializeEuCon() override;
    virtual void EuConInitializationComplete() override;
    virtual void SendEuConMessage(string oscAddress, double value);
    virtual void SendEuConMessage(string oscAddress, string value);
    virtual void ReceiveEuConMessage(string oscAddress, double value) override;
    virtual void ReceiveEuConMessage(string oscAddress, string value) override;
    virtual void HandleExternalInput() override;
    void HandleEuConMessage(string oscAddress, double value);
    void HandleEuConMessage(string oscAddress, string value);

    virtual void ResetAll() override
    {
        LoadingZone("Home");
    }
    
    void AddCSIMessageGenerator(string message, EuCon_CSIMessageGenerator* messageGenerator)
    {
        CSIMessageGeneratorsByMessage_[message] = messageGenerator;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Page* const page_ = nullptr;
    bool followMCP_ = true;
    bool synchPages_ = false;
    bool scrollLink_ = false;
    int targetScrollLinkChannel_ = 0;
    int trackOffset_ = 0;
    int folderTrackOffset_ = 0;
    vector<MediaTrack*> tracks_;
    vector<MediaTrack*> pinnedTracks_;
    vector<MediaTrack*> unpinnedTracks_;
    vector<Navigator*> navigators_;
    
public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages) : page_(page), followMCP_(followMCP), synchPages_(synchPages) {}
    
    void PinTrackToChannel(MediaTrack* track, int channelNum)
    {
        pinnedTracks_.push_back(track);
        
        for(int i = channelNum + 1; i < navigators_.size(); i++)
            navigators_[i]->IncBias();
    }
    
    void UnpinTrackFromChannel(MediaTrack* track, int channelNum)
    {
        vector<MediaTrack*>::iterator it = find(pinnedTracks_.begin(), pinnedTracks_.end(), track);
        
        if(it != pinnedTracks_.end())
            pinnedTracks_.erase(it);
        
        for(int i = channelNum + 1; i < navigators_.size(); i++)
            navigators_[i]->DecBias();
    }
    
    void TogglePin(MediaTrack* track)
    {
        if(track == tracks_[tracks_.size() - 1]) // GAW TBD -- prevent Pinning last Track -- this is a hack because of a bug in subtract_vectors, or maybe ny usage :)
            return;
        
        for(auto navigator : navigators_)
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
    
    Navigator* AddNavigator(Zone* zone);
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

    void RebuildTrackList()
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
            for(auto navigator : navigators_)
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
        
        int top = GetNumTracks() - navigators_.size();
        
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
        for(auto navigator : navigators_)
            if(navigator->GetTrack() == track && navigator->GetIsZoneTouched())
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
    rgb_color colour_;
    vector<ControlSurface*> surfaces_;
    
    bool isShift_ = false;
    double shiftPressedTime_ = 0;
    bool isOption_ = false;
    double optionPressedTime_ = 0;
    bool isControl_ = false;
    double controlPressedTime_ = 0;
    bool isAlt_ = false;
    double altPressedTime_ = 0;

    TrackNavigationManager* const trackNavigationManager_ = nullptr;
   
public:
    Page(string name, rgb_color colour, bool followMCP, bool synchPages) : name_(name), colour_(colour), trackNavigationManager_(new TrackNavigationManager(this, followMCP, synchPages)) { }
    
    string GetName() { return name_; }
    TrackNavigationManager* GetTrackNavigationManager() { return trackNavigationManager_; }
    
    vector<ControlSurface*> &GetSurfaces() { return surfaces_; }
    
    void OpenLearnModeWindow();
    void ToggleLearnMode();
    void InputReceived(Widget* widget, double value);
    void ActionPerformed(Action* action);

    bool GetShift() { return isShift_; }
    bool GetOption() { return isOption_; }
    bool GetControl() { return isControl_; }
    bool GetAlt() { return isAlt_; }
   
    void InitializeEuCon()
    {
        for(auto surface : surfaces_)
            surface->InitializeEuCon();
    }
    
    void InitializeEuConWidget(string name, string control, string FB_Processor)
    {
        for(auto surface : surfaces_)
            surface->InitializeEuConWidget(name, control, FB_Processor);
    }
    
    void EuConInitializationComplete()
    {
        for(auto surface : surfaces_)
            surface->EuConInitializationComplete();
    }
    
    void ReceiveEuConMessage(string oscAddress, double value)
    {
        for(auto surface : surfaces_)
            surface->ReceiveEuConMessage(oscAddress, value);
    }
    
    void ReceiveEuConMessage(string oscAddress, string value)
    {
        for(auto surface : surfaces_)
            surface->ReceiveEuConMessage(oscAddress, value);
    }
    
    void Run()
    {
        trackNavigationManager_->RebuildTrackList();
        
        for(auto surface : surfaces_)
            surface->HandleExternalInput();
        
        for(auto surface : surfaces_)
            surface->RequestUpdate();
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
        SetModifier(value, isShift_, shiftPressedTime_);
    }
 
    void SetOption(bool value)
    {
        SetModifier(value, isOption_, optionPressedTime_);
    }
    
    void SetControl(bool value)
    {
        SetModifier(value, isControl_, controlPressedTime_);
    }
    
    void SetAlt(bool value)
    {
        SetModifier(value, isAlt_, altPressedTime_);
    }
  
    void SetModifier(bool value, bool &modifier, double &modifierPressedTime)
    {
        if(value && modifier == false)
        {
            modifier = value;
            modifierPressedTime = DAW::GetCurrentNumberOfMilliseconds();
        }
        else
        {
            double keyReleasedTime = DAW::GetCurrentNumberOfMilliseconds();
            
            if(keyReleasedTime - modifierPressedTime > 100)
            {
                modifier = value;
            }
        }
    }

    string GetModifiers()
    {
        string modifiers = "";
        
        if(isShift_)
            modifiers += Shift + "+";
        if(isOption_)
            modifiers += Option + "+";
        if(isControl_)
            modifiers +=  Control + "+";
        if(isAlt_)
            modifiers += Alt + "+";
        
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
    
    void GoZone(ControlSurface* surface, string zoneName, Widget* sender)
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
    CSurfIntegrator* const CSurfIntegrator_;
    map<string, function<Action*(string name, Widget* widget, Zone* zone, vector<string>)>> actions_;
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
            actionNames.push_back(regex_replace(key, regex("[\"]"), ""));
        
        return actionNames;
    }
    
    Page* GetCurrentPage()
    {
        if(pages_.size() > 0)
            return pages_[currentPageIndex_];
        else
            return nullptr;
    }
    
    Action* GetAction(Widget* widget, Zone* zone, vector<string> params)
    {
        if(actions_.count(params[0]) > 0)
            return actions_[params[0]](params[0], widget, zone, params);
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
       
    void InitializeEuCon()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->InitializeEuCon();
    }
    
    void InitializeEuConWidget(string name, string control, string FB_Processor)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->InitializeEuConWidget(name, control, FB_Processor);
    }
    
    void EuConInitializationComplete()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->EuConInitializationComplete();
    }
    
    void ReceiveEuConMessage(string oscAddress, double value)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ReceiveEuConMessage(oscAddress, value);
    }
    
    void ReceiveEuConMessage(string oscAddress, string value)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ReceiveEuConMessage(oscAddress, value);
    }
    
    //int repeats = 0;
    
    void Run()
    {
        //int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->Run();
        /*
        repeats++;
        
        if(repeats > 15)
        {
            repeats = 0;
            
            int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
            
            char msgBuffer[250];
            
            sprintf(msgBuffer, "%d microseconds\n", duration);
            DAW::ShowConsoleMsg(msgBuffer);
        }
        */
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

// GetAsyncKeyState(VK_SHIFT) // get the state of the Shift key00

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
