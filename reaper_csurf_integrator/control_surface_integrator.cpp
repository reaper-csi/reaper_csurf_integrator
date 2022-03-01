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

WDL_Mutex WDL_mutex;

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


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ActionTemplate
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string actionName;
    vector<string> params;
    vector<vector<string>> properties;
    bool isFeedbackInverted;
    double holdDelayAmount;
    
    ActionTemplate(string action, vector<string> prams, bool isInverted, double amount) : actionName(action), params(prams), isFeedbackInverted(isInverted), holdDelayAmount(amount) {}
};

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

static void GetWidgetNameAndProperties(string line, string &widgetName, string &modifier, string &touchId, bool &isFeedbackInverted, double &holdDelayAmount, bool &isProperty)
{
    istringstream modified_role(line);
    vector<string> modifier_tokens;
    vector<string> modifierSlots = { "", "", "", "", ""};
    string modifier_token;
    
    while (getline(modified_role, modifier_token, '+'))
        modifier_tokens.push_back(modifier_token);
    
    if(modifier_tokens.size() > 1)
    {
        for(int i = 0; i < modifier_tokens.size() - 1; i++)
        {
            if(modifier_tokens[i].find("Touch") != string::npos)
            {
                touchId = modifier_tokens[i];
                modifierSlots[0] = modifier_tokens[i] + "+";
            }
            else if(modifier_tokens[i] == Shift)
                modifierSlots[1] = Shift + "+";
            else if(modifier_tokens[i] == Option)
                modifierSlots[2] = Option + "+";
            else if(modifier_tokens[i] == Control)
                modifierSlots[3] = Control + "+";
            else if(modifier_tokens[i] == Alt)
                modifierSlots[4] = Alt + "+";

            else if(modifier_tokens[i] == "InvertFB")
                isFeedbackInverted = true;
            else if(modifier_tokens[i] == "Hold")
                holdDelayAmount = 1.0;
            else if(modifier_tokens[i] == "Property")
                isProperty = true;
        }
    }

    widgetName = modifier_tokens[modifier_tokens.size() - 1];
    
    modifier = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3] + modifierSlots[4];
}

static void PreProcessZoneFile(string filePath, ControlSurface* surface)
{
    string zoneName = "";
    string navigatorName = "";
    int lineNumber = 0;
    
    try
    {
        ifstream file(filePath);
        
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
                    zoneName = tokens.size() > 1 ? tokens[1] : "";
                    surface->AddZoneFilename(zoneName, filePath);
                    break;
                }
            }
        }
    }
    catch (exception &e)
    {
        (void)e;  // Removes C4101 warning
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble in %s, around line %d\n", filePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
}

static void ProcessZoneFile(string filePath, ControlSurface* surface)
{
    vector<string> includedZones;
    bool isInIncludedZonesSection = false;
    vector<string> subZones;
    bool isInSubZonesSection = false;

    map<string, string> touchIds;
    
    map<string, map<string, vector<ActionTemplate*>>> widgetActions;
    
    string zoneName = "";
    string zoneAlias = "";
    string navigatorName = "";
    string actionName = "";
    int lineNumber = 0;
    
    ActionTemplate* currentActionTemplate = nullptr;
    
    try
    {
        ifstream file(filePath);
        
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
                    zoneName = tokens.size() > 1 ? tokens[1] : "";
                    zoneAlias = tokens.size() > 2 ? tokens[2] : "";
                }
                else if(tokens[0] == "ZoneEnd" && zoneName != "")
                {
                    currentActionTemplate = nullptr;
                    
                    vector<Navigator*> navigators;
                    
                    NavigationStyle navigationStyle = NavigationStyle::Standard;
                    
                    if(navigatorName == "")
                        navigators.push_back(surface->GetPage()->GetDefaultNavigator());
                    if(navigatorName == "SelectedTrackNavigator")
                        navigators.push_back(surface->GetPage()->GetSelectedTrackNavigator());
                    else if(navigatorName == "FocusedFXNavigator")
                        navigators.push_back(surface->GetPage()->GetFocusedFXNavigator());
                    else if(navigatorName == "MasterTrackNavigator")
                        navigators.push_back(surface->GetPage()->GetMasterTrackNavigator());
                    else if(navigatorName == "TrackNavigator")
                    {
                        for(int i = 0; i < surface->GetNumChannels(); i++)
                            navigators.push_back(surface->GetNavigatorForChannel(i));
                    }
                    else if(navigatorName == "SelectedTrackSendNavigator")
                    {
                        for(int i = 0; i < surface->GetNumSendSlots(); i++)
                            navigators.push_back(surface->GetPage()->GetSelectedTrackNavigator());
                    }
                    else if(navigatorName == "SelectedTrackReceiveNavigator")
                    {
                        for(int i = 0; i < surface->GetNumReceiveSlots(); i++)
                            navigators.push_back(surface->GetPage()->GetSelectedTrackNavigator());
                    }
                    else if(navigatorName == "SelectedTrackFXMenuNavigator")
                    {
                        for(int i = 0; i < surface->GetNumFXSlots(); i++)
                            navigators.push_back(surface->GetPage()->GetSelectedTrackNavigator());
                    }
                    else if(navigatorName == "TrackSendSlotNavigator")
                    {
                        for(int i = 0; i < surface->GetNumChannels(); i++)
                            navigators.push_back(surface->GetNavigatorForChannel(i));
                        
                        navigationStyle = NavigationStyle::SendSlot;
                    }
                    else if(navigatorName == "TrackReceiveSlotNavigator")
                    {
                        for(int i = 0; i < surface->GetNumChannels(); i++)
                            navigators.push_back(surface->GetNavigatorForChannel(i));
                        
                        navigationStyle = NavigationStyle::ReceiveSlot;
                    }
                    else if(navigatorName == "TrackFXMenuSlotNavigator")
                    {
                        for(int i = 0; i < surface->GetNumChannels(); i++)
                            navigators.push_back(surface->GetNavigatorForChannel(i));
                        
                        navigationStyle = NavigationStyle::FXMenuSlot;
                    }
                    else if(navigatorName == "SelectedTrackSendSlotNavigator")
                    {
                        for(int i = 0; i < surface->GetNumSendSlots(); i++)
                        {
                            navigators.push_back(surface->GetPage()->GetSelectedTrackNavigator());
                            navigationStyle = NavigationStyle::SelectedTrackSendSlot;
                        }
                    }
                    else if(navigatorName == "SelectedTrackReceiveSlotNavigator")
                    {
                        for(int i = 0; i < surface->GetNumReceiveSlots(); i++)
                        {
                            navigators.push_back(surface->GetPage()->GetSelectedTrackNavigator());
                            navigationStyle = NavigationStyle::SelectedTrackReceiveSlot;
                        }
                    }

                    for(int i = 0; i < navigators.size(); i++)
                    {
                        string numStr = to_string(i + 1);
                        
                        string newZoneName = zoneName;
                        
                        map<string, string> expandedTouchIds;
                        
                        if(navigators.size() > 1)
                        {
                            newZoneName += numStr;
                        
                            for(auto [key, value] : touchIds)
                            {
                                string expandedKey = regex_replace(key, regex("[|]"), numStr);
                                string expandedValue = regex_replace(value, regex("[|]"), numStr);

                                expandedTouchIds[expandedKey] = expandedValue;
                            }
                        }
                        else
                        {
                            expandedTouchIds = touchIds;
                        }
                        
                        Zone* zone = new Zone(surface, navigators[i], navigationStyle, i, expandedTouchIds, newZoneName, zoneAlias, filePath);
                        
                        for(auto includedZoneName : includedZones)
                        {
                            int numItems = 1;
                            
                            if((       includedZoneName == "Channel"
                                    || includedZoneName == "TrackSendSlot"
                                    || includedZoneName == "TrackReceiveSlot"
                                    || includedZoneName == "TrackFXMenuSlot") && surface->GetNumChannels() > 1)
                                numItems = surface->GetNumChannels();
                            else if(includedZoneName == "SelectedTrackSend" && surface->GetNumSendSlots() > 1)
                                numItems = surface->GetNumSendSlots();
                            else if(includedZoneName == "SelectedTrackReceive" && surface->GetNumReceiveSlots() > 1)
                                numItems = surface->GetNumReceiveSlots();
                            else if(includedZoneName == "SelectedTrackFXMenu" && surface->GetNumFXSlots() > 1)
                                numItems = surface->GetNumFXSlots();
                            
                            for(int j = 0; j < numItems; j++)
                            {
                                string expandedName = includedZoneName;
                                
                                if(numItems > 1)
                                    expandedName = includedZoneName + to_string(j + 1);
                                
                                Zone* includedZone = surface->GetZone(expandedName);
                                
                                if(includedZone)
                                    zone->AddIncludedZone(includedZone);
                            }
                        }
                        
                        for(auto subZoneName : subZones)
                        {
                            Zone* subZone = surface->GetZone(subZoneName);
                            
                            if(subZone)
                                zone->AddSubZone(subZone);
                        }
                        
                        for(auto [widgetName, modifierActions] : widgetActions)
                        {
                            string surfaceWidgetName = widgetName;
                            
                            if(navigators.size() > 1)
                                surfaceWidgetName = regex_replace(surfaceWidgetName, regex("[|]"), to_string(i + 1));
                            
                            Widget* widget = surface->GetWidgetByName(surfaceWidgetName);
                            
                            if(widget == nullptr)
                                continue;
                            
                            if(actionName == Shift || actionName == Option || actionName == Control || actionName == Alt)
                                widget->SetIsModifier();
                            
                            zone->AddWidget(widget);
                            
                            for(auto [modifier, actions] : modifierActions)
                            {
                                for(auto action : actions)
                                {
                                    
                                    
                                    
                                    #ifdef _WIN32
                                    // GAW -- This hack is only needed for Mac OS
                                    #else
                                    // GAW HACK to ensure only SubZone1, SubZone2, SubZone3, etc. get used to trigger GoSubZone
                                    if(action->actionName == "GoSubZone" && widget->GetName().find("SubZone") == string::npos)
                                        continue;
                                    #endif
                                    
                                    
                                    
                                    string actionName = regex_replace(action->actionName, regex("[|]"), numStr);
                                    vector<string> memberParams;
                                    for(int j = 0; j < action->params.size(); j++)
                                        memberParams.push_back(regex_replace(action->params[j], regex("[|]"), numStr));
                                    
                                    ActionContext context = TheManager->GetActionContext(actionName, widget, zone, memberParams, action->properties);
                                                                        
                                    if(action->isFeedbackInverted)
                                        context.SetIsFeedbackInverted();
                                    
                                    if(action->holdDelayAmount != 0.0)
                                        context.SetHoldDelayAmount(action->holdDelayAmount);
                                    
                                    string expandedModifier = regex_replace(modifier, regex("[|]"), numStr);
                                    
                                    zone->AddActionContext(widget, expandedModifier, context);
                                }
                            }
                        }
                        
                        surface->AddZone(zone);
                    }
                    
                    includedZones.clear();
                    subZones.clear();
                    widgetActions.clear();
                    touchIds.clear();
                    
                    break;
                }
                
                else if(   tokens[0] == "TrackNavigator"
                        || tokens[0] == "TrackSendSlotNavigator"
                        || tokens[0] == "TrackReceiveSlotNavigator"
                        || tokens[0] == "TrackFXMenuSlotNavigator"
                        || tokens[0] == "MasterTrackNavigator"
                        || tokens[0] == "FocusedFXNavigator"
                        || tokens[0] == "SelectedTrackNavigator"
                        || tokens[0] == "SelectedTrackSendNavigator"
                        || tokens[0] == "SelectedTrackReceiveNavigator"
                        || tokens[0] == "SelectedTrackFXMenuNavigator"
                        || tokens[0] == "SelectedTrackSendSlotNavigator"
                        || tokens[0] == "SelectedTrackReceiveSlotNavigator")
                    navigatorName = tokens[0];
                
                else if(tokens[0] == "IncludedZones")
                    isInIncludedZonesSection = true;
                
                else if(tokens[0] == "IncludedZonesEnd")
                    isInIncludedZonesSection = false;
                
                else if(tokens.size() == 1 && isInIncludedZonesSection)
                    includedZones.push_back(tokens[0]);
                
                else if(tokens[0] == "SubZones")
                    isInSubZonesSection = true;
                
                else if(tokens[0] == "SubZonesEnd")
                    isInSubZonesSection = false;
                
                else if(tokens.size() == 1 && isInSubZonesSection)
                    subZones.push_back(tokens[0]);
                
                else if(tokens.size() > 1)
                {
                    actionName = tokens[1];
                    
                    string widgetName = "";
                    string modifier = "";
                    string touchId = "";
                    bool isFeedbackInverted = false;
                    double holdDelayAmount = 0.0;
                    bool isProperty = false;
                    
                    GetWidgetNameAndProperties(tokens[0], widgetName, modifier, touchId, isFeedbackInverted, holdDelayAmount, isProperty);
                    
                    if(touchId != "")
                        touchIds[widgetName] = touchId;
                    
                    vector<string> params;
                    for(int i = 1; i < tokens.size(); i++)
                        params.push_back(tokens[i]);
                    
                    if(isProperty)
                    {
                        if(currentActionTemplate != nullptr)
                            currentActionTemplate->properties.push_back(params);
                    }
                    else
                    {
                        currentActionTemplate = new ActionTemplate(actionName, params, isFeedbackInverted, holdDelayAmount);
                        widgetActions[widgetName][modifier].push_back(currentActionTemplate);
                    }
                }
            }
        }
    }
    catch (exception &e)
    {
        (void)e;  // Removes C4101 warning
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

static void ProcessMidiWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  Midi_ControlSurface* surface)
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
        else if(widgetClass == "MFTEncoder" && size > 4)
            new MFT_AcceleratedEncoder_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])), tokenLines[i]);
        else if(widgetClass == "EncoderPlain" && size == 4)
            new EncoderPlain_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "EncoderPlainReverse" && size == 4)
            new EncoderPlainReverse_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        else if(widgetClass == "Touch" && size == 7)
            new Touch_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])), new MIDI_event_ex_t(strToHex(tokenLines[i][4]), strToHex(tokenLines[i][5]), strToHex(tokenLines[i][6])));
        else if(widgetClass == "Toggle" && size == 4)
            new Toggle_Midi_CSIMessageGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));

        // Feedback Processors
        FeedbackProcessor* feedbackProcessor = nullptr;

        if(widgetClass == "FB_TwoState" && size == 7)
        {
            feedbackProcessor = new TwoState_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])), new MIDI_event_ex_t(strToHex(tokenLines[i][4]), strToHex(tokenLines[i][5]), strToHex(tokenLines[i][6])));
        }
        else if(widgetClass == "FB_NovationLaunchpadMiniRGB7Bit" && size == 4)
        {
            feedbackProcessor = new NovationLaunchpadMiniRGB7Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_MFT_RGB" && size == 4)
        {
            feedbackProcessor = new MFT_RGB_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if (widgetClass == "FB_GP_Midi")
        {
            feedbackProcessor = new GP_Midi_FeedbackProcessor(surface, widget, tokenLines[i]);
        }
        else if(widgetClass == "FB_FaderportRGB7Bit" && size == 4)
        {
            feedbackProcessor = new FaderportRGB7Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_Fader14Bit" && size == 4)
        {
            feedbackProcessor = new Fader14Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_Fader7Bit" && size == 4)
        {
            feedbackProcessor = new Fader7Bit_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_Encoder" && size == 4)
        {
            feedbackProcessor = new Encoder_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_VUMeter" && size == 4)
        {
            feedbackProcessor = new VUMeter_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_GainReductionMeter" && size == 4)
        {
            feedbackProcessor = new GainReductionMeter_Midi_FeedbackProcessor(surface, widget, new MIDI_event_ex_t(strToHex(tokenLines[i][1]), strToHex(tokenLines[i][2]), strToHex(tokenLines[i][3])));
        }
        else if(widgetClass == "FB_MCUTimeDisplay" && size == 1)
        {
            feedbackProcessor = new MCU_TimeDisplay_Midi_FeedbackProcessor(surface, widget);
        }
        else if(widgetClass == "FB_QConProXMasterVUMeter" && size == 2)
        {
            feedbackProcessor = new QConProXMasterVUMeter_Midi_FeedbackProcessor(surface, widget, stoi(tokenLines[i][1]));
        }
        else if((widgetClass == "FB_MCUVUMeter" || widgetClass == "FB_MCUXTVUMeter") && size == 2)
        {
            int displayType = widgetClass == "FB_MCUVUMeter" ? 0x14 : 0x15;
            
            feedbackProcessor = new MCUVUMeter_Midi_FeedbackProcessor(surface, widget, displayType, stoi(tokenLines[i][1]));
            
            surface->SetHasMCUMeters(displayType);
        }
        else if(widgetClass == "FB_SCE24_Text" && size == 3)
        {
            feedbackProcessor = new SCE24_Text_Midi_FeedbackProcessor(surface, widget, stoi(tokenLines[i][1]), stoi(tokenLines[i][2]));
        }
        else if(widgetClass == "FB_SCE24_Bar" && size == 3)
        {
            feedbackProcessor = new SCE24_Bar_Midi_FeedbackProcessor(surface, widget, stoi(tokenLines[i][1]), stoi(tokenLines[i][2]));
        }
        else if(widgetClass == "FB_SCE24_OLEDButton" && size == 3)
        {
            feedbackProcessor = new SCE24_OLEDButton_Midi_FeedbackProcessor(surface, widget, strToHex(tokenLines[i][1]), stoi(tokenLines[i][2]));
        }
        else if(widgetClass == "FB_SCE24_LEDButton" && size == 2)
        {
            feedbackProcessor = new SCE24_LEDButton_Midi_FeedbackProcessor(surface, widget, strToHex(tokenLines[i][1]));
        }
        else if(widgetClass == "FB_SCE24_Background" && size == 2)
        {
            feedbackProcessor = new SCE24_Background_Midi_FeedbackProcessor(surface, widget, strToHex(tokenLines[i][1]));
        }
        else if(widgetClass == "FB_SCE24_Ring" && size == 2)
        {
            feedbackProcessor = new SCE24_Ring_Midi_FeedbackProcessor(surface, widget, stoi(tokenLines[i][1]));
        }
        else if((widgetClass == "FB_MCUDisplayUpper" || widgetClass == "FB_MCUDisplayLower" || widgetClass == "FB_MCUXTDisplayUpper" || widgetClass == "FB_MCUXTDisplayLower") && size == 2)
        {
            if(widgetClass == "FB_MCUDisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_MCUDisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_MCUXTDisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x15, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_MCUXTDisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x15, 0x12, stoi(tokenLines[i][1]));
        }
        
        else if((widgetClass == "FB_C4DisplayUpper" || widgetClass == "FB_C4DisplayLower") && size == 3)
        {
            if(widgetClass == "FB_C4DisplayUpper")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x17, stoi(tokenLines[i][1]) + 0x30, stoi(tokenLines[i][2]));
            else if(widgetClass == "FB_C4DisplayLower")
                feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x17, stoi(tokenLines[i][1]) + 0x30, stoi(tokenLines[i][2]));
        }
        
        else if((widgetClass == "FB_FP8Display" || widgetClass == "FB_FP16Display"
                 || widgetClass == "FB_FP8DisplayUpper" || widgetClass == "FB_FP16DisplayUpper"
                 || widgetClass == "FB_FP8DisplayUpperMiddle" || widgetClass == "FB_FP16DisplayUpperMiddle"
                 || widgetClass == "FB_FP8DisplayLowerMiddle" || widgetClass == "FB_FP16DisplayLowerMiddle"
                 || widgetClass == "FB_FP8DisplayLower" || widgetClass == "FB_FP16DisplayLower") && size == 2)
        {
            if(widgetClass == "FB_FP8Display" || widgetClass == "FB_FP8DisplayUpper")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x02, stoi(tokenLines[i][1]), 0x00);
            else if(widgetClass == "FB_FP8DisplayUpperMiddle")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x02, stoi(tokenLines[i][1]), 0x01);
            else if(widgetClass == "FB_FP8DisplayLowerMiddle")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x02, stoi(tokenLines[i][1]), 0x02);
            else if(widgetClass == "FB_FP8DisplayLower")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x02, stoi(tokenLines[i][1]), 0x03);

            else if(widgetClass == "FB_FP16Display" ||  widgetClass == "FB_FP16DisplayUpper")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x16, stoi(tokenLines[i][1]), 0x00);
            else if(widgetClass == "FB_FP16DisplayUpperMiddle")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x16, stoi(tokenLines[i][1]), 0x01);
            else if(widgetClass == "FB_FP16DisplayLowerMiddle")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x16, stoi(tokenLines[i][1]), 0x02);
            else if(widgetClass == "FB_FP16DisplayLower")
                feedbackProcessor = new FPDisplay_Midi_FeedbackProcessor(surface, widget, 0x16, stoi(tokenLines[i][1]), 0x03);
        }
        
        else if((widgetClass == "FB_QConLiteDisplayUpper" || widgetClass == "FB_QConLiteDisplayUpperMid" || widgetClass == "FB_QConLiteDisplayLowerMid" || widgetClass == "FB_QConLiteDisplayLower") && size == 2)
        {
            if(widgetClass == "FB_QConLiteDisplayUpper")
                feedbackProcessor = new QConLiteDisplay_Midi_FeedbackProcessor(surface, widget, 0, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_QConLiteDisplayUpperMid")
                feedbackProcessor = new QConLiteDisplay_Midi_FeedbackProcessor(surface, widget, 1, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_QConLiteDisplayLowerMid")
                feedbackProcessor = new QConLiteDisplay_Midi_FeedbackProcessor(surface, widget, 2, 0x14, 0x12, stoi(tokenLines[i][1]));
            else if(widgetClass == "FB_QConLiteDisplayLower")
                feedbackProcessor = new QConLiteDisplay_Midi_FeedbackProcessor(surface, widget, 3, 0x14, 0x12, stoi(tokenLines[i][1]));
        }

        if(feedbackProcessor != nullptr)
            widget->AddFeedbackProcessor(feedbackProcessor);
    }
}

static void ProcessOSCWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  OSC_ControlSurface* surface)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(surface, tokens[1]);
    
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

    for(auto tokenLine : tokenLines)
    {
        if(tokenLine.size() > 1 && tokenLine[0] == "Control")
            new CSIMessageGenerator(surface, widget, tokenLine[1]);
        else if(tokenLine.size() > 1 && tokenLine[0] == "Touch")
            new Touch_CSIMessageGenerator(surface, widget, tokenLine[1]);
        else if(tokenLine.size() > 1 && tokenLine[0] == "FB_Processor")
            widget->AddFeedbackProcessor(new OSC_FeedbackProcessor(surface, widget, tokenLine[1]));
    }
}

static void ProcessWidgetFile(string filePath, ControlSurface* surface)
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
                    ProcessMidiWidget(lineNumber, file, tokens, (Midi_ControlSurface*)surface);
                if(filePath[filePath.length() - 3] == 'o')
                    ProcessOSCWidget(lineNumber, file, tokens, (OSC_ControlSurface*)surface);
            }
        }
    }
    catch (exception &e)
    {
        (void)e;  // Removes C4101 warning
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
    actions_["TrackAutoMode"] =                     new TrackAutoMode();
    actions_["GlobalAutoMode"] =                    new GlobalAutoMode();
    actions_["TrackAutoModeDisplay"] =              new TrackAutoModeDisplay();
    actions_["TimeDisplay"] =                       new TimeDisplay();
    actions_["EuConTimeDisplay"] =                  new EuConTimeDisplay();
    actions_["NoAction"] =                          new NoAction();
    actions_["Reaper"] =                            new ReaperAction();
    actions_["FixedTextDisplay"] =                  new FixedTextDisplay(); ;
    actions_["FixedRGBColourDisplay"] =             new FixedRGBColourDisplay();
    actions_["Rewind"] =                            new Rewind();
    actions_["FastForward"] =                       new FastForward();
    actions_["Play"] =                              new Play();
    actions_["Stop"] =                              new Stop();
    actions_["Record"] =                            new Record();
    actions_["CycleTimeline"] =                     new CycleTimeline();
    actions_["ToggleScrollLink"] =                  new ToggleScrollLink();
    actions_["ForceScrollLink"] =                   new ForceScrollLink();
    actions_["ToggleVCAMode"] =                     new ToggleVCAMode();
    actions_["CycleTimeDisplayModes"] =             new CycleTimeDisplayModes();
    actions_["NextPage"] =                          new GoNextPage();
    actions_["GoPage"] =                            new GoPage();
    actions_["PageNameDisplay"] =                   new PageNameDisplay();
    actions_["GoZone"] =                            new GoZone();
    actions_["GoSubZone"] =                         new GoSubZone();
    actions_["SetBroadcastGoZone"] =                new SetBroadcastGoZone();
    actions_["SetReceiveGoZone"] =                  new SetReceiveGoZone();
    actions_["SetBroadcastGoFXSlot"] =              new SetBroadcastGoFXSlot();
    actions_["SetReceiveGoFXSlot"] =                new SetReceiveGoFXSlot();
    actions_["SetBroadcastMapSelectedTrackSendsToWidgets"] =    new SetBroadcastMapSelectedTrackSendsToWidgets();
    actions_["SetReceiveMapSelectedTrackSendsToWidgets"] =      new SetReceiveMapSelectedTrackSendsToWidgets();
    actions_["SetBroadcastMapSelectedTrackReceivesToWidgets"] = new SetBroadcastMapSelectedTrackReceivesToWidgets();
    actions_["SetReceiveMapSelectedTrackReceivesToWidgets"] =   new SetReceiveMapSelectedTrackReceivesToWidgets();
    actions_["SetBroadcastMapSelectedTrackFXToWidgets"] =       new SetBroadcastMapSelectedTrackFXToWidgets();
    actions_["SetReceiveMapSelectedTrackFXToWidgets"] =         new SetReceiveMapSelectedTrackFXToWidgets();
    actions_["SetBroadcastMapSelectedTrackFXToMenu"] =          new SetBroadcastMapSelectedTrackFXToMenu();
    actions_["SetReceiveMapSelectedTrackFXToMenu"] =            new SetReceiveMapSelectedTrackFXToMenu();
    actions_["SetBroadcastMapTrackSendsSlotToWidgets"] =        new SetBroadcastMapTrackSendsSlotToWidgets();
    actions_["SetReceiveMapTrackSendsSlotToWidgets"] =          new SetReceiveMapTrackSendsSlotToWidgets();
    actions_["SetBroadcastMapTrackReceivesSlotToWidgets"] =     new SetBroadcastMapTrackReceivesSlotToWidgets();
    actions_["SetReceiveMapTrackReceivesSlotToWidgets"] =       new SetReceiveMapTrackReceivesSlotToWidgets();
    actions_["SetBroadcastMapTrackFXMenusSlotToWidgets"] =      new SetBroadcastMapTrackFXMenusSlotToWidgets();
    actions_["SetReceiveMapTrackFXMenusSlotToWidgets"] =        new SetReceiveMapTrackFXMenusSlotToWidgets();
    actions_["TrackBank"] =                             new TrackBank();
    actions_["SelectedTrackBank"] =                     new SelectedTrackBank();
    actions_["SendSlotBank"] =                          new SendSlotBank();
    actions_["ReceiveSlotBank"] =                       new ReceiveSlotBank();
    actions_["FXMenuSlotBank"] =                        new FXMenuSlotBank();
    actions_["ClearAllSolo"] =                          new ClearAllSolo();
    actions_["Shift"] =                                 new SetShift();
    actions_["Option"] =                                new SetOption();
    actions_["Control"] =                               new SetControl();
    actions_["Alt"] =                                   new SetAlt();
    actions_["MapSelectedTrackSendsToWidgets"] =        new MapSelectedTrackSendsToWidgets();
    actions_["MapSelectedTrackReceivesToWidgets"] =     new MapSelectedTrackReceivesToWidgets();
    actions_["MapSelectedTrackFXToWidgets"] =           new MapSelectedTrackFXToWidgets();
    actions_["MapSelectedTrackFXToMenu"] =              new MapSelectedTrackFXToMenu();
    actions_["MapTrackSendsSlotToWidgets"] =            new MapTrackSendsSlotToWidgets();
    actions_["MapTrackReceivesSlotToWidgets"] =         new MapTrackReceivesSlotToWidgets();
    actions_["MapTrackFXMenusSlotToWidgets"] =          new MapTrackFXMenusSlotToWidgets();
    actions_["MapSelectedTrackSendsSlotToWidgets"] =    new MapSelectedTrackSendsSlotToWidgets();
    actions_["MapSelectedTrackReceivesSlotToWidgets"] = new MapSelectedTrackReceivesSlotToWidgets();
    actions_["UnmapSelectedTrackSendsFromWidgets"] =    new UnmapSelectedTrackSendsFromWidgets();
    actions_["UnmapSelectedTrackReceivesFromWidgets"] = new UnmapSelectedTrackReceivesFromWidgets();
    actions_["UnmapSelectedTrackFXFromWidgets"] =       new UnmapSelectedTrackFXFromWidgets();
    actions_["UnmapSelectedTrackFXFromMenu"] =          new UnmapSelectedTrackFXFromMenu();
    actions_["UnmapTrackSendsSlotFromWidgets"] =        new UnmapTrackSendsSlotFromWidgets();
    actions_["UnmapTrackReceivesSlotFromWidgets"] =     new UnmapTrackReceivesSlotFromWidgets();
    actions_["UnmapTrackFXMenusSlotFromWidgets"] =      new UnmapTrackFXMenusSlotFromWidgets();
    actions_["UnmapSelectedTrackSendsSlotFromWidgets"] =    new UnmapSelectedTrackSendsSlotFromWidgets();
    actions_["UnmapSelectedTrackReceivesSlotFromWidgets"] = new UnmapSelectedTrackReceivesSlotFromWidgets();
    actions_["GoFXSlot"] =                          new GoFXSlot();
    actions_["GoCurrentFXSlot"] =                   new GoCurrentFXSlot();
    actions_["CycleTrackAutoMode"] =                new CycleTrackAutoMode();
    actions_["FocusedFXParam"] =                    new FocusedFXParam();
    actions_["FocusedFXParamNameDisplay"] =         new FocusedFXParamNameDisplay();
    actions_["FocusedFXParamValueDisplay"] =        new FocusedFXParamValueDisplay();
    actions_["TrackVolume"] =                       new TrackVolume();
    actions_["SoftTakeover7BitTrackVolume"] =       new SoftTakeover7BitTrackVolume();
    actions_["SoftTakeover14BitTrackVolume"] =      new SoftTakeover14BitTrackVolume();
    actions_["TrackVolumeDB"] =                     new TrackVolumeDB();
    actions_["TrackToggleVCASpill"] =               new TrackToggleVCASpill();
    actions_["TrackSelect"] =                       new TrackSelect();
    actions_["TrackUniqueSelect"] =                 new TrackUniqueSelect();
    actions_["TrackRangeSelect"] =                  new TrackRangeSelect();
    actions_["TrackRecordArm"] =                    new TrackRecordArm();
    actions_["TrackMute"] =                         new TrackMute();
    actions_["TrackSolo"] =                         new TrackSolo();
    actions_["TrackInvertPolarity"] =               new TrackInvertPolarity();
    actions_["MCUTrackPan"] =                       new MCUTrackPan();
    actions_["TrackPan"] =                          new TrackPan();
    actions_["TrackPanPercent"] =                   new TrackPanPercent();
    actions_["TrackPanWidth"] =                     new TrackPanWidth();
    actions_["TrackPanWidthPercent"] =              new TrackPanWidthPercent();
    actions_["TrackPanL"] =                         new TrackPanL();
    actions_["TrackPanLPercent"] =                  new TrackPanLPercent();
    actions_["TrackPanR"] =                         new TrackPanR();
    actions_["TrackPanRPercent"] =                  new TrackPanRPercent();
    actions_["TogglePin"] =                         new TogglePin();
    actions_["TrackNameDisplay"] =                  new TrackNameDisplay();
    actions_["TrackVolumeDisplay"] =                new TrackVolumeDisplay();
    actions_["MCUTrackPanDisplay"] =                new MCUTrackPanDisplay();
    actions_["TrackPanDisplay"] =                   new TrackPanDisplay();
    actions_["TrackPanWidthDisplay"] =              new TrackPanWidthDisplay();
    actions_["TrackPanLeftDisplay"] =               new TrackPanLeftDisplay();
    actions_["TrackPanRightDisplay"] =              new TrackPanRightDisplay();
    actions_["TrackOutputMeter"] =                  new TrackOutputMeter();
    actions_["TrackOutputMeterAverageLR"] =         new TrackOutputMeterAverageLR();
    actions_["TrackOutputMeterMaxPeakLR"] =         new TrackOutputMeterMaxPeakLR();
    actions_["FXParam"] =                           new FXParam();
    actions_["FXParamRelative"] =                   new FXParamRelative();
    actions_["FXNameDisplay"] =                     new FXNameDisplay();
    actions_["FXMenuNameDisplay"] =                 new FXMenuNameDisplay();
    actions_["FXParamNameDisplay"] =                new FXParamNameDisplay();
    actions_["FXParamValueDisplay"] =               new FXParamValueDisplay();
    actions_["FXGainReductionMeter"] =              new FXGainReductionMeter();
    actions_["TrackSendVolume"] =                   new TrackSendVolume();
    actions_["TrackSendVolumeDB"] =                 new TrackSendVolumeDB();
    actions_["TrackSendPan"] =                      new TrackSendPan();
    actions_["TrackSendPanPercent"] =               new TrackSendPanPercent();
    actions_["TrackSendMute"] =                     new TrackSendMute();
    actions_["TrackSendInvertPolarity"] =           new TrackSendInvertPolarity();
    actions_["TrackSendStereoMonoToggle"] =         new TrackSendStereoMonoToggle();
    actions_["TrackSendPrePost"] =                  new TrackSendPrePost();
    actions_["TrackSendNameDisplay"] =              new TrackSendNameDisplay();
    actions_["TrackSendVolumeDisplay"] =            new TrackSendVolumeDisplay();
    actions_["TrackSendPanDisplay"] =               new TrackSendPanDisplay();
    actions_["TrackSendPrePostDisplay"] =           new TrackSendPrePostDisplay();
    actions_["TrackReceiveVolume"] =                new TrackReceiveVolume();
    actions_["TrackReceiveVolumeDB"] =              new TrackReceiveVolumeDB();
    actions_["TrackReceivePan"] =                   new TrackReceivePan();
    actions_["TrackReceivePanPercent"] =            new TrackReceivePanPercent();
    actions_["TrackReceiveMute"] =                  new TrackReceiveMute();
    actions_["TrackReceiveInvertPolarity"] =        new TrackReceiveInvertPolarity();
    actions_["TrackReceivePrePost"] =               new TrackReceivePrePost();
    actions_["TrackReceiveNameDisplay"] =           new TrackReceiveNameDisplay();
    actions_["TrackReceiveVolumeDisplay"] =         new TrackReceiveVolumeDisplay();
    actions_["TrackReceivePanDisplay"] =            new TrackReceivePanDisplay();
    actions_["TrackReceivePrePostDisplay"] =        new TrackReceivePrePostDisplay();
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
        
        int numChannels = 0;
    
        for (string line; getline(iniFile, line) ; )
        {
            line = regex_replace(line, regex(TabChars), " ");
            line = regex_replace(line, regex(CRLFChars), "");
            
            vector<string> tokens(GetTokens(line));
            
            if(tokens.size() > 4) // ignore comment lines and blank lines
            {
                if(tokens[0] == MidiSurfaceToken && tokens.size() == 10)
                {
                    if(atoi(tokens[6].c_str()) + atoi(tokens[9].c_str()) > numChannels )
                        numChannels = atoi(tokens[6].c_str()) + atoi(tokens[9].c_str());
                }
                else if(tokens[0] == OSCSurfaceToken && tokens.size() == 11)
                {
                    if(atoi(tokens[6].c_str()) + atoi(tokens[9].c_str()) > numChannels )
                        numChannels = atoi(tokens[6].c_str()) + atoi(tokens[9].c_str());
                }
            }
        }
        
        iniFile.clear();
        iniFile.seekg(0);
        
        for (string line; getline(iniFile, line) ; )
        {
            line = regex_replace(line, regex(TabChars), " ");
            line = regex_replace(line, regex(CRLFChars), "");
            
            vector<string> tokens(GetTokens(line));
            
            if(tokens.size() > 4) // ignore comment lines and blank lines
            {
                if(tokens[0] == PageToken)
                {
                    if(tokens.size() != 5)
                        continue;

                    currentPage = new Page(tokens[1], tokens[2] == "FollowMCP" ? true : false, tokens[3] == "SynchPages" ? true : false, tokens[4] == "UseScrollLink" ? true : false, numChannels);
                    pages_.push_back(currentPage);
                }
                else if(tokens[0] == MidiSurfaceToken || tokens[0] == OSCSurfaceToken)
                {
                    int inPort = 0;
                    int outPort = 0;
                    
                    inPort = atoi(tokens[2].c_str());
                    outPort = atoi(tokens[3].c_str());
                    
                    if(currentPage)
                    {
                        ControlSurface* surface = nullptr;
                        
                        if(tokens[0] == MidiSurfaceToken && tokens.size() == 10)
                            surface = new Midi_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], atoi(tokens[6].c_str()), atoi(tokens[7].c_str()), atoi(tokens[8].c_str()), atoi(tokens[9].c_str()), GetMidiInputForPort(inPort), GetMidiOutputForPort(outPort));
                        else if(tokens[0] == OSCSurfaceToken && tokens.size() == 11)
                            surface = new OSC_ControlSurface(CSurfIntegrator_, currentPage, tokens[1], tokens[4], tokens[5], atoi(tokens[6].c_str()), atoi(tokens[7].c_str()), atoi(tokens[8].c_str()), atoi(tokens[9].c_str()), GetInputSocketForPort(tokens[1], inPort), GetOutputSocketForAddressAndPort(tokens[1], tokens[10], outPort));

                        currentPage->AddSurface(surface);
                    }
                }
            }
        }
        
        // Restore the PageIndex
        currentPageIndex_ = 0;
        
        char buf[512];
        
        int result = DAW::GetProjExtState(0, "CSI", "PageIndex", buf, sizeof(buf));
        
        if(result > 0)
        {
            currentPageIndex_ = atoi(buf);
 
            if(currentPageIndex_ > pages_.size() - 1)
                currentPageIndex_ = 0;
        }
        
        // Restore the BankIndex
        result = DAW::GetProjExtState(0, "CSI", "BankIndex", buf, sizeof(buf));
        
        if(result > 0 && pages_.size() > 0)
        {
            if(MediaTrack* leftmosttrack = DAW::GetTrack(atoi(buf) + 1))
                DAW::SetMixerScroll(leftmosttrack);
            
            pages_[currentPageIndex_]->AdjustTrackBank(atoi(buf));
        }
        
        // Restore the Pinned Tracks
        if(pages_.size() > 0)
            pages_[currentPageIndex_]->RestorePinnedTracks();
    }
    catch (exception &e)
    {
        (void)e;  // Removes C4101 warning
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble in %s, around line %d\n", iniFilePath.c_str(), lineNumber);
        DAW::ShowConsoleMsg(buffer);
    }
      
    for(auto page : pages_)
        page->OnInitialization();
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
    if(pinnedTrack_ == nullptr)
    {
        pinnedTrack_ = GetTrack();
        
        manager_->IncChannelBias(channelNum_);
    }
}

void TrackNavigator::UnpinChannel()
{
    if(pinnedTrack_ != nullptr)
    {
        manager_->DecChannelBias(channelNum_);
        
        pinnedTrack_ = nullptr;
    }
}

MediaTrack* TrackNavigator::GetTrack()
{
    return manager_->GetTrackFromChannel(channelNum_, bias_, pinnedTrack_);
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
    return page_->GetSelectedTrack();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FocusedFXNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* FocusedFXNavigator::GetTrack()
{
    int trackNumber = 0;
    int itemNumber = 0;
    int fxIndex = 0;
    
    if(DAW::GetFocusedFX2(&trackNumber, &itemNumber, &fxIndex) == 1) // Track FX
        return DAW::GetTrack(trackNumber);
    else
        return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ActionContext::ActionContext(Action* action, Widget* widget, Zone* zone, vector<string> params, vector<vector<string>> properties): action_(action), widget_(widget), zone_(zone), properties_(properties)
{   
    for(auto property : properties)
    {
        if(property.size() == 0)
            continue;

        if(property[0] == "NoFeedback")
            noFeedback_ = true;
    }
    
    widget->SetProperties(properties);
    
    string actionName = "";
    
    if(params.size() > 0)
        actionName = params[0];
    
    // Action with int param, could include leading minus sign
    if(params.size() > 1 && (isdigit(params[1][0]) ||  params[1][0] == '-'))  // C++ 11 says empty strings can be queried without catastrophe :)
    {
        intParam_= atol(params[1].c_str());
    }
    
    // Action with param index, must be positive
    if(params.size() > 1 && isdigit(params[1][0]))  // C++ 11 says empty strings can be queried without catastrophe :)
    {
        paramIndex_ = atol(params[1].c_str());
    }
    
    // Action with string param
    if(params.size() > 1)
        stringParam_ = params[1];
    
    if(actionName == "TrackVolumeDB" || actionName == "TrackSendVolumeDB")
    {
        rangeMinimum_ = -144.0;
        rangeMaximum_ = 24.0;
    }
    
    if(actionName == "TrackPanPercent" || actionName == "TrackPanWidthPercent" || actionName == "TrackPanLPercent" || actionName == "TrackPanRPercent")
    {
        rangeMinimum_ = -100.0;
        rangeMaximum_ = 100.0;
    }
   
    if(actionName == "Reaper" && params.size() > 1)
    {
        if (isdigit(params[1][0]))
        {
            commandId_ =  atol(params[1].c_str());
        }
        else // look up by string
        {
            commandId_ = DAW::NamedCommandLookup(params[1].c_str());
            
            if(commandId_ == 0) // can't find it
                commandId_ = 65535; // no-op
        }
    }
    
    if(actionName == "FXParam" && params.size() > 1 && isdigit(params[1][0])) // C++ 11 says empty strings can be queried without catastrophe :)
    {
        paramIndex_ = atol(params[1].c_str());
    }
    
    if(actionName == "FXParamValueDisplay" && params.size() > 1 && isdigit(params[1][0]))
    {
        paramIndex_ = atol(params[1].c_str());
        
        if(params.size() > 2 && params[2] != "[" && params[2] != "{" && isdigit(params[2][0]))
        {
            shouldUseDisplayStyle_ = true;
            displayStyle_ = atol(params[2].c_str());
        }
    }
    
    if(actionName == "FXParamNameDisplay" && params.size() > 1 && isdigit(params[1][0]))
    {
        paramIndex_ = atol(params[1].c_str());
        
        if(params.size() > 2 && params[2] != "{" && params[2] != "[")
            fxParamDisplayName_ = params[2];
    }
    
    if(actionName == "MCUTrackPanDisplay"&& params.size() > 1)
    {
        SetAssociatedWidget(GetSurface()->GetWidgetByName(params[1]));
    }
    
    if(params.size() > 0)
    {
        SetRGB(params, supportsRGB_, supportsTrackColor_, RGBValues_);
        SetSteppedValues(params, deltaValue_, acceleratedDeltaValues_, rangeMinimum_, rangeMaximum_, steppedValues_, acceleratedTickValues_);
    }
    
    if(acceleratedTickValues_.size() < 1)
        acceleratedTickValues_.push_back(10);

}

Page* ActionContext::GetPage()
{
    return widget_->GetSurface()->GetPage();
}

ControlSurface* ActionContext::GetSurface()
{
    return widget_->GetSurface();
}

MediaTrack* ActionContext::GetTrack()
{
    return zone_->GetNavigator()->GetTrack();
}

int ActionContext::GetSlotIndex()
{
    return zone_->GetSlotIndex();
}

string ActionContext::GetName()
{
    return zone_->GetNameOrAlias();
}

void ActionContext::RunDeferredActions()
{
    if(holdDelayAmount_ != 0.0 && delayStartTime_ != 0.0 && DAW::GetCurrentNumberOfMilliseconds() > (delayStartTime_ + holdDelayAmount_))
    {
        DoRangeBoundAction(deferredValue_);
        
        delayStartTime_ = 0.0;
        deferredValue_ = 0.0;
    }
}

void ActionContext::RequestUpdate()
{
    if(noFeedback_)
        return;
    
    action_->RequestUpdate(this);
}

void ActionContext::ClearWidget()
{
    widget_->Clear();
}

void ActionContext::UpdateWidgetValue(double value)
{
    if(steppedValues_.size() > 0)
        SetSteppedValueIndex(value);

    value = isFeedbackInverted_ == false ? value : 1.0 - value;
   
    widget_->UpdateValue(value);

    if(supportsRGB_)
    {
        currentRGBIndex_ = value == 0 ? 0 : 1;
        widget_->UpdateRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
    }
    else if (supportsZoneFeedback_)
    {
        widget_->UpdateDataValue(FeedBackData_);
    }
    else if(supportsTrackColor_)
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
            
            int r = (*rgb_colour >> 0) & 0xff;
            int g = (*rgb_colour >> 8) & 0xff;
            int b = (*rgb_colour >> 16) & 0xff;
            
            widget_->UpdateRGBValue(r, g, b);
        }
    }
}

void ActionContext::UpdateWidgetValue(int param, double value)
{
    if(steppedValues_.size() > 0)
        SetSteppedValueIndex(value);

    value = isFeedbackInverted_ == false ? value : 1.0 - value;
        
    widget_->UpdateValue(param, value);
    
    currentRGBIndex_ = value == 0 ? 0 : 1;
    
    if(supportsRGB_)
    {
        currentRGBIndex_ = value == 0 ? 0 : 1;
        widget_->UpdateRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
    }
    else if(supportsTrackColor_)
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
            
            int r = (*rgb_colour >> 0) & 0xff;
            int g = (*rgb_colour >> 8) & 0xff;
            int b = (*rgb_colour >> 16) & 0xff;
            
            widget_->UpdateRGBValue(r, g, b);
        }
    }
}

void ActionContext::UpdateWidgetValue(string value)
{
    widget_->UpdateValue(value);
}

void ActionContext::ForceWidgetValue(double value)
{
    if(steppedValues_.size() > 0)
        SetSteppedValueIndex(value);
    
    value = isFeedbackInverted_ == false ? value : 1.0 - value;
    
    widget_->ForceValue(value);

    if(supportsRGB_)
    {
        currentRGBIndex_ = value == 0 ? 0 : 1;
        widget_->ForceRGBValue(RGBValues_[currentRGBIndex_].r, RGBValues_[currentRGBIndex_].g, RGBValues_[currentRGBIndex_].b);
    }
    else if(supportsTrackColor_)
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            unsigned int* rgb_colour = (unsigned int*)DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", NULL);
            
            int r = (*rgb_colour >> 0) & 0xff;
            int g = (*rgb_colour >> 8) & 0xff;
            int b = (*rgb_colour >> 16) & 0xff;
            
            widget_->ForceRGBValue(r, g, b);
        }
    }
}

void ActionContext::DoAction(double value)
{
    if(holdDelayAmount_ != 0.0)
    {
        if(value == 0.0)
        {
            deferredValue_ = 0.0;
            delayStartTime_ = 0.0;
        }
        else
        {
            deferredValue_ = value;
            delayStartTime_ =  DAW::GetCurrentNumberOfMilliseconds();
        }
    }
    else
    {
        if(steppedValues_.size() > 0)
        {
            if(value != 0.0) // ignore release messages
            {
                if(steppedValuesIndex_ == steppedValues_.size() - 1)
                {
                    if(steppedValues_[0] < steppedValues_[steppedValuesIndex_]) // GAW -- only wrap if 1st value is lower
                        steppedValuesIndex_ = 0;
                }
                else
                    steppedValuesIndex_++;
                
                DoRangeBoundAction(steppedValues_[steppedValuesIndex_]);
            }
        }
        else
            DoRangeBoundAction(value);
    }
}

void ActionContext::DoRelativeAction(double delta)
{
    if(steppedValues_.size() > 0)
        DoSteppedValueAction(delta);
    else
        DoRangeBoundAction(action_->GetCurrentNormalizedValue(this) + delta);
}

void ActionContext::DoRelativeAction(int accelerationIndex, double delta)
{
    if(steppedValues_.size() > 0)
        DoAcceleratedSteppedValueAction(accelerationIndex, delta);
    else if(acceleratedDeltaValues_.size() > 0)
        DoAcceleratedDeltaValueAction(accelerationIndex, delta);
    else
        DoRangeBoundAction(action_->GetCurrentNormalizedValue(this) + delta);
}

void ActionContext::DoRangeBoundAction(double value)
{
    if(value > rangeMaximum_)
        value = rangeMaximum_;
    
    if(value < rangeMinimum_)
        value = rangeMinimum_;
    
    action_->Do(this, value);
}

void ActionContext::DoSteppedValueAction(double delta)
{
    if(delta > 0)
    {
        steppedValuesIndex_++;
        
        if(steppedValuesIndex_ > steppedValues_.size() - 1)
            steppedValuesIndex_ = steppedValues_.size() - 1;
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_]);
    }
    else
    {
        steppedValuesIndex_--;
        
        if(steppedValuesIndex_ < 0 )
            steppedValuesIndex_ = 0;
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_]);
    }
}

void ActionContext::DoAcceleratedSteppedValueAction(int accelerationIndex, double delta)
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
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_]);
    }
    else if(delta < 0 && accumulatedDecTicks_ >= acceleratedTickValues_[accelerationIndex])
    {
        accumulatedIncTicks_ = 0;
        accumulatedDecTicks_ = 0;
        
        steppedValuesIndex_--;
        
        if(steppedValuesIndex_ < 0 )
            steppedValuesIndex_ = 0;
        
        DoRangeBoundAction(steppedValues_[steppedValuesIndex_]);
    }
}

void ActionContext::DoAcceleratedDeltaValueAction(int accelerationIndex, double delta)
{
    accelerationIndex = accelerationIndex > acceleratedDeltaValues_.size() - 1 ? acceleratedDeltaValues_.size() - 1 : accelerationIndex;
    accelerationIndex = accelerationIndex < 0 ? 0 : accelerationIndex;
    
    if(delta > 0.0)
        DoRangeBoundAction(action_->GetCurrentNormalizedValue(this) + acceleratedDeltaValues_[accelerationIndex]);
    else
        DoRangeBoundAction(action_->GetCurrentNormalizedValue(this) - acceleratedDeltaValues_[accelerationIndex]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Zone::Activate()
{
    surface_->LoadingZone(GetName());
    
    for(auto zone : includedZones_)
        zone->Activate();
}

void Zone::Activate(vector<Zone*> *activeZones)
{
    Activate();
    
    auto it = find(activeZones->begin(), activeZones->end(), this);
    
    if ( it != activeZones->end() )
        activeZones->erase(it);

    activeZones->insert(activeZones->begin(), this);
    
    surface_->MoveToFirst(activeZones);
}

void Zone::Deactivate()
{
    for(auto widget : widgets_)
        widget->Clear();
}

vector<ActionContext>& Zone::GetActionContexts(Widget* widget)
{
    string widgetName = widget->GetName();
    string modifier = "";
    
    if( ! widget->GetIsModifier())
        modifier = surface_->GetPage()->GetModifier();
    
    if(touchIds_.count(widgetName) > 0 && activeTouchIds_.count(touchIds_[widgetName]) > 0 && activeTouchIds_[touchIds_[widgetName]] == true && actionContextDictionary_[widget].count(touchIds_[widgetName] + "+" + modifier) > 0)
        return actionContextDictionary_[widget][touchIds_[widgetName] + "+" + modifier];
    else if(actionContextDictionary_[widget].count(modifier) > 0)
        return actionContextDictionary_[widget][modifier];
    else if(actionContextDictionary_[widget].count("") > 0)
        return actionContextDictionary_[widget][""];
    else
        return defaultContexts_;
}

int Zone::GetSlotIndex()
{
    if(navigationStyle_ == NavigationStyle::Standard)
        return slotIndex_;
    else if(navigationStyle_ == NavigationStyle::SendSlot)
        return surface_->GetPage()->GetSendSlot();
    else if(navigationStyle_ == NavigationStyle::ReceiveSlot)
        return surface_->GetPage()->GetReceiveSlot();
    else if(navigationStyle_ == NavigationStyle::FXMenuSlot)
        return surface_->GetPage()->GetFXMenuSlot();
    else if(navigationStyle_ == NavigationStyle::SelectedTrackSendSlot)
        return slotIndex_ + surface_->GetPage()->GetSendSlot();
    else if(navigationStyle_ == NavigationStyle::SelectedTrackReceiveSlot)
        return slotIndex_ + surface_->GetPage()->GetReceiveSlot();
    else
        return 0;
}

void Zone::RequestUpdateWidget(Widget* widget)
{
    widget->HandleQueuedActions(this);
    
    // GAW TBD -- This is where we might cut loose multiple feedback if we can individually control it
    
    for(auto &context : GetActionContexts(widget))
        context.RunDeferredActions();
    
    if(GetActionContexts(widget).size() > 0)
    {
        ActionContext& context = GetActionContexts(widget)[0];
        context.RequestUpdate();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
Widget::Widget(ControlSurface* surface, string name) : surface_(surface), name_(name) {}

Widget::~Widget()
{
    for(auto feedbackProcessor :feedbackProcessors_)
    {
        delete feedbackProcessor;
        feedbackProcessor = nullptr;
    }
};


void Widget::HandleQueuedActions(Zone* zone)
{
    vector<double> queuedActionValues;
    vector<double> queuedRelativeActionValues;
    vector<QueuedAcceleratedRelativeAction> queuedAcceleratedRelativeActionValues;
    vector<double> queuedTouchActionValues;

    WDL_mutex.Enter();
    
    for(auto value : queuedActionValues_)
        queuedActionValues.push_back(value);
    queuedActionValues_.clear();

    for(auto delta : queuedRelativeActionValues_)
        queuedRelativeActionValues.push_back(delta);
    queuedRelativeActionValues_.clear();

    for(auto acceleratedRelativeAction : queuedAcceleratedRelativeActionValues_)
        queuedAcceleratedRelativeActionValues.push_back(acceleratedRelativeAction);
    queuedAcceleratedRelativeActionValues_.clear();

    for(auto value : queuedTouchActionValues_)
        queuedTouchActionValues.push_back(value);
    queuedTouchActionValues_.clear();

    WDL_mutex.Leave();
    

    for(auto value : queuedActionValues)
        zone->DoAction(this, value);
        
    for(auto delta : queuedRelativeActionValues)
        zone->DoRelativeAction(this, delta);
     
    for(auto acceleratedRelativeAction : queuedAcceleratedRelativeActionValues)
        zone->DoRelativeAction(this, acceleratedRelativeAction.index, acceleratedRelativeAction.delta);

    for(auto value : queuedTouchActionValues)
        zone->DoTouch(this, name_, value);
}

void Widget::QueueAction(double value)
{
    LogInput(value);
    
    WDL_mutex.Enter();
    queuedActionValues_.push_back(value);
    WDL_mutex.Leave();
}

void Widget::QueueRelativeAction(double delta)
{
    LogInput(delta);
    
    WDL_mutex.Enter();
    queuedRelativeActionValues_.push_back(delta);
    WDL_mutex.Leave();
}

void Widget::QueueRelativeAction(int accelerationIndex, double delta)
{
    LogInput(accelerationIndex);
    
    WDL_mutex.Enter();
    queuedAcceleratedRelativeActionValues_.push_back(QueuedAcceleratedRelativeAction(accelerationIndex, delta));
    WDL_mutex.Leave();
}

void Widget::QueueTouch(double value)
{
    LogInput(value);
    
    WDL_mutex.Enter();
    queuedTouchActionValues_.push_back(value);
    WDL_mutex.Leave();
}

void Widget::SetProperties(vector<vector<string>> properties)
{
    for(auto processor : feedbackProcessors_)
        processor->SetProperties(properties);
}

void  Widget::UpdateValue(double value)
{
    for(auto processor : feedbackProcessors_)
        processor->SetValue(value);
}

void  Widget::UpdateValue(int mode, double value)
{
    for(auto processor : feedbackProcessors_)
        processor->SetValue(mode, value);
}

void  Widget::UpdateValue(string value)
{
    for(auto processor : feedbackProcessors_)
        processor->SetValue(value);
}

void  Widget::UpdateRGBValue(int r, int g, int b)
{
    for(auto processor : feedbackProcessors_)
        processor->SetRGBValue(r, g, b);
}

void  Widget::UpdateDataValue(vector<string> FeedbackData)
{
    for (auto processor : feedbackProcessors_)
        processor->SetDataValue(FeedbackData);
}

void  Widget::ForceValue(double value)
{
    for(auto processor : feedbackProcessors_)
        processor->ForceValue(value);
}

void  Widget::ForceRGBValue(int r, int g, int b)
{
    for(auto processor : feedbackProcessors_)
        processor->ForceRGBValue(r, g, b);
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

void Widget::LogInput(double value)
{
    if(TheManager->GetSurfaceInDisplay())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN <- %s %s %f\n", GetSurface()->GetName().c_str(), GetName().c_str(), value);
        DAW::ShowConsoleMsg(buffer);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSIMessageGenerator::CSIMessageGenerator(ControlSurface* surface, Widget* widget, string message) : widget_(widget)
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
        ForceMidiMessage(first, second, third);
}

void Midi_FeedbackProcessor::ForceMidiMessage(int first, int second, int third)
{
    lastMessageSent_->midi_message[0] = first;
    lastMessageSent_->midi_message[1] = second;
    lastMessageSent_->midi_message[2] = third;
    surface_->SendMidiMessage(first, second, third);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_FeedbackProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSC_FeedbackProcessor::SetRGBValue(int r, int g, int b)
{
    if(lastRValue != r)
    {
        lastRValue = r;
        surface_->SendOSCMessage(this, oscAddress_ + "/rColor", r);
    }
    
    if(lastGValue != g)
    {
        lastGValue = g;
        surface_->SendOSCMessage(this, oscAddress_ + "/gColor", g);
    }
    
    if(lastBValue != b)
    {
        lastBValue = b;
        surface_->SendOSCMessage(this, oscAddress_ + "/bColor", b);
    }
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlSurface::ControlSurface(CSurfIntegrator* CSurfIntegrator, Page* page, const string name, string zoneFolder, int numChannels, int numSends, int numFX, int channelOffset):  CSurfIntegrator_(CSurfIntegrator), page_(page), name_(name), zoneFolder_(zoneFolder), numChannels_(numChannels), numSends_(numSends), numFXSlots_(numFX)
{
    for(int i = 0; i < numChannels; i++)
        navigators_[i] = GetPage()->GetNavigatorForChannel(i + channelOffset);
   
    LoadDefaultZoneOrder();
}

void ControlSurface::InitZones(string zoneFolder)
{
    try
    {
        vector<string> zoneFilesToProcess;
        listZoneFiles(DAW::GetResourcePath() + string("/CSI/Zones/") + zoneFolder + "/", zoneFilesToProcess); // recursively find all the .zon files, starting at zoneFolder
        
        for(auto zoneFilename : zoneFilesToProcess)
            PreProcessZoneFile(zoneFilename, this);
    }
    catch (exception &e)
    {
        (void)e;  // Removes C4101 warning
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "Trouble parsing Zone folders\n");
        DAW::ShowConsoleMsg(buffer);
    }
}

void ControlSurface::SurfaceOutMonitor(Widget* widget, string address, string value)
{
    if(TheManager->GetSurfaceOutDisplay())
        DAW::ShowConsoleMsg(("OUT->" + name_ + " " + address + " " + value + "\n").c_str());
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

void ControlSurface::UnmapTrackSendsSlotFromWidgets()
{
    for(int i = 0; i < numChannels_; i ++)
    {
        string trackNum = to_string(i + 1);
        
        GoZone(&activeSelectedTrackSendsZones_, "TrackSendSlot" + trackNum, 0);
    }
}

void ControlSurface::MapTrackSendsSlotToWidgets()
{
    for(int i = 0; i < numChannels_; i ++)
    {
        string trackNum = to_string(i + 1);
        
        GoZone(&activeSelectedTrackSendsZones_, "TrackSendSlot" + trackNum, 1);
    }
}

void ControlSurface::UnmapSelectedTrackSendsSlotFromWidgets()
{
    for(auto zone : activeSelectedTrackSendsZones_)
        zone->Deactivate();

    if(numSends_ == 1)
    {
        GoZone(&activeSelectedTrackSendsZones_, "SelectedTrackSendSlot", 0);
    }
    else
    {
        for(int i = 0; i < numSends_; i ++)
        {
            string sendNum = to_string(i + 1);
            
            GoZone(&activeSelectedTrackSendsZones_, "SelectedTrackSendSlot" + sendNum, 0);
    }
    }
}

void ControlSurface::MapSelectedTrackSendsSlotToWidgets()
{
    if(numSends_ == 1)
    {
        GoZone(&activeSelectedTrackSendsZones_, "SelectedTrackSendSlot", 1);
    }
    else
    {
        for(int i = 0; i < numSends_; i ++)
        {
            string sendNum = to_string(i + 1);
            
            GoZone(&activeSelectedTrackSendsZones_, "SelectedTrackSendSlot" + sendNum, 1);
        }
    }
}

void ControlSurface::UnmapSelectedTrackSendsFromWidgets()
{
    for(auto zone : activeSelectedTrackSendsZones_)
        zone->Deactivate();

    activeSelectedTrackSendsZones_.clear();
}

void ControlSurface::MapSelectedTrackSendsToWidgets()
{
    UnmapSelectedTrackSendsFromWidgets();
    
    if(MediaTrack* track = GetPage()->GetSelectedTrack())
        MapSelectedTrackItemsToWidgets(track, "SelectedTrackSend", GetNumSendSlots(), &activeSelectedTrackSendsZones_);
}

void ControlSurface::UnmapTrackReceivesSlotFromWidgets()
{
    for(int i = 0; i < numChannels_; i ++)
    {
        string trackNum = to_string(i + 1);
        
        GoZone(&activeSelectedTrackReceivesZones_, "TrackReceiveSlot" + trackNum, 0);
    }
}

void ControlSurface::MapTrackReceivesSlotToWidgets()
{
    for(int i = 0; i < numChannels_; i ++)
    {
        string trackNum = to_string(i + 1);
        
        GoZone(&activeSelectedTrackReceivesZones_, "TrackReceiveSlot" + trackNum, 1);
    }
}

void ControlSurface::UnmapSelectedTrackReceivesSlotFromWidgets()
{
    for(auto zone : activeSelectedTrackReceivesZones_)
        zone->Deactivate();

    if(numSends_ == 1)
    {
        GoZone(&activeSelectedTrackReceivesZones_, "SelectedTrackReceiveSlot", 0);
    }
    else
    {
        for(int i = 0; i < numSends_; i ++)
        {
            string receiveNum = to_string(i + 1);
            
            GoZone(&activeSelectedTrackReceivesZones_, "SelectedTrackReceiveSlot" + receiveNum, 0);
        }
    }
}

void ControlSurface::MapSelectedTrackReceivesSlotToWidgets()
{
    if(numSends_ == 1)
    {
        GoZone(&activeSelectedTrackReceivesZones_, "SelectedTrackReceiveSlot", 1);
    }
    else
    {
        for(int i = 0; i < numSends_; i ++)
        {
            string receiveNum = to_string(i + 1);
            
            GoZone(&activeSelectedTrackReceivesZones_, "SelectedTrackReceiveSlot" + receiveNum, 1);
        }
    }
}

void ControlSurface::UnmapSelectedTrackReceivesFromWidgets()
{
    for(auto zone : activeSelectedTrackReceivesZones_)
        zone->Deactivate();
    activeSelectedTrackReceivesZones_.clear();
}

void ControlSurface::MapSelectedTrackReceivesToWidgets()
{
    UnmapSelectedTrackReceivesFromWidgets();
    
    if(MediaTrack* track = GetPage()->GetSelectedTrack())
        MapSelectedTrackItemsToWidgets(track, "SelectedTrackReceive", GetNumSendSlots(), &activeSelectedTrackReceivesZones_);
}

void ControlSurface::UnmapTrackFXMenusSlotFromWidgets()
{
    UnmapSelectedTrackReceivesFromWidgets();
    
    if(MediaTrack* track = GetPage()->GetSelectedTrack())
        MapSelectedTrackItemsToWidgets(track, "SelectedTrackReceive", GetNumSendSlots(), &activeSelectedTrackReceivesZones_);
}

void ControlSurface::MapTrackFXMenusSlotToWidgets()
{
    for(int i = 0; i < numChannels_; i ++)
    {
        string trackNum = to_string(i + 1);
        
        GoZone(&activeSelectedTrackFXMenuZones_, "TrackFXMenuSlot" + trackNum, 1);
    }
}

void ControlSurface::UnmapSelectedTrackFXFromMenu()
{
    for(auto zone : activeSelectedTrackFXMenuZones_)
        zone->Deactivate();
    activeSelectedTrackFXMenuZones_.clear();
    
    for(auto zone : activeSelectedTrackFXMenuFXZones_)
        zone->Deactivate();
    activeSelectedTrackFXMenuFXZones_.clear();
}

void ControlSurface::MapSelectedTrackFXToMenu()
{
    UnmapSelectedTrackFXFromMenu();
    
    
    
#ifdef _WIN32
// GAW -- This hack is only needed for Mac OS
#else
    // GAW TOTAL Hack to prevent crash
    for(int i = 0; i < 100; i++)
    {
        string numStr = to_string(i + 1);
        
        if(Widget* widget = GetWidgetByName("SubZone" + numStr))
        {
            for(int i = 0; i < 10; i++)
                widget->QueueAction(1.0);
        }
    }
#endif

    
    
    if(MediaTrack* track = GetPage()->GetSelectedTrack())
        MapSelectedTrackItemsToWidgets(track, "SelectedTrackFXMenu", GetNumFXSlots(), &activeSelectedTrackFXMenuZones_);
}

void ControlSurface::MapSelectedTrackItemsToWidgets(MediaTrack* track, string baseName, int numberOfSlots, vector<Zone*> *activeZones)
{
    for(int i = 0; i < numberOfSlots; i++)
    {
        string name = baseName;
        
        if(numberOfSlots > 1)
            name += to_string(i + 1);
        
        if(Zone* zone = GetZone(name))
            zone->Activate(activeZones);
    }
}

void ControlSurface::UnmapSelectedTrackFXFromWidgets()
{
    for(auto zone : activeSelectedTrackFXZones_)
        zone->Deactivate();
    activeSelectedTrackFXZones_.clear();
}

void ControlSurface::MapSelectedTrackFXToWidgets()
{
    UnmapSelectedTrackFXFromWidgets();
    
    if(MediaTrack* selectedTrack = GetPage()->GetSelectedTrack())
        for(int i = 0; i < DAW::TrackFX_GetCount(selectedTrack); i++)
            MapSelectedTrackFXSlotToWidgets(&activeSelectedTrackFXZones_, i);
}

void ControlSurface::MapSelectedTrackFXMenuSlotToWidgets(int fxSlot)
{
    for(auto zone : activeSelectedTrackFXMenuFXZones_)
        zone->Deactivate();
    activeSelectedTrackFXMenuFXZones_.clear();
    
    MapSelectedTrackFXSlotToWidgets(&activeSelectedTrackFXMenuFXZones_, fxSlot);
}

void ControlSurface::MapSelectedTrackFXSlotToWidgets(vector<Zone*> *activeZones, int fxSlot)
{
    MediaTrack* selectedTrack = GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    char FXName[BUFSZ];
    
    DAW::TrackFX_GetFXName(selectedTrack, fxSlot, FXName, sizeof(FXName));
    
    if(Zone* zone = GetZone(FXName))
    {
        if( ! zone->GetNavigator()->GetIsFocusedFXNavigator())
        {
            zone->SetSlotIndex(fxSlot);
            zone->Activate(activeZones);
        }
    }
}

void ControlSurface::UnmapFocusedFXFromWidgets()
{
    for(auto zone : activeFocusedFXZones_)
        zone->Deactivate();

    activeFocusedFXZones_.clear();
}

void ControlSurface::MapFocusedFXToWidgets()
{
    UnmapFocusedFXFromWidgets();
    
    int trackNumber = 0;
    int itemNumber = 0;
    int fxSlot = 0;
    MediaTrack* focusedTrack = nullptr;
    
    if(DAW::GetFocusedFX2(&trackNumber, &itemNumber, &fxSlot) == 1)
        if(trackNumber > 0)
            focusedTrack = DAW::GetTrack(trackNumber);
    
    if(focusedTrack)
    {
        char FXName[BUFSZ];
        DAW::TrackFX_GetFXName(focusedTrack, fxSlot, FXName, sizeof(FXName));
        
        if(Zone* zone = GetZone(FXName))
        {
            if(zone->GetNavigator()->GetIsFocusedFXNavigator())
            {
                zone->SetSlotIndex(fxSlot);
                zone->Activate(&activeFocusedFXZones_);
            }
        }
    }
}

void ControlSurface::TrackFXListChanged()
{
    OnTrackSelection();
}

void ControlSurface::OnTrackSelection()
{
    if(widgetsByName_.count("OnTrackSelection") > 0)
    {
        if(page_->GetSelectedTrack())
            widgetsByName_["OnTrackSelection"]->QueueAction(1.0);
        else
            widgetsByName_["OnTrackSelection"]->QueueAction(0.0);
    }
}

void ControlSurface::LoadZone(string zoneName)
{
    if(zonesByName_.count(zoneName) == 0)
    {
        if(zoneFilenames_.count(zoneName) > 0)
            ProcessZoneFile(zoneFilenames_[zoneName], this);
    }
}

Zone* ControlSurface::GetZone(string zoneName)
{
    if(zonesByName_.count(zoneName) > 0)
        return zonesByName_[zoneName];

    if(zoneFilenames_.count(zoneName) > 0)
    {
        LoadZone(zoneName);
        
        if(zonesByName_.count(zoneName) > 0)
            return zonesByName_[zoneName];
    }
    else if(isdigit(zoneName.back())) // e.g Channel14 -- the base Zone is Channel
    {
        string baseName = zoneName;
        
        while(isdigit(baseName.back()))
             baseName = baseName.substr(0, baseName.length() - 1);
        
        if(zoneFilenames_.count(baseName) > 0)
        {
            LoadZone(baseName);
            
            if(zonesByName_.count(zoneName) > 0)
                return zonesByName_[zoneName];
        }
    }

    return nullptr;
}

void ControlSurface::GoSubZone(Zone* enclosingZone, string zoneName, double value)
{
    for(auto activeZones : allActiveZones_)
    {
        for(auto zone : *activeZones)
        {
            if(zone == enclosingZone)
            {
                GoZone(activeZones, zoneName, value);
                if(zonesByName_.count(zoneName) > 0)
                    zonesByName_[zoneName]->SetSlotIndex(enclosingZone->GetSlotIndex());
            }
        }
    }
}

void ControlSurface::GoZone(string zoneName, double value)
{
    // GAW TBD -- Use Zone name to determine in which activeZoneList to put this activated Zone
    
    GoZone(&activeZones_, zoneName, value);
}

void ControlSurface::GoZone(vector<Zone*> *activeZones, string zoneName, double value)
{
    if(zoneName == "Home")
    {
        activeZones_.clear();
        activeSelectedTrackSendsZones_.clear();
        activeSelectedTrackReceivesZones_.clear();
        activeSelectedTrackFXMenuZones_.clear();
        activeSelectedTrackFXMenuZones_.clear();
        activeSelectedTrackFXMenuFXZones_.clear();
        activeFocusedFXZones_.clear();
        
        for(auto widget : widgets_)
            widget->ClearAllQueues();
        
        LoadDefaultZoneOrder();
        
        if(homeZone_ != nullptr)
            homeZone_->Activate();
    }
    else
    {
        GetZone(zoneName);

        if(zonesByName_.count(zoneName) > 0)
        {
            Zone* zone = zonesByName_[zoneName];
            
            if(value == 1) // adding
            {
                zone->Activate(activeZones);
            }
            else // removing
            {
                zone->Deactivate();
                
                auto it = find(activeZones->begin(),activeZones->end(), zone);
                
                if ( it != activeZones->end() )
                    activeZones->erase(it);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename, string zoneFolder)
{
    ProcessWidgetFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename, this);
    InitHardwiredWidgets();
    Initialize();
    InitZones(zoneFolder);
    MakeHomeDefault();
    ForceClearAllWidgets();
    GetPage()->ForceRefreshTimeDisplay();
}

void Midi_ControlSurface::ProcessMidiMessage(const MIDI_event_ex_t* evt)
{
    bool isMapped = false;
    
    // At this point we don't know how much of the message comprises the key, so try all three
    if(Midi_CSIMessageGeneratorsByMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]) > 0)
    {
        isMapped = true;
        for( auto generator : Midi_CSIMessageGeneratorsByMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100 + evt->midi_message[2]])
            generator->ProcessMidiMessage(evt);
    }
    else if(Midi_CSIMessageGeneratorsByMessage_.count(evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100) > 0)
    {
        isMapped = true;
        for( auto generator : Midi_CSIMessageGeneratorsByMessage_[evt->midi_message[0] * 0x10000 + evt->midi_message[1] * 0x100])
            generator->ProcessMidiMessage(evt);
    }
    else if(Midi_CSIMessageGeneratorsByMessage_.count(evt->midi_message[0] * 0x10000) > 0)
    {
        isMapped = true;
        for( auto generator : Midi_CSIMessageGeneratorsByMessage_[evt->midi_message[0] * 0x10000])
            generator->ProcessMidiMessage(evt);
    }
    
    if(TheManager->GetSurfaceRawInDisplay() || (! isMapped && TheManager->GetSurfaceInDisplay()))
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "IN <- %s %02x  %02x  %02x \n", name_.c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
        DAW::ShowConsoleMsg(buffer);
    }
}

void Midi_ControlSurface::SendMidiMessage(MIDI_event_ex_t* midiMessage)
{
    if(midiOutput_)
        midiOutput_->SendMsg(midiMessage, -1);
    
    string output = "OUT->" + name_ + " ";
    
    for(int i = 0; i < midiMessage->size; i++)
    {
        char buffer[32];
        
        snprintf(buffer, sizeof(buffer), "%02x ", midiMessage->midi_message[i]);
        
        output += buffer;
    }
    
    output += "\n";

    if(TheManager->GetSurfaceOutDisplay())
        DAW::ShowConsoleMsg(output.c_str());
}

void Midi_ControlSurface::SendMidiMessage(int first, int second, int third)
{
    if(midiOutput_)
        midiOutput_->Send(first, second, third, -1);
    
    if(TheManager->GetSurfaceOutDisplay())
    {
        char buffer[250];
        snprintf(buffer, sizeof(buffer), "%s  %02x  %02x  %02x \n", ("OUT->" + name_).c_str(), first, second, third);
        DAW::ShowConsoleMsg(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// OSC_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSC_ControlSurface::InitWidgets(string templateFilename, string zoneFolder)
{
    ProcessWidgetFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/OSC/" + templateFilename, this);
    InitHardwiredWidgets();
    InitZones(zoneFolder);
    MakeHomeDefault();
    ForceClearAllWidgets();
    GetPage()->ForceRefreshTimeDisplay();
}

void OSC_ControlSurface::ProcessOSCMessage(string message, double value)
{
    if(CSIMessageGeneratorsByMessage_.count(message) > 0)
        CSIMessageGeneratorsByMessage_[message]->ProcessMessage(value);
    
    if(TheManager->GetSurfaceInDisplay())
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
    
    if(TheManager->GetSurfaceOutDisplay())
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
    
    if(TheManager->GetSurfaceOutDisplay())
    {
        if(TheManager->GetSurfaceOutDisplay())
            DAW::ShowConsoleMsg(("OUT->" + name_ + " " + oscAddress + " " + to_string(value) + "\n").c_str());
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

void Midi_ControlSurface::InitializeMCU()
{
    vector<vector<int>> sysExLines;
    
    sysExLines.push_back({0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x00, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x21, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x00, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x01, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x02, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x03, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x04, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x05, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x06, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x07, 0x01, 0xF7});
    
    struct
    {
        MIDI_event_ex_t evt;
        char data[BUFSZ]{};
    } midiSysExData;
    
    for(auto line : sysExLines)
    {
        memset(midiSysExData.data, 0, sizeof(midiSysExData.data));
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;

        for(auto value : line)
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = value;
        
        SendMidiMessage(&midiSysExData.evt);
    }
}

void Midi_ControlSurface::InitializeMCUXT()
{
    vector<vector<int>> sysExLines;
    
    sysExLines.push_back({0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x00, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x21, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x00, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x01, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x02, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x03, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x04, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x05, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x06, 0x01, 0xF7});
    sysExLines.push_back({0xF0, 0x00, 0x00, 0x66, 0x15, 0x20, 0x07, 0x01, 0xF7});
    
    struct
    {
        MIDI_event_ex_t evt;
        char data[BUFSZ]{};
    } midiSysExData;
    
    for(auto line : sysExLines)
    {
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        
        for(auto value : line)
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = value;
        
        SendMidiMessage(&midiSysExData.evt);
    }

}

