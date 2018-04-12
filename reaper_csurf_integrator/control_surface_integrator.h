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

#include "control_surface_integrator_Reaper.h"

#ifndef _WIN32
#include <dirent.h>
#else
#include <memory>
#include "direntWin.h"
#endif

const string Control_Surface_Integrator = "Control Surface Integrator";
const string ControlSurfaceIntegrator = "ControlSurfaceIntegrator";
const string DefaultGUID = "Control Surface Integrator GUID";
const string GainReductionDB = "GainReductionDB";
const string TrackOnSelection = "TrackOnSelection";
const string Channel = "Channel";
const string ChannelEnd = "ChannelEnd";
const string MidiInMonitor = "MidiInMonitor";
const string MidiOutMonitor = "MidiOutMonitor";
const string VSTMonitor = "VSTMonitor";
const string FollowMCP = "FollowMCP";
const string RealSurface_ = "RealSurface";
const string Shift = "Shift";
const string Option = "Option";
const string Control = "Control";
const string Alt = "Alt";
const string Page_ = "Page";
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
class Midi_RealSurface;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string role_ = "";
    
protected:
    Widget(string role) : role_(role) {}

public:
    virtual ~Widget() {};
    
    string GetRole() { return role_; }
    virtual Midi_RealSurface* GetSurface() { return nullptr; }

    void RequestUpdate();
    virtual void SetValue(double value) {}
    virtual void SetValue(string value) {}
};

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

public:
    Midi_Widget(Midi_RealSurface* surface, string role, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Widget(role), surface_(surface),  midiPressMessage_(press), midiReleaseMessage_(release) {}
    virtual ~Midi_Widget() {};
    
    Midi_RealSurface* GetSurface() override { return surface_; }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage);
    virtual void SendMidiMessage(int first, int second, int third);
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
    vector<vector<Widget*>> channels_;
    map<Widget*, string> suffixes_;
    
    RealSurface(const string name, string templateFilename, int numChannels, bool isBankable) : name_(name), isBankable_(isBankable)
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
    bool IsBankable() { return isBankable_; }
    vector<Widget*> & GetAllWidgets() { return allWidgets_; }
    
    string GetWidgetSuffix(Widget* widget)
    {
        if(suffixes_.count(widget) > 0)
            return suffixes_[widget];
        
        return "";
    }
    
    void RequestUpdate()
    {
        for(auto widget : widgets_)
            widget->RequestUpdate();
        
        for(auto channel : channels_)
            for(auto widget : channel)
                widget->RequestUpdate();
    }
    
    void AddWidget(Widget* widget)
    {
        widgets_.push_back(widget);
        allWidgets_.push_back(widget);
        suffixes_[widget] = "";
    }
    
    void AddWidget(int channelNum, Widget* widget)
    {
        if(channelNum >= 0 && channelNum < channels_.size())
        {
            channels_[channelNum].push_back(widget);
            allWidgets_.push_back(widget);
            suffixes_[widget] = to_string(channelNum + 1);
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
class BankableChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    bool isPinned_ = false;
    string GUID_ = "";
    vector<Widget*> widgets_;
    
public:
    BankableChannel(vector<Widget*> & widgets) : widgets_(widgets) {}
    
    vector<Widget*> & GetWidgets() { return widgets_; }
    bool GetIsPinned() { return isPinned_; }
    string GetGUID() { return GUID_; }
    
    void SetIsPinned(bool pinned)
    {
        isPinned_ = pinned;
    }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Action() {}
    
    virtual void RequestUpdate(Widget* widget, Page* page, vector<string> & params) {}
    virtual void Do(Widget* widget, Page* page, vector<string> & params, double value) {}
    virtual void Do(RealSurface* surface, MediaTrack* track, Page* page) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FXWindow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FXWindow
{
    MediaTrack* track = nullptr;;
    string fxGUID = "";
    
    FXWindow(MediaTrack* aTrack, string anFxGUID) : track(aTrack), fxGUID(anFxGUID) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Page
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    enum class WidgetMode {
        Track,
        FX
    };
    
    string name_ = "";
    bool followMCP_ = true;
    int trackOffset_ = 0;
    int currentNumTracks_ = 0;
    vector<RealSurface*> realSurfaces_;
    vector<BankableChannel*> bankableChannels_;
    vector<MediaTrack*> touchedTracks_;
    map<Widget*, string> widgetTrackGUIDs_;
    map<Widget*, WidgetMode> widgetModes_;
    map<string, Widget*> widgetsByName_;
    map<Widget*, vector<string>> widgetParamBundle_;
    
    map<string, map<string, vector<vector<string>>>> actionTemplates_;
    map<string, map<string, map<string, vector<string>>>> fxTemplates_;

    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;
    map<Widget*, string> widgetFXGUIDs_;

    bool zoom_ = false;
    bool scrub_ = false;

    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;
    
    string CurrentModifers(Widget* widget)
    {
        string modifiers = "";
     
        string role = widget->GetRole();
        if(role == Shift || role == Option || role == Control || role == Alt)
            return modifiers;
        
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
                bankableChannels_[i]->SetGUID(buffer);
                bankableChannels_[i]->SetIsPinned(true);
            }
        }
    }

    bool IsShowFXWindows() { return showFXWindows_; }

    void AddFXWindow(FXWindow fxWindow)
    {
        openFXWindows_.push_back(fxWindow);
    }
    
    void OpenFXWindows()
    {
        if(showFXWindows_)
            for(auto fxWindow : openFXWindows_)
                DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 3);
    }
    
    void CloseFXWindows()
    {
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 2);
    }
    
    void DeleteFXWindows()
    {
        CloseFXWindows();
        openFXWindows_.clear();
    }
    
public:
    Page(string name, bool followMCP) : name_(name), followMCP_(followMCP) {}
    string GetName() { return name_; }
    
    void InitActionTemplates(RealSurface* surface, string templateDirectory);
    void InitFXTemplates(RealSurface* surface, string templateDirectory);
    void OnTrackSelection(MediaTrack* track);    
    void TrackFXListChanged(MediaTrack* track);

    void Init()
    {
        currentNumTracks_ = DAW::CSurf_NumTracks(followMCP_);
        
        for(auto * surface : realSurfaces_)
        {
            for(auto * widget : surface->GetAllWidgets())
            {
                widgetsByName_[widget->GetRole() + surface->GetWidgetSuffix(widget)] = widget;
                widgetModes_[widget] = WidgetMode::Track; // Set to Track mode at startup
            }
        }
        
        // Set the initial Widget / Track contexts
        for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_) && i < bankableChannels_.size(); i++)
        {
            string trackGUID = DAW::GetTrackGUIDAsString(i, followMCP_);
            bankableChannels_[i]->SetGUID(trackGUID);
            for(auto widget : bankableChannels_[i]->GetWidgets())
                widgetTrackGUIDs_[widget] = trackGUID;
        }
        
        SetPinnedTracks();
    }
    
    // Widgets -> Actions -- this is the grand switchboard that does all the heavy lifting wrt routing and context
    void RequestActionUpdate(Widget* widget);
    void DoAction(Widget* widget, double value);
    
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
   
        InitActionTemplates(surface, resourcePath + "axt/" + actionTemplateDirectory);
        InitFXTemplates(surface, resourcePath + "fxt/" + fxTemplateDirectory);
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

    MediaTrack* GetTrack(Widget* widget)
    {
        if(widgetTrackGUIDs_.count(widget) > 0)
            return DAW::GetTrackFromGUID(widgetTrackGUIDs_[widget], followMCP_);

        return nullptr;
    }
    
    int GetFXIndex(Widget* widget)
    {
        if(MediaTrack* track = GetTrack(widget))
        {
            char fxGUID[BUFSZ];
            
            for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
            {
                DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
                if(widgetFXGUIDs_.count(widget) > 0 && widgetFXGUIDs_[widget] == fxGUID)
                {
                    return i;
                }
            }
        }

        return 0;
    }
    
    int GetFXParamIndex(Widget* widget, MediaTrack* track, int fxIndex)
    {
        char fxName[BUFSZ];
        char fxParamName[BUFSZ];
        RealSurface* surface = widget->GetSurface();
        DAW::TrackFX_GetFXName(track, fxIndex, fxName, sizeof(fxName));
        string widgetName = widget->GetRole() + surface->GetWidgetSuffix(widget);
        
        if(fxTemplates_.count(surface->GetName()) > 0  && fxTemplates_[surface->GetName()].count(fxName) > 0 && fxTemplates_[surface->GetName()][fxName].count(widgetName) > 0)
        {
            for(int i = 0; i < DAW::TrackFX_GetNumParams(track, fxIndex); i++)
            {
                DAW::TrackFX_GetParamName(track, fxIndex, i, fxParamName, sizeof(fxParamName));
                if(fxTemplates_[surface->GetName()][fxName][widgetName][1] == fxParamName)
                {
                    return i;
                }
            }
        }

        return 0;
    }
    
    void UnmapWidgetsFromTrack(RealSurface* surface, MediaTrack* track)
    {
        int blah = 0;
        /*
         if(zones_.count(zoneName) > 0)
         zones_[zoneName]->UnmapWidgetsFromTrack(track, surfaceName);
         */
    }

    
    void MapTrackAndFXToWidgets(RealSurface* surface, MediaTrack* track)
    {
        MapTrackToWidgets(surface, track);
        MapFXToWidgets(surface, track);
    }
    
    void MapTrackToWidgets(RealSurface* surface, MediaTrack* track)
    {
        string trackGUID = DAW::GetTrackGUIDAsString(track, followMCP_);

        for(auto channel : surface->GetChannels())
            for(auto widget : channel)
                widgetTrackGUIDs_[widget] = trackGUID;
    }

    void MapFXToWidgets(RealSurface* surface, MediaTrack* track)
    {
        char fxName[BUFSZ];
        char fxGUID[BUFSZ];
        
        DeleteFXWindows();
        
        string trackGUID = DAW::GetTrackGUIDAsString(track, followMCP_);
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            
            if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
            {
                for(auto [widgetName, paramBundle] : fxTemplates_[surface->GetName()][fxName])
                {
                    if(widgetsByName_.count(widgetName) > 0)
                    {
                        Widget* widget = widgetsByName_[widgetName];
                        widgetTrackGUIDs_[widget] = trackGUID;
                        widgetFXGUIDs_[widget] = fxGUID;
                        widgetModes_[widget] = WidgetMode::FX;
                        widgetParamBundle_[widget] = paramBundle;
                    }
                }
                
                AddFXWindow(FXWindow(track, fxGUID));
            }
        }
        
        OpenFXWindows();
    }
    
    void UnmapWidgetsFromFX(MediaTrack* track)
    {
        for(auto [widget, mode] : widgetModes_)
            mode = WidgetMode::Track;
    }
    
    void PinSelectedTracks()
    {
        BankableChannel* channel = nullptr;
        
        for(int i = 0; i < bankableChannels_.size(); i++)
        {
            channel = bankableChannels_[i];
            
            MediaTrack* track = DAW::GetTrackFromGUID(channel->GetGUID(), followMCP_);
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsPinned(true);
                DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetName() + "Channel" + to_string(i + 1)).c_str(), channel->GetGUID().c_str());
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
            
            MediaTrack* track = DAW::GetTrackFromGUID(channel->GetGUID(), followMCP_);
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
    }
    
    void TrackListChanged()
    {
        if(currentNumTracks_ != DAW::CSurf_NumTracks(followMCP_))
        {
            DAW::ClearCache();
            currentNumTracks_ = DAW::CSurf_NumTracks(followMCP_);
        }
        
        int currentOffset = trackOffset_;
        bool shouldRefreshLayout = false;
        
        for(int i = 0; i < bankableChannels_.size() && currentOffset < DAW::CSurf_NumTracks(followMCP_); i++)
        {
            if(bankableChannels_[i]->GetIsPinned())
            {
                if(DAW::GetTrackFromGUID(bankableChannels_[i]->GetGUID(), followMCP_) == nullptr) // track has been removed
                {
                    bankableChannels_[i]->SetIsPinned(false); // unlock this, since there is no longer a track to lock to
                    shouldRefreshLayout = true;
                    break;
                }
                else
                {
                    currentOffset++; // track exists, move on
                }
            }
            
            else if(bankableChannels_[i]->GetGUID() == GetNextVisibleTrackGUID(currentOffset))
            {
                currentOffset++; // track exists and positions are in synch
            }
            else
            {
                shouldRefreshLayout = true;
                break;
            }
        }
        
        if(shouldRefreshLayout)
            RefreshLayout();
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
                pinnedChannels.push_back(channel->GetGUID());
        
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
        vector<string> pinnedChannelLayout;
        vector<string> pinnedChannels;
        vector<string> movableChannelLayout;
        vector<string> channelLayout;
        
        // Layout locked channel GUIDs
        for(auto* channel : bankableChannels_)
        {
            if(channel->GetIsPinned())
            {
                pinnedChannelLayout.push_back(channel->GetGUID());
                pinnedChannels.push_back(channel->GetGUID());
            }
            else
                pinnedChannelLayout.push_back("");
        }
        
        // Layout channel GUIDs
        int offset = trackOffset_;
        
        for(int i = 0; i < pinnedChannelLayout.size(); i++)
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
                movableChannelLayout.push_back(GetNextVisibleTrackGUID(offset));
                offset++;
            }
        }
        
        // Remove the locked GUIDs
        for(int i = 0; i < pinnedChannels.size(); i++)
        {
            auto iter = find(movableChannelLayout.begin(), movableChannelLayout.end(), pinnedChannels[i]);
            if(iter != movableChannelLayout.end())
            {
                movableChannelLayout.erase(iter);
            }
        }
        
        // Merge the layouts
        offset = 0;
        for(int i = 0; i < pinnedChannelLayout.size(); i++)
        {
            if(pinnedChannelLayout[i] != "")
                channelLayout.push_back(pinnedChannelLayout[i]);
            else
                channelLayout.push_back(movableChannelLayout[offset++]);
        }
        
        // Apply new layout
        offset = 0;
        for(auto* channel : bankableChannels_)
        {
            for(auto widget : channel->GetWidgets())
                widgetTrackGUIDs_[widget] = channelLayout[offset];
            channel->SetGUID(channelLayout[offset++]);
        }
    }
    
    int GetNumLockedTracks()
    {
        int numLockedTracks = 0;
        
        for(auto* channel : bankableChannels_)
            if(channel->GetIsPinned())
                numLockedTracks++;
        
        return numLockedTracks;
    }
    
    string GetNextVisibleTrackGUID(int & offset)
    {
        while(! IsTrackVisible(DAW::CSurf_TrackFromID(offset, followMCP_)) && offset < DAW::CSurf_NumTracks(followMCP_))
            offset++;
        
        if(offset >= DAW::CSurf_NumTracks(followMCP_))
            return "";
        else
            return DAW::GetTrackGUIDAsString(offset, followMCP_);
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
    vector <Page*> pages_;
    vector<Midi_RealSurface*> midi_realSurfaces_;
    
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
        midiIOManager_ = new MidiIOManager();
    }
    
    void Init();
    
    MidiIOManager* GetMidiIOManager() { return midiIOManager_; }
    bool GetVSTMonitor() { return VSTMonitor_; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    Action* GetAction(string actionName)
    {
        if(actions_.count(actionName) > 0)
            return actions_[actionName];
        
        return nullptr;
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
        for(auto surface : midi_realSurfaces_)
            surface->RequestUpdate();
    }
    
    void NextPage()
    {
        if(pages_.size() > 0)
        {
            currentPageIndex_ = currentPageIndex_ == pages_.size() - 1 ? 0 : ++currentPageIndex_;
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
            page->TrackListChanged();
    }
    
    void TrackFXListChanged(MediaTrack* trackid)
    {
        for(auto & page : pages_)
            page->TrackFXListChanged(trackid);
    }
    
    // Widgets -> Actions
    void RequestActionUpdate(Widget* widget) { pages_[currentPageIndex_]->RequestActionUpdate(widget); }
    void DoAction(Widget* widget, double value) { pages_[currentPageIndex_]->DoAction(widget, value); }
};





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const string Layer_ = "Layer";
const string Zone_ = "Zone";
const string VirtualSurface_ = "VirtualSurface";



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TemplateEntry
{
    string widgetRole;
    vector<string> params;
    
    TemplateEntry(string aWidgetRole, vector<string> aParamsCollcrion) : widgetRole(aWidgetRole), params(aParamsCollcrion) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Template
{
private:
    string name;
    vector<TemplateEntry> entries_;
    
public:
    Template(string aName) : name(aName) {}
    
    string GetName() { return name; }
    vector<TemplateEntry>& GetTemplateEntries() { return entries_; }
    
    void AddEntry(string widgetRole, vector<string> params)
    {
        entries_.push_back(TemplateEntry(widgetRole, params));
    }
};








/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Layer;
class Zone;
class OldRealSurface;
class RealSurfaceChannel;
class CSurfManager;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OldAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Layer* layer_ = nullptr;
    Layer* GetLayer() { return layer_; }
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) {}
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, string value) {}
    
public:
    OldAction(Layer* layer) : layer_(layer) {}
    virtual ~OldAction() {}
    
    virtual int GetDisplayMode() { return 0; }
    virtual double GetCurrentNormalizedValue (string groupName, string surfaceName, string widgetName) { return 0.0; }
    
    virtual void AddAction(OldAction* action) {}
    virtual void Update(string zoneName, string surfaceName, string widgetName) {}
    virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) {}
    virtual void Cycle(string zoneName, string surfaceName, string widgetName) {}
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OldMidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string GUID_ = DefaultGUID;
    OldRealSurface* realSurface_ = nullptr;
    string suffix_= "";
    string name_ = "";
    MIDI_event_ex_t* midiPressMessage_ = nullptr;
    MIDI_event_ex_t* midiReleaseMessage_ = nullptr;
    
protected:
    OldRealSurface* GetRealSurface() { return realSurface_; }
    MIDI_event_ex_t* GetMidiReleaseMessage() { return midiReleaseMessage_; }
    MIDI_event_ex_t* GetMidiPressMessage() { return midiPressMessage_; }
    
public:
    OldMidiWidget(OldRealSurface* surface, string name, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : realSurface_(surface), name_(name),  midiPressMessage_(press), midiReleaseMessage_(release) {}
    virtual ~OldMidiWidget() {};
    
    string GetGUID() { return GUID_; }
    virtual double GetMinDB() { return 0.0; }
    virtual double GetMaxDB() { return 0.0; }
    string GetName() { return name_ + suffix_; }
    string GetActionName() { return name_; }

    void SetSuffix(string suffix)
    {
        suffix_ = suffix;
    }
    
    void SetGUID(string GUID)
    {
        GUID_ = GUID;
        
        if(GUID_ == "")
        {
            SetValueToZero();
            ForceUpdate();
        }
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) {}
    virtual void AddToRealSurface(OldRealSurface* surface);
    void Update();
    void ForceUpdate();
    virtual void SetValue(double value) {}
    virtual void SetValue(double value, int displaymode) {}
    virtual void SetValue(string value) {}
    virtual void SetValueToZero() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OldRealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    const string name_ = "";
    string templateFilename_ = "";
    Zone* zone_ = nullptr;
    bool isBankable_ = true;
    
    vector<RealSurfaceChannel*> channels_;
    vector<RealSurfaceChannel*> emptyChannels_;
    map<string, OldMidiWidget*> widgetsByName_;
    map<string, OldMidiWidget*> widgetsByMessage_;
    map<string, string> remappedFXWidgets_;
    
    bool zoom_ = false;
    bool scrub_ = false;
    
    OldRealSurface(const string name, string templateFilename, int numChannels, bool isBankable_);

public:
    virtual ~OldRealSurface() {};
    
    const string GetName() const { return name_; }
    string GetTemplateFilename() const { return templateFilename_; }
    Zone* GetZone() { return zone_; }
    vector<RealSurfaceChannel*> & GetChannels() { return channels_; }
    bool IsZoom() { return zoom_; }
    bool IsScrub() { return scrub_; }
    
    /*
    string GetWidgetGUID(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetGUID();
        
        return "";
    }
     */
    
    vector<RealSurfaceChannel*> & GetBankableChannels()
    {
        if(isBankable_)
            return GetChannels();
        else
            return emptyChannels_;
    }
    
    //void AddAction(string actionAddress, OldAction* action);
    void MapTrackToWidgets(MediaTrack *track);
    void UnmapWidgetsFromTrack(MediaTrack *track);
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) {}
    virtual void SendMidiMessage(int first, int second, int third) {}
    
    virtual void RunAndUpdate() {}

    void UnmapFXFromWidgets(MediaTrack *track)
    {
        for(auto [widgetName, GUID] : remappedFXWidgets_)
            if(widgetsByName_.count(widgetName) > 0)
                widgetsByName_[widgetName]->SetGUID(GUID);
        
        remappedFXWidgets_.clear();
    }
    
    void SetZone(Zone* zone)
    {
        zone_ = zone;
    }
    
    void AddWidget(OldMidiWidget* widget)
    {
        widget->AddToRealSurface(this);
    }
    
    void AddWidgetToNameMap(string name, OldMidiWidget* widget)
    {
        widgetsByName_[name] = widget;
    }
    
    void AddWidgetToMessageMap(string message, OldMidiWidget* widget)
    {
        widgetsByMessage_[message] = widget;
    }

    void SetWidgetGUID(string widgetName, string GUID)
    {
        if(remappedFXWidgets_.count(widgetName) > 0)
            remappedFXWidgets_[widgetName] = GUID;
        else if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetGUID(GUID);
    }
    
    void SetWidgetFXGUID(string widgetName, string GUID)
    {
        if(widgetsByName_.count(widgetName) > 0)
        {
            remappedFXWidgets_[widgetName] = widgetsByName_[widgetName]->GetGUID();
            widgetsByName_[widgetName]->SetGUID(GUID);
        }
    }
    
    void SetZoom(bool value)
    {
        zoom_ = value;
        ForceUpdateWidgets();
    }
    
    void SetScrub(bool value)
    {
        scrub_ = value;
        ForceUpdateWidgets();
    }
    
    // to Widgets ->
    virtual void UpdateWidgets()
    {
        for(auto const& [name, widget] : widgetsByName_ )
            widget->Update();
    }

    virtual void ForceUpdateWidgets()
    {
        for(auto const& [name, widget] : widgetsByName_ )
            widget->ForceUpdate();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string surfaceName, string actionName, string widgetName);
    void UpdateAction(string surfaceName, string actionName, string widgetName);
    void ForceUpdateAction(string surfaceName, string actionName, string widgetName);
    void CycleAction(string surfaceName, string actionName, string widgetName);
    void DoAction(string surfaceName, string actionName, string widgetName, double value);
    
    // to Widgets ->
    double GetWidgetMaxDB(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetMaxDB();
        
        return 0.0;
    }
    
    double GetWidgetMinDB(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            return widgetsByName_[widgetName]->GetMinDB();
        
        return 0.0;
    }
    
    void SetWidgetValue(string widgetName, double value)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValue(value);
    }
 
    void SetWidgetValue(string widgetName, double value, int mode)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValue(value, mode);
    }

    void SetWidgetValue(string widgetName, string value)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValue(value);
    }
    
    void SetWidgetValueToZero(string widgetName)
    {
        if(widgetsByName_.count(widgetName) > 0)
            widgetsByName_[widgetName]->SetValueToZero();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiCSurf : public OldRealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    midi_Input* midiInput_ = nullptr;
    midi_Output* midiOutput_ = nullptr;
    bool midiInMonitor_ = false;
    bool midiOutMonitor_ = false;
    
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
    virtual ~MidiCSurf()
    {
        if (midiInput_) delete midiInput_;
        if(midiOutput_) delete midiOutput_;
    }
    
    MidiCSurf(const string name, string templateFilename, int numChannels, bool isBankable, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor)
    : OldRealSurface(name, templateFilename, numChannels, isBankable), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor) {}
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) override
    {
        if(midiOutput_)
            midiOutput_->SendMsg(midiMessage, -1);
    }
    
    virtual void SendMidiMessage(int first, int second, int third) override
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
    
    virtual void RunAndUpdate() override
    {
        HandleMidiInput();
        OldRealSurface::UpdateWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string suffix_= "";
    string GUID_ = DefaultGUID;
    OldRealSurface* realSurface_= nullptr;
    bool isMovable_ = true;
    vector<string> widgetNames_;
    
public:
    RealSurfaceChannel(string suffix, OldRealSurface* surface) : suffix_(suffix), realSurface_(surface) {}
   
    string GetSuffix() { return suffix_; }
    string GetGUID() { return GUID_; }
    bool GetIsMovable() { return isMovable_; }
    
    void SetGUID(string GUID);

    void SetIsMovable(bool isMovable)
    {
        isMovable_ = isMovable;
    }
    
    void AddWidget(OldMidiWidget* widget)
    {
        widget->SetSuffix(suffix_);
        widgetNames_.push_back(widget->GetName());
        realSurface_->AddWidget(widget);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string name_ = "";
    Layer* layer_= nullptr;
    int numBankableChannels_ = 0;
    int trackOffset_ = 0;
    bool followMCP_ = true;
    vector<OldRealSurface*> realSurfaces_;
    map<string, string> actionTemplateDirectory_;
    map<string, string> fxTemplateDirectory_;
    map<string, map<string, Template *>> fxTemplates_;
    vector<FXWindow> openFXWindows_;
    bool showFXWindows_ = false;

    bool shift_ = false;
    bool option_ = false;
    bool control_ = false;
    bool alt_ = false;

    void AddAction(string actionAddress, OldAction* action);
    void MapRealSurfaceActions(OldRealSurface* surface);
    void MapTrackActions(string trackGUID, OldRealSurface* surface);

    string CurrentModifers()
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
    
    string ActionAddressFor(string GUID, string surfaceName, string actionName)
    {
        string currentModifiers = "";
        
        if(actionName != Shift && actionName != Option && actionName != Control && actionName != Alt)
            currentModifiers = CurrentModifers();
        
        return GUID + GetName() + surfaceName + currentModifiers + actionName;
    }
    
    void InitFXMaps(OldRealSurface* surface)
    {
        Template* fxTemplate = nullptr;
        string templateDirectory = fxTemplateDirectory_[surface->GetName()];
        
        for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
        {
            if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'f' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
            {
                ifstream fxTemplateFile(string(templateDirectory + "/" + filename));
                
                string firstLine;
                getline(fxTemplateFile, firstLine);
                fxTemplate = new Template(firstLine);
                
                for (string line; getline(fxTemplateFile, line) ; )
                {
                    if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                    {
                        istringstream iss(line);
                        vector<string> tokens;
                        string token;
                        while (iss >> quoted(token))
                            tokens.push_back(token);
                        
                        if(tokens.size() == 2)
                        {
                            if(fxTemplate != nullptr)
                            {
                                replace(tokens[1].begin(), tokens[1].end(), '_', ' ');
                                fxTemplate->AddEntry(tokens[0], {tokens[1]});
                            }
                        }
                    }
                }

                fxTemplates_[surface->GetName()][fxTemplate->GetName()] = fxTemplate;
            }
        }
    }

public:
    Zone(string name, Layer* layout, bool followMCP) : name_(name), layer_(layout), followMCP_(followMCP) {}
    
    string GetName() { return name_; }
    Layer* GetLayer() { return layer_; }
    bool IsShowFXWindows() { return showFXWindows_; }
    void MapFXActions(string trackGUID, OldRealSurface* surface);
    void TrackListChanged();
    void TrackFXListChanged(MediaTrack* track);
    void AdjustTrackBank(int stride);
    void RefreshLayout();
    void OnTrackSelection(MediaTrack* track);
    void MapFXToWidgets(MediaTrack *track, OldRealSurface* surface);
    void SetPinnedTracks();
    void PinSelectedTracks();
    void UnpinSelectedTracks();

    string GetTrackGUIDAsString(MediaTrack* track) { return DAW::GetTrackGUIDAsString(track, followMCP_); }
    string GetTrackGUIDAsString(int trackNumber) { return DAW::GetTrackGUIDAsString(trackNumber, followMCP_); }
    MediaTrack *GetTrackFromGUID(string trackGUID) { return DAW::GetTrackFromGUID(trackGUID, followMCP_); }
    int GetNumTracks() { return DAW::CSurf_NumTracks(followMCP_); }
    MediaTrack* CSurf_TrackFromID(int index) { return DAW::CSurf_TrackFromID(index, followMCP_); }
    int CSurf_TrackToID(MediaTrack* track) { return DAW::CSurf_TrackToID(track, followMCP_); }

    string GetNextVisibleTrackGUID(int & offset)
    {
        while(! IsTrackVisible(CSurf_TrackFromID(offset)) && offset < GetNumTracks())
            offset++;
        
        if(offset >= GetNumTracks())
            return "";
        else
            return GetTrackGUIDAsString(offset);
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
    
    void Init()
    {
        for(auto* surface : realSurfaces_)
        {
            InitFXMaps(surface);
            MapRealSurfaceActions(surface);
        }
    }
    
    void SetContext()
    {
        for(auto surface : realSurfaces_)
            surface->SetZone(this);        
    }
    
    void AddSurface(OldRealSurface* surface, string actionTemplateDirectory, string fxTemplateDirectory)
    {
        string resourcePath(DAW::GetResourcePath());
        resourcePath += "/CSI/";
        
        actionTemplateDirectory_[surface->GetName()] = resourcePath + "axt/" + actionTemplateDirectory;
        fxTemplateDirectory_[surface->GetName()] = resourcePath + "fxt/" + fxTemplateDirectory;

        numBankableChannels_ += surface->GetBankableChannels().size();
        
        realSurfaces_.push_back(surface);
    }
    
    void AddFXWindow(FXWindow fxWindow)
    {
        openFXWindows_.push_back(fxWindow);
    }
    
    void SetShowFXWindows(bool value)
    {
        showFXWindows_ = ! showFXWindows_;
        
        if(showFXWindows_ == true)
            OpenFXWindows();
        else
            CloseFXWindows();
    }
    
    void OpenFXWindows()
    {
        if(showFXWindows_)
            for(auto fxWindow : openFXWindows_)
                DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 3);
    }
    
    void CloseFXWindows()
    {
        for(auto fxWindow : openFXWindows_)
            DAW::TrackFX_Show(fxWindow.track, DAW::IndexFromFXGUID(fxWindow.track, fxWindow.fxGUID), 2);
    }
    
    void DeleteFXWindows()
    {
        CloseFXWindows();
        openFXWindows_.clear();
    }
    
    void MapTrackToWidgets(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->MapTrackToWidgets(track);
    }
    
    void UnmapWidgetsFromTrack(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->UnmapWidgetsFromTrack(track);
    }
    
    void MapFXToWidgets(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                MapFXToWidgets(track, surface);
    }
    
    void UnmapWidgetsFromFX(MediaTrack* track, string surfaceName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->UnmapFXFromWidgets(track);
    }
    
    void MapTrackAndFXActions(string trackGUID)
    {
        for(auto* surface : realSurfaces_)
            MapTrackActions(trackGUID, surface);
        
        for(auto* surface : realSurfaces_)
            MapFXActions(trackGUID, surface);
    }
    
    int GetNumLockedTracks()
    {
        int numLockedTracks = 0;
        
        for(auto surface : realSurfaces_)
            for(auto* channel : surface->GetBankableChannels())
                if(channel->GetIsMovable() == false)
                    numLockedTracks++;
        
        return numLockedTracks;
    }
    
    void RunAndUpdate()
    {
        for(auto* surface : realSurfaces_)
            surface->RunAndUpdate();
    }

    void SetShift(bool value)
    {
        shift_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetOption(bool value)
    {
        option_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetControl(bool value)
    {
        control_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetAlt(bool value)
    {
        alt_ = value;
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    void SetZoom(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetZoom(value);
    }
    
    void SetScrub(bool value)
    {
        for(auto* surface : realSurfaces_)
            surface->SetScrub(value);
    }
    
    // to Widgets ->
    void ForceUpdateWidgets()
    {
        for(auto* surface : realSurfaces_)
            surface->ForceUpdateWidgets();
    }
    
    // to Actions ->
    double GetActionCurrentNormalizedValue(string GUID, string surfaceName, string actionName, string widgetName);
    void UpdateAction(string GUID, string surfaceName, string actionName, string widgetName);
    void ForceUpdateAction(string GUID, string surfaceName, string actionName, string widgetName);
    void CycleAction(string GUID, string surfaceName, string actionName, string widgetName);
    void DoAction(double value, string GUID, string surfaceName, string actionName, string widgetName);
    
    // to Widgets ->
    double GetWidgetMaxDB(string surfaceName, string widgetName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface->GetWidgetMaxDB(widgetName);
        
        return 0.0;
    }
    
    double GetWidgetMinDB(string surfaceName, string widgetName)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                return surface->GetWidgetMinDB(widgetName);
        
        return 0.0;
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, double value, int mode)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value, mode);
    }
    
    void SetWidgetValue(string surfaceName, string widgetName, string value)
    {
        for(auto* surface : realSurfaces_)
            if(surface->GetName() == surfaceName)
                surface->SetWidgetValue(widgetName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Layer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string name_ = "";
    CSurfManager* manager_ = nullptr;
    map<string, Zone*> zones_;
    map<string, vector<OldAction*>> actions_;
    vector<string> mappedTrackGUIDs_;
    vector<MediaTrack*> touchedTracks_;
    
    void SetPinnedTracks()
    {
        for(auto [name, zone] : zones_)
            zone->SetPinnedTracks();
    }
    
public:
    Layer(string name, CSurfManager* manager) : name_(name), manager_(manager) {}

    string GetName() { return name_; }
    CSurfManager* GetManager() { return manager_; }
    void OnTrackSelection(MediaTrack* track);
    
    Zone* GetZone(string zoneName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName];
        
        return nullptr;
    }
    
    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        for(MediaTrack* touchedTrack : touchedTracks_)
            if(touchedTrack == track)
                return true;
        
        return false;
    }
   
    void SetTouchState(MediaTrack* track,  bool state)
    {
        if(state)
            touchedTracks_.push_back(track);
        else
            touchedTracks_.erase(remove(touchedTracks_.begin(), touchedTracks_.end(), track), touchedTracks_.end());
    }
    
    void AddAction(string actionAddress, OldAction* action)
    {
        actions_[actionAddress].push_back(action);
    }
   
    void MapTrackAndFXToWidgets(MediaTrack* track, string zoneName, string surfaceName)
    {
        MapTrackToWidgets(track, zoneName, surfaceName);
        MapFXToWidgets(track, zoneName, surfaceName);
    }

    void MapTrackToWidgets(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->MapTrackToWidgets(track, surfaceName);
    }
    
    void UnmapWidgetsFromTrack(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->UnmapWidgetsFromTrack(track, surfaceName);
    }
    
    void MapFXToWidgets(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->MapFXToWidgets(track, surfaceName);
    }
    
    void UnmapWidgetsFromFX(MediaTrack* track, string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->UnmapWidgetsFromFX(track, surfaceName);
    }
    
    void SetShowFXWindows(string zoneName, string surfaceName, bool value)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetShowFXWindows(value);
    }
    
    bool IsShowFXWindows(string zoneName, string surfaceName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->IsShowFXWindows();
        
        return false;
    }
    
    void TrackFXListChanged(MediaTrack* track)
    {
        for(auto [name, zone] : zones_)
            zone->TrackFXListChanged(track);
    }
    
    void AddZone(Zone* zone)
    {
        zones_[zone->GetName()] = zone;
    }
    
    void Init()
    {
        SetContext();
        
        for(auto [name, zone] : zones_)
            zone->Init();
        
        SetPinnedTracks();
    }
 
    void MapTrack(string trackGUID)
    {
        if(trackGUID == "")
            return; // Nothing to map
        
        if(find(mappedTrackGUIDs_.begin(), mappedTrackGUIDs_.end(), trackGUID) != mappedTrackGUIDs_.end())
            return; // Already did this track
        
        for(auto [name, zone] : zones_)
            zone->MapTrackAndFXActions(trackGUID);
        
        mappedTrackGUIDs_.push_back(trackGUID);
    }
    
    void AdjustTrackBank(string zoneName, string surfaceName, int stride)
    {
        touchedTracks_.clear(); // GAW TBD -- in case anyone is touching a fader -- this is slightly pessimistic, if a fader from another zone is being touched it gets cleared too
        zones_[zoneName]->AdjustTrackBank(stride);
    }
    
    void TrackListChanged()
    {
        for(auto const& [name, zone] : zones_)
            zone->TrackListChanged();
    }
    
    void SetContext()
    {
        for(auto const& [name, zone] : zones_)
            zone->SetContext();
    }
    
    void RunAndUpdate()
    {
        for(auto const& [name, zone] : zones_)
            zone->RunAndUpdate();
    }

    void SetShift(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetShift(value);
    }
    
    void SetOption(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetOption(value);
    }
    
    void SetControl(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetControl(value);
    }
    
    void SetAlt(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetAlt(value);
    }
    
    void SetZoom(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetZoom(value);
    }
    
    void SetScrub(string zoneName, string surfaceName, bool value)
    {
        zones_[zoneName]->SetScrub(value);
    }

    void PinSelectedTracks()
    {
        for(auto [name, zone] : zones_)
            zone->PinSelectedTracks();
    }
    
    void UnpinSelectedTracks()
    {
        for(auto [name, zone] : zones_)
            zone->UnpinSelectedTracks();
    }
    
    // to Widgets ->
    void ForceUpdateWidgets()
    {
        for(auto [name, zone] : zones_)
            zone->ForceUpdateWidgets();
    }

    // to Actions ->
    double GetActionCurrentNormalizedValue(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0 && actions_[actionAddress].size() > 0)
            return actions_[actionAddress][0]->GetCurrentNormalizedValue(zoneName, surfaceName, widgetName);
        else
            return 0.0;
    }

    void UpdateAction(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Update(zoneName, surfaceName, widgetName);
    }
    
    void ForceUpdateAction(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->ForceUpdate(zoneName, surfaceName, widgetName);
    }

    void CycleAction(string actionAddress, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Cycle(zoneName, surfaceName, widgetName);
    }
    
    void DoAction(string actionAddress, double value, string zoneName, string surfaceName, string widgetName)
    {
        if(actions_.count(actionAddress) > 0)
            for(auto* action : actions_[actionAddress])
                action->Do(value, zoneName, surfaceName, widgetName);
    }
    
    // to Widgets ->
    double GetWidgetMaxDB(string zoneName, string surfaceName, string widgetName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->GetWidgetMaxDB(surfaceName, widgetName);
        
        return 0.0;
    }
    
    double GetWidgetMinDB(string zoneName, string surfaceName, string widgetName)
    {
        if(zones_.count(zoneName) > 0)
            return zones_[zoneName]->GetWidgetMinDB(surfaceName, widgetName);
        
        return 0.0;
    }
    
    void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetWidgetValue(surfaceName, widgetName, value);
    }
    
    void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value, int mode)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetWidgetValue(surfaceName, widgetName, value, mode);
    }
    
    void SetWidgetValue(string zoneName, string surfaceName, string widgetName, string value)
    {
        if(zones_.count(zoneName) > 0)
            zones_[zoneName]->SetWidgetValue(surfaceName, widgetName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MidiIOManager* midiIOManager_ = nullptr;
    vector <Layer*> layers_;
    vector<OldRealSurface*> realSurfaces_;
    bool isInitialized_ = false;
    int currentLayerIndex_ = 0; 
    bool VSTMonitor_ = false;
    
    void InitRealSurface(OldRealSurface* surface);

    void AddRealSurface(OldRealSurface* realSurface)
    {
        InitRealSurface(realSurface);
        realSurfaces_.push_back(realSurface);
    }

    void RunAndUpdate()
    {
        if(!isInitialized_)
        {
            Init();
        }
        
        if(layers_.size() > 0)
            layers_[currentLayerIndex_]->RunAndUpdate();
    }
    
    double GetPrivateProfileDouble(string key)
    {
        char tmp[512];
        memset(tmp, 0, sizeof(tmp));
        
        DAW::GetPrivateProfileString("REAPER", key.c_str() , "", tmp, sizeof(tmp), DAW::get_ini_file());

        return strtod (tmp, NULL);
    }

public:
    virtual ~CSurfManager() {};
    
    CSurfManager() { midiIOManager_ = new MidiIOManager(); }
    
    void Init()
    {
        layers_.clear();
        realSurfaces_.clear();
        isInitialized_ = false;

        bool midiInMonitor = false;
        bool midiOutMonitor = false;
        VSTMonitor_ = false;
        
        Layer* currentLayer = nullptr;
        Zone* currentZone = nullptr;
        
        ifstream iniFile(string(DAW::GetResourcePath()) + "/CSI/CSI.ini");
        
        for (string line; getline(iniFile, line) ; )
        {
            if(line[0] != '/' && line != "") // ignore comment lines and blank lines
            {
                istringstream iss(line);
                vector<string> tokens;
                string token;
                
                while (iss >> quoted(token))
                    tokens.push_back(token);
                
                if(tokens[0] == MidiInMonitor)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    if(tokens[1] == "On")
                        midiInMonitor = true;
                }
                else if(tokens[0] == MidiOutMonitor)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    if(tokens[1] == "On")
                        midiOutMonitor = true;
                }
                else if(tokens[0] == VSTMonitor)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    if(tokens[1] == "On")
                        VSTMonitor_ = true;
                }
                else if(tokens[0] == RealSurface_)
                {
                    if(tokens.size() != 7)
                        continue;
                    
                    int numChannels = atoi(tokens[2].c_str());
                    bool isBankable = tokens[3] == "1" ? true : false;
                    int channelIn = atoi(tokens[4].c_str());
                    int channelOut = atoi(tokens[5].c_str());
                    
                    AddRealSurface(new MidiCSurf(tokens[1], string(DAW::GetResourcePath()) + "/CSI/rst/" + tokens[6], numChannels, isBankable, GetMidiIOManager()->GetMidiInputForChannel(channelIn), GetMidiIOManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
                }
                else if(tokens[0] == Layer_)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    currentLayer = new Layer(tokens[1], this);
                    layers_.push_back(currentLayer);
                    
                }
                else if(tokens[0] == Zone_ && currentLayer != nullptr)
                {
                    if(tokens.size() != 3)
                        continue;
                    
                    currentZone = new Zone(tokens[1], currentLayer, tokens[2] == "Yes" ? true : false);
                    currentLayer->AddZone(currentZone);
                }
                else if(tokens[0] == VirtualSurface_ && currentZone != nullptr)
                {
                    if(tokens.size() != 4)
                        continue;
                    
                    for(auto surface : realSurfaces_)
                        if(surface->GetName() == tokens[1])
                            currentZone->AddSurface(surface, tokens[2], tokens[3]);
                }
            }
        }
        
        for(auto layer : layers_)
            layer->Init();
        
        if(layers_.size() > 0)
            layers_[0]->SetContext();
        
        isInitialized_ = true;
    }

    
    MidiIOManager* GetMidiIOManager() { return midiIOManager_; }
    bool GetVSTMonitor() { return isInitialized_ ? VSTMonitor_ : false; }
    double GetFaderMaxDB() { return GetPrivateProfileDouble("slidermaxv"); }
    double GetFaderMinDB() { return GetPrivateProfileDouble("sliderminv"); }
    double GetVUMaxDB() { return GetPrivateProfileDouble("vumaxvol"); }
    double GetVUMinDB() { return GetPrivateProfileDouble("vuminvol"); }
    
    void OnTrackSelection(MediaTrack *track)
    {
        if(layers_.size() > 0)
            layers_[currentLayerIndex_]->OnTrackSelection(track);
    }
    
    void Run()
    {
        RunAndUpdate();
    }
    
    void NextLayer()
    {
        if(layers_.size() > 0)
        {
            currentLayerIndex_ = currentLayerIndex_ == layers_.size() - 1 ? 0 : ++currentLayerIndex_;
            layers_[currentLayerIndex_]->SetContext();
        }
    }

    bool GetTouchState(MediaTrack* track, int touchedControl)
    {
        if(layers_.size() > 0)
            return layers_[currentLayerIndex_]->GetTouchState(track, touchedControl);
        else
            return false;
    }
    
    void TrackListChanged()
    {
        for(auto & layout : layers_)
            layout->TrackListChanged();
    }

    void TrackFXListChanged(MediaTrack* trackid)
    {
        for(auto & layout : layers_)
            layout->TrackFXListChanged(trackid);
    }
};

#endif /* control_surface_integrator.h */
