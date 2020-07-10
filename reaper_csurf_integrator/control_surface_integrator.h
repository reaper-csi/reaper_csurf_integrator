//
//  control_surface_integrator.h
//  reaper_control_surface_integrator
//
//

//  Note for Windows environments:
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
#include <cmath>

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

const string BadFileChars = "[ \\:*?<>|.,()/]";
const string CRLFChars = "[\r\n]";
const string TabChars = "[\t]";

const int TempDisplayTime = 1250;

class Manager;
extern Manager* TheManager;

struct CSIWidgetInfo
{
    string group = "General";
    int channelNumber = 0;
    int sendNumber = 0;
    bool isVisible = true;
    string name = "";
    string control = "";
    string FB_Processor = "";
    
    CSIWidgetInfo(string aName, string aControl, string aFB_Processor) : CSIWidgetInfo(aName, aControl, aFB_Processor, "General", 0, true) {}
    
    CSIWidgetInfo(string aName, string aControl, string aFB_Processor, string aGroup, int aChannelNumber, bool isVisible) :  CSIWidgetInfo(aName, aControl, aFB_Processor, aGroup, aChannelNumber, 0, isVisible) {}
    
    CSIWidgetInfo(string aName, string aControl, string aFB_Processor, string aGroup, int aChannelNumber, int aSendNumber, bool itemIsVisible) :  name(aName), control(aControl), FB_Processor(aFB_Processor), group(aGroup), channelNumber(aChannelNumber), sendNumber(aSendNumber), isVisible(itemIsVisible) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfIntegrator;
class Page;
class ControlSurface;
class Midi_ControlSurface;
class OSC_ControlSurface;
class EuCon_ControlSurface;
class Widget;
class TrackNavigationManager;
class FeedbackProcessor;
class Zone;
class ActionContext;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* const page_ = nullptr;
    bool isFaderTouched_ = false;
    bool isRotaryTouched_ = false;

public:
    Navigator(Page*  page) : page_(page) {}
    virtual ~Navigator() {}
    
    void SetIsFaderTouched(bool isFaderTouched) { isFaderTouched_ = isFaderTouched;  }
    bool GetIsFaderTouched() { return isFaderTouched_;  }
    
    void SetIsRotaryTouched(bool isRotaryTouched) { isRotaryTouched_ = isRotaryTouched; }
    bool GetIsRotaryTouched() { return isRotaryTouched_;  }

    virtual string GetName() { return "Navigator"; }
    virtual MediaTrack* GetTrack() { return nullptr; }
    virtual bool GetIsChannelPinned() { return false; }
    virtual void IncBias() {}
    virtual void DecBias() {}
    virtual void PinChannel() {}
    virtual void UnpinChannel() {}
    virtual bool GetIsFocusedFXNavigator() { return false; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNavigator : public Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int const channelNum_ = 0;
    int bias_ = 0;
    MediaTrack* pinnedTrack_ = nullptr;
    bool isChannelPinned_ = false;
    
protected:
    TrackNavigationManager* const manager_;

public:
    TrackNavigator(Page*  page, TrackNavigationManager* manager, int channelNum) : Navigator(page), manager_(manager), channelNum_(channelNum) {}
    TrackNavigator(Page*  page, TrackNavigationManager* manager) : Navigator(page), manager_(manager) {}
    virtual ~TrackNavigator() {}
    
    virtual bool GetIsChannelPinned() override { return isChannelPinned_; }
    virtual void IncBias() override { bias_++; }
    virtual void DecBias() override { bias_--; }
    
    virtual void PinChannel() override;
    virtual void UnpinChannel() override;
    
    virtual string GetName() override { return "TrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MasterTrackNavigator : public Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MasterTrackNavigator(Page*  page) : Navigator(page) {}
    virtual ~MasterTrackNavigator() {}
    
    virtual string GetName() override { return "MasterTrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectedTrackNavigator : public Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectedTrackNavigator(Page*  page) : Navigator(page) {}
    virtual ~SelectedTrackNavigator() {}
    
    virtual string GetName() override { return "SelectedTrackNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXNavigator : public Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXNavigator(Page*  page) : Navigator(page) {}
    virtual ~FocusedFXNavigator() {}
    
    virtual bool GetIsFocusedFXNavigator() override { return true; }
    
    virtual string GetName() override { return "FocusedFXNavigator"; }
    
    virtual MediaTrack* GetTrack() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Action() {}
    
    virtual void RequestUpdate(ActionContext* context) {}
    virtual void Do(ActionContext* context, double value) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* const action_ = nullptr;
    Widget* const widget_ = nullptr;
    Zone* const zone_ = nullptr;
    
    double lastValue_ = 0.0;
    string lastStringValue_ = "";
    
    int intParam_ = 0;
    
    string stringParam_ = "";
    
    int paramIndex_ = 0;
    
    string fxParamDisplayName_ = "";
    
    
    
    int commandId_ = 0;
    string commandStr_ = "";
    
    
    double rangeMinimum_ = 0.0;
    double rangeMaximum_ = 1.0;
    
    vector<double> steppedValues_;
    int steppedValuesIndex_ = 0;
    
    double deltaValue_ = 0.0;
    vector<double> acceleratedDeltaValues_;
    vector<int> acceleratedTickValues_;
    int accumulatedIncTicks_ = 0;
    int accumulatedDecTicks_ = 0;
    
    
    bool supportsRelease_ = false;
    bool isInverted_ = false;
    bool shouldToggle_ = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    double deferredValue_ = 0.0;
    
    bool shouldUseDisplayStyle_ = false;
    int displayStyle_ = 0;
    
    bool supportsRGB_ = false;
    vector<rgb_color> RGBValues_;
    int currentRGBIndex_ = 0;
    
    bool supportsTrackColor_ = false;
    
public:
    ActionContext(Action* action, Widget* widget, Zone* zone, vector<string> params);
    virtual ~ActionContext() {}

    Widget* GetWidget() { return widget_; }
    Zone* GetZone() { return zone_; }
    int GetSlotIndex();

    int GetIntParam() { return intParam_; }
    string GetStringParam() { return stringParam_; }
    string GetFxParamDisplayName() { return fxParamDisplayName_; }
    int GetCommandId() { return commandId_; }
    string GetCommandString() { return commandStr_; }
    bool GetShouldUseDisplayStyle() { return shouldUseDisplayStyle_; }
    int GetDisplayStyle() { return displayStyle_; }

    MediaTrack* GetTrack();
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void DoRangeBoundAction(double value);
    void DoAcceleratedSteppedValueAction(int accelerationIndex, double value);
    void DoAcceleratedDeltaValueAction(int accelerationIndex, double value);
    
    
    Page* GetPage();
    ControlSurface* GetSurface();
    TrackNavigationManager* GetTrackNavigationManager();
    int GetParamIndex() { return paramIndex_; }
    
    virtual string GetAlias() { return ""; }
    bool GetSupportsRGB() { return supportsRGB_; }
    
    void SetSupportsRelease() { supportsRelease_ = true; }
    void SetIsInverted() { isInverted_ = true; }
    void SetShouldToggle() { shouldToggle_ = true; }
    void SetDelayAmount(double delayAmount) { delayAmount_ = delayAmount; }
    
    void DoAction(double value);
    void DoRelativeAction(double value);
    void DoRelativeAction(int accelerationIndex, double value);
    double GetCurrentValue() { return 0.0; }
    
    
    
    

    
    void RequestUpdate();

    
    
    
    void ClearWidget();
    void UpdateWidgetValue(double value);
    void UpdateWidgetValue(int param, double value);
    void UpdateWidgetValue(string value);
    
    void PerformDeferredActions()
    {
        if(delayAmount_ != 0.0 && delayStartTime_ != 0.0 && DAW::GetCurrentNumberOfMilliseconds() > (delayStartTime_ + delayAmount_))
        {
            double savedDelayAmount = delayAmount_;
            delayAmount_ = 0.0;
            DoAction(deferredValue_);
            delayAmount_ = savedDelayAmount;
            delayStartTime_ = 0.0;
            deferredValue_ = 0.0;
        }
    }
    
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
    
    void SetSteppedValueIndex(double value)
    {
        if(steppedValues_.size() > 0)
        {
            int index = 0;
            double delta = 100000000.0;
            
            for(int i = 0; i < steppedValues_.size(); i++)
                if(abs(steppedValues_[i] - value) < delta)
                {
                    delta = abs(steppedValues_[i] - value);
                    index = i;
                }
            
            steppedValuesIndex_ = index;
        }
    }
    
    
    
    /*
     //////////////////////////////////////////////////
     // CycleTrackAutoMode and EuConCycleTrackAutoMode
     map<int, string> autoModes_ = { {0, "Trim"}, {1, "Read"}, {2, "Touch"}, {3, "Write"}, {4, "Latch"}, {5, "LtchPre"}  };
     Widget* displayWidget_ = nullptr;
     double timeSilentlySet_ = 0.0;
     // CycleTrackAutoMode and EuConCycleTrackAutoMode
     //////////////////////////////////////////////////
     */

    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionBundle
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string modifier_ = "";
    vector<ActionContext> actionContexts_;
    
public:
    ActionBundle(string modifier) : modifier_(modifier) {}
    ActionBundle() : ActionBundle("") {}
    
    ActionBundle& operator=(ActionBundle &bundle)
    {
        this->modifier_ = bundle.modifier_;
        
        for(auto context : bundle.actionContexts_)
            this->AddActionContext(context);
        
        return *this;
    }

    string GetModifier() { return modifier_; }
        
    void RequestUpdate()
    {
        if(actionContexts_.size() > 0)
            actionContexts_[0].RequestUpdate();
    }
    
    void AddActionContext(ActionContext context)
    {
        actionContexts_.push_back(context);
    }
    
    void DoAction(double value)
    {
        for(auto context : actionContexts_)
            context.DoAction(value);
    }
    
    void DoRelativeAction(double delta)
    {
        for(auto context : actionContexts_)
            context.DoRelativeAction(delta);
    }
    
    void DoRelativeAction(int accelerationIndex, double delta)
    {
        for(auto context : actionContexts_)
            context.DoRelativeAction(accelerationIndex, delta);
    }
    
    
    
    void GetFormattedFXParamValue(MediaTrack* track, int slotIndex, char *buffer, int bufferSize)
    {
        int paramIndex = actionContexts_.size() > 0 ? actionContexts_[0].GetParamIndex() : 0;
        
        DAW::TrackFX_GetFormattedParamValue(track, slotIndex, paramIndex, buffer, bufferSize);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetActionBroker
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Widget* widget_ = nullptr;
    Zone* zone_ = nullptr;
    map<string, ActionBundle> actionBundles_;
    ActionBundle defaultBundle_;
    
public:
    WidgetActionBroker(Widget* widget, Zone* zone) : widget_(widget), zone_(zone) {}
   
    WidgetActionBroker& operator=(WidgetActionBroker &broker)
    {
        this->widget_ = broker.widget_;
        this->zone_ = broker.zone_;

        for(auto [key, actionBundle] : broker.actionBundles_)
            this->AddActionBundle(actionBundle);
        
        return *this;
    }

    void AddActionBundle(ActionBundle actionBundle)
    {
        actionBundles_[actionBundle.GetModifier()] = actionBundle;
    }
    
    ActionBundle &GetActionBundle();
    void GetFormattedFXParamValue(char *buffer, int bufferSize);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_ = nullptr;
    Navigator* const navigator_= nullptr;
    string const name_ = "";
    string const alias_ = "";
    string const sourceFilePath_ = "";
        
    int slotIndex_ = 0;

    vector<Widget*> widgets_;

public:
    Zone(ControlSurface* surface, Navigator* navigator, string name, string alias, string sourceFilePath): surface_(surface), navigator_(navigator), name_(name), alias_(alias), sourceFilePath_(sourceFilePath) {}
    
    ControlSurface* GetSurface() { return surface_; }
    Navigator* GetNavigator() { return navigator_; }
    string GetName() { return name_; }
    string GetAlias() { return alias_; }
    string GetPath() { return sourceFilePath_; }
    
    void SetSlotIndex(int index) { slotIndex_ = index; }
    int GetSlotIndex() { return slotIndex_; }

    void Deactivate();
    
    void OpenFXWindow()
    {
        if(MediaTrack* track = navigator_->GetTrack())
            DAW::TrackFX_Show(track, slotIndex_, 3);
    }
    
    void CloseFXWindow()
    {
        if(MediaTrack* track = navigator_->GetTrack())
            DAW::TrackFX_Show(track, slotIndex_, 2);
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ActionTemplate
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string actionName;
    vector<string> params;
    bool supportsRelease;
    bool isInverted;
    bool shouldToggle;
    double delayAmount;
    
    ActionTemplate(string action, vector<string> prams, bool isPR, bool isI, bool shouldT, double amount) : actionName(action), params(prams), supportsRelease(isPR), isInverted(isI), shouldToggle(shouldT), delayAmount(amount) {}
    
    void SetProperties(ActionContext &context);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ActionBundleTemplate
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string modifier = "";
    vector<ActionTemplate*> members;
    
    ActionBundleTemplate(string modifierStr) : modifier(modifierStr) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct WidgetActionTemplate
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string widgetName = "";
    bool isModifier = false;
    vector<ActionBundleTemplate*> actionBundleTemplates;
    
    WidgetActionTemplate(string widgetNameStr) : widgetName(widgetNameStr) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ZoneTemplate
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    vector<Navigator*> navigators;
    string name = "";
    string alias = "";
    string sourceFilePath = "";
    vector<string> includedZoneTemplates;
    vector<WidgetActionTemplate*> widgetActionTemplates;
    
    ZoneTemplate(vector<Navigator*> &navigatorList, string zoneName, string zoneAlias, string path, vector<string> includedZones, vector<WidgetActionTemplate*> &templates)
    : name(zoneName), alias(zoneAlias), sourceFilePath(path), includedZoneTemplates(includedZones)
    {
        for(auto navigator : navigatorList)
            navigators.push_back(navigator);

        for(auto widgetActionTemplate : templates)
            widgetActionTemplates.push_back(widgetActionTemplate);
    }
    
    void ProcessWidgetActionTemplates(ControlSurface* surface, Zone* zone, string channelNumStr);
    
    void  Activate(ControlSurface* surface, vector<Zone*> *activeZones);
    void  Activate(ControlSurface* surface, vector<Zone*> *activeZones, int slotindex);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_;
    string const name_;
    vector<FeedbackProcessor*> feedbackProcessors_;
    bool isModifier_ = false;
    
    WidgetActionBroker currentWidgetActionBroker_;
    WidgetActionBroker defaultWidgetActionBroker_;

    void LogInput(double value);

public:
    Widget(ControlSurface* surface, string name);    
    virtual ~Widget() {};
    
    ControlSurface* GetSurface() { return surface_; }
    string GetName() { return name_; }
    bool GetIsModifier() { return isModifier_; }
    void SetIsModifier() { isModifier_ = true; }
    virtual void SilentSetValue(string displayText);
    
    void GetFormattedFXParamValue(char *buffer, int bufferSize);

    void Deactivate();
    void RequestUpdate();
    void DoAction(double value);
    void DoRelativeAction(double delta);
    void DoRelativeAction(int accelerationIndex, double delta);
    void UpdateValue(double value);
    void UpdateValue(int mode, double value);
    void UpdateValue(string value);
    void UpdateRGBValue(int r, int g, int b);
    void ClearCache();
    void Clear();
    void ForceClear();
   
    void Activate(WidgetActionBroker currentWidgetActionBroker)
    {
        currentWidgetActionBroker_ = currentWidgetActionBroker;
    }
    
    void MakeCurrentDefault()
    {
        defaultWidgetActionBroker_ = currentWidgetActionBroker_;
    }
    
    void AddFeedbackProcessor(FeedbackProcessor* feedbackProcessor)
    {
        feedbackProcessors_.push_back(feedbackProcessor);
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
    bool mustForce_ = false;
    bool isSilent_ = false;
    bool shouldRefresh_ = false;
    double refreshInterval_ = 0.0;
    double lastRefreshed_ = 0.0;
    Widget* const widget_ = nullptr;
    
public:
    FeedbackProcessor(Widget* widget) : widget_(widget) {}
    virtual ~FeedbackProcessor() {}
    Widget* GetWidget() { return widget_; }
    void SetRefreshInterval(double refreshInterval) { shouldRefresh_ = true; refreshInterval_ = refreshInterval * 1000.0; }
    virtual void UpdateValue(double value) {}
    virtual void UpdateValue(int param, double value) {}
    virtual void UpdateValue(string value) {}
    virtual void UpdateRGBValue(int r, int g, int b) {}
    virtual void ForceValue(double value) {}
    virtual void ForceValue(int param, double value) {}
    virtual void ForceRGBValue(int r, int g, int b) {}
    virtual void ClearCache() {}

    virtual void ForceValue(string displayText)
    {
        mustForce_ = true;
        UpdateValue(displayText);
        mustForce_ = false;
    }
    
    virtual void SilentSetValue(string displayText)
    {
        isSilent_ = true;
        mustForce_ = true;
        UpdateValue(displayText);
        mustForce_ = false;
        isSilent_ = false;
    }
    
    virtual void Clear()
    {
        UpdateValue(0.0);
        UpdateValue(0, 0.0);
        UpdateValue("");
        UpdateRGBValue(0, 0, 0);
    }
    
    virtual void ForceClear()
    {
        ForceValue(0.0);
        ForceValue(0, 0.0);
        ForceValue("");
        ForceRGBValue(0, 0, 0);
    }
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
    
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget) : FeedbackProcessor(widget), surface_(surface) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : FeedbackProcessor(widget), surface_(surface), midiFeedbackMessage1_(feedback1) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1, MIDI_event_ex_t* feedback2) : FeedbackProcessor(widget), surface_(surface), midiFeedbackMessage1_(feedback1), midiFeedbackMessage2_(feedback2) {}
    
    void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(int first, int second, int third);
    void ForceMidiMessage(int first, int second, int third);

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
    
    OSC_FeedbackProcessor(OSC_ControlSurface* surface, Widget* widget, string oscAddress) : FeedbackProcessor(widget), surface_(surface), oscAddress_(oscAddress) {}
    ~OSC_FeedbackProcessor() {}
    
    virtual void UpdateValue(double value) override;
    virtual void UpdateValue(int param, double value) override;
    virtual void UpdateValue(string value) override;
    virtual void ForceValue(double value) override;
    virtual void ForceValue(int param, double value) override;
    virtual void ForceValue(string value) override;
    virtual void SilentSetValue(string value) override;

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
    
    EuCon_FeedbackProcessor(EuCon_ControlSurface* surface, Widget* widget, string address) : FeedbackProcessor(widget), surface_(surface), address_(address) {}
    ~EuCon_FeedbackProcessor() {}
    
    virtual void UpdateValue(double value) override;
    virtual void UpdateValue(int param, double value) override;
    virtual void UpdateValue(string value) override;
    virtual void ForceValue(double value) override;
    virtual void ForceValue(int param, double value) override;
    virtual void ForceValue(string value) override;
    virtual void SilentSetValue(string value) override;
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
        lastStringValue_ = "";
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuCon_FeedbackProcessorDB : public EuCon_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    EuCon_FeedbackProcessorDB(EuCon_ControlSurface* surface, Widget* widget, string address) : EuCon_FeedbackProcessor(surface, widget, address) {}
    ~EuCon_FeedbackProcessorDB() {}
    
    virtual void Clear() override;
    virtual void ForceClear() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_ = nullptr;
    int const numFXSlots_ = 0;
    bool shouldMapSelectedTrackFX_ = false;
    bool shouldMapSelectedTrackFXMenus_ = false;
    bool shouldMapFocusedFX_ = false;
    vector<Zone*> activeSelectedTrackFXZones_;
    vector<Zone*> activeSelectedTrackFXMenuZones_;
    vector<Zone*> activeSelectedTrackFXMenuFXZones_;
    vector<Zone*> activeFocusedFXZones_;
    
    bool shouldShowFXWindows_ = false;
    
public:
    FXActivationManager(ControlSurface* surface, int numFXSlots) : surface_(surface), numFXSlots_(numFXSlots) {}
    
    bool GetShouldMapSelectedTrackFXMenus() { return shouldMapSelectedTrackFXMenus_; }
    bool GetShouldMapSelectedTrackFX() { return shouldMapSelectedTrackFX_; }
    bool GetShouldMapFocusedFX() { return shouldMapFocusedFX_; }
    int  GetNumFXSlots() { return numFXSlots_; }
    bool GetShowFXWindows() { return shouldShowFXWindows_; }
    
    void SetShouldShowFXWindows(bool shouldShowFXWindows) { shouldShowFXWindows_ = shouldShowFXWindows; }
    void SetShouldMapSelectedTrackFX(bool shouldMapSelectedTrackFX) { shouldMapSelectedTrackFX_ = shouldMapSelectedTrackFX; }
    void SetShouldMapSelectedTrackFXMenus(bool shouldMapSelectedTrackFXMenus) { shouldMapSelectedTrackFXMenus_ = shouldMapSelectedTrackFXMenus; }
    void SetShouldMapFocusedFX(bool shouldMapFocusedFX) { shouldMapFocusedFX_ = shouldMapFocusedFX; }
    void ToggleMapSelectedTrackFX();
    void ToggleMapFocusedFX();
    void ToggleMapSelectedTrackFXMenu();
    void MapSelectedTrackFXToWidgets();
    void MapSelectedTrackFXToMenu();
    void MapSelectedTrackFXSlotToWidgets(MediaTrack* selectedTrack, int slot);
    void MapFocusedFXToWidgets();
    
    void ToggleShowFXWindows()
    {
        shouldShowFXWindows_ = ! shouldShowFXWindows_;
        
        if(shouldShowFXWindows_ == true)
            for(auto zone : activeSelectedTrackFXZones_)
                zone->OpenFXWindow();
        else
            for(auto zone : activeSelectedTrackFXZones_)
                zone->CloseFXWindow();
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
    ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int options);

    CSurfIntegrator* const CSurfIntegrator_ ;
    Page* const page_;
    string const name_;
    Zone* const defaultZone_ = nullptr;

    string zoneFolder_ = "";
    int numChannels_ = 0;
    int numSends_ = 0;
    int options_ = 0;

    map<int, Navigator*> navigators_;
    
    
    vector<Widget*> widgets_;
    map<string, Widget*> widgetsByName_;

    vector<Zone*> activeZones_;

    bool shouldMapSends_ = false;
    vector<Zone*> activeSendZones_;
    
    FXActivationManager* const fxActivationManager_;

    virtual void SurfaceOutMonitor(Widget* widget, string address, string value);

    void InitZones(string zoneFolder);
    map<string, vector<string>> zoneFileLines_;
    map<string, ZoneTemplate*> zoneTemplates_;
    
    virtual void InitHardwiredWidgets()
    {
        // Add the "hardwired" widgets
        AddWidget(new Widget(this, "OnTrackSelection"));
        AddWidget(new Widget(this, "OnFXFocus"));
    }
    
public:
    virtual ~ControlSurface() {};
    
    Page* GetPage() { return page_; }
    string GetName() { return name_; }
    
    virtual string GetSourceFileName() { return ""; }
    vector<Widget*> &GetWidgets() { return widgets_; }
    
    int GetNumChannels() { return numChannels_; }
    int GetNumSends() { return numSends_; }

    bool GetShouldMapSends() { return shouldMapSends_; }
    void SetShouldMapSends(bool shouldMapSends) { shouldMapSends_ = shouldMapSends;  }
    void ToggleMapSends();
    void MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones);
    
    Navigator* GetNavigatorForChannel(int channelNum);
    
    FXActivationManager* GetFXActivationManager() { return fxActivationManager_; }
    Zone* GetDefaultZone() { return defaultZone_; }
    
    
    virtual void LoadingZone(string zoneName) {}
    virtual void HandleExternalInput() {}
    virtual void InitializeEuCon() {}
    virtual void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems) {}
    virtual void ReceiveEuConMessage(string oscAddress, double value) {}
    virtual void ReceiveEuConMessage(string oscAddress, string value) {}
    virtual void UpdateTimeDisplay() {}
    virtual void ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible) {}
    virtual void HandleEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible) {}
    virtual void ReceiveEuConGetMeterValues(int id, int iLeg, float& oLevel, float& oPeak, bool& oLegClip) {}
    virtual void GetFormattedFXParamValue(const char* address, char *buffer, int bufferSize) {}
  
    virtual bool GetIsEuConFXAreaFocused() { return false; }

    virtual void ForceRefreshTimeDisplay() {}

    void MakeHomeDefault()
    {
        if(zoneTemplates_.count("Home") > 0)
        {
            vector<Zone*> dummyZones; //  We don't want to put "Home" in any active Zones list - it is the default Zone and has Control Surface lifetime
            
            zoneTemplates_["Home"]->Activate(this, &dummyZones);
        
            for(auto widget : widgets_)
                widget->MakeCurrentDefault();
        }
    }
    
    void GoZone(string zoneName)
    {
        if(zoneTemplates_.count(zoneName) > 0)
        {
            if(zoneName == "Home")
            {
                // GAW TDB, just deactivate all active Zones (including FXActivationManager) - "Home" is default
            }
            else
            {
                zoneTemplates_[zoneName]->Activate(this, &activeZones_);
            }
        }
    }

    ZoneTemplate* GetZoneTemplate(string zoneName)
    {
        if(zoneTemplates_.count(zoneName) > 0)
            return zoneTemplates_[zoneName];
        else
            return nullptr;
    }
   
    void AddZoneTemplate(ZoneTemplate* zoneTemplate)
    {
        zoneTemplates_[zoneTemplate->name] = zoneTemplate;
    }

    void AddZoneFileLine(string fileName, string line)
    {
        zoneFileLines_[fileName].push_back(line);
    }
    
    virtual void RequestUpdate()
    {
        for(auto widget : widgets_)
            widget->RequestUpdate();
    }

    virtual void ForceClearAllWidgets()
    {
        for(auto widget : widgets_)
            widget->ForceClear();
    }
    
    void ClearCache()
    {
        for(auto widget : widgets_)
            widget->ClearCache();
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
        widgetsByName_[widget->GetName()] = widget;
    }

    Widget* GetWidgetByName(string name)
    {
        if(widgetsByName_.count(name) > 0)
            return widgetsByName_[name];
        else
            return nullptr;
    }
    
    void OnTrackSelection()
    {
        if(widgetsByName_.count("OnTrackSelection") > 0)
            widgetsByName_["OnTrackSelection"]->DoAction(1.0);
    }
    
    void OnFXFocus(MediaTrack* track, int fxIndex)
    {
        if(widgetsByName_.count("OnFXFocus") > 0)
            widgetsByName_["OnFXFocus"]->DoAction(1.0);
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
   
    void InitWidgets(string templateFilename, string zoneFolder);

public:
    Midi_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int numChannels, int numSends, int numFX, int options, midi_Input* midiInput, midi_Output* midiOutput)
    : ControlSurface(CSurfIntegrator, page, name, zoneFolder, numChannels, numSends, numFX, options), templateFilename_(templateFilename), midiInput_(midiInput), midiOutput_(midiOutput)
    {
        InitWidgets(templateFilename, zoneFolder);
    }
    
    virtual ~Midi_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "/CSI/Surfaces/Midi/" + templateFilename_; }
    
    void SendMidiMessage(Midi_FeedbackProcessor* feedbackProcessor, MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(Midi_FeedbackProcessor* feedbackProcessor, int first, int second, int third);

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
    string templateFilename_ = "";
    oscpkt::UdpSocket* const inSocket_ = nullptr;
    oscpkt::UdpSocket* const outSocket_ = nullptr;
    oscpkt::PacketReader packetReader_;
    oscpkt::PacketWriter packetWriter_;
    map<string, OSC_CSIMessageGenerator*> CSIMessageGeneratorsByOSCMessage_;
    
    void InitWidgets(string templateFilename, string zoneFolder);
    void ProcessOSCMessage(string message, double value);

public:
    OSC_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int numChannels, int numSends, int numFX, int options, oscpkt::UdpSocket* inSocket, oscpkt::UdpSocket* outSocket)
    : ControlSurface(CSurfIntegrator, page, name, zoneFolder, numChannels, numSends, numFX, options), templateFilename_(templateFilename), inSocket_(inSocket), outSocket_(outSocket)
    {
        InitWidgets(templateFilename, zoneFolder);
    }
    
    virtual ~OSC_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "/CSI/Surfaces/OSC/" + templateFilename_; }
    
    virtual void LoadingZone(string zoneName) override;
    void SendOSCMessage(OSC_FeedbackProcessor* feedbackProcessor, string oscAddress, double value);
    void SendOSCMessage(OSC_FeedbackProcessor* feedbackProcessor, string oscAddress, string value);
    
    virtual void ForceClearAllWidgets() override
    {
        LoadingZone("Home");
        ControlSurface::ForceClearAllWidgets();
    }
    
    virtual void HandleExternalInput() override
    {
        if(inSocket_ != nullptr && inSocket_->isOk())
        {
            while (inSocket_->receiveNextPacket(0))  // timeout, in ms
            {
                packetReader_.init(inSocket_->packetData(), inSocket_->packetSize());
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// For EuCon_ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetGroup
{
private:
    bool isVisible_ = false;
    
    vector<Widget*> widgets_;
    map<string, WidgetGroup*> subGroups_;
    
public:
    void SetIsVisible(bool isVisible)
    {
        isVisible_ = isVisible;
    }
    
    void SetIsVisible(string subgroupName, bool isVisible)
    {
        if(subGroups_.count(subgroupName) > 0)
           subGroups_[subgroupName]->SetIsVisible(isVisible);
    }
    
    void RequestUpdate()
    {
        if(isVisible_)
        {
            for(auto widget : widgets_)
                widget->RequestUpdate();
            
            for(auto [name, group] : subGroups_)
                group->RequestUpdate();
        }
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
    }
    
    void AddWidgetToSubgroup(string subgroupName, Widget* widget)
    {
        if(subGroups_.count(subgroupName) < 1)
            subGroups_[subgroupName] = new WidgetGroup();
        
        subGroups_[subgroupName]->AddWidget(widget);
    }
};

typedef struct PeakInfoStruct
{
    double timePeakSet;
    float peakValue;
    bool isClipping;
} PeakInfo;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MarshalledFunctionCall;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuCon_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool isEuConFXAreaFocused_ = false;
    double previousPP = 0.0;
    
    map<string, EuCon_CSIMessageGenerator*> CSIMessageGeneratorsByMessage_;

    vector<Widget*> generalWidgets_;
    map<int, WidgetGroup*> channelGroups_;
    
    WDL_Mutex mutex_;
    list<MarshalledFunctionCall*> workQueue_;

    Widget* InitializeEuConWidget(CSIWidgetInfo &widgetInfo);
    
    map<int, PeakInfo> peakInfo_;
    
protected:
    virtual void InitHardwiredWidgets() override
    {
        ControlSurface::InitHardwiredWidgets();
        // Add the "hardwired" widgets
        AddWidget(new Widget(this, "OnEuConFXAreaGainedFocus"));
        AddWidget(new Widget(this, "OnEuConFXAreaLostFocus"));
    }
    
public:
    EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int options);
    virtual ~EuCon_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "EuCon"; }
    
    virtual bool GetIsEuConFXAreaFocused() override { return isEuConFXAreaFocused_; }

    virtual void InitializeEuCon() override;
    virtual void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems) override;
    void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, double value);
    void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, double value, int param);
    void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, string value);
    void SendEuConMessage(string address, string value);
    void HandleEuConMessage(string address, double value);
    void HandleEuConMessage(string address, string value);
    virtual void UpdateTimeDisplay() override;
    virtual void ReceiveEuConMessage(string address, double value) override;
    virtual void ReceiveEuConMessage(string address, string value) override;
    virtual void HandleExternalInput() override;
    virtual void ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible) override;
    virtual void HandleEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible) override;
    virtual void ReceiveEuConGetMeterValues(int id, int iLeg, float& oLevel, float& oPeak, bool& oLegClip) override;
    virtual void GetFormattedFXParamValue(const char* address, char *buffer, int bufferSize) override;

    virtual void RequestUpdate() override
    {
        for(auto widget : generalWidgets_)
            widget->RequestUpdate();
        
        for(auto [channel, group] : channelGroups_)
            group->RequestUpdate();
        
        SendEuConMessage("RequestUpdateMeters", "Update");
    }

    virtual void ForceRefreshTimeDisplay() override
    {
        previousPP = 0.5;
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
    bool vcaMode_ = false;
    int targetScrollLinkChannel_ = 0;
    int trackOffset_ = 0;
    int savedTrackOffset_ = 0;
    int savedVCAOffset_ = 0;
    vector<MediaTrack*> tracks_;
    vector<MediaTrack*> vcaSpillTracks_;
    vector<Navigator*> navigators_;
    Navigator* const masterTrackNavigator_ = nullptr;
    Navigator* const selectedTrackNavigator_ = nullptr;
    Navigator* const focusedFXNavigator_ = nullptr;
    Navigator* const defaultNavigator_ = nullptr;

public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages) : page_(page), followMCP_(followMCP), synchPages_(synchPages),
    masterTrackNavigator_(new MasterTrackNavigator(page_)), selectedTrackNavigator_(new SelectedTrackNavigator(page_)), focusedFXNavigator_(new FocusedFXNavigator(page_)), defaultNavigator_(new Navigator(page_)) {}
    
    Page* GetPage() { return page_; }
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    bool GetVCAMode() { return vcaMode_; }
    int  GetNumTracks() { return DAW::CSurf_NumTracks(followMCP_); }
    Navigator* GetMasterTrackNavigator() { return masterTrackNavigator_; }
    Navigator* GetSelectedTrackNavigator() { return selectedTrackNavigator_; }
    Navigator* GetFocusedFXNavigator() { return focusedFXNavigator_; }
    Navigator* GetDefaultNavigator() { return defaultNavigator_; }

    void SetScrollLink(bool scrollLink) { scrollLink_ = scrollLink; }
    
    Navigator* AddNavigator();
    void ForceScrollLink();
    void OnTrackSelectionBySurface(MediaTrack* track);
    void AdjustTrackBank(int amount);

    void IncChannelBias(MediaTrack* track, int channelNum)
    {
        for(int i = channelNum + 1; i < navigators_.size(); i++)
            navigators_[i]->IncBias();
    }
    
    void DecChannelBias(MediaTrack* track, int channelNum)
    {
        for(int i = channelNum + 1; i < navigators_.size(); i++)
            navigators_[i]->DecBias();
    }
    
    void TogglePin(MediaTrack* track)
    {
        for(auto navigator : navigators_)
        {
            if(track == navigator->GetTrack())
            {
                if(navigator->GetIsChannelPinned())
                    navigator->UnpinChannel();
                else
                    navigator->PinChannel();
                
                break;
            }
        }
    }
    
    void ToggleVCAMode()
    {
        if(vcaMode_)
        {
            savedVCAOffset_ = trackOffset_;
            trackOffset_ = savedTrackOffset_;
            vcaMode_ = false;
        }
        else
        {
            savedTrackOffset_ = trackOffset_;
            trackOffset_ = savedVCAOffset_;
            vcaMode_ = true;
        }
    }

    MediaTrack* GetTrackFromChannel(int channelNumber)
    {
        int trackNumber = channelNumber + trackOffset_;
        
        if(tracks_.size() > trackNumber && DAW::ValidateTrackPtr(tracks_[trackNumber]))
            return tracks_[trackNumber];
        else
            return nullptr;
    }
    
    MediaTrack* GetTrackFromId(int trackNumber)
    {
        if(trackNumber <= GetNumTracks())
            return DAW::CSurf_TrackFromID(trackNumber, followMCP_);
        else
            return nullptr;
    }

    void OnTrackSelection()
    {
        if(scrollLink_)
            ForceScrollLink();
    }
    
    void OnTrackListChange()
    {
        if(scrollLink_)
            ForceScrollLink();
    }

    void ToggleVCASpill(MediaTrack* track)
    {
        if(find(vcaSpillTracks_.begin(), vcaSpillTracks_.end(), track) == vcaSpillTracks_.end())
            vcaSpillTracks_.push_back(track);
        else
            vcaSpillTracks_.erase(find(vcaSpillTracks_.begin(), vcaSpillTracks_.end(), track));
    }
    
    // For vcaSpillTracks_.erase -- see Clean up vcaSpillTracks below
    static bool IsTrackPointerStale(MediaTrack* track)
    {
        return ! DAW::ValidateTrackPtr(track);
    }
    
    void RebuildTrackList()
    {
        int top = GetNumTracks() - navigators_.size();
        
        if(top < 0)
            trackOffset_ = 0;
        else if(trackOffset_ >  top)
            trackOffset_ = top;

        tracks_.clear();
        
        // Clean up vcaSpillTracks
        vcaSpillTracks_.erase(remove_if(vcaSpillTracks_.begin(), vcaSpillTracks_.end(), IsTrackPointerStale), vcaSpillTracks_.end());

        // Get Visible Tracks
        for (int i = 1; i <= GetNumTracks(); i++)
        {
            MediaTrack* track = DAW::CSurf_TrackFromID(i, followMCP_);
            
            if(DAW::IsTrackVisible(track, followMCP_))
            {
                if(vcaMode_)
                {
                    int vcaMasterGroup = DAW::GetSetTrackGroupMembership(track, "VOLUME_VCA_MASTER", 0, 0);

                    if(vcaMasterGroup != 0 && DAW::GetSetTrackGroupMembership(track, "VOLUME_VCA_SLAVE", 0, 0) == 0) // Only top level Masters for now
                    {
                        tracks_.push_back(track);
                        
                        if(find(vcaSpillTracks_.begin(), vcaSpillTracks_.end(), track) != vcaSpillTracks_.end()) // should spill slaves for this master
                            for (int j = 1; j <= GetNumTracks(); j++)
                                if(vcaMasterGroup == DAW::GetSetTrackGroupMembership(DAW::CSurf_TrackFromID(j, followMCP_), "VOLUME_VCA_SLAVE", 0, 0)) // if this track is slave of master
                                    tracks_.push_back(DAW::CSurf_TrackFromID(j, followMCP_));
                    }
                }
                else
                    tracks_.push_back(track);
            }
        }

        for(auto navigator : navigators_)
        {
            if(navigator->GetIsChannelPinned())
            {
                if(DAW::ValidateTrackPtr(navigator->GetTrack()))
                    remove(tracks_.begin(), tracks_.end(), navigator->GetTrack());
                else
                    navigator->UnpinChannel();
            }
        }
    }
    
    bool GetIsControlTouched(MediaTrack* track, int touchedControl)
    {
        if(track == masterTrackNavigator_->GetTrack())
        {
            if(touchedControl == 0)
                return masterTrackNavigator_->GetIsFaderTouched();
            else
                return masterTrackNavigator_->GetIsRotaryTouched();
        }
        
        for(auto navigator : navigators_)
        {
            if(track == navigator->GetTrack())
            {
                if(touchedControl == 0)
                    return navigator->GetIsFaderTouched();
                else
                    return navigator->GetIsRotaryTouched();
            }
        }

        return false;
    }
    
    void ClearTouchStates()
    {
        for(auto navigator : navigators_)
        {
            navigator->SetIsFaderTouched(false);
            navigator->SetIsRotaryTouched(false);
        }
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
        
        for(int i = 0; i <= GetNumTracks(); i++)
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
    
    Navigator* defaultNavigator_ = new Navigator(this);
    
public:
    Page(string name, rgb_color colour, bool followMCP, bool synchPages) : name_(name), colour_(colour), trackNavigationManager_(new TrackNavigationManager(this, followMCP, synchPages)) { }
    
    string GetName() { return name_; }
    TrackNavigationManager* GetTrackNavigationManager() { return trackNavigationManager_; }
    vector<ControlSurface*> &GetSurfaces() { return surfaces_; }
    
    void OpenEditModeWindow();
    void ToggleEditMode();
    void InputReceived(Widget* widget, double value);
    void ActionPerformed(Action* action);
    void UpdateEditModeWindow();
    
    bool GetShift() { return isShift_; }
    bool GetOption() { return isOption_; }
    bool GetControl() { return isControl_; }
    bool GetAlt() { return isAlt_; }

    Navigator* GetDefaultNavigator() { return defaultNavigator_; }
    
    void InitializeEuCon()
    {
        for(auto surface : surfaces_)
            surface->InitializeEuCon();
    }
    
    void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems)
    {
        for(auto surface : surfaces_)
            surface->InitializeEuConWidgets(widgetInfoItems);
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
    
    void ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible)
    {
        for(auto surface : surfaces_)
            surface->ReceiveEuConGroupVisibilityChange(groupName, channelNumber, isVisible);
    }
    
    void ReceiveEuConGetMeterValues(int id, int iLeg, float& oLevel, float& oPeak, bool& oLegClip)
    {
        for(auto surface : surfaces_)
            surface->ReceiveEuConGetMeterValues(id, iLeg, oLevel, oPeak, oLegClip);
    }

    void GetFormattedFXParamValue(const char* address, char *buffer, int bufferSize)
    {
        for(auto surface : surfaces_)
            surface->GetFormattedFXParamValue(address, buffer, bufferSize);
    }
    
   /*
    int repeats = 0;
    
    void Run()
    {
        int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        trackNavigationManager_->RebuildTrackList();
        
        for(auto surface : surfaces_)
            surface->HandleExternalInput();
        
        for(auto surface : surfaces_)
            surface->RequestUpdate();
        
        UpdateEditModeWindow();

         repeats++;
         
         if(repeats > 50)
         {
             repeats = 0;
             
             int totalDuration = 0;

             start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
             trackNavigationManager_->RebuildTrackList();
             int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
             totalDuration += duration;
             ShowDuration("Rebuild Track List", duration);
             
             for(auto surface : surfaces_)
             {
                 start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                 surface->HandleExternalInput();
                 duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
                 totalDuration += duration;
                 ShowDuration(surface->GetName(), "HandleExternalInput", duration);
             }
             
             for(auto surface : surfaces_)
             {
                 start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                 surface->RequestUpdate();
                 duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
                 totalDuration += duration;
                 ShowDuration(surface->GetName(), "Request Update", duration);
             }
             
             start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
             UpdateEditModeWindow();
             duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
             totalDuration += duration;
             ShowDuration("Update Edit Mode Window", duration);
             
             char msgBuffer[250];
             
             sprintf(msgBuffer, "Total duration = %d\n\n\n", totalDuration);
             DAW::ShowConsoleMsg(msgBuffer);
         }
    }
    
    
    void ShowDuration(string item, int duration)
    {
        char msgBuffer[250];
        
        sprintf(msgBuffer, "%s - %d microseconds\n", item.c_str(), duration);
        DAW::ShowConsoleMsg(msgBuffer);
    }
    
    void ShowDuration(string surface, string item, int duration)
    {
        char msgBuffer[250];
        
        sprintf(msgBuffer, "%s - %s - %d microseconds\n", surface.c_str(), item.c_str(), duration);
        DAW::ShowConsoleMsg(msgBuffer);
    }
    
*/


    void Run()
    {
        trackNavigationManager_->RebuildTrackList();
        
        for(auto surface : surfaces_)
            surface->HandleExternalInput();
        
        for(auto surface : surfaces_)
            surface->RequestUpdate();
        
        UpdateEditModeWindow();
    }

    void ForceClearAllWidgets()
    {
        for(auto surface : surfaces_)
            surface->ForceClearAllWidgets();
    }
    
    void ForceRefreshTimeDisplay()
    {
        for(auto surface : surfaces_)
            surface->ForceRefreshTimeDisplay();
    }

    void AddSurface(ControlSurface* surface)
    {
        surfaces_.push_back(surface);
    }
    
    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        return trackNavigationManager_->GetIsControlTouched(track, touchedControl);
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

    string GetModifier()
    {
        string modifier = "";
        
        if(isShift_)
            modifier += Shift + "+";
        if(isOption_)
            modifier += Option + "+";
        if(isControl_)
            modifier +=  Control + "+";
        if(isAlt_)
            modifier += Alt + "+";
        
        return modifier;
    }
    
    void OnTrackSelection()
    {
        trackNavigationManager_->OnTrackSelection();
        
        for(auto surface : surfaces_)
            surface->OnTrackSelection();
    }
    
    void OnTrackListChange()
    {
        trackNavigationManager_->OnTrackListChange();
    }
    
    void OnTrackSelectionBySurface(MediaTrack* track)
    {
        trackNavigationManager_->OnTrackSelectionBySurface(track);
        
        for(auto surface : surfaces_)
            surface->OnTrackSelection();
    }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        for(auto surface : surfaces_)
            surface->OnFXFocus(track, fxIndex);
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

    map<string, Action*> actions_;

    vector <Page*> pages_;
    
    map<string, map<string, int>> fxParamIndices_;
    
    int currentPageIndex_ = 0;
    bool surfaceInMonitor_ = false;
    bool surfaceOutMonitor_ = false;
    bool fxMonitor_ = false;
    
    bool shouldRun_ = true;
    
    int *timeModePtr_ = nullptr;
    int *timeMode2Ptr_ = nullptr;
    int *measOffsPtr_ = nullptr;
    double *timeOffsPtr_ = nullptr;
    
    void InitActionsDictionary();

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
        InitActionsDictionary();

        int size = 0;
        int index = projectconfig_var_getoffs("projtimemode", &size);
        timeModePtr_ = (int *)projectconfig_var_addr(nullptr, index);
        
        index = projectconfig_var_getoffs("projtimemode2", &size);
        timeMode2Ptr_ = (int *)projectconfig_var_addr(nullptr, index);
        
        index = projectconfig_var_getoffs("projmeasoffs", &size);
        measOffsPtr_ = (int *)projectconfig_var_addr(nullptr, index);
        
        index = projectconfig_var_getoffs("projtimeoffs", &size);
        timeOffsPtr_ = (double *)projectconfig_var_addr(nullptr, index);
    }
    
    void Shutdown()
    {
        fxMonitor_ = false;
        surfaceInMonitor_ = false;
        surfaceOutMonitor_ = false;
       
        // GAW -- IMPORTANT
        // We want to stop polling and zero out all Widgets before shutting down
        shouldRun_ = false;
        
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ForceClearAllWidgets();
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
    
    int *GetTimeModePtr() { return timeModePtr_; }
    int *GetTimeMode2Ptr() { return timeMode2Ptr_; }
    int *GetMeasOffsPtr() { return measOffsPtr_; }
    double *GetTimeOffsPtr() { return timeOffsPtr_; }
   
    ActionContext GetActionContext(string actionName, Widget* widget, Zone* zone, vector<string> params)
    {
        if(actions_.count(actionName) > 0)
            return ActionContext(actions_[actionName], widget, zone, params);
        else
            return ActionContext(actions_["NoAction"], widget, zone, params);
    }

    void OnTrackSelection(MediaTrack *track)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnTrackSelection();
    }
    
    void OnTrackListChange()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnTrackListChange();
    }
    
    void OnFXFocus(MediaTrack *track, int fxIndex)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnFXFocus(track, fxIndex);
    }
    
    void NextTimeDisplayMode()
    {
        int *tmodeptr = GetTimeMode2Ptr();
        if (tmodeptr && *tmodeptr>=0)
        {
            (*tmodeptr)++;
            if ((*tmodeptr)>5)
                (*tmodeptr)=0;
        }
        else
        {
            tmodeptr = GetTimeModePtr();
            
            if (tmodeptr)
            {
                (*tmodeptr)++;
                if ((*tmodeptr)>5)
                    (*tmodeptr)=0;
            }
        }

        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ForceRefreshTimeDisplay();
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
    
    void GoToPage(string pageName)
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
            pages_[currentPageIndex_]->OpenEditModeWindow();
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
                DAW::ShowConsoleMsg(("Zone \"" + string(fxName) + "\"").c_str());
                
                DAW::ShowConsoleMsg("\n\n\tSelectedTrackNavigator\n");

                for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                {
                    DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                    
                    
                    
                    DAW::ShowConsoleMsg(("\n\tFXParam " + to_string(j) + " \"" + string(fxParamName)+ "\"").c_str());
                    
                    /* Uncomment this and comment the line above if you want to show step sizes
                    double stepOut = 0;
                    double smallstepOut = 0;
                    double largestepOut = 0;
                    bool istoggleOut = false;
                    TrackFX_GetParameterStepSizes(track, i, j, &stepOut, &smallstepOut, &largestepOut, &istoggleOut);

                    DAW::ShowConsoleMsg(("\n\n" + to_string(j) + " - \"" + string(fxParamName) + "\"\t\t\t\t Step = " +  to_string(stepOut) + " Small Step = " + to_string(smallstepOut)  + " LargeStep = " + to_string(largestepOut)  + " Toggle Out = " + (istoggleOut == 0 ? "false" : "true")).c_str());
                    */
                }
                
                DAW::ShowConsoleMsg("\nZoneEnd\n\n");
            }
        }
    }

    void InitializeEuCon()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->InitializeEuCon();
    }
    
    void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->InitializeEuConWidgets(widgetInfoItems);
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
    
    void ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ReceiveEuConGroupVisibilityChange(groupName, channelNumber, isVisible);
    }
    
    void ReceiveEuConGetMeterValues(int id, int iLeg, float& oLevel, float& oPeak, bool& oLegClip)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ReceiveEuConGetMeterValues(id, iLeg, oLevel, oPeak, oLegClip);
    }

    void GetFormattedFXParamValue(const char* address, char *buffer, int bufferSize)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->GetFormattedFXParamValue(address, buffer, bufferSize);
    }
    
    //int repeats = 0;
    
    void Run()
    {
        //int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        if(shouldRun_ && pages_.size() > 0)
            pages_[currentPageIndex_]->Run();
        /*
         repeats++;
         
         if(repeats > 50)
         {
         repeats = 0;
         
         int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;
         
         char msgBuffer[250];
         
         sprintf(msgBuffer, "%d microseconds\n", duration);
         DAW::ShowConsoleMsg(msgBuffer);
         }
        */
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
