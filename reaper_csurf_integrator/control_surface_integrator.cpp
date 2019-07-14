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

extern Manager* TheManager;





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

void ExpandZone(vector<string> tokens, string filePath, vector<Zone*> &expandedZones, vector<string> &expandedZonesIds, ControlSurface* surface)
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
            
            for(int i = rangeBegin; i <= rangeEnd; i++)
                expandedZonesIds.push_back(to_string(i));
            
            for(int i = 0; i <= rangeEnd - rangeBegin; i++)
            {
                Zone* zone = new Zone(surface, zoneBaseName + expandedZonesIds[i], filePath);
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

        Zone* zone = new Zone(surface, tokens[1], filePath);
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
        trackNavigators[channelName] = surface->AddTrackNavigator();
    
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
    
    vector<Zone*> expandedZones;
    vector<string> expandedZonesIds;
    
    ExpandZone(passedTokens, filePath, expandedZones, expandedZonesIds, surface);
    
    map<Widget*, WidgetActionManager*> widgetActionManagerForWidget;

    bool hasTrackNavigator = false;
    vector<TrackNavigator*> expandedTrackNavigators;
    
    bool hasSelectedTrackNavigator = false;
    bool hasFocusedFXTrackNavigator = false;

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
                            TrackNavigator* trackNavigator = nullptr;
                            
                            if(hasTrackNavigator)
                                trackNavigator = expandedTrackNavigators[i];
                            else if(hasSelectedTrackNavigator)
                                trackNavigator = new SelectedTrackNavigator(surface);
                            else if(hasFocusedFXTrackNavigator)
                                trackNavigator = new FocusedFXTrackNavigator(surface);
                            
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
    //actions_["ToggleMapSends"] = [this](WidgetActionManager* manager, vector<string> params) { return new SurfaceAction(manager, actions_[params[0]]); };

    actions_["NoAction"] =                          [this](WidgetActionManager* manager, vector<string> params) { return new Action(manager); };
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
    actions_["TrackTouch"] =                          [this](WidgetActionManager* manager, vector<string> params) { return new SetTrackTouch(manager); };
    actions_["MasterTrackTouch"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new SetMasterTrackTouch(manager); };
    actions_["CycleTimeline"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new CycleTimeline(manager); };
    actions_["TrackOutputMeter"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeter(manager, params); };
    actions_["TrackOutputMeterAverageLR"] =         [this](WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterAverageLR(manager); };
    actions_["TrackOutputMeterMaxPeakLR"] =         [this](WidgetActionManager* manager, vector<string> params) { return new TrackOutputMeterMaxPeakLR(manager); };
    actions_["MasterTrackOutputMeter"] =            [this](WidgetActionManager* manager, vector<string> params) { return new MasterTrackOutputMeter(manager, params); };
    actions_["SetShowFXWindows"] =                  [this](WidgetActionManager* manager, vector<string> params) { return new SetShowFXWindows(manager); };
    actions_["SetScrollLink"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new SetScrollLink(manager); };
    actions_["CycleTimeDisplayModes"] =             [this](WidgetActionManager* manager, vector<string> params) { return new CycleTimeDisplayModes(manager); };
    actions_["NextPage"] =                          [this](WidgetActionManager* manager, vector<string> params) { return new GoNextPage(manager); };
    actions_["GoPage"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new class GoPage(manager, params); };
    actions_["GoZone"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new class GoZone(manager, params); };
    actions_["SelectTrackRelative"] =               [this](WidgetActionManager* manager, vector<string> params) { return new SelectTrackRelative(manager, params); };
    actions_["TrackBank"] =                         [this](WidgetActionManager* manager, vector<string> params) { return new TrackBank(manager, params); };
    actions_["TrackSendBank"] =                     [this](WidgetActionManager* manager, vector<string> params) { return new TrackSendBank(manager, params); };
    actions_["Shift"] =                             [this](WidgetActionManager* manager, vector<string> params) { return new SetShift(manager); };
    actions_["Option"] =                            [this](WidgetActionManager* manager, vector<string> params) { return new SetOption(manager); };
    actions_["Control"] =                           [this](WidgetActionManager* manager, vector<string> params) { return new SetControl(manager); };
    actions_["Alt"] =                               [this](WidgetActionManager* manager, vector<string> params) { return new SetAlt(manager); };
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
        
        for(auto page : pages_)
            page->Init();
        
        char buffer[BUFSZ];
        if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), "PageIndex", buffer, sizeof(buffer)))
            currentPageIndex_ = atol(buffer);
        
        if(currentPageIndex_ >= pages_.size())
            currentPageIndex_ = pages_.size() - 1;
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
// Zone
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Zone::Deactivate()
{
    for(auto widgetActionManager : widgetActionManagers_)
        {
            widgetActionManager->GetWidget()->SetWidgetActionManager(nullptr);
            widgetActionManager->GetWidget()->Reset();
        }
    
    for(auto zone : includedZones_)
        zone->Deactivate();
}

void Zone::Activate()
{
    for(auto widgetActionManager : widgetActionManagers_)
        widgetActionManager->Activate();
    
    for(auto zone : includedZones_)
        zone->Activate();
}

void Zone::Activate(int fxIndex)
{
    for(auto widgetActionManager : widgetActionManagers_)
        widgetActionManager->Activate(fxIndex);
    
    for(auto zone : includedZones_)
        zone->Activate(fxIndex);
}

void Zone::Activate(MediaTrack* track, int sendsIndex)
{
    for(auto widgetActionManager : widgetActionManagers_)
        widgetActionManager->Activate(track, sendsIndex);
    
    for(auto zone : includedZones_)
        zone->Activate(track, sendsIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* TrackNavigator::GetTrack()
{
    return surface_->GetPage()->GetTrackFromChannel(channelNum_ + surface_->GetSurfacChannelOffset());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectedTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* SelectedTrackNavigator::GetTrack()
{
    if(DAW::CountSelectedTracks(nullptr) != 1)
        return nullptr;
    
    for(int i = 0; i < surface_->GetPage()->GetNumTracks(); i++)
    {
        if(MediaTrack* track = surface_->GetPage()->GetTrackFromId(i))
        {
            int flags = 0;
            
            DAW::GetTrackInfo(track, &flags);
            
            if(flags & 0x02) // Selected
                return track;
        }
    }
    
    return nullptr;
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
        return surface_->GetPage()->GetTrackFromId(trackNumber);
    else
        return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlSurface::ControlSurface(Page* page, const string name, bool useZoneLink) : page_(page), name_(name), useZoneLink_(useZoneLink)
{
    surfaceChannelOffset_ = page->GetTotalNumChannels();
    FXActivationManager_ = new FXActivationManager(this);
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

void ControlSurface::GoZone(string zoneName)
{
    if(zones_.count(zoneName) > 0)
        ActivateZone(zoneName);
}

TrackNavigator* ControlSurface::AddTrackNavigator()
{
    page_->AddTrackNavigator();
    
    int channelNum = trackNavigators_.size();
    trackNavigators_.push_back(new TrackNavigator(this, channelNum));
    return trackNavigators_[channelNum];
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
// WidgetActionManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string WidgetActionManager::GetModifiers()
{
    if(widget_->GetIsModifier())
        return NoModifiers; // Modifier Widgets cannot have Modifiers
    else
        return widget_->GetSurface()->GetPage()->GetModifiers(widget_->GetTrack());
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

void WidgetActionManager::DoAction(double value)
{
    if(actions_.count(GetModifiers()) > 0)
        for(auto action : actions_[GetModifiers()])
            action->DoAction(value);
}

void WidgetActionManager::Activate()
{
    if(actions_.count(GetModifiers()) > 0)
        for(auto action : actions_[GetModifiers()])
            action->Activate(this);
}

void WidgetActionManager::Activate(int actionIndex)
{
    if(actions_.count(GetModifiers()) > 0)
        for(auto action : actions_[GetModifiers()])
        {
            action->SetIndex(actionIndex);
            action->Activate(this);
        }
}

void WidgetActionManager::Activate(MediaTrack* track, int actionIndex)
{
    if(actions_.count(GetModifiers()) > 0)
        for(auto action : actions_[GetModifiers()])
        {
            action->SetTrack(track);
            action->SetIndex(actionIndex);
            action->Activate(this);
        }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void TrackNavigationManager::Init()
{
    char buffer[BUFSZ];
    if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "BankOffset").c_str(), buffer, sizeof(buffer)))
        trackOffset_ = atol(buffer);
}

void TrackNavigationManager::OnTrackSelection()
{
    if(scrollLink_)
    {
        // Make sure selected track is visble on the control surface
        int low = trackOffset_;
        int high = low + totalNumChannels_ - 1;
        
        MediaTrack* selectedTrack = GetSelectedTrack();
        
        if(selectedTrack == nullptr)
            return;
        
        int selectedTrackOffset = DAW::CSurf_TrackToID(selectedTrack, followMCP_);
        
        if(selectedTrackOffset < low)
            TheManager->AdjustTrackBank(page_, selectedTrackOffset - low);
        if(selectedTrackOffset > high)
            TheManager->AdjustTrackBank(page_, selectedTrackOffset - high);
    }
}

void TrackNavigationManager::TrackListChanged()
{
    // GAW TBD -- Future support for pinned tracks here
}

void TrackNavigationManager::AdjustTrackBank(int stride)
{
    int numTracks = GetNumTracks();

    if(numTracks <= totalNumChannels_)
        return;
    
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ <  0)
        trackOffset_ =  0;
    
    int top = numTracks - totalNumChannels_;
    
    if(trackOffset_ >  top)
        trackOffset_ = top;
    
    if(previousTrackOffset != trackOffset_)
    {
        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "BankOffset").c_str(), to_string(trackOffset_).c_str());
        DAW::MarkProjectDirty(nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SendsNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
int SendsNavigationManager::GetMaxSends()
{
    int maxSends = 0;
    
    for(int i = 0; i < page_->GetNumTracks(); i++)
    {
        MediaTrack* track = page_->GetTrackFromId(i);
        
        int numSends = DAW::GetTrackNumSends(track, 0);
        
        if(numSends > maxSends)
            maxSends = numSends;
    }
    
    return maxSends;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SendsActivationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void SendsActivationManager::ActivateSendsZones(ControlSurface* surface, MediaTrack* selectedTrack)
{
    if(selectedTrack == nullptr)
        return;
    
    int flags;
    
    DAW::GetTrackInfo(selectedTrack, &flags);
    
    if((flags & 0x02) && shouldMapSends_) // track is selected -- not deselected and shouldMapSends_ == true
    {
        
        // GAW TBD -- Zero all Sends Zones for this surface (and all if zoneLink)
        
        for(int i = 0; i < DAW::GetTrackNumSends(selectedTrack, 0); i++)
        {
            string zoneName = "Send" + to_string(i + 1);
            
            ActivateSendsZone(surface, selectedTrack, i, zoneName);
        }
    }
}

void SendsActivationManager::ActivateSendsZone(ControlSurface* surface, MediaTrack* selectedTrack, int sendsIndex, string zoneName)
{
    if(! surface->GetUseZoneLink())
    {
        surface->ActivateSendsZone(zoneName, selectedTrack, sendsIndex);
    }
    else
    {
        for(auto surface : page_->GetSurfaces())
            if(surface->GetUseZoneLink())
                surface->ActivateSendsZone(zoneName, selectedTrack, sendsIndex);
    }
}

void SendsActivationManager::MapSelectedTrackSendsToWidgets(ControlSurface* surface, MediaTrack* selectedTrack)
{
    ActivateSendsZones(surface, selectedTrack);
}

void SendsActivationManager::ToggleMapSends(ControlSurface* surface)
{
    if(DAW::CountSelectedTracks(NULL) != 1)
        return;
    
    shouldMapSends_ = ! shouldMapSends_;
        
    MediaTrack* selectedTrack = nullptr;
    
    for(int i = 0; i < page_->GetNumTracks(); i++)
    {
        if(DAW::GetMediaTrackInfo_Value(page_->GetTrackFromId(i), "I_SELECTED"))
        {
            selectedTrack = page_->GetTrackFromId(i);
            break;
        }
    }
    
    ActivateSendsZones(surface, selectedTrack);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// FXActivationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXActivationManager::MapSelectedTrackFXToWidgets()
{
    DeleteFXWindows();
    
    MediaTrack* selectedTrack = surface_->GetPage()->GetSelectedTrack();
    
    int flags;
    
    DAW::GetTrackInfo(selectedTrack, &flags);
    
    if(flags & 0x02) // track is selected -- not deselected
    {
        for(int i = 0; i < DAW::TrackFX_GetCount(selectedTrack); i++)
        {
            char FXName[BUFSZ];
            
            DAW::TrackFX_GetFXName(selectedTrack, i, FXName, sizeof(FXName));
            
            if(surface_->ActivateFXZone(FXName, i))
                AddFXWindow(FXWindow(selectedTrack, i));
        }
        
        OpenFXWindows();
    }
}

void FXActivationManager::MapFocusedTrackFXToWidgets()
{
    
    
    int tracknumberOut = 0;
    int itemnumberOut = 0;
    int fxIndex = 0;
    MediaTrack* selectedTrack = nullptr;
    
    if( DAW::GetFocusedFX(&tracknumberOut, &itemnumberOut, &fxIndex) == 1)
        selectedTrack = surface_->GetPage()->GetTrackFromId(tracknumberOut);

    if(selectedTrack)
    {
        char FXName[BUFSZ];
        DAW::TrackFX_GetFXName(selectedTrack, fxIndex, FXName, sizeof(FXName));
        surface_->ActivateFXZone(FXName, fxIndex);
    }
}



/*
int start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

int duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start;


char msgBuffer[250];

sprintf(msgBuffer, "%d microseconds\n", duration);
DAW::ShowConsoleMsg(msgBuffer);

*/


/*
 // subtracts b<T> from a<T>
 template <typename T>
 static void subtract_vector(std::vector<T>& a, const std::vector<T>& b)
 {
 typename std::vector<T>::iterator       ita = a.begin();
 typename std::vector<T>::const_iterator itb = b.begin();
 typename std::vector<T>::iterator       enda = a.end();
 typename std::vector<T>::const_iterator endb = b.end();
 
 while (ita != enda)
 {
 while (itb != endb)
 {
 if (*ita == *itb)
 {
 ita = a.erase(ita);
 enda = a.end();
 itb = b.begin();
 }
 else
 ++itb;
 }
 ++ita;
 
 itb = b.begin();
 }
 }
 */




/*

void ProcessFile(string filePath, ControlSurface* surface, vector<Widget*> &widgets)
{
    int lineNumber = 0;
    
    try
    {
        //string outFilePath = filePath;
        //outFilePath[outFilePath.size() - 1] = 'x';
        
        //ofstream outfile(outFilePath);
        
        ifstream file(filePath);
        
        for (string line; getline(file, line) ; )
        {
            lineNumber++;
            
            if(line == "" || line[0] == '\r' || line[0] == '/') // ignore comment lines and blank lines
                continue;
            
            vector<string> tokens(GetTokens(line));
            
            
            
            
            
            // old .mst -> new .msx

             if(tokens.size() < 1)
             continue;
             
             outfile << "Widget " + tokens[0] + "\n";
             
             
             if(tokens.size() > 1)
             {
             if(tokens[1] == "Press" && tokens.size() == 5)
             {
             outfile << "\tPress " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if(tokens[1] == "PressFB" && tokens.size() == 8)
             {
             outfile << "\tPress " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             outfile << "\tFB_TwoState " + tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + tokens[5] + " " + tokens[6] + " " + tokens[7] + "\n";
             }
             else if(tokens[1] == "PressRelease" && tokens.size() == 8)
             {
             outfile << "\tPressRelease " + tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + tokens[5] + " " + tokens[6] + " " + tokens[7] +"\n";
             }
             else if(tokens[1] == "PressReleaseFB" && tokens.size() == 8)
             {
             outfile << "\tPressRelease " + tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + tokens[5] + " " + tokens[6] + " " + tokens[7] +"\n";
             outfile << "\tFB_TwoState " + tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + tokens[5] + " " + tokens[6] + " " + tokens[7] + "\n";
             }
             else if(tokens[1] == "Encoder" && tokens.size() == 8)
             {
             outfile << "\tEncoder " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if(tokens[1] == "EncoderFB" && tokens.size() == 8)
             {
             outfile << "\tEncoder " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             outfile << "\tFB_Encoder " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if(tokens[1] == "Fader7Bit" && tokens.size() == 8)
             {
             outfile << "\tFader7Bit " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if(tokens[1] == "Fader7BitFB" && tokens.size() == 8)
             {
             outfile << "\tFader7Bit " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             outfile << "\tFB_Fader7Bit " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if(tokens[1] == "Fader14Bit" && tokens.size() == 8)
             {
             outfile << "\tFader14Bit " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if(tokens[1] == "Fader14BitFB" && tokens.size() == 8)
             {
             outfile << "\tFader14Bit " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             outfile << "\tFB_Fader14Bit " + tokens[2] + " " + tokens[3] + " " + tokens[4] + "\n";
             }
             else if((tokens[1] == "MCUDisplayUpper" || tokens[1] == "MCUDisplayLower" || tokens[1] == "MCUXTDisplayUpper" || tokens[1] == "MCUXTDisplayLower") && tokens.size() == 3)
             {
             outfile << "\tFB_" + tokens[1] + " "  + tokens[2] + "\n";
             }
             else if((tokens[1] == "C4DisplayUpper" || tokens[1] == "C4DisplayLower") && tokens.size() == 4)
             {
             outfile << "\tFB_" + tokens[1] + " "  + tokens[2] + " " + tokens[3] + "\n";
             }
             else if(tokens[1] == "MCUTimeDisplay" && tokens.size() == 2)
             {
             outfile << "\tFB_MCUTimeDisplay\n";
             }
             else if(tokens[1] == "MCUVUMeter" && tokens.size() == 3)
             {
             outfile << "\tFB_MCUVUMeter " + tokens[2] + "\n";
             }
             else if(tokens[1] == "VUMeter" && tokens.size() == 5)
             {
             outfile << "\tFB_VUMeter " + tokens[2] + " "  + tokens[3] + " "  + tokens[4] + "\n";
             }
             else if(tokens[1] == "GainReductionMeter" && tokens.size() == 5)
             {
             outfile << "\tFB_GainReductionMeter " + tokens[2] + " "  + tokens[3] + " "  + tokens[4] + "\n";
             }
             else if(tokens[1] == "QConProXMasterVUMeter" && tokens.size() == 2)
             {
             outfile << "\tFB_QConProXMasterVUMeter\n";
             }
             }
             
             outfile << "WidgetEnd\n\n";
             

            // old .mst -> new .msx
            
            
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
 */
