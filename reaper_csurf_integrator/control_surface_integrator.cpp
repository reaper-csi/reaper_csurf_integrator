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

static map<string, vector<Zone*>> includedZoneMembers;

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

static vector<string> GetTokens(string line)
{
    vector<string> tokens;

    istringstream iss(line);
    string token;
    while (iss >> quoted(token))
        tokens.push_back(token);
    
    return tokens;
}

static void GetWidgetNameAndModifiers(string line, string &widgetName, bool &isInverted, bool &shouldToggle, bool &isDelayed,  double &delayAmount)
{
    istringstream modified_role(line);
    vector<string> modifier_tokens;
    string modifier_token;
    
    while (getline(modified_role, modifier_token, '+'))
        modifier_tokens.push_back(modifier_token);
    
    if(modifier_tokens.size() > 1)
    {
        for(int i = 0; i < modifier_tokens.size() - 1; i++)
        {
            if(modifier_tokens[i] == "Invert")
                isInverted = true;
            else if(modifier_tokens[i] == "Toggle")
                shouldToggle = true;
            else if(modifier_tokens[i] == "Hold")
            {
                isDelayed = true;
                delayAmount = 1.0;
            }
        }
    }
    
    widgetName = modifier_tokens[modifier_tokens.size() - 1];
}

static int strToHex(string valueStr)
{
    return strtol(valueStr.c_str(), nullptr, 16);
}

static double strToDouble(string valueStr)
{
    return strtod(valueStr.c_str(), nullptr);
}

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
    if(navigator_ == nullptr)
        return nullptr;
    else
        return DAW::GetTrackFromGUID(navigator_->GetTrackGUID(), surface_->GetPage()->GetFollowMCP());
}

void Widget::RequestUpdate()
{
    if(actionContext_ != nullptr)
        actionContext_->RequestUpdate();
}

void  Widget::SetValue(double value)
{
    for(auto feebackProcessor : feedbackProcessors_)
        feebackProcessor->SetValue(value);
}

void  Widget::SetValue(int mode, double value)
{
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
    if(actionContext_ != nullptr)
        actionContext_->DoAction(value);
}

void Widget::DoRelativeAction(double value)
{
    if(actionContext_ != nullptr)
        actionContext_->DoRelativeAction(value);
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
void Zone::Activate()
{
    for(auto actionContext : actionContexts_)
        actionContext->GetWidget()->SetActionContext(actionContext);
    
    for(auto zone : includedZones_)
        zone->Activate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_ControlSurface::InitWidgets(string templateFilename)
{
    ProcessFile(string(DAW::GetResourcePath()) + "/CSI/Surfaces/Midi/" + templateFilename);
    
    // Add the "hardcoded" widgets
    widgets_.push_back(new Widget(this, "TrackOnSelection"));
    widgets_.push_back(new Widget(this, "TrackOnFocusedFX"));

}

void Midi_ControlSurface::ProcessWidget(int &lineNumber, ifstream &surfaceTemplateFile, vector<string> tokens)
{
    if(tokens.size() < 2)
        return;
    
    Widget* widget = new Widget(this, tokens[1]);
    widgets_.push_back(widget);
    
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
            Midi_FeedbackProcessor* feedbackProcessor = nullptr;
            
            string widgetClass = tokens[0];
            
            // Control Signal Generators
            if(widgetClass == "Press" && tokens.size() == 4)
                new Press_Midi_ControlSignalGenerator(this, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "PressRelease" && tokens.size() == 7)
                new PressRelease_Midi_ControlSignalGenerator(this, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])), new MIDI_event_ex_t(strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6])));
            else if(widgetClass == "Fader14Bit" && tokens.size() == 4)
                new Fader14Bit_Midi_ControlSignalGenerator(this, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Fader7Bit" && tokens.size() == 4)
                new Fader7Bit_Midi_ControlSignalGenerator(this, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            else if(widgetClass == "Encoder" && tokens.size() == 4)
                new Encoder_Midi_ControlSignalGenerator(this, widget, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
            
            // Feedback Processors
            else if(widgetClass == "FB_TwoState" && (tokens.size() == 7 || tokens.size() == 8))
            {
                feedbackProcessor = new TwoState_Midi_FeedbackProcessor(this, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])), new MIDI_event_ex_t(strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6])));
                
                if(tokens.size() == 8)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[7]));

                widget->AddFeedbackProcessor(feedbackProcessor);
            }
            else if(tokens.size() == 4 || tokens.size() == 5)
            {
                if(widgetClass == "FB_Fader14Bit")
                    feedbackProcessor = new Fader14Bit_Midi_FeedbackProcessor(this, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_Fader7Bit")
                    feedbackProcessor = new Fader7Bit_Midi_FeedbackProcessor(this, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_Encoder")
                    feedbackProcessor = new Encoder_Midi_FeedbackProcessor(this, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_VUMeter")
                    feedbackProcessor = new VUMeter_Midi_FeedbackProcessor(this, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_GainReductionMeter")
                    feedbackProcessor = new GainReductionMeter_Midi_FeedbackProcessor(this, new MIDI_event_ex_t(strToHex(tokens[1]), strToHex(tokens[2]), strToHex(tokens[3])));
                else if(widgetClass == "FB_QConProXMasterVUMeter")
                    feedbackProcessor = new QConProXMasterVUMeter_Midi_FeedbackProcessor(this);
                
                if(tokens.size() == 5)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[4]));
                
                widget->AddFeedbackProcessor(feedbackProcessor);
            }
            else if(widgetClass == "FB_MCU_TimeDisplay_Midi" && tokens.size() == 1)
            {
                feedbackProcessor = new MCU_TimeDisplay_Midi_FeedbackProcessor(this);
            }
            else if(widgetClass == "FB_MCUVUMeter" && (tokens.size() == 2 || tokens.size() == 3))
            {
                feedbackProcessor = new MCUVUMeter_Midi_FeedbackProcessor(this, stoi(tokens[1]));
               
                if(tokens.size() == 3)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[2]));
                
                widget->AddFeedbackProcessor(feedbackProcessor);
            }
            else if((widgetClass == "FB_MCUDisplayUpper" || widgetClass == "FB_MCUDisplayLower" || widgetClass == "FB_MCUXTDisplayUpper" || widgetClass == "FB_MCUXTDisplayLower") && (tokens.size() == 2 || tokens.size() == 3))
            {
                if(widgetClass == "FB_MCUDisplayUpper")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(this, 0, 0x14, 0x12, stoi(tokens[1]));
                else if(widgetClass == "FB_MCUDisplayLower")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(this, 1, 0x14, 0x12, stoi(tokens[1]));
                else if(widgetClass == "FB_MCUXTDisplayUpper")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(this, 0, 0x15, 0x12, stoi(tokens[1]));
                else if(widgetClass == "FB_MCUXTDisplayLower")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(this, 1, 0x15, 0x12, stoi(tokens[1]));
                
                if(tokens.size() == 3)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[2]));
                
                widget->AddFeedbackProcessor(feedbackProcessor);
            }
            
            else if((widgetClass == "FB_C4DisplayUpper" || widgetClass == "FB_C4DisplayLower") && (tokens.size() == 3 || tokens.size() == 4))
            {
                if(widgetClass == "FB_MCUDisplayUpper")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(this, 0, 0x17, stoi(tokens[1]) + 0x30, stoi(tokens[2]));
                else if(widgetClass == "FB_MCUDisplayLower")
                    feedbackProcessor = new MCUDisplay_Midi_FeedbackProcessor(this, 1, 0x17, stoi(tokens[1]) + 0x30, stoi(tokens[2]));
                
                if(tokens.size() == 4)
                    feedbackProcessor->SetRefreshInterval(strToDouble(tokens[3]));
                
                widget->AddFeedbackProcessor(feedbackProcessor);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void ControlSurface::ProcessFile(string filePath)
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
                    ProcessZone(lineNumber, file, tokens, filePath);
                else if(tokens[0] == "Widget")
                    ProcessWidget(lineNumber, file, tokens);
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

void ControlSurface::InitZones(string zoneFolder)
{
    try
    {
        vector<string> zoneFilesToProcess;
        listZoneFiles(DAW::GetResourcePath() + string("/CSI/Zones/") + zoneFolder + "/", zoneFilesToProcess); // recursively find all the .zon files, starting at zoneFolder
        
        for(auto zoneFilename : zoneFilesToProcess)
            ProcessFile(zoneFilename);
        
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

void ControlSurface::ExpandIncludedZone(vector<string> tokens, vector<string> &localZones)
{
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// Expand syntax of type "Channel|1-8" into 8 Zones named Channel1, Channel2, ... Channel8
    //////////////////////////////////////////////////////////////////////////////////////////////
    
    vector<string> localZoneIds;

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
                localZoneIds.push_back(to_string(i));
            
            for(int i = 0; i <= rangeEnd - rangeBegin; i++)
            {
                localZones.push_back(zoneBaseName + localZoneIds[i]);
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// Just regular syntax of type "Channel1"
    //////////////////////////////////////////////////////////////////////////////////////////////
    else
    {
        localZones.push_back(tokens[0]);
    }
}

void ControlSurface::ProcessIncludedZones(int &lineNumber, ifstream &zoneFile, string filePath, Zone* zone)
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
            
            vector<string> localZones;
            
            ExpandIncludedZone(tokens, localZones);
            
            for(int i = 0; i < localZones.size(); i++)
            {
                if(zone->GetName() != localZones[i]) // prevent recursive defintion
                    includedZoneMembers[localZones[i]].push_back(zone);
            }
        }
    }
}

void ControlSurface::ExpandZone(vector<string> tokens, string filePath, vector<Zone*> &localZones, vector<string> &localZoneIds)
{
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// Expand syntax of type "Channel|1-8" into 8 Zones named Channel1, Channel2, ... Channel8
    //////////////////////////////////////////////////////////////////////////////////////////////
    

    istringstream expandedZone(tokens[1]);
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
                localZoneIds.push_back(to_string(i));
            
            for(int i = 0; i <= rangeEnd - rangeBegin; i++)
            {
                Zone* zone = new Zone(this, zoneBaseName + localZoneIds[i], filePath);
                if(AddZone(zone))
                    localZones.push_back(zone);
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// Just regular syntax of type "Channel1"
    //////////////////////////////////////////////////////////////////////////////////////////////
    else
    {
        Zone* zone = new Zone(this, tokens[1], filePath);
        if(AddZone(zone))
        {
            localZones.push_back(zone);
            localZoneIds.push_back("");
        }
    }
}

TrackNavigator* ControlSurface::TrackNavigatorForChannel(int channel)
{
    if(trackNavigators_.count(channel) < 1)
    {
        trackNavigators_[channel] = new TrackNavigator();
        page_->AddTrackNavigator(trackNavigators_[channel]);
    }
    
    return trackNavigators_[channel];
}


void ControlSurface::ProcessZone(int &lineNumber, ifstream &zoneFile, vector<string> passedTokens, string filePath)
{
    const string GainReductionDB = "GainReductionDB"; // GAW TBD don't forget this logic
    
    if(passedTokens.size() < 2)
        return;

    vector<Zone*> localZones;
    vector<string> localZoneIds;
    
    ExpandZone(passedTokens, filePath, localZones, localZoneIds);

    bool hasNavigator = false;
    string navigatorType = "";
    vector<Navigator*> localNavigators;
    
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
        
        if(tokens.size() == 2 && tokens[0] == "Navigator")
        {
            hasNavigator = true;
            navigatorType = tokens[1];
            
            for(int i = 0; i < localZones.size(); i++)
            {
                if(navigatorType == "TrackNavigator")
                    localNavigators.push_back(TrackNavigatorForChannel(i));
            }
            
            continue;
        }
        
        for(int i = 0; i < localZones.size(); i++)
        {
            // Pre-process for "Channel|1-8" syntax
            string localZoneLine(line);
            localZoneLine = regex_replace(localZoneLine, regex("\\|"), localZoneIds[i]);

            vector<string> tokens(GetTokens(localZoneLine));
        
            if(tokens.size() > 0)
            {
                if(tokens[0] == "IncludedZones")
                {
                    ProcessIncludedZones(lineNumber, zoneFile, filePath, localZones[i]);
                    continue;
                }
                
                // GAW -- the first token is the Widget name, possibly decorated with modifiers
                string widgetName = "";
                bool isInverted = false;
                bool shouldToggle = false;
                bool isDelayed = false;
                double delayAmount = 0.0;

                GetWidgetNameAndModifiers(tokens[0], widgetName, isInverted, shouldToggle, isDelayed, delayAmount);
                
                Widget* widget = nullptr;
                
                for(auto * aWidget : widgets_)
                    if(aWidget->GetName() == widgetName)
                        widget = aWidget;
                
                vector<string> params;
                for(int i = 1; i < tokens.size(); i++)
                    params.push_back(tokens[i]);
                
                if(params.size() > 0 && widget != nullptr)
                {
                    if(hasNavigator)
                        widget->AddNavigator(localNavigators[i]);
                    
                    if(ActionContext* context = TheManager->GetActionContext(widget, params))
                    {
                        if(isInverted)
                            context->SetIsInverted();
                        
                        if(shouldToggle)
                            context->SetShouldToggle();
                        
                        if(isDelayed)
                            context->SetDelayAmount(delayAmount * 1000.0);
                        
                        localZones[i]->AddActionContext(context);
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Page
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Page::TrackFXListChanged(MediaTrack* track)
{
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(TheManager->GetVSTMonitor())
        {
            DAW::ShowConsoleMsg(("\n\n" + string(fxName) + "\n").c_str());
            
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                DAW::ShowConsoleMsg((string(fxParamName) + "\n").c_str());
            }
        }
    }
    
    // GAW TBD -- clear all fx items and rebuild
}

int Page::GetFXParamIndex(MediaTrack* track, Widget* widget, int fxIndex, string fxParamName)
{
    char fxName[BUFSZ];
    
    DAW::TrackFX_GetFXName(track, fxIndex, fxName, sizeof(fxName));
    
    if(TheManager->GetFXParamIndices().count(fxName) > 0 && TheManager->GetFXParamIndices()[fxName].count(fxParamName) > 0)
        return TheManager->GetFXParamIndices()[fxName][fxParamName];
    
    char paramName[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetNumParams(track, fxIndex); i++)
    {
        DAW::TrackFX_GetParamName(track, fxIndex, i, paramName, sizeof(paramName));
        
        if(paramName == fxParamName)
        {
            TheManager->GetFXParamIndices()[fxName][fxParamName] = i;
            return i;
        }
    }
    
    return 0;
}

void Page::OnTrackSelectionBySurface(MediaTrack* track)
{
    if(scrollLink_)
    {
        if(DAW::IsTrackVisible(track, true))
            DAW::SetMixerScroll(track); // scroll selected MCP tracks into view

        if(DAW::IsTrackVisible(track, false))
            DAW::SendCommandMessage(40913); // scroll selected TCP tracks into view
    }

    OnTrackSelection(track);
}

void Page::OnTrackSelection(MediaTrack* track)
{
    if(scrollLink_)
    {
        // Make sure selected track is visble on the control surface
        int low = trackOffset_;
        int high = low + trackNavigators_.size() - 1 - GetNumPinnedTracks();
        
        int selectedTrackOffset = DAW::CSurf_TrackToID(track, followMCP_);
        
        if(selectedTrackOffset < low)
            TheManager->AdjustTrackBank(this, selectedTrackOffset - low);
        if(selectedTrackOffset > high)
            TheManager->AdjustTrackBank(this, selectedTrackOffset - high);
    }
    /*
    for(auto surface : realSurfaces_)
        for(auto widget : surface->GetAllWidgets())
            if(widget->GetName() == TrackOnSelection)
                if(widgetContexts_.count(widget) > 0)
                    widgetContexts_[widget]->DoAction(this, GetCurrentModifiers(), surface);*/
}

void Page::OnGlobalMapTrackAndFxToWidgetsForTrack(MediaTrack* track)
{/*
    for(auto surface : realSurfaces_)
        for(auto widget : surface->GetAllWidgets())
            if(widget->GetName() == TrackOnMapTrackAndFXToWidgets)
                if(widgetContexts_.count(widget) > 0)
                    widgetContexts_[widget]->DoAction(this, GetCurrentModifiers(), surface, track);*/
}

void Page::OnFXFocus(MediaTrack* track, int fxIndex)
{/*
    // GAW WIP  -- currently doesn't take FX index into account
    for(auto surface : realSurfaces_)
        for(auto widget : surface->GetAllWidgets())
            if(widget->GetName() == TrackOnFocusedFX)
                if(widgetContexts_.count(widget) > 0)
                    widgetContexts_[widget]->DoAction(this, GetCurrentModifiers(), surface, track, fxIndex);*/
}

void Page::AdjustTrackBank(int stride)
{
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ <  1)
        trackOffset_ =  1;
    
    int top = DAW::CSurf_NumTracks(followMCP_) - trackNavigators_.size();
    
    if(trackOffset_ >  top)
        trackOffset_ = top;
    
    vector<string> pinnedChannels;
    
    GetPinnedChannelGUIDs(pinnedChannels);

    while(trackOffset_ <= DAW::CSurf_NumTracks(followMCP_))
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
        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "BankOffset").c_str(), to_string(trackOffset_).c_str());
        DAW::MarkProjectDirty(nullptr);
        
            RefreshLayout();
    }
}

void Page::RefreshLayout()
{
    currentlyRefreshingLayout_ = true;
    
    vector<string> pinnedChannels;

    GetPinnedChannelGUIDs(pinnedChannels);
    
    vector<string> layoutChannels(trackNavigators_.size() + pinnedChannels.size());

    int layoutChannelIndex = 0;
    
    for(int i = trackOffset_; i < DAW::CSurf_NumTracks(followMCP_) && layoutChannelIndex < layoutChannels.size(); i++)
    {
        if(! IsTrackVisible(DAW::CSurf_TrackFromID(i, followMCP_)))
            pinnedChannels.push_back(DAW::GetTrackGUIDAsString(i, followMCP_));
        else
            layoutChannels[layoutChannelIndex++] = DAW::GetTrackGUIDAsString(i, followMCP_);
    }
    
    subtract_vector(layoutChannels, pinnedChannels);
    
    if(colourTracks_ && TheManager->GetCurrentPage() == this)
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

    
    if(colourTracks_ && TheManager->GetCurrentPage() == this)
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
// Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Manager::InitActionDictionary()
{
    actions_["Reaper"] = new ReaperAction();
    actions_["TrackFX"] = new TrackFX();
    actions_["TrackFXParamNameDisplay"] = new TrackFXParamNameDisplay();
    actions_["TrackFXParamValueDisplay"] = new TrackFXParamValueDisplay();
    actions_["GainReductionDB"] = new TrackGainReductionMeter();
    actions_["TrackVolume"] = new TrackVolume();
    actions_["MasterTrackVolume"] = new MasterTrackVolume();
    actions_["TrackSendVolume"] = new TrackSendVolume();
    actions_["TrackSendPan"] = new TrackSendPan();
    actions_["TrackSendMute"] = new TrackSendMute();
    actions_["TrackVolumeDB"] = new TrackVolumeDB();
    actions_["TrackPan"] = new TrackPan();
    actions_["TrackPanWidth"] = new TrackPanWidth();
    actions_["TrackNameDisplay"] = new TrackNameDisplay();
    actions_["TrackVolumeDisplay"] = new TrackVolumeDisplay();
    actions_["TrackSendNameDisplay"] = new TrackSendNameDisplay();
    actions_["TrackSendVolumeDisplay"] = new TrackSendVolumeDisplay();
    actions_["TrackPanDisplay"] = new TrackPanDisplay();
    actions_["TrackPanWidthDisplay"] = new TrackPanWidthDisplay();
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
    actions_["TrackTouch"] = new TrackTouch();
    actions_["MasterTrackTouch"] = new MasterTrackTouch();
    actions_["TrackTouchControlled"] = new TrackTouchControlled();
    actions_["TrackSendTouchControlled"] = new TrackTouchControlled();
    actions_["CycleTimeline"] = new CycleTimeline();
    actions_["TrackOutputMeter"] = new TrackOutputMeter();
    actions_["MasterTrackOutputMeter"] = new MasterTrackOutputMeter();
    actions_["SetShowFXWindows"] = new SetShowFXWindows();
    actions_["SetScrollLink"] = new SetScrollLink();
    actions_["CycleTimeDisplayModes"] = new CycleTimeDisplayModes();
    actions_["NextPage"] = new class NextPage();
    actions_["GoPage"] = new class GoPage();
    actions_["SelectTrackRelative"] = new SelectTrackRelative();
    actions_["TrackBank"] = new TrackBank();
    actions_["TrackSendBank"] = new TrackSendBank();
    actions_["PinSelectedTracks"] = new PinSelectedTracks();
    actions_["UnpinSelectedTracks"] = new UnpinSelectedTracks();
    
     }

void Manager::InitActionContextDictionary()
{
    InitActionDictionary();
    
    actionContexts_["Reaper"] = [this](Widget* widget, vector<string> params) { return new ReaperActionContext(widget, actions_[params[0]], params[1]); };
    actionContexts_["TrackFX"] = [this](Widget* widget, vector<string> params) { return new FXContext(widget, actions_[params[0]], params[1]); };
    actionContexts_["TrackFXParamNameDisplay"] = [this](Widget* widget, vector<string> params) { return new FXContext(widget, actions_[params[0]], params[1]); };
    actionContexts_["TrackFXParamValueDisplay"] = [this](Widget* widget, vector<string> params) { return new FXContext(widget, actions_[params[0]], params[1]); };
    actionContexts_["GainReductionDB"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackVolume"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["MasterTrackVolume"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSendVolume"] = [this](Widget* widget, vector<string> params) { return new TrackSendContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSendPan"] = [this](Widget* widget, vector<string> params) { return new TrackSendContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSendMute"] = [this](Widget* widget, vector<string> params) { return new TrackSendContext(widget, actions_[params[0]]); };
    actionContexts_["TrackVolumeDB"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackPan"] = [this](Widget* widget, vector<string> params) { return new TrackContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackPanWidth"] = [this](Widget* widget, vector<string> params) { return new TrackContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackNameDisplay"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackVolumeDisplay"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSendNameDisplay"] = [this](Widget* widget, vector<string> params) { return new TrackSendContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSendVolumeDisplay"] = [this](Widget* widget, vector<string> params) { return new TrackSendContext(widget, actions_[params[0]]); };
    actionContexts_["TrackPanDisplay"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackPanWidthDisplay"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TimeDisplay"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["Rewind"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["FastForward"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["Play"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["Stop"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["Record"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSelect"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackUniqueSelect"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["MasterTrackUniqueSelect"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["TrackRangeSelect"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackRecordArm"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackMute"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackSolo"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["TrackTouch"] = [this](Widget* widget, vector<string> params) { return new TrackContext(widget, actions_[params[0]]); };
    actionContexts_["MasterTrackTouch"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["TrackTouchControlled"] = [this](Widget* widget, vector<string> params) { return new TrackTouchControlledContext(widget, actions_[params[1]], actions_[params[2]]); };
    actionContexts_["TrackSendTouchControlled"] = [this](Widget* widget, vector<string> params) { return new TrackSendTouchControlledContext(widget, actions_[params[1]], actions_[params[2]]); };
    actionContexts_["CycleTimeline"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["TrackOutputMeter"] = [this](Widget* widget, vector<string> params) { return new TrackContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["MasterTrackOutputMeter"] = [this](Widget* widget, vector<string> params) { return new GlobalContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["SetShowFXWindows"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["SetScrollLink"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["CycleTimeDisplayModes"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["NextPage"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["GoPage"] = [this](Widget* widget, vector<string> params) { return new GlobalContextWithStringParam(widget, actions_[params[0]], params[1]); };
    actionContexts_["SelectTrackRelative"] = [this](Widget* widget, vector<string> params) { return new GlobalContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackBank"] = [this](Widget* widget, vector<string> params) { return new GlobalContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackSendBank"] = [this](Widget* widget, vector<string> params) { return new GlobalContextWithIntParam(widget, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["PinSelectedTracks"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
    actionContexts_["UnpinSelectedTracks"] = [this](Widget* widget, vector<string> params) { return new GlobalContext(widget, actions_[params[0]]); };
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////
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
                    
                    currentPage = new Page(tokens[1], pages_.size(), tokens[2] == "FollowMCP" ? true : false, tokens[3] == "SynchPages" ? true : false, tokens[4] == "UseTrackColoring" ? true : false, atoi(tokens[5].c_str()), atoi(tokens[6].c_str()), atoi(tokens[7].c_str()));
                    pages_.push_back(currentPage);
                    
                }
                else if(tokens[0] == MidiSurfaceToken)
                {
                    if(tokens.size() != 6)
                        continue;
                    
                    int channelIn = atoi(tokens[2].c_str());
                    int channelOut = atoi(tokens[3].c_str());
                    
                     if(currentPage)
                         currentPage->AddSurface(new Midi_ControlSurface(currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForChannel(channelIn), GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
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
