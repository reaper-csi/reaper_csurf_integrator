//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_Reaper_actions.h"
#include "control_surface_manager_actions.h"

extern Manager* TheManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidgeta available for inclusion in Real Surface Templates, we will add widgets as necessary
////////////////////////////////////////////////////////////////////////////////////////////////////////
int strToHex(string valueStr)
{
    return strtol(valueStr.c_str(), nullptr, 16);
}

double strToDouble(string valueStr)
{
    return strtod(valueStr.c_str(), nullptr);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string widgetClass, int index)
{
    if(widgetClass == "DisplayUpper") return new DisplayUpper_Midi_Widget(surface, role, index);
    if(widgetClass == "DisplayLower") return new DisplayLower_Midi_Widget(surface, role, index);
    
    return new Midi_Widget(surface, role, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00));
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string widgetClass, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "Button") return new PushButton_Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithLatch") return new PushButtonWithLatch_Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithRelease") return new PushButtonWithRelease_Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonCycler") return new PushButtonCycler_Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Encoder") return new Encoder_Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Fader7Bit") return new Fader7Bit_Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Min), new MIDI_event_ex_t(byte1, byte2, byte3Max));
    
    return new Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string widgetClass, double minDB, double maxDB, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "VUMeter") return new VUMeter_Midi_Widget(surface, role, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "GainReductionMeter") return new GainReductionMeter_Midi_Widget(surface, role, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    
    return new Midi_Widget(surface, role,  new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string widgetClass, double minDB, double maxDB, int byte1, int byte2Min, int byte2Max, int byte3Min, int byte3Max)
{
    if(widgetClass == "Fader14Bit") return new Fader14Bit_Midi_Widget(surface, role, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
    
    return new Midi_Widget(surface, role, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Widget::RequestUpdate()
{
    TheManager->RequestActionUpdate(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_Widget::SendMidiMessage(MIDI_event_ex_t* midiMessage)
{
    surface_->SendMidiMessage(midiMessage);
}

void Midi_Widget::SendMidiMessage(int first, int second, int third)
{
    if(first != lastMessageSent_->midi_message[0] || second != lastMessageSent_->midi_message[1] || third != lastMessageSent_->midi_message[2])
    {
        lastMessageSent_->midi_message[0] = first;
        lastMessageSent_->midi_message[1] = second;
        lastMessageSent_->midi_message[2] = third;
        surface_->SendMidiMessage(first, second, third);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
Midi_RealSurface::Midi_RealSurface(const string name, string templateFilename, int numChannels, bool isBankable, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor)
: RealSurface(name, templateFilename, numChannels, isBankable), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor)
{
    ifstream surfaceTemplateFile(templateFilename);
    bool inChannel = false;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        if(line[0] != '/' && line != "") // ignore comment lines and blank lines
        {
            istringstream iss(line);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);
            
            if(tokens.size() == 1)
            {
                if(tokens[0] == Channel)
                    inChannel = true;
                else if(tokens[0] == ChannelEnd)
                    inChannel = false;
            }
            else if(tokens.size() == 2)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[1], i));
            }
            else if(tokens.size() == 6)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4]), strToHex(tokens[5])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5])));
            }
            else if(tokens.size() == 8)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]) + i, strToHex(tokens[6]), strToHex(tokens[7])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7])));
            }
            else if(tokens.size() == 9)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]) + i, strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Page
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Page::Init()
{
    currentNumTracks_ = DAW::CSurf_NumTracks(followMCP_);
    
    for(auto * surface : realSurfaces_)
    {
        for(auto * widget : surface->GetAllWidgets())
        {
            widgetsByName_[widget->GetRole() + surface->GetWidgetSuffix(widget)] = widget;
            widgetContexts_[widget] = WidgetContext();
            
            if(actionTemplates_.count(widget->GetSurface()->GetName()) > 0 && actionTemplates_[widget->GetSurface()->GetName()].count(CurrentModifers(widget) + widget->GetRole()) > 0)
            {
                for(auto paramBundle : actionTemplates_[widget->GetSurface()->GetName()][CurrentModifers(widget) + widget->GetRole()])
                {
                    if(Action* action = TheManager->GetAction(paramBundle[0]))
                    {
                        widgetContexts_[widget].SetContext(WidgetMode::Track);
                        widgetContexts_[widget].GetContextInfo()->actionsWithParamBundle.push_back(make_pair(action, paramBundle));
                    }
                }
            }
        }
    }
    
    // Set the initial Widget / Track contexts
    for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_) && i < bankableChannels_.size(); i++)
    {
        string trackGUID = DAW::GetTrackGUIDAsString(i, followMCP_);
        bankableChannels_[i]->SetGUID(trackGUID);
        for(auto widget : bankableChannels_[i]->GetWidgets())
        {
            if(actionTemplates_.count(widget->GetSurface()->GetName()) > 0 && actionTemplates_[widget->GetSurface()->GetName()].count(CurrentModifers(widget) + widget->GetRole()) > 0)
            {
                for(auto paramBundle : actionTemplates_[widget->GetSurface()->GetName()][CurrentModifers(widget) + widget->GetRole()])
                {
                    if(Action* action = TheManager->GetAction(paramBundle[0]))
                    {
                        widgetContexts_[widget].SetContext(WidgetMode::Track);
                        widgetContexts_[widget].GetContextInfo()->actionsWithParamBundle.push_back(make_pair(action, paramBundle));
                        widgetContexts_[widget].GetContextInfo()->trackGUID = trackGUID;
                    }
                }
            }
        }
    }
    
    SetPinnedTracks();
}

void Page::TrackFXListChanged(MediaTrack* track)
{
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(TheManager->GetVSTMonitor())
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

void Page::InitActionTemplates(RealSurface* surface, string templateDirectory)
{
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/" + filename));
            
            for (string line; getline(actionTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    vector<string> params;
                    for(int i = 1; i < tokens.size(); i++)
                        params.push_back(tokens[i]);
                    
                    actionTemplates_[surface->GetName()][tokens[0]].push_back(params);
                }
            }
        }
    }
}

void Page::InitFXTemplates(RealSurface* surface, string templateDirectory)
{
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'f' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream fxTemplateFile(string(templateDirectory + "/" + filename));
            
            string firstLine;
            getline(fxTemplateFile, firstLine);
            
            fxTemplates_[surface->GetName()][firstLine] = map<string, vector<string>>();
            
            for (string line; getline(fxTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    // GAW -- the first token is the Widget role, the reat is the FX param, possibly with spaces.
                    string fxParameter = "";
                    
                    if(tokens.size() > 2)
                    {
                        fxParameter = line.substr(tokens[0].size() + 1, line.size());
                    }
                    else if(tokens.size() == 2)
                    {
                        fxParameter = tokens[1];
                    }
                    
                    if(tokens.size() > 1)
                    {
                        if(fxParameter == "GainReductionDB")
                            fxTemplates_[surface->GetName()][firstLine][tokens[0]].push_back("GainReductionDB");
                        else
                            fxTemplates_[surface->GetName()][firstLine][tokens[0]].push_back("TrackFX");
                        
                        fxTemplates_[surface->GetName()][firstLine][tokens[0]].push_back(fxParameter);
                    }
                }
            }
        }
    }
}

int Page::GetFXParamIndex(Widget* widget, MediaTrack* track, int fxIndex, string paramName)
{
    char fxName[BUFSZ];
    DAW::TrackFX_GetFXName(track, fxIndex, fxName, sizeof(fxName));
    
    if(TheManager->GetFXParamIndices().count(fxName) > 0 && TheManager->GetFXParamIndices()[fxName].count(paramName) > 0)
        return TheManager->GetFXParamIndices()[fxName][paramName];
    
    char fxParamName[BUFSZ];
    RealSurface* surface = widget->GetSurface();
    string widgetName = widget->GetRole() + surface->GetWidgetSuffix(widget);
    
    if(fxTemplates_.count(surface->GetName()) > 0  && fxTemplates_[surface->GetName()].count(fxName) > 0 && fxTemplates_[surface->GetName()][fxName].count(widgetName) > 0)
    {
        for(int i = 0; i < DAW::TrackFX_GetNumParams(track, fxIndex); i++)
        {
            DAW::TrackFX_GetParamName(track, fxIndex, i, fxParamName, sizeof(fxParamName));
            if(fxTemplates_[surface->GetName()][fxName][widgetName][1] == fxParamName)
            {
                TheManager->GetFXParamIndices()[fxName][paramName] = i;
                return i;
            }
        }
    }
    
    return 0;
}

void Page::MapTrackToWidgets(RealSurface* surface, MediaTrack* track)
{
    string trackGUID = DAW::GetTrackGUIDAsString(track, followMCP_);
    
    for(auto channel : surface->GetChannels())
    {
        for(auto widget : channel)
        {
            if(actionTemplates_.count(widget->GetSurface()->GetName()) > 0 && actionTemplates_[widget->GetSurface()->GetName()].count(CurrentModifers(widget) + widget->GetRole()) > 0)
            {
                for(auto paramBundle : actionTemplates_[widget->GetSurface()->GetName()][CurrentModifers(widget) + widget->GetRole()])
                {
                    if(Action* action = TheManager->GetAction(paramBundle[0]))
                    {
                        widgetContexts_[widget].SetContext(WidgetMode::Track);
                        widgetContexts_[widget].GetContextInfo()->actionsWithParamBundle.push_back(make_pair(action, paramBundle));
                        widgetContexts_[widget].GetContextInfo()->trackGUID = trackGUID;
                    }
                }
            }
        }
    }
}

void Page::MapFXToWidgets(RealSurface* surface, MediaTrack* track)
{
    char fxName[BUFSZ];
    char fxGUID[BUFSZ];
    
    DeleteFXWindows();
    
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
                    
                    if(Action* action = TheManager->GetAction(paramBundle[0]))
                    {
                        WidgetContext & context = widgetContexts_[widget];
                        context.SetContext(WidgetMode::FX);
                        context.GetContextInfo()->actionsWithParamBundle.push_back(make_pair(action, paramBundle));
                        context.GetContextInfo()->trackGUID = DAW::GetTrackGUIDAsString(track, followMCP_);
                        context.GetContextInfo()->fxIndex = i;
                    }
                }
            }
            
            AddFXWindow(FXWindow(track, fxGUID));
        }
    }
    
    OpenFXWindows();
}

// Widgets -> Actions -- this is the grand switchboard that does all the realtime heavy lifting wrt routing and context 
void Page::RequestActionUpdate(Widget* widget)
{
    for(auto [action, paramBundle] : widgetContexts_[widget].GetContextInfo()->actionsWithParamBundle)
        action->RequestUpdate(widget, this, widgetContexts_[widget]);
}

void Page::DoAction(Widget* widget, double value)
{
    for(auto [action, paramBundle] : widgetContexts_[widget].GetContextInfo()->actionsWithParamBundle)
        action->Do(widget, this, widgetContexts_[widget], value);
}

void Page::OnTrackSelection(MediaTrack* track)
{
    for(auto surface : realSurfaces_)
        if(actionTemplates_.count(surface->GetName()) > 0 && actionTemplates_[surface->GetName()].count(TrackOnSelection) > 0)
            for(auto paramBundle : actionTemplates_[surface->GetName()][TrackOnSelection])
                if(Action* action = TheManager->GetAction(paramBundle[0]))
                    action->Do(surface, track, this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Manager::InitActionDictionary()
{
    actions_["Reaper"] = new ReaperAction();
    actions_["TrackFX"] = new TrackFX();
    actions_["GainReductionDB"] = new TrackGainReductionMeter();
    actions_["TrackVolume"] = new TrackVolume();
    actions_["TrackPan"] = new TrackPan();
    actions_["TrackPanWidth"] = new TrackPanWidth();
    actions_["TrackNameDisplay"] = new TrackNameDisplay();
    actions_["TrackVolumeDisplay"] = new TrackVolumeDisplay();
    actions_["TrackPanDisplay"] = new TrackPanDisplay();
    actions_["TrackPanWidthDisplay"] = new TrackPanWidthDisplay();
    actions_["Rewind"] = new Rewind();
    actions_["FastForward"] = new FastForward();
    actions_["Play"] = new Play();
    actions_["Stop"] = new Stop();
    actions_["Record"] = new Record();
    actions_["RepeatingArrow"] = new RepeatingArrow();
    actions_["TrackSelect"] = new TrackSelect();
    actions_["TrackUniqueSelect"] = new TrackUniqueSelect();
    actions_["TrackRangeSelect"] = new TrackRangeSelect();
    actions_["TrackRecordArm"] = new TrackRecordArm();
    actions_["TrackMute"] = new TrackMute();
    actions_["TrackSolo"] = new TrackSolo();
    actions_["TrackTouch"] = new TrackTouch();
    actions_["TrackTouchControlled"] = new TrackTouchControlled();
    actions_["GlobalAutoMode"] = new GlobalAutoMode();
    actions_["TrackAutoMode"] = new TrackAutoMode();
    actions_["CycleTimeline"] = new CycleTimeline();
    actions_["TrackOutputMeter"] = new TrackOutputMeter();
    actions_["SetShowFXWindows"] = new SetShowFXWindows();
    actions_["Shift"] = new class Shift();
    actions_["Option"] = new class Option();
    actions_["Control"] = new class Control();
    actions_["Alt"] = new class Alt();
    actions_["Latched"] = new Latched();
    actions_["LatchedZoom"] = new LatchedZoom();
    actions_["LatchedScrub"] = new LatchedScrub();
    actions_["NextPage"] = new class NextPage();
    actions_["TrackBank"] = new TrackBank();
    actions_["PinSelectedTracks"] = new PinSelectedTracks();
    actions_["UnpinSelectedTracks"] = new UnpinSelectedTracks();
    actions_["MapTrackAndFXToWidgets"] = new MapTrackAndFXToWidgets();
}

void Manager::Init()
{
    pages_.clear();
    midi_realSurfaces_.clear();
    
    bool midiInMonitor = false;
    bool midiOutMonitor = false;
    VSTMonitor_ = false;
    
    Page* currentPage = nullptr;
    
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
                
                midi_realSurfaces_.push_back(new Midi_RealSurface(tokens[1], string(DAW::GetResourcePath()) + "/CSI/rst/" + tokens[6], numChannels, isBankable, GetMidiIOManager()->GetMidiInputForChannel(channelIn), GetMidiIOManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
            }
            else if(tokens[0] == Page_)
            {
                if(tokens.size() != 3)
                    continue;
                
                currentPage = new Page(tokens[1], tokens[2] == "Yes" ? true : false);
                pages_.push_back(currentPage);
                
            }
            else if(tokens[0] == VirtualSurface_)
            {
                if(tokens.size() != 4)
                    continue;
                
                for(auto surface : midi_realSurfaces_)
                    if(surface->GetName() == tokens[1])
                        currentPage->AddSurface(surface, tokens[2], tokens[3]);
            }
        }
    }
    
    for(auto page : pages_)
        page->Init();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reaper Actions available for mapping, this list will get added to substantially over time
////////////////////////////////////////////////////////////////////////////////////////////////////////
OldAction* ActionFor(string name, Layer* layer)
{
    if(name == "SetShowFXWindows")  return new SetShowFXWindows_Action(layer);
    else if(name == "Rewind")  return new Rewind_Action(layer);
    else if(name == "FastForward")  return new FastForward_Action(layer);
    else if(name == "Stop")  return new Stop_Action(layer);
    else if(name == "Play")  return new Play_Action(layer);
    else if(name == "Record")  return new Record_Action(layer);
    else if(name == "NextLayer")  return new NextLayer_Action(layer);
    else if(name == "PinSelectedTracks")  return new PinSelectedTracks_Action(layer);
    else if(name == "UnpinSelectedTracks")  return new UnpinSelectedTracks_Action(layer);
    else if(name == "Shift")  return new Shift_Action(layer);
    else if(name == "Option")  return new Option_Action(layer);
    else if(name == "Control")  return new Control_Action(layer);
    else if(name == "Alt")  return new Alt_Action(layer);
    else if(name == "LatchedZoom")  return new LatchedZoom_Action(layer);
    else if(name == "LatchedScrub")  return new LatchedScrub_Action(layer);
    else if(name == "Cycled")  return new Cycled_Action(layer);
    else if(name == "CycleTimeline")  return new CycleTimeline_Action(layer);
    
    return new OldAction(layer);
}

OldAction* ActionFor(string name, Layer* layer, string param)
{
    if(name == "TrackBank")  return new TrackBank_Action(layer, param);
    else if(name == "TrackAutoMode")  return new TrackAutoMode_Action(layer, param);
    else if(name == "GlobalAutoMode")  return new GlobalAutoMode_Action(layer, param);
    else if(name == "Reaper")  return new OldReaper_Action(layer, param);
    
    return new OldAction(layer);
}

OldAction* TrackActionFor(string name, Layer* layer, string trackGUID)
{
    if(name == "TrackVolume")  return new TrackVolume_Action(layer, trackGUID);
    else if(name == "TrackVolumeDisplay")  return new TrackVolumeDisplay_Action(layer, trackGUID);
    else if(name == "TrackPanDisplay")  return new TrackPanDisplay_Action(layer, trackGUID);
    else if(name == "TrackPanWidthDisplay")  return new TrackPanWidthDisplay_Action(layer, trackGUID);
    else if(name == "TrackPan")  return new TrackPan_Action(layer, trackGUID);
    else if(name == "TrackPanWidth")  return new TrackPanWidth_Action(layer, trackGUID);
    else if(name == "TrackTouch")  return new TrackTouch_Action(layer, trackGUID);
    else if(name == "TrackMute")  return new TrackMute_Action(layer, trackGUID);
    else if(name == "TrackSolo")  return new TrackSolo_Action(layer, trackGUID);
    else if(name == "TrackUniqueSelect")  return new TrackUniqueSelect_Action(layer, trackGUID);
    else if(name == "TrackRangeSelect")  return new TrackRangeSelect_Action(layer, trackGUID);
    else if(name == "TrackSelect")  return new TrackSelect_Action(layer, trackGUID);
    else if(name == "TrackRecordArm")  return new TrackRecordArm_Action(layer, trackGUID);
    else if(name == "TrackNameDisplay")  return new TrackNameDisplay_Action(layer, trackGUID);
    else if(name == "MapTrackAndFXToWidgets")  return new MapTrackAndFXToWidgets_Action(layer, trackGUID);
    
    return new OldAction(layer);
}

OldAction* TrackActionFor(string name, Layer* layer, string trackGUID, string param)
{
    if(name == "TrackOutputMeter")  return new TrackOutputMeter_Action(layer, trackGUID, param);
    
    return new OldAction(layer);
}

OldAction* TrackActionFor(string name, string actionAddress, Layer* layer, string trackGUID, OldAction* baseAction)
{
    if(name == "TrackTouchControlled")  return new TrackTouchControlled_Action(actionAddress, layer, trackGUID, baseAction);
    
    return new OldAction(layer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidgeta available for inclusion in Real Surface Templates, we will add widgets as necessary
////////////////////////////////////////////////////////////////////////////////////////////////////////
OldMidiWidget* WidgetFor(OldRealSurface* surface, string name, string widgetClass, int index)
{
    if(widgetClass == "DisplayUpper") return new DisplayUpper_MidiWidget(surface, name, index);
    if(widgetClass == "DisplayLower") return new DisplayLower_MidiWidget(surface, name, index);

    return new OldMidiWidget(surface, name, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00));
}

OldMidiWidget* WidgetFor(OldRealSurface* surface, string name, string widgetClass, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "Button") return new PushButton_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithLatch") return new PushButtonWithLatch_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithRelease") return new PushButtonWithRelease_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonCycler") return new PushButtonCycler_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Encoder") return new Encoder_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Fader7Bit") return new Fader7Bit_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Min), new MIDI_event_ex_t(byte1, byte2, byte3Max));
    
    return new OldMidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

OldMidiWidget* WidgetFor(OldRealSurface* surface, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "VUMeter") return new VUMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "GainReductionMeter") return new GainReductionMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
  
    return new OldMidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

OldMidiWidget* WidgetFor(OldRealSurface* surface, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2Min, int byte2Max, int byte3Min, int byte3Max)
{
    if(widgetClass == "Fader14Bit") return new Fader14Bit_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
    
    return new OldMidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OldMidiWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OldMidiWidget::AddToRealSurface(OldRealSurface* surface)
{
    surface->AddWidgetToNameMap(GetName(), this);
}

void OldMidiWidget::Update()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->UpdateAction(GetGUID(), GetActionName(), GetName());
}

void OldMidiWidget::ForceUpdate()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->ForceUpdateAction(GetGUID(), GetActionName(), GetName());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RealSurfaceChannel::SetGUID(string GUID)
{
    GUID_ = GUID;
    
    for (auto widgetName : widgetNames_)
        realSurface_->SetWidgetGUID(widgetName, GUID);
    
    realSurface_->GetZone()->GetLayer()->MapTrack(GUID_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
OldRealSurface::OldRealSurface(const string name, string templateFilename, int numChannels, bool isBankable) : name_(name),  templateFilename_(templateFilename), isBankable_(isBankable)
{
    for(int i = 0; i < numChannels; i++)
        channels_.push_back(new RealSurfaceChannel(to_string(i + 1), this));
}

void OldRealSurface::MapTrackToWidgets(MediaTrack *track)
{
    string trackGUID = GetZone()->GetTrackGUIDAsString(track);
    
    for(auto* channel : channels_)
        channel->SetGUID(trackGUID);
}

void  OldRealSurface::UnmapWidgetsFromTrack(MediaTrack *track)
{
    for(auto* channel : channels_)
        channel->SetGUID("");
}

//void OldRealSurface::AddAction(string actionAddress, OldAction* action)
//{
    //GetZone()->GetLayer()->AddAction(actionAddress, action);
//}

// to Actions ->
double OldRealSurface::GetActionCurrentNormalizedValue(string GUID, string actionName, string widgetName)
{
    return GetZone()->GetActionCurrentNormalizedValue(GUID, GetName(), actionName, widgetName);
}

void OldRealSurface::UpdateAction(string GUID, string actionName, string widgetName)
{
    GetZone()->UpdateAction(GUID, GetName(), actionName, widgetName);
}

void OldRealSurface::ForceUpdateAction(string GUID, string actionName, string widgetName)
{
    GetZone()->ForceUpdateAction(GUID, GetName(), actionName, widgetName);
}

void OldRealSurface::CycleAction(string GUID, string actionName, string widgetName)
{
    GetZone()->CycleAction(GUID, GetName(), actionName, widgetName);
}

void OldRealSurface::DoAction(string GUID, string actionName, string widgetName, double value)
{
    GetZone()->DoAction(value, GUID, GetName(), actionName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Zone::OnTrackSelection(MediaTrack* track)
{
    for(auto* surface : realSurfaces_)
        DoAction(1.0, GetTrackGUIDAsString(track), surface->GetName(), TrackOnSelection, TrackOnSelection);
}

void Zone::TrackListChanged()
{
    vector<RealSurfaceChannel*> channels;
    
    for(auto* surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            channels.push_back(channel);
    
    int currentOffset = trackOffset_;
    bool shouldRefreshLayout = false;
    
    for(int i = 0; i < channels.size() && currentOffset < DAW::CSurf_NumTracks(followMCP_); i++)
    {
        if(channels[i]->GetIsMovable() == false)
        {
            if(GetTrackFromGUID(channels[i]->GetGUID()) == nullptr) // track has been removed
            {
                channels[i]->SetIsMovable(true); // unlock this, since there is no longer a track to lock to
                shouldRefreshLayout = true;
                break;
            }
            else
            {
                currentOffset++; // track exists, move on
            }
        }
        
        else if(channels[i]->GetGUID() == GetNextVisibleTrackGUID(currentOffset))
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

void Zone::AdjustTrackBank(int stride)
{
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ < 1 - numBankableChannels_ + GetNumLockedTracks())
        trackOffset_ = 1 - numBankableChannels_ + GetNumLockedTracks();
    
    if(trackOffset_ >  DAW::CSurf_NumTracks(followMCP_) - 1)
        trackOffset_ = DAW::CSurf_NumTracks(followMCP_) - 1;
    
    // Jump over any pinned channels and invisible tracks
    vector<string> pinnedChannels;
    for(auto surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            if(channel->GetIsMovable() == false)
                pinnedChannels.push_back(channel->GetGUID());
    
    bool skipThisChannel = false;
    
    while(trackOffset_ >= 0 && trackOffset_ < DAW::CSurf_NumTracks(followMCP_))
    {
        string trackGUID = GetTrackGUIDAsString(trackOffset_);
        
        for(auto pinnedChannel : pinnedChannels)
            if(pinnedChannel == trackGUID)
            {
                skipThisChannel = true;
                previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
                break;
            }
        
        if( ! IsTrackVisible(CSurf_TrackFromID(trackOffset_)))
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

void Zone::RefreshLayout()
{
    vector<string> pinnedChannelLayout;
    vector<string> pinnedChannels;
    vector<string> movableChannelLayout;
    vector<string> channelLayout;
    
    // Layout locked channel GUIDs
    for(auto surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            if(channel->GetIsMovable() == false)
            {
                pinnedChannelLayout.push_back(channel->GetGUID());
                pinnedChannels.push_back(channel->GetGUID());
            }
            else
                pinnedChannelLayout.push_back("");
    
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
    for(auto* surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            channel->SetGUID(channelLayout[offset++]);
    
    for(auto* surface : realSurfaces_)
        surface->ForceUpdateWidgets();
}

void Zone::MapFXToWidgets(MediaTrack *track, OldRealSurface* surface)
{
    char fxName[BUFSZ];
    char fxGUID[BUFSZ];
    char fxParamName[BUFSZ];
    
    DeleteFXWindows();
    surface->UnmapFXFromWidgets(track);
    
    string trackGUID = GetTrackGUIDAsString(track);
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
        
        if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
        {
            Template* map = fxTemplates_[surface->GetName()][fxName];
            
            for(auto mapEntry : map->GetTemplateEntries())
            {
                if(mapEntry.params[0] == GainReductionDB)
                    surface->SetWidgetFXGUID(mapEntry.widgetRole, trackGUID + fxGUID);
                else
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); DAW::TrackFX_GetParamName(track, i, j++, fxParamName, sizeof(fxParamName)))
                        if(mapEntry.params[0] == fxParamName)
                            surface->SetWidgetFXGUID(mapEntry.widgetRole, trackGUID + fxGUID);
            }
            
            AddFXWindow(FXWindow(track, fxGUID));
        }
    }
    
    OpenFXWindows();
    
    surface->ForceUpdateWidgets();
}

void Zone::MapFXActions(string trackGUID, OldRealSurface* surface)
{
    MediaTrack* track = GetTrackFromGUID(trackGUID);
    if(track == nullptr)
        return;
    
    Layer* layer = GetLayer();
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    char fxGUID[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
        {
            Template* map = fxTemplates_[surface->GetName()][fxName];
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            string actionBaseAddress = trackGUID + fxGUID + GetName() + surface->GetName();
            
            for(auto mapEntry : map->GetTemplateEntries())
            {
                if(mapEntry.params[0] == GainReductionDB)
                    GetLayer()->AddAction(actionBaseAddress + mapEntry.widgetRole, new TrackGainReductionMeter_Action(layer, trackGUID, fxGUID));
                else
                {
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                    {
                        DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                        
                        if(mapEntry.params[0] == fxParamName)
                            GetLayer()->AddAction(actionBaseAddress + mapEntry.widgetRole, new TrackFX_Action(layer, trackGUID, fxGUID, j));
                    }
                }
            }
        }
    }
}

void Zone::TrackFXListChanged(MediaTrack* track)
{
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];

    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(GetLayer()->GetManager()->GetVSTMonitor())
        {
            DAW::ShowConsoleMsg(("\n\n" + string(fxName) + "\n").c_str());
            
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                DAW::ShowConsoleMsg((string(fxParamName) + "\n").c_str());
            }
        }
    }
    
    for(auto* surface : realSurfaces_)
        MapFXActions(GetTrackGUIDAsString(track), surface);
}

void Zone::AddAction(string actionAddress, OldAction* action)
{
    GetLayer()->AddAction(actionAddress, action);
}

void Zone::MapRealSurfaceActions(OldRealSurface* surface)
{
    Layer* layer = GetLayer();
    string actionBaseAddress = DefaultGUID + GetName() + surface->GetName();;
    string templateDirectory = actionTemplateDirectory_[surface->GetName()];
    
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/" + filename));
           
            for (string line; getline(actionTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens.size() == 2)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layer));
                    else if(tokens.size() == 3)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layer, tokens[2]));
                }
            }
        }
    }
}

void Zone::MapTrackActions(string trackGUID, OldRealSurface* surface)
{
    MediaTrack* track = GetTrackFromGUID(trackGUID);
    if(track == nullptr)
        return;

    Layer* layer = GetLayer();
    string actionBaseAddress = trackGUID + GetName() + surface->GetName();
    string templateDirectory = actionTemplateDirectory_[surface->GetName()];
    
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/Track/" + filename));
            
            for (string line; getline(actionTemplateFile, line) ; )
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
                        AddAction(actionBaseAddress + tokens[0], TrackActionFor(tokens[1], layer, trackGUID));
                    }
                    else if(tokens.size() == 3)
                    {
                        if(tokens[1] == "TrackTouchControlled")
                        {
                            string actionAddress = actionBaseAddress + tokens[0];
                            OldAction* controlledAction = TrackActionFor(tokens[2], layer, trackGUID);
                            AddAction(actionAddress, TrackActionFor(tokens[1], actionAddress, layer, trackGUID, controlledAction));
                        }
                        else
                        {
                            AddAction(actionBaseAddress + tokens[0], TrackActionFor(tokens[1], layer, trackGUID, tokens[2]));
                        }
                    }
                    else if(tokens[1] == "Cycled" && tokens.size() > 4)
                    {
                        OldAction* cycledAction = ActionFor(tokens[1], layer);
                        AddAction(actionBaseAddress + tokens[0], cycledAction);
                        AddAction(actionBaseAddress + tokens[2], cycledAction);
                        for(int i = 3 ; i < tokens.size(); i++)
                            cycledAction->AddAction(TrackActionFor(tokens[i], layer, trackGUID));
                    }
                }
            }
        }
    }
}

void Zone::SetPinnedTracks()
{
    char buffer[BUFSZ];
    RealSurfaceChannel* channel = nullptr;
    
    for(auto* surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetBankableChannels().size(); i++)
        {
            channel = surface->GetBankableChannels()[i];
            
            if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayer()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), buffer, sizeof(buffer)))
            {
                channel->SetGUID(buffer);
                channel->SetIsMovable(false);
            }
        }
    }
}

void Zone::PinSelectedTracks()
{
    RealSurfaceChannel* channel = nullptr;
    
    for(auto* surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetBankableChannels().size(); i++)
        {
            channel = surface->GetBankableChannels()[i];
            MediaTrack* track = GetTrackFromGUID(channel->GetGUID());
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsMovable(false);
                DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayer()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), channel->GetGUID().c_str());
                DAW::MarkProjectDirty(nullptr);
            }
        }
    }
}

void Zone::UnpinSelectedTracks()
{
    char buffer[BUFSZ];
    RealSurfaceChannel* channel = nullptr;
    
    for(auto* surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetBankableChannels().size(); i++)
        {
            channel = surface->GetBankableChannels()[i];
            MediaTrack* track = GetTrackFromGUID(channel->GetGUID());
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsMovable(true);
                if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayer()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), buffer, sizeof(buffer)))
                {
                    DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayer()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), "");
                    DAW::MarkProjectDirty(nullptr);
                }
            }
        }
    }
}

// to Actions ->
double Zone::GetActionCurrentNormalizedValue(string GUID, string surfaceName, string actionName, string widgetName)
{
    return GetLayer()->GetActionCurrentNormalizedValue(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::UpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayer()->UpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::ForceUpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayer()->ForceUpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::CycleAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayer()->CycleAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::DoAction(double value, string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayer()->DoAction(ActionAddressFor(GUID, surfaceName, actionName), value, GetName(), surfaceName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Layer
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Layer::OnTrackSelection(MediaTrack* track)
{
    MapTrack(DAW::GetTrackGUIDAsString(track));
    
    for(auto const& [name, zone] : zones_)
        zone->OnTrackSelection(track);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfManager::InitRealSurface(OldRealSurface* surface)
{
    ifstream surfaceTemplateFile(surface->GetTemplateFilename());
    bool inChannel = false;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        if(line[0] != '/' && line != "") // ignore comment lines and blank lines
        {
            istringstream iss(line);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);
            
            if(tokens.size() == 1)
            {
                if(tokens[0] == Channel)
                    inChannel = true;
                else if(tokens[0] == ChannelEnd)
                    inChannel = false;
            }
            else if(tokens.size() == 2)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], i));
            }
            else if(tokens.size() == 6)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4]), strToHex(tokens[5])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5])));
            }
            else if(tokens.size() == 8)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]) + i, strToHex(tokens[6]), strToHex(tokens[7])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7])));
            }
            else if(tokens.size() == 9)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]) + i, strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
            }
        }
    }
}


