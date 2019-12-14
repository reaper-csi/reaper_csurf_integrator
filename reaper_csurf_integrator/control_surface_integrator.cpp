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

extern reaper_plugin_info_t *g_reaper_plugin_info;

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
        line = regex_replace(line, regex(TabChars), " ");
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

static void GetWidgetNameAndModifiers(string line, string &widgetName, string &modifiers, bool &isTrackTouch, bool &isInverted, bool &shouldToggle, double &delayAmount)
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
            else if(modifier_tokens[i] == "Hold")
                delayAmount = 1.0;
        }
    }
    
    widgetName = modifier_tokens[modifier_tokens.size() - 1];
    
    modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
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
    
    for (string line; getline(zoneFile, line) ; )
    {
        line = regex_replace(line, regex(TabChars), " ");
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
            for(int i = 0; i < expandedZones.size(); i++)
                expandedZones[i]->SetTrackNavigator(TrackNavigatorForChannel(i, surface->GetName() + to_string(i), surface));
            
            continue;
        }
        
        if(tokens.size() == 1 && tokens[0] == "MasterTrackNavigator")
        {
            for(int i = 0; i < expandedZones.size(); i++)
                expandedZones[i]->SetTrackNavigator(new MasterTrackNavigator(surface->GetPage()->GetTrackNavigationManager()));
            
            continue;
        }
        
        if(tokens.size() == 1 && tokens[0] == "SelectedTrackNavigator")
        {
            for(int i = 0; i < expandedZones.size(); i++)
                expandedZones[i]->SetTrackNavigator(new SelectedTrackNavigator(surface->GetPage()->GetTrackNavigationManager()));
            
            continue;
        }
        
        if(tokens.size() == 1 && tokens[0] == "FocusedFXNavigator")
        {
            for(int i = 0; i < expandedZones.size(); i++)
                expandedZones[i]->SetTrackNavigator(new FocusedFXNavigator(surface->GetPage()->GetTrackNavigationManager()));

            continue;
        }
        
        for(int i = 0; i < expandedZones.size(); i++)
        {            
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
                bool isDelayed = false;
                double delayAmount = 0.0;
                
                GetWidgetNameAndModifiers(tokens[0], widgetName, modifiers, isTrackTouch, isInverted, shouldToggle, delayAmount);
                
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
                            widgetActionManagerForWidget[widget] = new WidgetActionManager(widget, expandedZones[i]);

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
                            action->SetShouldToggle();
                        
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
        line = regex_replace(line, regex(TabChars), " ");
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
            else if(widgetClass == "FB_NovationLaunchpadMiniRGB7Bit" && tokens.size() == 4)
            {
                feedbackProcessor = new FB_NovationLaunchpadMiniRGB7Bit(surface, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
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
            else if((widgetClass == "FB_MCUVUMeter" || widgetClass == "FB_MCUXTVUMeter") && (tokens.size() == 2 || tokens.size() == 3))
            {
                int displayType = widgetClass == "FB_MCUVUMeter" ? 0x14 : 0x15;
                
                feedbackProcessor = new MCUVUMeter_Midi_FeedbackProcessor(surface, displayType, stoi(tokens[1]));
                
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
        line = regex_replace(line, regex(TabChars), " ");
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
            line = regex_replace(line, regex(TabChars), " ");
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
    actions_["NoAction"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new NoAction(name, manager, params); };
    actions_["Reaper"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new ReaperAction(name, manager, params); };
    actions_["FXNameDisplay"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXNameDisplay(name, manager, params); };
    actions_[FXParam] =                             [this](string name, WidgetActionManager* manager, vector<string> params) { return new class FXParam(name, manager, params); };
    actions_["FXParamRelative"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParamRelative(name, manager, params); };
    actions_["FXParamNameDisplay"] =                [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParamNameDisplay(name, manager, params); };
    actions_["FXParamValueDisplay"] =               [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXParamValueDisplay(name, manager, params); };
    actions_["FXGainReductionMeter"] =              [this](string name, WidgetActionManager* manager, vector<string> params) { return new FXGainReductionMeter(name, manager, params); };
    actions_["TrackVolume"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackVolume(name, manager, params); };
    actions_["TrackVolumeDB"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackVolumeDB(name, manager, params); };
    actions_["TrackSendVolume"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendVolume(name, manager, params); };
    actions_["TrackSendVolumeDB"] =                 [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendVolumeDB(name, manager, params); };
    actions_["TrackSendPan"] =                      [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendPan(name, manager, params); };
    actions_["TrackSendMute"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendMute(name, manager, params); };
    actions_["TrackSendInvertPolarity"] =           [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendInvertPolarity(name, manager, params); };
    actions_["TrackSendPrePost"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendPrePost(name, manager, params); };
    actions_["TrackPan"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPan(name, manager, params); };
    actions_["TrackPanWidth"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPanWidth(name, manager, params); };
    actions_["TrackNameDisplay"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackNameDisplay(name, manager, params); };
    actions_["TrackVolumeDisplay"] =                [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackVolumeDisplay(name, manager, params); };
    actions_["TrackSendNameDisplay"] =              [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendNameDisplay(name, manager, params); };
    actions_["TrackSendVolumeDisplay"] =            [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSendVolumeDisplay(name, manager, params); };
    actions_["TrackPanDisplay"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPanDisplay(name, manager, params); };
    actions_["TrackPanWidthDisplay"] =              [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackPanWidthDisplay(name, manager, params); };
    actions_["TimeDisplay"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new TimeDisplay(name, manager, params); };
    actions_["Rewind"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new Rewind(name, manager, params); };
    actions_["FastForward"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new FastForward(name, manager, params); };
    actions_["Play"] =                              [this](string name, WidgetActionManager* manager, vector<string> params) { return new Play(name, manager, params); };
    actions_["Stop"] =                              [this](string name, WidgetActionManager* manager, vector<string> params) { return new Stop(name, manager, params); };
    actions_["Record"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new Record(name, manager, params); };
    actions_["TrackFolderDive"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackFolderDive(name, manager, params); };
    actions_["TrackSelect"] =                       [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSelect(name, manager, params); };
    actions_["TrackUniqueSelect"] =                 [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackUniqueSelect(name, manager, params); };
    actions_["TrackRangeSelect"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackRangeSelect(name, manager, params); };
    actions_["TrackRecordArm"] =                    [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackRecordArm(name, manager, params); };
    actions_["TrackMute"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackMute(name, manager, params); };
    actions_["TrackSolo"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackSolo(name, manager, params); };
    actions_["TrackTouch"] =                        [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetTrackTouch(name, manager, params); };
    actions_["CycleTimeline"] =                     [this](string name, WidgetActionManager* manager, vector<string> params) { return new CycleTimeline(name, manager, params); };
    actions_["TrackOutputMeter"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeter(name, manager, params); };
    actions_["TrackOutputMeterAverageLR"] =         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterAverageLR(name, manager, params); };
    actions_["TrackOutputMeterMaxPeakLR"] =         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterMaxPeakLR(name, manager, params); };
    actions_["SetShowFXWindows"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetShowFXWindows(name, manager, params); };
    actions_["ToggleScrollLink"] =                  [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleScrollLink(name, manager, params); };
    actions_["CycleTimeDisplayModes"] =             [this](string name, WidgetActionManager* manager, vector<string> params) { return new CycleTimeDisplayModes(name, manager, params); };
    actions_["NextPage"] =                          [this](string name, WidgetActionManager* manager, vector<string> params) { return new GoNextPage(name, manager, params); };
    actions_["GoPage"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new class GoPage(name, manager, params); };
    actions_["GoZone"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new GoZone(name, manager, params); };
    actions_["SelectTrackRelative"] =               [this](string name, WidgetActionManager* manager, vector<string> params) { return new SelectTrackRelative(name, manager, params); };
    actions_["TrackBank"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TrackBank(name, manager, params); };
    actions_["Shift"] =                             [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetShift(name, manager, params); };
    actions_["Option"] =                            [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetOption(name, manager, params); };
    actions_["Control"] =                           [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetControl(name, manager, params); };
    actions_["Alt"] =                               [this](string name, WidgetActionManager* manager, vector<string> params) { return new SetAlt(name, manager, params); };
    actions_["TogglePin"] =                         [this](string name, WidgetActionManager* manager, vector<string> params) { return new TogglePin(name, manager, params); };
    actions_["ToggleLearnMode"] =                   [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleLearnMode(name, manager, params); };
    actions_["ToggleMapSelectedTrackSends"] =       [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapSelectedTrackSends(name, manager, params); };
    actions_["MapSelectedTrackSendsToWidgets"] =    [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackSendsToWidgets(name, manager, params); };
    actions_["ToggleMapSelectedTrackFX"] =          [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapSelectedTrackFX(name, manager, params); };
    actions_["MapSelectedTrackFXToWidgets"] =       [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackFXToWidgets(name, manager, params); };
    actions_["ToggleMapSelectedTrackFXMenu"] =      [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapSelectedTrackFXMenu(name, manager, params); };
    actions_["MapSelectedTrackFXToMenu"] =          [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackFXToMenu(name, manager, params); };
    actions_["ToggleMapFocusedFX"] =                [this](string name, WidgetActionManager* manager, vector<string> params) { return new ToggleMapFocusedFX(name, manager, params); };
    actions_["MapFocusedFXToWidgets"] =             [this](string name, WidgetActionManager* manager, vector<string> params) { return new MapFocusedFXToWidgets(name, manager, params); };
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
            line = regex_replace(line, regex(TabChars), " ");
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
                else if(tokens[0] == MidiSurfaceToken || tokens[0] == OSCSurfaceToken || tokens[0] == EuConSurfaceToken)
                {
                    if(tokens.size() != 11 && tokens.size() != 12)
                        continue;
                    
                    int inPort = atoi(tokens[2].c_str());
                    int outPort = atoi(tokens[3].c_str());
                    
                    if(currentPage)
                    {
                        ControlSurface* surface = nullptr;
                        
                        if(tokens[0] == MidiSurfaceToken)
                            surface = new Midi_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForPort(inPort), GetMidiOutputForPort(outPort));
                        else if(tokens[0] == OSCSurfaceToken && tokens.size() > 11)
                            surface = new OSC_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], inPort, outPort, tokens[11]);
                        else if(tokens[0] == EuConSurfaceToken && tokens.size() == 11)
                            surface = new EuCon_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4],
                                                               atoi(tokens[2].c_str()), // firstChannel
                                                               atoi(tokens[3].c_str()), // last Channel
                                                               atoi(tokens[6].c_str()), // numSends
                                                               atoi(tokens[7].c_str()), // numFX
                                                               atoi(tokens[8].c_str()), // NumInputs
                                                               atoi(tokens[9].c_str()), // numOutputs
                                                               atoi(tokens[10].c_str()) // options
                                                               );

                        currentPage->AddSurface(surface);
                        
                        if(tokens[0] == EuConSurfaceToken)
                        {
                            if(tokens[5] == "UseZoneLink")
                                surface->SetUseZoneLink(true);
                        }
                        else
                        {
                            if(tokens[6] == "UseZoneLink")
                                surface->SetUseZoneLink(true);
                            
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

void  Widget::SetRGBValue(int r, int g, int b)
{
    for(auto feebackProcessor : feedbackProcessors_)
        feebackProcessor->SetRGBValue(r, g, b);
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_CSIMessageGenerator : public CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EuCon_CSIMessageGenerator::EuCon_CSIMessageGenerator(EuCon_ControlSurface* surface, Widget* widget, string message) : CSIMessageGenerator(widget)
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
    if(lastDoubleValue_ != value)
    {
        lastDoubleValue_ = value;
        surface_->SendOSCMessage(oscAddress_, value);
    }
}

void OSC_FeedbackProcessor::SetValue(int param, double value)
{
    if(lastDoubleValue_ != value)
    {
        lastDoubleValue_ = value;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_FeedbackProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////
void EuCon_FeedbackProcessor::SetValue(double value)
{
    if(lastDoubleValue_ != value)
    {
        lastDoubleValue_ = value;
        surface_->SendEuConMessage(oscAddress_, value);
    }
}

void EuCon_FeedbackProcessor::SetValue(int param, double value)
{
    if(lastDoubleValue_ != value)
    {
        lastDoubleValue_ = value;
        surface_->SendEuConMessage(oscAddress_, value);
    }
}

void EuCon_FeedbackProcessor::SetValue(string value)
{
    if(lastStringValue_ != value)
    {
        lastStringValue_ = value;
        surface_->SendEuConMessage(oscAddress_, value);
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

Action::Action(string name, WidgetActionManager* widgetActionManager, vector<string> params) : Action(name, widgetActionManager)
{
    SetRGB( { params.begin() + 1, params.end() } );
}

void Action::SetRGB(vector<string> params)
{
    // GAW -- translate the bytes to RGB on and off values;
    if(params.size() == 6 )
    {
        supportsRGB_ = true;
        
        for(int i = 0; i < params.size(); i++)
        {
            int value = stoi(params[i]);
            value = value < 0 ? 0 : value;
            value = value > 255 ? 255 : value;
            
            if(i == 0)
                RGBValues_[1].r = value;
            
            else if(i == 1)
                RGBValues_[1].g = value;
            
            else if(i == 2)
                RGBValues_[1].b = value;
            
            else if(i == 3)
                RGBValues_[0].r = value;
            
            else if(i == 4)
                RGBValues_[0].g = value;
            
            else if(i == 5)
                RGBValues_[0].b = value;
        }
    }
}

void Action::DoAction(double value, WidgetActionManager* sender)
{
    value = isInverted_ == false ? value : 1.0 - value;
    
    if(shouldToggle_ && value != 0.0)
        Do( ! GetCurrentValue(), sender);
    else
        Do(value, sender);
    
    if( ! widget_->GetIsModifier())
        page_->ActionPerformed(GetWidgetActionManager(), this);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WidgetActionManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackNavigator* WidgetActionManager::GetTrackNavigator()
{
    return zone_->GetTrackNavigator();
}

string WidgetActionManager::GetModifiers()
{
    if(widget_->GetIsModifier())
        return ""; // Modifier Widgets cannot have Modifiers
    else
        return widget_->GetSurface()->GetPage()->GetModifiers();
}

string WidgetActionManager::GetNavigatorName()
{
    if(GetTrackNavigator() == nullptr)
        return "";
    else
        return GetTrackNavigator()->GetName();
}

MediaTrack* WidgetActionManager::GetTrack()
{
    if(GetTrackNavigator() == nullptr)
        return nullptr;
    else
        return GetTrackNavigator()->GetTrack();
}

void WidgetActionManager::RequestUpdate()
{
    if(trackTouchedActions_.size() > 0 && GetTrackNavigator() != nullptr && GetTrackNavigator()->GetIsChannelTouched())
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
    if(GetTrackNavigator() != nullptr)
        GetTrackNavigator()->SetTouchState((isTouched));
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
void Zone::AddAction(ActionLineItem actionLineItem, int actionIndex)
{
    WidgetActionManager* widgetActionManager = nullptr;
    
    for(auto manager : widgetActionManagers_)
        if(manager->GetWidget() == actionLineItem.widget)
            widgetActionManager = manager;
    
    if(widgetActionManager == nullptr)
        widgetActionManager = new WidgetActionManager(actionLineItem.widget, this);
    
    AddWidgetActionManager(widgetActionManager);
    
    vector<string> params;

    params.push_back(actionLineItem.actionName);
    
    params.push_back(actionLineItem.param);

    if(actionLineItem.alias != "")
        params.push_back(actionLineItem.alias);

    Action* action = TheManager->GetAction(widgetActionManager, params);

    action->SetIndex(actionIndex);
    
    widgetActionManager->AddAction(actionLineItem.modifiers, action);
}

void Zone::Activate(WidgetActionManager* sender)
{
    int index = 0;
    
    if(sender != nullptr)
        index = sender->GetZone()->GetIndex();
    
    Activate(index);
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

void ControlSurface::RemoveZone(Zone* zoneToDelete, int zoneIndexInZoneFile)
{
    zoneToDelete->Deactivate();
    zones_.erase(zoneToDelete->GetName());
    zonesInZoneFile_[zoneToDelete->GetSourceFilePath()].erase(zonesInZoneFile_[zoneToDelete->GetSourceFilePath()].begin() + zoneIndexInZoneFile);
    
    if(zonesInZoneFile_[zoneToDelete->GetSourceFilePath()].size() > 0)
    {
        for(auto zone : zonesInZoneFile_[zoneToDelete->GetSourceFilePath()])
        {
            for(int i = 0; i < zone->GetIncludedZones().size(); i++)
                if(zone->GetIncludedZones()[i]->GetName() == zoneToDelete->GetName())
                    zone->RemoveZone(i);
        }
    }
}

void ControlSurface::GoZone(string zoneName, WidgetActionManager* sender)
{
    if(zones_.count(zoneName) > 0)
    {
        zones_[zoneName]->Activate(sender);
        activeZone_ = zones_[zoneName];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename, this, widgets_);
    ControlSurface::InitHardwiredWidgets();
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
        for( auto generator : CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]])
            generator->ProcessMidiMessage(evt);
    else if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100) > 0)
        for( auto generator : CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100])
            generator->ProcessMidiMessage(evt);
    else if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000) > 0)
        for( auto generator : CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000])
            generator->ProcessMidiMessage(evt);
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
void OSC_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/OSC/" + templateFilename, this, widgets_);
    ControlSurface::InitHardwiredWidgets();
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

void EuConRequestsInitialization()
{
    if(TheManager)
        TheManager->InitializeEuCon();
}

void InitializeEuConWidget(char *name, char *control, char *FB_Processor)
{
    if(TheManager)
        TheManager->InitializeEuConWidget(string(name), string(control), string(FB_Processor));
}

void EuConInitializationComplete()
{
    if(TheManager)
        TheManager->EuConInitializationComplete();
}

void HandleEuConMessageWithDouble(const char *oscAddress, double value)
{
    if(TheManager)
        TheManager->HandleEuConMessage(string(oscAddress), value);
}

void HandleEuConMessageWithString(const char *oscAddress, const char *value)
{
    if(TheManager)
        TheManager->HandleEuConMessage(string(oscAddress), string(value));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
EuCon_ControlSurface::EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int lowChannel, int highChannel, int numSends, int numFX, int numInputs, int numOutputs, int options)
: ControlSurface(CSurfIntegrator, page, name), lowChannel_(lowChannel), highChannel_(highChannel), numSends_(numSends), numFX_(numFX), numInputs_(numInputs), numOutputs_(numOutputs), options_(options)
{
    // EuCon takes care of managing navigation, so we just blast everything always
    sendsActivationManager_->SetShouldMapSends(true);
    fxActivationManager_->SetShouldMapSelectedTrackFX(true);
    fxActivationManager_->SetShouldMapSelectedTrackFXMenus(true);
    fxActivationManager_->SetShouldMapFocusedFX(true);
    
    zoneFolder_ = zoneFolder;
    
    if( ! plugin_register("API_EuConRequestsInitialization", (void *)::EuConRequestsInitialization))
        LOG::InitializationFailure("EuConRequestsInitialization failed to register");
    
    if( ! plugin_register("API_InitializeEuConWidget", (void *)::InitializeEuConWidget))
        LOG::InitializationFailure("InitializeEuConWidget failed to register");
    
    if( ! plugin_register("API_EuConInitializationComplete", (void *)::EuConInitializationComplete))
        LOG::InitializationFailure("EuConInitializationComplete failed to register");
    
    if( ! plugin_register("API_HandleEuConMessageWithDouble", (void *)::HandleEuConMessageWithDouble))
        LOG::InitializationFailure("HandleEuConMessageWithDouble failed to register");
    
    if( ! plugin_register("API_HandleEuConMessageWithString", (void *)::HandleEuConMessageWithString))
        LOG::InitializationFailure("HandleEuConMessageWithString failed to register");
    
    InitializeEuCon();
}

void EuCon_ControlSurface::InitializeEuCon()
{
    if(g_reaper_plugin_info)
    {
        void (*InitializeEuConWithParameters)(int firstChannel, int lastChannel, int numSends, int numFX, int numInputs, int numOutputs, int options);
        
        InitializeEuConWithParameters = (void (*)(int, int, int, int, int, int, int))g_reaper_plugin_info->GetFunc("InitializeEuConWithParameters");

        if(InitializeEuConWithParameters)
            InitializeEuConWithParameters(lowChannel_, highChannel_, numSends_, numFX_, numInputs_, numOutputs_, options_);
    }
}

void EuCon_ControlSurface::InitializeEuConWidget(string name, string control, string FB_Processor)
{
    if(name != "")
    {
        Widget* widget = new Widget(this, string(name));
        if(widget)
        {
            widgets_.push_back(widget);
            if(control != "")
                new EuCon_CSIMessageGenerator(this, widget, string(control));
            if(FB_Processor != "")
                widget->AddFeedbackProcessor(new EuCon_FeedbackProcessor(this, string(FB_Processor)));
        }
    }
}

void EuCon_ControlSurface::EuConInitializationComplete()
{
    ControlSurface::InitHardwiredWidgets();
    InitZones(zoneFolder_);
    GoHome();
}

void EuCon_ControlSurface::SendEuConMessage(string oscAddress, double value)
{
    if(g_reaper_plugin_info)
    {
        void (*HandleReaperMessageWthDouble)(const char *, double);
        
        HandleReaperMessageWthDouble = (void (*)(const char *, double))g_reaper_plugin_info->GetFunc("HandleReaperMessageWthDouble");
        
        if(HandleReaperMessageWthDouble)
            HandleReaperMessageWthDouble(oscAddress.c_str(), value);
    }
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s %s  %f  \n", name_.c_str(), oscAddress.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::SendEuConMessage(string oscAddress, string value)
{
    if(g_reaper_plugin_info)
    {
        void (*HandleReaperMessageWthString)(const char *, const char *);
        
        HandleReaperMessageWthString = (void (*)(const char *, const char *))g_reaper_plugin_info->GetFunc("HandleReaperMessageWithString");
        
        if(HandleReaperMessageWthString)
            HandleReaperMessageWthString(oscAddress.c_str(), value.c_str());
    }

    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        sprintf(buffer, "OUT -> %s %s  %s  \n", name_.c_str(), oscAddress.c_str(), value.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::HandleEuConMessage(string oscAddress, double value)
{
    if(CSIMessageGeneratorsByOSCMessage_.count(oscAddress) > 0)
        CSIMessageGeneratorsByOSCMessage_[oscAddress]->ProcessOSCMessage(oscAddress, value);
    
    if(TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        sprintf(buffer, "IN -> %s %s  %f  \n", name_.c_str(), oscAddress.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::HandleEuConMessage(string oscAddress, string value)
{
    // GAW TBD
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
// MasterTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* MasterTrackNavigator::GetTrack()
{
    return DAW::GetMasterTrack(0);
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



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Learn Mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddComboBoxEntry(HWND hwndDlg, int x, string entryName, int comboId)
{
    int a=SendDlgItemMessage(hwndDlg,comboId,CB_ADDSTRING,0,(LPARAM)entryName.c_str());
    SendDlgItemMessage(hwndDlg,comboId,CB_SETITEMDATA,a,x);
}

static char buffer[BUFSZ * 4];
static HWND hwndLearn = nullptr;
static bool hasEdits = false;
static int dlgResult = 0;
static Page* currentPage = nullptr;
static ControlSurface* currentSurface = nullptr;
static Widget* currentWidget = nullptr;
static WidgetActionManager* currentWidgetActionManager = nullptr;
static Action* currentAction = nullptr;
static string trackNavigatorName = "";

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
static bool isHold = false;
static bool isInvert = false;

// Focused FX
static int trackNumber = 0;
static int itemNumber = 0;
static int focusedFXIndex = 0;
static MediaTrack* focusedFXTrack = nullptr;
static string focusedFXName = "";

// For adding new Actions to FX Zones in realtime
static int currentFXIndex = 0;

// Guards for Set Current Selection messages
static bool widgetNameWasSelectedBySurface = false;
static bool actionNameWasSelectedBySurface = false;
static bool zoneComponentWasSelectedBySurface = false;
static bool zoneWasSelectedBySurface = false;

static void EnableButtons()
{
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddLineItem), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteLineItem), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddIncludedZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteIncludedZone), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_NewFile), true);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_SaveFile), true);
}

static void DisableButtons()
{
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_AddLineItem), false);
    EnableWindow(GetDlgItem(hwndLearn, IDC_BUTTON_DeleteLineItem), false);
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

static void SetCheckBoxes(ActionLineItem actionLineItem)
{
    if(actionLineItem.isShift)
        CheckDlgButton(hwndLearn, IDC_CHECK_Shift, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Shift, BST_UNCHECKED);
    
    if(actionLineItem.isOption)
        CheckDlgButton(hwndLearn, IDC_CHECK_Option, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Option, BST_UNCHECKED);
    
    if(actionLineItem.isControl)
        CheckDlgButton(hwndLearn, IDC_CHECK_Control, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Control, BST_UNCHECKED);
    
    if(actionLineItem.isAlt)
        CheckDlgButton(hwndLearn, IDC_CHECK_Alt, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Alt, BST_UNCHECKED);
    
    if(actionLineItem.isToggle)
        CheckDlgButton(hwndLearn, IDC_CHECK_Toggle, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Toggle, BST_UNCHECKED);
    
    if(actionLineItem.isInvert)
        CheckDlgButton(hwndLearn, IDC_CHECK_Invert, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Invert, BST_UNCHECKED);
    
    if(actionLineItem.isTouch)
        CheckDlgButton(hwndLearn, IDC_CHECK_Touch, BST_CHECKED);
    else
        CheckDlgButton(hwndLearn, IDC_CHECK_Touch, BST_UNCHECKED);
    
    if(actionLineItem.isHold)
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
                        rawFXFile << "\"" + string(zoneName) + "\"" + GetLineEnding();
                        
                        for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                        {
                            DAW::TrackFX_GetParamName(track, i, j, buffer, sizeof(buffer));
                            
                            rawFXFile << "\"" + string(buffer) + "\"" + GetLineEnding();
                        }
                    }
                    
                    rawFXFile.close();
                }
            }
        }
        
        for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
        {
            DAW::TrackFX_GetFXName(track, i, buffer, sizeof(buffer));
            
            if(string(buffer) == zoneName)
            {
                currentFXIndex = i;
                break;
            }
        }

        ifstream rawFXFile(filePath);
        
        if(!rawFXFile)
            return false;
        
        int rawFileLineIndex = 0;
        
        for (string line; getline(rawFXFile, line) ; )
        {
            line = regex_replace(line, regex(TabChars), " ");
            line = regex_replace(line, regex(CRLFChars), "");
            line = regex_replace(line, regex("[\"]"), "");

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
}

static void ClearZones()
{
    SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_ZoneFilename), "");
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_Zones), LB_RESETCONTENT, 0, 0);
    SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), "");
}

static void ClearSubZones()
{
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ZoneComponents), LB_RESETCONTENT, 0, 0);
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
}

static void ClearActions()
{
    SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_RESETCONTENT, 0, 0);
    SetDlgItemText(hwndLearn, IDC_EDIT_ActionName, "");
    SetDlgItemText(hwndLearn, IDC_EDIT_ActionParameter, "");
    SetDlgItemText(hwndLearn, IDC_EDIT_ActionAlias, "");
}

static int FillZones(Zone* zone)
{
    ClearZones();

    // Zone FileName
    istringstream path(zone->GetSourceFilePath());
    vector<string> pathFolders;
    string pathfolder;
    
    while (getline(path, pathfolder, '/'))
        pathFolders.push_back(pathfolder);

    int CSIFolderIndex = -1;
    
    for(int i = 0; i < pathFolders.size(); i++)
    {
        if(pathFolders[i] == "CSI")
        {
            CSIFolderIndex = i;
            break;
        }
    }
    
    if(CSIFolderIndex >= 0)
    {
        string pathString = "CSI";
        
        for(int i = CSIFolderIndex + 1; i < pathFolders.size(); i++)
            pathString += "/" + pathFolders[i];
        
        SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_ZoneFilename), pathString.c_str());
    }
    else
        SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_ZoneFilename), zone->GetSourceFilePath().c_str());
    
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
            if(zonesInThisFile[i]->GetName() != zonesInThisFile[i]->GetAlias())
                SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), zonesInThisFile[i]->GetAlias().c_str());
            zoneIndex = i;
        }
    }

    return zoneIndex;
}

static void FillSubZones(Zone* zone, int zoneIndex)
{
    ClearSubZones();
    ClearActions();
    
    // Navigator
    string navigatorName = currentWidgetActionManager->GetNavigatorName();
    
    if(navigatorName == "")
        navigatorName = "NoNavigator";
    
    SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_Navigator), navigatorName.c_str());
    
    // Included Zones
    for(auto includedZone : zone->GetIncludedZones())
        SendDlgItemMessage(hwndLearn, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)includedZone->GetName().c_str());
    
    // Zone line Items
    bool hasLoadedRawFXFile = false;
    
    vector<ActionLineItem> actionLineItems = zone->GetActionLineItems();
    
    for(int i = 0; i < actionLineItems.size(); i++)
    {
        ActionLineItem actionLineItem = actionLineItems[i];
        
        if (actionLineItem.actionName.find(FXParam) != string::npos && hasLoadedRawFXFile == false)
            hasLoadedRawFXFile = LoadRawFXFile(currentWidget->GetTrack(), zone->GetName());
        
        if(hasLoadedRawFXFile && actionLineItem.actionName.find(FXParam) != string::npos && actionLineItem.param == currentAction->GetParamAsString())
        {
            actionNameWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_SETCURSEL, currentAction->GetParam(), 0);
        }
        
        string lineItemAsString = actionLineItem.allModifiers + actionLineItem.widgetName + " " + actionLineItem.actionName;
        
        if(actionLineItem.param != "")
            lineItemAsString += " " + actionLineItem.param;
        
        if(actionLineItem.alias != "")
            lineItemAsString += " " + actionLineItem.alias;
        
        SendDlgItemMessage(hwndLearn, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)lineItemAsString.c_str());
        
        string currentModifiers = "";
        
        if(isShift)
            currentModifiers += "Shift+";
        
        if(isOption)
            currentModifiers += "Option+";
        
        if(isControl)
            currentModifiers += "Control+";
        
        if(isAlt)
            currentModifiers += "Alt+";
        
        if(currentModifiers == actionLineItem.modifiers && currentWidget->GetName() == actionLineItem.widgetName)
        {
            zoneComponentWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ZoneComponents), LB_SETCURSEL, i, 0);
            
            SetCheckBoxes(actionLineItem);
        }
    }
    
    //Action Names
    vector<string> actionNames = TheManager->GetActionNames();
     
    for(int i = 0; i < actionNames.size(); i++)
    {
        string name = actionNames[i];
        
        SendDlgItemMessage(hwndLearn, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)name.c_str());
        
        if(name != FXParam && currentAction->GetName() == name)
        {
            actionNameWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_SETCURSEL, i, 0);
        }

    }
}

static WDL_DLGRET dlgProcAddZone(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /*
            if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &focusedFXIndex) == 1)
                if(trackNumber > 0)
                {
                    currentFXIndex = focusedFXIndex;
                    
                    focusedFXTrack = DAW::CSurf_TrackFromID(trackNumber, currentSurface->GetPage()->GetTrackNavigationManager()->GetFollowMCP());
                    
                    DAW::TrackFX_GetFXName(focusedFXTrack, focusedFXIndex, buffer, sizeof(buffer));
                    
                    focusedFXName = string(buffer);
                    
                    string focusedFXFilename =  regex_replace(focusedFXName, regex(BadFileChars), "_");
                    
                    SetDlgItemText(hwndDlg, IDC_EDIT_ZoneFileName, focusedFXFilename.c_str());
                    SetDlgItemText(hwndDlg, IDC_EDIT_ZoneName, focusedFXName.c_str());
                }
            */
            
            AddComboBoxEntry(hwndDlg, 0, "NoNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 1, "TrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 2, "SelectedTrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 3, "FocusedFXNavigator", IDC_COMBO_Navigator);
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 2, 0);
        }
            break;
            
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
                        
                        int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_GETCURSEL, 0, 0);
                        if(index >= 0)
                        {
                            if(index == 2)
                                trackNavigatorName = "SelectedTrackNavigator";
                            else if (index == 3)
                                trackNavigatorName = "FocusedFXNavigator";
                            else
                                trackNavigatorName = "";
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
                        int zoneIndex = SendDlgItemMessage(hwndLearn, IDC_LIST_Zones, LB_GETCURSEL, 0, 0);
                        if (zoneIndex >= 0)
                        {
                            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_Zones), LB_GETTEXT, zoneIndex, (LPARAM)(LPCTSTR)(buffer));
                            string zoneName = string(buffer);

                            int includedZoneIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_IncludedZone), CB_GETCURSEL, 0, 0);
                            
                            if(includedZoneIndex > 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_IncludedZone), CB_GETLBTEXT, includedZoneIndex, (LPARAM)(LPCTSTR)(buffer));
                                string includedZoneName = string(buffer);

                                if(currentSurface->GetZones().count(zoneName) > 0)
                                {
                                    Zone* enclosingZone = currentSurface->GetZones()[zoneName];
                                    
                                    if(currentSurface->GetZones().count(includedZoneName) > 0)
                                    {
                                        Zone* includedZone = currentSurface->GetZones()[includedZoneName];
                                        enclosingZone->AddZone(includedZone);
                                        SendDlgItemMessage(hwndLearn, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)includedZoneName.c_str());
                                    }
                                }
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
                    currentFXIndex = focusedFXIndex;
                    
                    focusedFXTrack = DAW::CSurf_TrackFromID(trackNumber, currentSurface->GetPage()->GetTrackNavigationManager()->GetFollowMCP());
                
                    DAW::TrackFX_GetFXName(focusedFXTrack, focusedFXIndex, buffer, sizeof(buffer));

                    focusedFXName = string(buffer);
                    
                    string focusedFXFilename =  regex_replace(focusedFXName, regex(BadFileChars), "_");

                    SetDlgItemText(hwndDlg, IDC_EDIT_ZoneFileName, focusedFXFilename.c_str());
                    SetDlgItemText(hwndDlg, IDC_EDIT_ZoneName, focusedFXName.c_str());
                }
            
            AddComboBoxEntry(hwndDlg, 0, "NoNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 1, "TrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 2, "SelectedTrackNavigator", IDC_COMBO_Navigator);
            AddComboBoxEntry(hwndDlg, 3, "FocusedFXNavigator", IDC_COMBO_Navigator);
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_SETCURSEL, 2, 0);
        }
            break;
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneFileName , buffer, sizeof(buffer));
                        newZoneFilename = string(buffer);
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneName , buffer, sizeof(buffer));
                        newZoneName = string(buffer);
                        
                        GetDlgItemText(hwndDlg, IDC_EDIT_ZoneAlias , buffer, sizeof(buffer));
                        newZoneAlias = string(buffer);
                        
                        int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_GETCURSEL, 0, 0);
                        if(index >= 0)
                        {
                            if(index == 2)
                                trackNavigatorName = "SelectedTrackNavigator";
                            else if (index == 3)
                                trackNavigatorName = "FocusedFXNavigator";
                            else
                                trackNavigatorName = "";
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
    }
    
    return 0 ;
}

static WDL_DLGRET dlgProcLearn(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_HSCROLL:
        {
            if(currentAction)
            {
                hasEdits = true;

                HWND sliderHwnd = (HWND)lParam;
                
                int sliderPos = (int)SendMessage(sliderHwnd, TBM_GETPOS, 0, 0);

                rgb_color color = currentAction->GetCurrentRGB();
                
                if(sliderHwnd == GetDlgItem(hwndDlg, IDC_SLIDER_Red))
                    color.r = sliderPos;
                else if(sliderHwnd == GetDlgItem(hwndDlg, IDC_SLIDER_Green))
                    color.g = sliderPos;
                else if(sliderHwnd == GetDlgItem(hwndDlg, IDC_SLIDER_Blue))
                    color.b = sliderPos;

                currentAction->SetCurrentRGB(color);
            }
            break;
        }
            
        case WM_USER+1024:
        {
            currentSurface = currentWidget->GetSurface();
            EnableButtons();

            ClearWidgets();
            
            SetDlgItemText(hwndDlg, IDC_EDIT_WidgetName, currentWidget->GetName().c_str());
            
            int index = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceName), CB_FINDSTRING, -1, (LPARAM)currentSurface->GetSourceFileName().c_str());
            if(index >= 0)
                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceName), CB_SETCURSEL, index, 0);
            
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, "");
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, "");
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, "");

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
            Zone* zone = currentWidgetActionManager->GetZone();
            
            int zoneIndex = FillZones(zone);
            
            FillSubZones(zone, zoneIndex);
            
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, currentAction->GetName().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, currentAction->GetParamAsString().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, currentAction->GetAlias().c_str());

            rgb_color color = currentAction->GetCurrentRGB();

            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Red), TBM_SETPOS, 1, color.r);
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Green), TBM_SETPOS, 1, color.g);
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Blue), TBM_SETPOS, 1, color.b);

            break;
        }

        case WM_INITDIALOG:
        {
            hasEdits = false;
            
            for(auto surface : currentPage->GetSurfaces())
                AddComboBoxEntry(hwndDlg, 0, surface->GetSourceFileName().c_str(), IDC_COMBO_SurfaceName);
            
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceName), CB_SETCURSEL, 0, 0);

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
            
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Red), TBM_SETRANGE, 1, MAKELONG(0, 255));
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Red), TBM_SETPOS, 1, 0);
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Green), TBM_SETRANGE, 1, MAKELONG(0, 255));
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Green), TBM_SETPOS, 1, 0);
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Blue), TBM_SETRANGE, 1, MAKELONG(0, 255));
            SendMessage (GetDlgItem(hwndDlg, IDC_SLIDER_Blue), TBM_SETPOS, 1, 0);

            break;
        }
            
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_EDIT_Alias:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        int zoneIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);

                        if(zoneIndex >= 0)
                        {
                            GetDlgItemText(hwndDlg, IDC_EDIT_Alias, buffer, sizeof(buffer));
                            zonesInThisFile[zoneIndex]->SetAlias(string(buffer));
                        }
                    }
                    break ;

                case IDC_BUTTON_SaveFile:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if(zonesInThisFile.size() > 0)
                        {
                            string filePath = zonesInThisFile[0]->GetSourceFilePath();

                            try
                            {
                                ofstream zonFile(filePath);
                                
                                if(zonFile.is_open())
                                {
                                    for(auto zone : zonesInThisFile)
                                    {
                                        zonFile << "Zone \"" + zone->GetName() + "\"";
                                        
                                        if (zone->GetAlias() != "" && zone->GetAlias() != zone->GetName())
                                            zonFile << " \"" + zone->GetAlias() + "\"";
                                        
                                        zonFile << GetLineEnding();
                                        
                                        if(zone->GetNavigatorName() != "")
                                            zonFile << zone->GetNavigatorName() + GetLineEnding();
                                        
                                        if(zone->GetIncludedZones().size() > 0)
                                        {
                                            zonFile << "IncludedZones" + GetLineEnding();

                                            for(auto includedZone : zone->GetIncludedZones())
                                                zonFile << includedZone->GetName() + GetLineEnding();
                                            
                                            zonFile << "IncludedZonesEnd" + GetLineEnding();
                                        }
                                        
                                        for(auto actionLineItem : zone->GetActionLineItems())
                                        {
                                            string lineItemAsString = actionLineItem.allModifiers + actionLineItem.widgetName + " " + actionLineItem.actionName;
                                            
                                            if(actionLineItem.param != "")
                                                lineItemAsString += " \"" + actionLineItem.param + "\"";
                                            
                                            if(actionLineItem.alias != "")
                                                lineItemAsString += " \"" + actionLineItem.alias + "\"";
                                            
                                            if(actionLineItem.supportsRGB)
                                                for(auto color : actionLineItem.colors)
                                                    lineItemAsString += " " + to_string(color.r) + " " + to_string(color.g) + " " + to_string(color.b);
                                            
                                            zonFile << lineItemAsString + GetLineEnding();

                                        }
                                        
                                        zonFile << "ZoneEnd" + GetLineEnding() + GetLineEnding();
                                    }
                                }
                                
                                zonFile.close();
                                hasEdits = false;
                            }
                            catch (exception &e)
                            {
                                char buffer[250];
                                sprintf(buffer, "Trouble writing %s", filePath.c_str());
                                DAW::ShowConsoleMsg(buffer);
                            }
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_NewFile:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        trackNumber = 0;
                        itemNumber = 0;
                        focusedFXIndex = 0;
                        focusedFXTrack = nullptr;
                        focusedFXName = "";

                        char filenameNeed4096[BUFSZ * 16];
                        string title = "New Zone file";
                        string defext = ".zon";
                        
                        strcpy(filenameNeed4096, (string(DAW::GetResourcePath()) + "/CSI/Zones/" + currentSurface->GetName() + "/*.*").c_str());
                        
                        bool result = GetUserFileNameForRead(filenameNeed4096, title.c_str(), defext.c_str());

                        if(result == false)
                            break;
                        
                        istringstream path(filenameNeed4096);
                        vector<string> pathFolders;
                        string pathfolder;
                        
                        while (getline(path, pathfolder, '/'))
                            pathFolders.push_back(pathfolder);

                        string filePath = "";
                        
                        for(int i = 0; i < pathFolders.size() - 1; i++)
                            filePath += pathFolders[i] + "/";
                        
                        dlgResult = false;
                        DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_NewZoneFile), g_hwnd, dlgProcNewZoneFile);
                        if(dlgResult == IDOK)
                        {
                            hasEdits = true;
                            
                            ClearActions();
                            ClearZones();
                            ClearSubZones();
                            
                            if(focusedFXTrack && focusedFXName == newZoneName)
                                LoadRawFXFile(focusedFXTrack, focusedFXName);
                            
                            for(auto name : TheManager->GetActionNames())
                                if(name != Shift && name != Option && name != Control && name != Alt)
                                    SendDlgItemMessage(hwndDlg, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)name.c_str());
                            
                            newZoneFilename = filePath + newZoneFilename + ".zon";
                            
                            Zone* newZone = new Zone(currentSurface, newZoneName, newZoneFilename, newZoneAlias);
                            
                            if(trackNavigatorName == "SelectedTrackNavigator")
                                newZone->SetTrackNavigator(new SelectedTrackNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));
                            else if(trackNavigatorName == "FocusedFXNavigator")
                                newZone->SetTrackNavigator(new FocusedFXNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));

                            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_Navigator), trackNavigatorName.c_str());
                            
                            currentSurface->AddZone(newZone);
                            zonesInThisFile = currentSurface->GetZonesInZoneFile()[newZoneFilename];
                            
                            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_ZoneFilename), newZoneFilename.c_str());
                            if(newZoneName != newZoneAlias)
                                SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_Alias), newZoneAlias.c_str());
                            else
                                SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), "");
                            SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_Alias), newZoneAlias.c_str());
                            SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_ADDSTRING, 0, (LPARAM)newZoneName.c_str());
                            zoneWasSelectedBySurface = true;
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_SETCURSEL, (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCOUNT, 0, 0) - 1, 0);
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
                            if(zonesInThisFile.size() > 0)
                            {
                                hasEdits = true;
                                ClearSubZones();
                                
                                string zoneFilename = zonesInThisFile[0]->GetSourceFilePath();
                                
                                Zone* newZone = new Zone(currentSurface, newZoneName, zoneFilename, newZoneAlias);
                                
                                if(trackNavigatorName == "SelectedTrackNavigator")
                                    newZone->SetTrackNavigator(new SelectedTrackNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));
                                else if(trackNavigatorName == "FocusedFXNavigator")
                                    newZone->SetTrackNavigator(new FocusedFXNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));

                                SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_Navigator), trackNavigatorName.c_str());

                                currentSurface->AddZone(newZone);
                                zonesInThisFile = currentSurface->GetZonesInZoneFile()[newZoneFilename];
                                if(newZoneName != newZoneAlias)
                                    SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_Alias), newZoneAlias.c_str());
                                else
                                    SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), "");
                                SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_ADDSTRING, 0, (LPARAM)newZoneName.c_str());
                                zoneWasSelectedBySurface = true;
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_SETCURSEL, (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCOUNT, 0, 0) - 1, 0);
                            }
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
                            hasEdits = true;
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_AddLineItem:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int zoneIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
                        int widgetIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETCURSEL, 0, 0);
                        int actionIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETCURSEL, 0, 0);

                        if(zoneIndex >= 0 && widgetIndex >= 0 && actionIndex >= 0)
                        {
                            hasEdits = true;
                            
                            ActionLineItem actionLineItem;
                            
                            int navIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_GETCURSEL, 0, 0);
                            
                            if(navIndex >= 0)
                            {
                                SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_Navigator), CB_GETLBTEXT, navIndex, (LPARAM)(LPCTSTR)(buffer));
                                actionLineItem.navigator = string(buffer);
                            }
                            
                            actionLineItem.isShift = isShift;
                            actionLineItem.isOption = isOption;
                            actionLineItem.isControl = isControl;
                            actionLineItem.isAlt = isAlt;
                            actionLineItem.isToggle = shouldToggle;
                            actionLineItem.isInvert = isInvert;
                            actionLineItem.isTouch = isTouch;
                            actionLineItem.isHold = isHold;
                            
                            if(actionLineItem.isShift)
                                actionLineItem.modifiers += "Shift+";
                            
                            if(actionLineItem.isOption)
                                actionLineItem.modifiers += "Option+";
                            
                            if(actionLineItem.isControl)
                                actionLineItem.modifiers += "Control+";
                            
                            if(actionLineItem.isAlt)
                                actionLineItem.modifiers += "Altt+";
                            
                            actionLineItem.allModifiers = actionLineItem.modifiers;
                            
                            if(actionLineItem.isToggle)
                                actionLineItem.allModifiers += "Toggle+";
                            
                            if(actionLineItem.isInvert)
                                actionLineItem.allModifiers += "Invert+";
                            
                            if(actionLineItem.isHold)
                                actionLineItem.allModifiers += "Hold+";

                            actionLineItem.widget = currentSurface->GetWidgets()[widgetIndex];
                            actionLineItem.widgetName = actionLineItem.widget->GetName();

                            GetDlgItemText(hwndDlg, IDC_EDIT_ActionName, buffer, sizeof(buffer));
                            actionLineItem.actionName = string(buffer);
                            
                            if(actionLineItem.actionName == FXParam)
                            {
                                actionLineItem.param = to_string(actionIndex);
                            }
                            else
                            {
                                GetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter , buffer, sizeof(buffer));
                                actionLineItem.param = string(buffer);
                            }
                            
                            GetDlgItemText(hwndDlg, IDC_EDIT_ActionAlias, buffer, sizeof(buffer));
                            actionLineItem.alias = string(buffer);
                            
                            zonesInThisFile[zoneIndex]->AddAction(actionLineItem, currentFXIndex);
                            zonesInThisFile[zoneIndex]->Activate(currentFXIndex);
                            
                            string lineString = actionLineItem.modifiers + actionLineItem.widgetName + " " + actionLineItem.actionName;
                            
                            if(actionLineItem.param != "")
                                lineString += " " + actionLineItem.param;
                            
                            if(actionLineItem.alias != "")
                                lineString += " " + actionLineItem.alias;
                            
                            SetCheckBoxes(actionLineItem);

                            SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_ADDSTRING, 0, (LPARAM)lineString.c_str());
                            zoneComponentWasSelectedBySurface = true;
                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_SETCURSEL, (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ZoneComponents), LB_GETCOUNT, 0, 0) - 1, 0);
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_DeleteZone:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int index = SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_GETCURSEL, 0, 0);
                        if (index >= 0)
                        {
                            hasEdits = true;
                            Zone* zoneToDelete = zonesInThisFile[index];
                            currentSurface->RemoveZone(zoneToDelete, index);
                            
                            SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_DELETESTRING, index, 0);
                            SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_Alias), "");

                            ClearSubZones();

                            if(zonesInThisFile.size() > 0)
                            {
                                zoneWasSelectedBySurface = true;
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_SETCURSEL, 0, 0);
                                if(zonesInThisFile[0]->GetName() != zonesInThisFile[0]->GetAlias())
                                    SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), zonesInThisFile[0]->GetAlias().c_str());
                                FillSubZones(zonesInThisFile[0], 0);
                            }
                        }
                    }
                    break ;
                    
                case IDC_BUTTON_DeleteIncludedZone:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int index = SendDlgItemMessage(hwndDlg, IDC_LIST_IncludedZones, LB_GETCURSEL, 0, 0);
                        if (index >= 0)
                        {
                            int zoneIndex = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
                            if(zoneIndex >= 0)
                            {
                                hasEdits = true;
                                Zone* zone = zonesInThisFile[zoneIndex];
                                zone->RemoveZone(index);
                                
                                SendMessage(GetDlgItem(hwndLearn, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
                                for(auto includedZone : zone->GetIncludedZones())
                                    SendDlgItemMessage(hwndLearn, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)includedZone->GetName().c_str());
                            }
                        }
                        break ;
                    }
                    
                case IDC_BUTTON_DeleteLineItem:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        int zoneIndex = SendDlgItemMessage(hwndDlg, IDC_LIST_Zones, LB_GETCURSEL, 0, 0);
                        
                        if(zoneIndex >= 0)
                        {
                            int index = SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_GETCURSEL, 0, 0);
                            if (index >= 0)
                            {
                                hasEdits = true;
                           
                                ActionLineItem actionLineItem = zonesInThisFile[zoneIndex]->GetActionLineItems()[index];

                                zonesInThisFile[zoneIndex]->RemoveAction(actionLineItem);
                                
                                SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_DELETESTRING, index, 0);
                            }
                        }
                    }
                    break ;
                }
                    
                case IDC_COMBO_SurfaceName:
                {
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_SurfaceName), CB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                ClearWidgets();
                                ClearZones();
                                ClearSubZones();
                                ClearActions();
                                
                                currentSurface = currentPage->GetSurfaces()[index];
                                
                                for(auto widget : currentSurface->GetWidgets())
                                    SendDlgItemMessage(hwndDlg, IDC_LIST_WidgetNames, LB_ADDSTRING, 0, (LPARAM)widget->GetName().c_str());
                                
                                currentWidget = nullptr;
                                
                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_SETCURSEL, 0, 0);
                            }
                        }
                    }
                    
                    break;
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
                                currentSurface->GetWidgets()[index]->DoAction(currentSurface->GetWidgets()[index]->GetLastValue());

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
                                    SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, FXParam.c_str());
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

                            int index = (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
                            if(index >= 0)
                            {
                                FillSubZones(zonesInThisFile[index], index);
                                if(zonesInThisFile[index]->GetName() != zonesInThisFile[index]->GetAlias())
                                    SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), zonesInThisFile[index]->GetAlias().c_str());
                                else
                                    SetWindowText(GetDlgItem(hwndLearn, IDC_EDIT_Alias), "");
                            }
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
                                    ActionLineItem actionLineItem = zonesInThisFile[zoneIndex]->GetActionLineItems()[index];

                                    SetCheckBoxes(actionLineItem);
                                    
                                    for(int i = 0; i < (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETCOUNT, 0, 0); i++)
                                    {
                                        SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(buffer));
                                        if(string(buffer) == actionLineItem.widgetName)
                                        {
                                            widgetNameWasSelectedBySurface = true;
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_WidgetNames), LB_SETCURSEL, i, 0);
                                            break;
                                        }
                                    }
                                    
                                    if(actionLineItem.actionName.find(FXParam) != string::npos)
                                    {
                                        actionNameWasSelectedBySurface = true;
                                        SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, atoi(actionLineItem.param.c_str()), 0);
                                    }
                                    else
                                    {
                                        for(int i = 0; i < (int)SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETCOUNT, 0, 0); i++)
                                        {
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_GETTEXT, i, (LPARAM)(LPCTSTR)(buffer));
                                            if(string(buffer) == actionLineItem.actionName)
                                            {
                                                actionNameWasSelectedBySurface = true;
                                                SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ActionNames), LB_SETCURSEL, i, 0);
                                                break;
                                            }
                                        }
                                    }
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
    }
    
    return 0 ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Page
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Page::OpenLearnModeWindow()
{
    currentPage = this;
    
    if(hwndLearn == nullptr)
    {
        hwndLearn = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Learn), g_hwnd, dlgProcLearn);
        DisableButtons();
        ShowWindow(hwndLearn, true);
    }
}

void Page::ToggleLearnMode()
{
    currentPage = this;

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
    currentPage = this;

    if(hwndLearn == nullptr)
        return;
    
    if(widget == currentWidget)
        return;
    
    currentWidget = widget;

    if(currentWidget != nullptr)
        SendMessage(hwndLearn, WM_USER+1024, 0, 0);
}

void Page::ActionPerformed(WidgetActionManager* widgetActionManager, Action* action)
{
    currentPage = this;

    if(hwndLearn == nullptr)
        return;

    if(widgetActionManager == nullptr || action == nullptr)
        return;

    if(widgetActionManager == currentWidgetActionManager)
        return;
    
    currentWidget = widgetActionManager->GetWidget();
    currentWidgetActionManager = widgetActionManager;
    currentAction = action;
    
    isShift = isShift_;
    isOption = isOption_;
    isControl = isControl_;
    isAlt = isAlt_;
   
    if(currentWidget != nullptr && currentWidgetActionManager != nullptr && currentAction != nullptr)
        SendMessage(hwndLearn, WM_USER+1025, 0, 0);
}




/*
 string filePath = string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + "LaunchPadRAW.txt";
 
 string outputFilePath = string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename;
 
 
 if(name_ == "LaunchPad")
 {
 vector<string> rows = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K" };
 int rowIndex = 0;
 int columnIndex = 1;
 
 
 
 try
 {
 ofstream outputFile(outputFilePath);
 
 if(outputFile.is_open())
 {
 
 
 ifstream file(filePath);
 
 
 for (string line; getline(file, line) ; )
 {
 line = regex_replace(line, regex(CRLFChars), "");
 
 
 if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
 continue;
 
 vector<string> tokens(GetTokens(line));
 
 
 if(tokens.size() == 6 && tokens[5] == "7f")
 {
 outputFile << "Widget Button" + rows[rowIndex] + to_string(columnIndex)  + GetLineEnding();
 
 outputFile << "Press " + tokens[3] + " " + tokens[4] + " " + tokens[5] + " " + tokens[3] + " " + tokens[4] + " 00"  + GetLineEnding();
 outputFile << "FB_NovationLaunchpadMiniRGB7Bit " + tokens[3] + " " + tokens[4] + " " + tokens[5] + GetLineEnding();
 outputFile << "WidgetEnd" + GetLineEnding() + GetLineEnding();
 
 if(columnIndex % 9 == 0)
 {
 rowIndex++;
 columnIndex = 1;
 }
 else
 columnIndex++;
 }
 }
 }
 
 
 
 outputFile.close();
 
 
 
 
 }
 catch (exception &e)
 {
 char buffer[250];
 //sprintf(buffer, "Trouble in %s, around line %d\n", filePath.c_str(), lineNumber);
 DAW::ShowConsoleMsg(buffer);
 }
 
 
 
 }
 
 */
