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

extern string GetLineEnding();

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
const string FaderTouch = "FaderTouch";

const string BadFileChars = "[ \\:*?<>|.,()/]";
const string CRLFChars = "[\r\n]";
const string TabChars = "[\t]";

const int TempDisplayTime = 1250;

enum NavigationStyle
{
    Standard,
    Send,
    Receive,
    FXMenu,
};

class Manager;
extern Manager* TheManager;

static vector<string> GetTokens(string line)
{
    vector<string> tokens;
    
    istringstream iss(line);
    string token;
    while (iss >> quoted(token))
        tokens.push_back(token);
    
    return tokens;
}

struct CSIWidgetInfo
{
    std::string group = "General";
    int channelNumber = 0;
    int sendNumber = 0;
    bool isVisible = true;
    std::string name = "";
    std::string control = "";
    std::string FB_Processor = "";
    std::string touch = "";
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor) : CSIWidgetInfo(aName, aControl, aFB_Processor, "General", 0, 0, true) {}
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor, std::string aGroup, int aChannelNumber, bool isVisible) :  CSIWidgetInfo(aName, aControl, aFB_Processor, aGroup, aChannelNumber, 0, isVisible) {}
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor, std::string aTouchName, std::string aGroup, int aChannelNumber, bool isVisible) :  CSIWidgetInfo(aName, aControl, aFB_Processor, aTouchName, aGroup, aChannelNumber, 0, isVisible) {}
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor, std::string aGroup, int aChannelNumber, int aSendNumber, bool itemIsVisible) :  name(aName), control(aControl), FB_Processor(aFB_Processor), group(aGroup), channelNumber(aChannelNumber), sendNumber(aSendNumber), isVisible(itemIsVisible) {}
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor, std::string aTouchName, std::string aGroup, int aChannelNumber, int aSendNumber, bool itemIsVisible) :  name(aName), control(aControl), FB_Processor(aFB_Processor), touch(aTouchName), group(aGroup), channelNumber(aChannelNumber), sendNumber(aSendNumber), isVisible(itemIsVisible) {}
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
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Action() {}
    
    virtual void Touch(ActionContext* context, double value);
    virtual string GetName() { return "Action"; }
    virtual void RequestUpdate(ActionContext* context) {}
    virtual void Do(ActionContext* context, double value) {}
    virtual double GetCurrentNormalizedValue(ActionContext* context) { return 0.0; }
    virtual double GetCurrentDBValue(ActionContext* context) { return 0.0; }

    int GetPanMode(MediaTrack* track)
    {
        double pan1, pan2 = 0.0;
        int panMode = 0;
        DAW::GetTrackUIPan(track, &pan1, &pan2, &panMode);
        return panMode;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* const page_ = nullptr;
    bool isNavigatorTouched_ = false;
    bool isVolumeTouched_ = false;
    bool isPanTouched_ = false;
    bool isPanWidthTouched_ = false;
    bool isPanLeftTouched_ = false;
    bool isPanRightTouched_ = false;

public:
    Navigator(Page*  page) : page_(page) {}
    virtual ~Navigator() {}
    
    void SetIsNavigatorTouched(bool isNavigatorTouched) { isNavigatorTouched_ = isNavigatorTouched;  }
    bool GetIsNavigatorTouched() { return isNavigatorTouched_;  }

    void SetIsVolumeTouched(bool isVolumeTouched) { isVolumeTouched_ = isVolumeTouched;  }
    bool GetIsVolumeTouched() { return isVolumeTouched_;  }
    
    void SetIsPanTouched(bool isPanTouched) { isPanTouched_ = isPanTouched; }
    bool GetIsPanTouched() { return isPanTouched_;  }
    
    void SetIsPanWidthTouched(bool isPanWidthTouched) { isPanWidthTouched_ = isPanWidthTouched; }
    bool GetIsPanWidthTouched() { return isPanWidthTouched_;  }
    
    void SetIsPanLeftTouched(bool isPanLeftTouched) { isPanLeftTouched_ = isPanLeftTouched; }
    bool GetIsPanLeftTouched() { return isPanLeftTouched_;  }
    
    void SetIsPanRightTouched(bool isPanRightTouched) { isPanRightTouched_ = isPanRightTouched; }
    bool GetIsPanRightTouched() { return isPanRightTouched_;  }
    
    virtual string GetName() { return "Navigator"; }
    virtual MediaTrack* GetTrack() { return nullptr; }
    virtual string GetChannelNumString() { return ""; }
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
    
    virtual string GetChannelNumString() override { return to_string(channelNum_ + 1); }
    
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
class ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* const action_ = nullptr;
    Widget* const widget_ = nullptr;
    Zone* const zone_ = nullptr;
    
    Widget* associatedWidget_ = nullptr;
    
    string lastStringValue_ = "";
    
    int intParam_ = 0;
    
    string stringParam_ = "";
    
    int paramIndex_ = 0;
    
    string fxParamDisplayName_ = "";
    
    int commandId_ = 0;
    
    double rangeMinimum_ = 0.0;
    double rangeMaximum_ = 1.0;
    
    vector<double> steppedValues_;
    int steppedValuesIndex_ = 0;
    
    double deltaValue_ = 0.0;
    vector<double> acceleratedDeltaValues_;
    vector<int> acceleratedTickValues_;
    int accumulatedIncTicks_ = 0;
    int accumulatedDecTicks_ = 0;
    
    bool isFeedbackInverted_ = false;
    double holdDelayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    double deferredValue_ = 0.0;
    
    bool shouldUseDisplayStyle_ = false;
    int displayStyle_ = 0;
    
    bool supportsRGB_ = false;
    vector<rgb_color> RGBValues_;
    int currentRGBIndex_ = 0;
    
    bool supportsTrackColor_ = false;
    
    vector<string> autoModeDisplayNames__ = { "Trim", "Read", "Touch", "Write", "Latch", "LtchPre" };
    int autoModeIndex_ = 0;
    
    vector<vector<string>> properties_;
    
public:
    ActionContext(Action* action, Widget* widget, Zone* zone, vector<string> params, vector<vector<string>> properties);
    virtual ~ActionContext() {}
    
    Widget* GetWidget() { return widget_; }
    Zone* GetZone() { return zone_; }
    int GetSlotIndex();
    string GetName();

    void SetAssociatedWidget(Widget* widget) { associatedWidget_ = widget; }
    Widget* GetAssociatedWidget() { return associatedWidget_; }

    int GetIntParam() { return intParam_; }
    string GetStringParam() { return stringParam_; }
    int GetCommandId() { return commandId_; }
    bool GetShouldUseDisplayStyle() { return shouldUseDisplayStyle_; }
    int GetDisplayStyle() { return displayStyle_; }
    
    MediaTrack* GetTrack();
    
    void DoRangeBoundAction(double value);
    void DoSteppedValueAction(double value);
    void DoAcceleratedSteppedValueAction(int accelerationIndex, double value);
    void DoAcceleratedDeltaValueAction(int accelerationIndex, double value);
    
    Page* GetPage();
    ControlSurface* GetSurface();
    TrackNavigationManager* GetTrackNavigationManager();
    int GetParamIndex() { return paramIndex_; }
    
    bool GetSupportsRGB() { return supportsRGB_; }
    
    void SetIsFeedbackInverted() { isFeedbackInverted_ = true; }
    void SetHoldDelayAmount(double holdDelayAmount) { holdDelayAmount_ = holdDelayAmount * 1000.0; } // holdDelayAmount is specified in seconds, holdDelayAmount_ is in milliseconds
    
    void DoAction(double value);
    void DoRelativeAction(double value);
    void DoRelativeAction(int accelerationIndex, double value);
    
    void RequestUpdate();
    void RunDeferredActions();
    void ClearWidget();
    void UpdateWidgetValue(double value);
    void UpdateWidgetValue(int param, double value);
    void UpdateWidgetValue(string value);
    void ForceWidgetValue(double value);

    void SetAutoModeIndex()
    {
        if(MediaTrack* track = GetTrack())
            autoModeIndex_ = DAW::GetMediaTrackInfo_Value(track, "I_AUTOMODE");
    }
    
    void NextAutoMode()
    {
        if(MediaTrack* track = GetTrack())
        {
            if(autoModeIndex_ == 2) // skip over write mode when cycling
                autoModeIndex_ += 2;
            else
                autoModeIndex_++;
            
            if(autoModeIndex_ > autoModeDisplayNames__.size() - 1)
                autoModeIndex_ = 0;
    
            DAW::GetSetMediaTrackInfo(track, "I_AUTOMODE", &autoModeIndex_);
        }
    }
    
    string GetAutoModeDisplayName()
    {
        int globalOverride = DAW::GetGlobalAutomationOverride();

        if(globalOverride > -1) // -1=no override, 0=trim/read, 1=read, 2=touch, 3=write, 4=latch, 5=bypass
            return autoModeDisplayNames__[globalOverride];
        else
            return autoModeDisplayNames__[autoModeIndex_];
    }
    
    void DoTouch(double value)
    {
        action_->Touch(this, value);
    }
    
    string GetFxParamDisplayName()
    {
        if(fxParamDisplayName_ != "")
            return fxParamDisplayName_;
        else if(MediaTrack* track = GetTrack())
        {
            char fxParamName[BUFSZ];
            DAW::TrackFX_GetParamName(track, GetSlotIndex(), paramIndex_, fxParamName, sizeof(fxParamName));
            return fxParamName;
        }
        
        return "";
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

    string GetPanValueString(double panVal)
    {
        bool left = false;
        
        if(panVal < 0)
        {
            left = true;
            panVal = -panVal;
        }
        
        int panIntVal = int(panVal * 100.0);
        string trackPanValueString = "";
        
        if(left)
        {
            if(panIntVal == 100)
                trackPanValueString += "<";
            else if(panIntVal < 100 && panIntVal > 9)
                trackPanValueString += "< ";
            else
                trackPanValueString += "<  ";
            
            trackPanValueString += to_string(panIntVal);
        }
        else
        {
            trackPanValueString += "   ";
            
            trackPanValueString += to_string(panIntVal);
            
            if(panIntVal == 100)
                trackPanValueString += ">";
            else if(panIntVal < 100 && panIntVal > 9)
                trackPanValueString += " >";
            else
                trackPanValueString += "  >";
        }
        
        if(panIntVal == 0)
            trackPanValueString = "  <C>  ";

        return trackPanValueString;
    }
    
    string GetPanWidthValueString(double widthVal)
    {
        bool reversed = false;
        
        if(widthVal < 0)
        {
            reversed = true;
            widthVal = -widthVal;
        }
        
        int widthIntVal = int(widthVal * 100.0);
        string trackPanWidthString = "";
        
        if(reversed)
            trackPanWidthString += "Rev ";
        
        trackPanWidthString += to_string(widthIntVal);
        
        if(widthIntVal == 0)
            trackPanWidthString = " <Mno> ";

        return trackPanWidthString;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_ = nullptr;
    Navigator* navigator_= nullptr;
    string const name_ = "";
    string const alias_ = "";
    string const sourceFilePath_ = "";
    
    NavigationStyle navigationStyle_ = Standard;
    
    int slotIndex_ = 0;

    vector<Widget*> widgets_;
    
    vector<Zone*> includedZones_;
    vector<Zone*> subZones_;

    map<Widget*, map<string, vector<ActionContext>>> actionContextDictionary_;
    vector<ActionContext> defaultContexts_;
    
public:   
    Zone(ControlSurface* surface, Navigator* navigator, int slotIndex, string name, string alias, string sourceFilePath): surface_(surface), navigator_(navigator), slotIndex_(slotIndex), name_(name), alias_(alias), sourceFilePath_(sourceFilePath) {}
    Zone() {}
    
    void Activate();
    void Activate(vector<Zone*> &activeZones);
    bool TryActivate(Widget* widget);
    void Deactivate(vector<Zone*> activeZones);
    int GetSlotIndex();
    vector<ActionContext> &GetActionContexts(Widget* widget);
    
    
    Navigator* GetNavigator() { return navigator_; }
    void SetNavigator(Navigator* navigator) { navigator_ = navigator; }
    void SetNavigationStyle(NavigationStyle navigationStyle) { navigationStyle_ = navigationStyle; }
    vector<Widget*> &GetWidgets() { return widgets_; }
    vector<Zone*> &GetIncludedZones() { return includedZones_; }
    vector<Zone*> &GetSubZones() { return subZones_; }
    void AddIncludedZone(Zone* &zone) { includedZones_.push_back(zone); }
    
    void SetSlotIndex(int index)
    {
        slotIndex_ = index;
        
        for(auto subZone : subZones_)
            subZone->SetSlotIndex(index);
    }

    void AddSubZone(Zone* &subZone)
    {
        subZone->SetNavigator(GetNavigator());
        subZones_.push_back(subZone);
    }

    string GetName()
    {
        return name_;
    }
    
    string GetNameOrAlias()
    {
        if(alias_ != "")
            return alias_;
        else
            return name_;
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
    }
    
    void AddActionContext(Widget* widget, string modifier, ActionContext actionContext)
    {
        actionContextDictionary_[widget][modifier].push_back(actionContext);
    }
    
    void RequestUpdate(vector<Widget*> &usedWidgets)
    {
        for(auto widget : widgets_)
        {
            if(find(usedWidgets.begin(), usedWidgets.end(), widget) == usedWidgets.end())
            {
                usedWidgets.push_back(widget);

                RequestUpdateWidget(widget);
            }
        }
        
        for(auto zone : includedZones_)
            zone->RequestUpdate(usedWidgets);
    }
    
    void RequestUpdateWidget(Widget* widget)
    {
        for(auto &context : GetActionContexts(widget))
            context.RunDeferredActions();
        
        if(GetActionContexts(widget).size() > 0)
        {
            ActionContext& context = GetActionContexts(widget)[0];
            context.RequestUpdate();
        }
    }
    
    void DoAction(Widget* widget, double value)
    {
        for(auto &context : GetActionContexts(widget))
            context.DoAction(value);
    }
    
    void DoTouch(Widget* widget, double value)
    {
        for(auto &context : GetActionContexts(widget))
            context.DoTouch(value);
    }
    
    void DoRelativeAction(Widget* widget, double delta)
    {
        for(auto &context : GetActionContexts(widget))
            context.DoRelativeAction(delta);
    }
    
    void DoRelativeAction(Widget* widget, int accelerationIndex, double delta)
    {
        for(auto &context : GetActionContexts(widget))
            context.DoRelativeAction(accelerationIndex, delta);
    }
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
    bool isToggled_ = false;
    
    Zone* currentZone_ = nullptr;
    
    void LogInput(double value);
    
public:
    Widget(ControlSurface* surface, string name);
    ~Widget();
    
    ControlSurface* GetSurface() { return surface_; }
    string GetName() { return name_; }
    bool GetIsModifier() { return isModifier_; }
    void SetIsModifier() { isModifier_ = true; }
    void SilentSetValue(string displayText);
    
    void Toggle() { isToggled_ = ! isToggled_; }
    bool GetIsToggled() { return isToggled_; }
    
    void SetProperties(vector<vector<string>> properties);
    void UpdateValue(double value);
    void UpdateValue(int mode, double value);
    void UpdateValue(string value);
    void UpdateRGBValue(int r, int g, int b);
    void ForceValue(double value);
    void ForceValue(int mode, double value);
    void ForceValue(string value);
    void ForceRGBValue(int r, int g, int b);
    void ClearCache();
    void Clear();
    void ForceClear();

    Zone* GetCurrentZone() { return currentZone_;  }
    void SetZone(Zone* zone) { currentZone_ = zone; }
    
    void GetFormattedFXParamValue(char *buffer, int bufferSize)
    {
        //currentWidgetContext_.GetFormattedFXParamValue(buffer, bufferSize);
    }

    void RequestUpdate()
    {
        vector<Widget*> usedWidgets;
        
        if(currentZone_ != nullptr)
            currentZone_->RequestUpdate(usedWidgets);
    }
 
    void DoAction(double value)
    {
        LogInput(value);
        
        if(currentZone_ != nullptr)
            currentZone_->DoAction(this, value);
    }
    
    void DoRelativeAction(double delta)
    {
        LogInput(delta);
        
        if(currentZone_ != nullptr)
            currentZone_->DoRelativeAction(this, delta);
    }
    
    void DoRelativeAction(int accelerationIndex, double delta)
    {
        LogInput(accelerationIndex);
        
        if(currentZone_ != nullptr)
            currentZone_->DoRelativeAction(this, accelerationIndex, delta);
    }
   
    void DoTouch(double value)
    {
        LogInput(value);
        
        if(currentZone_ != nullptr)
            currentZone_->DoTouch(this, value);
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
    
public:
    CSIMessageGenerator(ControlSurface* surface, Widget* widget, string message);
    virtual ~CSIMessageGenerator() {}
    
    virtual void ProcessMessage(double value)
    {
        widget_->DoAction(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Touch_CSIMessageGenerator : CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Touch_CSIMessageGenerator(ControlSurface* surface, Widget* widget, string message) : CSIMessageGenerator(surface, widget, message) {}
    virtual ~Touch_CSIMessageGenerator() {}
    
    virtual void ProcessMessage(double value) override
    {
        widget_->DoTouch(value);
    }
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
class FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    double lastDoubleValue_ = 0.0;
    string lastStringValue_ = "";

    Widget* const widget_ = nullptr;
    
public:
    FeedbackProcessor(Widget* widget) : widget_(widget) {}
    virtual ~FeedbackProcessor() {}
    Widget* GetWidget() { return widget_; }
    virtual void SetRGBValue(int r, int g, int b) {}
    virtual void ForceValue() {}
    virtual void ForceValue(double value) {}
    virtual void ForceValue(int param, double value) {}
    virtual void ForceRGBValue(int r, int g, int b) {}
    virtual void ForceValue(string value) {}
    virtual void SetColors(rgb_color textColor, rgb_color textBackground) {}
    virtual void SetCurrentColor(double value) {}
    virtual void SetProperties(vector<vector<string>> properties) {}
    
    virtual int GetMaxCharacters() { return 0; }
    
    virtual void SetValue(double value)
    {
        if(lastDoubleValue_ != value)
            ForceValue(value);
    }
    
    virtual void SetValue(int param, double value)
    {
        if(lastDoubleValue_ != value)
            ForceValue(value);
    }
    
    virtual void SetValue(string value)
    {
        if(lastStringValue_ != value)
            ForceValue(value);
    }

    virtual void ClearCache()
    {
        lastDoubleValue_ = 0.0;
        lastStringValue_ = "";
    }
    
    virtual void Clear()
    {
        SetValue(0.0);
        SetValue(0, 0.0);
        SetValue("");
        SetRGBValue(0, 0, 0);
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
    
public:
    
    OSC_FeedbackProcessor(OSC_ControlSurface* surface, Widget* widget, string oscAddress) : FeedbackProcessor(widget), surface_(surface), oscAddress_(oscAddress) {}
    ~OSC_FeedbackProcessor() {}

    virtual void ForceValue(double value) override;
    virtual void ForceValue(int param, double value) override;
    virtual void ForceValue(string value) override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuCon_FeedbackProcessor : public FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    EuCon_ControlSurface* const surface_ = nullptr;
    string address_ = "";
    
public:
    
    EuCon_FeedbackProcessor(EuCon_ControlSurface* surface, Widget* widget, string address) : FeedbackProcessor(widget), surface_(surface), address_(address) {}
    ~EuCon_FeedbackProcessor() {}

    virtual void ForceValue(double value) override;
    virtual void ForceValue(int param, double value) override;
    virtual void ForceValue(string value) override;
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
class ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int channelOffset);

    CSurfIntegrator* const CSurfIntegrator_ ;
    Page* const page_;
    string const name_;

    Zone* homeZone_ = nullptr;
    
    map<string, CSIMessageGenerator*> CSIMessageGeneratorsByMessage_;
    
    vector<Zone*> activeFocusedFXZones_;
    vector<Zone*> activeSelectedTrackFXZones_;
    vector<Zone*> activeSelectedTrackFXMenuFXZones_;
    vector<Zone*> activeSelectedTrackFXMenuZones_;
    
    vector<Zone*> activeSelectedTrackSendsZones_;
    vector<Zone*> activeSelectedTrackReceivesZones_;

    vector<Zone*> activeZones_;

    vector<vector<Zone*> *> allActiveZones_;
    
    void LoadDefaultZoneOrder()
    {
        allActiveZones_.clear();
        
        allActiveZones_.push_back(&activeFocusedFXZones_);
        allActiveZones_.push_back(&activeSelectedTrackFXZones_);
        allActiveZones_.push_back(&activeSelectedTrackFXMenuFXZones_);
        allActiveZones_.push_back(&activeSelectedTrackFXMenuZones_);
        allActiveZones_.push_back(&activeSelectedTrackSendsZones_);
        allActiveZones_.push_back(&activeSelectedTrackReceivesZones_);
        allActiveZones_.push_back(&activeZones_);
    }
    
    string const zoneFolder_ = "";
    int const numChannels_ = 0;
    int const numSends_ = 0;
    int const numFXSlots_ = 0;
    
    bool shouldBroadcastGoZone_ = false;
    bool shouldReceiveGoZone_ = false;
    bool shouldBroadcastGoFXSlot_ = false;
    bool shouldReceiveGoFXSlot_ = false;
    
    bool shouldBroadcastMapSelectedTrackSendsToWidgets_ = false;
    bool shouldReceiveMapSelectedTrackSendsToWidgets_ = false;
    bool shouldBroadcastMapSelectedTrackReceivesToWidgets_ = false;
    bool shouldReceiveMapSelectedTrackReceivesToWidgets_ = false;
    bool shouldBroadcastMapSelectedTrackFXToWidgets_ = false;
    bool shouldReceiveMapSelectedTrackFXToWidgets_ = false;
    bool shouldBroadcastMapSelectedTrackFXToMenu_ = false;
    bool shouldReceiveMapSelectedTrackFXToMenu_ = false;
    
    bool shouldBroadcastMapTrackSendsSlotToWidgets_ = false;
    bool shouldReceiveMapTrackSendsSlotToWidgets_ = false;
    bool shouldBroadcastMapTrackReceivesSlotToWidgets_ = false;
    bool shouldReceiveMapTrackReceivesSlotToWidgets_ = false;
    bool shouldBroadcastMapTrackFXMenusSlotToWidgets_ = false;
    bool shouldReceiveMapTrackFXMenusSlotToWidgets_ = false;

    map<int, Navigator*> navigators_;
    
    vector<Widget*> widgets_;
    map<string, Widget*> widgetsByName_;

    virtual void SurfaceOutMonitor(Widget* widget, string address, string value);

    void InitZones(string zoneFolder);

    map<string, string> zoneFilenames_;
    map<string, Zone*> zonesByName_;
    vector<Zone*> zones_;
    
    void MapSelectedTrackFXSlotToWidgets(vector<Zone*> &activeZones, int fxSlot);
    void MapSelectedTrackItemsToWidgets(MediaTrack* track, string baseName, int numberOfZones, vector<Zone*> &activeZones);
    
    void MapSelectedTrackFXMenuSlotToWidgetsImplementation(int slot);
    void GoZoneImplementation(vector<Zone*> &activeZones, string zoneName, double value);
    
    void MapSelectedTrackSendsToWidgetsImplementation();
    void MapSelectedTrackReceivesToWidgetsImplementation();
    void MapSelectedTrackFXToWidgetsImplementation();
    void MapSelectedTrackFXToMenuImplementation();
    
    void MapTrackSendsSlotToWidgetsImplementation();
    void MapTrackReceivesSlotToWidgetsImplementation();
    void MapTrackFXMenusSlotToWidgetsImplementation();
    
    void MapSelectedTrackSendsSlotToWidgetsImplementation();
    void MapSelectedTrackReceivesSlotToWidgetsImplementation();
    void MapSelectedTrackFXSlotToWidgetsImplementation();
    
    void UnmapSelectedTrackSendsFromWidgetsImplementation();
    void UnmapSelectedTrackReceivesFromWidgetsImplementation();
    void UnmapSelectedTrackFXFromWidgetsImplementation();
    void UnmapSelectedTrackFXFromMenuImplementation();
    
    void UnmapTrackSendsSlotFromWidgetsImplementation();
    void UnmapTrackReceivesSlotFromWidgetsImplementation();
    void UnmapTrackFXMenusSlotFromWidgetsImplementation();
    
    void UnmapSelectedTrackSendsSlotFromWidgetsImplementation();
    void UnmapSelectedTrackReceivesSlotFromWidgetsImplementation();
    void UnmapSelectedTrackFXSlotFromWidgetsImplementation();
    
    virtual void InitHardwiredWidgets()
    {
        // Add the "hardwired" widgets
        AddWidget(new Widget(this, "OnTrackSelection"));
        AddWidget(new Widget(this, "OnFXFocus"));
        AddWidget(new Widget(this, "OnPageEnter"));
        AddWidget(new Widget(this, "OnPageLeave"));
        AddWidget(new Widget(this, "OnInitialization"));
    }
    
public:
    virtual ~ControlSurface()
    {
        for(auto [key, messageGenerator] : CSIMessageGeneratorsByMessage_)
        {
            delete messageGenerator;
            messageGenerator = nullptr;
        }
        
        for(auto widget : widgets_)
        {
            delete widget;
            widget = nullptr;
        }
    };
    
    Page* GetPage() { return page_; }
    string GetName() { return name_; }
    
    virtual string GetSourceFileName() { return ""; }
    vector<Widget*> &GetWidgets() { return widgets_; }
    
    int GetNumChannels() { return numChannels_; }
    int GetNumSendSlots() { return numSends_; }
    int GetNumReceiveSlots() { return numSends_; }
    
    int  GetNumFXSlots() { return numFXSlots_; }
    
    void MapSelectedTrackSendsToWidgets();
    void MapSelectedTrackReceivesToWidgets();
    void MapSelectedTrackFXToWidgets();
    void MapSelectedTrackFXToMenu();
    
    void MapTrackSendsSlotToWidgets();
    void MapTrackReceivesSlotToWidgets();
    void MapTrackFXMenusSlotToWidgets();
    
    void MapSelectedTrackSendsSlotToWidgets();
    void MapSelectedTrackReceivesSlotToWidgets();
    void MapSelectedTrackFXSlotToWidgets();
    
    void UnmapSelectedTrackSendsFromWidgets();
    void UnmapSelectedTrackReceivesFromWidgets();
    void UnmapSelectedTrackFXFromWidgets();
    void UnmapSelectedTrackFXFromMenu();
    
    void UnmapTrackSendsSlotFromWidgets();
    void UnmapTrackReceivesSlotFromWidgets();
    void UnmapTrackFXMenusSlotFromWidgets();
    
    void UnmapSelectedTrackSendsSlotFromWidgets();
    void UnmapSelectedTrackReceivesSlotFromWidgets();
    void UnmapSelectedTrackFXSlotFromWidgets();
    
    void MapSelectedTrackFXMenuSlotToWidgets(int slot);
    void MapFocusedFXToWidgets();
    void UnmapFocusedFXFromWidgets();

    void TrackFXListChanged();
    
    void OnTrackSelection();

    Navigator* GetNavigatorForChannel(int channelNum);

    Zone* GetDefaultZone() { return homeZone_; }

    virtual void SetHasMCUMeters(int displayType) {}
    
    void LoadZone(string zoneName);
    Zone* GetZone(string zoneName);
    void GoZone(string zoneName, double value);
    void GoSubZone(Zone* enclosingZone, string zoneName, double value);
    virtual void LoadingZone(string zoneName) {}
    virtual void HandleExternalInput() {}
    virtual void InitializeEuCon() {}
    virtual void UpdateTimeDisplay() {}

    virtual bool GetIsEuConFXAreaFocused() { return false; }

    virtual void ForceRefreshTimeDisplay() {}
   
    virtual void SetBroadcastGoZone() { shouldBroadcastGoZone_ = true; }
    virtual void SetReceiveGoZone() { shouldReceiveGoZone_ = true; }
    virtual void SetBroadcastGoFXSlot() { shouldBroadcastGoFXSlot_ = true; }
    virtual void SetReceiveGoFXSlot() { shouldReceiveGoFXSlot_ = true; }
    
    virtual void SetBroadcastMapSelectedTrackSendsToWidgets() { shouldBroadcastMapSelectedTrackSendsToWidgets_ = true; }
    virtual void SetReceiveMapSelectedTrackSendsToWidgets() { shouldReceiveMapSelectedTrackSendsToWidgets_ = true; }
    virtual void SetBroadcastMapSelectedTrackReceivesToWidgets() { shouldBroadcastMapSelectedTrackReceivesToWidgets_ = true; }
    virtual void SetReceiveMapSelectedTrackReceivesToWidgets() { shouldReceiveMapSelectedTrackReceivesToWidgets_ = true; }
    virtual void SetBroadcastMapSelectedTrackFXToWidgets() { shouldBroadcastMapSelectedTrackFXToWidgets_ = true; }
    virtual void SetReceiveMapSelectedTrackFXToWidgets() { shouldReceiveMapSelectedTrackFXToWidgets_ = true; }
    virtual void SetBroadcastMapSelectedTrackFXToMenu() { shouldBroadcastMapSelectedTrackFXToMenu_ = true; }
    virtual void SetReceiveMapSelectedTrackFXToMenu() { shouldReceiveMapSelectedTrackFXToMenu_ = true; }
   
    virtual void SetBroadcastMapTrackSendsSlotToWidgets() { shouldBroadcastMapTrackSendsSlotToWidgets_ = true; }
    virtual void SetReceiveMapTrackSendsSlotToWidgets() { shouldReceiveMapTrackSendsSlotToWidgets_ = true; }
    virtual void SetBroadcastMapTrackReceivesSlotToWidgets() { shouldBroadcastMapTrackReceivesSlotToWidgets_ = true; }
    virtual void SetReceiveMapTrackReceivesSlotToWidgets() { shouldReceiveMapTrackReceivesSlotToWidgets_ = true; }
    virtual void SetBroadcastMapTrackFXMenusSlotToWidgets() { shouldBroadcastMapTrackFXMenusSlotToWidgets_ = true; }
    virtual void SetReceiveMapTrackFXMenusSlotToWidgets() { shouldReceiveMapTrackFXMenusSlotToWidgets_ = true; }
    
    void AcceptGoZone(string zoneName, double value)
    {
        if(shouldReceiveGoZone_)
            GoZoneImplementation(activeZones_, zoneName, value);
    }
    
    void AcceptGoFXSlot(int slot)
    {
        if(shouldReceiveGoFXSlot_)
            MapSelectedTrackFXMenuSlotToWidgetsImplementation(slot);
    }
    
    void AcceptMapSelectedTrackSendsToWidgets()
    {
        if(shouldReceiveMapSelectedTrackSendsToWidgets_)
            MapSelectedTrackSendsToWidgetsImplementation();
    }
    
    void AcceptMapSelectedTrackReceivesToWidgets()
    {
        if(shouldReceiveMapSelectedTrackReceivesToWidgets_)
            MapSelectedTrackReceivesToWidgetsImplementation();
    }
    
    void AcceptMapSelectedTrackFXToWidgets()
    {
        if(shouldReceiveMapSelectedTrackFXToWidgets_)
            MapSelectedTrackFXToWidgetsImplementation();
    }
    
    void AcceptMapSelectedTrackFXToMenu()
    {
        if(shouldReceiveMapSelectedTrackFXToMenu_)
            MapSelectedTrackFXToMenuImplementation();
    }
    
    void AcceptMapTrackSendsSlotToWidgets()
    {
        if(shouldReceiveMapTrackSendsSlotToWidgets_)
            MapTrackSendsSlotToWidgetsImplementation();
    }
    
    void AcceptMapTrackReceivesSlotToWidgets()
    {
        if(shouldReceiveMapTrackReceivesSlotToWidgets_)
            MapTrackReceivesSlotToWidgetsImplementation();
    }
    
    void AcceptMapTrackFXMenusSlotToWidgets()
    {
        if(shouldReceiveMapTrackFXMenusSlotToWidgets_)
            MapTrackFXMenusSlotToWidgetsImplementation();
    }
    
    void AcceptMapSelectedTrackSendsSlotToWidgets()
    {
        if(shouldReceiveMapTrackSendsSlotToWidgets_)
            MapSelectedTrackSendsSlotToWidgetsImplementation();
    }
    
    void AcceptMapSelectedTrackReceivesSlotToWidgets()
    {
        if(shouldReceiveMapTrackReceivesSlotToWidgets_)
            MapSelectedTrackReceivesSlotToWidgetsImplementation();
    }
    
    void AcceptMapSelectedTrackFXSlotToWidgets()
    {
        if(shouldReceiveMapTrackFXMenusSlotToWidgets_)
            MapSelectedTrackFXSlotToWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackSendsFromWidgets()
    {
        if(shouldReceiveMapSelectedTrackSendsToWidgets_)
            UnmapSelectedTrackSendsFromWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackReceivesFromWidgets()
    {
        if(shouldReceiveMapSelectedTrackReceivesToWidgets_)
            UnmapSelectedTrackReceivesFromWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackFXFromWidgets()
    {
        if(shouldReceiveMapSelectedTrackFXToWidgets_)
            UnmapSelectedTrackFXFromWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackFXFromMenu()
    {
        if(shouldReceiveMapSelectedTrackFXToMenu_)
            UnmapSelectedTrackFXFromMenuImplementation();
    }
    
    void AcceptUnmapTrackSendsSlotFromWidgets()
    {
        if(shouldReceiveMapTrackSendsSlotToWidgets_)
            UnmapTrackSendsSlotFromWidgetsImplementation();
    }
    
    void AcceptUnmapTrackReceivesSlotFromWidgets()
    {
        if(shouldReceiveMapTrackReceivesSlotToWidgets_)
            UnmapTrackReceivesSlotFromWidgetsImplementation();
    }
    
    void AcceptUnmapTrackFXMenusSlotFromWidgets()
    {
        if(shouldReceiveMapTrackFXMenusSlotToWidgets_)
            UnmapTrackFXMenusSlotFromWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackSendsSlotFromWidgets()
    {
        if(shouldReceiveMapTrackSendsSlotToWidgets_)
            UnmapSelectedTrackSendsSlotFromWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackReceivesSlotFromWidgets()
    {
        if(shouldReceiveMapTrackReceivesSlotToWidgets_)
            UnmapSelectedTrackReceivesSlotFromWidgetsImplementation();
    }
    
    void AcceptUnmapSelectedTrackFXSlotFromWidgets()
    {
        if(shouldReceiveMapTrackFXMenusSlotToWidgets_)
            UnmapSelectedTrackFXSlotFromWidgetsImplementation();
    }
    
    void MakeHomeDefault()
    {
        homeZone_ = GetZone("Home");

        if(homeZone_ != nullptr)
            homeZone_->Activate();
    }
    
    void PopWidget(vector<Zone*> &activeZones, Widget* widget)
    {
        for(auto zones : allActiveZones_)
            if(*zones != activeZones)
                for(auto zone : *zones)
                    if(zone->TryActivate(widget))
                        return;
        
        bool wentHome = false;
        
        if(homeZone_ != nullptr)
        {
            for(auto homeZoneWidget : homeZone_->GetWidgets())
            {
                if(widget == homeZoneWidget)
                {
                    widget->SetZone(homeZone_);
                    
                    wentHome = true;
                    break;
                }
            }
        }
        
        if(! wentHome)
        {
            widget->Clear();
            widget->SetZone(nullptr);
        }
    }
    
    void MoveToFirst(vector<Zone*> &zones)
    {
        auto result = find(allActiveZones_.begin(), allActiveZones_.end(), &zones);
        
        if(result != allActiveZones_.end())
        {
            auto resultValue = *result;
            allActiveZones_.erase(result);
            allActiveZones_.insert(allActiveZones_.begin(), resultValue);
        }
    }
    
    void DeactivateZones(vector<Zone*> &zones)
    {
        for(auto zone : zones)
            zone->Deactivate(zones);
        
        zones.clear();
    }

    void AddZoneFilename(string name, string filename)
    {
        zoneFilenames_[name] = filename;
    }
    
    void AddZone(Zone* zone)
    {
        zonesByName_[zone->GetName()] = zone;
        zones_.push_back(zone);
    }
   
    void CheckFocusedFXState()
    {
        int trackNumber = 0;
        int itemNumber = 0;
        int fxIndex = 0;
        
        if(DAW::GetFocusedFX2(&trackNumber, &itemNumber, &fxIndex) & 0x04) // 4 set if FX is no longer focused but still open
            UnmapFocusedFXFromWidgets();
        
        if(activeFocusedFXZones_.size() > 0)
        {
            Zone* activeZone = activeFocusedFXZones_[0];
            
            if(activeZone->GetNavigator()->GetTrack() == nullptr)
                UnmapFocusedFXFromWidgets();
        }
    }
    
    virtual void RequestUpdate()
    {
        CheckFocusedFXState();
        
        vector<Widget*> usedWidgets;

        for(auto activeZones : allActiveZones_)
            for(auto zone : *activeZones)
                zone->RequestUpdate(usedWidgets);
        
        if(homeZone_ != nullptr)
            homeZone_->RequestUpdate(usedWidgets);
    }

    virtual void ForceClearAllWidgets()
    {
        for(auto widget : widgets_)
            widget->ForceClear();
    }
    
    void ClearCache()
    {
        for(auto widget : widgets_)
        {
            widget->UpdateValue(0.0);
            widget->UpdateValue(0, 0.0);
            widget->UpdateValue("");
            widget->ClearCache();
        }
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
        widgetsByName_[widget->GetName()] = widget;
    }
    
    void AddCSIMessageGenerator(string message, CSIMessageGenerator* messageGenerator)
    {
        CSIMessageGeneratorsByMessage_[message] = messageGenerator;
    }

    Widget* GetWidgetByName(string name)
    {
        if(widgetsByName_.count(name) > 0)
            return widgetsByName_[name];
        else
            return nullptr;
    }
    
    void OnFXFocus(MediaTrack* track, int fxIndex)
    {
        if(widgetsByName_.count("OnFXFocus") > 0)
            widgetsByName_["OnFXFocus"]->DoAction(1.0);
    }
    
    void OnPageEnter()
    {
        if(widgetsByName_.count("OnPageEnter") > 0)
            widgetsByName_["OnPageEnter"]->DoAction(1.0);
    }
    
    void OnPageLeave()
    {
        if(widgetsByName_.count("OnPageLeave") > 0)
            widgetsByName_["OnPageLeave"]->DoAction(1.0);
    }
    
    void OnInitialization()
    {
        if(widgetsByName_.count("OnInitialization") > 0)
            widgetsByName_["OnInitialization"]->DoAction(1.0);
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
    map<int, vector<Midi_CSIMessageGenerator*>> Midi_CSIMessageGeneratorsByMessage_;
    
    // special processing for MCU meters
    bool hasMCUMeters_ = false;
    int displayType_ = 0x14;
    
    void ProcessMidiMessage(const MIDI_event_ex_t* evt);
   
    void InitWidgets(string templateFilename, string zoneFolder);

    void InitializeMCU();
    void InitializeMCUXT();
    
    virtual void Initialize()
    {
        if(hasMCUMeters_)
        {
            if(displayType_ == 0x14)
                InitializeMCU();
            else
                InitializeMCUXT();
        }
    }

public:
    Midi_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int numChannels, int numSends, int numFX, int channelOffset, midi_Input* midiInput, midi_Output* midiOutput)
    : ControlSurface(CSurfIntegrator, page, name, zoneFolder, numChannels, numSends, numFX, channelOffset), templateFilename_(templateFilename), midiInput_(midiInput), midiOutput_(midiOutput)
    {
        InitWidgets(templateFilename, zoneFolder);
    }
    
    virtual ~Midi_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "/CSI/Surfaces/Midi/" + templateFilename_; }
    
    void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(int first, int second, int third);

    virtual void SetHasMCUMeters(int displayType) override
    {
        hasMCUMeters_ = true;
        displayType_ = displayType;
    }
    
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
        Midi_CSIMessageGeneratorsByMessage_[message].push_back(messageGenerator);
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
    
    void InitWidgets(string templateFilename, string zoneFolder);
    void ProcessOSCMessage(string message, double value);

public:
    OSC_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int numChannels, int numSends, int numFX, int channelOffset, oscpkt::UdpSocket* inSocket, oscpkt::UdpSocket* outSocket)
    : ControlSurface(CSurfIntegrator, page, name, zoneFolder, numChannels, numSends, numFX, channelOffset), templateFilename_(templateFilename), inSocket_(inSocket), outSocket_(outSocket)
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
    ~WidgetGroup()
    {
        for(auto [key, group] : subGroups_)
        {
            delete group;
            group = nullptr;
        }
    }
    
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
class EuCon_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool isEuConFXAreaFocused_ = false;
    double previousPP = 0.0;
    
    vector<Widget*> generalWidgets_;
    map<int, WidgetGroup*> channelGroups_;
    
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
    EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int channelOffset);
    
    virtual ~EuCon_ControlSurface()
    {
        for(auto [key, group] : channelGroups_)
        {
            delete group;
            group = nullptr;
        }
    }
    
    virtual string GetSourceFileName() override { return "EuCon"; }
    
    virtual bool GetIsEuConFXAreaFocused() override { return isEuConFXAreaFocused_; }

    virtual void InitializeEuCon() override;
    virtual void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems);
    void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, double value);
    void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, double value, int param);
    void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, string value);
    void SendEuConMessage(string address, string value);
    void HandleEuConMessage(string address, double value);
    void HandleEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible);
    void HandleEuConGetMeterValues(int id, int iLeg, float* oLevel, float* oPeak, bool* oLegClip);
    void HandleEuConGetFormattedFXParamValue(const char* address, char *buffer, int bufferSize);
    virtual void UpdateTimeDisplay() override;
    
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
    
    int sendSlot_ = 0;
    int receiveSlot_ = 0;
    int fxMenuSlot_ = 0;
    
    int maxSendSlot_ = 0;
    int maxReceiveSlot_ = 0;
    int maxFXMenuSlot_ = 0;
    
public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages, bool scrollLink, int numChannels) : page_(page), followMCP_(followMCP), synchPages_(synchPages), scrollLink_(scrollLink),
    masterTrackNavigator_(new MasterTrackNavigator(page_)),
    selectedTrackNavigator_(new SelectedTrackNavigator(page_)),
    focusedFXNavigator_(new FocusedFXNavigator(page_)),
    defaultNavigator_(new Navigator(page_))
    {
        for(int i = 0; i < numChannels; i++)
            navigators_.push_back(new TrackNavigator(page_, this, i));
    }
    
    ~TrackNavigationManager()
    {
        for(auto navigator : navigators_)
        {
            delete navigator;
            navigator = nullptr;
        }
        
        delete masterTrackNavigator_;
        delete selectedTrackNavigator_;
        delete focusedFXNavigator_;
        delete defaultNavigator_;
    }
    
    Page* GetPage() { return page_; }
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    bool GetVCAMode() { return vcaMode_; }
    int  GetNumTracks() { return DAW::CSurf_NumTracks(followMCP_); }
    Navigator* GetMasterTrackNavigator() { return masterTrackNavigator_; }
    Navigator* GetSelectedTrackNavigator() { return selectedTrackNavigator_; }
    Navigator* GetFocusedFXNavigator() { return focusedFXNavigator_; }
    Navigator* GetDefaultNavigator() { return defaultNavigator_; }

    void ForceScrollLink();
    void OnTrackSelectionBySurface(MediaTrack* track);
    void AdjustTrackBank(int amount);
    
    int GetSendSlot() { return sendSlot_; }
    int GetReceiveSlot() { return receiveSlot_; }
    int GetFXMenuSlot() { return fxMenuSlot_; }
    
    void AdjustSendSlotBank(int amount)
    {
        sendSlot_ += amount;
        
        if(sendSlot_ < 0)
            sendSlot_ = 0;
        
        if(sendSlot_ > maxSendSlot_)
            sendSlot_ = maxSendSlot_;
    }
    
    void AdjustReceiveSlotBank(int amount)
    {
        receiveSlot_ += amount;
        
        if(receiveSlot_ < 0)
            receiveSlot_ = 0;
        
        if(receiveSlot_ > maxReceiveSlot_)
            receiveSlot_ = maxReceiveSlot_;
    }
    
    void AdjustFXMenuSlotBank(int amount)
    {
        fxMenuSlot_ += amount;
        
        if(fxMenuSlot_ < 0)
            fxMenuSlot_ = 0;
        
        if(fxMenuSlot_ > maxFXMenuSlot_)
            fxMenuSlot_ = maxFXMenuSlot_;
    }
    
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
 
    Navigator* GetNavigatorForChannel(int channelNum)
    {
        if(channelNum < navigators_.size())
            return navigators_[channelNum];
        else
            return nullptr;
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

    int GetIdFromTrack(MediaTrack* track)
    {
        return DAW::CSurf_TrackToID(track, followMCP_);
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
        maxSendSlot_ = 0;
        maxReceiveSlot_ = 0;
        maxFXMenuSlot_ = 0;
        
        // Clean up vcaSpillTracks
        vcaSpillTracks_.erase(remove_if(vcaSpillTracks_.begin(), vcaSpillTracks_.end(), IsTrackPointerStale), vcaSpillTracks_.end());

        // Get Visible Tracks
        for (int i = 1; i <= GetNumTracks(); i++)
        {
            MediaTrack* track = DAW::CSurf_TrackFromID(i, followMCP_);
            
            if(DAW::IsTrackVisible(track, followMCP_))
            {
                int maxSendSlot = DAW::GetTrackNumSends(track, 0) - 1;
                if(maxSendSlot > maxSendSlot_)
                    maxSendSlot_ = maxSendSlot;

                //if(sendSlot_ > maxSendSlot_)
                    //sendSlot_ = maxSendSlot_;
                
                int maxReceiveSlot = DAW::GetTrackNumSends(track, -1) - 1;
                if(maxReceiveSlot > maxReceiveSlot_)
                    maxReceiveSlot_ = maxReceiveSlot;
                
                //if(receiveSlot_ > maxReceiveSlot_)
                    //receiveSlot_ = maxReceiveSlot_;

                int maxFXMenuSlot = DAW::TrackFX_GetCount(track) - 1;
                if(maxFXMenuSlot > maxFXMenuSlot_)
                    maxFXMenuSlot_ = maxFXMenuSlot;
                
                //if(fxMenuSlot_ > maxFXMenuSlot)
                    //fxMenuSlot_ = maxFXMenuSlot;
                
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

        if(sendSlot_ > maxSendSlot_)
            sendSlot_ = maxSendSlot_;

        if(receiveSlot_ > maxReceiveSlot_)
            receiveSlot_ = maxReceiveSlot_;

        if(fxMenuSlot_ > maxFXMenuSlot_)
            fxMenuSlot_ = maxFXMenuSlot_;
        
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
        if(track == GetMasterTrackNavigator()->GetTrack())
            return GetIsNavigatorTouched(GetMasterTrackNavigator(), touchedControl);
        
        for(auto navigator : navigators_)
            if(track == navigator->GetTrack())
                return GetIsNavigatorTouched(navigator, touchedControl);
 
        if(MediaTrack* selectedTrack = GetSelectedTrack())
             if(track == selectedTrack)
                return GetIsNavigatorTouched(GetSelectedTrackNavigator(), touchedControl);
        
        if(MediaTrack* focusedFXTrack = GetFocusedFXNavigator()->GetTrack())
            if(track == focusedFXTrack)
                return GetIsNavigatorTouched(GetFocusedFXNavigator(), touchedControl);

        return false;
    }
    
    bool GetIsNavigatorTouched(Navigator* navigator,  int touchedControl)
    {
        if(touchedControl == 0)
            return navigator->GetIsVolumeTouched();
        else if(touchedControl == 1)
        {
            if(navigator->GetIsPanTouched() || navigator->GetIsPanLeftTouched())
                return true;
        }
        else if(touchedControl == 2)
        {
            if(navigator->GetIsPanWidthTouched() || navigator->GetIsPanRightTouched())
                return true;
        }

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

    Navigator* defaultNavigator_ = nullptr;
    
public:
    Page(string name, bool followMCP, bool synchPages, bool scrollLink, int numChannels) : name_(name),  trackNavigationManager_(new TrackNavigationManager(this, followMCP, synchPages, scrollLink, numChannels)), defaultNavigator_(new Navigator(this)) { }
    
    ~Page()
    {
        for(auto surface: surfaces_)
        {
            delete surface;
            surface = nullptr;
        }
        
        delete trackNavigationManager_;
        delete defaultNavigator_;
    }
    
    string GetName() { return name_; }
    TrackNavigationManager* GetTrackNavigationManager() { return trackNavigationManager_; }
    
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
    
    void GoZone(ControlSurface* originator, string zoneName, double value)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptGoZone(zoneName, value);
    }
    
    void GoFXSlot(ControlSurface* originator, int slot)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptGoFXSlot(slot);
    }
    
    void MapSelectedTrackSendsToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackSendsToWidgets();
    }
    
    void MapSelectedTrackReceivesToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackReceivesToWidgets();
    }
    
    void MapSelectedTrackFXToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackFXToWidgets();
    }
    
    void MapSelectedTrackFXToMenu(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackFXToMenu();
    }
    
    void MapTrackSendsSlotToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapTrackSendsSlotToWidgets();
    }
    
    void MapTrackReceivesSlotToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapTrackReceivesSlotToWidgets();
    }
    
    void MapTrackFXMenusSlotToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapTrackFXMenusSlotToWidgets();
    }
    
    void MapSelectedTrackSendsSlotToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackSendsSlotToWidgets();
    }
    
    void MapSelectedTrackReceivesSlotToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackReceivesSlotToWidgets();
    }
    
    void MapSelectedTrackFXSlotToWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptMapSelectedTrackFXSlotToWidgets();
    }
    
    void UnmapSelectedTrackSendsFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackSendsFromWidgets();
    }
    
    void UnmapSelectedTrackReceivesFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackReceivesFromWidgets();
    }
    
    void UnmapSelectedTrackFXFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackFXFromWidgets();
    }
    
    void UnmapSelectedTrackFXFromMenu(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackFXFromMenu();
    }
    
    void UnmapTrackSendsSlotFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapTrackSendsSlotFromWidgets();
    }
    
    void UnmapTrackReceivesSlotFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapTrackReceivesSlotFromWidgets();
    }
    
    void UnmapTrackFXMenusSlotFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapTrackFXMenusSlotFromWidgets();
    }
    
    void UnmapSelectedTrackSendsSlotFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackSendsSlotFromWidgets();
    }
    
    void UnmapSelectedTrackReceivesSlotFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackReceivesSlotFromWidgets();
    }
    
    void UnmapSelectedTrackFXSlotFromWidgets(ControlSurface* originator)
    {
        for(auto surface : surfaces_)
            if(surface != originator)
                surface->AcceptUnmapSelectedTrackFXSlotFromWidgets();
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
            surface->TrackFXListChanged();
    }

    void EnterPage()
    {
        trackNavigationManager_->EnterPage();
        
        for(auto surface : surfaces_)
            surface->ClearCache();
        
        for(auto surface : surfaces_)
            surface->OnPageEnter();
    }
    
    void LeavePage()
    {
        trackNavigationManager_->LeavePage();
        
        for(auto surface : surfaces_)
            surface->OnPageLeave();

        for(auto surface : surfaces_)
            surface->ClearCache();        
    }
    
    void OnInitialization()
    {
        for(auto surface : surfaces_)
            surface->OnInitialization();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    CSurfIntegrator* const CSurfIntegrator_ = nullptr;

    map<string, Action*> actions_;

    vector <Page*> pages_;
    
    map<string, map<string, int>> fxParamIndices_;
    
    int currentPageIndex_ = 0;
    bool surfaceInDisplay_ = false;
    bool surfaceOutDisplay_ = false;
    bool fxParamsDisplay_ = false;
    bool fxParamsWrite_ = false;

    bool shouldRun_ = true;
    
    int *timeModePtr_ = nullptr;
    int *timeMode2Ptr_ = nullptr;
    int *measOffsPtr_ = nullptr;
    double *timeOffsPtr_ = nullptr;
    int *projectPanModePtr_ = nullptr;
    
    void InitActionsDictionary();

    double GetPrivateProfileDouble(string key)
    {
        char tmp[512];
        memset(tmp, 0, sizeof(tmp));
        
        DAW::GetPrivateProfileString("REAPER", key.c_str() , "", tmp, sizeof(tmp), DAW::get_ini_file());
        
        return strtod (tmp, NULL);
    }
    
public:
    ~Manager()
    {
        for(auto page : pages_)
        {
            delete page;
            page = nullptr;
        }
        
        for(auto [key, action] : actions_)
        {
            delete action;
            action = nullptr;
        }
    }
    
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
        
        index = projectconfig_var_getoffs("panmode", &size);
        projectPanModePtr_ = (int*)projectconfig_var_addr(nullptr, index);
    }
    
    void Shutdown()
    {
        fxParamsDisplay_ = false;
        surfaceInDisplay_ = false;
        surfaceOutDisplay_ = false;
       
        // GAW -- IMPORTANT
        // We want to stop polling and zero out all Widgets before shutting down
        shouldRun_ = false;
        
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->ForceClearAllWidgets();
    }
    
    void Init();

    void ToggleSurfaceInDisplay() { surfaceInDisplay_ = ! surfaceInDisplay_;  }
    void ToggleSurfaceOutDisplay() { surfaceOutDisplay_ = ! surfaceOutDisplay_;  }
    void ToggleFXParamsDisplay() { fxParamsDisplay_ = ! fxParamsDisplay_;  }
    void ToggleFXParamsWrite() { fxParamsWrite_ = ! fxParamsWrite_;  }

    bool GetSurfaceInDisplay() { return surfaceInDisplay_;  }
    bool GetSurfaceOutDisplay() { return surfaceOutDisplay_;  }
    
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    int *GetTimeModePtr() { return timeModePtr_; }
    int *GetTimeMode2Ptr() { return timeMode2Ptr_; }
    int *GetMeasOffsPtr() { return measOffsPtr_; }
    double *GetTimeOffsPtr() { return timeOffsPtr_; }
    int GetProjectPanMode() { return *projectPanModePtr_; }
   
    ActionContext GetActionContext(string actionName, Widget* widget, Zone* zone, vector<string> params, vector<vector<string>> properties)
    {      
        if(actions_.count(actionName) > 0)
            return ActionContext(actions_[actionName], widget, zone, params, properties);
        else
            return ActionContext(actions_["NoAction"], widget, zone, params, properties);
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
    
    void AdjustSendSlotBank(Page* sendingPage, int amount)
    {
        if(! sendingPage->GetTrackNavigationManager()->GetSynchPages())
            sendingPage->GetTrackNavigationManager()->AdjustSendSlotBank(amount);
        else
            for(auto page: pages_)
                if(page->GetTrackNavigationManager()->GetSynchPages())
                    page->GetTrackNavigationManager()->AdjustSendSlotBank(amount);
    }
    
    void AdjustReceiveSlotBank(Page* sendingPage, int amount)
    {
        if(! sendingPage->GetTrackNavigationManager()->GetSynchPages())
            sendingPage->GetTrackNavigationManager()->AdjustReceiveSlotBank(amount);
        else
            for(auto page: pages_)
                if(page->GetTrackNavigationManager()->GetSynchPages())
                    page->GetTrackNavigationManager()->AdjustReceiveSlotBank(amount);
    }
    
    void AdjustFXMenuSlotBank(Page* sendingPage, int amount)
    {
        if(! sendingPage->GetTrackNavigationManager()->GetSynchPages())
            sendingPage->GetTrackNavigationManager()->AdjustFXMenuSlotBank(amount);
        else
            for(auto page: pages_)
                if(page->GetTrackNavigationManager()->GetSynchPages())
                    page->GetTrackNavigationManager()->AdjustFXMenuSlotBank(amount);
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
    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto & page : pages_)
            page->TrackFXListChanged(track);
        
        if(fxParamsDisplay_ || fxParamsWrite_)
        {
            char fxName[BUFSZ];
            char fxParamName[BUFSZ];
            
            ofstream fxFile;
            
            for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
            {
                DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
                
                if(fxParamsDisplay_)
                    DAW::ShowConsoleMsg(("Zone \"" + string(fxName) + "\"").c_str());
                
                if(fxParamsWrite_)
                {
                    string fxNameNoBadChars(fxName);
                    fxNameNoBadChars = regex_replace(fxNameNoBadChars, regex(BadFileChars), "_");

                    fxFile.open(string(DAW::GetResourcePath()) + "/CSI/Zones/ZoneRawFXFiles/" + fxNameNoBadChars + ".txt");
                    
                    if(fxFile.is_open())
                        fxFile << "Zone \"" + string(fxName) + "\"" + GetLineEnding();
                }

                if(fxParamsDisplay_)
                    DAW::ShowConsoleMsg("\n\n\tSelectedTrackNavigator\n");
                
                if(fxParamsWrite_ && fxFile.is_open())
                    fxFile << "\tSelectedTrackNavigator" + GetLineEnding();

                for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                {
                    DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));

                    if(fxParamsDisplay_)
                        DAW::ShowConsoleMsg(("\n\tFXParam " + to_string(j) + " \"" + string(fxParamName) + "\"").c_str());
  
                    if(fxParamsWrite_ && fxFile.is_open())
                        fxFile <<  "\tFXParam " + to_string(j) + " \"" + string(fxParamName)+ "\"" + GetLineEnding();
                        
                    /* step sizes
                    double stepOut = 0;
                    double smallstepOut = 0;
                    double largestepOut = 0;
                    bool istoggleOut = false;
                    TrackFX_GetParameterStepSizes(track, i, j, &stepOut, &smallstepOut, &largestepOut, &istoggleOut);

                    DAW::ShowConsoleMsg(("\n\n" + to_string(j) + " - \"" + string(fxParamName) + "\"\t\t\t\t Step = " +  to_string(stepOut) + " Small Step = " + to_string(smallstepOut)  + " LargeStep = " + to_string(largestepOut)  + " Toggle Out = " + (istoggleOut == 0 ? "false" : "true")).c_str());
                    */
                }
                
                if(fxParamsDisplay_)
                    DAW::ShowConsoleMsg("\nZoneEnd\n\n");

                if(fxParamsWrite_ && fxFile.is_open())
                {
                    fxFile << "ZoneEnd";
                    fxFile.close();
                }
            }
        }
    }

    void InitializeEuCon()
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->InitializeEuCon();
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
