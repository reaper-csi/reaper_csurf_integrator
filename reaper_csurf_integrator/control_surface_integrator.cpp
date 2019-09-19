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

string GetLineEnding()
{
#ifdef WIN32
    return "\n";
#else
    return "\r\n" ;
#endif
}

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

static void ExpandZone(vector<string> tokens, string filePath, vector<Zone*> &expandedZones, vector<string> &expandedZonesIds, ControlSurface* surface, string alias)
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

static void ExpandIncludedZone(vector<string> tokens, vector<string> &expandedZones)
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

static void ProcessIncludedZones(int &lineNumber, ifstream &zoneFile, string filePath, Zone* zone)
{
    for (string line; getline(zoneFile, line) ; )
    {
        line = regex_replace(line, regex(CRLFChars), "");

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

static map<string, TrackNavigator*> trackNavigators;

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
                modifierSlots[0] = Shift + "+";
            else if(modifier_tokens[i] == Option)
                modifierSlots[1] = Option + "+";
            else if(modifier_tokens[i] == Control)
                modifierSlots[2] = Control + "+";
            else if(modifier_tokens[i] == Alt)
                modifierSlots[3] = Alt + "+";
            
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

static void ProcessZone(int &lineNumber, ifstream &zoneFile, vector<string> passedTokens, string filePath, ControlSurface* surface, vector<Widget*> &widgets)
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
        line = regex_replace(line, regex(CRLFChars), "");

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
                            widgetActionManagerForWidget[widget]->AddTrackTouchedAction(modifiers, action);
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

static void ProcessMidiWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  Midi_ControlSurface* surface, vector<Widget*> &widgets)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(surface, tokens[1]);
    widgets.push_back(widget);
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        line = regex_replace(line, regex(CRLFChars), "");

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

static void ProcessOSCWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  OSC_ControlSurface* surface, vector<Widget*> &widgets)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(surface, tokens[1]);
    widgets.push_back(widget);
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        line = regex_replace(line, regex(CRLFChars), "");

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

static void ProcessFile(string filePath, ControlSurface* surface, vector<Widget*> &widgets)
{
    int lineNumber = 0;
    
    try
    {
        ifstream file(filePath);
        
        for (string line; getline(file, line) ; )
        {
            line = regex_replace(line, regex(CRLFChars), "");

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
            line = regex_replace(line, regex(CRLFChars), "");
            
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
        GetSurface()->GetPage()->InputReceived(this, value);

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
        if(trackTouchedActions_.count(GetModifiers()) > 0)
            for(auto action : trackTouchedActions_[GetModifiers()])
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

void WidgetActionManager::Deactivate()
{
    WidgetActionManager* managerForHome = widget_->GetSurface()->GetHomeWidgetActionManagerForWidget(widget_);
    if(managerForHome == nullptr)
        widget_->Reset();
    widget_->SetWidgetActionManager(managerForHome);
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
        widgetActionManager->Deactivate();
    
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
        zonesInZoneFile_[zone->GetSourceFilePath()].push_back(zone);
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
static void AddComboBoxEntry(HWND hwndDlg, int x, string entryName, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)entryName.c_str());
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
}

static char buffer[BUFSZ * 4];
static HWND hwndLearn = nullptr;
static bool hasEdits = false;
static int dlgResult = 0;
static ControlSurface* currentSurface = nullptr;
static Widget* currentWidget = nullptr;
static WidgetActionManager* currentWidgetActionManager = nullptr;
static Action* currentAction = nullptr;

static vector<Zone*> zonesInThisFile;

static string newZoneFilename = "";
static string newZoneName = "";
static string newZoneAlias = "";

// Modifiers
static bool isShift = false;
static bool isOption = false;
static bool isControl = false;
static bool isAlt = false;

static bool isTouch = false;
static bool shouldToggle = false;
static bool shouldIgnoreRelease = false;
static bool isHold = false;
static bool isInvert = false;

// Focused FX
static int trackNumber = 0;
static int itemNumber = 0;
static int focusedFXIndex = 0;
static MediaTrack* focusedFXTrack = nullptr;
static string focusedFXName = "";

// Guards for Set Current Selection messages
static bool widgetNameWasSelectedBySurface = false;
static bool actionNameWasSelectedBySurface = false;
static bool zoneComponentWasSelectedBySurface = false;
static bool zoneWasSelectedBySurface = false;

static void EnableButtons()
{
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_GenerateZoneEntry), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteZoneEntry), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddIncludedZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteIncludedZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_NewFile), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_SaveFile), true);
}

static void DisableButtons()
{
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_GenerateZoneEntry), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteZoneEntry), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddZone), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteZone), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddIncludedZone), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteIncludedZone), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_NewFile), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_SaveFile), false);
}

static vector<Zone*> GetAvailableZones(int zoneIndex)
{
    vector<Zone*> availableZones;
    
    if(zoneIndex >= 0)
    {       
        for(int i = 0; i < zonesInThisFile.size(); i++)
        {
            bool foundIt = false;

            for(int j = 0; j < zonesInThisFile[zoneIndex]->GetIncludedZones().size(); j++)
            {
                if(zonesInThisFile[zoneIndex]->GetIncludedZones()[j]->GetName() == zonesInThisFile[i]->GetName())
                {
                    foundIt = true;
                    break;
                }
            }

            if( ! foundIt && i != zoneIndex)
                availableZones.push_back(zonesInThisFile[i]);
        }
    }
    
    return availableZones;
}

static vector<Zone*> GetAvailableZones()
{
    int zoneIndex = (int)SendMessage(GetDlgItem(hwndLearn, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
    
    return GetAvailableZones(zoneIndex);
}

static void SetCheckBoxes(ActionLineItem lineItem)
{
    if(lineItem.isShift)
        CheckDlgButton(hwndLearn, IDC_CHECK_Shift, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Shift, BST_UNCHECKED);
    
    if(lineItem.isOption)
        CheckDlgButton(hwndLearn, IDC_CHECK_Option, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Option, BST_UNCHECKED);
    
    if(lineItem.isControl)
        CheckDlgButton(hwndLearn, IDC_CHECK_Control, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Control, BST_UNCHECKED);
    
    if(lineItem.isAlt)
        CheckDlgButton(hwndLearn, IDC_CHECK_Alt, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Alt, BST_UNCHECKED);
    
    if(lineItem.isToggle)
        CheckDlgButton(hwndLearn, IDC_CHECK_Toggle, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Toggle, BST_UNCHECKED);
    
    if(lineItem.isInvert)
        CheckDlgButton(hwndLearn, IDC_CHECK_Invert, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Invert, BST_UNCHECKED);
    
    if(lineItem.isTouch)
        CheckDlgButton(hwndLearn, IDC_CHECK_Touch, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Touch, BST_UNCHECKED);
    
    if(lineItem.isPress)
        CheckDlgButton(hwndLearn, IDC_CHECK_IgnoreRelease, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_IgnoreRelease, BST_UNCHECKED);
    
    if(lineItem.isHold)
        CheckDlgButton(hwndLearn, IDC_CHECK_Hold, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Hold, BST_UNCHECKED);
}

static bool LoadRawFXFile(MediaTrack* track, string zoneName)
{
    string zoneFilename(zoneName);
    zoneFilename = regex_replace(zoneFilename, regex(BadFileChars), "_");
    zoneFilename += ".txt";
    
    string filePath = string(DAW::GetResourcePath()) + "/CSI/Zones/ZoneRawFXFiles/" + zoneFilename;

    try
    {
        ifstream fileExists(filePath);
        
        if(fileExists)
        {
            fileExists.close();
        }
        else
        {
            for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
            {
                DAW::TrackFX_GetFXName(track, i, buffer, sizeof(buffer));
                
                if(string(buffer) == zoneName)
                {
                    ofstream rawFXFile(filePath);
                    
                    if(rawFXFile.is_open())
                    {
                        rawFXFile << string(zoneName) + GetLineEnding();
                        
                        for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                        {
                            DAW::TrackFX_GetParamName(track, i, j, buffer, sizeof(buffer));
                            
                            rawFXFile << string(buffer) + GetLineEnding();
                        }
                    }
                    
                    rawFXFile.close();
                }
            }
        }
        
        ifstream rawFXFile(filePath);
        
        if(!rawFXFile)
            return false;
        
        int rawFileLineIndex = 0;
        
        for (string line; getline(rawFXFile, line) ; )
        {
            line = regex_replace(line, regex(CRLFChars), "");
            
            if(line == zoneName)
                continue;
            
            string actionName = to_string(rawFileLineIndex) + " - " + line;
            
            SendDlgItemMessage(hwndLearn, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)actionName.c_str());
            rawFileLineIndex++;
        }
        
        return true;
    }
    catch (exception &e)
    {
        char buffer[250];
        sprintf(buffer, "Trouble loading Raw FX file %s", filePath.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
    
    return false;
}

static void ClearWidgets()
{
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_WidgetNames), LB_RESETCONTENT, 0, 0);
    SetDlgItemText(hwndLearn, IDC_EDIT_WidgetName, "");
    SetDlgItemText(hwndLearn, IDC_STATIC_SurfaceName, "");
}

static void ClearSubZones()
{
    SendMessage(GetDlgItem(hwndLearn, IDC_COMBO_Navigator), CB_SETCURSEL, 0, 0);
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ZoneComponents), LB_RESETCONTENT, 0, 0);
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
    SendMessage(GetDlgItem(hwndLearn, IDC_COMBO_ParentZone), CB_RESETCONTENT, 0, 0);
    AddComboBoxEntry(hwndLearn, 0, "No Parent Zone", IDC_COMBO_ParentZone);
}

static void ClearZones()
{
    SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_ZoneFilename), "");
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_Zones), LB_RESETCONTENT, 0, 0);
}

static void ClearActions()
{
    SetDlgItemText(hwndLearn, IDC_EDIT_ActionName, "");
    SetDlgItemText(hwndLearn, IDC_EDIT_ActionParameter, "");
    SetDlgItemText(hwndLearn, IDC_EDIT_ActionAlias, "");
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_RESETCONTENT, 0, 0);
}

static int FillZones(Zone* zone)
{
    // Zone Filename
    smatch match;
    string zoneFilename = zone->GetSourceFilePath();
    if (regex_search(zoneFilename, match, regex("[^/]+$)")) == true)
    {
        zoneFilename = match.str(0);
        SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_ZoneFilename), zoneFilename.c_str());
    }
    
    // Zones
    int zoneIndex = -1;
    
    zonesInThisFile = currentWidget->GetSurface()->GetZonesInZoneFile()[zone->GetSourceFilePath()];
    
    for(int i = 0; i < zonesInThisFile.size(); i++)
    {
        SendDlgItemMessage(hwndLearn, IDC_LIST_Zones, LB_ADDSTRING, 0, (LPARAM)zonesInThisFile[i]->GetName().c_str());
        
        if(zonesInThisFile[i]->GetName() == zone->GetName())
        {
            zoneWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_Zones), LB_SETCURSEL, i, 0);
            zoneIndex = i;
        }
    }

    return zoneIndex;
}

static void FillSubZones(Zone* zone, int zoneIndex)
{
    ClearSubZones();
    
    bool hasLoadedRawFXFile = false;

    // Navigator
    string navigatorName = currentWidgetActionManager->GetNavigatorName();
    
    if(navigatorName == "")
        navigatorName = "No Navigator";
    
    int index = SendMessage(GetDlgItem(hwndLearn, IDC_COMBO_Navigator), CB_FINDSTRING, -1, (LPARAM)navigatorName.c_str());
    if(index >= 0)
        SendMessage(GetDlgItem(hwndLearn, IDC_COMBO_Navigator), CB_SETCURSEL, index, 0);

    // Parent Zone
    if(zoneIndex >= 0)
    {
        SendMessage(GetDlgItem(hwndLearn, IDC_COMBO_ParentZone), CB_SETCURSEL, 0, 0);
        
        for(int i = 0; i < GetAvailableZones(zoneIndex).size(); i++)
        {
            Zone* availableZone = GetAvailableZones(zoneIndex)[i];
            AddComboBoxEntry(hwndLearn, 0, availableZone->GetName().c_str(), IDC_COMBO_ParentZone);
            
            if(zone->GetParentZoneName() == availableZone->GetName())
            {
                SendMessage(GetDlgItem(hwndLearn, IDC_COMBO_ParentZone), CB_SETCURSEL, i + 1, 0);
            }
        }
    }
    
    // Included Zones
    for(auto includedZone : zone->GetIncludedZones())
        SendDlgItemMessage(hwndLearn, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)includedZone->GetName().c_str());
    
    // Line Items
    vector<ActionLineItem> actionLineItems = zone->GetActionLineItems();
    
    for(int i = 0; i < actionLineItems.size(); i++)
    {
        ActionLineItem lineItem = actionLineItems[i];
        
        if (lineItem.actionName == "FXParam" && hasLoadedRawFXFile == false)
            hasLoadedRawFXFile = LoadRawFXFile(currentWidget->GetTrack(), zone->GetName());
        
        if(hasLoadedRawFXFile && lineItem.actionName == "FXParam" && lineItem.param == currentAction->GetParamAsString())
        {
            actionNameWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_SETCURSEL, currentAction->GetParam(), 0);
        }
        
        string lineString = lineItem.allModifiers + lineItem.widgetName + " " + lineItem.actionName;
        
        if(lineItem.param != "")
            lineString += " " + lineItem.param;
        
        if(lineItem.alias != "")
            lineString += " " + lineItem.alias;
        
        SendDlgItemMessage(hwndLearn, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)lineString.c_str());
        
        string currentModifiers = "";
        
        if(isShift)
            currentModifiers += "Shift+";
        
        if(isOption)
            currentModifiers += "Option+";
        
        if(isControl)
            currentModifiers += "Control+";
        
        if(isAlt)
            currentModifiers += "Alt+";
        
        if(currentModifiers == lineItem.modifiers && currentWidget->GetName() == lineItem.widgetName)
        {
            zoneComponentWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ZoneComponents), LB_SETCURSEL, i, 0);
            
            SetCheckBoxes(lineItem);
        }
    }
}

static void FillActionNames()
{
    vector<string> actionNames = TheManager->GetActionNames();
    
    int negBias = 0;
    
    for(int i = 0; i < actionNames.size(); i++)
    {
        string name = actionNames[i];
        
        if(name == Shift || name == Option || name == Control || name == Alt)
            negBias++;
        else
        {
            SendDlgItemMessage(hwndLearn, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)name.c_str());
            
            if(name != "FXParam" && currentAction->GetName() == name)
            {
                actionNameWasSelectedBySurface = true;
                SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_SETCURSEL, i - negBias, 0);
            }
        }
    }
}

static WDL_DLGRET dlgProcAddZone(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneName, buffer, sizeof(buffer));
                        newZoneName = string(buffer);
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneAlias, buffer, sizeof(buffer));
                        newZoneAlias = string(buffer);
                        
                        dlgResult = IDOK;
                        EndDialog(hwndDlg, 0);
                    }
                    break ;
                    
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EndDialog(hwndDlg, 0);
                    break ;
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

static WDL_DLGRET dlgProcAddIncludedZone(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_IncludedZone), CB_RESETCONTENT, 0, 0);
            for(auto zone : GetAvailableZones())
            {
                AddComboBoxEntry(hwndDlg, 0, zone->GetName().c_str(), IDC_COMBO_IncludedZone);
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_IncludedZone), CB_SETCURSEL, 0, 0);
            }
            
            break;
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_IncludedZone), CB_GETCURSEL, 0, 0);
                        
                        if(index >= 0)
                        {
                            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_IncludedZone), CB_GETLBTEXT, index, (LPARAM)(LPCTSTR)(buffer));
                           
                            if(string(buffer) != "")
                                SendDlgItemMessage(hwndLearn, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)buffer);
                            
                            // GAW TBD -- add included Zone to Surface
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                //zones[index].includedZones.push_back(string(buffer));
                            }
                        }
                        
                        dlgResult = IDOK;
                        EndDialog(hwndDlg, 0);
                    }
                    break ;
                    
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EndDialog(hwndDlg, 0);
                    break ;
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

static WDL_DLGRET dlgProcNewZoneFile(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &focusedFXIndex) == 1)
                if(trackNumber > 0)
                {
                    focusedFXTrack = DAW::CSurf_TrackFromID(trackNumber, currentSurface->GetPage()->GetTrackNavigationManager()->GetFollowMCP());
                
                    DAW::TrackFX_GetFXName(focusedFXTrack, focusedFXIndex, buffer, sizeof(buffer));

                    focusedFXName = string(buffer);
                    
                    string focusedFXFilename =  regex_replace(focusedFXName, regex(BadFileChars), "_");

                    SetDlgItemText(hwndDlg, IDC_EDIT_ZoneFileName, focusedFXFilename.c_str());
                    SetDlgItemText(hwndDlg, IDC_EDIT_ZoneName, focusedFXName.c_str());
                }
        }
            break;
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        ClearZones();
                        ClearActions();

                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneFileName , buffer, sizeof(buffer));
                        newZoneFilename = string(buffer);
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneName , buffer, sizeof(buffer));
                        newZoneName = string(buffer);
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneAlias , buffer, sizeof(buffer));
                        newZoneAlias = string(buffer);
                        
                        if(focusedFXTrack && focusedFXName == newZoneName)
                            LoadRawFXFile(focusedFXTrack, focusedFXName);

                        for(auto name : TheManager->GetActionNames())
                            if(name != Shift && name != Option && name != Control && name != Alt)
                                SendDlgItemMessage(hwndLearn, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)name.c_str());

                        dlgResult = IDOK;
                        EndDialog(hwndDlg, 0);
                    }
                    break ;
                    
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EndDialog(hwndDlg, 0);
                    break ;
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

static WDL_DLGRET dlgProcLearn(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_USER+1024:
        {
            currentSurface = currentWidget->GetSurface();
            EnableButtons();

            ClearWidgets();
            
            SetDlgItemText(hwndDlg, IDC_EDIT_WidgetName, currentWidget->GetName().c_str());
            SetDlgItemText(hwndDlg, IDC_STATIC_SurfaceName, currentWidget->GetSurface()->GetName().c_str());
            
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, "");
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, "");
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, "");

            zoneComponentWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_SETCURSEL, -1, 0);
            actionNameWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, -1, 0);

            for(auto widget : currentSurface->GetWidgets())
                SendDlgItemMessage(hwndDlg, IDC_LIST_WidgetNames, LB_ADDSTRING, 0, (LPARAM)widget->GetName().c_str());
            
            for(int i = 0; i < (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETCOUNT, 0, 0); i++)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(buffer));
                if(string(buffer) == currentWidget->GetName())
                {
                    widgetNameWasSelectedBySurface = true;
                    SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_SETCURSEL, i, 0);
                    break;
                }
            }
            break;
        }

        case WM_USER+1025:
        {
            ClearZones();
            ClearActions();
            
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, currentAction->GetName().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, currentAction->GetParamAsString().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, currentAction->GetAlias().c_str());
            
            Zone* zone = currentWidgetActionManager->GetZone();
            
            int zoneIndex = FillZones(zone);
            
            FillSubZones(zone, zoneIndex);

            FillActionNames();
            
            break;
        }

        case WM_INITDIALOG:
        {
            hasEdits = false;
            
            AddComboBoxEntry(hwndDlg, 0, "No Navigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 1, "TrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 2, "SelectedTrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 3, "FocusedFXNavigator", IDC_COMBO_Navigator);
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 0, 0);

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
            
            break;
        }
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_BUTTON_NewFile:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        trackNumber = 0;
                        itemNumber = 0;
                        focusedFXIndex = 0;
                        focusedFXTrack = nullptr;
                        focusedFXName = "";
                        
                        dlgResult = false;
                        DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_NewZoneFile), g_hwnd, dlgProcNewZoneFile);
                        if(dlgResult == IDOK)
                        {
                            hasEdits = true;

                            ClearZones();
                            
                            newZoneFilename += ".zon";
                            
                            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_ZoneFilename), newZoneFilename.c_str());
                            SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_ADDSTRING, 0, (LPARAM)newZoneName.c_str());
                            zoneWasSelectedBySurface = true;
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_SETCURSEL, (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCOUNT, 0, 0) - 1, 0);
                            
                            // GAW TBD Make new zone and Add to Surface
                            
                            
                            
                            
                            //LM_Zone newZone;
                            
                            //newZone.name = newZoneName;
                            //newZone.alias = newZoneAlias;
                            
                            //zones.push_back(newZone);
                            
                            
                            //////////
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_AddZone:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        dlgResult = false;
                        DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_AddZone), g_hwnd, dlgProcAddZone);
                        if(dlgResult == IDOK)
                        {
                            hasEdits = true;
                            
                            SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_ADDSTRING, 0, (LPARAM)newZoneName.c_str());
                            zoneWasSelectedBySurface = true;
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_SETCURSEL, (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCOUNT, 0, 0) - 1, 0);
                            
                            // GAW TBD Make new zone and Add to Surface
                            
                            
                            
                            
                            //LM_Zone newZone;
                            
                            //newZone.name = newZoneName;
                            //newZone.alias = newZoneAlias;
                            
                            //zones.push_back(newZone);
                            
                            
                            //////////
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_AddIncludedZone:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        dlgResult = false;
                        DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_AddIncludedZone), g_hwnd, dlgProcAddIncludedZone);
                        if(dlgResult == IDOK)
                        {
                            // GAW TBD -- add included Zone to Zone
                            
                            hasEdits = true;
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_GenerateZoneEntry:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int zoneIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
                        
                        if(zoneIndex >= 0)
                        {
                            hasEdits = true;
                            /*
                            LM_ZoneEntry entry;
                            
                            entry.isShift = isShift;
                            entry.isOption = isOption;
                            entry.isControl = isControl;
                            entry.isAlt = isAlt;
                            entry.shouldToggle = shouldToggle;
                            entry.isInvert = isInvert;
                            entry.shouldIgnoreRelease = shouldIgnoreRelease;
                            entry.isTouch = isTouch;
                            entry.isHold = isHold;
                            
                            
                            GetDlgItemText(hwndDlg, IDC_EDIT_WidgetName , buffer, sizeof(buffer));
                            entry.widgetName = string(buffer);
                            
                            GetDlgItemText(hwndDlg, IDC_EDIT_ActionName , buffer, sizeof(buffer));
                            entry.actionName = string(buffer);
                            
                            GetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter , buffer, sizeof(buffer));
                            entry.param = string(buffer);
                            
                            GetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias , buffer, sizeof(buffer));
                            entry.alias = string(buffer);
                            
                            // GAW TBD -- Add Widget Action Manager To Zone

                            zones[zoneIndex].zoneEntries.push_back(entry);

                            string zoneEntryLine = entry.GetLineAsString();
    
                            Zone* entryToAddZone = currentWidget->GetSurface()->GetZones()[zones[zoneIndex].name];

                            TrackNavigator* entryTrackNavigator = new SelectedTrackNavigator(currentWidget->GetSurface()->GetPage()->GetTrackNavigationManager());
                            
                            WidgetActionManager* manager = new WidgetActionManager(currentWidget, entryToAddZone, entryTrackNavigator);
                            
                            vector<string> entryParams;
                            
                            entryParams.push_back(entry.actionName);
                            
                            if(entry.param != "")
                                entryParams.push_back(entry.param);
                            
                            if(entry.alias != "")
                                entryParams.push_back(entry.alias);
                            
                            Action* actionToAdd = TheManager->GetAction(manager, entryParams);
                            
                            manager->AddAction(NoModifiers, actionToAdd);
                            
                            manager->Activate();
                            
                            entryToAddZone->AddWidgetActionManager(manager);
                          
                            
                            SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)zoneEntryLine.c_str());
                            zoneComponentWasSelectedBySurface = true;
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_SETCURSEL, (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_GETCOUNT, 0, 0) - 1, 0);
                        }
                      */
                    }
                    break ;
                }
                    
                case IDC_BUTTON_DeleteZone:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_GETCURSEL, 0, 0);
                        if (index >= 0)
                        {
                            hasEdits = true;
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(buffer));
                            string deletedZone = string(buffer);
                            /*
                            for(int i = 0; i < zones.size(); i++)
                            {
                                for(int j = 0; j < zones[i].includedZones.size(); j++)
                                {
                                    if(zones[i].includedZones[j] == deletedZone)
                                    {
                                        zones[i].includedZones.erase(zones[i].includedZones.begin() + j);
                                        break;
                                    }
                                }
                            }
                            
                            // GAW TBD -- Zones, Parent, Included
                            zones.erase(zones.begin() + index);
                            */
                            SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_DELETESTRING, index, 0);
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_RESETCONTENT, 0, 0);
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_DeleteZoneEntry:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int index = SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_GETCURSEL, 0, 0);
                        if (index >= 0)
                        {
                            int zoneIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_GETCURSEL, 0, 0);
                            
                            if(zoneIndex >= 0)
                            {
                                //Zone* zoneEntryToDeleteZone = currentWidget->GetSurface()->GetZones()[zones[zoneIndex].name];
                                //zoneEntryToDeleteZone->RemoveWidgetActionManager(currentWidgetActionManager);
                                
                                hasEdits = true;
                                
                                // GAW TBD -- Delete Zone Widget Action Manager
                                //zones[zoneIndex].zoneEntries.erase(zones[zoneIndex].zoneEntries.begin() + index);
                                
                                SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_DELETESTRING, index, 0);
                            }
                        }
                    }
                    break ;
                }
                    
                case IDC_LIST_WidgetNames:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            if(widgetNameWasSelectedBySurface)
                            {
                                widgetNameWasSelectedBySurface = false;
                                break;
                            }
                            
                            // Get selected index.
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                currentWidget = currentSurface->GetWidgets()[index];
                                
                                SetDlgItemText(hwndDlg, IDC_EDIT_WidgetName, currentWidget->GetName().c_str());
                                SetDlgItemText(hwndDlg, IDC_STATIC_SurfaceName, currentWidget->GetSurface()->GetName().c_str());

                                zoneComponentWasSelectedBySurface = true;
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_SETCURSEL, -1, 0);
                            }

                            break;
                        }
                    }
                    break;
                }

                case IDC_LIST_ActionNames:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            if(actionNameWasSelectedBySurface)
                            {
                                actionNameWasSelectedBySurface = false;
                                break;
                            }
                            
                            // Get selected index.
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETTEXT, index, (LPARAM)(LPCTSTR)(buffer));

                                if( ! isdigit(buffer[0]))
                                {
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, buffer);
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, "");
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, "");
                                }
                                else
                                {
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, "FXParam");
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, to_string(index).c_str());
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, "");
                                }
                                
                                zoneComponentWasSelectedBySurface = true;
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_SETCURSEL, -1, 0);
                            }
                            
                            break;
                        }
                    }
                    
                    break;
                }

                case IDC_LIST_Zones:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            if(zoneWasSelectedBySurface)
                            {
                                zoneWasSelectedBySurface = false;
                                break;
                            }
                            
                            // Get selected index.
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_RESETCONTENT, 0, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ParentZone), CB_RESETCONTENT, 0, 0);
                                
                                AddComboBoxEntry(hwndDlg, 0, "No Parent Zone", IDC_COMBO_ParentZone);
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ParentZone), CB_SETCURSEL, 0, 0);

                                /*
                                for(auto zone : GetAvailableZones(index))
                                    AddComboBoxEntry(hwndDlg, 0, zone.c_str(), IDC_COMBO_ParentZone);
                                
                                if(zones[index].parentZone != "")
                                {
                                    for(int i = 0; i < (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ParentZone), CB_GETCOUNT, 0, 0); i++)
                                    {
                                        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ParentZone), CB_GETLBTEXT, i, (LPARAM)(LPCTSTR)(buffer));

                                        if(string(buffer) == zones[index].parentZone)
                                        {
                                            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_ParentZone), CB_SETCURSEL, i, 0);
                                            break;
                                        }
                                    }
                                }
                               
                                
                                if(zones.size() > index)
                                {
                                    LM_Zone zone = zones[index];

                                    for(auto includedZone : zone.includedZones)
                                        SendDlgItemMessage(hwndDlg, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)includedZone.c_str());

                                    for(auto entry : zone.zoneEntries)
                                    {
                                        string entryLine = entry.GetLineAsString();
                                        SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)entryLine.c_str());
                                    }
                                }
                                  */
                            }
                            
                            break;
                        }
                    }
                    
                    break;
                }

                case IDC_LIST_ZoneComponents:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            if(zoneComponentWasSelectedBySurface)
                            {
                                zoneComponentWasSelectedBySurface = false;
                                break;
                            }
                            
                            // Get selected index.
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                int zoneIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
                                
                                if(zoneIndex >= 0)
                                {
                                    //LM_ZoneEntry entry = zones[zoneIndex].zoneEntries[index];
                                    //entry.SetGlobalModifers();
                                    //SetCheckBoxes();

                                    //SetDlgItemText(hwndDlg, IDC_EDIT_WidgetName, entry.widgetName.c_str());
                                    //SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, entry.actionName.c_str());
                                    //SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, entry.param.c_str());
                                    //SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, entry.alias.c_str());

                                    for(int i = 0; i < (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETCOUNT, 0, 0); i++)
                                    {
                                        SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(buffer));
                                        //if(string(buffer) == entry.widgetName)
                                        {
                                            widgetNameWasSelectedBySurface = true;
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_SETCURSEL, i, 0);
                                            break;
                                        }
                                    }
                                    /*
                                    if(entry.actionName == "FXParam" || entry.actionName == "FXParamNameDisplay" || entry.actionName == "FXParamValueDisplay" || entry.actionName == "FXParamRelative")
                                    {
                                        actionNameWasSelectedBySurface = true;
                                        SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, atoi(entry.param.c_str()), 0);
                                    }
                                    else
                                    {
                                        for(int i = 0; i < (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETCOUNT, 0, 0); i++)
                                        {
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(buffer));
                                            if(string(buffer) == entry.actionName)
                                            {
                                                actionNameWasSelectedBySurface = true;
                                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, i, 0);
                                                break;
                                            }
                                        }
                                    }
                                     */
                                }
                            }
                            
                            break;
                        }
                    }
                    
                    break;
                }

                case IDC_CHECK_Shift:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Shift))
                        isShift = true;
                    else
                        isShift = false;

                    break;
                }
                    
                case IDC_CHECK_Option:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Option))
                        isOption = true;
                    else
                        isOption = false;

                    break;
                }
                    
                case IDC_CHECK_Control:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Control))
                        isControl = true;
                    else
                        isControl = false;

                    break;
                }
                    
                case IDC_CHECK_Alt:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Alt))
                        isAlt = true;
                    else
                        isAlt = false;

                    break;
                }
                    
                case IDC_CHECK_Toggle:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Toggle))
                        shouldToggle = true;
                    else
                        shouldToggle = false;

                    break;
                }
                    
                case IDC_CHECK_Invert:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Invert))
                        isInvert = true;
                    else
                        isInvert = false;
                    
                    break;
                }
                    
                case IDC_CHECK_Touch:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Touch))
                        isTouch = true;
                    else
                        isTouch = false;
                    
                    break;
                }
                    
                case IDC_CHECK_IgnoreRelease:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_IgnoreRelease))
                        shouldIgnoreRelease = true;
                    else
                        shouldIgnoreRelease = false;
                    
                    break;
                }
                    
                case IDC_CHECK_Hold:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Hold))
                        isHold = true;
                    else
                        isHold = false;
                    
                    break;
                }
                    
                case IDC_CHECK_SurfaceInMon:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SurfaceInMon))
                        TheManager->SetSurfaceInMonitor(true);
                    else
                        TheManager->SetSurfaceInMonitor(false);
                    
                    break;
                }

                case IDC_CHECK_SurfaceOutMon:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_SurfaceOutMon))
                        TheManager->SetSurfaceOutMonitor(true);
                    else
                        TheManager->SetSurfaceOutMonitor(false);
                    
                    break;
                }
                    
                case IDC_CHECK_FXParamMon:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_FXParamMon))
                        TheManager->SetFXMonitor(true);
                    else
                        TheManager->SetFXMonitor(false);
                    
                    break;
                }
                    
                case IDC_BUTTON_Close:
                {
                    SendMessage(hwndLearn, WM_CLOSE, 0, 0);
                    break;
                }
            }
        }
            break ;
            
        case WM_CLOSE:
            if(hasEdits)
            {
                if(MessageBox(hwndDlg, "You have unsaved changes, are you sure you want to close the Learn Mode Window ?", "Unsaved Changes", MB_YESNO) == IDYES)
                    DestroyWindow(hwndDlg);
            }
            else
                DestroyWindow(hwndDlg) ;
            break ;
            
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            hwndLearn = nullptr;
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
        DisableButtons();
        ShowWindow(hwndLearn, true);
    }
}

void Page::ToggleLearnMode()
{
    if(hwndLearn == nullptr)
    {
        hwndLearn = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Learn), g_hwnd, dlgProcLearn);
        DisableButtons();
        ShowWindow(hwndLearn, true);
    }
    else
    {
        SendMessage(hwndLearn, WM_CLOSE, 0, 0);
    }
}

void Page::InputReceived(Widget* widget, double value)
{
    if(hwndLearn == nullptr || value == 0.0)
        return;
    
    currentWidget = widget;
    currentWidgetActionManager = nullptr;
    currentAction = nullptr;

    if(currentWidget != nullptr)
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
    
    isShift = isShift_ || GetAsyncKeyState(VK_SHIFT);
    isOption = isOption_;
    isControl = isControl_ || GetAsyncKeyState(VK_CONTROL);
    isAlt = isAlt_;
   
    if(currentWidget != nullptr && currentWidgetActionManager != nullptr && currentAction != nullptr)
        SendMessage(hwndLearn, WM_USER+1025, 0, 0);
}
