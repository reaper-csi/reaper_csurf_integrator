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

static TrackNavigator* TrackNavigatorForChannel(string channel, Page* page)
{
        if(trackNavigators.count(channel) < 1)
        {
            trackNavigators[channel] = new TrackNavigator();
            page->AddTrackNavigator(trackNavigators[channel]);
        }
        
        return trackNavigators[channel];
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

static void GetWidgetNameAndModifiers(string line, string &widgetName, string &modifiers, string &customSlotName, bool &isInverted, bool &shouldToggle, double &delayAmount)
{
    istringstream modified_role(line);
    vector<string> modifier_tokens;
    vector<string> modifierSlots = { "", "", "", "", "" };
    string modifier_token;
    
    while (getline(modified_role, modifier_token, '+'))
        modifier_tokens.push_back(modifier_token);
    
    if(modifier_tokens.size() > 1)
    {
        for(int i = 0; i < modifier_tokens.size() - 1; i++)
        {
            if(modifier_tokens[i] == TrackTouch)
                modifierSlots[0] = TrackTouch;
            else if(modifier_tokens[i] == Shift)
                modifierSlots[1] = Shift;
            else if(modifier_tokens[i] == Option)
                modifierSlots[2] = Option;
            else if(modifier_tokens[i] == Control)
                modifierSlots[3] = Control;
            else if(modifier_tokens[i] == Alt)
                modifierSlots[4] = Alt;
            
            else if(modifier_tokens[i] == "Invert")
                isInverted = true;
            else if(modifier_tokens[i] == "Toggle")
                shouldToggle = true;
            else if(modifier_tokens[i] == "Hold")
                delayAmount = 1.0;

            else
                customSlotName = modifier_tokens[i];
        }
    }
    
    widgetName = modifier_tokens[modifier_tokens.size() - 1];
    
    modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3] + modifierSlots[4];
    
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
    
    map<Widget*, WidgetActionContextManager*> widgetActionContextManagerForWidget;
    map< string, map<string, map <string, TrackSlotCycleContext*>>> customSlotContexts;

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
                expandedTrackNavigators.push_back(TrackNavigatorForChannel(surface->GetName() + to_string(i), surface->GetPage()));
            
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
                string customSlotName = "";
                bool isInverted = false;
                bool shouldToggle = false;
                bool isDelayed = false;
                double delayAmount = 0.0;
                
                GetWidgetNameAndModifiers(tokens[0], widgetName, modifiers, customSlotName, isInverted, shouldToggle, delayAmount);
                
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
                    if(TheManager->IsActionContextAvailable(params[0]))
                    {
                        
                        if(widgetActionContextManagerForWidget.count(widget) < 1)
                        {
                            widgetActionContextManagerForWidget[widget] = new WidgetActionContextManager(widget);
                            
                            if(hasTrackNavigator)
                                widgetActionContextManagerForWidget[widget]->SetTrackNavigator(expandedTrackNavigators[i]);
                            else if(hasSelectedTrackNavigator)
                                widgetActionContextManagerForWidget[widget]->SetTrackNavigator(new SelectedTrackNavigator(widget->GetSurface()->GetPage()));
                            else if(hasFocusedFXTrackNavigator)
                                widgetActionContextManagerForWidget[widget]->SetTrackNavigator(new FocusedFXTrackNavigator(widget->GetSurface()->GetPage()));

                            expandedZones[i]->AddActionContextManager(widgetActionContextManagerForWidget[widget]);
                        }
                        
                        ActionContext* context = TheManager->GetActionContext(widgetActionContextManagerForWidget[widget], params);

                        // If there is a custom slot, ensure there is a wrapper (don't forget to account for modifiers)
                        // then add custom slot wrapper context to contextManager
                        // then add context to custom slot wrapper
                        if(customSlotName != "")
                        {
                            string zoneName = expandedZones[i]->GetName();
                            
                            if(customSlotContexts.count(zoneName) < 1 || customSlotContexts[zoneName].count(customSlotName) < 1 || customSlotContexts[zoneName][customSlotName].count(modifiers) < 1)
                            {
                                customSlotContexts[zoneName][customSlotName][modifiers] = new TrackSlotCycleContext(widgetActionContextManagerForWidget[widget], new Action(), customSlotName);
                                
                                widgetActionContextManagerForWidget[widget]->AddActionContext(modifiers, customSlotContexts[zoneName][customSlotName][modifiers]);
                            }

                            customSlotContexts[zoneName][customSlotName][modifiers]->AddActionContext(context);
                        }
                        else // no custom slots ? -- just add directly to contextManager
                            widgetActionContextManagerForWidget[widget]->AddActionContext(modifiers, context);
                        
                        if(isInverted)
                            context->SetIsInverted();
                        
                        if(shouldToggle)
                            context->SetShouldToggle();
                        
                        if(isDelayed)
                            context->SetDelayAmount(delayAmount * 1000.0);

                        if(params[0] == TrackTouch || params[0] == Shift || params[0] == Option || params[0] == Control || params[0] == Alt)
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
            /*
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
             
             */
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Manager::InitActionDictionary()
{
    actions_["NoAction"] = new Action();
    actions_["Reaper"] = new ReaperAction();
    actions_["FXParam"] = new FXParam();
    actions_["FXParamNameDisplay"] = new FXParamNameDisplay();
    actions_["FXParamValueDisplay"] = new FXParamValueDisplay();
    actions_["FXGainReductionMeter"] = new FXGainReductionMeter();
    actions_["TrackVolume"] = new TrackVolume();
    actions_["MasterTrackVolume"] = new MasterTrackVolume();
    actions_["TrackVolumeDB"] = new TrackVolumeDB();
    actions_["TrackPan"] = new TrackPan();
    actions_["TrackPanWidth"] = new TrackPanWidth();
    actions_["TrackPanDisplay"] = new TrackPanDisplay();
    actions_["TrackPanWidthDisplay"] = new TrackPanWidthDisplay();
    actions_["TrackNameDisplay"] = new TrackNameDisplay();
    actions_["TrackVolumeDisplay"] = new TrackVolumeDisplay();
    actions_["TrackSendVolume"] = new TrackSendVolume();
    actions_["TrackSendPan"] = new TrackSendPan();
    actions_["TrackSendMute"] = new TrackSendMute();
    actions_["TrackSendInvertPolarity"] = new TrackSendInvertPolarity();
    actions_["TrackSendPrePost"] = new TrackSendPrePost();
    actions_["TrackSendNameDisplay"] = new TrackSendNameDisplay();
    actions_["TrackSendVolumeDisplay"] = new TrackSendVolumeDisplay();
    actions_["TimeDisplay"] = new TimeDisplay();
    actions_["Rewind"] = new Rewind();
    actions_["FastForward"] = new FastForward();
    actions_["Play"] = new Play();
    actions_["Stop"] = new Stop();
    actions_["Record"] = new Record();
    actions_["TrackSelect"] = new TrackSelect();
    actions_["TrackUniqueSelect"] = new TrackUniqueSelect();
    actions_["MasterTrackUniqueSelect"] = new MasterTrackUniqueSelect();
    actions_["TrackRangeSelect"] = new TrackRangeSelect();
    actions_["TrackRecordArm"] = new TrackRecordArm();
    actions_["TrackMute"] = new TrackMute();
    actions_["TrackSolo"] = new TrackSolo();
    actions_[TrackTouch] = new SetTrackTouch();
    actions_["MasterTrackTouch"] = new SetMasterTrackTouch();
    actions_["CycleTimeline"] = new CycleTimeline();
    actions_["TrackOutputMeter"] = new TrackOutputMeter();
    actions_["TrackOutputMeterAverageLR"] = new TrackOutputMeterAverageLR();
    actions_["TrackOutputMeterMaxPeakLR"] = new TrackOutputMeterMaxPeakLR();
    actions_["MasterTrackOutputMeter"] = new MasterTrackOutputMeter();
    actions_["SetShowFXWindows"] = new SetShowFXWindows();
    actions_["SetScrollLink"] = new SetScrollLink();
    actions_["CycleTimeDisplayModes"] = new CycleTimeDisplayModes();
    actions_["NextPage"] = new class NextPage();
    actions_["GoPage"] = new class GoPage();
    actions_["ToggleZone"] = new class ToggleZone();
    actions_["GoZone"] = new class GoZone();
    actions_["GoSubZone"] = new class GoSubZone();
    actions_["SelectTrackRelative"] = new SelectTrackRelative();
    actions_["TrackBank"] = new TrackBank();
    actions_["TrackSendBank"] = new TrackSendBank();
    actions_["PinSelectedTracks"] = new PinSelectedTracks();
    actions_["UnpinSelectedTracks"] = new UnpinSelectedTracks();
    actions_["Shift"] = new SetShift();
    actions_["Option"] = new SetOption();
    actions_["Control"] = new SetControl();
    actions_["Alt"] = new SetAlt();
    actions_["TrackCycle"] = new CycleTrackSlotIndex();
    actions_["MapSelectedTrackSendsToWidgets"] = new MapSelectedTrackSendsToWidgets();
    actions_["MapSelectedTrackFXToWidgets"] = new MapSelectedTrackFXToWidgets();
    actions_["MapFocusedTrackFXToWidgets"] = new MapFocusedTrackFXToWidgets();
    actions_["ToggleShouldMapSends"] = new ToggleShouldMapSends();

    
}

void Manager::InitActionContextDictionary()
{
    InitActionDictionary();
    
    actionContexts_["NoAction"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new ActionContext(manager, actions_[params[0]]); };
    actionContexts_["Reaper"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new ReaperActionContext(manager, actions_[params[0]], params); };
    actionContexts_["FXParam"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new FXContext(manager, actions_[params[0]], params); };
    actionContexts_["FXParamNameDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new FXContext(manager, actions_[params[0]], params); };
    actionContexts_["FXParamValueDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new FXContext(manager, actions_[params[0]], params); };
    actionContexts_["FXGainReductionMeter"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackVolume"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["MasterTrackVolume"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendVolume"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendPan"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendMute"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendInvertPolarity"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendPrePost"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackVolumeDB"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackPan"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContextWithIntFeedbackParam(manager, actions_[params[0]], params); };
    actionContexts_["TrackPanWidth"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContextWithIntFeedbackParam(manager, actions_[params[0]], params); };
    actionContexts_["TrackNameDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackVolumeDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendNameDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSendVolumeDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackSendContext(manager, actions_[params[0]]); };
    actionContexts_["TrackPanDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackPanWidthDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TimeDisplay"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Rewind"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["FastForward"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Play"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Stop"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Record"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSelect"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackUniqueSelect"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["MasterTrackUniqueSelect"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["TrackRangeSelect"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackRecordArm"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackMute"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackSolo"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_[TrackTouch] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["MasterTrackTouch"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["CycleTimeline"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["TrackOutputMeter"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContextWithIntFeedbackParam(manager, actions_[params[0]], params); };
    actionContexts_["TrackOutputMeterAverageLR"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["TrackOutputMeterMaxPeakLR"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContext(manager, actions_[params[0]]); };
    actionContexts_["MasterTrackOutputMeter"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContextWithIntParam(manager, actions_[params[0]], params); };
    actionContexts_["SetShowFXWindows"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["SetScrollLink"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["CycleTimeDisplayModes"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["NextPage"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["GoPage"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContextWithStringParam(manager, actions_[params[0]], params); };
    actionContexts_["ToggleZone"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new SurfaceContextWithStringParam(manager, actions_[params[0]], params); };
    actionContexts_["GoZone"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new SurfaceContextWithStringParam(manager, actions_[params[0]], params); };
    actionContexts_["GoSubZone"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new SurfaceContextWith2StringParams(manager, actions_[params[0]], params); };
    actionContexts_["SelectTrackRelative"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContextWithIntParam(manager, actions_[params[0]], params); };
    actionContexts_["TrackBank"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContextWithIntParam(manager, actions_[params[0]], params); };
    actionContexts_["TrackSendBank"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContextWithIntParam(manager, actions_[params[0]], params); };
    actionContexts_["PinSelectedTracks"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["UnpinSelectedTracks"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Shift"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Option"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Control"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["Alt"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["TrackCycle"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new TrackContextWithStringAndIntParams(manager, actions_[params[0]], params); };
    actionContexts_["MapSelectedTrackSendsToWidgets"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["MapSelectedTrackFXToWidgets"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["MapFocusedTrackFXToWidgets"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
    actionContexts_["ToggleShouldMapSends"] = [this](WidgetActionContextManager* manager, vector<string> params) { return new GlobalContext(manager, actions_[params[0]]); };
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
MediaTrack* Widget::GetTrack()
{
    if(widgetActionContextManager_ != nullptr)
        return widgetActionContextManager_->GetTrack();
    else
        return nullptr;
}

void Widget::RequestUpdate()
{
    if(widgetActionContextManager_ != nullptr)
        widgetActionContextManager_->RequestUpdate();
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
    if(widgetActionContextManager_ != nullptr)
        widgetActionContextManager_->DoAction(value);
}

void Widget::DoRelativeAction(double value)
{
    if(widgetActionContextManager_ != nullptr)
        widgetActionContextManager_->DoAction(lastValue_ + value);
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
    for(auto actionContextManager : actionContextManagers_)
        if(actionContextManager->GetWidget() != nullptr)
        {
            actionContextManager->GetWidget()->SetWidgetActionContextManager(nullptr);
            actionContextManager->GetWidget()->Reset();
        }
    
    for(auto zone : includedZones_)
        zone->Deactivate();
}

void Zone::Activate()
{
    for(auto actionContextManager : actionContextManagers_)
        actionContextManager->Activate();
    
    for(auto zone : includedZones_)
        zone->Activate();
}

void Zone::Activate(int fxIndex)
{
    for(auto actionContextManager : actionContextManagers_)
        actionContextManager->Activate(fxIndex);
    
    for(auto zone : includedZones_)
        zone->Activate(fxIndex);
}

void Zone::Activate(MediaTrack* track, int sendsIndex)
{
    for(auto actionContextManager : actionContextManagers_)
        actionContextManager->Activate(track, sendsIndex);
    
    for(auto zone : includedZones_)
        zone->Activate(track, sendsIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectedTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
string SelectedTrackNavigator::GetTrackGUID()
{
    if(DAW::CountSelectedTracks(nullptr) != 1)
        return "";
    
    for(int i = 1; i <= page_->GetNumTracks(); i++)
    {
        MediaTrack* track = page_->GetTrackFromId(i);
        
        int flags = 0;
        
        DAW::GetTrackInfo(track, &flags);
        
        if(flags & 0x02) // Selected
            return page_->GetTrackGUID(track);
    }
    
    return "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FocusedFXTrackNavigator
////////////////////////////////////////////////////////////////////////////////////////////////////////
string FocusedFXTrackNavigator::GetTrackGUID()
{
    int trackNumber = 0;
    int itemNumber = 0;
    int fxIndex = 0;
    
    if(DAW::GetFocusedFX(&trackNumber, &itemNumber, &fxIndex) == 1) // Track FX
        return page_->GetTrackGUID(page_->GetTrackFromId(trackNumber));
    else
        return "";
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

void ControlSurface::GoZone(string zoneName)
{
    if(zones_.count(zoneName) > 0)
    {
        if(zoneName == "Home")
        {
            for(auto zone : activeZones_)
                zone->Deactivate();
            
            activeZones_.clear();
            activeSubZones_.clear();
            page_->ClearActiveFXZones(this);
        }
        else if(HasActiveZone(zoneName))
            DeactivateZone(zoneName);
        
        ActivateZone(zoneName);
    }
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
// ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Widget* ActionContext::GetWidget()
{
    return widgetActionContextManager_->GetWidget();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WidgetActionContextManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string WidgetActionContextManager::GetModifiers()
{
    if(widget_->GetIsModifier())
        return NoModifiers; // Modifier Widgets cannot have Modifiers
    else
        return widget_->GetSurface()->GetPage()->GetModifiers(widget_->GetTrack());
}

MediaTrack* WidgetActionContextManager::GetTrack()
{
    if(trackNavigator_ == nullptr)
        return nullptr;
    else
        return widget_->GetSurface()->GetPage()->GetTrackFromGUID(trackNavigator_->GetTrackGUID());
}

void WidgetActionContextManager::RequestUpdate()
{
    if(widgetActionContexts_.count(GetModifiers()) > 0)
        for(auto context : widgetActionContexts_[GetModifiers()])
            context->RequestUpdate();
}

void WidgetActionContextManager::DoAction(double value)
{
    if(widgetActionContexts_.count(GetModifiers()) > 0)
        for(auto context : widgetActionContexts_[GetModifiers()])
            context->DoAction(value);
}

void WidgetActionContextManager::Activate()
{
    if(widgetActionContexts_.count(GetModifiers()) > 0)
        for(auto context : widgetActionContexts_[GetModifiers()])
            context->Activate(this);
}

void WidgetActionContextManager::Activate(int contextIndex)
{
    if(widgetActionContexts_.count(GetModifiers()) > 0)
        for(auto context : widgetActionContexts_[GetModifiers()])
        {
            context->SetIndex(contextIndex);
            context->Activate(this);
        }
}

void WidgetActionContextManager::Activate(MediaTrack* track, int contextIndex)
{
    if(widgetActionContexts_.count(GetModifiers()) > 0)
        for(auto context : widgetActionContexts_[GetModifiers()])
        {
            context->SetTrack(track);
            context->SetIndex(contextIndex);
            context->Activate(this);
        }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void TrackNavigationManager::Init()
{
    for(int i = 1; i <= DAW::CSurf_NumTracks(followMCP_) && i < trackNavigators_.size(); i++)
        trackNavigators_[i]->SetTrackGUID(DAW::GetTrackGUIDAsString(i, followMCP_));
    
    SetPinnedTracks();
    
    char buffer[BUFSZ];
    if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "BankOffset").c_str(), buffer, sizeof(buffer)))
        trackOffset_ = atol(buffer);
}

void TrackNavigationManager::SetPinnedTracks()
{
    char buffer[BUFSZ];
    
    for(int i = 0; i < trackNavigators_.size(); i++)
    {
        if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
        {
            trackNavigators_[i]->SetTrackGUID(buffer);
            trackNavigators_[i]->SetIsPinned(true);
        }
    }
}

void TrackNavigationManager::OnTrackSelection(MediaTrack* track)
{
    if(scrollLink_)
    {
        // Make sure selected track is visble on the control surface
        int low = trackOffset_;
        int high = low + trackNavigators_.size() - 1 - GetNumPinnedTracks();
        
        int selectedTrackOffset = DAW::CSurf_TrackToID(track, followMCP_);
        
        if(selectedTrackOffset < low)
            TheManager->AdjustTrackBank(page_, selectedTrackOffset - low);
        if(selectedTrackOffset > high)
            TheManager->AdjustTrackBank(page_, selectedTrackOffset - high);
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

void TrackNavigationManager::PinSelectedTracks()
{
    TrackNavigator* navigator = nullptr;
    
    for(int i = 0; i < trackNavigators_.size(); i++)
    {
        navigator = trackNavigators_[i];
        
        MediaTrack* track = DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_);
        if(track == nullptr)
            continue;
        
        if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
        {
            navigator->SetIsPinned(true);
            DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "Channel" + to_string(i + 1)).c_str(), navigator->GetTrackGUID().c_str());
            DAW::MarkProjectDirty(nullptr);
        }
    }
}

void TrackNavigationManager::UnpinSelectedTracks()
{
    TrackNavigator* navigator = nullptr;
    char buffer[BUFSZ];
    
    for(int i = 0; i < trackNavigators_.size(); i++)
    {
        navigator = trackNavigators_[i];
        
        MediaTrack* track =  DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_);
        if(track == nullptr)
            continue;
        
        if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
        {
            navigator->SetIsPinned(false);
            if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
            {
                DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "Channel" + to_string(i + 1)).c_str(), "");
                DAW::MarkProjectDirty(nullptr);
            }
        }
    }
    
    RefreshLayout();
}

bool TrackNavigationManager::TrackListChanged()
{
    if(currentlyRefreshingLayout_)
        return false;
    
    int currentNumVisibleTracks = 0;
    
    for(int i = 1; i <= DAW::CSurf_NumTracks(followMCP_); i++)
        if(IsTrackVisible(DAW::CSurf_TrackFromID(i, followMCP_)))
            currentNumVisibleTracks++;
    
    if(currentNumVisibleTracks != previousNumVisibleTracks_)
    {
        if(previousTrackList_ != nullptr)
            delete[] previousTrackList_;
        
        previousNumVisibleTracks_ = currentNumVisibleTracks;
        previousTrackList_ = new MediaTrack* [currentNumVisibleTracks];
        
        for(int i = 0; i < currentNumVisibleTracks; i++)
            previousTrackList_[i] = DAW::CSurf_TrackFromID(i, followMCP_);
        
        TrackNavigator* navigator = nullptr;
        char buffer[BUFSZ];
        for(int i = 0; i < trackNavigators_.size(); i++)
        {
            navigator = trackNavigators_[i];
            
            if(DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_) == nullptr) // track has been removed
            {
                page_->TrackHasBeenRemoved(navigator->GetTrackGUID());
                
                if(navigator->GetIsPinned())
                {
                    navigator->SetIsPinned(false);
                    navigator->SetTrackGUID("");
                    
                    // GAW remove this from pinned tracks list in project
                    if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "Channel" + to_string(i + 1)).c_str(), buffer, sizeof(buffer)))
                    {
                        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "Channel" + to_string(i + 1)).c_str(), "");
                        DAW::MarkProjectDirty(nullptr);
                    }
                }
            }
        }
        
        return true;
    }
    else if(currentNumVisibleTracks == previousNumVisibleTracks_)
    {
        MediaTrack **currentTrackList = new MediaTrack* [currentNumVisibleTracks];
        for(int i = 0; i < currentNumVisibleTracks; i++)
            currentTrackList[i] = DAW::CSurf_TrackFromID(i, followMCP_);
        
        if(memcmp(previousTrackList_, currentTrackList, currentNumVisibleTracks * sizeof(MediaTrack*)))
        {
            if(previousTrackList_ != nullptr)
                delete[] previousTrackList_;
            previousTrackList_ = currentTrackList;
            return true;
        }
        else
        {
            delete[]currentTrackList;
            return false;
        }
    }
    
    return false;
}

void TrackNavigationManager::AdjustTrackBank(int stride)
{
    int numTracks = DAW::CSurf_NumTracks(followMCP_);
    
    if(numTracks <= trackNavigators_.size())
        return;
    
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ <  1)
        trackOffset_ =  1;
    
    int top = numTracks - trackNavigators_.size() + 1;
    
    if(trackOffset_ >  top)
        trackOffset_ = top;
    
    vector<string> pinnedChannels;
    
    GetPinnedChannelGUIDs(pinnedChannels);
    
    while(trackOffset_ <= numTracks)
    {
        string trackGUID = DAW::GetTrackGUIDAsString(trackOffset_, followMCP_);
        
        if(find(pinnedChannels.begin(), pinnedChannels.end(), trackGUID) != pinnedChannels.end())
            previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
        else if(! IsTrackVisible(DAW::CSurf_TrackFromID(trackOffset_, followMCP_)))
            previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
        else
            break;
    }
    
    if(previousTrackOffset != trackOffset_)
    {
        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (page_->GetName() + "BankOffset").c_str(), to_string(trackOffset_).c_str());
        DAW::MarkProjectDirty(nullptr);
        
        RefreshLayout();
    }
}

void TrackNavigationManager::RefreshLayout()
{
    currentlyRefreshingLayout_ = true;
    
    vector<string> pinnedChannels;
    
    GetPinnedChannelGUIDs(pinnedChannels);
    
    vector<string> layoutChannels(trackNavigators_.size() + pinnedChannels.size());
    
    int layoutChannelIndex = 0;
    
    for(int i = trackOffset_; i <= DAW::CSurf_NumTracks(followMCP_) && layoutChannelIndex < layoutChannels.size(); i++)
    {
        if(! IsTrackVisible(DAW::CSurf_TrackFromID(i, followMCP_)))
            pinnedChannels.push_back(DAW::GetTrackGUIDAsString(i, followMCP_));
        else
            layoutChannels[layoutChannelIndex++] = DAW::GetTrackGUIDAsString(i, followMCP_);
    }
    
    subtract_vector(layoutChannels, pinnedChannels);
    
    if(colourTracks_ && TheManager->GetCurrentPage() == page_)
    {
        DAW::PreventUIRefresh(1);
        
        // reset track colors
        for(auto* navigator : trackNavigators_)
            if(MediaTrack* track = DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_))
                if(trackColours_.count(navigator->GetTrackGUID()) > 0)
                    DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", &trackColours_[navigator->GetTrackGUID()]);
    }
    
    
    // Apply new layout
    layoutChannelIndex = 0;
    
    for(auto* navigator : trackNavigators_)
        if(! navigator->GetIsPinned())
            navigator->SetTrackGUID(layoutChannels[layoutChannelIndex++]);
    
    
    if(colourTracks_ && TheManager->GetCurrentPage() == page_)
    {
        // color tracks
        int color = DAW::ColorToNative(trackColourRedValue_, trackColourGreenValue_, trackColourBlueValue_) | 0x1000000;
        for(auto* navigator : trackNavigators_)
            if(MediaTrack* track = DAW::GetTrackFromGUID(navigator->GetTrackGUID(), followMCP_))
            {
                trackColours_[navigator->GetTrackGUID()] = DAW::GetTrackColor(track);
                DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", &color);
            }
        
        DAW::PreventUIRefresh(-1);
    }
    
    currentlyRefreshingLayout_ = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SendsNavigationManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
int SendsNavigationManager::GetMaxSends()
{
    int maxSends = 0;
    
    for(int i = 1; i <= page_->GetNumTracks(); i++)
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
void SendsActivationManager::MapSelectedTrackSendsToWidgets(ControlSurface* surface, MediaTrack* selectedTrack)
{
    for(auto surface : page_->GetSurfaces())
    {
        if(activeSendZoneNames_.count(surface) > 0)
        {
            for(auto zoneName :activeSendZoneNames_[surface])
                surface->DeactivateZone(zoneName);
            
            activeSendZoneNames_[surface].clear();
        }
    }
    
    int flags;
    
    DAW::GetTrackInfo(selectedTrack, &flags);
    
    if((flags & 0x02) && shouldMapSends_) // track is selected -- not deselected and shouldMapSends_ == true
    {
        for(int i = 0; i < DAW::DAW::GetTrackNumSends(selectedTrack, 0); i++)
        {
            string sendName = "Send" + to_string(i + 1);
            
            if( ! surface->GetUseZoneLink())
            {
                if(surface->ActivateSendsZone(sendName, selectedTrack, i))
                    activeSendZoneNames_[surface].push_back(sendName);
            }
            else
            {
                for(auto surface : page_->GetSurfaces())
                    if(surface->GetUseZoneLink())
                        if(surface->ActivateSendsZone(sendName, selectedTrack, i))
                            activeSendZoneNames_[surface].push_back(sendName);
            }
        }
    }
}

void SendsActivationManager::ToggleShouldMapSends()
{
    shouldMapSends_ = ! shouldMapSends_;
    
    if(DAW::CountSelectedTracks(NULL) == 1)
    {
        for(auto surface : page_->GetSurfaces())
        {
            if(activeSendZoneNames_.count(surface) > 0)
            {
                for(auto zoneName :activeSendZoneNames_[surface])
                    surface->DeactivateZone(zoneName);
                
                activeSendZoneNames_[surface].clear();
            }
        }
        
        int flags;
        
        MediaTrack* selectedTrack = nullptr;
        
        for(int i = 1; i <= page_->GetNumTracks(); i++)
        {
            MediaTrack* currentTrack = page_->GetTrackFromId(i);
            
            if(DAW::GetMediaTrackInfo_Value(currentTrack, "I_SELECTED"))
            {
                selectedTrack = currentTrack;
                break;
            }
        }
        
        DAW::GetTrackInfo(selectedTrack, &flags);
        
        if((flags & 0x02) && shouldMapSends_) // track is selected -- not deselected and shouldMapSends_ == true
        {
            for(int i = 0; i < DAW::DAW::GetTrackNumSends(selectedTrack, 0); i++)
            {
                string sendName = "Send" + to_string(i + 1);
                
                    for(auto surface : page_->GetSurfaces())
                        if(surface->ActivateSendsZone(sendName, selectedTrack, i))
                            activeSendZoneNames_[surface].push_back(sendName);
            }
        }
    }
}
