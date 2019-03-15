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
    
    if(line[0] != '\r' && line[0] != '/' && line != "") // ignore comment lines and blank lines
    {
        istringstream iss(line);
        string token;
        while (iss >> quoted(token))
            tokens.push_back(token);
    }
    
    return tokens;
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
void Widget::RequestUpdate()
{
    if(actionContext_ != nullptr)
        actionContext_->RequestUpdate(this);
}

void  Widget::SetValue(double value)
{
    
}

void  Widget::SetValue(int mode, double value)
{
    
}

void  Widget::SetValue(string value)
{
    
}

void Widget::DoAction(double value)
{
    if(actionContext_ != nullptr)
        actionContext_->DoAction(this, value);
}

void Widget::DoRelativeAction(double value)
{
    if(actionContext_ != nullptr)
        actionContext_->DoRelativeAction(this, value);
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
// Midi_Widget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Midi_Widget::SendMidiMessage(MIDI_event_ex_t* midiMessage)
{
    surface_->SendMidiMessage(midiMessage);
}

void Midi_Widget::SendMidiMessage(int first, int second, int third)
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
void TrackNavigator::SetTrackGUID(Page* page, string trackGUID)
{
    trackGUID_ = trackGUID;
    /*
    for(auto widget : widgets_)
        if(WidgetContext* widgetContext = page->GetWidgetContext(widget))
            widgetContext->SetComponentTrackContext(Track, trackGUID);
     */
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
Midi_ControlSurface::Midi_ControlSurface(Page* page, const string name, string templateFilename, string zoneFolder, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor)
: ControlSurface(page, name), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor)
{
    ifstream surfaceTemplateFile(string(DAW::GetResourcePath()) + "/CSI/mst/" + templateFilename);

    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        vector<string> tokens(GetTokens(line));
        
        if(tokens.size() > 0)
        {
            string name = tokens[0];

            if(tokens.size() > 1)
            {
                string widgetClass = tokens[1];

                /*
                if(widgetClass == "MCUTimeDisplay")
                {
                    if(tokens.size() == 2 || tokens.size() == 3)
                        widgets_.push_back(new MCU_TimeDisplay_Midi_Widget(this, name, true));
                    
                    if(tokens.size() == 3)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[2]));
                }
                else if(widgetClass == "QConProXMasterVUMeter")
                {
                    if(tokens.size() == 2 || tokens.size() == 3)
                        widgets_.push_back(new QConProXMasterVUMeter_Midi_Widget(this, name, true));
                    
                    if(tokens.size() == 3)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[2]));
                }
                else if(widgetClass == "MCUDisplayUpper")
                {
                    if(tokens.size() == 3 || tokens.size() == 4)
                        widgets_.push_back(new MCUDisplay_Midi_Widget(this, name, true, 0, 0x14, 0x12, stoi(tokens[2])));
                    
                    if(tokens.size() == 4)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[3]));
                }
                else if(widgetClass == "MCUDisplayLower")
                {
                    if(tokens.size() == 3 || tokens.size() == 4)
                        widgets_.push_back(new MCUDisplay_Midi_Widget(this, name, true, 1, 0x14, 0x12, stoi(tokens[2])));
                    
                    if(tokens.size() == 4)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[3]));
                }
                else if(widgetClass == "MCUXTDisplayUpper")
                {
                    if(tokens.size() == 3 || tokens.size() == 4)
                        widgets_.push_back(new MCUDisplay_Midi_Widget(this, name, true, 0, 0x15, 0x12, stoi(tokens[2])));
                    
                    if(tokens.size() == 4)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[3]));
                }
                else if(widgetClass == "MCUXTDisplayLower")
                {
                    if(tokens.size() == 3 || tokens.size() == 4)
                        widgets_.push_back(new MCUDisplay_Midi_Widget(this, name, true, 1, 0x15, 0x12, stoi(tokens[2])));
                    
                    if(tokens.size() == 4)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[3]));
                }
                else if(widgetClass == "C4DisplayUpper")
                {
                    if(tokens.size() == 4 || tokens.size() == 5)
                        widgets_.push_back(new MCUDisplay_Midi_Widget(this, name, true, 0, 0x17, stoi(tokens[2]) + 0x30, stoi(tokens[3])));
                    
                    if(tokens.size() == 5)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[4]));
                }
                else if(widgetClass == "C4DisplayLower")
                {
                    if(tokens.size() == 4 || tokens.size() == 5)
                        widgets_.push_back(new MCUDisplay_Midi_Widget(this, name, true, 1, 0x17, stoi(tokens[2]) + 0x30, stoi(tokens[3])));
                    
                    if(tokens.size() == 5)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[4]));
                }
                else if(widgetClass == "MCUVUMeter")
                {
                    if(tokens.size() == 3 || tokens.size() == 4)
                        widgets_.push_back(new MCUVUMeter_Midi_Widget(this, name, true, stoi(tokens[2])));
                    
                    if(tokens.size() == 4)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[3]));
                }
                else if(widgetClass == "VUMeter")
                {
                    if(tokens.size() == 5 || tokens.size() == 6)
                        widgets_.push_back(new VUMeter_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]))));
                    
                    if(tokens.size() == 6)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[5]));
                }
                else if(widgetClass == "GainReductionMeter")
                {
                    if(tokens.size() == 5 || tokens.size() == 6)
                        widgets_.push_back(new GainReductionMeter_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]))));
                    
                    if(tokens.size() == 6)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[5]));
                }
                else if(widgetClass == "Press")
                {
                    if(tokens.size() == 5 || tokens.size() == 6)
                        widgets_.push_back(new Press_Midi_Widget(this, name, false, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]))));
                    else if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Press_Midi_Widget(this, name, false, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 6)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[5]));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "PressFB")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Press_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "PressRelease")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new PressRelease_Midi_Widget(this, name, false, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "PressReleaseFB")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new PressRelease_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "Fader7Bit")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Fader7Bit_Midi_Widget(this, name, false, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "Fader7BitFB")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Fader7Bit_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "Fader14Bit")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Fader14Bit_Midi_Widget(this, name, false, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "Fader14BitFB")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Fader14Bit_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "Encoder")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Encoder_Midi_Widget(this, name, false, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                else if(widgetClass == "EncoderFB")
                {
                    if(tokens.size() == 8 || tokens.size() == 9)
                        widgets_.push_back(new Encoder_Midi_Widget(this, name, true, new MIDI_event_ex_t(strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])), new MIDI_event_ex_t(strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]))));
                    
                    if(tokens.size() == 9)
                        widgets_.back()->SetRefreshInterval(strToDouble(tokens[8]));
                }
                */
            }
        }
    }

    // Add the "hardcoded" widgets
    widgets_.push_back(new Midi_Widget(this, "TrackOnSelection", true));
    widgets_.push_back(new Midi_Widget(this, "TrackOnMapTrackAndFXToWidgets", true));
    widgets_.push_back(new Midi_Widget(this, "TrackOnFocusedFX", true));

    
    // GAW IMPORTANT -- This must happen AFTER the Widgets have been instantiated
    InitZones(string(DAW::GetResourcePath()) + "/CSI/Zones/" + zoneFolder + "/");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// ControlSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void ControlSurface::InitZones(string zoneFolder)
{
    vector<string> zoneFilesToProcess;
    listZoneFiles(zoneFolder, zoneFilesToProcess); // recursively find all the .zon files, starting at zoneFolder
    
    map<string, vector<CompositeZone*>> compositeZoneMembers;
    
    for(auto zoneFilename : zoneFilesToProcess)
    {
        ifstream zoneFile(zoneFilename);
        
        for (string line; getline(zoneFile, line) ; )
        {
            vector<string> tokens(GetTokens(line));
            
            if(tokens.size() == 2)
            {
                if(tokens[0] == "Zone")
                    ProcessZone(zoneFile, tokens);
                else if(tokens[0] == "CompositeZone")
                    ProcessCompositeZone(zoneFile, tokens, compositeZoneMembers);
            }
        }
    }
}

void ControlSurface::ProcessCompositeZone(ifstream &zoneFile, vector<string> tokens, map<string, vector<CompositeZone*>> &compositeZoneMembers)
{
    CompositeZone* compositeZone = new CompositeZone(this, tokens[1]);
    zones_[compositeZone->GetName()] = compositeZone;
    
    for (string line; getline(zoneFile, line) ; )
    {
        vector<string> tokens(GetTokens(line));
        
        if(tokens.size() == 1)
        {
            if(tokens[0] == "CompositeZoneEnd")    // finito baybay - CompositeZone processing complete
                return;
            else
                compositeZoneMembers[tokens[0]].push_back(compositeZone);
        }
    }
}

void ControlSurface::ProcessZone(ifstream &zoneFile, vector<string> tokens)
{
    
    string aString("Fader|");
    aString = regex_replace(aString, regex("\\|"), "1");
    
    

    
    
    
    const string GainReductionDB = "GainReductionDB"; // GAW TBD don't forget this logic

    Zone* zone = new Zone(this, tokens[1]);
    zones_[zone->GetName()] = zone;
    
    for (string line; getline(zoneFile, line) ; )
    {
        vector<string> tokens(GetTokens(line));

        if(tokens.size() > 0 && tokens[0] == "ZoneEnd")    // finito baybay - Zone processing complete
            return;
        
        // GAW -- the first token is the (possibly decorated with modifiers) Widget name.
        
        string widgetName = "";
        bool isInverted = false;
        bool shouldToggle = false;
        bool isDelayed = false;
        double delayAmount = 0.0;
        
        if(tokens.size() > 0)
        {
            istringstream modified_role(tokens[0]);
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

            Widget* widget = nullptr;
            
            for(auto * aWidget : widgets_)
                if(aWidget->GetName() == widgetName)
                    widget = aWidget;
            
            vector<string> params;
            for(int i = 1; i < tokens.size(); i++)
                params.push_back(tokens[i]);
            
            if(params.size() > 0 && widget != nullptr)
            {
                if(ActionContext* context = TheManager->GetActionContext(page_, this, params))
                {
                    if(isInverted)
                        context->SetIsInverted();
                    
                    if(shouldToggle)
                        context->SetShouldToggle();
                    
                    if(isDelayed)
                        context->SetDelayAmount(delayAmount * 1000.0);
                    
                    //if(widgetContexts_.count(widget) < 1)
                    //widgetContexts_[widget] = new WidgetContext(widget);
                    
                    //widgetContexts_[widget]->AddActionContext(Track, modifiers, context);
                    
                    if(params[0] == "TrackCycle")
                    {
                        for(auto * cyclerWidget : widgets_)
                            if(cyclerWidget->GetName() == params[1])
                            {
                                //if(widgetContexts_.count(cyclerWidget) < 1)
                                //widgetContexts_[cyclerWidget] = new WidgetContext(cyclerWidget);
                                
                                //widgetContexts_[cyclerWidget]->AddActionContext(Track, modifiers, context);
                                context->SetCyclerWidget(cyclerWidget);
                            }
                    }
                }
            }
        }
    }
}

void ControlSurface::ProcessActionZone(ifstream &zoneFile, vector<string> tokens)
{
    Zone* actionZone = new ActionZone(this, tokens[1]);
    zones_[actionZone->GetName()] = actionZone;
    
    for (string line; getline(zoneFile, line) ; )
    {
        vector<string> tokens(GetTokens(line));
        
        if(tokens.size() > 0)
        {
            if(tokens[0] == "ActionZoneEnd")    // finito baybay - ActionZone processing complete
                return;
            
            
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Page
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Page::InitActionContexts(ControlSurface* surface, string templateFilename)
{
    /*
    bool inChannel = false;
    
    ifstream actionTemplateFile(templateFilename);
    
    for (string line; getline(actionTemplateFile, line) ; )
    {
        if(line[0] != '\r' && line[0] != '/' && line != "") // ignore comment lines and blank lines
        {
            istringstream iss(line);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);
            
            // GAW -- the first token is the (possibly decorated with modifiers) Widget name.
            
            string modifiers = "";
            string widgetName = "";
            bool isInverted = false;
            bool shouldToggle = false;
            bool isDelayed = false;
            double delayAmount = 0.0;
            
            if(tokens.size() > 0)
            {
                if(tokens[0] == "BankableChannel")
                {
                    inChannel = true;
                    bankableChannels_.push_back(new BankableChannel());
                }
                else if(tokens[0] == "BankableChannelEnd")
                {
                    inChannel = false;
                }
                
                istringstream modified_role(tokens[0]);
                vector<string> modifier_tokens;
                string modifier_token;
                
                while (getline(modified_role, modifier_token, '+'))
                    modifier_tokens.push_back(modifier_token);
                
                widgetName = modifier_tokens[modifier_tokens.size() - 1];
                
                if(modifier_tokens.size() > 1)
                {
                    vector<string> modifierSlots = { "", "", "", "" };
                    
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
                        else if(modifier_tokens[i] == "Invert")
                            isInverted = true;
                        else if(modifier_tokens[i] == "Toggle")
                            shouldToggle = true;
                        else if(modifier_tokens[i] == "Hold")
                        {
                            isDelayed = true;
                            delayAmount = 1.0;
                        }
                    }
                    
                    modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
                }
            }
            
            vector<string> params;
            for(int i = 1; i < tokens.size(); i++)
                params.push_back(tokens[i]);
            
            if(tokens.size() > 1)
                for(auto * widget : surface->GetAllWidgets())
                    if(widget->GetName() == widgetName)
                        if(ActionContext* context = TheManager->GetActionContext(this, surface, params))
                        {
                            if(inChannel)
                                (bankableChannels_.back())->AddWidget(widget);
                            
                            if(isInverted)
                                context->SetIsInverted();
                            
                            if(shouldToggle)
                                context->SetShouldToggle();
                            
                            if(isDelayed)
                                context->SetDelayAmount(delayAmount * 1000.0);
                           
                            if(widgetContexts_.count(widget) < 1)
                                widgetContexts_[widget] = new WidgetContext(widget);
                            
                            widgetContexts_[widget]->AddActionContext(Track, modifiers, context);
                            
                            if(params[0] == "TrackCycle")
                            {
                                for(auto * cyclerWidget : surface->GetAllWidgets())
                                    if(cyclerWidget->GetName() == params[1])
                                    {
                                        if(widgetContexts_.count(cyclerWidget) < 1)
                                            widgetContexts_[cyclerWidget] = new WidgetContext(cyclerWidget);
                                        
                                        widgetContexts_[cyclerWidget]->AddActionContext(Track, modifiers, context);
                                        context->SetCyclerWidget(cyclerWidget);
                                    }
                            }
                        }
        }
    }
*/
}

void Page::InitFXContexts(ControlSurface* surface, string templateDirectory)
{
    /*
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'f' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream fxTemplateFile(string(templateDirectory + "/" + filename));
            
            string fxName;
            getline(fxTemplateFile, fxName);
            
            for (string line; getline(fxTemplateFile, line) ; )
            {
                if(line[0] != '\r' && line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    // GAW -- the first token is the (possibly decorated with modifiers) Widget name
                    
                    string modifiers = "";
                    string widgetName = "";
                    bool isInverted = false;
                    bool shouldToggle = false;
                    bool isDelayed = false;
                    double delayTime = 0.0;

                    if(tokens.size() > 0)
                    {
                        istringstream modified_name(tokens[0]);
                        vector<string> modifier_tokens;
                        string modifier_token;
                        
                        while (getline(modified_name, modifier_token, '+'))
                            modifier_tokens.push_back(modifier_token);
                        
                        widgetName = modifier_tokens[modifier_tokens.size() - 1];
                        
                        if(modifier_tokens.size() > 1)
                        {
                            vector<string> modifierSlots = { "", "", "", "" };
                            
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
                                else if(modifier_tokens[i] == Invert)
                                    isInverted = true;
                                else if(modifier_tokens[i] == Toggle)
                                    shouldToggle = true;
                                else if(modifier_tokens[i] == Hold)
                                {
                                    isDelayed = true;
                                    delayTime = 2.0;
                                }
                            }
                            
                            modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
                        }
                    }
                    
                    string fxParamName = tokens[1];
                    
                    vector<string> params;
                    
                    string alias = "";
                    
                    if(fxParamName == GainReductionDB)
                        params.push_back(fxParamName);
                    else if(tokens.size() > 2 && tokens[2] == "TrackFXParamNameDisplay")
                    {
                        if(tokens.size() > 3)
                            alias = tokens[3];
                        params.push_back("TrackFXParamNameDisplay");
                    }
                    else if(tokens.size() > 2 && tokens[2] == "TrackFXParamValueDisplay")
                        params.push_back("TrackFXParamValueDisplay");
                    else params.push_back("TrackFX");
                    params.push_back(fxParamName);
                    params.push_back(fxName);

                    if(tokens.size() > 1)
                        for(auto * widget : surface->GetAllWidgets())
                            if(widget->GetName() == widgetName)
                                if(ActionContext* context = TheManager->GetFXActionContext(this, surface, widget, params, alias))
                                {
                                    if(isInverted)
                                        context->SetIsInverted();

                                    if(shouldToggle)
                                        context->SetShouldToggle();

                                    if(isDelayed)
                                        context->SetDelayAmount(delayTime * 1000.0);

                                    if(widgetContexts_.count(widget) < 1)
                                        widgetContexts_[widget] = new WidgetContext(widget);
                                    
                                    widgetContexts_[widget]->AddActionContext(fxName, modifiers, context);
                                    fxWidgets_[fxName].push_back(widget);
                                }
                }
            }
        }
    }
*/
}

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
        int high = low + bankableChannels_.size() - 1 - GetNumPinnedTracks();
        
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
    
    int bottom = 1 - bankableChannels_.size() + GetNumPinnedTracks();
    
    if(trackOffset_ <  bottom)
        trackOffset_ =  bottom;
    
    int top = DAW::CSurf_NumTracks(followMCP_) - 1;
    
    if(trackOffset_ >  top)
        trackOffset_ = top;
    
    vector<string> pinnedChannels;
    
    GetPinnedChannelGUIDs(pinnedChannels);

    while(trackOffset_ >= 0 && trackOffset_ < DAW::CSurf_NumTracks(followMCP_))
    {
        string trackGUID = DAW::GetTrackGUIDAsString(trackOffset_, followMCP_);

        if(find(pinnedChannels.begin(), pinnedChannels.end(), trackGUID) != pinnedChannels.end())
            previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
        else if(! IsTrackVisible(DAW::CSurf_TrackFromID(trackOffset_, followMCP_)))
            previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
        else
           break;
    }
    
    
    
    /*
    
    // Jump over any pinned channels and invisible tracks
    //vector<string> pinnedChannels;
    //for(auto* channel : bankableChannels_)
        //if(channel->GetIsPinned())
            //pinnedChannels.push_back(channel->GetTrackGUID());
    
    bool skipThisChannel = false;
    
    while(trackOffset_ >= 0 && trackOffset_ < DAW::CSurf_NumTracks(followMCP_))
    {
        string trackGUID = DAW::GetTrackGUIDAsString(trackOffset_, followMCP_);
        
        for(auto pinnedChannel : pinnedChannels)
            if(pinnedChannel == trackGUID)
            {
                skipThisChannel = true;
                previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
                break;
            }
        
        if( ! IsTrackVisible(DAW::CSurf_TrackFromID(trackOffset_, followMCP_)))
        {
            skipThisChannel = true;
            previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
        }
        
        if(skipThisChannel)
        {
            skipThisChannel = false;
            continue;
        }
        else
            break;
    }
    
    */
    
    if(previousTrackOffset != trackOffset_)
    {
        DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetNumberString() + "BankOffset").c_str(), to_string(trackOffset_).c_str());
        DAW::MarkProjectDirty(nullptr);
    }
    
    RefreshLayout();        
}

void Page::RefreshLayout()
{
    currentlyRefreshingLayout_ = true;
    
    vector<string> pinnedChannels;

    GetPinnedChannelGUIDs(pinnedChannels);
    
    vector<string> layoutChannels(bankableChannels_.size() + pinnedChannels.size());

    int layoutChannelIndex = 0;
    
    for(int i = trackOffset_; i < DAW::CSurf_NumTracks(followMCP_) && layoutChannelIndex < layoutChannels.size(); i++)
    {
        if(i < 0)
            layoutChannelIndex++;
        else if(! IsTrackVisible(DAW::CSurf_TrackFromID(i, followMCP_)))
            pinnedChannels.push_back(DAW::GetTrackGUIDAsString(i, followMCP_));
        else
            layoutChannels[layoutChannelIndex++] = DAW::GetTrackGUIDAsString(i, followMCP_);
    }
    
    subtract_vector(layoutChannels, pinnedChannels);
    
    if(colourTracks_ && TheManager->GetCurrentPage() == this)
    {
        DAW::PreventUIRefresh(1);
        
        // reset track colors
        for(auto* channel : bankableChannels_)
            if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
                if(trackColours_.count(channel->GetTrackGUID()) > 0)
                    DAW::GetSetMediaTrackInfo(track, "I_CUSTOMCOLOR", &trackColours_[channel->GetTrackGUID()]);
    }
    
    
    // Apply new layout
    layoutChannelIndex = 0;
    
    for(auto* channel : bankableChannels_)
        if(! channel->GetIsPinned())
            channel->SetTrackGUID(this, layoutChannels[layoutChannelIndex++]);

    
    if(colourTracks_ && TheManager->GetCurrentPage() == this)
    {
        // color tracks
        int color = DAW::ColorToNative(trackColourRedValue_, trackColourGreenValue_, trackColourBlueValue_) | 0x1000000;
        for(auto* channel : bankableChannels_)
            if(MediaTrack* track = DAW::GetTrackFromGUID(channel->GetTrackGUID(), followMCP_))
            {
                trackColours_[channel->GetTrackGUID()] = DAW::GetTrackColor(track);
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
    actions_["MapTrackToWidgets"] = new MapTrackToWidgets();
    actions_["MapFXToWidgets"] = new MapFXToWidgets();
    actions_["MapTrackAndFXToWidgets"] = new MapTrackAndFXToWidgets();
    actions_["MapTrackAndFXToWidgetsForTrack"] = new MapTrackAndFXToWidgetsForTrack();
    actions_["MapSingleFXToWidgetsForTrack"] = new MapSingleFXToWidgetsForTrack();
    actions_["GlobalMapTrackAndFXToWidgetsForTrack"] = new GlobalMapTrackAndFXToWidgetsForTrack();
}

void Manager::InitActionContextDictionary()
{
    InitActionDictionary();
    
    actionContexts_["Reaper"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new ReaperActionContext(page, surface, actions_[params[0]], params[1]); };
    actionContexts_["TrackFX"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new FXContext(page, surface, actions_[params[0]], params[1]); };
    actionContexts_["TrackFXParamNameDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new FXContext(page, surface, actions_[params[0]], params[1]); };
    actionContexts_["TrackFXParamValueDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new FXContext(page, surface, actions_[params[0]], params[1]); };
    actionContexts_["GainReductionDB"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackVolume"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["MasterTrackVolume"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSendVolume"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackSendContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSendPan"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackSendContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSendMute"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackSendContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackVolumeDB"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackPan"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackPanWidth"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackNameDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackVolumeDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSendNameDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackSendContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSendVolumeDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackSendContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackPanDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackPanWidthDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TimeDisplay"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["Rewind"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["FastForward"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["Play"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["Stop"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["Record"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSelect"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackUniqueSelect"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["MasterTrackUniqueSelect"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackRangeSelect"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackRecordArm"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackMute"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackSolo"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackTouch"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["MasterTrackTouch"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackTouchControlled"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackTouchControlledContext(page, surface, actions_[params[1]], actions_[params[2]]); };
    actionContexts_["TrackSendTouchControlled"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackSendTouchControlledContext(page, surface, actions_[params[1]], actions_[params[2]]); };
    actionContexts_["CycleTimeline"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackOutputMeter"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["MasterTrackOutputMeter"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["SetShowFXWindows"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["SetScrollLink"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["CycleTimeDisplayModes"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["NextPage"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["GoPage"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContextWithStringParam(page, surface, actions_[params[0]], params[1]); };
    actionContexts_["SelectTrackRelative"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackBank"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["TrackSendBank"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContextWithIntParam(page, surface, actions_[params[0]], atol(params[1].c_str())); };
    actionContexts_["PinSelectedTracks"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["UnpinSelectedTracks"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new GlobalContext(page, surface, actions_[params[0]]); };
    actionContexts_["MapTrackToWidgets"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new PageSurfaceContext(page, surface, actions_[params[0]]); };
    actionContexts_["MapFXToWidgets"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new PageSurfaceContext(page, surface, actions_[params[0]]); };
    actionContexts_["MapTrackAndFXToWidgets"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new PageSurfaceContext(page, surface, actions_[params[0]]); };
    actionContexts_["MapTrackAndFXToWidgetsForTrack"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new PageSurfaceContext(page, surface, actions_[params[0]]); };
    actionContexts_["MapSingleFXToWidgetsForTrack"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new PageSurfaceContext(page, surface, actions_[params[0]]); };
    actionContexts_["GlobalMapTrackAndFXToWidgetsForTrack"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackContext(page, surface, actions_[params[0]]); };
    actionContexts_["TrackCycle"] = [this](Page* page, ControlSurface* surface, vector<string> params) { return new TrackCycleContext(page, surface, actions_[params[0]], params); };
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

static vector<MidiChannelInput> midiInputs_;
static vector<MidiChannelOutput> midiOutputs_;

static midi_Input* GetMidiInputForChannel(int inputChannel)
{
    for(auto input : midiInputs_)
        if(input.channel_ == inputChannel)
            return input.midiInput_; // return existing
    
    // make new
    midi_Input* newInput = DAW::CreateMIDIInput(inputChannel);
    
    if(newInput)
    {
        newInput->start();
        midiInputs_.push_back(MidiChannelInput(inputChannel, newInput));
        return newInput;
    }
    
    return nullptr;
}

static midi_Output* GetMidiOutputForChannel(int outputChannel)
{
    for(auto output : midiOutputs_)
        if(output.channel_ == outputChannel)
            return output.midiOutput_; // return existing
    
    // make new
    midi_Output* newOutput = DAW::CreateMIDIOutput(outputChannel, false, NULL );
    
    if(newOutput)
    {
        midiOutputs_.push_back(MidiChannelOutput(outputChannel, newOutput));
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
    
    ifstream iniFile(string(DAW::GetResourcePath()) + "/CSI/CSI.ini");
    
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
                     currentPage->AddSurface(new Midi_ControlSurface(currentPage, tokens[1], tokens[4], tokens[5], GetMidiInputForChannel(channelIn), GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor), tokens[5], tokens[6]);
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
