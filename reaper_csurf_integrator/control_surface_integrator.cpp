//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_action_contexts.h"
#include "control_surface_Reaper_actions.h"
#include "control_surface_manager_actions.h"
#include "control_surface_integrator_ui.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiInputPort
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int port_ = 0;
    midi_Input* midiInput_ = nullptr;
    
    MidiInputPort(int port, midi_Input* midiInput) : port_(port), midiInput_(midiInput) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiOutputPort
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int port_ = 0;
    midi_Output* midiOutput_ = nullptr;
    
    MidiOutputPort(int port, midi_Output* midiOutput) : port_(port), midiOutput_(midiOutput) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi I/O Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static map<int, MidiInputPort*> midiInputs_;
static map<int, MidiOutputPort*> midiOutputs_;

static midi_Input* GetMidiInputForPort(int inputPort)
{
    if(midiInputs_.count(inputPort) > 0)
        return midiInputs_[inputPort]->midiInput_; // return existing
    
    // otherwise make new
    midi_Input* newInput = DAW::CreateMIDIInput(inputPort);
    
    if(newInput)
    {
        newInput->start();
        midiInputs_[inputPort] = new MidiInputPort(inputPort, newInput);
        return newInput;
    }
    
    return nullptr;
}

static midi_Output* GetMidiOutputForPort(int outputPort)
{
    if(midiOutputs_.count(outputPort) > 0)
        return midiOutputs_[outputPort]->midiOutput_; // return existing
    
    // otherwise make new
    midi_Output* newOutput = DAW::CreateMIDIOutput(outputPort, false, NULL);
    
    if(newOutput)
    {
        midiOutputs_[outputPort] = new MidiOutputPort(outputPort, newOutput);
        return newOutput;
    }
    
    return nullptr;
}

void ShutdownMidiIO()
{
    for(auto [index, input] : midiInputs_)
        input->midiInput_->stop();
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Parsing
//////////////////////////////////////////////////////////////////////////////////////////////
static vector<string> GetTokens(string line)
{
    vector<string> tokens;
    
    istringstream iss(line);
    string token;
    while (iss >> quoted(token))
        tokens.push_back(token);
    
    return tokens;
}

static map<string, vector<Zone*>> includedZoneMembers;

void ExpandZone(vector<string> tokens, string filePath, vector<Zone*> &expandedZones, vector<string> &expandedZonesIds, ControlSurface* surface, string alias)
{
    istringstream expandedZone(tokens[1]);
    vector<string> expandedZoneTokens;
    string expandedZoneToken;
    
    while (getline(expandedZone, expandedZoneToken, '|'))
        expandedZoneTokens.push_back(expandedZoneToken);
    
    if(expandedZoneTokens.size() > 1)
    {
        //////////////////////////////////////////////////////////////////////////////////////////////
        /// Expand syntax of type "Channel|1-8" into 8 Zones named Channel1, Channel2, ... Channel8
        //////////////////////////////////////////////////////////////////////////////////////////////

        string zoneBaseName = "";
        int rangeBegin = 0;
        int rangeEnd = 1;
        
        zoneBaseName = expandedZoneTokens[0];
        
        istringstream range(expandedZoneTokens[1]);
        vector<string> rangeTokens;
        string rangeToken;
        
        while (getline(range, rangeToken, '-'))
            rangeTokens.push_back(rangeToken);
        
        if(rangeTokens.size() > 1)
        {
            rangeBegin = stoi(rangeTokens[0]);
            rangeEnd = stoi(rangeTokens[1]);
            
            if(zoneBaseName == "Send")
                surface->SetNumSendSlots(rangeEnd - rangeBegin + 1);
            
            if(zoneBaseName == "FXMenu")
                surface->GetFXActivationManager()->SetNumFXSlots(rangeEnd - rangeBegin + 1);
            
            for(int i = rangeBegin; i <= rangeEnd; i++)
                expandedZonesIds.push_back(to_string(i));
            
            for(int i = 0; i <= rangeEnd - rangeBegin; i++)
            {
                Zone* zone = new Zone(surface, zoneBaseName + expandedZonesIds[i], filePath, alias + expandedZonesIds[i]);
                if(surface->AddZone(zone))
                    expandedZones.push_back(zone);
            }
        }
    }
    else
    {
        //////////////////////////////////////////////////////////////////////////////////////////////
        /// Just regular syntax of type "Channel1"
        //////////////////////////////////////////////////////////////////////////////////////////////

        Zone* zone = new Zone(surface, tokens[1], filePath, alias);
        if(surface->AddZone(zone))
        {
            if((tokens[1].compare(0, 4, "Send")) == 0)
                surface->SetNumSendSlots(surface->GetNumSendSlots() + 1);
            if((tokens[1].compare(0, 6, "FXMenu")) == 0)
                surface->GetFXActivationManager()->SetNumFXSlots(surface->GetFXActivationManager()->GetNumFXSlots() + 1);
            expandedZones.push_back(zone);
            expandedZonesIds.push_back("");
        }
    }
}

void ExpandIncludedZone(vector<string> tokens, vector<string> &expandedZones)
{
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// Expand syntax of type "Channel|1-8" into 8 Zones named Channel1, Channel2, ... Channel8
    //////////////////////////////////////////////////////////////////////////////////////////////
    
    vector<string> expandedZonesIds;
    
    istringstream expandedZone(tokens[0]);
    vector<string> expandedZoneTokens;
    string expandedZoneToken;
    
    while (getline(expandedZone, expandedZoneToken, '|'))
        expandedZoneTokens.push_back(expandedZoneToken);
    
    if(expandedZoneTokens.size() > 1)
    {
        string zoneBaseName = "";
        int rangeBegin = 0;
        int rangeEnd = 1;
        
        zoneBaseName = expandedZoneTokens[0];
        
        istringstream range(expandedZoneTokens[1]);
        vector<string> rangeTokens;
        string rangeToken;
        
        while (getline(range, rangeToken, '-'))
            rangeTokens.push_back(rangeToken);
        
        if(rangeTokens.size() > 1)
        {
            rangeBegin = stoi(rangeTokens[0]);
            rangeEnd = stoi(rangeTokens[1]);
            
            for(int i = rangeBegin; i <= rangeEnd; i++)
                expandedZonesIds.push_back(to_string(i));
            
            for(int i = 0; i <= rangeEnd - rangeBegin; i++)
            {
                expandedZones.push_back(zoneBaseName + expandedZonesIds[i]);
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// Just regular syntax of type "Channel1"
    //////////////////////////////////////////////////////////////////////////////////////////////
    else
    {
        expandedZones.push_back(tokens[0]);
    }
}

void ProcessIncludedZones(int &lineNumber, ifstream &zoneFile, string filePath, Zone* zone)
{
    for (string line; getline(zoneFile, line) ; )
    {
        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens.size() == 1)
        {
            if(tokens[0] == "IncludedZonesEnd")    // finito baybay - IncludedZone processing complete
                return;
            
            vector<string> expandedZones;
            
            ExpandIncludedZone(tokens, expandedZones);
            
            for(int i = 0; i < expandedZones.size(); i++)
            {
                if(zone->GetName() != expandedZones[i]) // prevent recursive defintion
                    includedZoneMembers[expandedZones[i]].push_back(zone);
            }
        }
    }
}

map<string, TrackNavigator*> trackNavigators;

static TrackNavigator* TrackNavigatorForChannel(int channelNum, string channelName, ControlSurface* surface)
{
    if(trackNavigators.count(channelName) < 1)
        trackNavigators[channelName] = surface->GetPage()->GetTrackNavigationManager()->AddTrackNavigator();
    
    return trackNavigators[channelName];
}

static void listZoneFiles(const string &path, vector<string> &results)
{
    regex rx(".*\\.zon$");
    
    if (auto dir = opendir(path.c_str())) {
        while (auto f = readdir(dir)) {
            if (!f->d_name || f->d_name[0] == '.') continue;
            if (f->d_type == DT_DIR)
                listZoneFiles(path + f->d_name + "/", results);
            
            if (f->d_type == DT_REG)
                if(regex_match(f->d_name, rx))
                    results.push_back(path + f->d_name);
        }
        closedir(dir);
    }
}

static void GetWidgetNameAndModifiers(string line, string &widgetName, string &modifiers, bool &isTrackTouch, bool &isInverted, bool &shouldToggle, bool &shouldIgnoreRelease, double &delayAmount)
{
    istringstream modified_role(line);
    vector<string> modifier_tokens;
    vector<string> modifierSlots = { "", "", "", "" };
    string modifier_token;
    
    while (getline(modified_role, modifier_token, '+'))
        modifier_tokens.push_back(modifier_token);
    
    if(modifier_tokens.size() > 1)
    {
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
            
            else if(modifier_tokens[i] == "TrackTouch")
                isTrackTouch = true;
            else if(modifier_tokens[i] == "Invert")
                isInverted = true;
            else if(modifier_tokens[i] == "Toggle")
                shouldToggle = true;
            else if(modifier_tokens[i] == "Press")
                shouldIgnoreRelease = true;
            else if(modifier_tokens[i] == "Hold")
                delayAmount = 1.0;
        }
    }
    
    widgetName = modifier_tokens[modifier_tokens.size() - 1];
    
    modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
    
    if(modifiers == "")
        modifiers = NoModifiers;
}

void ProcessZone(int &lineNumber, ifstream &zoneFile, vector<string> passedTokens, string filePath, ControlSurface* surface, vector<Widget*> &widgets)
{
    const string FXGainReductionMeter = "FXGainReductionMeter"; // GAW TBD don't forget this logic
    
    if(passedTokens.size() < 2)
        return;
    
    string alias = "";
    
    if(passedTokens.size() > 2)
        alias = passedTokens[2];
    else
        alias = passedTokens[1];

    
    vector<Zone*> expandedZones;
    vector<string> expandedZonesIds;
    
    ExpandZone(passedTokens, filePath, expandedZones, expandedZonesIds, surface, alias);
    
    map<Widget*, WidgetActionManager*> widgetActionManagerForWidget;

    bool hasTrackNavigator = false;
    vector<TrackNavigator*> expandedTrackNavigators;
    
    bool hasSelectedTrackNavigator = false;
    bool hasFocusedFXTrackNavigator = false;
    
    string parentZone = "";

    for (string line; getline(zoneFile, line) ; )
    {
        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens.size() > 0)
        {
            if(tokens[0] == "ZoneEnd")    // finito baybay - Zone processing complete
                return;
        }
        
        if(tokens.size() == 1 && tokens[0] == "TrackNavigator")
        {
            hasTrackNavigator = true;
            
            for(int i = 0; i < expandedZones.size(); i++)
                expandedTrackNavigators.push_back(TrackNavigatorForChannel(i, surface->GetName() + to_string(i), surface));
            
            continue;
        }

        if(tokens.size() == 1 && tokens[0] == "SelectedTrackNavigator")
        {
            hasSelectedTrackNavigator = true;
            continue;
        }
        
        if(tokens.size() == 1 && tokens[0] == "FocusedFXNavigator")
        {
            hasFocusedFXTrackNavigator = true;
            continue;
        }
        
        if(tokens.size() == 2 && tokens[0] == "ParentZone")
        {
            parentZone = tokens[1];
            continue;
        }

        for(int i = 0; i < expandedZones.size(); i++)
        {
            // Pre-process for "Channel|1-8" syntax

            string parentZoneLine(parentZone);
            parentZoneLine = regex_replace(parentZoneLine, regex("\\|"), expandedZonesIds[i]);
            expandedZones[i]->SetParentZoneName(parentZoneLine);
            
            // Pre-process for "Channel|1-8" syntax
            string localZoneLine(line);
            localZoneLine = regex_replace(localZoneLine, regex("\\|"), expandedZonesIds[i]);
            
            vector<string> tokens(GetTokens(localZoneLine));
            
            if(tokens.size() > 0)
            {
                if(tokens[0] == "IncludedZones")
                {
                    ProcessIncludedZones(lineNumber, zoneFile, filePath, expandedZones[i]);
                    continue;
                }
                
                // GAW -- the first token is the Widget name, possibly decorated with modifiers
                string widgetName = "";
                string modifiers = "";
                bool isTrackTouch = false;
                bool isInverted = false;
                bool shouldToggle = false;
                bool shouldIgnoreRelease = false;
                bool isDelayed = false;
                double delayAmount = 0.0;
                
                GetWidgetNameAndModifiers(tokens[0], widgetName, modifiers, isTrackTouch, isInverted, shouldToggle, shouldIgnoreRelease, delayAmount);
                
                if(delayAmount > 0.0)
                    isDelayed = true;
                
                Widget* widget = nullptr;
                
                for(auto * aWidget : widgets)
                {
                    if(aWidget->GetName() == widgetName)
                    {
                        widget = aWidget;
                        break;
                    }
                }
                
                vector<string> params;
                for(int j = 1; j < tokens.size(); j++)
                    params.push_back(tokens[j]);
                
                if(params.size() > 0 && widget != nullptr)
                {
                    if(TheManager->IsActionAvailable(params[0]))
                    {
                        if(widgetActionManagerForWidget.count(widget) < 1)
                        {
                            TrackNavigator* trackNavigator = nullptr;
                            
                            if(hasTrackNavigator)
                                trackNavigator = expandedTrackNavigators[i];
                            else if(hasSelectedTrackNavigator)
                                trackNavigator = new SelectedTrackNavigator(surface->GetPage()->GetTrackNavigationManager());
                            else if(hasFocusedFXTrackNavigator)
                                trackNavigator = new FocusedFXNavigator(surface->GetPage()->GetTrackNavigationManager());
                            
                            widgetActionManagerForWidget[widget] = new WidgetActionManager(widget, expandedZones[i], trackNavigator);

                            expandedZones[i]->AddWidgetActionManager(widgetActionManagerForWidget[widget]);
                        }
                        
                        Action* action = TheManager->GetAction(widgetActionManagerForWidget[widget], params);

                        if(isTrackTouch)
                            widgetActionManagerForWidget[widget]->AddTrackTouchedAction(action);
                        else
                            widgetActionManagerForWidget[widget]->AddAction(modifiers, action);
                        
                        if(isInverted)
                            action->SetIsInverted();
                        
                        if(shouldToggle)
                        {
                            action->SetShouldToggle();
                            action->SetShouldIgnoreRelease();
                        }
                        
                        if(shouldIgnoreRelease)
                            action->SetShouldIgnoreRelease();

                        if(isDelayed)
                            action->SetDelayAmount(delayAmount * 1000.0);

                        if(params[0] == Shift || params[0] == Option || params[0] == Control || params[0] == Alt)
                            widget->SetIsModifier();
                    }
                    else
                    {
                        // log error, etc.
                    }
                }
            }
        }
    }
}

static int strToHex(string valueStr)
{
    return strtol(valueStr.c_str(), nullptr, 16);
}

static double strToDouble(string valueStr)
{
    return strtod(valueStr.c_str(), nullptr);
}

void ProcessMidiWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  Midi_ControlSurface* surface, vector<Widget*> &widgets)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(surface, tokens[1]);
    widgets.push_back(widget);
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens[0] == "WidgetEnd")    // finito baybay - Widget processing complete
            return;
        
        if(tokens.size() > 0)
        {
            Midi_FeedbackProcessor* feedbackProcessor = nullptr;
            
            string widgetClass = tokens[0];
            
            // Control Signal Generators
            if(widgetClass == "Press" && tokens.size() == 4)
                new PressRelease_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Press" && tokens.size() == 7)
                new PressRelease_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])), new MIDI_event_ex_t(strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6])));
            else if(widgetClass == "Fader14Bit" && tokens.size() == 4)
                new Fader14Bit_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Fader7Bit" && tokens.size() == 4)
                new Fader7Bit_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Encoder" && tokens.size() == 4)
                new Encoder_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            
            // Feedback Processors
            else if(widgetClass == "FB_TwoState" && (tokens.size() == 7 || tokens.size() == 8))
            {
                feedbackProcessor = new TwoState_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])), new MIDI_event_ex_t(strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6])));
                
                if(tokens.size() == 8)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[7]));
            }
            else if(tokens.size() == 4 || tokens.size() == 5)
            {
                if(widgetClass == "FB_Fader14Bit")
                    feedbackProcessor = new Fader14Bit_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_Fader7Bit")
                    feedbackProcessor = new Fader7Bit_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_Encoder")
                    feedbackProcessor = new Encoder_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_VUMeter")
                    feedbackProcessor = new VUMeter_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_GainReductionMeter")
                    feedbackProcessor = new GainReductionMeter_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_QConProXMasterVUMeter")
                    feedbackProcessor = new QConProXMasterVUMeter_Midi_FeedbackProcessor(surface);
                
                if(tokens.size() == 5 && feedbackProcessor != nullptr)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[4]));
            }
            else if(widgetClass == "FB_MCUTimeDisplay" && tokens.size() == 1)
            {
                feedbackProcessor = new MCU_TimeDisplay_Midi_FeedbackProcessor(surface);
            }
            else if(widgetClass == "FB_MCUVUMeter" && (tokens.size() == 2 || tokens.size() == 3))
            {
                feedbackProcessor = new MCUVUMeter_Midi_FeedbackProcessor(surface, stoi(tokens[1]));
                
                if(tokens.size() == 3 && feedbackProcessor != nullptr)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[2]));
            }
            else if((widgetClass == "FB_MCUDisplayUpper" || widgetClass == "FB_MCUDisplayLower" || widgetClass == "FB_MCUXTDisplayUpper" || widgetClass == "FB_MCUXTDisplayLower") && (tokens.size() == 2 || tokens.size() == 3))
            {
                if(widgetClass == "FB_MCUDisplayUpper")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 0, 0x14, 0x12, stoi(tokens[1]));
                else if(widgetClass == "FB_MCUDisplayLower")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 1, 0x14, 0x12, stoi(tokens[1]));
                else if(widgetClass == "FB_MCUXTDisplayUpper")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 0, 0x15, 0x12, stoi(tokens[1]));
                else if(widgetClass == "FB_MCUXTDisplayLower")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 1, 0x15, 0x12, stoi(tokens[1]));
                
                if(tokens.size() == 3 && feedbackProcessor != nullptr)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[2]));
            }
            
            else if((widgetClass == "FB_C4DisplayUpper" || widgetClass == "FB_C4DisplayLower") && (tokens.size() == 3 || tokens.size() == 4))
            {
                if(widgetClass == "FB_C4DisplayUpper")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 0, 0x17, stoi(tokens[1]) + 0x30, stoi(tokens[2]));
                else if(widgetClass == "FB_C4DisplayLower")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 1, 0x17, stoi(tokens[1]) + 0x30, stoi(tokens[2]));
                
                if(tokens.size() == 4 && feedbackProcessor != nullptr)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[3]));
            }
            
            if(feedbackProcessor != nullptr)
                widget->AddFeedbackProcessor(feedbackProcessor);
        }
    }
}

void ProcessOSCWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  OSC_ControlSurface* surface, vector<Widget*> &widgets)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(surface, tokens[1]);
    widgets.push_back(widget);
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens[0] == "WidgetEnd")    // finito baybay - Widget processing complete
            return;
        
        if(tokens.size() > 1)
        {
            string widgetClass = tokens[0];

            // Control Signal Generator
            if(widgetClass == "Control")
                new OSC_CSIMessageGenerator(surface, widget, tokens[1]);
            // Feedback Processor
            else if(widgetClass == "FB_Processor")
                widget->AddFeedbackProcessor(new OSC_FeedbackProcessor(surface, tokens[1]));
        }
    }
}

void ProcessFile(string filePath, ControlSurface* surface, vector<Widget*> &widgets)
{
    int lineNumber = 0;
    
    try
    {
        ifstream file(filePath);
        
        for (string line; getline(file, line) ; )
        {
            lineNumber++;
            
            if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
                continue;
            
            vector<string> tokens(GetTokens(line));
            
            if(tokens.size() > 0)
            {
                if(tokens[0] == "Zone")
                    ProcessZone(lineNumber, file, tokens, filePath, surface, widgets);
                else if(tokens[0] == "Widget")
                {
                    if(filePath[filePath.length() - 3] == 'm')
                        ProcessMidiWidget(lineNumber, file, tokens, (Midi_ControlSurface*)surface, widgets);
                    if(filePath[filePath.length() - 3] == 'o')
                        ProcessOSCWidget(lineNumber, file, tokens, (OSC_ControlSurface*)surface, widgets);
                }
            }
        }
    }
    catch (exception &e)
    {
        char buffer[250];
        sprintf(buffer, "Trouble in %s, around line %d\n", filePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Manager::InitActionDictionary()
{
    actions_["NoAction"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new NoAction(name, manager); };
    actions_["Reaper"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new ReaperAction(name, manager, params); };
    actions_["FXNameDisplay"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXNameDisplay(name, manager, params); };
    actions_["FXParam"] =                           [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParam(name, manager, params); };
    actions_["FXParamRelative"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParamRelative(name, manager, params); };
    actions_["FXParamNameDisplay"] =                [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParamNameDisplay(name, manager, params); };
    actions_["FXParamValueDisplay"] =               [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParamValueDisplay(name, manager, params); };
    actions_["FXGainReductionMeter"] =              [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXGainReductionMeter(name, manager, params); };
    actions_["TrackVolume"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackVolume(name, manager); };
    actions_["MasterTrackVolume"] =                 [this](string name, WidgetActionManager* manager, vector<string> params) { return new MasterTrackVolume(name, manager); };
    actions_["TrackSendVolume"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendVolume(name, manager); };
    actions_["TrackSendPan"] =                      [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendPan(name, manager); };
    actions_["TrackSendMute"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendMute(name, manager); };
    actions_["TrackSendInvertPolarity"] =           [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendInvertPolarity(name, manager); };
    actions_["TrackSendPrePost"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendPrePost(name, manager); };
    actions_["TrackPan"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPan(name, manager, params); };
    actions_["TrackPanWidth"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPanWidth(name, manager, params); };
    actions_["TrackNameDisplay"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackNameDisplay(name, manager); };
    actions_["TrackVolumeDisplay"] =                [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackVolumeDisplay(name, manager); };
    actions_["TrackSendNameDisplay"] =              [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendNameDisplay(name, manager); };
    actions_["TrackSendVolumeDisplay"] =            [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendVolumeDisplay(name, manager); };
    actions_["TrackPanDisplay"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPanDisplay(name, manager); };
    actions_["TrackPanWidthDisplay"] =              [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPanWidthDisplay(name, manager); };
    actions_["TimeDisplay"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new TimeDisplay(name, manager); };
    actions_["Rewind"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new Rewind(name, manager); };
    actions_["FastForward"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new FastForward(name, manager); };
    actions_["Play"] =                              [this](string name, WidgetActionManager* manager, vector<string> params) { return new Play(name, manager); };
    actions_["Stop"] =                              [this](string name, WidgetActionManager* manager, vector<string> params) { return new Stop(name, manager); };
    actions_["Record"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new Record(name, manager); };
    actions_["TrackFolderDive"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackFolderDive(name, manager); };
    actions_["TrackSelect"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSelect(name, manager); };
    actions_["TrackUniqueSelect"] =                 [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackUniqueSelect(name, manager); };
    actions_["MasterTrackUniqueSelect"] =           [this](string name, WidgetActionManager* manager, vector<string> params) { return new MasterTrackUniqueSelect(name, manager); };
    actions_["TrackRangeSelect"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackRangeSelect(name, manager); };
    actions_["TrackRecordArm"] =                    [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackRecordArm(name, manager); };
    actions_["TrackMute"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackMute(name, manager); };
    actions_["TrackSolo"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSolo(name, manager); };
    actions_["TrackTouch"] =                        [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetTrackTouch(name, manager); };
    actions_["MasterTrackTouch"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetMasterTrackTouch(name, manager); };
    actions_["CycleTimeline"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new CycleTimeline(name, manager); };
    actions_["TrackOutputMeter"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeter(name, manager, params); };
    actions_["TrackOutputMeterAverageLR"] =         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterAverageLR(name, manager); };
    actions_["TrackOutputMeterMaxPeakLR"] =         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterMaxPeakLR(name, manager); };
    actions_["MasterTrackOutputMeter"] =            [this](string name, WidgetActionManager* manager, vector<string> params) { return new MasterTrackOutputMeter(name, manager, params); };
    actions_["SetShowFXWindows"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetShowFXWindows(name, manager); };
    actions_["ToggleScrollLink"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleScrollLink(name, manager, params); };
    actions_["CycleTimeDisplayModes"] =             [this](string name, WidgetActionManager* manager, vector<string> params) { return new CycleTimeDisplayModes(name, manager); };
    actions_["NextPage"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new GoNextPage(name, manager); };
    actions_["GoPage"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new class GoPage(name, manager, params); };
    actions_["GoZone"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new GoZone(name, manager, params); };
    actions_["SelectTrackRelative"] =               [this](string name, WidgetActionManager* manager, vector<string> params) { return new SelectTrackRelative(name, manager, params); };
    actions_["TrackBank"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackBank(name, manager, params); };
    actions_["Shift"] =                             [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetShift(name, manager); };
    actions_["Option"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetOption(name, manager); };
    actions_["Control"] =                           [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetControl(name, manager); };
    actions_["Alt"] =                               [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetAlt(name, manager); };
    actions_["TogglePin"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TogglePin(name, manager); };
    actions_["ToggleLearnMode"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleLearnMode(name, manager); };
    actions_["ToggleMapSelectedTrackSends"] =       [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapSelectedTrackSends(name, manager); };
    actions_["MapSelectedTrackSendsToWidgets"] =    [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackSendsToWidgets(name, manager); };
    actions_["ToggleMapSelectedTrackFX"] =          [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapSelectedTrackFX(name, manager); };
    actions_["MapSelectedTrackFXToWidgets"] =       [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackFXToWidgets(name, manager); };
    actions_["ToggleMapSelectedTrackFXMenu"] =      [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapSelectedTrackFXMenu(name, manager); };
    actions_["MapSelectedTrackFXToMenu"] =          [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackFXToMenu(name, manager); };
    actions_["ToggleMapFocusedFX"] =                [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapFocusedFX(name, manager); };
    actions_["MapFocusedFXToWidgets"] =             [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapFocusedFXToWidgets(name, manager); };
    actions_["GoFXSlot"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new GoFXSlot(name, manager, params); };
    actions_["TrackAutoMode"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackAutoMode(name, manager, params); };
    actions_["GlobalAutoMode"] =                    [this](string name, WidgetActionManager* manager, vector<string> params) { return new GlobalAutoMode(name, manager, params); };
}

void Manager::Init()
{
    pages_.clear();

    Page* currentPage = nullptr;
    
    string iniFilePath = string(DAW::GetResourcePath()) + "/CSI/CSI.ini";
    
    int lineNumber = 0;
    
    try
    {
        ifstream iniFile(iniFilePath);
        
        for (string line; getline(iniFile, line) ; )
        {
            vector<string> tokens(GetTokens(line));
            
            if(tokens.size() > 0) // ignore comment lines and blank lines
            {
                if(tokens[0] == PageToken)
                {
                    if(tokens.size() != 9)
                        continue;
                    
                    currentPage = new Page(tokens[1], tokens[2] == "FollowMCP" ? true : false, tokens[3] == "SynchPages" ? true : false, tokens[5] == "UseTrackColoring" ? true : false, atoi(tokens[6].c_str()), atoi(tokens[7].c_str()), atoi(tokens[8].c_str()));
                    pages_.push_back(currentPage);
                    
                    if(tokens[4] == "UseScrollLink")
                        currentPage->GetTrackNavigationManager()->SetScrollLink(true);
                    else
                        currentPage->GetTrackNavigationManager()->SetScrollLink(false);
                }
                else if(tokens[0] == MidiSurfaceToken || tokens[0] == OSCSurfaceToken)
                {
                    if(tokens.size() != 11 && tokens.size() != 12)
                        continue;
                    
                    int inPort = atoi(tokens[2].c_str());
                    int outPort = atoi(tokens[3].c_str());
                    
                    if(currentPage)
                    {
                        ControlSurface* surface = nullptr;
                        
                        if(tokens[0] == MidiSurfaceToken)
                            surface = new Midi_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForPort(inPort), GetMidiOutputForPort(outPort), tokens[6] == "UseZoneLink" ? true : false);
                        else if(tokens[0] == OSCSurfaceToken && tokens.size() > 11)
                            surface = new OSC_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], inPort, outPort, tokens[6] == "UseZoneLink" ? true : false, tokens[11]);

                        currentPage->AddSurface(surface);
                        
                        if(tokens[7] == "AutoMapSends")
                            surface->SetShouldMapSends(true);
                        
                        if(tokens[8] == "AutoMapFX")
                            surface->GetFXActivationManager()->SetShouldMapSelectedTrackFX(true);
                        
                        if(tokens[9] == "AutoMapFXMenu")
                            surface->GetFXActivationManager()->SetShouldMapSelectedTrackFXMenus(true);
                        
                        if(tokens[10] == "AutoMapFocusedFX")
                            surface->GetFXActivationManager()->SetShouldMapFocusedFX(true);
                    }
                }
            }
        }
    }
    catch (exception &e)
    {
        char buffer[250];
        sprintf(buffer, "Trouble in %s, around line %d\n", iniFilePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////
// Parsing end
//////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////
// Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* Widget::GetTrack()
{
    if(widgetActionManager_ != nullptr)
        return widgetActionManager_->GetTrack();
    else
        return nullptr;
}

void Widget::RequestUpdate()
{
    if(widgetActionManager_ != nullptr)
        widgetActionManager_->RequestUpdate();
}

void Widget::DoAction(double value)
{
    if( ! GetIsModifier())
        GetSurface()->GetPage()->InputReceived(this);

    if(widgetActionManager_ != nullptr)
        widgetActionManager_->DoAction(value);    
}

void Widget::DoRelativeAction(double value)
{
    DoAction(lastValue_ + value);
}

void Widget::SetIsTouched(bool isTouched)
{
    if(widgetActionManager_ != nullptr)
        widgetActionManager_->SetIsTouched(isTouched);
}

void  Widget::SetValue(double value)
{
    lastValue_ = value;
    
    for(auto feebackProcessor : feedbackProcessors_)
        feebackProcessor->SetValue(value);
}

void  Widget::SetValue(int mode, double value)
{
    lastValue_ = value;
    
    for(auto feebackProcessor : feedbackProcessors_)
        feebackProcessor->SetValue(mode, value);
}

void  Widget::SetValue(string value)
{
    for(auto feebackProcessor : feedbackProcessors_)
        feebackProcessor->SetValue(value);
}

void Widget::ClearCache()
{
    for(auto feedbackProcessor : feedbackProcessors_)
        feedbackProcessor->ClearCache();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_CSIMessageGenerator : public CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSC_CSIMessageGenerator::OSC_CSIMessageGenerator(OSC_ControlSurface* surface, Widget* widget, string message) : CSIMessageGenerator(widget)
{
    surface->AddCSIMessageGenerator(message, this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_FeedbackProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_FeedbackProcessor::SendMidiMessage(MIDI_event_ex_t* midiMessage)
{
    surface_->SendMidiMessage(midiMessage);
}

void Midi_FeedbackProcessor::SendMidiMessage(int first, int second, int third)
{
    if(first != lastMessageSent_->midi_message[0] || second != lastMessageSent_->midi_message[1] || third != lastMessageSent_->midi_message[2])
    {
        lastMessageSent_->midi_message[0] = first;
        lastMessageSent_->midi_message[1] = second;
        lastMessageSent_->midi_message[2] = third;
        surface_->SendMidiMessage(first, second, third);
    }
    else if(shouldRefresh_ && DAW::GetCurrentNumberOfMilliseconds() > lastRefreshed_ + refreshInterval_)
    {
        lastRefreshed_ = DAW::GetCurrentNumberOfMilliseconds();
        surface_->SendMidiMessage(first, second, third);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_FeedbackProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSC_FeedbackProcessor::SetValue(double value)
{
    if(lastFloatValue_ != value)
    {
        lastFloatValue_ = value;
        surface_->SendOSCMessage(oscAddress_, value);
    }
}

void OSC_FeedbackProcessor::SetValue(int param, double value)
{
    if(lastFloatValue_ != value)
    {
        lastFloatValue_ = value;
        surface_->SendOSCMessage(oscAddress_, value);
    }
}

void OSC_FeedbackProcessor::SetValue(string value)
{
    if(lastStringValue_ != value)
    {
        lastStringValue_ = value;
        surface_->SendOSCMessage(oscAddress_, value);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Action::Action(string name, WidgetActionManager* widgetActionManager) : name_(name), widgetActionManager_(widgetActionManager)
{
    page_ = widgetActionManager_->GetWidget()->GetSurface()->GetPage();
    widget_ = widgetActionManager_->GetWidget();
}

void Action::DoAction(double value)
{
    if(shouldIgnoreRelease_ && value == 0)
        return;
    
    value = isInverted_ == false ? value : 1.0 - value;
    
    if(shouldToggle_)
        Do( ! GetCurrentValue());
    else
        Do(value);
    
    if( ! widget_->GetIsModifier())
        page_->ActionPerformed(GetWidgetActionManager(), this);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WidgetActionManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string WidgetActionManager::GetModifiers()
{
    if(widget_->GetIsModifier())
        return NoModifiers; // Modifier Widgets cannot have Modifiers
    else
        return widget_->GetSurface()->GetPage()->GetModifiers();
}

bool WidgetActionManager::GetHasFocusedFXNavigator()
{
    if(trackNavigator_ == nullptr)
        return false;
    else
        return trackNavigator_->GetIsFocusedFXNavigator();
}

string WidgetActionManager::GetNavigatorName()
{
    if(trackNavigator_ == nullptr)
        return "";
    else
        return trackNavigator_->GetName();
}

MediaTrack* WidgetActionManager::GetTrack()
{
    if(trackNavigator_ == nullptr)
        return nullptr;
    else
        return trackNavigator_->GetTrack();
}

void WidgetActionManager::RequestUpdate()
{
    if(trackTouchedActions_.size() > 0 && trackNavigator_ != nullptr && trackNavigator_->GetIsChannelTouched())
    {
        for(auto action : trackTouchedActions_)
            action->RequestUpdate();
    }
    else
    {
        if(actions_.count(GetModifiers()) > 0)
            for(auto action : actions_[GetModifiers()])
                action->RequestUpdate();
    }
}

void WidgetActionManager::SetIsTouched(bool isTouched)
{
    if(trackNavigator_ != nullptr)
        trackNavigator_->SetTouchState((isTouched));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Zone::Activate()
{
    if(parentZoneName_ != "")
    {
        int index = surface_->GetParentZoneIndex(this);
        
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->Activate(index);
        
        for(auto zone : includedZones_)
            zone->Activate(index);
    }
    else
    {
        for(auto widgetActionManager : widgetActionManagers_)
            widgetActionManager->Activate();
        
        for(auto zone : includedZones_)
            zone->Activate();
    }
}

void Zone::Deactivate()
{
    for(auto widgetActionManager : widgetActionManagers_)
    {
        Widget* widget =  widgetActionManager->GetWidget();
        WidgetActionManager* manager = surface_->GetHomeWidgetActionManagerForWidget(widget);
        if(manager == nullptr)
            widget->Reset();
        widget->SetWidgetActionManager(manager);
    }
    
    for(auto zone : includedZones_)
        zone->Deactivate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SendsActivationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void SendsActivationManager::ToggleMapSends()
{
    shouldMapSends_ = ! shouldMapSends_;
    
    if( ! shouldMapSends_)
    {
        for(auto zone : activeSendZones_)
            zone->Deactivate();
        
        activeSendZones_.clear();
    }
    
    surface_->GetPage()->OnTrackSelection();
}

void SendsActivationManager::MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones)
{
    for(auto zone : activeSendZones_)
        zone->Deactivate();
    
    activeSendZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    int numTrackSends = DAW::GetTrackNumSends(selectedTrack, 0);
    
    for(int i = 0; i < numSendSlots_; i++)
    {
        string zoneName = "Send" + to_string(i + 1);
        
        if(shouldMapSends_ && zones.count(zoneName) > 0)
        {
            Zone* zone =  zones[zoneName];
            
            if(i < numTrackSends)
            {
                zone->Activate(i);
                activeSendZones_.push_back(zone);
            }
            else
            {
                zone->ActivateNoAction(i);
                activeSendZones_.push_back(zone);
                zone->SetWidgetsToZero();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FXActivationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXActivationManager::ToggleMapSelectedTrackFX()
{
    shouldMapSelectedTrackFX_ = ! shouldMapSelectedTrackFX_;
    
    if( ! shouldMapSelectedTrackFX_)
    {
        for(auto zone : activeSelectedTrackFXZones_)
        {
            surface_->LoadingZone(zone->GetName());
            zone->Deactivate();
        }
        
        activeSelectedTrackFXZones_.clear();
        
        DeleteFXWindows();
    }
    
    surface_->GetPage()->OnTrackSelection();
}

void FXActivationManager::ToggleMapFocusedFX()
{
    shouldMapFocusedFX_ = ! shouldMapFocusedFX_;
    
    MapFocusedFXToWidgets();
}

void FXActivationManager::ToggleMapSelectedTrackFXMenu()
{
    shouldMapSelectedTrackFXMenus_ = ! shouldMapSelectedTrackFXMenus_;
    
    if( ! shouldMapSelectedTrackFXMenus_)
    {
        for(auto zone : activeSelectedTrackFXMenuZones_)
        {
            surface_->LoadingZone(zone->GetName());
            zone->Deactivate();
        }

        activeSelectedTrackFXMenuZones_.clear();
    }

    surface_->GetPage()->OnTrackSelection();
}

void FXActivationManager::MapSelectedTrackFXToWidgets()
{
    for(auto zone : activeSelectedTrackFXZones_)
    {
        surface_->LoadingZone("Home");
        zone->Deactivate();
    }
    
    activeSelectedTrackFXZones_.clear();
    DeleteFXWindows();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    for(int i = 0; i < DAW::TrackFX_GetCount(selectedTrack); i++)
    {
        char FXName[BUFSZ];
        
        DAW::TrackFX_GetFXName(selectedTrack, i, FXName, sizeof(FXName));
        
        if(shouldMapSelectedTrackFX_ && surface_->GetZones().count(FXName) > 0 && ! surface_->GetZones()[FXName]->GetHasFocusedFXTrackNavigator())
        {
            Zone* zone = surface_->GetZones()[FXName];
            surface_->LoadingZone(FXName);
            zone->Activate(i);
            activeSelectedTrackFXZones_.push_back(zone);
            openFXWindows_.push_back(FXWindow(FXName, selectedTrack, i));
        }
    }
    
    OpenFXWindows();
}

void FXActivationManager::MapSelectedTrackFXToMenu()
{
    for(auto zone : activeSelectedTrackFXMenuZones_)
        zone->Deactivate();
    
    activeSelectedTrackFXMenuZones_.clear();
    
    for(auto zone : activeSelectedTrackFXMenuFXZones_)
        zone->Deactivate();
    
    activeSelectedTrackFXMenuFXZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    int numTrackFX = DAW::TrackFX_GetCount(selectedTrack);
    
    for(int i = 0; i < numFXlots_; i ++)
    {
        string zoneName = "FXMenu" + to_string(i + 1);
        
        if(shouldMapSelectedTrackFXMenus_ && surface_->GetZones().count(zoneName) > 0)
        {
            Zone* zone =  surface_->GetZones()[zoneName];
            
            if(i < numTrackFX)
            {
                zone->Activate(i);
                activeSelectedTrackFXMenuZones_.push_back(zone);
            }
            else
            {
                zone->ActivateNoAction(i);
                zone->SetWidgetsToZero();
                activeSelectedTrackFXMenuZones_.push_back(zone);
            }
        }
    }
}

void FXActivationManager::MapSelectedTrackFXSlotToWidgets(int fxIndex)
{
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    if(fxIndex >= DAW::TrackFX_GetCount(selectedTrack))
        return;
    
    char FXName[BUFSZ];
    DAW::TrackFX_GetFXName(selectedTrack, fxIndex, FXName, sizeof(FXName));
    
    if(surface_->GetZones().count(FXName) > 0 && ! surface_->GetZones()[FXName]->GetHasFocusedFXTrackNavigator())
    {
        surface_->GetZones()[FXName]->Activate(fxIndex);
        activeSelectedTrackFXMenuFXZones_.push_back(surface_->GetZones()[FXName]);
    }
}

void FXActivationManager::MapFocusedFXToWidgets()
{
    int trackNumber = 0;
    int itemNumber = 0;
    int fxIndex = 0;
    MediaTrack* focusedTrack = nullptr;
    
    if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &fxIndex) == 1)
        if(trackNumber > 0)
            focusedTrack = DAW::CSurf_TrackFromID(trackNumber, surface_->GetPage()->GetTrackNavigationManager()->GetFollowMCP());
    
    for(auto zone : activeFocusedFXZones_)
    {
        surface_->LoadingZone("Home");
        zone->Deactivate();
    }
    
    activeFocusedFXZones_.clear();
    
    if(shouldMapFocusedFX_ && focusedTrack)
    {
        char FXName[BUFSZ];
        DAW::TrackFX_GetFXName(focusedTrack, fxIndex, FXName, sizeof(FXName));
        if(surface_->GetZones().count(FXName) > 0 && surface_->GetZones()[FXName]->GetHasFocusedFXTrackNavigator())
        {
            Zone* zone = surface_->GetZones()[FXName];
            surface_->LoadingZone(FXName);
            zone->Activate(fxIndex);
            activeFocusedFXZones_.push_back(zone);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlSurface::ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, bool useZoneLink) : CSurfIntegrator_(CSurfIntegrator), page_(page), name_(name), useZoneLink_(useZoneLink)
{
    fxActivationManager_ = new FXActivationManager(this);
    sendsActivationManager_ = new SendsActivationManager(this);
}

void ControlSurface::InitZones(string zoneFolder)
{
    includedZoneMembers.clear();
    trackNavigators.clear();
    
    try
    {
        vector<string> zoneFilesToProcess;
        listZoneFiles(DAW::GetResourcePath() + string("/CSI/Zones/") + zoneFolder + "/", zoneFilesToProcess); // recursively find all the .zon files, starting at zoneFolder
        
        for(auto zoneFilename : zoneFilesToProcess)
            ProcessFile(zoneFilename, this, widgets_);
        
        // now add appropriate included zones to zones
        for(auto [includedZoneName, includedZones] : includedZoneMembers)
            if(zones_.count(includedZoneName) > 0)
                for(auto includedZone : includedZones)
                    includedZone->AddZone(zones_[includedZoneName]);
    }
    catch (exception &e)
    {
        char buffer[250];
        sprintf(buffer, "Trouble parsing Zone folders\n");
        DAW::ShowConsoleMsg(buffer);
    }
}

WidgetActionManager* ControlSurface::GetHomeWidgetActionManagerForWidget(Widget* widget)
{
    if(zones_.count("Home") > 0)
        return zones_["Home"]->GetHomeWidgetActionManagerForWidget(widget);
    else
        return nullptr;
}

string ControlSurface::GetZoneAlias(string zoneName)
{
    if(zones_.count(zoneName) > 0)
        return zones_[zoneName]->GetAlias();
    else
        return "";
}

string ControlSurface::GetLocalZoneAlias(string zoneName)
{
    if(GetZoneAlias(zoneName) != "")
        return GetZoneAlias(zoneName);
    else
        return page_->GetZoneAlias(zoneName);
}

int ControlSurface::GetParentZoneIndex(Zone* childZone)
{
    for(auto zone : fxActivationManager_->GetActiveZones())
        if(childZone->GetParentZoneName() == zone->GetName())
            return zone->GetZoneIndex();
    
    for(auto zone : sendsActivationManager_->GetActiveZones())
        if(childZone->GetParentZoneName() == zone->GetName())
            return zone->GetZoneIndex();
    
    return 0;
}

bool ControlSurface::AddZone(Zone* zone)
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

void ControlSurface::GoZone(string zoneName)
{
    if(zones_.count(zoneName) > 0)
    {
        zones_[zoneName]->Activate();
        activeZone_ = zones_[zoneName];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename, this, widgets_);
    
    // Add the "hardcoded" widgets
    widgets_.push_back(new Widget(this, "OnTrackSelection"));
    widgets_.push_back(new Widget(this, "OnFXFocus"));
}

void Midi_ControlSurface::ProcessMidiMessage(const MIDI_event_ex_t* evt)
{
    if(TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        sprintf(buffer, "IN -> %s %02x  %02x  %02x \n", name_.c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
        DAW::ShowConsoleMsg(buffer);
    }
    
    // At this point we don't know how much of the message comprises the key, so try all three
    if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]) > 0)
        CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]]->ProcessMidiMessage(evt);
    else if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100) > 0)
        CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100]->ProcessMidiMessage(evt);
    else if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000) > 0)
        CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000]->ProcessMidiMessage(evt);
}

void Midi_ControlSurface::SendMidiMessage(MIDI_event_ex_t* midiMessage)
{
    if(midiOutput_)
        midiOutput_->SendMsg(midiMessage, -1);
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s SysEx \n", name_.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
}

void Midi_ControlSurface::SendMidiMessage(int first, int second, int third)
{
    if(midiOutput_)
        midiOutput_->Send(first, second, third, -1);
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s %02x  %02x  %02x \n", name_.c_str(), first, second, third);
        DAW::ShowConsoleMsg(buffer);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
OSC_ControlSurface::OSC_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string templateFilename, string zoneFolder, int inPort, int outPort, bool useZoneLink, string remoteDeviceIP)
: ControlSurface(CSurfIntegrator, page, name, useZoneLink), inPort_(inPort), outPort_(outPort), remoteDeviceIP_(remoteDeviceIP)
{
    fxActivationManager_->SetShouldMapSelectedTrackFX(true);
    
    InitWidgets(templateFilename);
    
    ResetAllWidgets();
    
    // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
    InitZones(zoneFolder);
    
    runServer();
    
    GoZone("Home");
}

void OSC_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/OSC/" + templateFilename, this, widgets_);
    
    // Add the "hardcoded" widgets
    widgets_.push_back(new Widget(this, "OnTrackSelection"));
    widgets_.push_back(new Widget(this, "OnFXFocus"));
}

void OSC_ControlSurface::ProcessOSCMessage(string message, double value)
{
    if(CSIMessageGeneratorsByOSCMessage_.count(message) > 0)
        CSIMessageGeneratorsByOSCMessage_[message]->ProcessOSCMessage(message, value);
    
    if(TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        sprintf(buffer, "IN -> %s %s  %f  \n", name_.c_str(), message.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void OSC_ControlSurface::LoadingZone(string zoneName)
{
    string oscAddress(zoneName);
    oscAddress = regex_replace(oscAddress, regex(BadFileChars), "_");
    oscAddress = "/" + oscAddress;
    
    if(outSocket_.isOk())
    {
        oscpkt::Message message;
        message.init(oscAddress);
        packetWriter_.init().addMessage(message);
        outSocket_.sendPacket(packetWriter_.packetData(), packetWriter_.packetSize());
    }
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s %s \n", name_.c_str(), oscAddress.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
}

void OSC_ControlSurface::SendOSCMessage(string oscAddress, double value)
{
    if(outSocket_.isOk())
    {
        oscpkt::Message message;
        message.init(oscAddress).pushFloat(value);
        packetWriter_.init().addMessage(message);
        outSocket_.sendPacket(packetWriter_.packetData(), packetWriter_.packetSize());
    }
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s %s  %f  \n", name_.c_str(), oscAddress.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void OSC_ControlSurface::SendOSCMessage(string oscAddress, string value)
{
    if(outSocket_.isOk())
    {
        oscpkt::Message message;
        message.init(oscAddress).pushStr(value);
        packetWriter_.init().addMessage(message);
        outSocket_.sendPacket(packetWriter_.packetData(), packetWriter_.packetSize());
    }
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s %s  %s  \n", name_.c_str(), oscAddress.c_str(), value.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
void TrackNavigator::Pin()
{
    if( ! isChannelPinned_)
    {
        pinnedTrack_ = GetTrack();
        
        isChannelPinned_ = true;
        
        manager_->PinTrackToChannel(pinnedTrack_, channelNum_);
    }
}

void TrackNavigator::Unpin()
{
    if(isChannelPinned_)
    {
        manager_->UnpinTrackFromChannel(pinnedTrack_, channelNum_);
        
        isChannelPinned_ = false;
        
        pinnedTrack_ = nullptr;
    }
}

MediaTrack* TrackNavigator::GetTrack()
{
    if(isChannelPinned_)
        return pinnedTrack_;
    else
        return manager_->GetTrackFromChannel(channelNum_ - bias_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectedTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* SelectedTrackNavigator::GetTrack()
{
    return manager_->GetSelectedTrack();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FocusedFXNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* FocusedFXNavigator::GetTrack()
{
    int trackNumber = 0;
    int itemNumber = 0;
    int fxIndex = 0;
    
    if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &fxIndex) == 1) // Track FX
    {
        if(trackNumber > 0)
            return DAW::CSurf_TrackFromID(trackNumber, manager_->GetPage()->GetTrackNavigationManager()->GetFollowMCP());
        else
            return nullptr;
    }
    else
        return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackNavigator* TrackNavigationManager::AddTrackNavigator()
{
    int channelNum = trackNavigators_.size();
    trackNavigators_.push_back(new TrackNavigator(page_->GetTrackNavigationManager(), channelNum));
    return trackNavigators_[channelNum];
}

void TrackNavigationManager::OnTrackSelection()
{
    if(scrollLink_)
    {
        // Make sure selected track is visble on the control surface      
        MediaTrack* selectedTrack = GetSelectedTrack();
        
        if(selectedTrack != nullptr)
        {
            for(auto navigator : trackNavigators_)
                if(selectedTrack == navigator->GetTrack())
                    return;

            for(int i = 0; i < unpinnedTracks_.size(); i++)
                if(selectedTrack == unpinnedTracks_[i])
                    trackOffset_ = i;
            
            trackOffset_ -= targetScrollLinkChannel_;
            
            if(trackOffset_ <  0)
                trackOffset_ =  0;

            int top = GetNumTracks() - trackNavigators_.size();
            
            if(trackOffset_ >  top)
                trackOffset_ = top;
        }
    }
}

void TrackNavigationManager::OnTrackSelectionBySurface(MediaTrack* track)
{
    if(scrollLink_)
    {
        if(DAW::IsTrackVisible(track, true))
            DAW::SetMixerScroll(track); // scroll selected MCP tracks into view
        
        if(DAW::IsTrackVisible(track, false))
            DAW::SendCommandMessage(40913); // scroll selected TCP tracks into view
    }
}

void TrackNavigationManager::AdjustTrackBank(int amount)
{
    int numTracks = GetNumTracks();

    if(numTracks <= trackNavigators_.size())
        return;
    
    trackOffset_ += amount;
    
    if(trackOffset_ <  0)
        trackOffset_ =  0;
    
    int top = numTracks - trackNavigators_.size();
    
    if(trackOffset_ >  top)
        trackOffset_ = top;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Learn Mode
////////////////////////////////////////////////////////////////////////////////////////////////////////
void AddComboBoxEntry(HWND hwndDlg, int x, string entryName, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)entryName.c_str());
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
}

char name[BUFSZ];
HWND hwndLearn = nullptr;
Widget* currentWidget = nullptr;
WidgetActionManager* currentWidgetActionManager = nullptr;
Action* currentAction = nullptr;
int actionListSize = 0;

bool isShift = false;
bool isOption = false;
bool isControl = false;
bool isAlt = false;

struct LM_ZoneEntry
{
    vector<string> modifiers;
    bool isShift = false;
    bool isOption = false;
    bool isControl = false;
    bool isAlt = false;

    bool isTouch = false;
    bool shouldToggle = false;
    bool shouldIgnoreRelease = false;
    bool isHold = false;
    bool isInvert = false;
    
    string widgetName = "";
    string action = "";
    string param = "";
    string alias = "";
};

struct LM_Zone
{
    string name = "";
    string parentZone = "";
    string navigator = "";
    vector<string> includedZones;
    vector<LM_ZoneEntry> zoneEntries;
};

vector<LM_Zone> zones;

static void GetEntryWidgetNameAndModifiers(string line, LM_ZoneEntry &entry)
{
    string widgetName = "";
    string modifiers = "";
    bool isTrackTouch = false;
    bool isInverted = false;
    bool shouldToggle = false;
    bool shouldIgnoreRelease = false;
    double delayAmount = 0.0;

    GetWidgetNameAndModifiers(line, widgetName, modifiers, isTrackTouch, isInverted, shouldToggle, shouldIgnoreRelease, delayAmount);
    
    entry.widgetName = widgetName;
    entry.isTouch = isTrackTouch;
    entry.isInvert = isInverted;
    entry.shouldToggle = shouldToggle;
    entry.shouldIgnoreRelease = shouldIgnoreRelease;
    entry.isHold = delayAmount == 0.0 ? false : true;
    
    size_t found = line.find(Shift);
    if (found != string::npos)
        entry.isShift = true;
    
    found = line.find(Option);
    if (found != string::npos)
        entry.isOption = true;
    
    found = line.find(Control);
    if (found != string::npos)
        entry.isControl = true;
    
    found = line.find(Alt);
    if (found != string::npos)
        entry.isAlt = true;
}

static bool LoadRawFXFile(HWND hwndDlg)
{
    MediaTrack* track = currentWidget->GetTrack();
    Zone* zone = currentWidgetActionManager->GetZone();
    int index = zone->GetZoneIndex();
    
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    
    DAW::TrackFX_GetFXName(track, index, fxName, sizeof(fxName));
    
    
    string filename(fxName);
    filename = regex_replace(filename, regex(BadFileChars), "_");
    filename += ".txt";
    
    string filePath = string(DAW::GetResourcePath()) + "/CSI/Zones/ZoneRawFXFiles/" + filename;
    
    ifstream fileExists(filePath);
    
    if( ! fileExists)
    {
        ofstream rawFXFile(filePath);
        
        if(rawFXFile.is_open())
        {
            rawFXFile << string(fxName);
            
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, index); j++)
            {
                DAW::TrackFX_GetParamName(track, index, j, fxParamName, sizeof(fxParamName));

                rawFXFile << "\n" + string(fxParamName);
            }
        }
        
        rawFXFile.close();
    }
    
    ifstream rawFXFile(filePath);
    
    if(!rawFXFile)
        return false;
    
    int rawFileLineIndex = 0;
    
    for (string line; getline(rawFXFile, line) ; )
    {
        if(line == zone->GetName())
            continue;
        
        string actionName = to_string(rawFileLineIndex) + " - " + line;
        
        SendDlgItemMessage(hwndDlg, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)actionName.c_str());
        rawFileLineIndex++;
        actionListSize++;
    }
    
    return true;
}

static WDL_DLGRET dlgProcLearn(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_USER+1024:
        {
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, "");
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, "");
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, "");
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 0, 0);

            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_ZoneFilename), "");
            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_CurrentZone), "");
            
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_RESETCONTENT, 0, 0);
            
            SetDlgItemText(hwndDlg, IDC_EDIT_WidgetName, currentWidget->GetName().c_str());
            SetDlgItemText(hwndDlg, IDC_STATIC_SurfaceName, currentWidget->GetSurface()->GetName().c_str());
            
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_RESETCONTENT, 0, 0);
            
            for(auto widget : currentWidget->GetSurface()->GetWidgets())
                SendDlgItemMessage(hwndDlg, IDC_LIST_WidgetNames, LB_ADDSTRING, 0, (LPARAM)widget->GetName().c_str());
            
            for(int i = 0; i < currentWidget->GetSurface()->GetWidgets().size(); i++)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(name));
                if(string(name) == currentWidget->GetName())
                {
                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_SETCURSEL, i, 0);
                    break;
                }
            }
        }
            break;
            
        case WM_USER+1025:
        {
            actionListSize = 0;
            zones.clear();
            
            bool hasLoadedRawFXFile = false;
            
            if(isShift)
                CheckDlgButton(hwndDlg, IDC_CHECK_Shift, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_Shift, BST_UNCHECKED);
            
            if(isOption)
                CheckDlgButton(hwndDlg, IDC_CHECK_Option, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_Option, BST_UNCHECKED);
            
            if(isControl)
                CheckDlgButton(hwndDlg, IDC_CHECK_Control, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_Control, BST_UNCHECKED);
            
            if(isAlt)
                CheckDlgButton(hwndDlg, IDC_CHECK_Alt, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_Alt, BST_UNCHECKED);
            
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, currentAction->GetName().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, currentAction->GetParamAsString().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, currentAction->GetAlias().c_str());
            
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_RESETCONTENT, 0, 0);
            
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 0, 0);

            int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_FINDSTRING, -1, (LPARAM)currentWidgetActionManager->GetNavigatorName().c_str());
            if(index >= 0)
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, index, 0);

            Zone* zone = currentWidgetActionManager->GetZone();
            
            istringstream filePath(zone->GetSourceFilePath());
            vector<string> filePath_tokens;
            string filePathComponent;
            
            while (getline(filePath, filePathComponent, '/'))
                filePath_tokens.push_back(filePathComponent);
            
            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_ZoneFilename), filePath_tokens[filePath_tokens.size() - 1].c_str());
            
            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_CurrentZone), zone->GetName().c_str());

            bool isInIncludedZonesSection = false;
            
            int lineNumber = 0;
            
            try
            {
                ifstream file(zone->GetSourceFilePath());
                
                for (string line; getline(file, line) ; )
                {
                    lineNumber++;
                    
                    if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
                        continue;
                    
                    vector<string> tokens(GetTokens(line));
                    
                    if(tokens.size() > 0)
                    {
                        if(tokens[0] == "Zone")
                        {
                            if(tokens.size() > 1)
                            {
                                SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_ADDSTRING, 0, (LPARAM)tokens[1].c_str());
                                
                                LM_Zone newZone;
                                newZone.name = tokens[1];
                                zones.push_back(newZone);
                            }
                        }
                        else if(tokens[0] == "ZoneEnd")
                        {
                            continue;
                        }
                        else if(tokens[0] == "IncludedZones")
                        {
                            isInIncludedZonesSection = true;
                        }
                        else if(tokens[0] == "IncludedZonesEnd")
                        {
                            isInIncludedZonesSection = false;
                        }
                        else if(tokens[0] == "TrackNavigator" || tokens[0] == "SelectedTrackNavigator" || tokens[0] == "FocusedFXNavigator")
                        {
                            if(tokens[0] == "TrackNavigator")
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 1, 0);
                            
                            else if(tokens[0] == "SelectedTrackNavigator")
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 2, 0);
                            
                            else if(tokens[0] == "FocusedFXNavigator")
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 3, 0);
                            
                            if(zones.size() > 0)
                                zones[zones.size() - 1].navigator = tokens[0];
                        }
                        else if(isInIncludedZonesSection)
                        {
                            SendDlgItemMessage(hwndDlg, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)tokens[0].c_str());
                            if(zones.size() > 0)
                                zones[zones.size() - 1].includedZones.push_back(tokens[0]);
                        }
                        else if(tokens[0] == "ParentZone")
                        {
                            // TBD select right one from dropdown -- needs two pass
                            
                            if(tokens.size() > 1 && zones.size() > 0)
                                zones[zones.size() - 1].parentZone = tokens[1];
                        }
                        else if(tokens.size() > 2 && (tokens[1] == "FXParam" || tokens[1] == "FXParamNameDisplay" || tokens[1] == "FXParamValueDisplay" || tokens[1] == "FXParamRelative"))
                        {
                            if(hasLoadedRawFXFile == false && zones.size() > 0 && zones[zones.size() - 1].name != "")
                                hasLoadedRawFXFile = LoadRawFXFile(hwndDlg);
                        
                            string zoneComponentEntry = tokens[0] + " " + tokens[1] + " " + tokens[2];
                           
                            if(tokens.size() > 3)
                                zoneComponentEntry += " " + tokens[3];
                            
                            SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)zoneComponentEntry.c_str());
                            
                            if(zones.size() > 0)
                            {
                                LM_ZoneEntry entry;
                                GetEntryWidgetNameAndModifiers(tokens[0], entry);
                                entry.action = tokens[1];
                                entry.param = tokens[2];
                                if(tokens.size() > 3)
                                    entry.alias = tokens[3];
                                
                                zones[zones.size() - 1].zoneEntries.push_back(entry);
                            }
                        }
                        else
                        {
                            SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)line.c_str());

                            if(tokens.size() > 1 && zones.size() > 0)
                            {
                                LM_ZoneEntry entry;
                                GetEntryWidgetNameAndModifiers(tokens[0], entry);
                                entry.action = tokens[1];
                                if(tokens.size() > 2)
                                    entry.param = tokens[2];
                                if(tokens.size() > 3)
                                    entry.alias = tokens[3];
                                
                                zones[zones.size() - 1].zoneEntries.push_back(entry);
                            }
                        }
                    }
                }
            }
            catch (exception &e)
            {
                char buffer[250];
                sprintf(buffer, "Trouble loadong Zone file\n");
                DAW::ShowConsoleMsg(buffer);
            }

            for(auto name : TheManager->GetActionNames())
                if(name != Shift && name != Option && name != Control && name != Alt)
                {
                    SendDlgItemMessage(hwndDlg, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)name.c_str());
                    actionListSize++;
                }
            
            if(currentAction->GetName() == "FXParam" || currentAction->GetName() == "FXParamNameDisplay" || currentAction->GetName() == "FXParamValueDisplay" || currentAction->GetName() == "FXParamRelative")
                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, currentAction->GetParam(), 0);
            else
            {
                for(int i = 0; i < actionListSize; i++)
                {
                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(name));
                    if(string(name) == currentAction->GetName())
                    {
                        SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, i, 0);
                        break;
                    }
                }
            }

            string testString = currentWidget->GetName() + " " + currentAction->GetName();
            char lineStringBuf[BUFSZ];

            if(zones.size() > 0)
            {
                for(int i = 0; i < zones[zones.size() - 1].zoneEntries.size(); i++)
                {
                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(lineStringBuf));
                    
                    string lineString = string(lineStringBuf);
                    
                    size_t found = lineString.find(testString);
                    
                    if (found != string::npos)
                    {
                        SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_SETCURSEL, i, 0);
                        break;
                    }
                }
            }

            for(int i = 0; i < zones.size(); i++)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(lineStringBuf));
                
                string lineString = string(lineStringBuf);
                
                if (lineString == zone->GetName())
                {
                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_SETCURSEL, i, 0);
                    break;
                }
            }
        }
            break;
            
        case WM_INITDIALOG:
        {
            AddComboBoxEntry(hwndDlg, 0, "None", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 1, "TrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 2, "SelectedTrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 3, "FocusedFXNavigator", IDC_COMBO_Navigator);
            
            if(TheManager->GetSurfaceInMonitor())
                CheckDlgButton(hwndDlg, IDC_CHECK_SurfaceInMon, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_SurfaceInMon, BST_UNCHECKED);
            
            if(TheManager->GetSurfaceOutMonitor())
                CheckDlgButton(hwndDlg, IDC_CHECK_SurfaceOutMon, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_SurfaceOutMon, BST_UNCHECKED);
            
            if(TheManager->GetFXMonitor())
                CheckDlgButton(hwndDlg, IDC_CHECK_FXParamMon, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_CHECK_FXParamMon, BST_UNCHECKED);
            
        }
            break;
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_CHECK_SurfaceInMon:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SurfaceInMon))
                        TheManager->SetSurfaceInMonitor(true);
                    else
                        TheManager->SetSurfaceInMonitor(false);
                }
                    break;
                    
                case IDC_CHECK_SurfaceOutMon:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SurfaceOutMon))
                        TheManager->SetSurfaceOutMonitor(true);
                    else
                        TheManager->SetSurfaceOutMonitor(false);
                }
                    break;
                    
                case IDC_CHECK_FXParamMon:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_FXParamMon))
                        TheManager->SetFXMonitor(true);
                    else
                        TheManager->SetFXMonitor(false);
                }
                    break;
                    
                case IDC_BUTTON_Close:
                {
                    TheManager->CloseLearnModeWindow();
                }
                    break;


                    
                    /*
                     case IDC_RADIO_MCP:
                     CheckDlgButton(hwndDlg, IDC_RADIO_TCP, BST_UNCHECKED);
                     break;
                     
                     case IDC_RADIO_TCP:
                     CheckDlgButton(hwndDlg, IDC_RADIO_MCP, BST_UNCHECKED);
                     break;
                     
                     case IDOK:
                     if (HIWORD(wParam) == BN_CLICKED)
                     {
                     GetDlgItemText(hwndDlg, IDC_EDIT_PageName , name, sizeof(name));
                     
                     if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_MCP))
                     followMCP = true;
                     else
                     followMCP = false;
                     
                     if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SynchPages))
                     synchPages = true;
                     else
                     synchPages = false;
                     
                     if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ScrollLink))
                     useScrollLink = true;
                     else
                     useScrollLink = false;
                     
                     if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_ColourTracks))
                     trackColouring = true;
                     else
                     trackColouring = false;
                     
                     dlgResult = IDOK;
                     EndDialog(hwndDlg, 0);
                     }
                     break ;
                     
                     case IDCANCEL:
                     if (HIWORD(wParam) == BN_CLICKED)
                     EndDialog(hwndDlg, 0);
                     break ;
                     */
            }
        }
            break ;
            
        case WM_CLOSE:
            DestroyWindow(hwndDlg) ;
            break ;
            
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            break;
            
        default:
            return DefWindowProc(hwndDlg, uMsg, wParam, lParam) ;
    }
    
    return 0 ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Page
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Page::OpenLearnModeWindow()
{
    if(hwndLearn == nullptr)
    {
        hwndLearn = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Learn), g_hwnd, dlgProcLearn);
        ShowWindow(hwndLearn, true);
    }
}

void Page::CloseLearnModeWindow()
{
    if(hwndLearn != nullptr)
    {
        ShowWindow(hwndLearn, false);
        DestroyWindow(hwndLearn);
        hwndLearn = nullptr;
    }
}

void Page::ToggleLearnMode()
{
    if(hwndLearn == nullptr)
    {
        hwndLearn = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Learn), g_hwnd, dlgProcLearn);
        ShowWindow(hwndLearn, true);
    }
    else
    {
        ShowWindow(hwndLearn, false);
        DestroyWindow(hwndLearn);
        hwndLearn = nullptr;
    }
}

void Page::InputReceived(Widget* widget)
{
    if(hwndLearn == nullptr)
        return;
    
    currentWidget = widget;
    currentWidgetActionManager = nullptr;
    currentAction = nullptr;

    SendMessage(hwndLearn, WM_USER+1024, 0, 0);
}

void Page::ActionPerformed(WidgetActionManager* widgetActionManager, Action* action)
{
    if(hwndLearn == nullptr)
        return;

    if(widgetActionManager == nullptr || action == nullptr)
        return;
    
    currentWidget = widgetActionManager->GetWidget();
    currentWidgetActionManager = widgetActionManager;
    currentAction = action;
    
    isShift = isShift_;
    isOption = isOption_;
    isControl = isControl_;
    isAlt = isAlt_;

    SendMessage(hwndLearn, WM_USER+1025, 0, 0);
}
