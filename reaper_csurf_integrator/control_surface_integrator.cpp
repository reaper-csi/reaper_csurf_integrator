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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidgeta available for inclusion in Real Surface Templates, we will add widgets as necessary
////////////////////////////////////////////////////////////////////////////////////////////////////////
int strToHex(string valueStr)
{
    return strtol(valueStr.c_str(), nullptr, 16);
}

double strToDouble(string valueStr)
{
    return strtod(valueStr.c_str(), nullptr);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string name, string widgetClass)
{
    if(widgetClass == "MCUTimeDisplay") return new MCU_TimeDisplay_Midi_Widget(surface, role, name, true);
    
    return new Midi_Widget(surface, role, name, false);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string name, string widgetClass, int channel)
{
    if(widgetClass == "MCUDisplayUpper") return new MCUDisplay_Midi_Widget(surface, role, name, true, 0, 0x14, 0x12, channel);
    else if(widgetClass == "MCUDisplayLower") return new MCUDisplay_Midi_Widget(surface, role, name, true, 1, 0x14, 0x12, channel);
    else if(widgetClass == "MCUXTDisplayUpper") return new MCUDisplay_Midi_Widget(surface, role, name, true, 0, 0x15, 0x12, channel);
    else if(widgetClass == "MCUXTDisplayLower") return new MCUDisplay_Midi_Widget(surface, role, name, true, 1, 0x15, 0x12, channel);
    else if(widgetClass == "MCUVUMeter") return new MCUVUMeter_Midi_Widget(surface, role, name, true, channel);
    
    else if(widgetClass == "C4DisplayUpper" || widgetClass == "C4DisplayLower")
    {
        int displayRow = 0;
       
        for(int i = name.length() - 1; i > 0; i--)
            if(isalpha(name[i]))
            {
                if(name[i] == 'A')
                    displayRow = 0;
                else if(name[i] == 'B')
                    displayRow = 1;
                else if(name[i] == 'C')
                    displayRow = 2;
                else if(name[i] == 'D')
                    displayRow = 3;
                break;
            }
        
        if(widgetClass == "C4DisplayUpper") return new MCUDisplay_Midi_Widget(surface, role, name, true, 0, 0x17, displayRow + 0x30, channel);
        else if(widgetClass == "C4DisplayLower") return new MCUDisplay_Midi_Widget(surface, role, name, true, 1, 0x17, displayRow + 0x30, channel);
    }
    
    return new Midi_Widget(surface, role, name, false);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string name, string widgetClass, int byte1, int byte2, int byte3)
{
    if(widgetClass == "Press") return new Press_Midi_Widget(surface, role, name, false, new MIDI_event_ex_t(byte1, byte2, byte3));
    
    return new Midi_Widget(surface, role, name, false);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2, int byte3)
{
    if(widgetClass == "VUMeter") return new VUMeter_Midi_Widget(surface, role, name, true, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3));
    else if(widgetClass == "GainReductionMeter") return new GainReductionMeter_Midi_Widget(surface, role, name, true, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3));
    
    return new Midi_Widget(surface, role, name, false);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string name, string widgetClass, int byte1, int byte2, int byte3, int byte4, int byte5, int byte6)
{
    if(widgetClass == "Press") return new Press_Midi_Widget(surface, role, name, false, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    
    else if(widgetClass == "PressFB") return new Press_Midi_Widget(surface, role, name, true, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "PressRelease") return new PressRelease_Midi_Widget(surface, role, name, false, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "PressReleaseFB") return new PressRelease_Midi_Widget(surface, role, name, true, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    
    // Special for X Touch Compact
    else if(widgetClass == "PressFBR") return new PressWithResendOnRelease_Midi_Widget(surface, role, name, true, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "PressReleaseR") return new PressReleaseWithResendOnRelease_Midi_Widget(surface, role, name, false, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "PressReleaseFBR") return new PressReleaseWithResendOnRelease_Midi_Widget(surface, role, name, true, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    
    else if(widgetClass == "Fader7Bit") return new Fader7Bit_Midi_Widget(surface, role, name, false, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "Fader7BitFB") return new Fader7Bit_Midi_Widget(surface, role, name, true, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "Encoder") return new Encoder_Midi_Widget(surface, role, name, false, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "EncoderFB") return new Encoder_Midi_Widget(surface, role, name, true, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));

    return new Midi_Widget(surface, role, name, false);
}

Midi_Widget* WidgetFor(Midi_RealSurface* surface, string role, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2, int byte3, int byte4, int byte5, int byte6)
{
    if(widgetClass == "Fader14Bit") return new Fader14Bit_Midi_Widget(surface, role, name, false, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));
    else if(widgetClass == "Fader14BitFB") return new Fader14Bit_Midi_Widget(surface, role, name, true, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3), new MIDI_event_ex_t(byte4, byte5, byte6));

    return new Midi_Widget(surface, role, name, false);
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Midi_RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
Midi_RealSurface::Midi_RealSurface(Page* page, const string name, string templateFilename, int numChannels, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor)
: RealSurface(page, name, numChannels), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor)
{
    ifstream surfaceTemplateFile(string(DAW::GetResourcePath()) + "/CSI/rst/" + templateFilename);
    bool inChannel = false;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        if(line[0] != '\r' && line[0] != '/' && line != "") // ignore comment lines and blank lines
        {
            istringstream iss(line);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);
            
            if(tokens.size() == 1)
            {
                if(tokens[0] == Channel)
                    inChannel = true;
                else if(tokens[0] == ChannelEnd)
                    inChannel = false;
            }
            else if(tokens.size() == 2)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[0] + to_string(i + 1), tokens[1], i));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[0], tokens[1]));
            }
            else if(tokens.size() == 5)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[0] + to_string(i + 1), tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4])));
            }
            else if(tokens.size() == 7)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[0] + to_string(i + 1), tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]) + i, strToHex(tokens[6])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6])));
            }
            else if(tokens.size() == 8)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[0] + to_string(i + 1), tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]) + i, strToHex(tokens[7])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7])));
            }
            else if(tokens.size() == 10)
            {
                if(inChannel)
                    for(int i = 0; i < GetNumChannels(); i++)
                        AddWidget(i, WidgetFor(this, tokens[0], tokens[0] + to_string(i + 1), tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]) + i, strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8]), strToHex(tokens[9])));
                else
                    AddWidget(WidgetFor(this, tokens[0], tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8]), strToHex(tokens[9])));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// BankableChannel
////////////////////////////////////////////////////////////////////////////////////////////////////////
void BankableChannel::SetTrackGUID(Page* page, string trackGUID)
{
    trackGUID_ = trackGUID;
    
    for(auto widget : widgets_)
        if(WidgetContext* widgetContext = page->GetWidgetContext(widget))
            widgetContext->SetComponentTrackContext(Track, trackGUID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Page
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Page::InitActionContexts(RealSurface* surface, string templateFilename)
{
    bool trackOnSelectionWidgetAdded_ = false;
   
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
            string widgetRole = "";
            bool isInverted = false;
            
            if(tokens.size() > 0)
            {
                istringstream modified_role(tokens[0]);
                vector<string> modifier_tokens;
                string modifier_token;
                
                while (getline(modified_role, modifier_token, '+'))
                    modifier_tokens.push_back(modifier_token);
                
                widgetRole = modifier_tokens[modifier_tokens.size() - 1];
                
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
                    }
                    
                    modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
                }
            }

            // GAW IMPORTANT -- If widgetRole == "OnTrackSelection", add a MIDI widget to the surface so that we can attach ActionContexts
            // Timing is important here, the widget must be added BEFORE the widget->GetRole() == widgetRole comparison below
            if(widgetRole == "TrackOnSelection" && ! trackOnSelectionWidgetAdded_)
            {
                trackOnSelectionWidgetAdded_ = true;
                surface->AddWidget(new Midi_Widget((Midi_RealSurface*)surface, widgetRole, widgetRole, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00)));
            }

            vector<string> params;
            for(int i = 1; i < tokens.size(); i++)
                params.push_back(tokens[i]);
            
            if(tokens.size() > 1)
                for(auto * widget : surface->GetAllWidgets())
                    if(widget->GetRole() == widgetRole)
                        if(ActionContext* context = TheManager->GetActionContext(params, isInverted))
                        {
                            if(widgetContexts_.count(widget) < 1)
                                widgetContexts_[widget] = new WidgetContext(widget);
                            
                            widgetContexts_[widget]->AddActionContext(Track, modifiers, context);
                            
                            if(params[0] == "TrackCycle")
                            {
                                for(auto * cyclerWidget : surface->GetChannelWidgets(widget))
                                    if(cyclerWidget->GetRole() == params[1])
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
}

void Page::InitFXContexts(RealSurface* surface, string templateDirectory)
{
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
                            }
                            
                            modifiers = modifierSlots[0] + modifierSlots[1] + modifierSlots[2] + modifierSlots[3];
                        }
                    }
                    
                    string fxParamName = tokens[1];
                    
                    vector<string> params;
                    
                    string alias = "";
                    
                    if(fxParamName == "GainReductionDB")
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
                                if(ActionContext* context = TheManager->GetFXActionContext(params, isInverted, alias))
                                {
                                    if(shouldToggle)
                                        context->SetShouldtoggle();
                                    
                                    if(widgetContexts_.count(widget) < 1)
                                        widgetContexts_[widget] = new WidgetContext(widget);
                                    
                                    widgetContexts_[widget]->AddActionContext(fxName, modifiers, context);
                                    fxWidgets_[fxName].push_back(widget);
                                }
                }
            }
        }
    }
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
    
    for(auto surface : realSurfaces_)
        for(auto widget : surface->GetAllWidgets())
            if(widget->GetRole() == "TrackOnSelection")
                if(widgetContexts_.count(widget) > 0)
                    widgetContexts_[widget]->DoAction(this, GetCurrentModifiers(), surface, track);
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
    
    // Jump over any pinned channels and invisible tracks
    vector<string> pinnedChannels;
    for(auto* channel : bankableChannels_)
        if(channel->GetIsPinned())
            pinnedChannels.push_back(channel->GetTrackGUID());
    
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
    
    vector<MediaTrack*> visibleTracks;
    
    for(int i = 0; i < DAW::CSurf_NumTracks(followMCP_); i++)
        visibleTracks.push_back(DAW::CSurf_TrackFromID(i, followMCP_));
    
    vector<string> pinnedChannelLayout;
    vector<string> pinnedChannels;
    int offset = trackOffset_;
    vector<string> movableChannelLayout;
    
    for(auto* channel : bankableChannels_)
    {
        // Layout Pinned GUIDs
        if(channel->GetIsPinned())
        {
            pinnedChannelLayout.push_back(channel->GetTrackGUID());
            pinnedChannels.push_back(channel->GetTrackGUID());
        }
        else
            pinnedChannelLayout.push_back("");
        
        // Layout Channel GUIDs
        if(offset < 0)
        {
            movableChannelLayout.push_back("");
            offset++;
        }
        else if(offset >= DAW::CSurf_NumTracks(followMCP_))
        {
            movableChannelLayout.push_back("");
        }
        else
        {
            movableChannelLayout.push_back(DAW::GetTrackGUIDAsString(visibleTracks[offset++], followMCP_));
        }
    }
    
    // Remove Pinned Channels
    for(int i = 0; i < pinnedChannels.size(); i++)
    {
        auto iter = find(movableChannelLayout.begin(), movableChannelLayout.end(), pinnedChannels[i]);
        if(iter != movableChannelLayout.end())
            movableChannelLayout.erase(iter);
    }
    
    // Merge the layouts
    offset = 0;
    vector<string> channelLayout;
    
    for(int i = 0; i < bankableChannels_.size(); i++)
    {
        if(pinnedChannelLayout[i] != "")
            channelLayout.push_back(pinnedChannelLayout[i]);
        else
            channelLayout.push_back(movableChannelLayout[offset++]);
    }
    
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
    offset = 0;
    for(auto* channel : bankableChannels_)
        channel->SetTrackGUID(this, channelLayout[offset++]);
    
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
    actions_["TrackRangeSelect"] = new TrackRangeSelect();
    actions_["TrackRecordArm"] = new TrackRecordArm();
    actions_["TrackMute"] = new TrackMute();
    actions_["TrackSolo"] = new TrackSolo();
    actions_["TrackTouch"] = new TrackTouch();
    actions_["TrackTouchControlled"] = new TrackTouchControlled();
    actions_["TrackSendTouchControlled"] = new TrackTouchControlled();
    actions_["CycleTimeline"] = new CycleTimeline();
    actions_["TrackOutputMeter"] = new TrackOutputMeter();
    actions_["SetShowFXWindows"] = new SetShowFXWindows();
    actions_["SetScrollLink"] = new SetScrollLink();
    actions_["CycleTimeDisplayModes"] = new CycleTimeDisplayModes();
    actions_["NextPage"] = new class NextPage();
    actions_["TrackBank"] = new TrackBank();
    actions_["TrackSendBank"] = new TrackSendBank();
    actions_["PinSelectedTracks"] = new PinSelectedTracks();
    actions_["UnpinSelectedTracks"] = new UnpinSelectedTracks();
    actions_["MapTrackToWidgets"] = new MapTrackToWidgets();
    actions_["MapFXToWidgets"] = new MapFXToWidgets();
    actions_["MapTrackAndFXToWidgets"] = new MapTrackAndFXToWidgets();
}

void Manager::InitActionContextDictionary()
{
    InitActionDictionary();
    
    actionContexts_["Reaper"] = [this](vector<string> params, bool isInverted) { return new ReaperActionContext(actions_[params[0]], params[1], isInverted); };
    actionContexts_["TrackFX"] = [this](vector<string> params, bool isInverted) { return new FXContext(actions_[params[0]], params[1], isInverted); };
    actionContexts_["TrackFXParamNameDisplay"] = [this](vector<string> params, bool isInverted) { return new FXContext(actions_[params[0]], params[1], isInverted); };
    actionContexts_["TrackFXParamValueDisplay"] = [this](vector<string> params, bool isInverted) { return new FXContext(actions_[params[0]], params[1], isInverted); };
    actionContexts_["GainReductionDB"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackVolume"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSendVolume"] = [this](vector<string> params, bool isInverted) { return new TrackSendContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSendPan"] = [this](vector<string> params, bool isInverted) { return new TrackSendContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSendMute"] = [this](vector<string> params, bool isInverted) { return new TrackSendContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackVolumeDB"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPan"] = [this](vector<string> params, bool isInverted) { return new TrackContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["TrackPanWidth"] = [this](vector<string> params, bool isInverted) { return new TrackContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["TrackNameDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackVolumeDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSendNameDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackSendContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSendVolumeDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackSendContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPanDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackPanWidthDisplay"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TimeDisplay"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Rewind"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["FastForward"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Play"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Stop"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["Record"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSelect"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackUniqueSelect"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackRangeSelect"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackRecordArm"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackMute"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackSolo"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackTouch"] = [this](vector<string> params, bool isInverted) { return new TrackContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackTouchControlled"] = [this](vector<string> params, bool isInverted) { return new TrackTouchControlledContext(actions_[params[1]], actions_[params[2]], isInverted); };
    actionContexts_["TrackSendTouchControlled"] = [this](vector<string> params, bool isInverted) { return new TrackSendTouchControlledContext(actions_[params[1]], actions_[params[2]], isInverted); };
    actionContexts_["CycleTimeline"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackOutputMeter"] = [this](vector<string> params, bool isInverted) { return new TrackContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["SetShowFXWindows"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["SetScrollLink"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["CycleTimeDisplayModes"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["NextPage"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackBank"] = [this](vector<string> params, bool isInverted) { return new GlobalContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["TrackSendBank"] = [this](vector<string> params, bool isInverted) { return new GlobalContextWithIntParam(actions_[params[0]], atol(params[1].c_str()), isInverted); };
    actionContexts_["PinSelectedTracks"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["UnpinSelectedTracks"] = [this](vector<string> params, bool isInverted) { return new GlobalContext(actions_[params[0]], isInverted); };
    actionContexts_["MapTrackToWidgets"] = [this](vector<string> params, bool isInverted) { return new PageSurfaceContext(actions_[params[0]], isInverted); };
    actionContexts_["MapFXToWidgets"] = [this](vector<string> params, bool isInverted) { return new PageSurfaceContext(actions_[params[0]], isInverted); };
    actionContexts_["MapTrackAndFXToWidgets"] = [this](vector<string> params, bool isInverted) { return new PageSurfaceContext(actions_[params[0]], isInverted); };
    actionContexts_["TrackCycle"] = [this](vector<string> params, bool isInverted) { return new TrackCycleContext(params, actions_[params[0]], isInverted); };
}

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
        if(line[0] != '/' && line != "") // ignore comment lines and blank lines
        {
            istringstream iss(line);
            vector<string> tokens;
            string token;
            
            while (iss >> quoted(token))
                tokens.push_back(token);
            
            if(tokens[0] == MidiInMonitor)
            {
                if(tokens.size() != 2)
                    continue;
                
                if(tokens[1] == "On")
                    midiInMonitor = true;
            }
            else if(tokens[0] == MidiOutMonitor)
            {
                if(tokens.size() != 2)
                    continue;
                
                if(tokens[1] == "On")
                    midiOutMonitor = true;
            }
            else if(tokens[0] == VSTMonitor)
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
            else if(tokens[0] == MidiSurface)
            {
                if(tokens.size() != 9)
                    continue;
                
                int numChannels = atoi(tokens[2].c_str());
                bool isBankable = tokens[3] == "Bankable" ? true : false;
                int channelIn = atoi(tokens[4].c_str());
                int channelOut = atoi(tokens[5].c_str());

                if(currentPage)
                    currentPage->AddSurface(new Midi_RealSurface(currentPage, tokens[1], tokens[6], numChannels, midiIOManager_->GetMidiInputForChannel(channelIn), midiIOManager_->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor), isBankable, tokens[7], tokens[8]);
            }
        }
    }
    
    for(auto page : pages_)
        page->Init();
    
    char buffer[BUFSZ];
    if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), "PageIndex", buffer, sizeof(buffer)))
        currentPageIndex_ = atol(buffer);
}
