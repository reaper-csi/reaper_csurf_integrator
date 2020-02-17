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
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
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

static map<int, Navigator*> navigators;

static Navigator* GetNavigatorForChannel(Zone* zone, ControlSurface* surface, int channelNum)
{
    if(channelNum < 0)
        return nullptr;
    
    if(navigators.count(channelNum) < 1)
        navigators[channelNum] = surface->GetPage()->GetTrackNavigationManager()->AddNavigator(zone);
    
    return navigators[channelNum];
}

static void BuildIncludedZone(string includedZoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone);

static map<string, vector<vector<string>>> zoneTemplates;
static map<string, vector<vector<string>>> zoneDefinitions;

static void BuildZone(vector<vector<string>> &zoneLines, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone, int channelNum)
{
    const string FXGainReductionMeter = "FXGainReductionMeter"; // GAW TBD don't forget this logic

    Zone* zone = nullptr;
    vector<string> includedZones;
    bool isInIncludedZonesSection = false;
    

    for(int i = 0 ; i < zoneLines.size(); i++)
    {
        auto tokens = zoneLines[i];
        
        if(tokens.size() > 0 && tokens[0] == "Zone")
        {
            if(tokens.size() < 2)
                return;
            
            string navigatorName = "";

            if(zoneLines.size() > i + 1) // we have another line
            {
                auto navTokens = zoneLines[i + 1];
            
                if(navTokens.size() == 1 && (navTokens[0] == "TrackNavigator" || navTokens[0] == "MasterTrackNavigator" || navTokens[0] == "SelectedTrackNavigator"
                                             || navTokens[0] == "FocusedFXNavigator" || navTokens[0] == "ParentNavigator"))
                {
                    i++;
                    navigatorName = navTokens[0];
                }
            }
            
            zone = new Zone(navigatorName, surface, parentZone, channelNum, tokens[1], filePath, tokens.size() > 2 ? tokens[2] : tokens[1]); // tokens[2] == alias, if provided, otherwise just use name (tokens[1])

            if(zone == nullptr)
                return;
            
            if(parentZone != nullptr)
                parentZone->AddZone(zone);
            else
                surface->AddZone(zone);
            
            
            continue;
        }
    
        if(tokens.size() > 0)
        {
            if(tokens[0] == "ZoneEnd")    // finito baybay - Zone processing complete
                return;
            
            if(tokens[0] == "IncludedZones")
            {
                isInIncludedZonesSection = true;
                continue;
            }
            
            if(tokens[0] == "IncludedZonesEnd")
            {
                isInIncludedZonesSection = false;
                continue;
            }
            
            if(tokens.size() == 1 && isInIncludedZonesSection)
            {
                BuildIncludedZone(tokens[0], filePath, surface, widgets, zone);
                continue;
            }
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
                Action* action = TheManager->GetAction(widget, zone, params);
                
                if(action != nullptr)
                {
                    if(isTrackTouch)
                        widget->AddTrackTouchedAction(zone, modifiers, action);
                    else
                        widget->AddAction(zone, modifiers, action);
                    
                    if(isInverted)
                        action->SetIsInverted();
                    
                    if(shouldToggle)
                        action->SetShouldToggle();
                    
                    if(isDelayed)
                        action->SetDelayAmount(delayAmount * 1000.0);

                    if(params[0] == Shift || params[0] == Option || params[0] == Control || params[0] == Alt)
                        widget->SetIsModifier();
                }
            }
            else
            {
                // log error, etc.
            }
        }
    }
}

static void BuildExpandedZones(string zoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone)
{
    istringstream expandedZone(zoneName);
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
        
        if(zoneTemplates.count(zoneBaseName + "|") > 0)
        {
            istringstream range(expandedZoneTokens[1]);
            vector<string> rangeTokens;
            string rangeToken;
            
            while (getline(range, rangeToken, '-'))
                rangeTokens.push_back(rangeToken);
            
                if(rangeTokens.size() > 1)
                {
                    rangeBegin = stoi(rangeTokens[0]);
                    rangeEnd = stoi(rangeTokens[1]);
                    
                    for(int i = 0; i <= rangeEnd - rangeBegin; i++)
                    {
                        vector<vector<string>> zoneLines;
                        
                        bool isInIncludedZonesSection = false;
                        
                        for(auto line : zoneTemplates[zoneBaseName + "|"])
                        {
                            zoneLines.push_back(vector<string>());
                            
                            for(auto token : line)
                            {
                                if(token == "IncludedZones")
                                    isInIncludedZonesSection = true;
                                
                                if(token == "IncludedZonesEnd")
                                    isInIncludedZonesSection = false;
                            
                                if(isInIncludedZonesSection)
                                    zoneLines.back().push_back(token);
                                else
                                    zoneLines.back().push_back(regex_replace(token, regex("[|]"), to_string(i + 1)));
                            }
                        }
                        
                        BuildZone(zoneLines, filePath, surface, widgets, parentZone, i);
                    }
                }
        }
    }
}

static void BuildIncludedZone(string includedZoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone)
{
    if(includedZoneName.back() == '|')
    {
        if(zoneTemplates.count(includedZoneName) > 0)
            BuildZone(zoneTemplates[includedZoneName], filePath, surface, widgets, parentZone, -1); // track ptr == nullptr
    }
    else if(regex_search(includedZoneName, regex("[|][0-9]+[-][0-9]+"))) // This expands constructs like Channel|1-8 into multiple Zones
        BuildExpandedZones(includedZoneName, filePath, surface, widgets, parentZone);
}

static void ProcessZoneFile(string filePath, ControlSurface* surface, vector<Widget*> &widgets)
{
    string zoneName = "";
    zoneTemplates.clear();
    zoneDefinitions.clear();
    int lineNumber = 0;
    
    try
    {
        ifstream file(filePath);
        
        bool isTemplate = false;
        bool isTemplateAndDefintion = false;

        for (string line; getline(file, line) ; )
        {
            line = regex_replace(line, regex(TabChars), " ");
            line = regex_replace(line, regex(CRLFChars), "");
            
            line = line.substr(0, line.find("//")); // remove trailing commewnts
            
            lineNumber++;
            
            // Trim leading and trailing spaces
            line = regex_replace(line, regex("^\\s+|\\s+$"), "", regex_constants::format_default);
            
            if(line == "" || (line.size() > 0 && line[0] == '/')) // ignore blank lines and comment lines
                continue;
            
            vector<string> tokens(GetTokens(line));
            
            if(tokens.size() > 0)
            {
                if(tokens[0] == "Zone")
                {
                    zoneName = tokens[1];
                    
                    if(zoneName.size() > 1 && zoneName.back() == '|')
                        isTemplate = true;
                    else if(regex_search(zoneName, regex("[|][0-9]+[-][0-9]+")))
                        isTemplateAndDefintion = true;
                }
             
                if(tokens[0] == "ZoneEnd")
                {
                    isTemplate = false;
                    isTemplateAndDefintion = false;
                    continue;
                }
                
                if(isTemplateAndDefintion)
                {
                    zoneDefinitions[zoneName].push_back(tokens);
                    
                    string zoneTemplateName = regex_replace(zoneName, regex("[|][0-9]+[-][0-9]+"), "|", regex_constants::format_default);
                    vector<string> templateTokens;
                    templateTokens.assign(tokens.begin(), tokens.end());
                    for(int i = 0; i < templateTokens.size(); i++)
                        if(regex_search(templateTokens[i], regex("[|][0-9]+[-][0-9]+")))
                            templateTokens[i] = regex_replace(templateTokens[i], regex("[|][0-9]+[-][0-9]+"), "|", regex_constants::format_default);
                    zoneTemplates[zoneTemplateName].push_back(templateTokens);
                }
                else if(isTemplate)
                    zoneTemplates[zoneName].push_back(tokens);
                else
                    zoneDefinitions[zoneName].push_back(tokens);
             
             }
        }
    }
    catch (exception &e)
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble in %s, around line %d\n", filePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
    
    for(auto [zoneName, zoneLines] : zoneDefinitions)
    {
        if(regex_search(zoneName, regex("[|][0-9]+[-][0-9]+"))) // This expands constructs like PanWidth|1-8 into multiple Zones
            BuildExpandedZones(zoneName, filePath, surface, widgets, nullptr);
        else
            BuildZone(zoneLines, filePath, surface, widgets, nullptr, -1); // Start with the outermost Zones -- parentZone == nullptr, track ptr == nullptr
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
    
    string widgetName = tokens[1];

    vector<vector<string>> tokenLines;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        line = regex_replace(line, regex(TabChars), " ");
        line = regex_replace(line, regex(CRLFChars), "");

        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens[0] == "WidgetEnd")    // finito baybay - Widget processing complete
            break;
        
        tokenLines.push_back(tokens);
    }
    
    if(tokenLines.size() < 1)
        return;
    
    bool has_FB_Processor = false;
    int FB_ProcessorIndex = 0;
    
    for(int i = 0; i < tokenLines.size(); i++)
    {
        if(tokenLines[i].size() > 0)
        {
            if(tokenLines[i][0].size() > 2 && tokenLines[i][0].substr(0, 3) == "FB_")
            {
                has_FB_Processor = true;
                FB_ProcessorIndex = i;
                break;
            }
        }
    }
    
    FeedbackProcessor* feedbackProcessor = nullptr;

    if(tokenLines.size() > 0 && has_FB_Processor)
    {
        int size = tokenLines[FB_ProcessorIndex].size();
        
        string widgetClass = tokenLines[FB_ProcessorIndex][0];
        
        string byte1 = "";
        string byte2 = "";
        string byte3 = "";
        string byte4 = "";
        string byte5 = "";
        string byte6 = "";

        if(size > 3)
        {
            byte1 = tokenLines[FB_ProcessorIndex][1];
            byte2 = tokenLines[FB_ProcessorIndex][2];
            byte3 = tokenLines[FB_ProcessorIndex][3];
        }
        
        if(size > 6)
        {
            byte4 = tokenLines[FB_ProcessorIndex][4];
            byte5 = tokenLines[FB_ProcessorIndex][5];
            byte6 = tokenLines[FB_ProcessorIndex][6];
        }
        
        // Feedback Processors
        if(widgetClass == "FB_TwoState" && (size == 7 || size == 8))
        {
            feedbackProcessor = new TwoState_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)), new MIDI_event_ex_t(strToHex(byte4), strToHex(byte5), strToHex(byte6)));
            
            if(size == 8)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[FB_ProcessorIndex][7]));
        }
        else if(widgetClass == "FB_NovationLaunchpadMiniRGB7Bit" && size == 4)
        {
            feedbackProcessor = new NovationLaunchpadMiniRGB7Bit_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        }
        else if(widgetClass == "FB_FaderportRGB7Bit" && size == 4)
        {
            feedbackProcessor = new FaderportRGB7Bit_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        }
        else if(size == 4 || size== 5)
        {
            if(widgetClass == "FB_Fader14Bit")
                feedbackProcessor = new Fader14Bit_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
            else if(widgetClass == "FB_Fader7Bit")
                feedbackProcessor = new Fader7Bit_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
            else if(widgetClass == "FB_Encoder")
                feedbackProcessor = new Encoder_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
            else if(widgetClass == "FB_VUMeter")
                feedbackProcessor = new VUMeter_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
            else if(widgetClass == "FB_GainReductionMeter")
                feedbackProcessor = new GainReductionMeter_Midi_FeedbackProcessor(surface, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
            else if(widgetClass == "FB_QConProXMasterVUMeter")
                feedbackProcessor = new QConProXMasterVUMeter_Midi_FeedbackProcessor(surface);
            
            if(size == 5 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[FB_ProcessorIndex][4]));
        }
        else if(widgetClass == "FB_MCUTimeDisplay" && size == 1)
        {
            feedbackProcessor = new MCU_TimeDisplay_Midi_FeedbackProcessor(surface);
        }
        else if((widgetClass == "FB_MCUVUMeter" || widgetClass == "FB_MCUXTVUMeter") && (size == 2 || size == 3))
        {
            int displayType = widgetClass == "FB_MCUVUMeter" ? 0x14 : 0x15;
            
            feedbackProcessor = new MCUVUMeter_Midi_FeedbackProcessor(surface, displayType, stoi(tokenLines[FB_ProcessorIndex][1]));
            
            if(size == 3 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[FB_ProcessorIndex][2]));
        }
        else if((widgetClass == "FB_MCUDisplayUpper" || widgetClass == "FB_MCUDisplayLower" || widgetClass == "FB_MCUXTDisplayUpper" || widgetClass == "FB_MCUXTDisplayLower") && (size == 2 || size == 3))
        {
            if(widgetClass == "FB_MCUDisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 0, 0x14, 0x12, stoi(tokenLines[FB_ProcessorIndex][1]));
            else if(widgetClass == "FB_MCUDisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 1, 0x14, 0x12, stoi(tokenLines[FB_ProcessorIndex][1]));
            else if(widgetClass == "FB_MCUXTDisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 0, 0x15, 0x12, stoi(tokenLines[FB_ProcessorIndex][1]));
            else if(widgetClass == "FB_MCUXTDisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 1, 0x15, 0x12, stoi(tokenLines[FB_ProcessorIndex][1]));
            
            if(size == 3 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[FB_ProcessorIndex][2]));
        }
        
        else if((widgetClass == "FB_C4DisplayUpper" || widgetClass == "FB_C4DisplayLower") && (size == 3 || size == 4))
        {
            if(widgetClass == "FB_C4DisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 0, 0x17, stoi(tokenLines[FB_ProcessorIndex][1]) + 0x30, stoi(tokenLines[FB_ProcessorIndex][2]));
            else if(widgetClass == "FB_C4DisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, 1, 0x17, stoi(tokenLines[FB_ProcessorIndex][1]) + 0x30, stoi(tokenLines[FB_ProcessorIndex][2]));
            
            if(size == 4 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[FB_ProcessorIndex][3]));
        }
    }

    if(feedbackProcessor == nullptr)
        feedbackProcessor = new FeedbackProcessor();
    
    Widget* widget = new Widget(surface, widgetName, feedbackProcessor);
    
    if(! widget)
        return;
    
    widgets.push_back(widget);
   
    for(int i = 0; i < tokenLines.size(); i++)
    {
        int size = tokenLines[i].size();
        
        string widgetClass = tokenLines[i][0];
        
        string byte1 = "";
        string byte2 = "";
        string byte3 = "";
        string byte4 = "";
        string byte5 = "";
        string byte6 = "";
        
        if(size > 3)
        {
            byte1 = tokenLines[i][1];
            byte2 = tokenLines[i][2];
            byte3 = tokenLines[i][3];
        }
        
        if(size > 6)
        {
            byte4 = tokenLines[i][4];
            byte5 = tokenLines[i][5];
            byte6 = tokenLines[i][6];
        }
        
        // Control Signal Generators
        if(widgetClass == "AnyPress" && (size == 4 || size == 7))
            new AnyPress_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        if(widgetClass == "Press" && size == 4)
            new PressRelease_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        else if(widgetClass == "Press" && size == 7)
            new PressRelease_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)), new MIDI_event_ex_t(strToHex(byte4), strToHex(byte5), strToHex(byte6)));
        else if(widgetClass == "Fader14Bit" && size == 4)
            new Fader14Bit_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        else if(widgetClass == "Fader7Bit" && size== 4)
            new Fader7Bit_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        else if(widgetClass == "Encoder" && size == 4)
            new Encoder_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
        else if(widgetClass == "EncoderPlain" && size == 4)
            new EncoderPlain_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(byte1), strToHex(byte2), strToHex(byte3)));
    }
}

static void ProcessOSCWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  OSC_ControlSurface* surface, vector<Widget*> &widgets)
{
    if(tokens.size() < 2)
        return;
    
    vector<vector<string>> tokenLines;

    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        line = regex_replace(line, regex(TabChars), " ");
        line = regex_replace(line, regex(CRLFChars), "");

        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens[0] == "WidgetEnd")    // finito baybay - Widget processing complete
            break;
        
        tokenLines.push_back(tokens);
    }
    
    FeedbackProcessor* feedbackProcessor = nullptr;

    for(auto tokenLine : tokenLines)
    {
        if(tokenLine.size() > 1 && tokenLine[0] == "FB_Processor")
        {
            feedbackProcessor = new OSC_FeedbackProcessor(surface, tokenLine[1]);
            break;
        }
    }
    
    if(feedbackProcessor == nullptr)
        feedbackProcessor = new FeedbackProcessor();

    Widget* widget = new Widget(surface, tokens[1], feedbackProcessor);
    
    if(! widget)
        return;
    
    widgets.push_back(widget);

    for(auto tokenLine : tokenLines)
        if(tokenLine.size() > 1 && tokenLine[0] == "Control")
            new OSC_CSIMessageGenerator(surface, widget, tokenLine[1]);
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
            
            if(tokens.size() > 0 && tokens[0] == "Widget")
            {
                if(filePath[filePath.length() - 3] == 'm')
                    ProcessMidiWidget(lineNumber, file, tokens, (Midi_ControlSurface*)surface, widgets);
                if(filePath[filePath.length() - 3] == 'o')
                    ProcessOSCWidget(lineNumber, file, tokens, (OSC_ControlSurface*)surface, widgets);
            }
        }
    }
    catch (exception &e)
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble in %s, around line %d\n", filePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Manager::InitActionDictionary()
{
    actions_["NoAction"] =                          [](string name, Widget* widget, Zone* zone, vector<string> params) { return new NoAction(name, widget, zone, params); };
    actions_["Reaper"] =                            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ReaperAction(name, widget, zone, params); };
    actions_["FXNameDisplay"] =                     [](string name, Widget* widget, Zone* zone, vector<string> params) { return new FXNameDisplay(name, widget, zone, params); };
    actions_[FXParam] =                             [](string name, Widget* widget, Zone* zone, vector<string> params) { return new class FXParam(name, widget, zone, params); };
    actions_["FXParamRelative"] =                   [](string name, Widget* widget, Zone* zone, vector<string> params) { return new FXParamRelative(name, widget, zone, params); };
    actions_["FXParamNameDisplay"] =                [](string name, Widget* widget, Zone* zone, vector<string> params) { return new FXParamNameDisplay(name, widget, zone, params); };
    actions_["FXParamValueDisplay"] =               [](string name, Widget* widget, Zone* zone, vector<string> params) { return new FXParamValueDisplay(name, widget, zone, params); };
    actions_["FXGainReductionMeter"] =              [](string name, Widget* widget, Zone* zone, vector<string> params) { return new FXGainReductionMeter(name, widget, zone, params); };
    actions_["TrackVolume"] =                       [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackVolume(name, widget, zone, params); };
    actions_["TrackVolumeDB"] =                     [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackVolumeDB(name, widget, zone, params); };
    actions_["TrackSendVolume"] =                   [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendVolume(name, widget, zone, params); };
    actions_["TrackSendVolumeDB"] =                 [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendVolumeDB(name, widget, zone, params); };
    actions_["TrackSendPan"] =                      [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendPan(name, widget, zone, params); };
    actions_["TrackSendMute"] =                     [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendMute(name, widget, zone, params); };
    actions_["TrackSendInvertPolarity"] =           [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendInvertPolarity(name, widget, zone, params); };
    actions_["TrackSendPrePost"] =                  [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendPrePost(name, widget, zone, params); };
    actions_["TrackPan"] =                          [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackPan(name, widget, zone, params); };
    actions_["TrackPanPercent"] =                   [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackPanPercent(name, widget, zone, params); };
    actions_["TrackPanWidth"] =                     [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackPanWidth(name, widget, zone, params); };
    actions_["TrackPanWidthPercent"] =              [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackPanWidthPercent(name, widget, zone, params); };
    actions_["TrackNameDisplay"] =                  [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackNameDisplay(name, widget, zone, params); };
    actions_["TrackVolumeDisplay"] =                [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackVolumeDisplay(name, widget, zone, params); };
    actions_["TrackSendNameDisplay"] =              [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendNameDisplay(name, widget, zone, params); };
    actions_["TrackSendVolumeDisplay"] =            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSendVolumeDisplay(name, widget, zone, params); };
    actions_["TrackPanDisplay"] =                   [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackPanDisplay(name, widget, zone, params); };
    actions_["TrackPanWidthDisplay"] =              [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackPanWidthDisplay(name, widget, zone, params); };
    actions_["TimeDisplay"] =                       [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TimeDisplay(name, widget, zone, params); };
    actions_["Rewind"] =                            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new Rewind(name, widget, zone, params); };
    actions_["FastForward"] =                       [](string name, Widget* widget, Zone* zone, vector<string> params) { return new FastForward(name, widget, zone, params); };
    actions_["Play"] =                              [](string name, Widget* widget, Zone* zone, vector<string> params) { return new Play(name, widget, zone, params); };
    actions_["Stop"] =                              [](string name, Widget* widget, Zone* zone, vector<string> params) { return new Stop(name, widget, zone, params); };
    actions_["Record"] =                            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new Record(name, widget, zone, params); };
    actions_["TrackFolderDive"] =                   [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackFolderDive(name, widget, zone, params); };
    actions_["TrackSelect"] =                       [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSelect(name, widget, zone, params); };
    actions_["TrackUniqueSelect"] =                 [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackUniqueSelect(name, widget, zone, params); };
    actions_["TrackRangeSelect"] =                  [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackRangeSelect(name, widget, zone, params); };
    actions_["TrackRecordArm"] =                    [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackRecordArm(name, widget, zone, params); };
    actions_["TrackMute"] =                         [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackMute(name, widget, zone, params); };
    actions_["TrackSolo"] =                         [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackSolo(name, widget, zone, params); };
    actions_["TrackTouch"] =                        [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SetTrackTouch(name, widget, zone, params); };
    actions_["CycleTimeline"] =                     [](string name, Widget* widget, Zone* zone, vector<string> params) { return new CycleTimeline(name, widget, zone, params); };
    actions_["TrackOutputMeter"] =                  [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackOutputMeter(name, widget, zone, params); };
    actions_["TrackOutputMeterAverageLR"] =         [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackOutputMeterAverageLR(name, widget, zone, params); };
    actions_["TrackOutputMeterMaxPeakLR"] =         [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackOutputMeterMaxPeakLR(name, widget, zone, params); };
    actions_["SetShowFXWindows"] =                  [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SetShowFXWindows(name, widget, zone, params); };
    actions_["ToggleScrollLink"] =                  [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ToggleScrollLink(name, widget, zone, params); };
    actions_["CycleTimeDisplayModes"] =             [](string name, Widget* widget, Zone* zone, vector<string> params) { return new CycleTimeDisplayModes(name, widget, zone, params); };
    actions_["NextPage"] =                          [](string name, Widget* widget, Zone* zone, vector<string> params) { return new GoNextPage(name, widget, zone, params); };
    actions_["GoPage"] =                            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new class GoPage(name, widget, zone, params); };
    actions_["GoZone"] =                            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new GoZone(name, widget, zone, params); };
    actions_["SelectTrackRelative"] =               [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SelectTrackRelative(name, widget, zone, params); };
    actions_["TrackBank"] =                         [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackBank(name, widget, zone, params); };
    actions_["Shift"] =                             [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SetShift(name, widget, zone, params); };
    actions_["Option"] =                            [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SetOption(name, widget, zone, params); };
    actions_["Control"] =                           [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SetControl(name, widget, zone, params); };
    actions_["Alt"] =                               [](string name, Widget* widget, Zone* zone, vector<string> params) { return new SetAlt(name, widget, zone, params); };
    actions_["TogglePin"] =                         [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TogglePin(name, widget, zone, params); };
    actions_["ToggleLearnMode"] =                   [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ToggleLearnMode(name, widget, zone, params); };
    actions_["ToggleMapSelectedTrackSends"] =       [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ToggleMapSelectedTrackSends(name, widget, zone, params); };
    actions_["MapSelectedTrackSendsToWidgets"] =    [](string name, Widget* widget, Zone* zone, vector<string> params) { return new MapSelectedTrackSendsToWidgets(name, widget, zone, params); };
    actions_["ToggleMapSelectedTrackFX"] =          [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ToggleMapSelectedTrackFX(name, widget, zone, params); };
    actions_["MapSelectedTrackFXToWidgets"] =       [](string name, Widget* widget, Zone* zone, vector<string> params) { return new MapSelectedTrackFXToWidgets(name, widget, zone, params); };
    actions_["ToggleMapSelectedTrackFXMenu"] =      [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ToggleMapSelectedTrackFXMenu(name, widget, zone, params); };
    actions_["MapSelectedTrackFXToMenu"] =          [](string name, Widget* widget, Zone* zone, vector<string> params) { return new MapSelectedTrackFXToMenu(name, widget, zone, params); };
    actions_["ToggleMapFocusedFX"] =                [](string name, Widget* widget, Zone* zone, vector<string> params) { return new ToggleMapFocusedFX(name, widget, zone, params); };
    actions_["MapFocusedFXToWidgets"] =             [](string name, Widget* widget, Zone* zone, vector<string> params) { return new MapFocusedFXToWidgets(name, widget, zone, params); };
    actions_["GoFXSlot"] =                          [](string name, Widget* widget, Zone* zone, vector<string> params) { return new GoFXSlot(name, widget, zone, params); };
    actions_["TrackAutoMode"] =                     [](string name, Widget* widget, Zone* zone, vector<string> params) { return new TrackAutoMode(name, widget, zone, params); };
    actions_["GlobalAutoMode"] =                    [](string name, Widget* widget, Zone* zone, vector<string> params) { return new GlobalAutoMode(name, widget, zone, params); };
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
            
            if(tokens.size() > 8) // ignore comment lines and blank lines
            {
                if(tokens[0] == PageToken)
                {
                    if(tokens.size() != 11)
                        continue;
                    
                    rgb_color pageColour;
                    
                    if(tokens[6] == "{" && tokens[10] == "}")
                    {
                        pageColour.r = atoi(tokens[7].c_str());
                        pageColour.g = atoi(tokens[8].c_str());
                        pageColour.b = atoi(tokens[9].c_str());
                    }

                    currentPage = new Page(tokens[1], pageColour, tokens[2] == "FollowMCP" ? true : false, tokens[3] == "SynchPages" ? true : false);
                    pages_.push_back(currentPage);
                    
                    if(tokens[4] == "UseScrollLink")
                        currentPage->GetTrackNavigationManager()->SetScrollLink(true);
                    else
                        currentPage->GetTrackNavigationManager()->SetScrollLink(false);
                }
                else if(tokens[0] == MidiSurfaceToken || tokens[0] == OSCSurfaceToken || tokens[0] == EuConSurfaceToken)
                {
                    int inPort = atoi(tokens[2].c_str());
                    int outPort = atoi(tokens[3].c_str());
                    
                    if(currentPage)
                    {
                        ControlSurface* surface = nullptr;
                        
                        if(tokens[0] == MidiSurfaceToken && tokens.size() == 11)
                            surface = new Midi_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForPort(inPort), GetMidiOutputForPort(outPort));
                        else if(tokens[0] == OSCSurfaceToken && tokens.size() == 12)
                            surface = new OSC_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], inPort, outPort, tokens[11]);
                        else if(tokens[0] == EuConSurfaceToken && tokens.size() == 9)
                            surface = new EuCon_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4],
                                                               atoi(tokens[2].c_str()), // firstChannel
                                                               atoi(tokens[3].c_str()), // last Channel
                                                               atoi(tokens[6].c_str()), // numSends
                                                               atoi(tokens[7].c_str()), // numFX
                                                               atoi(tokens[8].c_str())  // options
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
                                surface->GetSendsActivationManager()->SetShouldMapSends(true);
                            
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
        snprintf(buffer, sizeof(buffer), "Trouble in %s, around line %d\n", iniFilePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////
// Parsing end
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TrackNavigator::GetIsZoneTouched()
{
    return zone_->GetIsTouched();
}

void TrackNavigator::Pin()
{
    if( ! isChannelPinned_)
    {
        pinnedTrack_ = GetTrack();
        
        isChannelPinned_ = true;
        
        manager_->IncChannelBias(pinnedTrack_, channelNum_);
    }
}

void TrackNavigator::Unpin()
{
    if(isChannelPinned_)
    {
        manager_->DecChannelBias(pinnedTrack_, channelNum_);
        
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
bool MasterTrackNavigator::GetIsZoneTouched()
{
    return zone_->GetIsTouched();
}

MediaTrack* MasterTrackNavigator::GetTrack()
{
    return DAW::GetMasterTrack(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectedTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* SelectedTrackNavigator::GetTrack()
{
    return zone_->GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack();
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
            return zone_->GetSurface()->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNumber);
        else
            return nullptr;
    }
    else
        return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
Navigator* TrackNavigationManager::AddNavigator(Zone* zone)
{
    int channelNum = navigators_.size();
    navigators_.push_back(new TrackNavigator(zone, this, channelNum));
    return navigators_[channelNum];
}

void TrackNavigationManager::OnTrackSelection()
{
    if(scrollLink_)
    {
        // Make sure selected track is visble on the control surface
        MediaTrack* selectedTrack = GetSelectedTrack();
        
        if(selectedTrack != nullptr)
        {
            for(auto navigator : navigators_)
                if(selectedTrack == navigator->GetTrack())
                    return;
            
            for(int i = 0; i < tracks_.size(); i++)
                if(selectedTrack == tracks_[i])
                    trackOffset_ = i;
            
            trackOffset_ -= targetScrollLinkChannel_;
            
            if(trackOffset_ <  0)
                trackOffset_ =  0;
            
            int top = GetNumTracks() - navigators_.size();
            
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
    
    if(numTracks <= navigators_.size())
        return;
    
    trackOffset_ += amount;
    
    if(trackOffset_ <  0)
        trackOffset_ =  0;
    
    int top = numTracks - navigators_.size();
    
    if(trackOffset_ >  top)
        trackOffset_ = top;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Widget::AddAction(Zone* zone, string modifiers, Action* action)
{
    actions_[zone][modifiers].push_back(action);
    zonesAvailable_[zone->GetName()] = zone;
}

void Widget::AddTrackTouchedAction(Zone* zone, string modifiers, Action* action)
{
    trackTouchedActions_[zone][modifiers].push_back(action);
}

MediaTrack* Widget::GetTrack()
{
    if(activeZone_ != nullptr)
        return activeZone_->GetNavigator()->GetTrack();
    else
        return nullptr;
}

void Widget::RequestUpdate()
{
    string modifiers = "";
    
    if( ! isModifier_ )
        modifiers = surface_->GetPage()->GetModifiers();
    
    if(activeZone_ != nullptr && activeZone_->GetIsTouched() && trackTouchedActions_.count(activeZone_) > 0 && trackTouchedActions_[activeZone_].count(modifiers) > 0 && trackTouchedActions_[activeZone_][modifiers].size() > 0)
            trackTouchedActions_[activeZone_][modifiers][0]->RequestUpdate();
    else if(actions_.count(activeZone_) > 0 && actions_[activeZone_].count(modifiers) > 0 && actions_[activeZone_][modifiers].size() > 0)
            actions_[activeZone_][modifiers][0]->RequestUpdate();
}

void Widget::DoAction(double value)
{
    if( TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN -> %s -> %s %f\n", GetSurface()->GetName().c_str(), GetName().c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }

    string modifiers = "";
    
    if( ! isModifier_)
    {
        modifiers = surface_->GetPage()->GetModifiers();
        
        // GAW TBD LearnMode
        // GetSurface()->GetPage()->InputReceived(this, value);
    }

    if(actions_.count(activeZone_) > 0 && actions_[activeZone_].count(modifiers) > 0)
        for(auto action : actions_[activeZone_][modifiers])
            action->DoAction(value, this);
    else if(modifiers != "" && actions_.count(activeZone_) > 0 && actions_[activeZone_].count("") > 0)
        for(auto action : actions_[activeZone_][""])
            action->DoAction(value, this);
}

void Widget::DoRelativeAction(double value)
{
    DoAction(lastValue_ + value);
}

void Widget::SetIsTouched(bool isZoneTouched)
{
    if(activeZone_ != nullptr)
        activeZone_->SetIsTouched(isZoneTouched);
}

void  Widget::SetValue(double value)
{
    lastValue_ = value;
    feedbackProcessor_->SetValue(value);
}

void  Widget::SetValue(int mode, double value)
{
    lastValue_ = value;
    feedbackProcessor_->SetValue(mode, value);
}

void  Widget::SetValue(string value)
{
    feedbackProcessor_->SetValue(value);
}

void  Widget::SetRGBValue(int r, int g, int b)
{
    feedbackProcessor_->SetRGBValue(r, g, b);
}

void Widget::ClearCache()
{
    feedbackProcessor_->ClearCache();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Page* Action::GetPage()
{
    return widget_->GetSurface()->GetPage();
}

ControlSurface* Action::GetSurface()
{
    return widget_->GetSurface();
}

int Action::GetSlotIndex()
{
    return zone_->GetIndex();
}

void Action::DoAction(double value, Widget* sender)
{
    value = isInverted_ == false ? value : 1.0 - value;
    
    if(shouldToggle_)
    {
        if(value != 0.0)
            Do( ! GetCurrentValue(), sender);
    }
    else
        Do(value, sender);
    
    // GAW TBD -- this will need to be changed
    /*
     if( ! GetWidget()->GetIsModifier())
     GetPage()->ActionPerformed(GetWidgetActionManager(), this);
     */
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Zone::Zone(string navigatorName, ControlSurface* surface, Zone* parentZone, int channelNum, string name, string sourceFilePath, string alias) : surface_(surface),  name_(name), sourceFilePath_(sourceFilePath), alias_(alias)
{
    // The Zone and Navigator MUST be coupled
    if(navigatorName== "TrackNavigator")
        navigator_ = GetNavigatorForChannel(this, surface, channelNum);
    else if(navigatorName == "MasterTrackNavigator")
        navigator_ = new MasterTrackNavigator(this);
    else if(navigatorName == "SelectedTrackNavigator")
        navigator_ = new SelectedTrackNavigator(this);
    else if(navigatorName == "FocusedFXNavigator")
        navigator_ = new FocusedFXNavigator(this);
    else if(navigatorName == "ParentNavigator" && parentZone != nullptr)
        navigator_ = parentZone->GetNavigator();
    else
        navigator_ = new Navigator(this);
}

void Zone::Activate()
{
    surface_->WidgetsGoZone(GetName());
    
    for(auto includedZone : includedZones_)
        surface_->WidgetsGoZone(includedZone->GetName());
}

void Zone::Deactivate()
{
    surface_->GoHome();
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
        surface_->SendEuConMessage(address_, value);
    }
}

void EuCon_FeedbackProcessor::SetValue(int param, double value)
{
    // GAW TBD -- if needed must implement on EuCon side
    
    if(lastDoubleValue_ != value)
    {
        lastDoubleValue_ = value;
        //surface_->SendEuConMessage(address_, param, value);
    }
}

void EuCon_FeedbackProcessor::SetValue(string value)
{
    if(lastStringValue_ != value)
    {
        lastStringValue_ = value;
        surface_->SendEuConMessage(address_, value);
    }
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
                surface_->ActivateNoActionForZone(zone->GetName());
                activeSendZones_.push_back(zone);
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
                surface_->ActivateNoActionForZone(zone->GetName());
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
            focusedTrack = surface_->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNumber);
    
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
    try
    {
        vector<string> zoneFilesToProcess;
        listZoneFiles(DAW::GetResourcePath() + string("/CSI/Zones/") + zoneFolder + "/", zoneFilesToProcess); // recursively find all the .zon files, starting at zoneFolder
        
        navigators.clear();
        
        for(auto zoneFilename : zoneFilesToProcess)
            ProcessZoneFile(zoneFilename, this, widgets_);
        
        Zone * zone = new Zone("", this, nullptr, 0, "NoAction", "", "NoAction");
        
        vector<string> params;
        
        for(auto widget : widgets_)
            widget->AddAction(zone, "", new NoAction("NoAction", widget, zone, params));
    }
    catch (exception &e)
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble parsing Zone folders\n");
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename, this, widgets_);
    InitHardwiredWidgets();
}

void Midi_ControlSurface::ProcessMidiMessage(const MIDI_event_ex_t* evt)
{
    bool isMapped = false;
    
    // At this point we don't know how much of the message comprises the key, so try all three
    if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]) > 0)
    {
        isMapped = true;
        for( auto generator : CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]])
            generator->ProcessMidiMessage(evt);
    }
    else if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100) > 0)
    {
        isMapped = true;
        for( auto generator : CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100])
            generator->ProcessMidiMessage(evt);
    }
    else if(CSIMessageGeneratorsByMidiMessage_.count(evt->midi_message[0] * 0x10000) > 0)
    {
        isMapped = true;
        for( auto generator : CSIMessageGeneratorsByMidiMessage_[evt->midi_message[0] * 0x10000])
            generator->ProcessMidiMessage(evt);
        
    }
    
    if( ! isMapped && TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN -> %s %02x  %02x  %02x \n", name_.c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
        DAW::ShowConsoleMsg(buffer);
        
    }
}

void Midi_ControlSurface::SendMidiMessage(MIDI_event_ex_t* midiMessage)
{
    if(midiOutput_)
        midiOutput_->SendMsg(midiMessage, -1);
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "OUT -> %s SysEx \n", name_.c_str());
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
        snprintf(buffer, sizeof(buffer), "OUT -> %s %02x  %02x  %02x \n", name_.c_str(), first, second, third);
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSC_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/OSC/" + templateFilename, this, widgets_);
    InitHardwiredWidgets();
}

void OSC_ControlSurface::ProcessOSCMessage(string message, double value)
{
    if(CSIMessageGeneratorsByOSCMessage_.count(message) > 0)
        CSIMessageGeneratorsByOSCMessage_[message]->ProcessOSCMessage(message, value);
    
    if(TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN -> %s %s  %f  \n", name_.c_str(), message.c_str(), value);
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
        snprintf(buffer, sizeof(buffer), "OUT -> %s %s \n", name_.c_str(), oscAddress.c_str());
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
        snprintf(buffer, sizeof(buffer), "OUT -> %s %s  %f  \n", name_.c_str(), oscAddress.c_str(), value);
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
        snprintf(buffer, sizeof(buffer), "OUT -> %s %s  %s  \n", name_.c_str(), oscAddress.c_str(), value.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// For EuCon
/////////////////////////////////////////////////////////////////////////////
class MarshalledFunctionCall
/////////////////////////////////////////////////////////////////////////////
{
protected:
    EuCon_ControlSurface* surface_ = nullptr;
    MarshalledFunctionCall(EuCon_ControlSurface * surface) : surface_(surface) {}
    
public:
    virtual void Execute() {}
    virtual ~MarshalledFunctionCall() {}
};

/////////////////////////////////////////////////////////////////////////////
class Marshalled_Double : public MarshalledFunctionCall
/////////////////////////////////////////////////////////////////////////////
{
private:
    std::string address_ = "";
    double value_ = 0;
public:
    Marshalled_Double(EuCon_ControlSurface* surface, std::string address, double value) : MarshalledFunctionCall(surface), address_(address), value_(value)  { }
    virtual ~Marshalled_Double() {}
    
    virtual void Execute() override { surface_->HandleEuConMessage(address_, value_); }
};

/////////////////////////////////////////////////////////////////////////////
class Marshalled_String : public MarshalledFunctionCall
/////////////////////////////////////////////////////////////////////////////
{
private:
    std::string address_ = "";
    std::string  value_ = "";
    
public:
    Marshalled_String(EuCon_ControlSurface* surface, std::string address, string value) : MarshalledFunctionCall(surface), address_(address), value_(value)  { }
    virtual ~Marshalled_String() {}
    
    virtual void Execute() override { surface_->HandleEuConMessage(address_, value_); }
};

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

void HandleEuConMessageWithDouble(const char *address, double value)
{
    if(TheManager)
        TheManager->ReceiveEuConMessage(string(address), value);
}

void HandleEuConMessageWithString(const char *address, const char *value)
{
    if(TheManager)
        TheManager->ReceiveEuConMessage(string(address), string(value));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
EuCon_ControlSurface::EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int lowChannel, int highChannel, int numSends, int numFX, int options)
: ControlSurface(CSurfIntegrator, page, name), lowChannel_(lowChannel), highChannel_(highChannel), numSends_(numSends), numFX_(numFX), options_(options)
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
            InitializeEuConWithParameters(lowChannel_, highChannel_, numSends_, numFX_, 0, 0, options_);
    }
}

void EuCon_ControlSurface::InitializeEuConWidget(string name, string control, string FB_Processor)
{
    if(name != "")
    {
        Widget* widget = nullptr;
        
        if(FB_Processor != "")
            widget = new Widget(this, name, new EuCon_FeedbackProcessor(this, FB_Processor));
        else
            widget = new Widget(this, name, new FeedbackProcessor());

        if(widget)
        {
            widgets_.push_back(widget);
            if(control != "")
                new EuCon_CSIMessageGenerator(this, widget, control);
        }
    }
}

void EuCon_ControlSurface::EuConInitializationComplete()
{
    InitHardwiredWidgets();
    InitZones(zoneFolder_);
    GoHome();
    for(auto widget : widgets_)
        widget->Reset();
}

void EuCon_ControlSurface::SendEuConMessage(string address, double value)
{
    if(g_reaper_plugin_info)
    {
        void (*HandleReaperMessageWthDouble)(const char *, double);
        
        HandleReaperMessageWthDouble = (void (*)(const char *, double))g_reaper_plugin_info->GetFunc("HandleReaperMessageWthDouble");
        
        if(HandleReaperMessageWthDouble)
            HandleReaperMessageWthDouble(address.c_str(), value);
    }
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "OUT -> %s %s  %f  \n", name_.c_str(), address.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::SendEuConMessage(string address, string value)
{
    if(g_reaper_plugin_info)
    {
        void (*HandleReaperMessageWthString)(const char *, const char *);
        
        HandleReaperMessageWthString = (void (*)(const char *, const char *))g_reaper_plugin_info->GetFunc("HandleReaperMessageWithString");
        
        if(HandleReaperMessageWthString)
            HandleReaperMessageWthString(address.c_str(), value.c_str());
    }

    if(TheManager->GetSurfaceOutMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "OUT -> %s %s  %s  \n", name_.c_str(), address.c_str(), value.c_str());
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::ReceiveEuConMessage(string address, double value)
{
    mutex_.Enter();
    workQueue_.push_front(new Marshalled_Double(this, address, value));
    mutex_.Leave();
}

void EuCon_ControlSurface::ReceiveEuConMessage(string address, string value)
{
    mutex_.Enter();
    workQueue_.push_front(new Marshalled_String(this, address, value));
    mutex_.Leave();
}

void EuCon_ControlSurface::HandleExternalInput()
{
    if(! workQueue_.empty())
    {
        mutex_.Enter();
        list<MarshalledFunctionCall*> localWorkQueue = workQueue_;
        workQueue_.clear();
        mutex_.Leave();
        
        while(! localWorkQueue.empty())
        {
            MarshalledFunctionCall *pCall = localWorkQueue.back();
            localWorkQueue.pop_back();
            pCall->Execute();
            delete pCall;
        }
    }
}

void EuCon_ControlSurface::HandleEuConMessage(string address, double value)
{
    if(address == "PostMessage" && g_hwnd != nullptr)
        PostMessage(g_hwnd, WM_COMMAND, (int)value, 0);
    else if(address == "LayoutChanged")
        DAW::MarkProjectDirty(nullptr);
    else if(CSIMessageGeneratorsByMessage_.count(address) > 0)
        CSIMessageGeneratorsByMessage_[address]->ProcessMessage(address, value);
        
    if(TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN -> %s %s  %f  \n", name_.c_str(), address.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::HandleEuConMessage(string oscAddress, string value)
{
    // GAW TBD
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Learn Mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ActionLineItem
{
    string navigator = "";
    string widgetName = "";
    Widget* widget = nullptr;
    string modifiers = "";
    string allModifiers = "";
    string actionName = "";
    Action* action = nullptr;
    string param = "";
    string alias = "";
    bool isShift = false;
    bool isOption = false;
    bool isControl = false;
    bool isAlt = false;
    bool isToggle = false;
    bool isInvert = false;
    bool isTouch = false;
    bool isHold = false;
    bool supportsRGB = false;
    vector<rgb_color> colors;
};

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

            for(int j = 0; j < zonesInThisFile[zoneIndex]->GetIncludedZones().size(); j++) // GAW TBD -- change to file based
            {
                if(zonesInThisFile[zoneIndex]->GetIncludedZones()[j]->GetName() == zonesInThisFile[i]->GetName()) // GAW TBD -- change to file based
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
        snprintf(buffer, sizeof(buffer), "Trouble loading Raw FX file %s", filePath.c_str());
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
    //string navigatorName = currentWidgetActionManager->GetNavigatorName();
    
    //if(navigatorName == "")
       string navigatorName = "NoNavigator";
    
    SetWindowText(GetDlgItem(hwndLearn, IDC_STATIC_Navigator), navigatorName.c_str());
    
    // Included Zones
    for(auto includedZone : zone->GetIncludedZones()) // GAW TBD -- change to file based
        SendDlgItemMessage(hwndLearn, IDC_LIST_IncludedZones, LB_ADDSTRING, 0, (LPARAM)includedZone->GetName().c_str());
    
    // Zone line Items
    bool hasLoadedRawFXFile = false;
    
    /* vector<ActionLineItem> actionLineItems = zone->GetActionLineItems();
    
    for(int i = 0; i < actionLineItems.size(); i++)
    {
        ActionLineItem actionLineItem = actionLineItems[i];
        
        if (actionLineItem.actionName.find(FXParam) != string::npos && hasLoadedRawFXFile == false)
            hasLoadedRawFXFile = LoadRawFXFile(currentWidget->GetTrack(), zone->GetName());
        
        if(hasLoadedRawFXFile && actionLineItem.actionName.find(FXParam) != string::npos && actionLineItem.param == currentAction->GetParamNumAsString())
        {
            actionNameWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_SETCURSEL, currentAction->GetParamNum(), 0);
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
    */
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
                    
                    focusedFXTrack = currentSurface->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNumber);
                
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
            Zone* zone = nullptr;
            
            int zoneIndex = FillZones(zone);
            
            FillSubZones(zone, zoneIndex);
            
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, currentAction->GetName().c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT_ActionParameter, currentAction->GetParamNumAsString().c_str());
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
                            //zonesInThisFile[zoneIndex]->SetAlias(string(buffer));
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
                                        
                                        //if(zone->GetNavigatorName() != "")
                                            //zonFile << zone->GetNavigatorName() + GetLineEnding();
                                        
                                        if(zone->GetIncludedZones().size() > 0) // GAW TBD -- change to file based
                                        {
                                            zonFile << "IncludedZones" + GetLineEnding();

                                            for(auto includedZone : zone->GetIncludedZones())     // GAW TBD -- change to file based
                                                zonFile << includedZone->GetName() + GetLineEnding();
                                            
                                            zonFile << "IncludedZonesEnd" + GetLineEnding();
                                        }
                                        
                                        /*for(auto actionLineItem : zone->GetActionLineItems())
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
                                        */
                                        zonFile << "ZoneEnd" + GetLineEnding() + GetLineEnding();
                                    }
                                }
                                
                                zonFile.close();
                                hasEdits = false;
                            }
                            catch (exception &e)
                            {
                                char buffer[250];
                                snprintf(buffer, sizeof(buffer), "Trouble writing %s", filePath.c_str());
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
                            
                            // GAW this will need to be changed

                            //Zone* newZone = new Zone(currentSurface, newZoneName, newZoneFilename, newZoneAlias);
                            
                            /*
                            if(trackNavigatorName == "SelectedTrackNavigator")
                                newZone->SetTrackNavigator(new SelectedTrackNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));
                            else if(trackNavigatorName == "FocusedFXNavigator")
                                newZone->SetTrackNavigator(new FocusedFXNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));
                             */
                            
                            SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_Navigator), trackNavigatorName.c_str());
                            
                            //currentSurface->AddZone(newZone);
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
                                
                                // GAW this will need to be changed

                               // Zone* newZone = new Zone(currentSurface, newZoneName, zoneFilename, newZoneAlias);
                                
                                /*

                                if(trackNavigatorName == "SelectedTrackNavigator")
                                    newZone->SetTrackNavigator(new SelectedTrackNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));
                                else if(trackNavigatorName == "FocusedFXNavigator")
                                    newZone->SetTrackNavigator(new FocusedFXNavigator(currentSurface->GetPage()->GetTrackNavigationManager()));
                                */
                                
                                SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_Navigator), trackNavigatorName.c_str());

                                //currentSurface->AddZone(newZone);
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
                            
                            //zonesInThisFile[zoneIndex]->AddAction(actionLineItem, currentFXIndex);
                            //zonesInThisFile[zoneIndex]->Activate(currentFXIndex);
                            
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
                            //currentSurface->RemoveZone(zoneToDelete, index);
                            
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
                                //zone->RemoveZone(index);
                                
                                SendMessage(GetDlgItem(hwndLearn, IDC_LIST_IncludedZones), LB_RESETCONTENT, 0, 0);
                                for(auto includedZone : zone->GetIncludedZones())     // GAW TBD -- change to file based
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
                           
                                //ActionLineItem actionLineItem = zonesInThisFile[zoneIndex]->GetActionLineItems()[index];

                                //zonesInThisFile[zoneIndex]->RemoveAction(actionLineItem);
                                
                                SendDlgItemMessage(hwndDlg, IDC_LIST_ZoneComponents, LB_DELETESTRING, index, 0);
                            }
                        }
                    }
                    break ;
                }
                    
                case IDC_BUTTON_GenerateWidgetList:
                    if (HIWORD(wParam) == BN_CLICKED && currentSurface != nullptr)
                    {
                        char buffer[1024];
                        snprintf(buffer, sizeof(buffer), "Surface Name: %s", (currentSurface->GetName() + GetLineEnding() + GetLineEnding()).c_str());
                        DAW::ShowConsoleMsg(buffer);
                        
                        for(auto widget : currentSurface->GetWidgets())
                        {
                            snprintf(buffer, sizeof(buffer), "%s", (widget->GetName() + GetLineEnding()).c_str());
                            DAW::ShowConsoleMsg(buffer);
                        }
                        
                        snprintf(buffer, sizeof(buffer), "%s", (GetLineEnding() + GetLineEnding() + GetLineEnding() + GetLineEnding()).c_str());
                        DAW::ShowConsoleMsg(buffer);
                    }
                    break ;
                    
                case IDC_BUTTON_GenerateActionList:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        char buffer[1024];
                        snprintf(buffer, sizeof(buffer), "CSI Actions %s",  (GetLineEnding() + GetLineEnding()).c_str() );
                        DAW::ShowConsoleMsg(buffer);
                        
                        for(auto name : TheManager->GetActionNames())
                        {
                            snprintf(buffer, sizeof(buffer), "%s", (name + GetLineEnding()).c_str());
                            DAW::ShowConsoleMsg(buffer);
                        }
                        
                        snprintf(buffer, sizeof(buffer), "%s", (GetLineEnding() + GetLineEnding() + GetLineEnding() + GetLineEnding()).c_str());
                        DAW::ShowConsoleMsg(buffer);
                    }
                    break ;
                    
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
                            //if(index >= 0)
                                //currentSurface->GetWidgets()[index]->DoAction(currentSurface->GetWidgets()[index]->GetLastValue());

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
                                    /*
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

void Page::ActionPerformed(Action* action)
{
    currentPage = this;

    if(hwndLearn == nullptr)
        return;
    
    currentAction = action;
    
    isShift = isShift_;
    isOption = isOption_;
    isControl = isControl_;
    isAlt = isAlt_;
   
    //if(currentWidget != nullptr && currentWidgetActionManager != nullptr && currentAction != nullptr)
        //SendMessage(hwndLearn, WM_USER+1025, 0, 0);
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
