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

#include "time.h"
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <functional>

#include "control_surface_integrator_Reaper.h"

#ifndef _WIN32
#include <dirent.h>
#else
#include <memory>
#include "direntWin.h"
#endif

const string Control_Surface_Integrator = "Control Surface Integrator";
const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";
const string GainReductionDB = "GainReductionDB";
const string TrackOnSelection = "TrackOnSelection";
const string Channel = "Channel";
const string ChannelEnd = "ChannelEnd";
const string MidiInMonitor = "MidiInMonitor";
const string MidiOutMonitor = "MidiOutMonitor";
const string VSTMonitor = "VSTMonitor";
const string FollowMCP = "FollowMCP";
const string RealSurface_ = "RealSurface";
const string VirtualSurface_ = "VirtualSurface";
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
const string Invert = "Invert";
const string PageToken = "Page";
const string Track = "Track";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FileSystem
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    static vector<string> GetDirectoryFilenames(const string& dir)
    {
        vector<string> filenames;
        shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
        struct dirent *dirent_ptr;
        
        if(directory_ptr == nullptr)
            return filenames;
        
        while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
            filenames.push_back(string(dirent_ptr->d_name));
        
        return filenames;
    }

    static vector<string> GetDirectoryFolderNames(const string& dir)
    {
        vector<string> folderNames;
        shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
        struct dirent *dirent_ptr;
        
        if(directory_ptr == nullptr)
            return folderNames;

        while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
            if(dirent_ptr->d_type == DT_DIR)
                folderNames.push_back(string(dirent_ptr->d_name));
        
        return folderNames;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiIOManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
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
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string role_ = "";
    string name_ = "";

protected:
    Widget(string role, string name) : role_(role), name_(name) {}

public:
    virtual ~Widget() {};
    
    string GetRole() { return role_; }
    string GetName() { return name_; }
    virtual void SetValue(int mode, double value) {}
    virtual void SetValue(string value) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_RealSurface;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_Widget : public Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Midi_RealSurface* surface_ = nullptr;
    
protected:
    MIDI_event_ex_t* lastMessageSent_ = new MIDI_event_ex_t(0, 0, 0);
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;

    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    virtual void SendMidiMessage(int first, int second, int third);

public:
    Midi_Widget(Midi_RealSurface* surface, string role, string name, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Widget(role, name), surface_(surface),  midiPressMessage_(press), midiReleaseMessage_(release) {}
    virtual ~Midi_Widget() {};
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    const string name_ = "";

    bool isBankable_ = true;
    vector<Widget*> widgets_;
    vector<Widget*> allWidgets_;
    vector<Widget*> emptyWidgets_;
    vector<vector<Widget*>> channels_;
    
    RealSurface(const string name, int numChannels, bool isBankable) : name_(name), isBankable_(isBankable)
    {
        for(int i = 0; i < numChannels; i++)
            channels_.push_back(vector<Widget*>());
    }

public:
    virtual ~RealSurface() {};
    
    string GetName() const { return name_; }
    int GetNumChannels() { return channels_.size(); }
    vector<vector<Widget*>> GetChannels() { return channels_; }
    vector<vector<Widget*>> GetBankableChannels() { return isBankable_ ? channels_ : vector<vector<Widget*>>() ; }
    vector<Widget*> & GetAllWidgets() { return allWidgets_; }

    vector<Widget*> & GetChannelWidgets(Widget* aChannelWidget)
    {
        for(int i = 0; i < GetNumChannels(); i++)
            for(int j = 0; j < channels_[i].size(); j++)
                if(channels_[i][j] == aChannelWidget)
                    return channels_[i];
        
        
        return emptyWidgets_;
    }

    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
        allWidgets_.push_back(widget);
    }
    
    void AddWidget(int channelNum, Widget* widget)
    {
        if(channelNum >= 0 && channelNum < channels_.size())
        {
            channels_[channelNum].push_back(widget);
            allWidgets_.push_back(widget);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Midi_RealSurface : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    midi_Input* midiInput_ = nullptr;
    midi_Output* midiOutput_ = nullptr;
    bool midiInMonitor_ = false;
    bool midiOutMonitor_ = false;
    map<string, Midi_Widget*> widgetsByMessage_;
    
    void ProcessMidiMessage(const MIDI_event_ex_t* evt)
    {
        // At this point we don't know how much of the message comprises the key, so try all three
        if(widgetsByMessage_.count(to_string(evt->midi_message[0])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0])]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(to_string(evt->midi_message[0]) + to_string(evt->midi_message[1])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0]) + to_string(evt->midi_message[1])]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(to_string(evt->midi_message[0]) + to_string(evt->midi_message[1]) + to_string(evt->midi_message[2])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0]) + to_string(evt->midi_message[1]) + to_string(evt->midi_message[2])]->ProcessMidiMessage(evt);
        
        if(midiInMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "IN -> %s %02x  %02x  %02x \n", GetName().c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
            DAW::ShowConsoleMsg(buffer);
        }
    }
    
public:
    Midi_RealSurface(const string name, string templateFilename, int numChannels, bool isBankable, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor);

    virtual ~Midi_RealSurface()
    {
        if (midiInput_) delete midiInput_;
        if(midiOutput_) delete midiOutput_;
    }
    
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
    
    void AddWidgetToMessageMap(string message, Midi_Widget* widget)
    {
        widgetsByMessage_[message] = widget;
    }
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage)
    {
        if(midiOutput_)
            midiOutput_->SendMsg(midiMessage, -1);
    }
    
    virtual void SendMidiMessage(int first, int second, int third)
    {
        if(midiOutput_)
            midiOutput_->Send(first, second, third, -1);
        
        if(midiOutMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "OUT -> %s %02x  %02x  %02x \n", GetName().c_str(), first, second, third);
            DAW::ShowConsoleMsg(buffer);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionContext;
class Page;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Action() {}
    
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget) {}                                     // GlobalContext
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, int commandId) {}                      // ReaperActionContext
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, MediaTrack* track) {}                  // TrackContext
    virtual void RequestUpdate(Page* page, ActionContext* actionContext, Widget* widget, MediaTrack* track, int param) {}       // TrackParamContext
    virtual void RequestUpdate(ActionContext* actionContext, Widget* widget, MediaTrack* track, int fxIndex, int paramIndex) {} // FXContext

    virtual void Do(Page* page, double value) {}                                                                                // GlobalContext / ReaperActionContext
    virtual void Do(Page* page, Widget* widget, MediaTrack* track, double value) {}                                             // TrackContext / TrackParamContext
    virtual void Do(MediaTrack* track, int fxIndex, int paramIndex, double value) {}                                            // FXContext
    virtual void Do(Page* page, RealSurface* surface, MediaTrack* track) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Action * action_ = nullptr;
    bool isInverted_ = false;

    ActionContext(Action* action, bool isInverted) : action_(action), isInverted_(isInverted) {}
    
public:
    virtual ~ActionContext() {}
    
    virtual void SetTrack(string trackGUID) {}
    virtual void SetIndex(int index) {}
    virtual void SetCyclerWidget(Widget* cyclerWidget) {}
    virtual void RequestActionUpdate(Page* page, Widget* widget) {}
    virtual void DoAction(Page* page, Widget* widget, double value) {}
    virtual void DoAction(Page* page, RealSurface* surface, MediaTrack* track) {}

    void SetWidgetValue(Widget* widget, int displayMode, double value)
    {
        isInverted_ == false ? widget->SetValue(displayMode, value) : widget->SetValue(displayMode, 1.0 - value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WidgetContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    map<string, map<string, vector<ActionContext*>>> actionContexts_;
    string component_ = Track;
    
public:
    void AddActionContext(string component, string modifiers, ActionContext* context)
    {
        actionContexts_[component][modifiers].push_back(context);
    }
    
    void RequestUpdate(Page* page, string modifiers, Widget*widget)
    {
        if(actionContexts_.count(component_) > 0 && actionContexts_[component_].count(modifiers) > 0)
            for(auto actionContext : actionContexts_[component_][modifiers])
                actionContext->RequestActionUpdate(page, widget);
    }
    
    void DoAction(Page* page, string modifiers, Widget*widget, double value)
    {
        if(actionContexts_.count(component_) > 0 && actionContexts_[component_].count(modifiers) > 0)
            for(auto actionContext : actionContexts_[component_][modifiers])
                actionContext->DoAction(page, widget, value);
    }
    
    void DoAction(Page* page, string modifiers, RealSurface* surface, MediaTrack* track)
    {
        if(actionContexts_.count(component_) > 0 && actionContexts_[component_].count(modifiers) > 0)
            for(auto actionContext : actionContexts_[component_][modifiers])
                actionContext->DoAction(page, surface, track);
    }
    
    void SetIndex(int index)
    {
        if(actionContexts_.count(component_) > 0)
            for(auto [modifier, actionContexts] : actionContexts_[component_])
                for(auto actionContext : actionContexts)
                    actionContext->SetIndex(index);
    }
    
    void SetComponentTrackContext(string component, string trackGUID)
    {
        if(actionContexts_.count(component) > 0)
            for(auto [modifier, actionContexts] : actionContexts_[component])
                for(auto actionContext : actionContexts)
                    actionContext->SetTrack(trackGUID);
    }
    
    void SetComponent(string component)
    {
        if(actionContexts_.count(component) > 0)
            component_ = component;
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
class BankableChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool isPinned_ = false;
    string trackGUID_ = "";
    vector<Widget*> widgets_;
    int colour_ = 0;
    
public:
    BankableChannel(vector<Widget*> & widgets) : widgets_(widgets) {}
    
    bool GetIsPinned() { return isPinned_; }
    string GetTrackGUID() { return trackGUID_; }
    int *GetColour() { return &colour_; }
    
    void SetIsPinned(bool pinned)
    {
        isPinned_ = pinned;
    }
    
    void SetColour(int colour)
    {
        colour_ = colour;
    }
    
    void SetTrackGUID(Page* page, string trackGUID);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";
    bool followMCP_ = true;
    bool colourTracks_ = false;
    int trackColourRedValue_ = 0;
    int trackColourGreenValue_ = 0;
    int trackColourBlueValue_ = 0;
    int trackOffset_ = 0;
    MediaTrack **previousTrackList_ = nullptr;
    int previousNumTracks_ = 0;
    vector<RealSurface*> realSurfaces_;
    vector<BankableChannel*> bankableChannels_;
    vector<MediaTrack*> touchedTracks_;
    map<Widget*, WidgetContext*> widgetContexts_;
    map <string, vector<Widget*>> fxWidgets_;
    bool currentlyRefreshingLayout_ = false;
    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;

    bool zoom_ = false;
    bool scrub_ = false;

    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
    void InitActionContexts(RealSurface* surface, string templateDirectory);
    void InitFXContexts(RealSurface* surface, string templateDirectory);

    string GetCurrentModifiers()
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

    void SetPinnedTracks()
    {
        char buffer[BUFSZ];
        
        for(int i = 0; i < bankableChannels_.size(); i++)
        {
            if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetName() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
            {
                bankableChannels_[i]->SetTrackGUID(this, buffer);
                bankableChannels_[i]->SetIsPinned(true);
            }
        }
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
    
    void MapTrackToWidgets(RealSurface* surface, MediaTrack* track)
    {
        for(auto channel : surface->GetChannels())
            for(auto widget : channel)
                if(widgetContexts_.count(widget) > 0)
                    widgetContexts_[widget]->SetComponentTrackContext(Track, DAW::GetTrackGUIDAsString(track, followMCP_));
    }
    
    void MapFXToWidgets(MediaTrack* track)
    {
        char fxName[BUFSZ];
        
        DeleteFXWindows();
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            
            if(fxWidgets_.count(fxName) > 0)
            {
                for(auto widget : fxWidgets_[fxName])
                {
                    if(widgetContexts_.count(widget) > 0)
                    {
                        widgetContexts_[widget]->SetComponentTrackContext(fxName, DAW::GetTrackGUIDAsString(track, followMCP_));
                        widgetContexts_[widget]->SetComponent(fxName);
                        widgetContexts_[widget]->SetIndex(i);
                    }
                }
                
                AddFXWindow(FXWindow(track, i));
            }
        }
        
        OpenFXWindows();
    }
    
    void UnmapWidgetsFromTrack(RealSurface* surface, MediaTrack* track)
    {
        for(auto channel : surface->GetChannels())
            for(auto widget : channel)
                if(widgetContexts_.count(widget) > 0)
                {
                    widget->SetValue(0, 0.0);
                    widgetContexts_[widget]->SetComponentTrackContext(Track, "");
                }
    }
    
    void UnmapWidgetsFromFX(MediaTrack* track)
    {
        char fxName[BUFSZ];
        
        DeleteFXWindows();
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            
            if(fxWidgets_.count(fxName) > 0)
            {
                for(auto widget : fxWidgets_[fxName])
                {
                    if(widgetContexts_.count(widget) > 0)
                    {
                        widget->SetValue(0, 0.0);
                        widgetContexts_[widget]->SetComponentTrackContext(fxName, "");
                        widgetContexts_[widget]->SetComponent(Track);
                    }
                }
            }
        }
    }

public:
    Page(string name, bool followMCP, bool colourTracks, int red, int green, int blue) : name_(name), followMCP_(followMCP), colourTracks_(colourTracks), trackColourRedValue_(red), trackColourGreenValue_(green), trackColourBlueValue_(blue) {}
    string GetName() { return name_; }
    int GetFXParamIndex(MediaTrack* track, Widget* widget, int fxIndex, string fxParamName);
    void TrackFXListChanged(MediaTrack* track);
    
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }
    
    void SetZoom(bool value)
    {
        zoom_ = value;
    }
    
    void SetScrub(bool value)
    {
        scrub_ = value;
    }
    
    void LeavePage()
    {
        if(colourTracks_)
        {
            DAW::PreventUIRefresh(1);
            // reset track colors
            for(auto* channel : bankableChannels_)
                if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                    DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", channel->GetColour());
            DAW::PreventUIRefresh(-1);
        }
    }
    
    WidgetContext* GetWidgetContext(Widget* widget)
    {
        if(widgetContexts_.count(widget) > 0)
            return widgetContexts_[widget];
        else
            return nullptr;
    }
    
    void RequestUpdate()
    {
        for(auto [widget, widgetContext] : widgetContexts_)
            widgetContext->RequestUpdate(this, GetCurrentModifiers(), widget);
    }
   
    void DoAction(Widget* widget, double value)
    {
        if(widget->GetRole() == Shift)
            SetShift(value);
        else if(widget->GetRole() == Option)
            SetOption(value);
        else if(widget->GetRole() == Control)
            SetControl(value);
        else if(widget->GetRole() == Alt)
            SetAlt(value);
        else if(widgetContexts_.count(widget) > 0)
            widgetContexts_[widget]->DoAction(this, GetCurrentModifiers(), widget, value);
    }
    
    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
    }

    void SetShift(bool value)
    {
        shift_ = value;
    }
    
    void SetOption(bool value)
    {
        option_ = value;
    }
    
    void SetControl(bool value)
    {
        control_ = value;
    }
    
    void SetAlt(bool value)
    {
        alt_ = value;
    }
    
    void AddSurface(RealSurface* surface, string actionTemplateDirectory, string fxTemplateDirectory)
    {
        string resourcePath(DAW::GetResourcePath());
        resourcePath += "/CSI/";
   
        InitActionContexts(surface, resourcePath + "axt/" + actionTemplateDirectory);
        InitFXContexts(surface, resourcePath + "fxt/" + fxTemplateDirectory);
        for(auto channel : surface->GetBankableChannels())
            bankableChannels_.push_back(new BankableChannel(channel));
        realSurfaces_.push_back(surface);
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
    
    void MapFXToWidgets(RealSurface* surface, MediaTrack* track)
    {
        MapFXToWidgets(track);
    }
    
    void UnmapWidgetsFromFX(RealSurface* surface, MediaTrack* track)
    {
        UnmapWidgetsFromFX(track);
    }
    
    void MapTrackAndFXToWidgets(RealSurface* surface, MediaTrack* track)
    {
        MapTrackToWidgets(surface, track);
        MapFXToWidgets(track);
    }
    
    void UnmapWidgetsFromTrackAndFX(RealSurface* surface, MediaTrack* track)
    {
        UnmapWidgetsFromTrack(surface, track);
        UnmapWidgetsFromFX(track);
    }
    
    void Init()
    {
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_) && i < bankableChannels_.size(); i++)
        {
            bankableChannels_[i]->SetTrackGUID(this, DAW::GetTrackGUIDAsString(i, followMCP_));
            bankableChannels_[i]->SetColour(GetTrackColor(DAW::CSurf_TrackFromID(i, followMCP_)));
        }

        SetPinnedTracks();
    }
    
    void OnTrackSelection(MediaTrack* track)
    {
        string trackGUID = DAW::GetTrackGUIDAsString(track, followMCP_);
        
        for(auto surface : realSurfaces_)
            for(auto widget : surface->GetAllWidgets())
                if(widget->GetRole() == "TrackOnSelection")
                    if(widgetContexts_.count(widget) > 0)
                        widgetContexts_[widget]->DoAction(this, GetCurrentModifiers(), surface, track);
    }
      
    void PinSelectedTracks()
    {
        BankableChannel* channel = nullptr;
        
        for(int i = 0; i < bankableChannels_.size(); i++)
        {
            channel = bankableChannels_[i];
            
            MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_);
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsPinned(true);
                DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetName() + "Channel" + to_string(i + 1)).c_str(), channel->GetTrackGUID().c_str());
                DAW::MarkProjectDirty(nullptr);
            }
        }
    }
    
    void UnpinSelectedTracks()
    {
        BankableChannel* channel = nullptr;
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
                if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetName() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
                {
                    DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetName() + "Channel" + to_string(i + 1)).c_str(), "");
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
        
        int currentNumTracks = DAW::CSurf_NumTracks(followMCP_);

        if(currentNumTracks != previousNumTracks_)
        {
            if(previousTrackList_ != nullptr)
                delete[] previousTrackList_;

            previousNumTracks_ = currentNumTracks;
            previousTrackList_ = new MediaTrack* [currentNumTracks];
            
            for(int i = 0; i < currentNumTracks; i++)
                previousTrackList_[i] = DAW::CSurf_TrackFromID(i, followMCP_);
            
            DAW::ClearCache();
            
            for(auto channel : bankableChannels_)
            {
                if(channel->GetIsPinned())
                {
                    if( DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_) == nullptr) // track has been removed
                    {
                        channel->SetIsPinned(false);
                        channel->SetTrackGUID(this, "");
                    }
                }
            }
            
            return true;
        }
        else if(currentNumTracks == previousNumTracks_)
        {
            MediaTrack **currentTrackList = new MediaTrack* [currentNumTracks];
            for(int i = 0; i < currentNumTracks; i++)
                currentTrackList[i] = DAW::CSurf_TrackFromID(i, followMCP_);

            if(memcmp(previousTrackList_, currentTrackList, currentNumTracks * sizeof(MediaTrack*)))
            {
                if(previousTrackList_ != nullptr)
                    delete[] previousTrackList_;
                previousTrackList_ = currentTrackList;
                
                DAW::ClearCache();
                return true;
            }
            else
            {
                delete[]currentTrackList;
                return false;
            }
        }

        return false;
        
        
        
  
        
        
        /*
        
        vector<string> visibleTrackGUIDs;
        
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_); i++)
            visibleTrackGUIDs.push_back(DAW::GetTrackGUIDAsString(i, followMCP_));
        
        int currentOffset = trackOffset_;
        bool shouldRefreshLayout = false;
        
        for(int i = 0; i < bankableChannels_.size() && currentOffset < DAW::CSurf_NumTracks(followMCP_); i++)
        {
            if(bankableChannels_[i]->GetIsPinned())
            {
                if(bankableChannels_[i]->GetTrackGUID() == "") // track has been removed
                {
                    bankableChannels_[i]->SetIsPinned(false); // unlock this, since there is no longer a track to lock to
                    shouldRefreshLayout = true;
                    DAW::ClearCache();
                    break;
                }
                else
                {
                    currentOffset++; // track exists, move on
                }
            }
            
            else if(bankableChannels_[i]->GetTrackGUID() == visibleTrackGUIDs[currentOffset])
            {
                currentOffset++; // track exists and positions are in synch
            }
            else
            {
                shouldRefreshLayout = true;
                DAW::ClearCache();
                break;
            }
        }
        
        return shouldRefreshLayout;
         */
    }
    
    void AdjustTrackBank(int stride)
    {
        int previousTrackOffset = trackOffset_;
        
        trackOffset_ += stride;
        
        int bottom = 1 - bankableChannels_.size() + GetNumLockedTracks();
        
        if(trackOffset_ <  bottom)
            trackOffset_ =  bottom;
        
        int top = DAW::CSurf_NumTracks(followMCP_) - 1;
        
        if(trackOffset_ >  top)
            trackOffset_ = top;
        
        // Jump over any pinned channels and invisible tracks
        vector<string> pinnedChannels;
        for(auto* channel : bankableChannels_)
            if(channel->GetIsPinned())
                pinnedChannels.push_back(channel->GetTrackGUID());
        
        bool skipThisChannel = false;
        
        while(trackOffset_ >= 0 && trackOffset_ < DAW::CSurf_NumTracks(followMCP_))
        {
            string trackGUID = DAW::GetTrackGUIDAsString(trackOffset_, followMCP_);
            
            for(auto pinnedChannel : pinnedChannels)
                if(pinnedChannel == trackGUID)
                {
                    skipThisChannel = true;
                    previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
                    break;
                }
            
            if( ! IsTrackVisible(DAW::CSurf_TrackFromID(trackOffset_, followMCP_)))
            {
                skipThisChannel = true;
                previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
            }
            
            if(skipThisChannel)
            {
                skipThisChannel = false;
                continue;
            }
            else
                break;
        }
        
        RefreshLayout();
    }
    
    void RefreshLayout()
    {
        currentlyRefreshingLayout_ = true;
        
        vector<MediaTrack*> visibleTracks;
        
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_); i++)
            visibleTracks.push_back(DAW::CSurf_TrackFromID(i, followMCP_));
        
        // Layout channel GUIDs
        int offset = trackOffset_;
        vector<string> movableChannelLayout;

        for(auto* channel : bankableChannels_)
        {
            if(offset < 0)
            {
                movableChannelLayout.push_back("");
                offset++;
            }
            else if(offset >= DAW::CSurf_NumTracks(followMCP_))
            {
                movableChannelLayout.push_back("");
            }
            else
            {
                if(! channel->GetIsPinned())
                    movableChannelLayout.push_back(DAW::GetTrackGUIDAsString(visibleTracks[offset++], followMCP_));
            }
        }
        
        // Merge in the Pinned Channels
        offset = 0;
        vector<string> channelLayout;
        
        for(auto* channel : bankableChannels_)
        {
            if(channel->GetIsPinned())
                channelLayout.push_back(channel->GetTrackGUID());
            else
                channelLayout.push_back(movableChannelLayout[offset++]);
        }

        if(colourTracks_)
        {
            DAW::PreventUIRefresh(1);
            // reset track colors
            for(auto* channel : bankableChannels_)
                if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                    DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", channel->GetColour());
        }
        
        // Apply new layout
        offset = 0;
        for(auto* channel : bankableChannels_)
            channel->SetTrackGUID(this, channelLayout[offset++]);
        
        if(colourTracks_)
        {
            // save current track colours
            for(auto* channel : bankableChannels_)
                if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                    channel->SetColour(DAW::GetTrackColor(track));
            
            // color tracks
            int color = DAW::ColorToNative(trackColourRedValue_, trackColourGreenValue_, trackColourBlueValue_) | 0x1000000;
            for(auto* channel : bankableChannels_)
                if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                    DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", &color);
            DAW::PreventUIRefresh(-1);
        }
        
        currentlyRefreshingLayout_ = false;
    }
    
private:
    int GetNumLockedTracks()
    {
        int numLockedTracks = 0;
        
        for(auto* channel : bankableChannels_)
            if(channel->GetIsPinned())
                numLockedTracks++;
        
        return numLockedTracks;
    }
    
    bool IsTrackVisible(MediaTrack* track)
    {
        if(DAW::GetMediaTrackInfo_Value(track, "IP_TRACKNUMBER") == -1) // Master
        {
            if(followMCP_ && DAW::GetMasterTrackVisibility() < 2)
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
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MidiIOManager* midiIOManager_ = nullptr;
    map<string, Action*> actions_;
    map<string , function<ActionContext*(vector<string>, bool isInverted)>> actionContexts_;
    vector <Page*> pages_;
    vector<Midi_RealSurface*> midi_realSurfaces_;
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
    
    void Init();

    MidiIOManager* GetMidiIOManager() { return midiIOManager_; }
    bool GetVSTMonitor() { return VSTMonitor_; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    map<string, map<string, int>> & GetFXParamIndices() { return fxParamIndices_; }
    
    Action* GetAction(string actionName)
    {
        if(actions_.count(actionName) > 0)
            return actions_[actionName];
        
        return nullptr;
    }
    
    ActionContext* GetActionContext(vector<string> params, bool isInverted)
    {
        if(actionContexts_.count(params[0]) > 0)
            return actionContexts_[params[0]](params, isInverted);
        
        return nullptr;
    }
    
    ActionContext* GetFXActionContext(vector<string> params, bool isInverted)
    {
        if(actionContexts_.count(params[0]) > 0)
            return actionContexts_[params[0]](params, isInverted);
        
        return nullptr;
    }
    
    void DoAction(Widget* widget, double value)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->DoAction(widget, value);
    }
    
    void OnTrackSelection(MediaTrack *track)
    {
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->OnTrackSelection(track);
    }
    
    void Run()
    {
        for(auto surface : midi_realSurfaces_)
            surface->HandleMidiInput();
        
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->RequestUpdate();
    }
    
    void NextPage()
    {
        if(pages_.size() > 0)
        {
            pages_[currentPageIndex_]->LeavePage();
            currentPageIndex_ = currentPageIndex_ == pages_.size() - 1 ? 0 : ++currentPageIndex_;
            //pages_[currentPageIndex_]->SetContext();
            pages_[currentPageIndex_]->RefreshLayout();
        }
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
