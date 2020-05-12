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
        navigators[channelNum] = surface->GetPage()->GetTrackNavigationManager()->AddNavigator();
    
    return navigators[channelNum];
}

static void BuildIncludedZoneOld(string includedZoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone);

static map<string, vector<vector<string>>> zoneTemplates;
static map<string, vector<vector<string>>> zoneDefinitions;

static void BuildZoneOld(vector<vector<string>> &zoneLines, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone, int channelNum)
{
    const string FXGainReductionMeter = "FXGainReductionMeter"; // GAW TBD - don't forget to re-implement this

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
            
            zone = new Zone(navigator, surface, tokens[1], filePath, tokens.size() > 2 ? tokens[2] : tokens[1]); // tokens[2] == alias, if provided, otherwise just use name (tokens[1])

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

static void BuildExpandedZonesOld(string zoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone)
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

static void BuildIncludedZoneOld(string includedZoneName, string filePath, ControlSurface* surface, vector<Widget*> &widgets, Zone* parentZone)
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
                    surface->AddZoneTemplate(ZoneTemplate(navigatorName, zoneName, zoneAlias, filePath, companionZones, includedZones, zoneMembers));
                
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
                   
                    zoneMembers.push_back(Wzat(widgetName, actionName, params, isModifier, isPressRelease, isTrackTouch, isTrackRotaryTouch, isInverted, shouldToggle, isDelayed, delayAmount));
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
void Manager::InitActionDictionary()
{
    actions_["NoAction"] =                          [](string name, Widget* widget, vector<string> params) { return NoAction(name, widget, params); };
    actions_["Reaper"] =                            [](string name, Widget* widget, vector<string> params) { return ReaperAction(name, widget, params); };
    actions_["TrackVolume"] =                       [](string name, Widget* widget, vector<string> params) { return TrackVolume(name, widget, params); };
    actions_["SoftTakeover7BitTrackVolume"] =       [](string name, Widget* widget, vector<string> params) { return SoftTakeover7BitTrackVolume(name, widget, params); };
    actions_["SoftTakeover14BitTrackVolume"] =      [](string name, Widget* widget, vector<string> params) { return SoftTakeover14BitTrackVolume(name, widget, params); };
    actions_["TrackVolumeDB"] =                     [](string name, Widget* widget, vector<string> params) { return TrackVolumeDB(name, widget, params); };
    actions_["TrackToggleVCASpill"] =               [](string name, Widget* widget, vector<string> params) { return TrackToggleVCASpill(name, widget, params); };
    actions_["TrackSelect"] =                       [](string name, Widget* widget, vector<string> params) { return TrackSelect(name, widget, params); };
    actions_["TrackUniqueSelect"] =                 [](string name, Widget* widget, vector<string> params) { return TrackUniqueSelect(name, widget, params); };
    actions_["TrackRangeSelect"] =                  [](string name, Widget* widget, vector<string> params) { return TrackRangeSelect(name, widget, params); };
    actions_["TrackRecordArm"] =                    [](string name, Widget* widget, vector<string> params) { return TrackRecordArm(name, widget, params); };
    actions_["TrackMute"] =                         [](string name, Widget* widget, vector<string> params) { return TrackMute(name, widget, params); };
    actions_["TrackSolo"] =                         [](string name, Widget* widget, vector<string> params) { return TrackSolo(name, widget, params); };
    actions_["TrackTouch"] =                        [](string name, Widget* widget, vector<string> params) { return SetFaderTouch(name, widget, params); };
    actions_["RotaryTouch"] =                       [](string name, Widget* widget, vector<string> params) { return SetRotaryTouch(name, widget, params); };
    actions_["TrackPan"] =                          [](string name, Widget* widget, vector<string> params) { return TrackPan(name, widget, params); };
    actions_["TrackPanPercent"] =                   [](string name, Widget* widget, vector<string> params) { return TrackPanPercent(name, widget, params); };
    actions_["TrackPanWidth"] =                     [](string name, Widget* widget, vector<string> params) { return TrackPanWidth(name, widget, params); };
    actions_["TrackPanWidthPercent"] =              [](string name, Widget* widget, vector<string> params) { return TrackPanWidthPercent(name, widget, params); };
    actions_["TrackPanLPercent"] =                  [](string name, Widget* widget, vector<string> params) { return TrackPanLPercent(name, widget, params); };
    actions_["TrackPanRPercent"] =                  [](string name, Widget* widget, vector<string> params) { return TrackPanRPercent(name, widget, params); };
    actions_["FixedTextDisplay"] =                  [](string name, Widget* widget, vector<string> params) { return FixedTextDisplay(name, widget, params); };
    actions_["FixedRGBColourDisplay"] =             [](string name, Widget* widget, vector<string> params) { return FixedRGBColourDisplay(name, widget, params); };
    actions_["TrackNameDisplay"] =                  [](string name, Widget* widget, vector<string> params) { return TrackNameDisplay(name, widget, params); };
    actions_["TrackVolumeDisplay"] =                [](string name, Widget* widget, vector<string> params) { return TrackVolumeDisplay(name, widget, params); };
    actions_["TrackPanDisplay"] =                   [](string name, Widget* widget, vector<string> params) { return TrackPanDisplay(name, widget, params); };
    actions_["TrackPanWidthDisplay"] =              [](string name, Widget* widget, vector<string> params) { return TrackPanWidthDisplay(name, widget, params); };
    actions_["TrackOutputMeter"] =                  [](string name, Widget* widget, vector<string> params) { return TrackOutputMeter(name, widget, params); };
    actions_["TrackOutputMeterAverageLR"] =         [](string name, Widget* widget, vector<string> params) { return TrackOutputMeterAverageLR(name, widget, params); };
    actions_["TrackOutputMeterMaxPeakLR"] =         [](string name, Widget* widget, vector<string> params) { return TrackOutputMeterMaxPeakLR(name, widget, params); };
    actions_["SelectTrackRelative"] =               [](string name, Widget* widget, vector<string> params) { return SelectTrackRelative(name, widget, params); };
    actions_["TrackAutoMode"] =                     [](string name, Widget* widget, vector<string> params) { return TrackAutoMode(name, widget, params); };
    actions_["TimeDisplay"] =                       [](string name, Widget* widget, vector<string> params) { return TimeDisplay(name, widget, params); };
    actions_["EuConTimeDisplay"] =                  [](string name, Widget* widget, vector<string> params) { return EuConTimeDisplay(name, widget, params); };
    actions_["Rewind"] =                            [](string name, Widget* widget, vector<string> params) { return Rewind(name, widget, params); };
    actions_["FastForward"] =                       [](string name, Widget* widget, vector<string> params) { return FastForward(name, widget, params); };
    actions_["Play"] =                              [](string name, Widget* widget, vector<string> params) { return Play(name, widget, params); };
    actions_["Stop"] =                              [](string name, Widget* widget, vector<string> params) { return Stop(name, widget, params); };
    actions_["Record"] =                            [](string name, Widget* widget, vector<string> params) { return Record(name, widget, params); };
    actions_["CycleTimeline"] =                     [](string name, Widget* widget, vector<string> params) { return CycleTimeline(name, widget, params); };
    actions_["SetShowFXWindows"] =                  [](string name, Widget* widget, vector<string> params) { return SetShowFXWindows(name, widget, params); };
    actions_["ToggleScrollLink"] =                  [](string name, Widget* widget, vector<string> params) { return ToggleScrollLink(name, widget, params); };
    actions_["ForceScrollLink"] =                   [](string name, Widget* widget, vector<string> params) { return ForceScrollLink(name, widget, params); };
    actions_["ToggleVCAMode"] =                     [](string name, Widget* widget, vector<string> params) { return ToggleVCAMode(name, widget, params); };
    actions_["CycleTimeDisplayModes"] =             [](string name, Widget* widget, vector<string> params) { return CycleTimeDisplayModes(name, widget, params); };
    actions_["NextPage"] =                          [](string name, Widget* widget, vector<string> params) { return GoNextPage(name, widget, params); };
    actions_["GoPage"] =                            [](string name, Widget* widget, vector<string> params) { return  GoPage(name, widget, params); };
    actions_["GoZone"] =                            [](string name, Widget* widget, vector<string> params) { return GoZone(name, widget, params); };
    actions_["TrackBank"] =                         [](string name, Widget* widget, vector<string> params) { return TrackBank(name, widget, params); };
    actions_["ClearAllSolo"] =                      [](string name, Widget* widget, vector<string> params) { return ClearAllSolo(name, widget, params); };
    actions_["Shift"] =                             [](string name, Widget* widget, vector<string> params) { return SetShift(name, widget, params); };
    actions_["Option"] =                            [](string name, Widget* widget, vector<string> params) { return SetOption(name, widget, params); };
    actions_["Control"] =                           [](string name, Widget* widget, vector<string> params) { return SetControl(name, widget, params); };
    actions_["Alt"] =                               [](string name, Widget* widget, vector<string> params) { return SetAlt(name, widget, params); };
    actions_["TogglePin"] =                         [](string name, Widget* widget, vector<string> params) { return TogglePin(name, widget, params); };
    actions_["ToggleLearnMode"] =                   [](string name, Widget* widget, vector<string> params) { return ToggleLearnMode(name, widget, params); };
    actions_["ToggleMapSelectedTrackSends"] =       [](string name, Widget* widget, vector<string> params) { return ToggleMapSelectedTrackSends(name, widget, params); };
    actions_["MapSelectedTrackSendsToWidgets"] =    [](string name, Widget* widget, vector<string> params) { return MapSelectedTrackSendsToWidgets(name, widget, params); };
    actions_["ToggleMapSelectedTrackFX"] =          [](string name, Widget* widget, vector<string> params) { return ToggleMapSelectedTrackFX(name, widget, params); };
    actions_["MapSelectedTrackFXToWidgets"] =       [](string name, Widget* widget, vector<string> params) { return MapSelectedTrackFXToWidgets(name, widget, params); };
    actions_["ToggleMapSelectedTrackFXMenu"] =      [](string name, Widget* widget, vector<string> params) { return ToggleMapSelectedTrackFXMenu(name, widget, params); };
    actions_["MapSelectedTrackFXToMenu"] =          [](string name, Widget* widget, vector<string> params) { return MapSelectedTrackFXToMenu(name, widget, params); };
    actions_["ToggleMapFocusedFX"] =                [](string name, Widget* widget, vector<string> params) { return ToggleMapFocusedFX(name, widget, params); };
    actions_["MapFocusedFXToWidgets"] =             [](string name, Widget* widget, vector<string> params) { return MapFocusedFXToWidgets(name, widget, params); };
    actions_["GoFXSlot"] =                          [](string name, Widget* widget, vector<string> params) { return GoFXSlot(name, widget, params); };
    actions_["CycleTrackAutoMode"] =                [](string name, Widget* widget, vector<string> params) { return CycleTrackAutoMode(name, widget, params); };
    actions_["EuConCycleTrackAutoMode"] =           [](string name, Widget* widget, vector<string> params) { return EuConCycleTrackAutoMode(name, widget, params); };
    actions_["GlobalAutoMode"] =                    [](string name, Widget* widget, vector<string> params) { return GlobalAutoMode(name, widget, params); };
    actions_["FXNameDisplay"] =                     [](string name, Widget* widget, vector<string> params) { return FXNameDisplay(name, widget, params); };
    actions_["FocusedFXParam"] =                    [](string name, Widget* widget, vector<string> params) { return FocusedFXParam(name, widget, params); };
    actions_["FocusedFXParamNameDisplay"] =         [](string name, Widget* widget, vector<string> params) { return FocusedFXParamNameDisplay(name, widget, params); };
    actions_["FocusedFXParamValueDisplay"] =        [](string name, Widget* widget, vector<string> params) { return FocusedFXParamValueDisplay(name, widget, params); };
}
void Manager::InitActionsWithIndexDictionary()
{
    actionsWithIndex_["FXParam"] =                  [](string name, Widget* widget, vector<string> params, int index) { return FXParam(name, widget, params, index); };
    actionsWithIndex_["FXParamRelative"] =          [](string name, Widget* widget, vector<string> params, int index) { return FXParamRelative(name, widget, params, index); };
    actionsWithIndex_["FXParamNameDisplay"] =       [](string name, Widget* widget, vector<string> params, int index) { return FXParamNameDisplay(name, widget, params, index); };
    actionsWithIndex_["FXParamValueDisplay"] =      [](string name, Widget* widget, vector<string> params, int index) { return FXParamValueDisplay(name, widget, params, index); };
    actionsWithIndex_["FXGainReductionMeter"] =     [](string name, Widget* widget, vector<string> params, int index) { return FXGainReductionMeter(name, widget, params, index); };
    actionsWithIndex_["TrackSendVolume"] =          [](string name, Widget* widget, vector<string> params, int index) { return TrackSendVolume(name, widget, params, index); };
    actionsWithIndex_["TrackSendVolumeDB"] =        [](string name, Widget* widget, vector<string> params, int index) { return TrackSendVolumeDB(name, widget, params, index); };
    actionsWithIndex_["TrackSendPan"] =             [](string name, Widget* widget, vector<string> params, int index) { return TrackSendPan(name, widget, params, index); };
    actionsWithIndex_["TrackSendMute"] =            [](string name, Widget* widget, vector<string> params, int index) { return TrackSendMute(name, widget, params, index); };
    actionsWithIndex_["TrackSendInvertPolarity"] =  [](string name, Widget* widget, vector<string> params, int index) { return TrackSendInvertPolarity(name, widget, params, index); };
    actionsWithIndex_["TrackSendPrePost"] =         [](string name, Widget* widget, vector<string> params, int index) { return TrackSendPrePost(name, widget, params, index); };
    actionsWithIndex_["TrackSendNameDisplay"] =     [](string name, Widget* widget, vector<string> params, int index) { return TrackSendNameDisplay(name, widget, params, index); };
    actionsWithIndex_["TrackSendVolumeDisplay"] =   [](string name, Widget* widget, vector<string> params, int index) { return TrackSendVolumeDisplay(name, widget, params, index); };
}

void Manager::InitActionDictionaryOld()
{
    actionsOld_["NoAction"] =                          [](string name, Widget* widget, vector<string> params) { return new NoAction(name, widget, params); };
    actionsOld_["Reaper"] =                            [](string name, Widget* widget, vector<string> params) { return new ReaperAction(name, widget, params); };
    actionsOld_["FXNameDisplay"] =                     [](string name, Widget* widget, vector<string> params) { return new FXNameDisplay(name, widget, params); };
    actionsOld_["FXParam"] =                           [](string name, Widget* widget, vector<string> params) { return new FXParam(name, widget, params); };
    actionsOld_["FXParamRelative"] =                   [](string name, Widget* widget, vector<string> params) { return new FXParamRelative(name, widget, params); };
    actionsOld_["FocusedFXParam"] =                    [](string name, Widget* widget, vector<string> params) { return new FocusedFXParam(name, widget, params); };
    actionsOld_["FXParamNameDisplay"] =                [](string name, Widget* widget, vector<string> params) { return new FXParamNameDisplay(name, widget, params); };
    actionsOld_["FXParamValueDisplay"] =               [](string name, Widget* widget, vector<string> params) { return new FXParamValueDisplay(name, widget, params); };
    actionsOld_["FocusedFXParamNameDisplay"] =         [](string name, Widget* widget, vector<string> params) { return new FocusedFXParamNameDisplay(name, widget, params); };
    actionsOld_["FocusedFXParamValueDisplay"] =        [](string name, Widget* widget, vector<string> params) { return new FocusedFXParamValueDisplay(name, widget, params); };
    actionsOld_["FXGainReductionMeter"] =              [](string name, Widget* widget, vector<string> params) { return new FXGainReductionMeter(name, widget, params); };
    actionsOld_["TrackVolume"] =                       [](string name, Widget* widget, vector<string> params) { return new TrackVolume(name, widget, params); };
    actionsOld_["SoftTakeover7BitTrackVolume"] =       [](string name, Widget* widget, vector<string> params) { return new SoftTakeover7BitTrackVolume(name, widget, params); };
    actionsOld_["SoftTakeover14BitTrackVolume"] =      [](string name, Widget* widget, vector<string> params) { return new SoftTakeover14BitTrackVolume(name, widget, params); };
    actionsOld_["TrackVolumeDB"] =                     [](string name, Widget* widget, vector<string> params) { return new TrackVolumeDB(name, widget, params); };
    actionsOld_["TrackSendVolume"] =                   [](string name, Widget* widget, vector<string> params) { return new TrackSendVolume(name, widget, params); };
    actionsOld_["TrackSendVolumeDB"] =                 [](string name, Widget* widget, vector<string> params) { return new TrackSendVolumeDB(name, widget, params); };
    actionsOld_["TrackSendPan"] =                      [](string name, Widget* widget, vector<string> params) { return new TrackSendPan(name, widget, params); };
    actionsOld_["TrackSendMute"] =                     [](string name, Widget* widget, vector<string> params) { return new TrackSendMute(name, widget, params); };
    actionsOld_["TrackSendInvertPolarity"] =           [](string name, Widget* widget, vector<string> params) { return new TrackSendInvertPolarity(name, widget, params); };
    actionsOld_["TrackSendPrePost"] =                  [](string name, Widget* widget, vector<string> params) { return new TrackSendPrePost(name, widget, params); };
    actionsOld_["TrackPan"] =                          [](string name, Widget* widget, vector<string> params) { return new TrackPan(name, widget, params); };
    actionsOld_["TrackPanPercent"] =                   [](string name, Widget* widget, vector<string> params) { return new TrackPanPercent(name, widget, params); };
    actionsOld_["TrackPanWidth"] =                     [](string name, Widget* widget, vector<string> params) { return new TrackPanWidth(name, widget, params); };
    actionsOld_["TrackPanWidthPercent"] =              [](string name, Widget* widget, vector<string> params) { return new TrackPanWidthPercent(name, widget, params); };
    actionsOld_["TrackPanLPercent"] =                  [](string name, Widget* widget, vector<string> params) { return new TrackPanLPercent(name, widget, params); };
    actionsOld_["TrackPanRPercent"] =                  [](string name, Widget* widget, vector<string> params) { return new TrackPanRPercent(name, widget, params); };
    actionsOld_["FixedTextDisplay"] =                  [](string name, Widget* widget, vector<string> params) { return new FixedTextDisplay(name, widget, params); };
    actionsOld_["FixedRGBColourDisplay"] =             [](string name, Widget* widget, vector<string> params) { return new FixedRGBColourDisplay(name, widget, params); };
    actionsOld_["TrackNameDisplay"] =                  [](string name, Widget* widget, vector<string> params) { return new TrackNameDisplay(name, widget, params); };
    actionsOld_["TrackVolumeDisplay"] =                [](string name, Widget* widget, vector<string> params) { return new TrackVolumeDisplay(name, widget, params); };
    actionsOld_["TrackSendNameDisplay"] =              [](string name, Widget* widget, vector<string> params) { return new TrackSendNameDisplay(name, widget, params); };
    actionsOld_["TrackSendVolumeDisplay"] =            [](string name, Widget* widget, vector<string> params) { return new TrackSendVolumeDisplay(name, widget, params); };
    actionsOld_["TrackPanDisplay"] =                   [](string name, Widget* widget, vector<string> params) { return new TrackPanDisplay(name, widget, params); };
    actionsOld_["TrackPanWidthDisplay"] =              [](string name, Widget* widget, vector<string> params) { return new TrackPanWidthDisplay(name, widget, params); };
    actionsOld_["TimeDisplay"] =                       [](string name, Widget* widget, vector<string> params) { return new TimeDisplay(name, widget, params); };
    actionsOld_["EuConTimeDisplay"] =                  [](string name, Widget* widget, vector<string> params) { return new EuConTimeDisplay(name, widget, params); };
    actionsOld_["Rewind"] =                            [](string name, Widget* widget, vector<string> params) { return new Rewind(name, widget, params); };
    actionsOld_["FastForward"] =                       [](string name, Widget* widget, vector<string> params) { return new FastForward(name, widget, params); };
    actionsOld_["Play"] =                              [](string name, Widget* widget, vector<string> params) { return new Play(name, widget, params); };
    actionsOld_["Stop"] =                              [](string name, Widget* widget, vector<string> params) { return new Stop(name, widget, params); };
    actionsOld_["Record"] =                            [](string name, Widget* widget, vector<string> params) { return new Record(name, widget, params); };
    actionsOld_["TrackToggleVCASpill"] =               [](string name, Widget* widget, vector<string> params) { return new TrackToggleVCASpill(name, widget, params); };
    actionsOld_["TrackSelect"] =                       [](string name, Widget* widget, vector<string> params) { return new TrackSelect(name, widget, params); };
    actionsOld_["TrackUniqueSelect"] =                 [](string name, Widget* widget, vector<string> params) { return new TrackUniqueSelect(name, widget, params); };
    actionsOld_["TrackRangeSelect"] =                  [](string name, Widget* widget, vector<string> params) { return new TrackRangeSelect(name, widget, params); };
    actionsOld_["TrackRecordArm"] =                    [](string name, Widget* widget, vector<string> params) { return new TrackRecordArm(name, widget, params); };
    actionsOld_["TrackMute"] =                         [](string name, Widget* widget, vector<string> params) { return new TrackMute(name, widget, params); };
    actionsOld_["TrackSolo"] =                         [](string name, Widget* widget, vector<string> params) { return new TrackSolo(name, widget, params); };
    actionsOld_["TrackTouch"] =                        [](string name, Widget* widget, vector<string> params) { return new SetFaderTouch(name, widget, params); };
    actionsOld_["RotaryTouch"] =                       [](string name, Widget* widget, vector<string> params) { return new SetRotaryTouch(name, widget, params); };
    actionsOld_["CycleTimeline"] =                     [](string name, Widget* widget, vector<string> params) { return new CycleTimeline(name, widget, params); };
    actionsOld_["TrackOutputMeter"] =                  [](string name, Widget* widget, vector<string> params) { return new TrackOutputMeter(name, widget, params); };
    actionsOld_["TrackOutputMeterAverageLR"] =         [](string name, Widget* widget, vector<string> params) { return new TrackOutputMeterAverageLR(name, widget, params); };
    actionsOld_["TrackOutputMeterMaxPeakLR"] =         [](string name, Widget* widget, vector<string> params) { return new TrackOutputMeterMaxPeakLR(name, widget, params); };
    actionsOld_["SetShowFXWindows"] =                  [](string name, Widget* widget, vector<string> params) { return new SetShowFXWindows(name, widget, params); };
    actionsOld_["ToggleScrollLink"] =                  [](string name, Widget* widget, vector<string> params) { return new ToggleScrollLink(name, widget, params); };
    actionsOld_["ForceScrollLink"] =                   [](string name, Widget* widget, vector<string> params) { return new ForceScrollLink(name, widget, params); };
    actionsOld_["ToggleVCAMode"] =                     [](string name, Widget* widget, vector<string> params) { return new ToggleVCAMode(name, widget, params); };
    actionsOld_["CycleTimeDisplayModes"] =             [](string name, Widget* widget, vector<string> params) { return new CycleTimeDisplayModes(name, widget, params); };
    actionsOld_["NextPage"] =                          [](string name, Widget* widget, vector<string> params) { return new GoNextPage(name, widget, params); };
    actionsOld_["GoPage"] =                            [](string name, Widget* widget, vector<string> params) { return new  GoPage(name, widget, params); };
    actionsOld_["GoZone"] =                            [](string name, Widget* widget, vector<string> params) { return new GoZone(name, widget, params); };
    actionsOld_["SelectTrackRelative"] =               [](string name, Widget* widget, vector<string> params) { return new SelectTrackRelative(name, widget, params); };
    actionsOld_["TrackBank"] =                         [](string name, Widget* widget, vector<string> params) { return new TrackBank(name, widget, params); };
    actionsOld_["ClearAllSolo"] =                      [](string name, Widget* widget, vector<string> params) { return new ClearAllSolo(name, widget, params); };
    actionsOld_["Shift"] =                             [](string name, Widget* widget, vector<string> params) { return new SetShift(name, widget, params); };
    actionsOld_["Option"] =                            [](string name, Widget* widget, vector<string> params) { return new SetOption(name, widget, params); };
    actionsOld_["Control"] =                           [](string name, Widget* widget, vector<string> params) { return new SetControl(name, widget, params); };
    actionsOld_["Alt"] =                               [](string name, Widget* widget, vector<string> params) { return new SetAlt(name, widget, params); };
    actionsOld_["TogglePin"] =                         [](string name, Widget* widget, vector<string> params) { return new TogglePin(name, widget, params); };
    actionsOld_["ToggleLearnMode"] =                   [](string name, Widget* widget, vector<string> params) { return new ToggleLearnMode(name, widget, params); };
    actionsOld_["ToggleMapSelectedTrackSends"] =       [](string name, Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackSends(name, widget, params); };
    actionsOld_["MapSelectedTrackSendsToWidgets"] =    [](string name, Widget* widget, vector<string> params) { return new MapSelectedTrackSendsToWidgets(name, widget, params); };
    actionsOld_["ToggleMapSelectedTrackFX"] =          [](string name, Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackFX(name, widget, params); };
    actionsOld_["MapSelectedTrackFXToWidgets"] =       [](string name, Widget* widget, vector<string> params) { return new MapSelectedTrackFXToWidgets(name, widget, params); };
    actionsOld_["ToggleMapSelectedTrackFXMenu"] =      [](string name, Widget* widget, vector<string> params) { return new ToggleMapSelectedTrackFXMenu(name, widget, params); };
    actionsOld_["MapSelectedTrackFXToMenu"] =          [](string name, Widget* widget, vector<string> params) { return new MapSelectedTrackFXToMenu(name, widget, params); };
    actionsOld_["ToggleMapFocusedFX"] =                [](string name, Widget* widget, vector<string> params) { return new ToggleMapFocusedFX(name, widget, params); };
    actionsOld_["MapFocusedFXToWidgets"] =             [](string name, Widget* widget, vector<string> params) { return new MapFocusedFXToWidgets(name, widget, params); };
    actionsOld_["GoFXSlot"] =                          [](string name, Widget* widget, vector<string> params) { return new GoFXSlot(name, widget, params); };
    actionsOld_["TrackAutoMode"] =                     [](string name, Widget* widget, vector<string> params) { return new TrackAutoMode(name, widget, params); };
    actionsOld_["CycleTrackAutoMode"] =                [](string name, Widget* widget, vector<string> params) { return new CycleTrackAutoMode(name, widget, params); };
    actionsOld_["EuConCycleTrackAutoMode"] =           [](string name, Widget* widget, vector<string> params) { return new EuConCycleTrackAutoMode(name, widget, params); };
    actionsOld_["GlobalAutoMode"] =                    [](string name, Widget* widget, vector<string> params) { return new GlobalAutoMode(name, widget, params); };
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
                        
                        if(tokens[0] == MidiSurfaceToken && tokens.size() == 11)
                            surface = new Midi_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForPort(inPort), GetMidiOutputForPort(outPort));
                        else if(tokens[0] == OSCSurfaceToken && tokens.size() == 12)
                            surface = new OSC_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], GetInputSocketForPort(tokens[1], inPort), GetOutputSocketForAddressAndPort(tokens[1], tokens[11], outPort));
                        else if(tokens[0] == EuConSurfaceToken && tokens.size() == 8)
                            surface = new EuCon_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[6], atoi(tokens[2].c_str()), atoi(tokens[3].c_str()), atoi(tokens[4].c_str()), atoi(tokens[5].c_str()));

                        currentPage->AddSurface(surface);
                        
                        if(tokens[0] == EuConSurfaceToken && tokens.size() == 8)
                        {
                            if(tokens[7] == "UseZoneLink")
                                surface->SetUseZoneLink(true);
                        }
                        else if(tokens.size() == 11 || tokens.size() == 12)
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
void Widget::AddAction(Zone* zone, string modifiers, Action* action)
{
    actionsOld_[zone][modifiers].push_back(action);
    zonesAvailable_[zone->GetName()] = zone;
}

void Widget::AddTrackTouchedAction(Zone* zone, string modifiers, Action* action)
{
    trackTouchedActions_[zone][modifiers].push_back(action);
}

void Widget::AddTrackRotaryTouchedAction(Zone* zone, string modifiers, Action* action)
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

string Widget::GetCurrentZoneActionDisplay(string surfaceName)
{
    string modifiers = surface_->GetPage()->GetModifiers();

    if(activeZone_ != nullptr && actionsOld_[activeZone_].count(modifiers) > 0 && actionsOld_[activeZone_][modifiers].size() > 0)
    {
        string actionName = "";
        
        if(actionsOld_[activeZone_][modifiers][0]->GetName() == "FXParam")
        {
            if(actionsOld_[activeZone_][modifiers][0]->GetDisplayName() == "")
                actionName = "FXParam " + actionsOld_[activeZone_][modifiers][0]->GetParamNumAsString() + " ";
            else
                actionName = actionsOld_[activeZone_][modifiers][0]->GetDisplayName();
        }
        else
            actionName = actionsOld_[activeZone_][modifiers][0]->GetName();
        
        return activeZone_->GetAlias() + "->" + actionName + "---->" + surfaceName + "->" + GetName();
    }
    else
        return GetName();
}

void Widget::RequestUpdate()
{
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

    if(activeZone_ != nullptr)
    {
        if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0)
            for(auto action : actionsOld_[activeZone_][modifiers])
                action->DoAction(value, this);
        else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0)
            for(auto action : actionsOld_[activeZone_][""])
                action->DoAction(value, this);
    }
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
    
    if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0)
        for(auto action : actionsOld_[activeZone_][modifiers])
            action->DoRelativeAction(delta, this);
    else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0)
        for(auto action : actionsOld_[activeZone_][""])
            action->DoRelativeAction(delta, this);
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
    
    if(actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count(modifiers) > 0)
        for(auto action : actionsOld_[activeZone_][modifiers])
            action->DoRelativeAction(accelerationIndex, delta, this);
    else if(modifiers != "" && actionsOld_.count(activeZone_) > 0 && actionsOld_[activeZone_].count("") > 0)
        for(auto action : actionsOld_[activeZone_][""])
            action->DoRelativeAction(accelerationIndex, delta, this);
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
Action::Action(string name, Widget* widget, vector<string> params): name_(name), widget_(widget)
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
    return widget_->GetNavigator()->GetTrack();
}

int Action::GetSlotIndex()
{
    return widget_->GetSlotIndex();
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

void SendsActivationManager::MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones)
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
            Zone* zone =  zones[zoneName];
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
            Zone* zone = surface_->GetZones()[FXName];
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
            Zone* zone =  surface_->GetZones()[zoneName];
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
        Zone* zone = surface_->GetZones()[FXName];
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
            Zone* zone = surface_->GetZones()[FXName];
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

void ControlSurface::SurfaceOutMonitor(Widget* widget, string address, string value)
{
    if(TheManager->GetSurfaceOutMonitor())
    {
        string displayString = "";
        
        if(widget)
            displayString = widget->GetCurrentZoneActionDisplay(name_) + " " + address + " " + value + "\n";
        else
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
        
        if(Widget* widget = feedbackProcessor->GetWidget())
            displayString = widget->GetCurrentZoneActionDisplay(name_) + " SysEx\n";
        else
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
        
        if(Widget* widget = feedbackProcessor->GetWidget())
            displayString = widget->GetCurrentZoneActionDisplay(name_);
        else
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
            
            if(Widget* widget = feedbackProcessor->GetWidget())
                displayString = widget->GetCurrentZoneActionDisplay(name_) + " " + oscAddress + " " + to_string(value) + "\n";
            else
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
EuCon_ControlSurface::EuCon_ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int panOptions)
: ControlSurface(CSurfIntegrator, page, name), numChannels_(numChannels), numSends_(numSends), numFX_(numFX), panOptions_(panOptions)
{
    // EuCon takes care of managing navigation, so we just blast everything always
    sendsActivationManager_->SetShouldMapSends(true);
    fxActivationManager_->SetShouldMapSelectedTrackFX(true);
    fxActivationManager_->SetShouldMapSelectedTrackFXMenus(true);
    fxActivationManager_->SetShouldMapFocusedFX(true);
    
    fxActivationManager_->SetShouldShowFXWindows(true);
    
    zoneFolder_ = zoneFolder;
    
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
        InitializeEuConWithParameters(numChannels_, numSends_, numFX_, panOptions_);
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
        
        if(Widget* widget = feedbackProcessor->GetWidget())
            displayString = widget->GetCurrentZoneActionDisplay(name_) + " " + address + " " + value + "\n";
        else
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

static vector<Zone*> zonesInThisFile;

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
        
        if(name != "FXParam" && currentAction->GetName() == name)
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
