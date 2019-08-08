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

// GAW TBD OSC integration

using namespace oscpkt;



const int PORT_NUM = 9109;

void runServer()
{
    UdpSocket sock;
    sock.bindTo(PORT_NUM);
    if (!sock.isOk())
    {
        //cerr << "Error opening port " << PORT_NUM << ": " << sock.errorMessage() << "\n";
    }
    else
    {
        //cout << "Server started, will listen to packets on port " << PORT_NUM << std::endl;
        PacketReader pr;
        PacketWriter pw;
        while (sock.isOk())
        {
            if (sock.receiveNextPacket(30 /* timeout, in ms */))
            {
                pr.init(sock.packetData(), sock.packetSize());
                oscpkt::Message *msg;
                while (pr.isOk() && (msg = pr.popMessage()) != 0)
                {
                    int iarg;
                    if (msg->match("/ping").popInt32(iarg).isOkNoMoreArgs())
                    {
                        //cout << "Server: received /ping " << iarg << " from " << sock.packetOrigin() << "\n";
                        Message repl; repl.init("/pong").pushInt32(iarg+1);
                        pw.init().addMessage(repl);
                        sock.sendPacketTo(pw.packetData(), pw.packetSize(), sock.packetOrigin());
                    }
                    else
                    {
                        //cout << "Server: unhandled message: " << *msg << "\n";
                    }
                }
            }
        }
    }
}

// GAW TBD OSC integration





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiChannelInput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int channel_ = 0;
    midi_Input* midiInput_ = nullptr;
    
    MidiChannelInput(int channel, midi_Input* midiInput)
    : channel_(channel), midiInput_(midiInput) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MidiChannelOutput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int channel_ = 0;
    midi_Output* midiOutput_ = nullptr;
    
    MidiChannelOutput(int channel, midi_Output* midiOutput)
    : channel_(channel), midiOutput_(midiOutput) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi I/O Manager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static map<int, MidiChannelInput*> midiInputs_;
static map<int, MidiChannelOutput*> midiOutputs_;

static midi_Input* GetMidiInputForChannel(int inputChannel)
{
    if(midiInputs_.count(inputChannel) > 0)
        return midiInputs_[inputChannel]->midiInput_; // return existing
    
    // otherwise make new
    midi_Input* newInput = DAW::CreateMIDIInput(inputChannel);
    
    if(newInput)
    {
        newInput->start();
        midiInputs_[inputChannel] = new MidiChannelInput(inputChannel, newInput);
        return newInput;
    }
    
    return nullptr;
}

static midi_Output* GetMidiOutputForChannel(int outputChannel)
{
    if(midiOutputs_.count(outputChannel) > 0)
        return midiOutputs_[outputChannel]->midiOutput_; // return existing
    
    // otherwise make new
    midi_Output* newOutput = DAW::CreateMIDIOutput(outputChannel, false, NULL);
    
    if(newOutput)
    {
        midiOutputs_[outputChannel] = new MidiChannelOutput(outputChannel, newOutput);
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
                surface->SetNumFXSlots(rangeEnd - rangeBegin + 1);
            
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
        trackNavigators[channelName] = surface->GetPage()->AddTrackNavigator();
    
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
        
        if(tokens.size() == 1 && tokens[0] == "FocusedFXTrackNavigator")
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
                            TrackNavigator* trackNavigator = nullptr;
                            
                            if(hasTrackNavigator)
                                trackNavigator = expandedTrackNavigators[i];
                            else if(hasSelectedTrackNavigator)
                                trackNavigator = new SelectedTrackNavigator(surface->GetPage()->GetTrackNavigationManager());
                            else if(hasFocusedFXTrackNavigator)
                                trackNavigator = new FocusedFXTrackNavigator(surface->GetPage()->GetTrackNavigationManager());
                            
                            widgetActionManagerForWidget[widget] = new WidgetActionManager(widget, trackNavigator);

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

void ProcessWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens,  Midi_ControlSurface* surface, vector<Widget*> &widgets)
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
                new Press_Midi_ControlSignalGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "PressRelease" && tokens.size() == 7)
                new PressRelease_Midi_ControlSignalGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])), new MIDI_event_ex_t(strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6])));
            else if(widgetClass == "Fader14Bit" && tokens.size() == 4)
                new Fader14Bit_Midi_ControlSignalGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Fader7Bit" && tokens.size() == 4)
                new Fader7Bit_Midi_ControlSignalGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Encoder" && tokens.size() == 4)
                new Encoder_Midi_ControlSignalGenerator(surface, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            
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
                    ProcessWidget(lineNumber, file, tokens, (Midi_ControlSurface*)surface, widgets);
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
    actions_["NoAction"] =                          [this](WidgetActionManager* manager, vector<string> params) { return new NoAction(manager); };
    actions_["Reaper"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new ReaperAction(manager, params); };
    actions_["FXParam"] =                           [this](WidgetActionManager* manager, vector<string> params) { return new FXParam(manager, params); };
    actions_["FXParamNameDisplay"] =                [this](WidgetActionManager* manager, vector<string> params) { return new FXParamNameDisplay(manager, params); };
    actions_["FXParamValueDisplay"] =               [this](WidgetActionManager* manager, vector<string> params) { return new FXParamValueDisplay(manager, params); };
    actions_["FXGainReductionMeter"] =              [this](WidgetActionManager* manager, vector<string> params) { return new FXGainReductionMeter(manager, params); };
    actions_["TrackVolume"] =                       [this](WidgetActionManager* manager, vector<string> params) { return new TrackVolume(manager); };
    actions_["MasterTrackVolume"] =                 [this](WidgetActionManager* manager, vector<string> params) { return new MasterTrackVolume(manager); };
    actions_["TrackSendVolume"] =                   [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendVolume(manager); };
    actions_["TrackSendPan"] =                      [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendPan(manager); };
    actions_["TrackSendMute"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendMute(manager); };
    actions_["TrackSendInvertPolarity"] =           [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendInvertPolarity(manager); };
    actions_["TrackSendPrePost"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendPrePost(manager); };
    actions_["TrackPan"] =                          [this](WidgetActionManager* manager, vector<string> params) { return new TrackPan(manager, params); };
    actions_["TrackPanWidth"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new TrackPanWidth(manager, params); };
    actions_["TrackNameDisplay"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new TrackNameDisplay(manager); };
    actions_["TrackVolumeDisplay"] =                [this](WidgetActionManager* manager, vector<string> params) { return new TrackVolumeDisplay(manager); };
    actions_["TrackSendNameDisplay"] =              [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendNameDisplay(manager); };
    actions_["TrackSendVolumeDisplay"] =            [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendVolumeDisplay(manager); };
    actions_["TrackPanDisplay"] =                   [this](WidgetActionManager* manager, vector<string> params) { return new TrackPanDisplay(manager); };
    actions_["TrackPanWidthDisplay"] =              [this](WidgetActionManager* manager, vector<string> params) { return new TrackPanWidthDisplay(manager); };
    actions_["TimeDisplay"] =                       [this](WidgetActionManager* manager, vector<string> params) { return new TimeDisplay(manager); };
    actions_["Rewind"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new Rewind(manager); };
    actions_["FastForward"] =                       [this](WidgetActionManager* manager, vector<string> params) { return new FastForward(manager); };
    actions_["Play"] =                              [this](WidgetActionManager* manager, vector<string> params) { return new Play(manager); };
    actions_["Stop"] =                              [this](WidgetActionManager* manager, vector<string> params) { return new Stop(manager); };
    actions_["Record"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new Record(manager); };
    actions_["TrackSelect"] =                       [this](WidgetActionManager* manager, vector<string> params) { return new TrackSelect(manager); };
    actions_["TrackUniqueSelect"] =                 [this](WidgetActionManager* manager, vector<string> params) { return new TrackUniqueSelect(manager); };
    actions_["MasterTrackUniqueSelect"] =           [this](WidgetActionManager* manager, vector<string> params) { return new MasterTrackUniqueSelect(manager); };
    actions_["TrackRangeSelect"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new TrackRangeSelect(manager); };
    actions_["TrackRecordArm"] =                    [this](WidgetActionManager* manager, vector<string> params) { return new TrackRecordArm(manager); };
    actions_["TrackMute"] =                         [this](WidgetActionManager* manager, vector<string> params) { return new TrackMute(manager); };
    actions_["TrackSolo"] =                         [this](WidgetActionManager* manager, vector<string> params) { return new TrackSolo(manager); };
    actions_["TrackTouch"] =                        [this](WidgetActionManager* manager, vector<string> params) { return new SetTrackTouch(manager); };
    actions_["MasterTrackTouch"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new SetMasterTrackTouch(manager); };
    actions_["CycleTimeline"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new CycleTimeline(manager); };
    actions_["TrackOutputMeter"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeter(manager, params); };
    actions_["TrackOutputMeterAverageLR"] =         [this](WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterAverageLR(manager); };
    actions_["TrackOutputMeterMaxPeakLR"] =         [this](WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterMaxPeakLR(manager); };
    actions_["MasterTrackOutputMeter"] =            [this](WidgetActionManager* manager, vector<string> params) { return new MasterTrackOutputMeter(manager, params); };
    actions_["SetShowFXWindows"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new SetShowFXWindows(manager); };
    actions_["ToggleScrollLink"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new ToggleScrollLink(manager, params); };
    actions_["CycleTimeDisplayModes"] =             [this](WidgetActionManager* manager, vector<string> params) { return new CycleTimeDisplayModes(manager); };
    actions_["NextPage"] =                          [this](WidgetActionManager* manager, vector<string> params) { return new GoNextPage(manager); };
    actions_["GoPage"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new class GoPage(manager, params); };
    actions_["GoZone"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new GoZone(manager, params); };
    actions_["SelectTrackRelative"] =               [this](WidgetActionManager* manager, vector<string> params) { return new SelectTrackRelative(manager, params); };
    actions_["TrackBank"] =                         [this](WidgetActionManager* manager, vector<string> params) { return new TrackBank(manager, params); };
    actions_["Shift"] =                             [this](WidgetActionManager* manager, vector<string> params) { return new SetShift(manager); };
    actions_["Option"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new SetOption(manager); };
    actions_["Control"] =                           [this](WidgetActionManager* manager, vector<string> params) { return new SetControl(manager); };
    actions_["Alt"] =                               [this](WidgetActionManager* manager, vector<string> params) { return new SetAlt(manager); };
    actions_["TogglePin"] =                         [this](WidgetActionManager* manager, vector<string> params) { return new TogglePin(manager); };
    actions_["ToggleMapSends"] =                    [this](WidgetActionManager* manager, vector<string> params) { return new ToggleMapSends(manager); };
    actions_["ToggleMapFX"] =                       [this](WidgetActionManager* manager, vector<string> params) { return new ToggleMapFX(manager); };
    actions_["MapSelectedTrackSendsToWidgets"] =    [this](WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackSendsToWidgets(manager); };
    actions_["MapSelectedTrackFXToWidgets"] =       [this](WidgetActionManager* manager, vector<string> params) { return new MapSelectedTrackFXToWidgets(manager); };
    actions_["MapFocusedTrackFXToWidgets"] =        [this](WidgetActionManager* manager, vector<string> params) { return new MapFocusedTrackFXToWidgets(manager); };
    actions_["TrackAutoMode"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new TrackAutoMode(manager, params); };
    actions_["GlobalAutoMode"] =                    [this](WidgetActionManager* manager, vector<string> params) { return new GlobalAutoMode(manager, params); };
}

void Manager::Init()
{
    pages_.clear();
    
    bool midiInMonitor = false;
    bool midiOutMonitor = false;
    VSTMonitor_ = false;
    
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
                if(tokens[0] == MidiInMonitorToken)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    if(tokens[1] == "On")
                        midiInMonitor = true;
                }
                else if(tokens[0] == MidiOutMonitorToken)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    if(tokens[1] == "On")
                        midiOutMonitor = true;
                }
                else if(tokens[0] == VSTMonitorToken)
                {
                    if(tokens.size() != 2)
                        continue;
                    
                    if(tokens[1] == "On")
                        VSTMonitor_ = true;
                }
                else if(tokens[0] == PageToken)
                {
                    if(tokens.size() != 8)
                        continue;
                    
                    currentPage = new Page(tokens[1], tokens[2] == "FollowMCP" ? true : false, tokens[3] == "SynchPages" ? true : false, tokens[4] == "UseTrackColoring" ? true : false, atoi(tokens[5].c_str()), atoi(tokens[6].c_str()), atoi(tokens[7].c_str()));
                    pages_.push_back(currentPage);
                    
                }
                else if(tokens[0] == MidiSurfaceToken)
                {
                    if(tokens.size() != 7)
                        continue;
                    
                    int channelIn = atoi(tokens[2].c_str());
                    int channelOut = atoi(tokens[3].c_str());
                    
                    if(currentPage)
                        currentPage->AddSurface(new Midi_ControlSurface(currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForChannel(channelIn), GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor, tokens[6] == "UseZoneLink" ? true : false));
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
    if(widgetActionManager_ != nullptr)
        widgetActionManager_->DoAction(value);
}

void Widget::DoRelativeAction(double value)
{
    if(widgetActionManager_ != nullptr)
        widgetActionManager_->DoAction(lastValue_ + value);
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
// TrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
void TrackNavigator::Pin()
{
    if(isChannelPinned_)
        return;
        
    pinnedTrack_ = GetTrack();

    isChannelPinned_ = true;
    
    manager_->PinTrackToChannel(pinnedTrack_, channelNum_);
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
// FocusedFXTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* FocusedFXTrackNavigator::GetTrack()
{
    int trackNumber = 0;
    int itemNumber = 0;
    int fxIndex = 0;
    
    if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &fxIndex) == 1) // Track FX
    {
        if(trackNumber > 0)
            return DAW::CSurf_TrackFromID(trackNumber, manager_->GetPage()->GetFollowMCP());
        else
            return nullptr;
    }
    else
        return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SendsActivationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void SendsActivationManager::MapSelectedTrackSendsToWidgets(map<string, Zone*> &zones)
{
    for(auto zone : activeSendZones_)
        zone->Deactivate();
    
    activeSendZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();

    if(selectedTrack == nullptr)
        return;

    int numTrackSends = DAW::GetTrackNumSends(selectedTrack, 0);
    
    for(int i = 0; i < numSendSlots_; i ++)
    {
        string zoneName = "Send" + to_string(i + 1);
        
        if(zones.count(zoneName) > 0)
        {
            Zone* zone =  zones[zoneName];

            if(shouldMapSends_)
            {
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
            else
            {
                zone->Deactivate();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FXActivationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXActivationManager::MapSelectedTrackFXToWidgets(map<string, Zone*> &zones)
{
    mapsSelectedTrackFXToWidgets_ = true;
    
    for(auto zone : activeSelectedTrackFXZones_)
        zone->Deactivate();
    
    activeSelectedTrackFXZones_.clear();
    DeleteFXWindows();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    for(int i = 0; i < DAW::TrackFX_GetCount(selectedTrack); i++)
    {
        char FXName[BUFSZ];
        
        DAW::TrackFX_GetFXName(selectedTrack, i, FXName, sizeof(FXName));
        
        if(zones.count(FXName) > 0 && ! zones[FXName]->GetHasFocusedFXTrackNavigator())
        {
            Zone* zone = zones[FXName];

            if(shouldMapSelectedFX_)
            {
                zone->Activate(i);
                activeSelectedTrackFXZones_.push_back(zone);
                openFXWindows_.push_back(FXWindow(FXName, selectedTrack, i));
            }
            else
            {
                zone->Deactivate();
            }
        }
    }
    
    OpenFXWindows();
}

void FXActivationManager::MapSelectedTrackFXToMenu(map<string, Zone*> &zones)
{
    for(auto zone : activeFXMenuZones_)
        zone->Deactivate();
    
    activeFXMenuZones_.clear();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    if(selectedTrack == nullptr)
        return;
    
    int numTrackFX = DAW::TrackFX_GetCount(selectedTrack);
    
    for(int i = 0; i < numFXlots_; i ++)
    {
        string zoneName = "FXMenu" + to_string(i + 1);
        
        if(zones.count(zoneName) > 0)
        {
            Zone* zone =  zones[zoneName];
            
            if(shouldMapFXMenus_)
            {
                if(i < numTrackFX)
                {
                    zone->Activate(i);
                    activeFXMenuZones_.push_back(zone);
                }
                else
                {
                    zone->ActivateNoAction(i);
                    activeFXMenuZones_.push_back(zone);
                    zone->SetWidgetsToZero();
                }
            }
            else
            {
                zone->Deactivate();
            }
        }
    }
}

void FXActivationManager::MapFocusedTrackFXToWidgets(map<string, Zone*> &zones)
{
    mapsFocusedTrackFXToWidgets_ = true;
    
    int trackNumber = 0;
    int itemNumber = 0;
    int fxIndex = 0;
    MediaTrack* focusedTrack = nullptr;
    
    if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &fxIndex) == 1)
        if(trackNumber > 0)
            focusedTrack = DAW::CSurf_TrackFromID(trackNumber, surface_->GetPage()->GetFollowMCP());
 
    for(auto zone : activeFocusedFXZones_)
        zone->Deactivate();
    
    activeFocusedFXZones_.clear();

    if(focusedTrack)
    {
        char FXName[BUFSZ];
        DAW::TrackFX_GetFXName(focusedTrack, fxIndex, FXName, sizeof(FXName));
        if(zones.count(FXName) > 0 && zones[FXName]->GetHasFocusedFXTrackNavigator())
        {
            Zone* zone = zones[FXName];
            zone->Activate(fxIndex);
            activeFocusedFXZones_.push_back(zone);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlSurface::ControlSurface(Page* page, const string name, bool useZoneLink) : page_(page), name_(name), useZoneLink_(useZoneLink)
{
    FXActivationManager_ = new FXActivationManager(this);
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

int ControlSurface::GetParentZoneIndex(Zone* childZone)
{
    for(auto zone : FXActivationManager_->GetActiveZones())
        if(childZone->GetParentZoneName() == zone->GetName())
            return zone->GetZoneIndex();

    for(auto zone : sendsActivationManager_->GetActiveZones())
        if(childZone->GetParentZoneName() == zone->GetName())
            return zone->GetZoneIndex();
    
    return 0;
}

void ControlSurface::GoZone(string zoneName)
{
    if(GetUseZoneLink())
        page_->GoZone(zoneName);
    else
        ActivateZone(zoneName);
}

void ControlSurface::MapSelectedTrackSendsToWidgets()
{
    if(GetUseZoneLink())
        page_->MapSelectedTrackSendsToWidgets();
    else
        ActivateSelectedTrackSends();
}

void ControlSurface::ToggleMapSends()
{
    if(GetUseZoneLink())
        page_->ToggleMapSends();
    else
        ActivateToggleMapSends();
}

void ControlSurface::ToggleMapFX()
{
    if(GetUseZoneLink())
        page_->ToggleMapSelectedFX();
    else
        ActivateToggleMapSelectedFX();
}

void ControlSurface::MapSelectedTrackFXToWidgets()
{
    if(GetUseZoneLink())
        page_->MapSelectedTrackFXToWidgets();
    else
        ActivateSelectedTrackFX();
}

void ControlSurface::MapFocusedTrackFXToWidgets()
{
    if(GetUseZoneLink())
        page_->MapFocusedTrackFXToWidgets();
    else
        ActivateFocusedTrackFX();
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

void ControlSurface::ActivateZone(string zoneName)
{
    if(zones_.count(zoneName) > 0)
        zones_[zoneName]->Activate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename, (ControlSurface*)this, widgets_);
    
    // Add the "hardcoded" widgets
    widgets_.push_back(new Widget(this, "OnTrackSelection"));
    widgets_.push_back(new Widget(this, "OnFXFocus"));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Action::Action(WidgetActionManager* widgetActionManager) : widgetActionManager_(widgetActionManager)
{
    page_ = widgetActionManager_->GetWidget()->GetSurface()->GetPage();
    widget_ = widgetActionManager_->GetWidget();
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
