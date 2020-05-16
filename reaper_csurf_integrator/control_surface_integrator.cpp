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


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC I/O Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static map<string, oscpkt::UdpSocket*> inputSockets_;
static map<string, oscpkt::UdpSocket*> outputSockets_;

static oscpkt::UdpSocket* GetInputSocketForPort(string surfaceName, int inputPort)
{
    if(inputSockets_.count(surfaceName) > 0)
        return inputSockets_[surfaceName]; // return existing
    
    // otherwise make new
    oscpkt::UdpSocket* newInputSocket = new oscpkt::UdpSocket();
    
    if(newInputSocket)
    {
        newInputSocket->bindTo(inputPort);
        
        if (! newInputSocket->isOk())
        {
            //cerr << "Error opening port " << PORT_NUM << ": " << inSocket_.errorMessage() << "\n";
            return nullptr;
        }
        
        inputSockets_[surfaceName] = newInputSocket;
        
        return inputSockets_[surfaceName];
    }
    
    return nullptr;
}

static oscpkt::UdpSocket* GetOutputSocketForAddressAndPort(string surfaceName, string address, int outputPort)
{
    if(outputSockets_.count(surfaceName) > 0)
        return outputSockets_[surfaceName]; // return existing
    
    // otherwise make new
    oscpkt::UdpSocket* newOutputSocket = new oscpkt::UdpSocket();
    
    if(newOutputSocket)
    {
        if( ! newOutputSocket->connectTo(address, outputPort))
        {
            //cerr << "Error connecting " << remoteDeviceIP_ << ": " << outSocket_.errorMessage() << "\n";
            return nullptr;
        }
        
        newOutputSocket->bindTo(outputPort);
        
        if ( ! newOutputSocket->isOk())
        {
            //cerr << "Error opening port " << outPort_ << ": " << outSocket_.errorMessage() << "\n";
            return nullptr;
        }

        outputSockets_[surfaceName] = newOutputSocket;
        
        return outputSockets_[surfaceName];
    }
    
    return nullptr;
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

static void GetWidgetNameAndModifiers(string line, string &widgetName, string &modifiers, bool &isPressRelease, bool &isTrackTouch, bool &isTrackRotaryTouch, bool &isInverted, bool &shouldToggle, double &delayAmount)
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
            
            else if(modifier_tokens[i] == "PR")
                isPressRelease = true;
            else if(modifier_tokens[i] == "TrackTouch")
                isTrackTouch = true;
            else if(modifier_tokens[i] == "RotaryTouch")
                isTrackRotaryTouch = true;
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

static Navigator* GetNavigatorForChannel(ControlSurface* surface, int channelNum)
{
    if(channelNum < 0)
        return nullptr;
    
    if(navigators.count(channelNum) < 1)
        navigators[channelNum] = surface->GetNavigatorForChannel(channelNum);
    
    return navigators[channelNum];
}

static void BuildIncludedZoneOld(string includedZoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, ZoneOld* parentZone);

static map<string, vector<vector<string>>> zoneTemplates;
static map<string, vector<vector<string>>> zoneDefinitions;

static void BuildZoneOld(vector<vector<string>> &zoneLines, string filePath, ControlSurface* surface, vector<Widget*> &widgets, ZoneOld* parentZone, int channelNum)
{
    const string FXGainReductionMeter = "FXGainReductionMeter"; // GAW TBD - don't forget to re-implement this

    ZoneOld* zone = nullptr;
    vector<string> includedZones;
    bool isInIncludedZonesSection = false;
    
    for(int i = 0 ; i < zoneLines.size(); i++)
    {
        auto tokens = zoneLines[i];
        
        if(tokens.size() > 0 && tokens[0] == "Zone")
        {
            if(tokens.size() < 2)
                return;
            
            Navigator* navigator = nullptr;

            if(zoneLines.size() > i + 1) // we have another line
            {
                auto navTokens = zoneLines[i + 1];
            
                if(navTokens.size() == 1 && (navTokens[0] == "TrackNavigator" || navTokens[0] == "MasterTrackNavigator" || navTokens[0] == "SelectedTrackNavigator"
                                             || navTokens[0] == "FocusedFXNavigator" || navTokens[0] == "ParentNavigator"))
                {
                    i++;
                    
                    if(navTokens[0] == "TrackNavigator")
                        navigator = GetNavigatorForChannel(surface, channelNum);
                    else if(navTokens[0] == "MasterTrackNavigator")
                        navigator = surface->GetPage()->GetTrackNavigationManager()->GetMasterTrackNavigator();
                    else if(navTokens[0] == "SelectedTrackNavigator")
                        navigator = surface->GetPage()->GetTrackNavigationManager()->GetSelectedTrackNavigator();
                    else if(navTokens[0] == "FocusedFXNavigator")
                        navigator = surface->GetPage()->GetTrackNavigationManager()->GetFocusedFXNavigator();
                    else if(navTokens[0] == "ParentNavigator" && parentZone != nullptr)
                        navigator = parentZone->GetNavigator();
                }
            }
            
            if(navigator == nullptr)
                navigator = new Navigator(surface->GetPage());
            
            zone = new ZoneOld(navigator, surface, tokens[1], filePath, tokens.size() > 2 ? tokens[2] : tokens[1]); // tokens[2] == alias, if provided, otherwise just use name (tokens[1])

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
                BuildIncludedZoneOld(tokens[0], filePath, surface, widgets, zone);
                continue;
            }
        }
    
        // GAW -- the first token is the Widget name, possibly decorated with modifiers
        string widgetName = "";
        string modifiers = "";
        bool isPressRelease = false;
        bool isTrackTouch = false;
        bool isTrackRotaryTouch = false;
        bool isInverted = false;
        bool shouldToggle = false;
        bool isDelayed = false;
        double delayAmount = 0.0;
    
        GetWidgetNameAndModifiers(tokens[0], widgetName, modifiers, isPressRelease, isTrackTouch, isTrackRotaryTouch, isInverted, shouldToggle, delayAmount);
    
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
        for(int j = 2; j < tokens.size(); j++)
            params.push_back(tokens[j]);
    
        if(widget != nullptr)
        {
            zone->AddWidget(widget);
            
            if(TheManager->IsActionAvailable(tokens[1]))
            {
                Action* action = TheManager->GetActionOld(widget, zone, tokens[1],  params);
                
                if(action != nullptr)
                {
                    if(isTrackTouch)
                        widget->AddTrackTouchedAction(zone, modifiers, action);
                    else if(isTrackRotaryTouch)
                        widget->AddTrackRotaryTouchedAction(zone, modifiers, action);
                    else
                        widget->AddAction(zone, modifiers, action);
                    
                    if(isPressRelease)
                    {
                        // GAS TBD -- action->SetIsPressRelease()
                    }
                    
                    if(isInverted)
                        action->SetIsInverted();
                    
                    if(shouldToggle)
                        action->SetShouldToggle();
                    
                    if(isDelayed)
                        action->SetDelayAmount(delayAmount * 1000.0);

                    if(tokens[1] == Shift || tokens[1] == Option || tokens[1] == Control || tokens[1] == Alt)
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

static void BuildExpandedZonesOld(string zoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, ZoneOld* parentZone)
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
                        
                        BuildZoneOld(zoneLines, filePath, surface, widgets, parentZone, i);
                    }
                }
        }
    }
}

static void BuildIncludedZoneOld(string includedZoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, ZoneOld* parentZone)
{
    if(includedZoneName.back() == '|')
    {
        if(zoneTemplates.count(includedZoneName) > 0)
            BuildZoneOld(zoneTemplates[includedZoneName], filePath, surface, widgets, parentZone, -1); // track ptr == nullptr
    }
    else if(regex_search(includedZoneName, regex("[|][0-9]+[-][0-9]+"))) // This expands constructs like Channel|1-8 into multiple Zones
        BuildExpandedZonesOld(includedZoneName, filePath, surface, widgets, parentZone);
}

static void ProcessZoneFileOld(string filePath, ControlSurface* surface, vector<Widget*> &widgets)
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
            surface->AddZoneFileLine(filePath, line);  // store in the raw map for EditMode
            
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
            BuildExpandedZonesOld(zoneName, filePath, surface, widgets, nullptr);
        else
            BuildZoneOld(zoneLines, filePath, surface, widgets, nullptr, -1); // Start with the outermost Zones -- parentZone == nullptr, track ptr == nullptr
    }
}









static void ProcessZoneFile(string filePath, ControlSurface* surface)
{
    vector<string> companionZones;
    bool isInCompanionZonesSection = false;

    vector<string> includedZones;
    bool isInIncludedZonesSection = false;
    
    vector<Wzat> zoneMembers;

    string zoneName = "";
    string zoneAlias = "";
    string navigatorName = "";
    string actionName = "";
    int lineNumber = 0;
    
    try
    {
        ifstream file(filePath);
        
        for (string line; getline(file, line) ; )
        {
            surface->AddZoneFileLine(filePath, line);  // store in the raw map for EditMode
            
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
                    zoneAlias = tokens.size() > 2 ? tokens[2] : tokens[1];
                }
                
                else if(tokens[0] == "ZoneEnd")
                {
                    surface->AddZoneTemplate(new ZoneTemplate(navigatorName, zoneName, zoneAlias, filePath, companionZones, includedZones, zoneMembers));
                    companionZones.clear();
                    includedZones.clear();
                    zoneMembers.clear();
                }
                
                else if(tokens[0] == "TrackNavigator" || tokens[0] == "MasterTrackNavigator" || tokens[0] == "SelectedTrackNavigator" || tokens[0] == "FocusedFXNavigator" || tokens[0] == "ParentNavigator")
                    navigatorName = tokens[0];
                
                else if(tokens[0] == "IncludedZones")
                    isInIncludedZonesSection = true;
                
                else if(tokens[0] == "IncludedZonesEnd")
                    isInIncludedZonesSection = false;
                                
                else if(tokens.size() == 1 && isInIncludedZonesSection)
                    includedZones.push_back(tokens[0]);
                
                else if(tokens[0] == "CompanionZones")
                    isInCompanionZonesSection = true;
                
                else if(tokens[0] == "CompanionZonesEnd")
                    isInCompanionZonesSection = false;
                
                else if(tokens.size() == 1 && isInCompanionZonesSection)
                    companionZones.push_back(tokens[0]);
                else
                {
                    actionName = tokens[1];

                    // GAW -- the first token is the Widget name, possibly decorated with modifiers
                    string widgetName = "";
                    string modifiers = "";
                    bool isPressRelease = false;
                    bool isTrackTouch = false;
                    bool isTrackRotaryTouch = false;
                    bool isInverted = false;
                    bool shouldToggle = false;
                    bool isDelayed = false;
                    double delayAmount = 0.0;
                    
                    GetWidgetNameAndModifiers(tokens[0], widgetName, modifiers, isPressRelease, isTrackTouch, isTrackRotaryTouch, isInverted, shouldToggle, delayAmount);
                    
                    if(delayAmount > 0.0)
                        isDelayed = true;
                    
                    bool isModifier = (actionName == Shift || actionName == Option || actionName == Control || actionName == Alt);
                    
                    vector<string> params;
                    for(int j = 2; j < tokens.size(); j++)
                        params.push_back(tokens[j]);
                   
                    zoneMembers.push_back(Wzat(widgetName, actionName, params, modifiers, isModifier, isPressRelease, isTrackTouch, isTrackRotaryTouch, isInverted, shouldToggle, isDelayed, delayAmount));
                }
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

void SetRGB(vector<string> params, bool &supportsRGB, bool &supportsTrackColor, vector<rgb_color> &RGBValues)
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
                supportsTrackColor = true;
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
            supportsRGB = true;
            
            for(int i = 0; i < rawValues.size(); i += 3)
            {
                rgb_color color;
                
                color.r = rawValues[i];
                color.g = rawValues[i + 1];
                color.b = rawValues[i + 2];
                
                RGBValues.push_back(color);
            }
        }
    }
}

void SetSteppedValues(vector<string> params, double &deltaValue, vector<double> &acceleratedDeltaValues, double &rangeMinimum, double &rangeMaximum, vector<double> &steppedValues, vector<int> &acceleratedTickValues)
{
    auto openSquareBrace = find(params.begin(), params.end(), "[");
    auto closeSquareBrace = find(params.begin(), params.end(), "]");
    
    if(openSquareBrace != params.end() && closeSquareBrace != params.end())
    {
        for(auto it = openSquareBrace + 1; it != closeSquareBrace; ++it)
        {
            string strVal = *(it);
            
            if(regex_match(strVal, regex("-?[0-9]+[.][0-9]+")) || regex_match(strVal, regex("-?[0-9]+")))
                steppedValues.push_back(stod(strVal));
            else if(regex_match(strVal, regex("[(]-?[0-9]+[.][0-9]+[)]")))
                deltaValue = stod(strVal.substr( 1, strVal.length() - 2 ));
            else if(regex_match(strVal, regex("[(]-?[0-9]+[)]")))
                acceleratedTickValues.push_back(stod(strVal.substr( 1, strVal.length() - 2 )));
            else if(regex_match(strVal, regex("[(](-?[0-9]+[.][0-9]+[,])+-?[0-9]+[.][0-9]+[)]")))
            {
                istringstream acceleratedDeltaValueStream(strVal.substr( 1, strVal.length() - 2 ));
                string deltaValue;
                
                while (getline(acceleratedDeltaValueStream, deltaValue, ','))
                    acceleratedDeltaValues.push_back(stod(deltaValue));
            }
            else if(regex_match(strVal, regex("[(](-?[0-9]+[,])+-?[0-9]+[)]")))
            {
                istringstream acceleratedTickValueStream(strVal.substr( 1, strVal.length() - 2 ));
                string tickValue;
                
                while (getline(acceleratedTickValueStream, tickValue, ','))
                    acceleratedTickValues.push_back(stod(tickValue));
            }
            else if(regex_match(strVal, regex("-?[0-9]+[.][0-9]+[>]-?[0-9]+[.][0-9]+")) || regex_match(strVal, regex("[0-9]+[-][0-9]+")))
            {
                istringstream range(strVal);
                vector<string> range_tokens;
                string range_token;
                
                while (getline(range, range_token, '>'))
                    range_tokens.push_back(range_token);
                
                if(range_tokens.size() == 2)
                {
                    double firstValue = stod(range_tokens[0]);
                    double lastValue = stod(range_tokens[1]);
                    
                    if(lastValue > firstValue)
                    {
                        rangeMinimum = firstValue;
                        rangeMaximum = lastValue;
                    }
                    else
                    {
                        rangeMinimum = lastValue;
                        rangeMaximum = firstValue;
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// Widgets
//////////////////////////////////////////////////////////////////////////////
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

    Widget* widget = new Widget(surface, widgetName);
    
    if(! widget)
        return;
    
    surface->AddWidget(widget);

    vector<vector<string>> tokenLines;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        line = regex_replace(line, regex(TabChars), " ");
        line = regex_replace(line, regex(CRLFChars), "");

        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens[0] == "WidgetEnd")    // finito baybay - Widget list complete
            break;
        
        tokenLines.push_back(tokens);
    }
    
    if(tokenLines.size() < 1)
        return;
    
    for(int i = 0; i < tokenLines.size(); i++)
    {
        int size = tokenLines[i].size();
        
        string widgetClass = tokenLines[i][0];

        // Control Signal Generators
        if(widgetClass == "AnyPress" && (size == 4 || size == 7))
            new AnyPress_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        if(widgetClass == "Press" && size == 4)
            new PressRelease_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "Press" && size == 7)
            new PressRelease_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])), new MIDI_event_ex_t(strToHex(tokenLines[i][4]), strToHex(tokenLines[i][5]), strToHex(tokenLines[i][6])));
        else if(widgetClass == "Fader14Bit" && size == 4)
            new Fader14Bit_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "Fader7Bit" && size== 4)
            new Fader7Bit_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "Encoder" && size == 4)
            new Encoder_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "Encoder" && size > 4)
            new AcceleratedEncoder_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])), tokenLines[i]);
        else if(widgetClass == "EncoderPlain" && size == 4)
            new EncoderPlain_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "EncoderPlainReverse" && size == 4)
            new EncoderPlainReverse_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        
        // Feedback Processors
        FeedbackProcessor* feedbackProcessor = nullptr;

        if(widgetClass == "FB_TwoState" && (size == 7 || size == 8))
        {
            feedbackProcessor = new TwoState_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])), new MIDI_event_ex_t(strToHex(tokenLines[i][4]), strToHex(tokenLines[i][5]), strToHex(tokenLines[i][6])));
            
            if(size == 8)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[i][7]));
        }
        else if(widgetClass == "FB_NovationLaunchpadMiniRGB7Bit" && size == 4)
        {
            feedbackProcessor = new NovationLaunchpadMiniRGB7Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_MFT_RGB" && size == 4)
        {
            feedbackProcessor = new MFT_RGB_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_FaderportRGB7Bit" && size == 4)
        {
            feedbackProcessor = new FaderportRGB7Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(size == 4 || size== 5)
        {
            if(widgetClass == "FB_Fader14Bit")
                feedbackProcessor = new Fader14Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
            else if(widgetClass == "FB_Fader7Bit")
                feedbackProcessor = new Fader7Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
            else if(widgetClass == "FB_Encoder")
                feedbackProcessor = new Encoder_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
            else if(widgetClass == "FB_VUMeter")
                feedbackProcessor = new VUMeter_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
            else if(widgetClass == "FB_GainReductionMeter")
                feedbackProcessor = new GainReductionMeter_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
            
            if(size == 5 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[i][4]));
        }
        else if((widgetClass == "FB_MCUTimeDisplay" || widgetClass == "FB_QConProXMasterVUMeter") && size == 1)
        {
            if(widgetClass == "FB_MCUTimeDisplay")
                feedbackProcessor = new MCU_TimeDisplay_Midi_FeedbackProcessor(surface, widget);
            else if(widgetClass == "FB_QConProXMasterVUMeter")
                feedbackProcessor = new QConProXMasterVUMeter_Midi_FeedbackProcessor(surface, widget);
        }
        else if((widgetClass == "FB_MCUVUMeter" || widgetClass == "FB_MCUXTVUMeter") && (size == 2 || size == 3))
        {
            int displayType = widgetClass == "FB_MCUVUMeter" ? 0x14 : 0x15;
            
            feedbackProcessor = new MCUVUMeter_Midi_FeedbackProcessor(surface, widget, displayType, stoi(tokenLines[i][1]));
            
            if(size == 3 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[i][2]));
        }
        else if((widgetClass == "FB_MCUDisplayUpper" || widgetClass == "FB_MCUDisplayLower" || widgetClass == "FB_MCUXTDisplayUpper" || widgetClass == "FB_MCUXTDisplayLower") && (size == 2 || size == 3))
        {
            if(widgetClass == "FB_MCUDisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_MCUDisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_MCUXTDisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x15, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_MCUXTDisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x15, 0x12, stoi(tokenLines[i][1]));
            
            if(size == 3 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[i][2]));
        }
        
        else if((widgetClass == "FB_C4DisplayUpper" || widgetClass == "FB_C4DisplayLower") && (size == 3 || size == 4))
        {
            if(widgetClass == "FB_C4DisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x17, stoi(tokenLines[i][1]) + 0x30, stoi(tokenLines[i][2]));
            else if(widgetClass == "FB_C4DisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x17, stoi(tokenLines[i][1]) + 0x30, stoi(tokenLines[i][2]));
            
            if(size == 4 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[i][3]));
        }
        
        else if((widgetClass == "FB_FP8Display" || widgetClass == "FB_FP16Display") && (size == 2 || size == 3))
        {
            if(widgetClass == "FB_FP8Display")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x02, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_FP16Display")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x16, stoi(tokenLines[i][1]));
            
            if(size == 3 && feedbackProcessor != nullptr)
                feedbackProcessor->SetRefreshInterval(strToDouble(tokenLines[i][2]));
        }
        
        if(feedbackProcessor != nullptr)
            widget->AddFeedbackProcessor(feedbackProcessor);
    }
}

static void ProcessOSCWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  OSC_ControlSurface* surface, vector<Widget*> &widgets)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(surface, tokens[1]);
    
    if(! widget)
        return;
    
    widgets.push_back(widget);

    vector<vector<string>> tokenLines;

    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        line = regex_replace(line, regex(TabChars), " ");
        line = regex_replace(line, regex(CRLFChars), "");

        lineNumber++;
        
        if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
            continue;
        
        vector<string> tokens(GetTokens(line));
        
        if(tokens[0] == "WidgetEnd")    // finito baybay - Widget list complete
            break;
        
        tokenLines.push_back(tokens);
    }

    for(auto tokenLine : tokenLines)
    {
        if(tokenLine.size() > 1 && tokenLine[0] == "Control")
            new OSC_CSIMessageGenerator(surface, widget, tokenLine[1]);
        else if(tokenLine.size() > 1 && tokenLine[0] == "FB_Processor")
            widget->AddFeedbackProcessor(new OSC_FeedbackProcessor(surface, widget, tokenLine[1]));
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
void Manager::InitActionsDictionary()
{
    actions_["SelectTrackRelative"] =               [](Widget* widget, vector<string> params) { return new SelectTrackRelative(widget, params); };
    actions_["TrackAutoMode"] =                     [](Widget* widget, vector<string> params) { return new TrackAutoMode(widget, params); };
    actions_["TimeDisplay"] =                       [](Widget* widget, vector<string> params) { return new TimeDisplay(widget, params); };
    actions_["EuConTimeDisplay"] =                  [](Widget* widget, vector<string> params) { return new EuConTimeDisplay(widget, params); };
    actions_["NoAction"] =                          [](Widget* widget, vector<string> params) { return new NoAction(widget, params); };
    actions_["Reaper"] =                            [](Widget* widget, vector<string> params) { return new ReaperAction(widget, params); };
    actions_["FixedTextDisplay"] =                  [](Widget* widget, vector<string> params) { return new FixedTextDisplay(widget, params); };
    actions_["FixedRGBColourDisplay"] =             [](Widget* widget, vector<string> params) { return new FixedRGBColourDisplay(widget, params); };
    actions_["Rewind"] =                            [](Widget* widget, vector<string> params) { return new Rewind(widget, params); };
    actions_["FastForward"] =                       [](Widget* widget, vector<string> params) { return new FastForward(widget, params); };
    actions_["Play"] =                              [](Widget* widget, vector<string> params) { return new Play(widget, params); };
    actions_["Stop"] =                              [](Widget* widget, vector<string> params) { return new Stop(widget, params); };
    actions_["Record"] =                            [](Widget* widget, vector<string> params) { return new Record(widget, params); };
    actions_["CycleTimeline"] =                     [](Widget* widget, vector<string> params) { return new CycleTimeline(widget, params); };
    actions_["SetShowFXWindows"] =                  [](Widget* widget, vector<string> params) { return new SetShowFXWindows(widget, params); };
    actions_["ToggleScrollLink"] =                  [](Widget* widget, vector<string> params) { return new ToggleScrollLink(widget, params); };
    actions_["ForceScrollLink"] =                   [](Widget* widget, vector<string> params) { return new ForceScrollLink(widget, params); };
    actions_["ToggleVCAMode"] =                     [](Widget* widget, vector<string> params) { return new ToggleVCAMode(widget, params); };
    actions_["CycleTimeDisplayModes"] =             [](Widget* widget, vector<string> params) { return new CycleTimeDisplayModes(widget, params); };
    actions_["NextPage"] =                          [](Widget* widget, vector<string> params) { return new GoNextPage(widget, params); };
    actions_["GoPage"] =                            [](Widget* widget, vector<string> params) { return new GoPage(widget, params); };
    actions_["GoZone"] =                            [](Widget* widget, vector<string> params) { return new GoZone(widget, params); };
    actions_["TrackBank"] =                         [](Widget* widget, vector<string> params) { return new TrackBank(widget, params); };
    actions_["ClearAllSolo"] =                      [](Widget* widget, vector<string> params) { return new ClearAllSolo(widget, params); };
    actions_["Shift"] =                             [](Widget* widget, vector<string> params) { return new SetShift(widget, params); };
    actions_["Option"] =                            [](Widget* widget, vector<string> params) { return new SetOption(widget, params); };
    actions_["Control"] =                           [](Widget* widget, vector<string> params) { return new SetControl(widget, params); };
    actions_["Alt"] =                               [](Widget* widget, vector<string> params) { return new SetAlt(widget, params); };
    actions_["ToggleLearnMode"] =                   [](Widget* widget, vector<string> params) { return new ToggleLearnMode(widget, params); };
    actions_["ToggleMapSelectedTrackSends"] =       [](Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackSends(widget, params); };
    actions_["MapSelectedTrackSendsToWidgets"] =    [](Widget* widget, vector<string> params) { return new MapSelectedTrackSendsToWidgets(widget, params); };
    actions_["ToggleMapSelectedTrackFX"] =          [](Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackFX(widget, params); };
    actions_["MapSelectedTrackFXToWidgets"] =       [](Widget* widget, vector<string> params) { return new MapSelectedTrackFXToWidgets(widget, params); };
    actions_["ToggleMapSelectedTrackFXMenu"] =      [](Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackFXMenu(widget, params); };
    actions_["MapSelectedTrackFXToMenu"] =          [](Widget* widget, vector<string> params) { return new MapSelectedTrackFXToMenu(widget, params); };
    actions_["ToggleMapFocusedFX"] =                [](Widget* widget, vector<string> params) { return new ToggleMapFocusedFX(widget, params); };
    actions_["MapFocusedFXToWidgets"] =             [](Widget* widget, vector<string> params) { return new MapFocusedFXToWidgets(widget, params); };
    actions_["GoFXSlot"] =                          [](Widget* widget, vector<string> params) { return new GoFXSlot(widget, params); };
    actions_["CycleTrackAutoMode"] =                [](Widget* widget, vector<string> params) { return new CycleTrackAutoMode(widget, params); };
    actions_["EuConCycleTrackAutoMode"] =           [](Widget* widget, vector<string> params) { return new EuConCycleTrackAutoMode(widget, params); };
    actions_["GlobalAutoMode"] =                    [](Widget* widget, vector<string> params) { return new GlobalAutoMode(widget, params); };
    actions_["FocusedFXParam"] =                    [](Widget* widget, vector<string> params) { return new FocusedFXParam(widget, params); };
    actions_["FocusedFXParamNameDisplay"] =         [](Widget* widget, vector<string> params) { return new FocusedFXParamNameDisplay(widget, params); };
    actions_["FocusedFXParamValueDisplay"] =        [](Widget* widget, vector<string> params) { return new FocusedFXParamValueDisplay(widget, params); };
}

void Manager::InitActionsWithNavigatorDictionary()
{
    actionsWithNavigator_["TrackVolume"] =                       [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackVolume(widget, params, navigator); };
    actionsWithNavigator_["SoftTakeover7BitTrackVolume"] =       [](Widget* widget, vector<string> params, Navigator* navigator) { return new SoftTakeover7BitTrackVolume(widget, params, navigator); };
    actionsWithNavigator_["SoftTakeover14BitTrackVolume"] =      [](Widget* widget, vector<string> params, Navigator* navigator) { return new SoftTakeover14BitTrackVolume(widget, params, navigator); };
    actionsWithNavigator_["TrackVolumeDB"] =                     [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackVolumeDB(widget, params, navigator); };
    actionsWithNavigator_["TrackToggleVCASpill"] =               [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackToggleVCASpill(widget, params, navigator); };
    actionsWithNavigator_["TrackSelect"] =                       [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackSelect(widget, params, navigator); };
    actionsWithNavigator_["TrackUniqueSelect"] =                 [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackUniqueSelect(widget, params, navigator); };
    actionsWithNavigator_["TrackRangeSelect"] =                  [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackRangeSelect(widget, params, navigator); };
    actionsWithNavigator_["TrackRecordArm"] =                    [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackRecordArm(widget, params, navigator); };
    actionsWithNavigator_["TrackMute"] =                         [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackMute(widget, params, navigator); };
    actionsWithNavigator_["TrackSolo"] =                         [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackSolo(widget, params, navigator); };
    actionsWithNavigator_["TrackTouch"] =                        [](Widget* widget, vector<string> params, Navigator* navigator) { return new SetFaderTouch(widget, params, navigator); };
    actionsWithNavigator_["RotaryTouch"] =                       [](Widget* widget, vector<string> params, Navigator* navigator) { return new SetRotaryTouch(widget, params, navigator); };
    actionsWithNavigator_["TrackPan"] =                          [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPan(widget, params, navigator); };
    actionsWithNavigator_["TrackPanPercent"] =                   [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanPercent(widget, params, navigator); };
    actionsWithNavigator_["TrackPanWidth"] =                     [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanWidth(widget, params, navigator); };
    actionsWithNavigator_["TrackPanWidthPercent"] =              [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanWidthPercent(widget, params, navigator); };
    actionsWithNavigator_["TrackPanLPercent"] =                  [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanLPercent(widget, params, navigator); };
    actionsWithNavigator_["TrackPanRPercent"] =                  [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanRPercent(widget, params, navigator); };
    actionsWithNavigator_["TogglePin"] =                         [](Widget* widget, vector<string> params, Navigator* navigator) { return new TogglePin(widget, params, navigator); };
    actionsWithNavigator_["FXNameDisplay"] =                     [](Widget* widget, vector<string> params, Navigator* navigator) { return new FXNameDisplay(widget, params, navigator); };
    actionsWithNavigator_["TrackNameDisplay"] =                  [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackNameDisplay(widget, params, navigator); };
    actionsWithNavigator_["TrackVolumeDisplay"] =                [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackVolumeDisplay(widget, params, navigator); };
    actionsWithNavigator_["TrackPanDisplay"] =                   [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanDisplay(widget, params, navigator); };
    actionsWithNavigator_["TrackPanWidthDisplay"] =              [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackPanWidthDisplay(widget, params, navigator); };
    actionsWithNavigator_["TrackOutputMeter"] =                  [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackOutputMeter(widget, params, navigator); };
    actionsWithNavigator_["TrackOutputMeterAverageLR"] =         [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackOutputMeterAverageLR(widget, params, navigator); };
    actionsWithNavigator_["TrackOutputMeterMaxPeakLR"] =         [](Widget* widget, vector<string> params, Navigator* navigator) { return new TrackOutputMeterMaxPeakLR(widget, params, navigator); };
}

void Manager::InitActionsWithNavigatorAndIndexDictionary()
{
    actionsWithNavigatorAndIndex_["FXParam"] =                  [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new FXParam(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["FXParamRelative"] =          [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new FXParamRelative(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["FXParamNameDisplay"] =       [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new FXParamNameDisplay(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["FXParamValueDisplay"] =      [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new FXParamValueDisplay(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["FXGainReductionMeter"] =     [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new FXGainReductionMeter(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendVolume"] =          [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendVolume(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendVolumeDB"] =        [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendVolumeDB(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendPan"] =             [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendPan(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendMute"] =            [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendMute(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendInvertPolarity"] =  [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendInvertPolarity(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendPrePost"] =         [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendPrePost(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendNameDisplay"] =     [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendNameDisplay(widget, params, navigator, index); };
    actionsWithNavigatorAndIndex_["TrackSendVolumeDisplay"] =   [](Widget* widget, vector<string> params, Navigator* navigator, int index) { return new TrackSendVolumeDisplay(widget, params, navigator, index); };
}

void Manager::InitActionDictionaryOld()
{
    actionsOld_["NoAction"] =                          [](Widget* widget, vector<string> params) { return new NoAction(widget, params); };
    actionsOld_["Reaper"] =                            [](Widget* widget, vector<string> params) { return new ReaperAction(widget, params); };
    actionsOld_["FXNameDisplay"] =                     [](Widget* widget, vector<string> params) { return new FXNameDisplay(widget, params); };
    actionsOld_["FXParam"] =                           [](Widget* widget, vector<string> params) { return new FXParam(widget, params); };
    actionsOld_["FXParamRelative"] =                   [](Widget* widget, vector<string> params) { return new FXParamRelative(widget, params); };
    actionsOld_["FocusedFXParam"] =                    [](Widget* widget, vector<string> params) { return new FocusedFXParam(widget, params); };
    actionsOld_["FXParamNameDisplay"] =                [](Widget* widget, vector<string> params) { return new FXParamNameDisplay(widget, params); };
    actionsOld_["FXParamValueDisplay"] =               [](Widget* widget, vector<string> params) { return new FXParamValueDisplay(widget, params); };
    actionsOld_["FocusedFXParamNameDisplay"] =         [](Widget* widget, vector<string> params) { return new FocusedFXParamNameDisplay(widget, params); };
    actionsOld_["FocusedFXParamValueDisplay"] =        [](Widget* widget, vector<string> params) { return new FocusedFXParamValueDisplay(widget, params); };
    actionsOld_["FXGainReductionMeter"] =              [](Widget* widget, vector<string> params) { return new FXGainReductionMeter(widget, params); };
    actionsOld_["TrackVolume"] =                       [](Widget* widget, vector<string> params) { return new TrackVolume(widget, params); };
    actionsOld_["SoftTakeover7BitTrackVolume"] =       [](Widget* widget, vector<string> params) { return new SoftTakeover7BitTrackVolume(widget, params); };
    actionsOld_["SoftTakeover14BitTrackVolume"] =      [](Widget* widget, vector<string> params) { return new SoftTakeover14BitTrackVolume(widget, params); };
    actionsOld_["TrackVolumeDB"] =                     [](Widget* widget, vector<string> params) { return new TrackVolumeDB(widget, params); };
    actionsOld_["TrackSendVolume"] =                   [](Widget* widget, vector<string> params) { return new TrackSendVolume(widget, params); };
    actionsOld_["TrackSendVolumeDB"] =                 [](Widget* widget, vector<string> params) { return new TrackSendVolumeDB(widget, params); };
    actionsOld_["TrackSendPan"] =                      [](Widget* widget, vector<string> params) { return new TrackSendPan(widget, params); };
    actionsOld_["TrackSendMute"] =                     [](Widget* widget, vector<string> params) { return new TrackSendMute(widget, params); };
    actionsOld_["TrackSendInvertPolarity"] =           [](Widget* widget, vector<string> params) { return new TrackSendInvertPolarity(widget, params); };
    actionsOld_["TrackSendPrePost"] =                  [](Widget* widget, vector<string> params) { return new TrackSendPrePost(widget, params); };
    actionsOld_["TrackPan"] =                          [](Widget* widget, vector<string> params) { return new TrackPan(widget, params); };
    actionsOld_["TrackPanPercent"] =                   [](Widget* widget, vector<string> params) { return new TrackPanPercent(widget, params); };
    actionsOld_["TrackPanWidth"] =                     [](Widget* widget, vector<string> params) { return new TrackPanWidth(widget, params); };
    actionsOld_["TrackPanWidthPercent"] =              [](Widget* widget, vector<string> params) { return new TrackPanWidthPercent(widget, params); };
    actionsOld_["TrackPanLPercent"] =                  [](Widget* widget, vector<string> params) { return new TrackPanLPercent(widget, params); };
    actionsOld_["TrackPanRPercent"] =                  [](Widget* widget, vector<string> params) { return new TrackPanRPercent(widget, params); };
    actionsOld_["FixedTextDisplay"] =                  [](Widget* widget, vector<string> params) { return new FixedTextDisplay(widget, params); };
    actionsOld_["FixedRGBColourDisplay"] =             [](Widget* widget, vector<string> params) { return new FixedRGBColourDisplay(widget, params); };
    actionsOld_["TrackNameDisplay"] =                  [](Widget* widget, vector<string> params) { return new TrackNameDisplay(widget, params); };
    actionsOld_["TrackVolumeDisplay"] =                [](Widget* widget, vector<string> params) { return new TrackVolumeDisplay(widget, params); };
    actionsOld_["TrackSendNameDisplay"] =              [](Widget* widget, vector<string> params) { return new TrackSendNameDisplay(widget, params); };
    actionsOld_["TrackSendVolumeDisplay"] =            [](Widget* widget, vector<string> params) { return new TrackSendVolumeDisplay(widget, params); };
    actionsOld_["TrackPanDisplay"] =                   [](Widget* widget, vector<string> params) { return new TrackPanDisplay(widget, params); };
    actionsOld_["TrackPanWidthDisplay"] =              [](Widget* widget, vector<string> params) { return new TrackPanWidthDisplay(widget, params); };
    actionsOld_["TimeDisplay"] =                       [](Widget* widget, vector<string> params) { return new TimeDisplay(widget, params); };
    actionsOld_["EuConTimeDisplay"] =                  [](Widget* widget, vector<string> params) { return new EuConTimeDisplay(widget, params); };
    actionsOld_["Rewind"] =                            [](Widget* widget, vector<string> params) { return new Rewind(widget, params); };
    actionsOld_["FastForward"] =                       [](Widget* widget, vector<string> params) { return new FastForward(widget, params); };
    actionsOld_["Play"] =                              [](Widget* widget, vector<string> params) { return new Play(widget, params); };
    actionsOld_["Stop"] =                              [](Widget* widget, vector<string> params) { return new Stop(widget, params); };
    actionsOld_["Record"] =                            [](Widget* widget, vector<string> params) { return new Record(widget, params); };
    actionsOld_["TrackToggleVCASpill"] =               [](Widget* widget, vector<string> params) { return new TrackToggleVCASpill(widget, params); };
    actionsOld_["TrackSelect"] =                       [](Widget* widget, vector<string> params) { return new TrackSelect(widget, params); };
    actionsOld_["TrackUniqueSelect"] =                 [](Widget* widget, vector<string> params) { return new TrackUniqueSelect(widget, params); };
    actionsOld_["TrackRangeSelect"] =                  [](Widget* widget, vector<string> params) { return new TrackRangeSelect(widget, params); };
    actionsOld_["TrackRecordArm"] =                    [](Widget* widget, vector<string> params) { return new TrackRecordArm(widget, params); };
    actionsOld_["TrackMute"] =                         [](Widget* widget, vector<string> params) { return new TrackMute(widget, params); };
    actionsOld_["TrackSolo"] =                         [](Widget* widget, vector<string> params) { return new TrackSolo(widget, params); };
    actionsOld_["TrackTouch"] =                        [](Widget* widget, vector<string> params) { return new SetFaderTouch(widget, params); };
    actionsOld_["RotaryTouch"] =                       [](Widget* widget, vector<string> params) { return new SetRotaryTouch(widget, params); };
    actionsOld_["CycleTimeline"] =                     [](Widget* widget, vector<string> params) { return new CycleTimeline(widget, params); };
    actionsOld_["TrackOutputMeter"] =                  [](Widget* widget, vector<string> params) { return new TrackOutputMeter(widget, params); };
    actionsOld_["TrackOutputMeterAverageLR"] =         [](Widget* widget, vector<string> params) { return new TrackOutputMeterAverageLR(widget, params); };
    actionsOld_["TrackOutputMeterMaxPeakLR"] =         [](Widget* widget, vector<string> params) { return new TrackOutputMeterMaxPeakLR(widget, params); };
    actionsOld_["SetShowFXWindows"] =                  [](Widget* widget, vector<string> params) { return new SetShowFXWindows(widget, params); };
    actionsOld_["ToggleScrollLink"] =                  [](Widget* widget, vector<string> params) { return new ToggleScrollLink(widget, params); };
    actionsOld_["ForceScrollLink"] =                   [](Widget* widget, vector<string> params) { return new ForceScrollLink(widget, params); };
    actionsOld_["ToggleVCAMode"] =                     [](Widget* widget, vector<string> params) { return new ToggleVCAMode(widget, params); };
    actionsOld_["CycleTimeDisplayModes"] =             [](Widget* widget, vector<string> params) { return new CycleTimeDisplayModes(widget, params); };
    actionsOld_["NextPage"] =                          [](Widget* widget, vector<string> params) { return new GoNextPage(widget, params); };
    actionsOld_["GoPage"] =                            [](Widget* widget, vector<string> params) { return new  GoPage(widget, params); };
    actionsOld_["GoZone"] =                            [](Widget* widget, vector<string> params) { return new GoZone(widget, params); };
    actionsOld_["SelectTrackRelative"] =               [](Widget* widget, vector<string> params) { return new SelectTrackRelative(widget, params); };
    actionsOld_["TrackBank"] =                         [](Widget* widget, vector<string> params) { return new TrackBank(widget, params); };
    actionsOld_["ClearAllSolo"] =                      [](Widget* widget, vector<string> params) { return new ClearAllSolo(widget, params); };
    actionsOld_["Shift"] =                             [](Widget* widget, vector<string> params) { return new SetShift(widget, params); };
    actionsOld_["Option"] =                            [](Widget* widget, vector<string> params) { return new SetOption(widget, params); };
    actionsOld_["Control"] =                           [](Widget* widget, vector<string> params) { return new SetControl(widget, params); };
    actionsOld_["Alt"] =                               [](Widget* widget, vector<string> params) { return new SetAlt(widget, params); };
    actionsOld_["TogglePin"] =                         [](Widget* widget, vector<string> params) { return new TogglePin(widget, params); };
    actionsOld_["ToggleLearnMode"] =                   [](Widget* widget, vector<string> params) { return new ToggleLearnMode(widget, params); };
    actionsOld_["ToggleMapSelectedTrackSends"] =       [](Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackSends(widget, params); };
    actionsOld_["MapSelectedTrackSendsToWidgets"] =    [](Widget* widget, vector<string> params) { return new MapSelectedTrackSendsToWidgets(widget, params); };
    actionsOld_["ToggleMapSelectedTrackFX"] =          [](Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackFX(widget, params); };
    actionsOld_["MapSelectedTrackFXToWidgets"] =       [](Widget* widget, vector<string> params) { return new MapSelectedTrackFXToWidgets(widget, params); };
    actionsOld_["ToggleMapSelectedTrackFXMenu"] =      [](Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackFXMenu(widget, params); };
    actionsOld_["MapSelectedTrackFXToMenu"] =          [](Widget* widget, vector<string> params) { return new MapSelectedTrackFXToMenu(widget, params); };
    actionsOld_["ToggleMapFocusedFX"] =                [](Widget* widget, vector<string> params) { return new ToggleMapFocusedFX(widget, params); };
    actionsOld_["MapFocusedFXToWidgets"] =             [](Widget* widget, vector<string> params) { return new MapFocusedFXToWidgets(widget, params); };
    actionsOld_["GoFXSlot"] =                          [](Widget* widget, vector<string> params) { return new GoFXSlot(widget, params); };
    actionsOld_["TrackAutoMode"] =                     [](Widget* widget, vector<string> params) { return new TrackAutoMode(widget, params); };
    actionsOld_["CycleTrackAutoMode"] =                [](Widget* widget, vector<string> params) { return new CycleTrackAutoMode(widget, params); };
    actionsOld_["EuConCycleTrackAutoMode"] =           [](Widget* widget, vector<string> params) { return new EuConCycleTrackAutoMode(widget, params); };
    actionsOld_["GlobalAutoMode"] =                    [](Widget* widget, vector<string> params) { return new GlobalAutoMode(widget, params); };
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
            
            if(tokens.size() > 4) // ignore comment lines and blank lines
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
                    int inPort = 0;
                    int outPort = 0;
                    
                    if(tokens[0] == MidiSurfaceToken || tokens[0] == OSCSurfaceToken)
                    {
                        inPort = atoi(tokens[2].c_str());
                        outPort = atoi(tokens[3].c_str());
                    }
                    
                    if(currentPage)
                    {
                        ControlSurface* surface = nullptr;
                        
                        if(tokens[0] == MidiSurfaceToken && tokens.size() == 15)
                            surface = new Midi_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], atoi(tokens[6].c_str()), atoi(tokens[7].c_str()), atoi(tokens[8].c_str()), atoi(tokens[9].c_str()), GetMidiInputForPort(inPort), GetMidiOutputForPort(outPort));
                        else if(tokens[0] == OSCSurfaceToken && tokens.size() == 16)
                            surface = new OSC_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], atoi(tokens[6].c_str()), atoi(tokens[7].c_str()), atoi(tokens[8].c_str()), atoi(tokens[9].c_str()), GetInputSocketForPort(tokens[1], inPort), GetOutputSocketForAddressAndPort(tokens[1], tokens[15], outPort));
                        else if(tokens[0] == EuConSurfaceToken && tokens.size() == 8)
                            surface = new EuCon_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[2], atoi(tokens[3].c_str()), atoi(tokens[4].c_str()), atoi(tokens[5].c_str()), atoi(tokens[6].c_str()));

                        currentPage->AddSurface(surface);
                        
                        if(tokens[0] == EuConSurfaceToken && tokens.size() == 8)
                        {
                            if(tokens[7] == "UseZoneLink")
                                surface->SetUseZoneLink(true);
                        }
                        else if(tokens.size() == 15 || tokens.size() == 16)
                        {
                            if(tokens[10] == "UseZoneLink")
                                surface->SetUseZoneLink(true);
                            
                            if(tokens[11] == "AutoMapSends")
                                surface->GetSendsActivationManager()->SetShouldMapSends(true);
                            
                            if(tokens[12] == "AutoMapFX")
                                surface->GetFXActivationManager()->SetShouldMapSelectedTrackFX(true);
                            
                            if(tokens[13] == "AutoMapFXMenu")
                                surface->GetFXActivationManager()->SetShouldMapSelectedTrackFXMenus(true);
                            
                            if(tokens[14] == "AutoMapFocusedFX")
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
void TrackNavigator::PinChannel()
{
    if( ! isChannelPinned_)
    {
        pinnedTrack_ = GetTrack();
        
        isChannelPinned_ = true;
        
        manager_->IncChannelBias(pinnedTrack_, channelNum_);
    }
}

void TrackNavigator::UnpinChannel()
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
MediaTrack* MasterTrackNavigator::GetTrack()
{
    return DAW::GetMasterTrack(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectedTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* SelectedTrackNavigator::GetTrack()
{
    return page_->GetTrackNavigationManager()->GetSelectedTrack();
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
        return page_->GetTrackNavigationManager()->GetTrackFromId(trackNumber);
    else
        return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
Navigator* TrackNavigationManager::AddNavigator()
{
    int channelNum = navigators_.size();
    navigators_.push_back(new TrackNavigator(page_, this, channelNum));
    return navigators_[channelNum];
}

void TrackNavigationManager::ForceScrollLink()
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









void Widget::AddAction(ZoneOld* zone, string modifiers, Action* action)
{
    actionsOld_[zone][modifiers].push_back(action);
    zonesAvailable_[zone->GetName()] = zone;
}

void Widget::AddTrackTouchedAction(ZoneOld* zone, string modifiers, Action* action)
{
    trackTouchedActions_[zone][modifiers].push_back(action);
}

void Widget::AddTrackRotaryTouchedAction(ZoneOld* zone, string modifiers, Action* action)
{
    trackRotaryTouchedActions_[zone][modifiers].push_back(action);
}

MediaTrack* Widget::GetTrack()
{
    if(activeZone_ != nullptr)
        return activeZone_->GetNavigator()->GetTrack();
    else
        return nullptr;
}

Navigator* Widget::GetNavigator()
{
    if(activeZone_ != nullptr)
        return activeZone_->GetNavigator();
    else
        return nullptr;
}

int Widget::GetSlotIndex()
{
    if(activeZone_ != nullptr)
        return activeZone_->GetIndex();
    else
        return 0;
}

int Widget::GetParamIndex()
{
    if(activeZone_ != nullptr)
    {
        string modifiers = surface_->GetPage()->GetModifiers();

        if(activeZone_ != nullptr && actionsOld_[activeZone_].count(modifiers) > 0 && actionsOld_[activeZone_][modifiers].size() > 0)
        {
            return actionsOld_[activeZone_][modifiers][0]->GetParamNum();
        }
        else
            return 0;
    }
    else
        return 0;
}










void Widget::RequestUpdate()
{
    string modifiers = "";
    if( ! isModifier_ )
        modifiers = surface_->GetPage()->GetModifiers();

    if(actions_.count(modifiers) > 0 && actions_[modifiers].size() > 0)
        actions_[modifiers][0]->RequestUpdate();
    
    /*
    
    if(activeZone_ != nullptr)
    {
        string modifiers = "";
        if( ! isModifier_ )
            modifiers = surface_->GetPage()->GetModifiers();
        
        if(GetIsRotaryTouched()  )
        {
            if(trackRotaryTouchedActions_.count(activeZone_) > 0 && trackRotaryTouchedActions_[activeZone_].count(modifiers) > 0 && trackRotaryTouchedActions_[activeZone_][modifiers].size() > 0)
            {
                for(auto action : trackRotaryTouchedActions_[activeZone_][modifiers])
                    action->PerformDeferredActions();
                
                trackRotaryTouchedActions_[activeZone_][modifiers][0]->RequestUpdate();
            }
        }
        else if(GetIsFaderTouched() && trackTouchedActions_.count(activeZone_) > 0 && trackTouchedActions_[activeZone_].count(modifiers) > 0 && trackTouchedActions_[activeZone_][modifiers].size() > 0)
        {
            for(auto action : trackTouchedActions_[activeZone_][modifiers])
                action->PerformDeferredActions();

            trackTouchedActions_[activeZone_][modifiers][0]->RequestUpdate();
        }
        else if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0 && actionsOld_[activeZone_][modifiers].size() > 0)
        {
            for(auto action : actionsOld_[activeZone_][modifiers])
                action->PerformDeferredActions();
            
            actionsOld_[activeZone_][modifiers][0]->RequestUpdate();
        }
        else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0 && actionsOld_[activeZone_][""].size() > 0)
        {
            for(auto action : actionsOld_[activeZone_][""])
                action->PerformDeferredActions();

            actionsOld_[activeZone_][""][0]->RequestUpdate();
        }
    }
    */
    
}

void Widget::DoAction(double value)
{
    if( TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN <- %s %s %f\n", GetSurface()->GetName().c_str(), GetName().c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }

    GetSurface()->GetPage()->InputReceived(this, value);

    string modifiers = "";
    
    if( ! isModifier_)
        modifiers = surface_->GetPage()->GetModifiers();

    if(actions_.count(modifiers) > 0 && actions_[modifiers].size() > 0)
        for(auto action : actions_[modifiers])
            action->DoAction(value, this);
    else if(modifiers != "" && actions_.count("") > 0 && actions_[""].size() > 0)
        for(auto action : actions_[""])
            action->DoAction(value, this);

    /*
    if(activeZone_ != nullptr)
    {
        if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0)
            for(auto action : actionsOld_[activeZone_][modifiers])
                action->DoAction(value, this);
        else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0)
            for(auto action : actionsOld_[activeZone_][""])
                action->DoAction(value, this);
    }
    */
}

void Widget::DoRelativeAction(double delta)
{
    if( TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN <- %s %s %f\n", GetSurface()->GetName().c_str(), GetName().c_str(), delta);
        DAW::ShowConsoleMsg(buffer);
    }
    
    GetSurface()->GetPage()->InputReceived(this, delta);
    
    string modifiers = "";
    
    if( ! isModifier_)
        modifiers = surface_->GetPage()->GetModifiers();
    
    if(actions_.count(modifiers) > 0 && actions_[modifiers].size() > 0)
        for(auto action : actions_[modifiers])
            action->DoRelativeAction(delta, this);
    else if(modifiers != "" && actions_.count("") > 0 && actions_[""].size() > 0)
        for(auto action : actions_[""])
            action->DoRelativeAction(delta, this);
    
    /*
    if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0)
        for(auto action : actionsOld_[activeZone_][modifiers])
            action->DoRelativeAction(delta, this);
    else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0)
        for(auto action : actionsOld_[activeZone_][""])
            action->DoRelativeAction(delta, this);
     */
}

void Widget::DoRelativeAction(int accelerationIndex, double delta)
{
    if( TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN <- %s %s %d\n", GetSurface()->GetName().c_str(), GetName().c_str(), accelerationIndex);
        DAW::ShowConsoleMsg(buffer);
    }
    
    GetSurface()->GetPage()->InputReceived(this, accelerationIndex);
    
    string modifiers = "";
    
    if( ! isModifier_)
        modifiers = surface_->GetPage()->GetModifiers();
    
    if(actions_.count(modifiers) > 0 && actions_[modifiers].size() > 0)
        for(auto action : actions_[modifiers])
            action->DoRelativeAction(accelerationIndex, delta, this);
    else if(modifiers != "" && actions_.count("") > 0 && actions_[""].size() > 0)
        for(auto action : actions_[""])
            action->DoRelativeAction(accelerationIndex, delta, this);
    
    /*
    if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0)
        for(auto action : actionsOld_[activeZone_][modifiers])
            action->DoRelativeAction(accelerationIndex, delta, this);
    else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0)
        for(auto action : actionsOld_[activeZone_][""])
            action->DoRelativeAction(accelerationIndex, delta, this);
     */
}

void Widget::SilentSetValue(string displayText)
{
    for(auto processor : feedbackProcessors_)
        processor->SilentSetValue(displayText);
}

void  Widget::UpdateValue(double value)
{
    for(auto processor : feedbackProcessors_)
        processor->UpdateValue(value);
}

void  Widget::UpdateValue(int mode, double value)
{
    for(auto processor : feedbackProcessors_)
        processor->UpdateValue(mode, value);
}

void  Widget::UpdateValue(string value)
{
    for(auto processor : feedbackProcessors_)
        processor->UpdateValue(value);
}

void  Widget::UpdateRGBValue(int r, int g, int b)
{
    for(auto processor : feedbackProcessors_)
        processor->UpdateRGBValue(r, g, b);
}

void  Widget::Clear()
{
    for(auto processor : feedbackProcessors_)
        processor->Clear();
}

void  Widget::ForceClear()
{
    for(auto processor : feedbackProcessors_)
        processor->ForceClear();
}

void Widget::ClearCache()
{
    for(auto processor : feedbackProcessors_)
        processor->ClearCache();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Action::Action(Widget* widget, vector<string> params): widget_(widget), navigator_(widget->GetSurface()->GetPage()->GetDefaultNavigator())
{
    SetParams(params);
}

Action::Action(Widget* widget, vector<string> params, Navigator* navigator): widget_(widget), navigator_(navigator)
{
    SetParams(params);
}

Action::Action(Widget* widget, vector<string> params, Navigator* navigator, int slotIndex, int paramIndex): widget_(widget), navigator_(navigator), slotIndex_(slotIndex), paramIndex_(paramIndex)
{
    SetParams(params);
}

void Action::SetParams(vector<string> params)
{
    if(params.size() > 0)
    {
        SetRGB(params, supportsRGB_, supportsTrackColor_, RGBValues_);
        SetSteppedValues(params, deltaValue_, acceleratedDeltaValues_, rangeMinimum_, rangeMaximum_, steppedValues_, acceleratedTickValues_);
    }
    
    if(acceleratedTickValues_.size() < 1)
        acceleratedTickValues_.push_back(10);
}

Page* Action::GetPage()
{
    return widget_->GetSurface()->GetPage();
}

ControlSurface* Action::GetSurface()
{
    return widget_->GetSurface();
}











TrackNavigationManager* Action::GetTrackNavigationManager()
{
    return GetPage()->GetTrackNavigationManager();
}

MediaTrack* Action::GetTrack()
{
    //return navigator_->GetTrack();
    return widget_->GetNavigator()->GetTrack();
}

int Action::GetSlotIndex()
{
    return widget_->GetSlotIndex();
}









void Action::RequestUpdate()
{
    if(supportsRGB_)
        widget_->UpdateRGBValue(RGBValues_[0].r, RGBValues_[0].g, RGBValues_[0].b);
}

void Action::ClearWidget()
{
    widget_->Clear();
}

void Action::UpdateWidgetValue(double value)
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
        if(MediaTrack* track = GetTrack())
        {
            unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
            
            int r = (*rgb_colour >> 0) & 0xff;
            int g = (*rgb_colour >> 8) & 0xff;
            int b = (*rgb_colour >> 16) & 0xff;
            
            widget_->UpdateRGBValue(r, g, b);
        }
    }
}

void Action::UpdateWidgetValue(int param, double value)
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
        if(MediaTrack* track = GetTrack())
        {
            unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
            
            int r = (*rgb_colour >> 0) & 0xff;
            int g = (*rgb_colour >> 8) & 0xff;
            int b = (*rgb_colour >> 16) & 0xff;
            
            widget_->UpdateRGBValue(r, g, b);
        }
    }
}

void Action::UpdateWidgetValue(string value)
{
    widget_->UpdateValue(value);
    
    if(supportsTrackColor_)
    {
        if(MediaTrack* track = GetTrack())
        {
            unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
            
            int r = (*rgb_colour >> 0) & 0xff;
            int g = (*rgb_colour >> 8) & 0xff;
            int b = (*rgb_colour >> 16) & 0xff;
            
            widget_->UpdateRGBValue(r, g, b);
        }
    }
}

void Action::DoAction(double value, Widget* sender)
{
    if(steppedValues_.size() > 0 && value != 0.0)
    {
        if(steppedValuesIndex_ == steppedValues_.size() - 1)
        {
            if(steppedValues_[0] < steppedValues_[steppedValuesIndex_]) // GAW -- only wrap if 1st value is lower
                steppedValuesIndex_ = 0;
        }
        else
            steppedValuesIndex_++;
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_], sender);
    }
    else
        DoRangeBoundAction(value, sender);
}

void Action::DoRelativeAction(double delta, Widget* sender)
{
    if(steppedValues_.size() > 0)
        DoAcceleratedSteppedValueAction(0, delta, sender);
    else
    {
        if(deltaValue_ != 0.0)
        {
            if(delta > 0.0)
                delta = deltaValue_;
            else if(delta < 0.0)
                delta = -deltaValue_;
        }
        
        DoRangeBoundAction(lastValue_ + delta, sender);
    }
}

void Action::DoRelativeAction(int accelerationIndex, double delta, Widget* sender)
{
    if(steppedValues_.size() > 0)
        DoAcceleratedSteppedValueAction(accelerationIndex, delta, sender);
    else if(acceleratedDeltaValues_.size() > 0)
        DoAcceleratedDeltaValueAction(accelerationIndex, delta, sender);
    else
    {
        if(deltaValue_ != 0.0)
        {
            if(delta >= 0.0)
                delta = deltaValue_;
            else if(delta < 0.0)
                delta = -deltaValue_;
        }
        
        DoRangeBoundAction(lastValue_ + delta, sender);
    }
}

void Action::DoRangeBoundAction(double value, Widget* sender)
{
    if(delayAmount_ != 0.0)
    {
        if(value == 0.0)
        {
            delayStartTime_ = 0.0;
            deferredValue_ = 0.0;
            deferredSender_ = nullptr;
        }
        else
        {
            delayStartTime_ = DAW::GetCurrentNumberOfMilliseconds();
            deferredValue_ = value;
            deferredSender_ = sender;
        }
    }
    else
    {
        if(shouldToggle_ && value != 0.0)
            value = ! GetCurrentValue();
        
        if(value > rangeMaximum_)
            value = rangeMaximum_;
        
        if(value < rangeMinimum_)
            value = rangeMinimum_;
        
        Do(value, sender);
    }
}

void Action::DoAcceleratedSteppedValueAction(int accelerationIndex, double delta, Widget* sender)
{
    if(delta > 0)
    {
        accumulatedIncTicks_++;
        accumulatedDecTicks_ = accumulatedDecTicks_ - 1 < 0 ? 0 : accumulatedDecTicks_ - 1;
    }
    else if(delta < 0)
    {
        accumulatedDecTicks_++;
        accumulatedIncTicks_ = accumulatedIncTicks_ - 1 < 0 ? 0 : accumulatedIncTicks_ - 1;
    }

    accelerationIndex = accelerationIndex > acceleratedTickValues_.size() - 1 ? acceleratedTickValues_.size() - 1 : accelerationIndex;
    accelerationIndex = accelerationIndex < 0 ? 0 : accelerationIndex;
    
    if(delta > 0 && accumulatedIncTicks_ >= acceleratedTickValues_[accelerationIndex])
    {
        accumulatedIncTicks_ = 0;
        accumulatedDecTicks_ = 0;
        
        steppedValuesIndex_++;
        
        if(steppedValuesIndex_ > steppedValues_.size() - 1)
            steppedValuesIndex_ = steppedValues_.size() - 1;
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_], sender);
    }
    else if(delta < 0 && accumulatedDecTicks_ >= acceleratedTickValues_[accelerationIndex])
    {
        accumulatedIncTicks_ = 0;
        accumulatedDecTicks_ = 0;
        
        steppedValuesIndex_--;
        
        if(steppedValuesIndex_ < 0 )
            steppedValuesIndex_ = 0;
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_], sender);
    }
}

void Action::DoAcceleratedDeltaValueAction(int accelerationIndex, double delta, Widget* sender)
{
    accelerationIndex = accelerationIndex > acceleratedDeltaValues_.size() - 1 ? acceleratedDeltaValues_.size() - 1 : accelerationIndex;
    accelerationIndex = accelerationIndex < 0 ? 0 : accelerationIndex;
    
    if(delta > 0.0)
        DoRangeBoundAction(lastValue_ + acceleratedDeltaValues_[accelerationIndex], sender);
    else
        DoRangeBoundAction(lastValue_ - acceleratedDeltaValues_[accelerationIndex], sender);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ZoneTemplate
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Zone ZoneTemplate::Activate(ControlSurface*  surface)
{
    Zone zone;
    
    for(auto includedZoneTemplate : includedZoneTemplates)
    {
        if(includedZoneTemplate.find("Channel|") != string::npos)
        {
            string chTemplate = "Channel|";
            
            if(ZoneTemplate* zoneTemplate = surface->GetZoneTemplate(chTemplate))
                for(int i = 0; i < surface->GetNumChannels(); i++)
                    zone.AddZone(zoneTemplate->Activate(surface, i, surface->GetNavigatorForChannel(i)));
        }
        else if(ZoneTemplate* zoneTemplate = surface->GetZoneTemplate(includedZoneTemplate))
            zone.AddZone(zoneTemplate->Activate(surface));
    }
    
    map<Widget*, map<string, vector<Action*>>> widgetActions;
    
    for(auto member : zoneMembers)
        if(Widget* widget = surface->GetWidgetByName(member.widgetName))
            widgetActions[widget][member.modifiers].push_back(TheManager->GetAction(widget, member.actionName, member.params));
    
    for(auto [widget, modifierActions] : widgetActions)
        for(auto [modifier, actions] : modifierActions)
        {
            widget->Activate(modifier, actions);
            zone.AddWidget(widget, modifier);
        }
    
    return zone;
}

Zone ZoneTemplate::Activate(ControlSurface*  surface, int channelNum, Navigator* navigator)
{
    Zone zone;

    map<Widget*, map<string, vector<Action*>>> widgetActions;

    if(navigator != nullptr)
        for(auto member : zoneMembers)
        {
            member.widgetName = regex_replace(member.widgetName, regex("[|]"), to_string(channelNum + 1));
            
            if(Widget* widget = surface->GetWidgetByName(member.widgetName))
                widgetActions[widget][member.modifiers].push_back(TheManager->GetAction(widget, member.actionName, member.params, navigator));
        }
    
    for(auto [widget, modifierActions] : widgetActions)
        for(auto [modifier, actions] : modifierActions)
        {
            widget->Activate(modifier, actions);
            zone.AddWidget(widget, modifier);
        }

    return zone;
}

Zone ZoneTemplate::Activate(ControlSurface*  surface, Navigator* navigator, int slotindex)
{
    Zone zone;

    return zone;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    surface_->SendMidiMessage(this, midiMessage);
}

void Midi_FeedbackProcessor::SendMidiMessage(int first, int second, int third)
{
    if(mustForce_ || first != lastMessageSent_->midi_message[0] || second != lastMessageSent_->midi_message[1] || third != lastMessageSent_->midi_message[2])
    {
        ForceMidiMessage(first, second, third);
    }
    else if(shouldRefresh_ && DAW::GetCurrentNumberOfMilliseconds() > lastRefreshed_ + refreshInterval_)
    {
        lastRefreshed_ = DAW::GetCurrentNumberOfMilliseconds();
        ForceMidiMessage(first, second, third);
    }
}

void Midi_FeedbackProcessor::ForceMidiMessage(int first, int second, int third)
{
    lastMessageSent_->midi_message[0] = first;
    lastMessageSent_->midi_message[1] = second;
    lastMessageSent_->midi_message[2] = third;
    surface_->SendMidiMessage(this, first, second, third);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_FeedbackProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSC_FeedbackProcessor::UpdateValue(double value)
{
    if(lastDoubleValue_ != value)
        ForceValue(value);
}

void OSC_FeedbackProcessor::UpdateValue(int param, double value)
{
    if(lastDoubleValue_ != value)
        ForceValue(value);
}

void OSC_FeedbackProcessor::UpdateValue(string value)
{
    if(lastStringValue_ != value)
        ForceValue(value);
}

void OSC_FeedbackProcessor::ForceValue(double value)
{
    lastDoubleValue_ = value;
    surface_->SendOSCMessage(this, oscAddress_, value);
}

void OSC_FeedbackProcessor::ForceValue(int param, double value)
{
    lastDoubleValue_ = value;
    surface_->SendOSCMessage(this, oscAddress_, value);
}

void OSC_FeedbackProcessor::ForceValue(string value)
{
    lastStringValue_ = value;
    surface_->SendOSCMessage(this, oscAddress_, value);
}

void OSC_FeedbackProcessor::SilentSetValue(string value)
{
    surface_->SendOSCMessage(this, oscAddress_, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_FeedbackProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////
void EuCon_FeedbackProcessor::UpdateValue(double value)
{
    if(lastDoubleValue_ != value)
        ForceValue(value);
}

void EuCon_FeedbackProcessor::UpdateValue(int param, double value)
{
    if(lastDoubleValue_ != value)
        ForceValue(param, value);
}

void EuCon_FeedbackProcessor::UpdateValue(string value)
{
    if(lastStringValue_ != value)
        ForceValue(value);
}

void EuCon_FeedbackProcessor::ForceValue(double value)
{
    lastDoubleValue_ = value;
    surface_->SendEuConMessage(this, address_, value);
}

void EuCon_FeedbackProcessor::ForceValue(int param, double value)
{
    lastDoubleValue_ = value;
    surface_->SendEuConMessage(this, address_, value, param);
}

void EuCon_FeedbackProcessor::ForceValue(string value)
{
    lastStringValue_ = value;
    surface_->SendEuConMessage(this, address_, value);
}

void EuCon_FeedbackProcessor::SilentSetValue(string value)
{
    surface_->SendEuConMessage(this, address_, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_FeedbackProcessorDB
////////////////////////////////////////////////////////////////////////////////////////////////////////
void EuCon_FeedbackProcessorDB::Clear()
{
    if(lastDoubleValue_ != -100.0)
        ForceClear();
}

void EuCon_FeedbackProcessorDB::ForceClear()
{
    lastDoubleValue_ = -100.0;
    surface_->SendEuConMessage(this, address_, -100.0);
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

void SendsActivationManager::MapSelectedTrackSendsToWidgets(map<string, ZoneOld*> &zones)
{
    for(auto zone : activeSendZones_)
        zone->Deactivate();
    
    activeSendZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetTrackNavigationManager()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    int numTrackSends = DAW::GetTrackNumSends(selectedTrack, 0);
    
    for(int i = 0; i < numSendSlots_; i++)
    {
        string zoneName = "Send" + to_string(i + 1);
        
        if(shouldMapSends_ && zones.count(zoneName) > 0)
        {
            ZoneOld* zone =  zones[zoneName];
            zone->SetIndex(i);
            
            if(i < numTrackSends)
            {
                zone->Activate();
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
        for(auto selectedZone : activeSelectedTrackFXZones_)
        {
            surface_->LoadingZone(selectedZone.zone->GetName());
            selectedZone.zone->Deactivate();
        }
        
        for(auto selectedZone : activeSelectedTrackFXZones_)
            DAW::TrackFX_Show(selectedZone.track, selectedZone.zone->GetIndex(), 2);
        activeSelectedTrackFXZones_.clear();
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
   for(auto activeZone : activeSelectedTrackFXZones_)
       activeZone.zone->Deactivate(activeZone.track, shouldShowFXWindows_);
    
    activeSelectedTrackFXZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetTrackNavigationManager()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;

    for(int i = 0; i < DAW::TrackFX_GetCount(selectedTrack); i++)
    {
        char FXName[BUFSZ];
        
        DAW::TrackFX_GetFXName(selectedTrack, i, FXName, sizeof(FXName));
        
        if(shouldMapSelectedTrackFX_ && surface_->GetZones().count(FXName) > 0 && ! surface_->GetZones()[FXName]->GetHasFocusedFXTrackNavigator())
        {
            ZoneOld* zone = surface_->GetZones()[FXName];
            zone->SetIndex(i);
            
            zone->Activate(selectedTrack, shouldShowFXWindows_);
            activeSelectedTrackFXZones_.push_back(OpenFXWindow(selectedTrack, zone));
        }
    }
}

void FXActivationManager::MapSelectedTrackFXToMenu()
{
    for(auto zone : activeSelectedTrackFXMenuZones_)
        zone->Deactivate();
    
    activeSelectedTrackFXMenuZones_.clear();
    
    for(auto zone : activeSelectedTrackFXMenuFXZones_)
        zone->Deactivate();
    
    activeSelectedTrackFXMenuFXZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetTrackNavigationManager()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    int numTrackFX = DAW::TrackFX_GetCount(selectedTrack);
    
    for(int i = 0; i < numFXlots_; i ++)
    {
        string zoneName = "FXMenu" + to_string(i + 1);
        
        if(shouldMapSelectedTrackFXMenus_ && surface_->GetZones().count(zoneName) > 0)
        {
            ZoneOld* zone =  surface_->GetZones()[zoneName];
            zone->SetIndex(i);
            
            if(i < numTrackFX)
            {
                surface_->LoadingZone(zone->GetName());
                zone->Activate();
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
    MediaTrack* selectedTrack = surface_->GetPage()->GetTrackNavigationManager()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    if(fxIndex >= DAW::TrackFX_GetCount(selectedTrack))
        return;
    
    char FXName[BUFSZ];
    DAW::TrackFX_GetFXName(selectedTrack, fxIndex, FXName, sizeof(FXName));
    
    if(surface_->GetZones().count(FXName) > 0 && ! surface_->GetZones()[FXName]->GetHasFocusedFXTrackNavigator())
    {
        ZoneOld* zone = surface_->GetZones()[FXName];
        zone->SetIndex(fxIndex);
        
        zone->Activate();
        activeSelectedTrackFXMenuFXZones_.push_back(zone);
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
            ZoneOld* zone = surface_->GetZones()[FXName];
            zone->SetIndex(fxIndex);
            
            surface_->LoadingZone(FXName);
            zone->Activate();
            activeFocusedFXZones_.push_back(zone);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlSurface::ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int options) :  CSurfIntegrator_(CSurfIntegrator), page_(page), name_(name), zoneFolder_(zoneFolder), fxActivationManager_(new FXActivationManager(this)), sendsActivationManager_(new SendsActivationManager(this)), numChannels_(numChannels), numSends_(numSends), numFX_(numFX), options_(options)
{
    
    for(int i = 0; i < numChannels; i++)
        navigators_[i] = GetPage()->GetTrackNavigationManager()->AddNavigator();
}

void ControlSurface::InitZones(string zoneFolder)
{
    try
    {
        vector<string> zoneFilesToProcess;
        listZoneFiles(DAW::GetResourcePath() + string("/CSI/Zones/") + zoneFolder + "/", zoneFilesToProcess); // recursively find all the .zon files, starting at zoneFolder
        
        navigators.clear();
        
        for(auto zoneFilename : zoneFilesToProcess)
        {
            ProcessZoneFile(zoneFilename, this);
            ProcessZoneFileOld(zoneFilename, this, widgets_);
        }
    }
    catch (exception &e)
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble parsing Zone folders\n");
        DAW::ShowConsoleMsg(buffer);
    }
}

Navigator* ControlSurface::GetNavigatorForChannel(int channelNum)
{
    if(channelNum < 0)
        return nullptr;
    
    if(navigators_.count(channelNum) > 0)
        return navigators_[channelNum];
    else
        return nullptr;
}

void ControlSurface::SurfaceOutMonitor(Widget* widget, string address, string value)
{
    if(TheManager->GetSurfaceOutMonitor())
    {
        string displayString = "";
        
        //if(widget)
            //displayString = widget->GetCurrentZoneActionDisplay(name_) + " " + address + " " + value + "\n";
        //else
            displayString = "OUT->" + name_ + " " + address + " " + value + "\n";
        
        DAW::ShowConsoleMsg(displayString.c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename, string zoneFolder)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename, this, widgets_);
    InitHardwiredWidgets();
    InitZones(zoneFolder);
    GoHome();
    ForceClearAllWidgets();
    GetPage()->ForceRefreshTimeDisplay();
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
        snprintf(buffer, sizeof(buffer), "IN <- %s %02x  %02x  %02x \n", name_.c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
        DAW::ShowConsoleMsg(buffer);
        
    }
}

void Midi_ControlSurface::SendMidiMessage(Midi_FeedbackProcessor* feedbackProcessor, MIDI_event_ex_t* midiMessage)
{
    if(midiOutput_)
        midiOutput_->SendMsg(midiMessage, -1);
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        string displayString = "";
        
        //f(Widget* widget = feedbackProcessor->GetWidget())
            //displayString = widget->GetCurrentZoneActionDisplay(name_) + " SysEx\n";
        //else
            displayString = "OUT->" + name_ + " SysEx\n";
        
        DAW::ShowConsoleMsg(displayString.c_str());
    }
}

void Midi_ControlSurface::SendMidiMessage(Midi_FeedbackProcessor* feedbackProcessor, int first, int second, int third)
{
    if(midiOutput_)
        midiOutput_->Send(first, second, third, -1);
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        string displayString = "";
        
        //if(Widget* widget = feedbackProcessor->GetWidget())
            //displayString = widget->GetCurrentZoneActionDisplay(name_);
        //else
            displayString = "OUT->" + name_;
        
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "%s  %02x  %02x  %02x \n", displayString.c_str(), first, second, third);
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSC_ControlSurface::InitWidgets(string templateFilename, string zoneFolder)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/OSC/" + templateFilename, this, widgets_);
    
    InitHardwiredWidgets();
    InitZones(zoneFolder);
    GoHome();
    ForceClearAllWidgets();
    GetPage()->ForceRefreshTimeDisplay();
}

void OSC_ControlSurface::ProcessOSCMessage(string message, double value)
{
    if(CSIMessageGeneratorsByOSCMessage_.count(message) > 0)
        CSIMessageGeneratorsByOSCMessage_[message]->ProcessOSCMessage(message, value);
    
    if(TheManager->GetSurfaceInMonitor())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN <- %s %s  %f  \n", name_.c_str(), message.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void OSC_ControlSurface::LoadingZone(string zoneName)
{
    string oscAddress(zoneName);
    oscAddress = regex_replace(oscAddress, regex(BadFileChars), "_");
    oscAddress = "/" + oscAddress;

    if(outSocket_ != nullptr && outSocket_->isOk())
    {
        oscpkt::Message message;
        message.init(oscAddress);
        packetWriter_.init().addMessage(message);
        outSocket_->sendPacket(packetWriter_.packetData(), packetWriter_.packetSize());
    }
    
    if(TheManager->GetSurfaceOutMonitor())
        DAW::ShowConsoleMsg((zoneName + "->" + "LoadingZone---->" + name_ + "\n").c_str());
}

void OSC_ControlSurface::SendOSCMessage(OSC_FeedbackProcessor* feedbackProcessor, string oscAddress, double value)
{
    if(outSocket_ != nullptr && outSocket_->isOk())
    {
        oscpkt::Message message;
        message.init(oscAddress).pushFloat(value);
        packetWriter_.init().addMessage(message);
        outSocket_->sendPacket(packetWriter_.packetData(), packetWriter_.packetSize());
    }
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        if(TheManager->GetSurfaceOutMonitor())
        {
            string displayString = "";
            
            //if(Widget* widget = feedbackProcessor->GetWidget())
                //displayString = widget->GetCurrentZoneActionDisplay(name_) + " " + oscAddress + " " + to_string(value) + "\n";
            //else
                displayString = "OUT->" + name_ + " " + oscAddress + " " + to_string(value) + "\n";
            
            DAW::ShowConsoleMsg(displayString.c_str());
        }
    }
}

void OSC_ControlSurface::SendOSCMessage(OSC_FeedbackProcessor* feedbackProcessor, string oscAddress, string value)
{
    if(outSocket_ != nullptr && outSocket_->isOk())
    {
        oscpkt::Message message;
        message.init(oscAddress).pushStr(value);
        packetWriter_.init().addMessage(message);
        outSocket_->sendPacket(packetWriter_.packetData(), packetWriter_.packetSize());
    }
    
    SurfaceOutMonitor(feedbackProcessor->GetWidget(), oscAddress, value);
    
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
    string address_ = "";
    double value_ = 0;
public:
    Marshalled_Double(EuCon_ControlSurface* surface, string address, double value) : MarshalledFunctionCall(surface), address_(address), value_(value)  { }
    virtual ~Marshalled_Double() {}
    
    virtual void Execute() override { surface_->HandleEuConMessage(address_, value_); }
};

/////////////////////////////////////////////////////////////////////////////
class Marshalled_String : public MarshalledFunctionCall
/////////////////////////////////////////////////////////////////////////////
{
private:
    string address_ = "";
    string  value_ = "";
    
public:
    Marshalled_String(EuCon_ControlSurface* surface, string address, string value) : MarshalledFunctionCall(surface), address_(address), value_(value)  { }
    virtual ~Marshalled_String() {}
    
    virtual void Execute() override { surface_->HandleEuConMessage(address_, value_); }
};

/////////////////////////////////////////////////////////////////////////////
class Marshalled_VisibilityChange : public MarshalledFunctionCall
/////////////////////////////////////////////////////////////////////////////
{
private:
    string groupName_ = "";
    int  channelNumber_ = 0;
    bool isVisible_ = false;
    
public:
    Marshalled_VisibilityChange(EuCon_ControlSurface* surface, string groupName, int channelNumber, bool isVisible) : MarshalledFunctionCall(surface), groupName_(groupName), channelNumber_(channelNumber), isVisible_(isVisible)  { }
    virtual ~Marshalled_VisibilityChange() {}
    
    virtual void Execute() override { surface_->HandleEuConGroupVisibilityChange(groupName_, channelNumber_, isVisible_); }
};

void EuConRequestsInitialization()
{
    if(TheManager)
        TheManager->InitializeEuCon();
}

void InitializeEuConWidgets(vector<CSIWidgetInfo> *assemblyInfoItems)
{
    if(TheManager)
        TheManager->InitializeEuConWidgets(assemblyInfoItems);
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

void HandleEuConGroupVisibilityChange(const char *groupName, int channelNumber, bool isVisible)
{
    if(TheManager)
        TheManager->ReceiveEuConGroupVisibilityChange(string(groupName), channelNumber, isVisible);
}

void HandleEuConGetMeterValues(int id, int iLeg, float& oLevel, float& oPeak, bool& oLegClip)
{
    if(TheManager)
        TheManager->ReceiveEuConGetMeterValues(id, iLeg, oLevel, oPeak, oLegClip);
}

void HandleEuConParamQuery(const char* address, MediaTrack* *track, int *fxSlot, int *fxParamIndex)
{
    if(TheManager)
        TheManager->ReceiveEuConParamQuery(address, track, fxSlot, fxParamIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// EuCon_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
EuCon_ControlSurface::EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int options)
: ControlSurface(CSurfIntegrator, page, name, zoneFolder, numChannels, numSends, numFX, options)
{
    // EuCon takes care of managing navigation, so we just blast everything always
    sendsActivationManager_->SetShouldMapSends(true);
    fxActivationManager_->SetShouldMapSelectedTrackFX(true);
    fxActivationManager_->SetShouldMapSelectedTrackFXMenus(true);
    fxActivationManager_->SetShouldMapFocusedFX(true);
    
    fxActivationManager_->SetShouldShowFXWindows(true);
    
    if( ! plugin_register("API_EuConRequestsInitialization", (void *)::EuConRequestsInitialization))
        LOG::InitializationFailure("EuConRequestsInitialization failed to register");

    if( ! plugin_register("API_InitializeEuConWidgets", (void *)::InitializeEuConWidgets))
        LOG::InitializationFailure("InitializeEuConWidgets failed to register");
   
    if( ! plugin_register("API_HandleEuConMessageWithDouble", (void *)::HandleEuConMessageWithDouble))
        LOG::InitializationFailure("HandleEuConMessageWithDouble failed to register");
    
    if( ! plugin_register("API_HandleEuConMessageWithString", (void *)::HandleEuConMessageWithString))
        LOG::InitializationFailure("HandleEuConMessageWithString failed to register");
    
    if( ! plugin_register("API_HandleEuConGroupVisibilityChange", (void *)::HandleEuConGroupVisibilityChange))
        LOG::InitializationFailure("HandleEuConGroupVisibilityChange failed to register");
    
    if( ! plugin_register("API_HandleEuConGetMeterValues", (void *)::HandleEuConGetMeterValues))
        LOG::InitializationFailure("HandleEuConGetMeterValues failed to register");
    
    if( ! plugin_register("API_HandleEuConParamQuery", (void *)::HandleEuConParamQuery))
        LOG::InitializationFailure("HandleEuConParamQuery failed to register");
    
    InitializeEuCon();
}

void EuCon_ControlSurface::InitializeEuCon()
{
    static void (*InitializeEuConWithParameters)(int numChannels, int numSends, int numFX, int panOptions) = nullptr;

    if(g_reaper_plugin_info && InitializeEuConWithParameters == nullptr)
        InitializeEuConWithParameters = (void (*)(int, int, int, int))g_reaper_plugin_info->GetFunc("InitializeEuConWithParameters");

    if(InitializeEuConWithParameters)
        InitializeEuConWithParameters(numChannels_, numSends_, numFX_, options_);
}

Widget*  EuCon_ControlSurface::InitializeEuConWidget(CSIWidgetInfo &widgetInfo)
{
    if(widgetInfo.name != "")
    {
        Widget* widget = new Widget(this, widgetInfo.name);
        
        if(!widget)
            return nullptr;
        
        if(widgetInfo.control != "")
            new EuCon_CSIMessageGenerator(this, widget, widgetInfo.control);
       
        if(widgetInfo.FB_Processor != "")
        {
            if(widgetInfo.FB_Processor.find("FaderDB") != string::npos)
                widget->AddFeedbackProcessor(new EuCon_FeedbackProcessorDB(this, widget, widgetInfo.FB_Processor));
            else
                widget->AddFeedbackProcessor(new EuCon_FeedbackProcessor(this, widget, widgetInfo.FB_Processor));
        }
        
        return widget;
    }
    
    return nullptr;
}

void EuCon_ControlSurface::InitializeEuConWidgets(vector<CSIWidgetInfo> *widgetInfoItems)
{
    for(auto item : *widgetInfoItems)
    {
        if(Widget* widget = InitializeEuConWidget(item))
        {
            AddWidget(widget);
            
            if(item.channelNumber > 0 && channelGroups_.count(item.channelNumber) < 1 )
                channelGroups_[item.channelNumber] = new WidgetGroup();

            if(item.group == "General")
                generalWidgets_.push_back(widget);
            
            if(channelGroups_.count(item.channelNumber) > 0)
            {
                if(item.group == "Channel")
                    channelGroups_[item.channelNumber]->AddWidget(widget);
                else
                    channelGroups_[item.channelNumber]->AddWidgetToSubgroup(item.group, widget);
            }
        }
    }
    
    InitHardwiredWidgets();
    InitZones(zoneFolder_);
    GoHome();
    ForceClearAllWidgets();
    GetPage()->ForceRefreshTimeDisplay();
}

void EuCon_ControlSurface::SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, double value)
{
    static void (*HandleReaperMessageWthDouble)(const char *, double) = nullptr;
    
    if(g_reaper_plugin_info && HandleReaperMessageWthDouble == nullptr)
        HandleReaperMessageWthDouble = (void (*)(const char *, double))g_reaper_plugin_info->GetFunc("HandleReaperMessageWthDouble");
    
    if(HandleReaperMessageWthDouble)
        HandleReaperMessageWthDouble(address.c_str(), value);
    
    SurfaceOutMonitor(feedbackProcessor->GetWidget(), address, to_string(value));
}

void EuCon_ControlSurface::SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, double value, int param)
{
    static void (*HandleReaperMessageWthParam)(const char *, double, int) = nullptr;
    
    if(g_reaper_plugin_info && HandleReaperMessageWthParam == nullptr)
        HandleReaperMessageWthParam = (void (*)(const char *, double, int))g_reaper_plugin_info->GetFunc("HandleReaperMessageWthParam");
    
    if(HandleReaperMessageWthParam)
        HandleReaperMessageWthParam(address.c_str(), value, param);
    
    SurfaceOutMonitor(feedbackProcessor->GetWidget(), address, to_string(value));
}

void EuCon_ControlSurface::SendEuConMessage(EuCon_FeedbackProcessor* feedbackProcessor, string address, string value)
{
    if(address.find("Pan_Display") != string::npos
       || address.find("Width_Display") != string::npos
       || address.find("PanL_Display") != string::npos
       || address.find("PanR_Display") != string::npos)
    {
        return; // GAW -- Hack to prevent overwrite of Pan, Width, etc. labels
    }
    
    static void (*HandleReaperMessageWthString)(const char *, const char *) = nullptr;
    
    if(g_reaper_plugin_info && HandleReaperMessageWthString == nullptr)
        HandleReaperMessageWthString = (void (*)(const char *, const char *))g_reaper_plugin_info->GetFunc("HandleReaperMessageWithString");
    
    if(HandleReaperMessageWthString)
        HandleReaperMessageWthString(address.c_str(), value.c_str());
    
    if(TheManager->GetSurfaceOutMonitor())
    {
        string displayString = "";
        
        //if(Widget* widget = feedbackProcessor->GetWidget())
            //displayString = widget->GetCurrentZoneActionDisplay(name_) + " " + address + " " + value + "\n";
        //else
            displayString = "OUT-> " + name_ + " " + address + " " + value + "\n";
        
        DAW::ShowConsoleMsg(displayString.c_str());
    }
}

void EuCon_ControlSurface::SendEuConMessage(string address, string value)
{
    static void (*HandleReaperMessageWthString)(const char *, const char *) = nullptr;
    
    if(g_reaper_plugin_info && HandleReaperMessageWthString == nullptr)
        HandleReaperMessageWthString = (void (*)(const char *, const char *))g_reaper_plugin_info->GetFunc("HandleReaperMessageWithString");
    
    if(HandleReaperMessageWthString)
        HandleReaperMessageWthString(address.c_str(), value.c_str());
}

void EuCon_ControlSurface::ReceiveEuConGetMeterValues(int id, int iLeg, float& oLevel, float& oPeak, bool& oLegClip)
{
    if(MediaTrack* track = GetPage()->GetTrackNavigationManager()->GetTrackFromChannel(id))
    {
        float left = VAL2DB(DAW::Track_GetPeakInfo(track, 0));
        float right = VAL2DB(DAW::Track_GetPeakInfo(track, 1));
        
        oLevel = (left + right) / 2.0;
       
        float max = left > right ? left : right;
        
        if(peakInfo_.count(id) > 0 && peakInfo_[id].peakValue < max)
        {
            peakInfo_[id].timePeakSet  = DAW::GetCurrentNumberOfMilliseconds();
            peakInfo_[id].peakValue = max;
            if(max > 0.0)
                peakInfo_[id].isClipping = true;
        }
        
        if(peakInfo_.count(id) < 1)
        {
            peakInfo_[id].timePeakSet  = DAW::GetCurrentNumberOfMilliseconds();
            peakInfo_[id].peakValue = max;
            if(max > 0.0)
                peakInfo_[id].isClipping = true;
        }
        
        if(peakInfo_.count(id) > 0 && (DAW::GetCurrentNumberOfMilliseconds() - peakInfo_[id].timePeakSet > 2000))
        {
            peakInfo_[id].timePeakSet  = DAW::GetCurrentNumberOfMilliseconds();
            peakInfo_[id].peakValue = max;
            peakInfo_[id].isClipping = false;
        }
        
        oPeak = peakInfo_[id].peakValue;
        oLegClip = peakInfo_[id].isClipping;
    }
    else
    {
        oLevel = -144.0;
        oPeak = -144.0;
        oLegClip = false;
    }
}

void EuCon_ControlSurface::ReceiveEuConParamQuery(const char* address, MediaTrack* *track, int *fxSlot, int *fxParamIndex)
{
    string name(address);
    
    if(widgetsByName_.count(name) > 0)
    {
        Widget* widget =  widgetsByName_[name];
        
        if((*track = widget->GetTrack()) != nullptr)
        {
            *fxSlot = widget->GetSlotIndex();
            *fxParamIndex = widget->GetParamIndex();
        }
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

void EuCon_ControlSurface::ReceiveEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible)
{
    mutex_.Enter();
    workQueue_.push_front(new Marshalled_VisibilityChange(this, groupName, channelNumber, isVisible));
    mutex_.Leave();
}

void EuCon_ControlSurface::HandleEuConGroupVisibilityChange(string groupName, int channelNumber, bool isVisible)
{
    if(groupName == "FX")
    {
        
        if(isVisible && widgetsByName_.count("OnEuConFXAreaGainedFocus") > 0)
        {
            isEuConFXAreaFocused_ = true;
            widgetsByName_["OnEuConFXAreaGainedFocus"]->DoAction(1.0);
        }
        
        if( ! isVisible && widgetsByName_.count("OnEuConFXAreaLostFocus") > 0)
        {
            isEuConFXAreaFocused_ = false;
            widgetsByName_["OnEuConFXAreaLostFocus"]->DoAction(1.0);
        }
    }
    
    if(groupName == "FX")
        for(auto [channel, group] : channelGroups_)
            group->SetIsVisible("FX", isVisible);
    
    if(groupName == "Pan")
        for(auto [channel, group] : channelGroups_)
            group->SetIsVisible("Pan", isVisible);
    
    else if(groupName == "Send")
        for(auto [channel, group] : channelGroups_)
            group->SetIsVisible("Send", isVisible);
    
    else if(groupName == "Channel" && channelGroups_.count(channelNumber) > 0)
        channelGroups_[channelNumber]->SetIsVisible(isVisible);
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
        snprintf(buffer, sizeof(buffer), "IN <- %s %s  %f  \n", name_.c_str(), address.c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

void EuCon_ControlSurface::HandleEuConMessage(string address, string value)
{
    // GAW TBD
}

void EuCon_ControlSurface::UpdateTimeDisplay()
{
    double playPosition = (GetPlayState() & 1 ) ? GetPlayPosition() : GetCursorPosition();
    
    if(previousPP != playPosition) // GAW :) Yeah I know shouldn't compare FP values, but the worst you get is an extra upadate or two, meh.
    {
        previousPP = playPosition;

        int *timeModePtr = TheManager->GetTimeMode2Ptr(); // transport
        
        int timeMode = 0;
        
        if (timeModePtr && (*timeModePtr) >= 0)
            timeMode = *timeModePtr;
        else
        {
            timeModePtr = TheManager->GetTimeModePtr(); // ruler
            
            if (timeModePtr)
                timeMode = *timeModePtr;
        }
        
        char samplesBuf[64];
        memset(samplesBuf, 0, sizeof(samplesBuf));
        char measuresBuf[64];
        memset(measuresBuf, 0, sizeof(measuresBuf));
        char chronoBuf[64];
        memset(chronoBuf, 0, sizeof(chronoBuf));

        if(timeMode == 4)  // Samples
        {
            format_timestr_pos(playPosition, samplesBuf, sizeof(samplesBuf), timeMode);
        }
        
        if(timeMode == 1 || timeMode == 2)  // Bars/Beats/Ticks
        {
            int num_measures = 0;
            double beats = TimeMap2_timeToBeats(NULL, playPosition, &num_measures, NULL, NULL, NULL) + 0.000000000001;
            double nbeats = floor(beats);
            beats -= nbeats;
            format_timestr_pos(playPosition, measuresBuf, sizeof(measuresBuf), 2);
        }
        
        if(timeMode == 0 || timeMode == 1 || timeMode == 3  ||  timeMode == 5)  // Hours/Minutes/Seconds/Frames
        {
            double *timeOffsetPtr = TheManager->GetTimeOffsPtr();
            if (timeOffsetPtr)
                playPosition += (*timeOffsetPtr);
            format_timestr_pos(playPosition, chronoBuf, sizeof(chronoBuf), timeMode == 1 ? 0 : timeMode);
        }
        
        switch(timeMode)
        {
            case 0: // Hours/Minutes/Seconds
            case 3: // Seconds
            case 5: // Hours/Minutes/Seconds/Frames
                SendEuConMessage("PrimaryTimeDisplay", chronoBuf);
                SendEuConMessage("SecondaryTimeDisplay", "");
                break;
                
            case 1:
                SendEuConMessage("PrimaryTimeDisplay", measuresBuf);
                SendEuConMessage("SecondaryTimeDisplay", chronoBuf);
                break;
                
            case 2:
                SendEuConMessage("PrimaryTimeDisplay", measuresBuf);
                SendEuConMessage("SecondaryTimeDisplay", "");
                break;
                
            case 4:
                SendEuConMessage("PrimaryTimeDisplay", samplesBuf);
                SendEuConMessage("SecondaryTimeDisplay", "");
                break;
        }
    }
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

static vector<ZoneOld*> zonesInThisFile;

static string newZoneFilename = "";
static string newZoneName = "";
static string newZoneAlias = "";

// Modifiers
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

static vector<ZoneOld*> GetAvailableZones(int zoneIndex)
{
    vector<ZoneOld*> availableZones;
    
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

static vector<ZoneOld*> GetAvailableZones()
{
    int zoneIndex = (int)SendMessage(GetDlgItem(hwndLearn, IDC_LIST_Zones), LB_GETCURSEL, 0, 0);
    
    return GetAvailableZones(zoneIndex);
}

static void UpdateCheckBoxes()
{
    if(currentPage != nullptr)
    {
        if (IsDlgButtonChecked(hwndLearn, IDC_CHECK_Shift) != currentPage->GetShift())
        {
            if(currentPage->GetShift())
                CheckDlgButton(hwndLearn, IDC_CHECK_Shift, BST_CHECKED);
            else
                CheckDlgButton(hwndLearn, IDC_CHECK_Shift, BST_UNCHECKED);
        }
        
        if (IsDlgButtonChecked(hwndLearn, IDC_CHECK_Option) != currentPage->GetOption())
        {
            if(currentPage->GetOption())
                CheckDlgButton(hwndLearn, IDC_CHECK_Option, BST_CHECKED);
            else
                CheckDlgButton(hwndLearn, IDC_CHECK_Option, BST_UNCHECKED);
        }
        
        if (IsDlgButtonChecked(hwndLearn, IDC_CHECK_Control) != currentPage->GetControl())
        {
            if(currentPage->GetControl())
                CheckDlgButton(hwndLearn, IDC_CHECK_Control, BST_CHECKED);
            else
                CheckDlgButton(hwndLearn, IDC_CHECK_Control, BST_UNCHECKED);
        }
        
        if (IsDlgButtonChecked(hwndLearn, IDC_CHECK_Alt) != currentPage->GetAlt())
        {
            if(currentPage->GetAlt())
                CheckDlgButton(hwndLearn, IDC_CHECK_Alt, BST_CHECKED);
            else
                CheckDlgButton(hwndLearn, IDC_CHECK_Alt, BST_UNCHECKED);
        }
    }
}

static void UpdateActionName()
{
    if(currentPage != nullptr)
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = currentPage->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char nameBuf[512];
                DAW::TrackFX_GetParamName(track, fxSlotNum, fxParamNum, nameBuf, sizeof(nameBuf));
                
                SetDlgItemText(hwndLearn, IDC_EDIT_ActionName, nameBuf);
            }
        }
        else
            SetDlgItemText(hwndLearn, IDC_EDIT_ActionName, "");
    }
    else
        SetDlgItemText(hwndLearn, IDC_EDIT_ActionName, "");
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

static int FillZones(ZoneOld* zone)
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

static void FillSubZones(ZoneOld* zone, int zoneIndex)
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
    
    //Action Names
    vector<string> actionNames = TheManager->GetActionNames();
     
    for(int i = 0; i < actionNames.size(); i++)
    {
        string name = actionNames[i];
        
        SendDlgItemMessage(hwndLearn, IDC_LIST_ActionNames, LB_ADDSTRING, 0, (LPARAM)name.c_str());
        
        if(name != "FXParam" && currentAction->GetName() == name)
        {
            actionNameWasSelectedBySurface = true;
            SendMessage(GetDlgItem(hwndLearn, IDC_LIST_ActionNames), LB_SETCURSEL, i, 0);
        }

    }
     */
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
                                    ZoneOld* enclosingZone = currentSurface->GetZones()[zoneName];
                                    
                                    if(currentSurface->GetZones().count(includedZoneName) > 0)
                                    {
                                        ZoneOld* includedZone = currentSurface->GetZones()[includedZoneName];
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
            //EnableButtons();

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
            ZoneOld* zone = nullptr;
            
            int zoneIndex = FillZones(zone);
            
            FillSubZones(zone, zoneIndex);
            
            //SetDlgItemText(hwndDlg, IDC_EDIT_ActionName, currentAction->GetName().c_str());
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
                            
                            actionLineItem.isShift =  currentPage != nullptr ? currentPage->GetShift() : false;
                            actionLineItem.isOption = currentPage != nullptr ? currentPage->GetOption() : false;
                            actionLineItem.isControl = currentPage != nullptr ? currentPage->GetControl() : false;
                            actionLineItem.isAlt = currentPage != nullptr ? currentPage->GetAlt() : false;
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
                            
                            if(actionLineItem.actionName == "FXParam")
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
                            ZoneOld* zoneToDelete = zonesInThisFile[index];
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
                                ZoneOld* zone = zonesInThisFile[zoneIndex];
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
                    if(currentPage != nullptr)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Shift))
                            currentPage->SetShift(true);
                        else
                            currentPage->SetShift(false);
                    }
                    
                    break;
                }
                    
                case IDC_CHECK_Option:
                {
                    if(currentPage != nullptr)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Option))
                            currentPage->SetOption(true);
                        else
                            currentPage->SetOption(false);
                    }

                    break;
                }
                    
                case IDC_CHECK_Control:
                {
                    if(currentPage != nullptr)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Control))
                            currentPage->SetControl(true);
                        else
                            currentPage->SetControl(false);
                    }

                    break;
                }
                    
                case IDC_CHECK_Alt:
                {
                    if(currentPage != nullptr)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_Alt))
                            currentPage->SetAlt(true);
                        else
                            currentPage->SetAlt(false);
                    }

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
    
    //if(currentWidget != nullptr && currentWidgetActionManager != nullptr && currentAction != nullptr)
        //SendMessage(hwndLearn, WM_USER+1025, 0, 0);
}

void Page::UpdateEditModeWindow()
{
    UpdateCheckBoxes();
    UpdateActionName();
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
