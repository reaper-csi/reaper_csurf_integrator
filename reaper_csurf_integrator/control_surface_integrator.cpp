//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_base_actions.h"
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
// ContextManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void ContextManager::RequestActionUpdate(Widget* widget)
{
    if(currentWidgetContext_)
    {
        for(auto context : *currentWidgetContext_->GetActionContexts())
            context->RequestActionUpdate(currentPage_, widget);
    }
}

void ContextManager::DoAction(Widget* widget, double value)
{
    if(currentWidgetContext_)
    {
        for(auto context : *currentWidgetContext_->GetActionContexts())
            context->DoAction(currentPage_, widget, value);
    }
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
: RealSurface(name, numChannels, isBankable), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor)
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
void Page::InitActionContexts(RealSurface* surface, string templateDirectory)
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
                    
                    // GAW -- the first token is the (possibly decorated with modifiers) Widget name.
                    
                    string modifiers = "";
                    string widgetRole = "";
                    bool isInverted = false;
                    
                    if(tokens.size() > 0)
                    {
                        istringstream modified_role(tokens[0]);
                        vector<string> modifier_tokens;
                        string modifier_token;
                        
                        while (getline(modified_role, modifier_token, '+'))
                            modifier_tokens.push_back(modifier_token);
                        
                        widgetRole = modifier_tokens[modifier_tokens.size() - 1];
                        
                        if(modifier_tokens.size() > 1)
                        {
                            vector<string> modifierSlots = { "", "", "", "" };
                            
                            for(int i = 0; i < modifier_tokens.size() - 1; i++)
                            {
                                if(modifier_tokens[i] == Shift)
                                    modifierSlots[0] = Shift;
                                else if(modifier_tokens[i] == Option)
                                    modifierSlots[1] = Option;
                                else if(modifier_tokens[i] == Control)
                                    modifierSlots[2] = Control;
                                else if(modifier_tokens[i] == Alt)
                                    modifierSlots[3] = Alt;
                                else if(modifier_tokens[i] == Invert)
                                    isInverted = true;
                            }
                            
                            modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
                        }
                    }

                    // GAW IMPORTANT -- If widgetRole == "OnTrackSelection", add a MIDI widget to the surface so that we can add ActionContexts
                    // Timing is important here, the widget must be added BEFORE the widget->GetRole() == widgetRole comparison below
                    if(widgetRole == "TrackOnSelection")
                        surface->AddWidget(new Midi_Widget((Midi_RealSurface*)surface, widgetRole, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00)));

                    vector<string> params;
                    for(int i = 1; i < tokens.size(); i++)
                        params.push_back(tokens[i]);
                    
                    if(tokens.size() > 1)
                        for(auto * widget : surface->GetAllWidgets())
                            if(widget->GetRole() == widgetRole)
                                if(ActionContext* context = TheManager->GetActionContext(params, isInverted))
                                {
                                    if(widgetContexts_.count(widget) < 1)
                                        widget->AddWidgetContext(this, widgetContexts_[widget] = new WidgetContext());
                                    
                                    widgetContexts_[widget]->AddActionContext(Track, modifiers, context);
                                }
                }
            }
        }
    }
}

void Page::InitFXContexts(RealSurface* surface, string templateDirectory)
{
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'f' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream fxTemplateFile(string(templateDirectory + "/" + filename));
            
            string fxName;
            getline(fxTemplateFile, fxName);
            
            for (string line; getline(fxTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    // GAW -- the first token is the (possibly decorated with modifiers) Widget name, the rest is the FX param, possibly with spaces.
                    
                    string modifiers = "";
                    string widgetName = "";
                    bool isInverted = false;

                    if(tokens.size() > 0)
                    {
                        istringstream modified_name(tokens[0]);
                        vector<string> modifier_tokens;
                        string modifier_token;
                        
                        while (getline(modified_name, modifier_token, '+'))
                            modifier_tokens.push_back(modifier_token);
                        
                        widgetName = modifier_tokens[modifier_tokens.size() - 1];
                        
                        if(modifier_tokens.size() > 1)
                        {
                            vector<string> modifierSlots = { "", "", "", "" };
                            
                            for(int i = 0; i < modifier_tokens.size() - 1; i++)
                            {
                                if(modifier_tokens[i] == Shift)
                                    modifierSlots[0] = Shift;
                                else if(modifier_tokens[i] == Option)
                                    modifierSlots[1] = Option;
                                else if(modifier_tokens[i] == Control)
                                    modifierSlots[2] = Control;
                                else if(modifier_tokens[i] == Alt)
                                    modifierSlots[3] = Alt;
                                else if(modifier_tokens[i] == Invert)
                                    isInverted = true;
                            }
                            
                            modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
                        }
                    }
                    
                    string fxParamName = "";
                    
                    if(tokens.size() >= 2)
                    {
                        fxParamName = line.substr(tokens[0].size(), line.size());
                        fxParamName.erase(0, fxParamName.find_first_not_of(" "));
                    }
                    
                    vector<string> params;
                    
                    if(fxParamName == "GainReductionDB")
                        params.push_back(fxParamName);
                    else params.push_back("TrackFX");
                    params.push_back(fxParamName);
                    params.push_back(fxName);

                    if(tokens.size() > 1)
                        for(auto * widget : surface->GetAllWidgets())
                            if(widget->GetRole() + widget->GetSurface()->GetWidgetSuffix(widget) == widgetName)
                                if(ActionContext* context = TheManager->GetFXActionContext(params, isInverted))
                                {
                                    if(widgetContexts_.count(widget) < 1)
                                        widget->AddWidgetContext(this, widgetContexts_[widget] = new WidgetContext());
                                    
                                    widgetContexts_[widget]->AddActionContext(fxName, modifiers, context);
                                    fxWidgets_[fxName].push_back(widget);
                                }
                }
            }
        }
    }
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
int Page::GetFXParamIndex(Widget* widget, int fxIndex, string fxParamName)
{
    char fxName[BUFSZ];

    MediaTrack* track = widget->GetTrack();
    
    DAW::TrackFX_GetFXName(track, fxIndex, fxName, sizeof(fxName));
    
    if(TheManager->GetFXParamIndices().count(fxName) > 0 && TheManager->GetFXParamIndices()[fxName].count(fxParamName) > 0)
        return TheManager->GetFXParamIndices()[fxName][fxParamName];
    
    char paramName[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetNumParams(track, fxIndex); i++)
    {
        DAW::TrackFX_GetParamName(track, fxIndex, i, paramName, sizeof(paramName));
        
        if(paramName == fxParamName)
        {
            TheManager->GetFXParamIndices()[fxName][fxParamName] = i;
            return i;
        }
    }
    
    return 0;
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

void Manager::InitActionContextDictionary()
{
    InitActionDictionary();
    
    actionContexts_["Reaper"] = [this](vector<string> params, bool isInverted) { return new ReaperActionContext(actions_[params[0]], params[1], isInverted); };
    actionContexts_["TrackFX"] = [this](vector<string> params, bool isInverted) { return new FXContext(actions_[params[0]], params[1], isInverted); };
    actionContexts_["GainReductionDB"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackVolume"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPan"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPanWidth"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackNameDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackVolumeDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPanDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPanWidthDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["Rewind"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["FastForward"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Play"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Stop"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Record"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    //actionContexts_["RepeatingArrow"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSelect"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackUniqueSelect"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackRangeSelect"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackRecordArm"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackMute"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSolo"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackTouch"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackTouchControlled"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["GlobalAutoMode"] = [this](vector<string> params, bool isInverted) { return new GlobalContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["TrackAutoMode"] = [this](vector<string> params, bool isInverted) { return new TrackContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["CycleTimeline"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackOutputMeter"] = [this](vector<string> params, bool isInverted) { return new TrackContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["SetShowFXWindows"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Shift"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Option"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Control"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Alt"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Latched"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["LatchedZoom"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["LatchedScrub"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["NextPage"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackBank"] = [this](vector<string> params, bool isInverted) { return new GlobalContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["PinSelectedTracks"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["UnpinSelectedTracks"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["MapTrackAndFXToWidgets"] = [this](vector<string> params, bool isInverted) { return new PageSurfaceTrackContext(actions_[params[0]], isInverted); };
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
            else if(tokens[0] == PageToken)
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

    currentPageIndex_ = 0;
   
    if(pages_.size() > 0)
        for(auto surface : midi_realSurfaces_)
            for(auto widget : surface->GetAllWidgets() )
                widget->SetPageContext(pages_[currentPageIndex_]);
}
