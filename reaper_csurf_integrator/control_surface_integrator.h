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
const string FXParam = "FXParam";

const string BadFileChars = "[ \\:*?<>|.,()/]";
const string CRLFChars = "[\r\n]";
const string TabChars = "[\t]";

const int TempDisplayTime = 1250;

class Manager;
extern Manager* TheManager;

struct CSIWidgetInfo
{
    std::string group = "General";
    int channelNumber = 0;
    int sendNumber = 0;
    bool isVisible = true;
    std::string name = "";
    std::string control = "";
    std::string FB_Processor = "";
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor) : CSIWidgetInfo(aName, aControl, aFB_Processor, "General", 0, true) {}
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor, std::string aGroup, int aChannelNumber, bool isVisible) :  CSIWidgetInfo(aName, aControl, aFB_Processor, aGroup, aChannelNumber, 0, isVisible) {}
    
    CSIWidgetInfo(std::string aName, std::string aControl, std::string aFB_Processor, std::string aGroup, int aChannelNumber, int aSendNumber, bool itemIsVisible) :  name(aName), control(aControl), FB_Processor(aFB_Processor), group(aGroup), channelNumber(aChannelNumber), sendNumber(aSendNumber), isVisible(itemIsVisible) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfIntegrator;
class Page;
class ControlSurface;
class Midi_ControlSurface;
class OSC_ControlSurface;
class EuCon_ControlSurface;
class Zone;
class Action;
class TrackNavigationManager;
class FeedbackProcessor;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Navigator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Page* const page_;
    bool isFaderTouched_ = false;
    bool isRotaryTouched_ = false;

public:
    Navigator(Page*  page) : page_(page) {}
    virtual ~Navigator() {}

    bool GetIsFaderTouched() { return isFaderTouched_; }
    void SetIsFaderTouched(bool isFaderTouched) { isFaderTouched_ = isFaderTouched; }
    
    bool GetIsRotaryTouched() { return isRotaryTouched_; }
    void SetIsRotaryTouched(bool isRotaryTouched) { isRotaryTouched_ = isRotaryTouched; }
    
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
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_;
    string const name_;
    FeedbackProcessor* const feedbackProcessor_;
    bool isModifier_ = false;
    Zone* activeZone_ = nullptr;
    map<string, Zone*> zonesAvailable_;
 
    map<Zone*, map<string, vector <Action*>>> actions_;   // vector<Action*> actionList = actions_[zoneName][modifiers];
    map<Zone*, map<string, vector <Action*>>> trackTouchedActions_;
    
public:
    Widget(ControlSurface* surface, string name, FeedbackProcessor*  feedbackProcessor);
    virtual ~Widget() {};
    
    ControlSurface* GetSurface() { return surface_; }
    string GetName() { return name_; }
    FeedbackProcessor* GetFeedbackProcessor() { return feedbackProcessor_; }
    string GetCurrentZoneActionDisplay(string surfaceName);
    void AddAction(Zone* zone, string modifiers, Action* action);
    void AddTrackTouchedAction(Zone* zone, string modifiers, Action* action);
    void SetIsModifier() { isModifier_ = true; }
    
    void GoZone(string zoneName)
    {
        if(zonesAvailable_.count(zoneName) > 0)
            activeZone_ = zonesAvailable_[zoneName];
    }
    
    void ActivateNoActionForZone(string zoneName)
    {
        if(zonesAvailable_.count(zoneName) > 0 && zonesAvailable_.count("NoAction") > 0)
        {
            activeZone_ = zonesAvailable_["NoAction"];
            
            Clear();
        }
    }
    
    void Deactivate(Zone* zone)
    {
        if(zone == activeZone_ && zonesAvailable_.count("Home") > 0)
            activeZone_ = zonesAvailable_["Home"];
    }
    
    void Clear();
    void ForceClear();

    MediaTrack* GetTrack();
    void RequestUpdate();
    void DoAction(double value);
    void DoRelativeAction(double value);
    void SetIsFaderTouched(bool isFaderTouched);
    void SetIsRotaryTouched(bool isRotaryTouched);
    void UpdateValue(double value);
    void UpdateValue(int mode, double value);
    void UpdateValue(string value);
    void UpdateRGBValue(int r, int g, int b);
    void ClearCache();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double lastValue_ = 0.0;
    string lastStringValue_ = "";

    bool supportsRGB_ = false;
    vector<rgb_color> RGBValues_;
    
    bool supportsTrackColor_ = false;
    
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
                
                if(strVal == "Track")
                {
                    supportsTrackColor_ = true;
                    break;
                }
                else
                {
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
                
                if(regex_match(strVal, regex("[0-9]+[.][0-9]+")) || regex_match(strVal, regex("[0-9]")))
                    steppedValues_.push_back(stod(strVal));
                else if(regex_match(strVal, regex("[0-9]+[.][0-9]+[-][0-9]+[.][0-9]+")))
                {
                    istringstream range(strVal);
                    vector<string> range_tokens;
                    string range_token;
                    
                    while (getline(range, range_token, '-'))
                        range_tokens.push_back(range_token);

                    if(range_tokens.size() == 2)
                    {
                        double firstValue = stod(range_tokens[0]);
                        double lastValue = stod(range_tokens[1]);

                        if(lastValue > firstValue)
                        {
                            rangeValues_.push_back(firstValue);
                            rangeValues_.push_back(lastValue);
                        }
                        else
                        {
                            rangeValues_.push_back(lastValue);
                            rangeValues_.push_back(firstValue);
                        }
                    }
                }
            }
        }
    }
    
protected:
    Action(string name, Widget* widget, Zone* zone, vector<string> params): name_(name), widget_(widget), zone_(zone)
    {
        SetRGB(params);
        SetSteppedValues(params);
    }

    string const name_;
    Widget* const widget_;
    Zone* const zone_;
    
    vector<double> steppedValues_;
    int steppedValuesIndex_ = 0;

    vector<double> rangeValues_;

    bool isInverted_ = false;
    bool shouldToggle_ = false;
    
    bool asa = false;
    double delayAmount_ = 0.0;
    double delayStartTime_ = 0.0;
    double deferredValue_ = 0.0;
    Widget* deferredSender_ = nullptr;
    
    int GetSlotIndex();
    
    virtual void RequestTrackUpdate(MediaTrack* track) {}
    virtual void Do(string value, Widget* sender) {}
    virtual void Do(double value, Widget* sender) {}
    
public:
    virtual ~Action() {}
    
    Page* GetPage();
    ControlSurface* GetSurface();
    Widget* GetWidget() { return widget_; }
    Zone* GetZone() { return zone_; }
    
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
    
    virtual void DoAction(double value, Widget* sender);
    virtual void DoRelativeAction(double value, Widget* sender);
    virtual double GetCurrentValue() { return 0.0; }
    
    void PerformDeferredActions()
    {
        if(deferredSender_ != nullptr && delayAmount_ != 0.0 && delayStartTime_ != 0.0 && DAW::GetCurrentNumberOfMilliseconds() > (delayStartTime_ + delayAmount_))
        {
            double savedDelayAmount = delayAmount_;
            delayAmount_ = 0.0;
            DoAction(deferredValue_, deferredSender_);
            delayAmount_ = savedDelayAmount;
            delayStartTime_ = 0.0;
            deferredValue_ = 0.0;
            deferredSender_ = nullptr;
        }
    }
    
    virtual void RequestUpdate()
    {
        if(supportsRGB_)
            GetWidget()->UpdateRGBValue(RGBValues_[0].r, RGBValues_[0].g, RGBValues_[0].b);
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
    
    void UpdateWidgetValue(double value)
    {
        value = isInverted_ == false ? value : 1.0 - value;
     
        SetSteppedValueIndex(value);
        
        lastValue_ = value;
       
        widget_->UpdateValue(value);
        
        
        if(supportsRGB_)
        {
            currentRGBIndex_ = value == 0 ? 0 : 1;
            widget_->UpdateRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
        }
        else if(supportsTrackColor_)
        {
            if(MediaTrack* track = GetWidget()->GetTrack())
            {
                unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);

                int r = (*rgb_colour >> 0) & 0xff;
                int g = (*rgb_colour >> 8) & 0xff;
                int b = (*rgb_colour >> 16) & 0xff;
                
                widget_->UpdateRGBValue(r, g, b);
            }
        }
    }
    
    void UpdateWidgetValue(int param, double value)
    {
        value = isInverted_ == false ? value : 1.0 - value;
        
        SetSteppedValueIndex(value);
        
        lastValue_ = value;

        widget_->UpdateValue(param, value);
        
        currentRGBIndex_ = value == 0 ? 0 : 1;
        
        if(supportsRGB_)
        {
            currentRGBIndex_ = value == 0 ? 0 : 1;
            widget_->UpdateRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
        }
        else if(supportsTrackColor_)
        {
            if(MediaTrack* track = GetWidget()->GetTrack())
            {
                unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
                
                int r = (*rgb_colour >> 0) & 0xff;
                int g = (*rgb_colour >> 8) & 0xff;
                int b = (*rgb_colour >> 16) & 0xff;
                
                widget_->UpdateRGBValue(r, g, b);
            }
        }
    }
    
    void UpdateWidgetValue(string value)
    {
        widget_->UpdateValue(value);
        
        if(supportsTrackColor_)
        {
            if(MediaTrack* track = GetWidget()->GetTrack())
            {
                unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
                
                int r = (*rgb_colour >> 0) & 0xff;
                int g = (*rgb_colour >> 8) & 0xff;
                int b = (*rgb_colour >> 16) & 0xff;
                
                widget_->UpdateRGBValue(r, g, b);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NoAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NoAction(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    virtual ~NoAction() {}
    
    virtual void RequestUpdate() {  GetWidget()->Clear(); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Navigator* const navigator_= nullptr;
    ControlSurface* const surface_ = nullptr;
    string const name_ = "";
    string const alias_ = "";
    string const sourceFilePath_ = "";
    vector<Zone*> includedZones_;
    int index_ = 0;
    
public:
    Zone(Navigator* navigator, ControlSurface* surface, int channelNum, string name, string sourceFilePath, string alias) : navigator_(navigator), surface_(surface), name_(name), sourceFilePath_(sourceFilePath), alias_(alias) {}

    virtual ~Zone() {}
    
    ControlSurface* GetSurface() { return surface_; }
    int GetIndex() { return index_; }
    string GetName() { return name_ ;}
    string GetAlias() { return alias_;}
    string GetSourceFilePath() { return sourceFilePath_; }
    Navigator* GetNavigator() { return navigator_; }

    bool GetIsFaderTouched() { return navigator_->GetIsFaderTouched();; }
    void SetIsFaderTouched(bool isFaderTouched) { navigator_->SetIsFaderTouched(isFaderTouched); }

    bool GetIsRotaryTouched() { return navigator_->GetIsRotaryTouched();; }
    void SetIsRotaryTouched(bool isRotaryTouched) { navigator_->SetIsRotaryTouched(isRotaryTouched); }

    void Activate();
    void Deactivate();

    // GAW TBD -- remove -- change learn mode editor to file based
    vector<Zone*> &GetIncludedZones() { return includedZones_; }
    
    // GAW TBD -- maybe allow this later after fully debugged
    //void SetTrackNavigator(Navigator* navigator) { navigator_ = navigator; }
    //void SetAlias(string alias) { alias_ = alias;}
       
    bool GetHasFocusedFXTrackNavigator()
    {
        return navigator_->GetIsFocusedFXNavigator();
    }
    
    void AddZone(Zone* zone)
    {
        includedZones_.push_back(zone);
    }
    
    void Activate(int index)
    {
        index_ = index;
        Activate();
        
        for(auto zone : includedZones_)
            zone->Activate(index);
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
    Widget* widget_ = nullptr;
    
public:
    virtual ~FeedbackProcessor() {}
    void SetWidget(Widget* widget) { widget_ = widget;  }
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
    
    Midi_FeedbackProcessor(Midi_ControlSurface* surface) : surface_(surface) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : surface_(surface), midiFeedbackMessage1_(feedback1) {}
    Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1, MIDI_event_ex_t* feedback2) : surface_(surface), midiFeedbackMessage1_(feedback1), midiFeedbackMessage2_(feedback2) {}
    
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
    
    OSC_FeedbackProcessor(OSC_ControlSurface* surface, string oscAddress) : surface_(surface), oscAddress_(oscAddress) {}
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
    
    EuCon_FeedbackProcessor(EuCon_ControlSurface* surface, string address) : surface_(surface), address_(address) {}
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
    
    EuCon_FeedbackProcessorDB(EuCon_ControlSurface* surface, string address) : EuCon_FeedbackProcessor(surface, address) {}
    ~EuCon_FeedbackProcessorDB() {}
    
    virtual void Clear() override;
    virtual void ForceClear() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SendsActivationManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* const surface_;
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
    map<string, vector<string>> zoneFileLines_;

    
    void InitHardwiredWidgets()
    {
        // Add the "hardwired" widgets
        widgets_.push_back(new Widget(this, "OnTrackSelection", new FeedbackProcessor()));
        widgets_.push_back(new Widget(this, "OnFXFocus", new FeedbackProcessor()));
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
    SendsActivationManager* GetSendsActivationManager() { return sendsActivationManager_; }
    bool GetUseZoneLink() { return useZoneLink_; }
    void SetUseZoneLink(bool useZoneLink) { useZoneLink_ = useZoneLink; }
    virtual void LoadingZone(string zoneName) {}
    virtual void HandleExternalInput() {}
    virtual void InitializeEuCon() {}
    virtual void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems) {}
    virtual void ReceiveEuConMessage(string oscAddress, double value) {}
    virtual void ReceiveEuConMessage(string oscAddress, string value) {}
    virtual void UpdateTimeDisplay() {}
    virtual void ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible) {}
    virtual void ForceRefreshTimeDisplay() {}

    void GoHome() { GoZone("Home"); }
    
    MediaTrack* GetTrack(string activeZoneName)
    {
        if(zones_.count(activeZoneName) > 0)
            return zones_[activeZoneName]->GetNavigator()->GetTrack();
        else
            return nullptr;
    }
    
    void GoZone(string zoneName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->Activate();
    }

    void ActivateNoActionForZone(string zoneName)
    {
        for(auto widget : widgets_)
            widget->ActivateNoActionForZone(zoneName);
    }

    string GetZoneAlias(string zoneName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->GetAlias();
        else
            return "";
    }
    
    string GetLocalZoneAlias(string zoneName)
    {
        if(GetZoneAlias(zoneName) != "")
            return GetZoneAlias(zoneName);
        else
            return "";
            //return page_->GetZoneAlias(zoneName);
    }
    
    void AddZone(Zone* zone)
    {
        if(zones_.count(zone->GetName()) > 0)
        {
            char buffer[5000];
            snprintf(buffer, sizeof(buffer), "The Zone named \"%s\" is already defined in file\n %s\n\n The new Zone named \"%s\" defined in file\n %s\n will not be added\n\n\n\n",
                     zone->GetName().c_str(), zones_[zone->GetName()]->GetSourceFilePath().c_str(), zone->GetName().c_str(), zone->GetSourceFilePath().c_str());
            DAW::ShowConsoleMsg(buffer);
        }
        else
        {
            string zoneName = zone->GetName();
            zones_[zoneName] = zone;
            zonesInZoneFile_[zone->GetSourceFilePath()].push_back(zone);
            
            if((zoneName.compare(0, 4, "Send")) == 0)
                GetSendsActivationManager()->SetNumSendSlots(GetSendsActivationManager()->GetNumSendSlots() + 1);
            if((zoneName.compare(0, 6, "FXMenu")) == 0)
                GetFXActivationManager()->SetNumFXSlots(GetFXActivationManager()->GetNumFXSlots() + 1);
        }
    }

    void AddZoneFileLine(string fileName, string line)
    {
        zoneFileLines_[fileName].push_back(line);
    }
    
    void WidgetsGoZone(string zoneName)
    {
        for(auto widget : widgets_)
            widget->GoZone(zoneName);
    }
    
    void Deactivate(Zone* zone)
    {
        for(auto widget : widgets_)
            widget->Deactivate(zone);
    }

    void ToggleMapSends()
    {
        sendsActivationManager_->ToggleMapSends();
    }
    
    void MapSelectedTrackSendsToWidgets()
    {
        sendsActivationManager_->MapSelectedTrackSendsToWidgets(zones_);
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
               
        // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
        InitZones(zoneFolder);
        GoHome();
        ForceClearAllWidgets();
    }
    
    virtual ~Midi_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "/CSI/Surfaces/Midi/" + templateFilename_; }
    
    void SendMidiMessage(Midi_FeedbackProcessor* feedbackProcessor, MIDI_event_ex_t* midiMessage);
    void SendMidiMessage(Midi_FeedbackProcessor* feedbackProcessor, int first, int second, int third);

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
    string templateFilename_ = "";
    oscpkt::UdpSocket* const inSocket_ = nullptr;
    oscpkt::UdpSocket* const outSocket_ = nullptr;
    oscpkt::PacketReader packetReader_;
    oscpkt::PacketWriter packetWriter_;
    map<string, OSC_CSIMessageGenerator*> CSIMessageGeneratorsByOSCMessage_;
    
    void InitWidgets(string templateFilename);
    void ProcessOSCMessage(string message, double value);

public:
    OSC_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, oscpkt::UdpSocket* inSocket, oscpkt::UdpSocket* outSocket)
    : ControlSurface(CSurfIntegrator, page, name), templateFilename_(templateFilename), inSocket_(inSocket), outSocket_(outSocket)
    {
        InitWidgets(templateFilename);
              
        // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
        InitZones(zoneFolder);
        GoHome();
        ForceClearAllWidgets();
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
// For EuCon_ControlSurface
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

class MarshalledFunctionCall;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuCon_ControlSurface : public ControlSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string zoneFolder_ = "";
    int numChannels_ = 0;
    double previousPP = 0.0;
    
    map<string, EuCon_CSIMessageGenerator*> CSIMessageGeneratorsByMessage_;

    vector<Widget*> generalWidgets_;
    map<int, WidgetGroup*> channelGroups_;
    
    WDL_Mutex mutex_;
    list<MarshalledFunctionCall*> workQueue_;

    Widget* InitializeEuConWidget(CSIWidgetInfo &widgetInfo);

public:
    EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels);
    virtual ~EuCon_ControlSurface() {}
    
    virtual string GetSourceFileName() override { return "EuCon"; }
    
    virtual void InitializeEuCon() override;
    virtual void InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems) override;
    virtual void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string oscAddress, double value);
    virtual void SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string oscAddress, string value);
    virtual void SendEuConMessage(string oscAddress, string value);
    virtual void ReceiveEuConMessage(string oscAddress, double value) override;
    virtual void ReceiveEuConMessage(string oscAddress, string value) override;
    virtual void HandleExternalInput() override;
    void HandleEuConMessage(string oscAddress, double value);
    void HandleEuConMessage(string oscAddress, string value);
    virtual void UpdateTimeDisplay() override;
    virtual void ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible) override;
    
    virtual void RequestUpdate() override
    {
        for(auto widget : generalWidgets_)
            widget->RequestUpdate();
        
        for(auto [channel, group] : channelGroups_)
            group->RequestUpdate();
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
    
public:
    TrackNavigationManager(Page* page, bool followMCP, bool synchPages) : page_(page), followMCP_(followMCP), synchPages_(synchPages),
    masterTrackNavigator_(new MasterTrackNavigator(page_)), selectedTrackNavigator_(new SelectedTrackNavigator(page_)), focusedFXNavigator_(new FocusedFXNavigator(page_)) {}
    
    Page* GetPage() { return page_; }
    bool GetSynchPages() { return synchPages_; }
    bool GetScrollLink() { return scrollLink_; }
    bool GetVCAMode() { return vcaMode_; }
    int  GetNumTracks() { return DAW::CSurf_NumTracks(followMCP_); }
    Navigator* GetMasterTrackNavigator() { return masterTrackNavigator_; }
    Navigator* GetSelectedTrackNavigator() { return selectedTrackNavigator_; }
    Navigator* GetFocusedFXNavigator() { return focusedFXNavigator_; }

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
        // GAW TBD -- maybe put a ValidatePtr in here ?
        if(tracks_.size() > channelNumber + trackOffset_)
            return tracks_[channelNumber + trackOffset_];
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
                return masterTrackNavigator_->GetIsFaderTouched(); // Fader/Volume
            else
                return masterTrackNavigator_->GetIsRotaryTouched(); // Rotary/Pan
        }
        
        for(auto navigator : navigators_)
        {
            if(track == navigator->GetTrack())
            {
                if(touchedControl == 0)
                    return navigator->GetIsFaderTouched(); // Fader/Volume
                else
                    return navigator->GetIsRotaryTouched(); // Rotary/Pan
            }
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
    void UpdateEditModeWindow();
    
    bool GetShift() { return isShift_; }
    bool GetOption() { return isOption_; }
    bool GetControl() { return isControl_; }
    bool GetAlt() { return isAlt_; }
   
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
    
    bool shouldRun_ = true;
    
    int currentPageIndex_ = 0;
    bool surfaceInMonitor_ = false;
    bool surfaceOutMonitor_ = false;
    bool fxMonitor_ = false;
    
    bool oscInMonitor_ = false;
    bool oscOutMonitor_ = false;
    
    int *timeModePtr_ = nullptr;
    int *timeMode2Ptr_ = nullptr;
    int *measOffsPtr_ = nullptr;
    double *timeOffsPtr_ = nullptr;
    
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
        oscInMonitor_ = false;
        oscOutMonitor_ = false;
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
                DAW::ShowConsoleMsg(("Zone \"" + string(fxName) + "\"").c_str());
                
                for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                {
                    DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                    DAW::ShowConsoleMsg(("\n\tFXParam " + to_string(j) + " \"" + string(fxParamName)+ "\"").c_str());
                    
                    /* Uncomdent this and comment the line ablove if you want to show steo sizes
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
