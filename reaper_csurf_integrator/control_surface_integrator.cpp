//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_Reaper_actions.h"
#include "control_surface_manager_actions.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reaper Actions available for mapping, this list will get added to substantially over time
////////////////////////////////////////////////////////////////////////////////////////////////////////
Action* ActionFor(string name, Layout* layout)
{
    if(name == "SetShowFXWindows")  return new SetShowFXWindows_Action(layout);
    else if(name == "Rewind")  return new Rewind_Action(layout);
    else if(name == "FastForward")  return new FastForward_Action(layout);
    else if(name == "Stop")  return new Stop_Action(layout);
    else if(name == "Play")  return new Play_Action(layout);
    else if(name == "Record")  return new Record_Action(layout);
    else if(name == "NextLayout")  return new NextLayout_Action(layout);
    else if(name == "PinSelectedTracks")  return new PinSelectedTracks_Action(layout);
    else if(name == "UnpinSelectedTracks")  return new UnpinSelectedTracks_Action(layout);
    else if(name == "Shift")  return new Shift_Action(layout);
    else if(name == "Option")  return new Option_Action(layout);
    else if(name == "Control")  return new Control_Action(layout);
    else if(name == "Alt")  return new Alt_Action(layout);
    else if(name == "LatchedZoom")  return new LatchedZoom_Action(layout);
    else if(name == "LatchedScrub")  return new LatchedScrub_Action(layout);
    else if(name == "Cycled")  return new Cycled_Action(layout);
    else if(name == "CycleTimeline")  return new CycleTimeline_Action(layout);
    
    return new Action(layout);
}

Action* ActionFor(string name, Layout* layout, string param)
{
    if(name == "TrackBank")  return new TrackBank_Action(layout, param);
    else if(name == "TrackAutoMode")  return new TrackAutoMode_Action(layout, param);
    else if(name == "GlobalAutoMode")  return new GlobalAutoMode_Action(layout, param);
    else if(name == "Reaper")  return new Reaper_Action(layout, param);
    
    return new Action(layout);
}

Action* TrackActionFor(string name, Layout* layout, string trackGUID)
{
    if(name == "TrackVolume")  return new TrackVolume_Action(layout, trackGUID);
    else if(name == "TrackVolumeDisplay")  return new TrackVolumeDisplay_Action(layout, trackGUID);
    else if(name == "TrackPanDisplay")  return new TrackPanDisplay_Action(layout, trackGUID);
    else if(name == "TrackPanWidthDisplay")  return new TrackPanWidthDisplay_Action(layout, trackGUID);
    else if(name == "TrackPan")  return new TrackPan_Action(layout, trackGUID);
    else if(name == "TrackPanWidth")  return new TrackPanWidth_Action(layout, trackGUID);
    else if(name == "TrackTouch")  return new TrackTouch_Action(layout, trackGUID);
    else if(name == "TrackMute")  return new TrackMute_Action(layout, trackGUID);
    else if(name == "TrackSolo")  return new TrackSolo_Action(layout, trackGUID);
    else if(name == "TrackUniqueSelect")  return new TrackUniqueSelect_Action(layout, trackGUID);
    else if(name == "TrackRangeSelect")  return new TrackRangeSelect_Action(layout, trackGUID);
    else if(name == "TrackSelect")  return new TrackSelect_Action(layout, trackGUID);
    else if(name == "TrackRecordArm")  return new TrackRecordArm_Action(layout, trackGUID);
    else if(name == "TrackNameDisplay")  return new TrackNameDisplay_Action(layout, trackGUID);
    else if(name == "MapTrackAndFXToWidgets")  return new MapTrackAndFXToWidgets_Action(layout, trackGUID);
    
    return new Action(layout);
}

Action* TrackActionFor(string name, Layout* layout, string trackGUID, string param)
{
    if(name == "TrackOutputMeter")  return new TrackOutputMeter_Action(layout, trackGUID, param);
    
    return new Action(layout);
}

Action* TrackActionFor(string name, string actionAddress, Layout* layout, string trackGUID, Action* baseAction)
{
    if(name == "TrackTouchControlled")  return new TrackTouchControlled_Action(actionAddress, layout, trackGUID, baseAction);
    
    return new Action(layout);
}

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

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, int index)
{
    if(widgetClass == "DisplayUpper") return new DisplayUpper_MidiWidget(surface, name, index);
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00));
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "Button") return new PushButton_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithLatch") return new PushButtonWithLatch_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithRelease") return new PushButtonWithRelease_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonCycler") return new PushButtonCycler_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Encoder") return new Encoder_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Fader7Bit") return new Fader7Bit_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Min), new MIDI_event_ex_t(byte1, byte2, byte3Max));
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "VUMeter") return new VUMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "GainReductionMeter") return new GainReductionMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
  
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2Min, int byte2Max, int byte3Min, int byte3Max)
{
    if(widgetClass == "Fader14Bit") return new Fader14Bit_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void MidiWidget::AddToRealSurface(RealSurface* surface)
{
    surface->AddWidgetToNameMap(GetName(), this);
}

void MidiWidget::Update()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->UpdateAction(GetGUID(), GetActionName(), GetName());
}

void MidiWidget::ForceUpdate()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->ForceUpdateAction(GetGUID(), GetActionName(), GetName());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RealSurfaceChannel::SetGUID(string GUID)
{
    GUID_ = GUID;
    
    for (auto widgetName : widgetNames_)
        realSurface_->SetWidgetGUID(widgetName, GUID);
    
    realSurface_->GetZone()->GetLayout()->MapTrack(GUID_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
RealSurface::RealSurface(const string name, string templateFilename, int numChannels, bool isBankable) : name_(name),  templateFilename_(templateFilename), isBankable_(isBankable)
{
    for(int i = 0; i < numChannels; i++)
        channels_.push_back(new RealSurfaceChannel(to_string(i + 1), this));
}

void RealSurface::MapTrackToWidgets(MediaTrack *track)
{
    string trackGUID = GetZone()->GetTrackGUIDAsString(track);
    
    for(auto* channel : channels_)
        channel->SetGUID(trackGUID);
}

void  RealSurface::UnmapWidgetsFromTrack(MediaTrack *track)
{
    for(auto* channel : channels_)
        channel->SetGUID("");
}

void RealSurface::AddAction(string actionAddress, Action* action)
{
    GetZone()->GetLayout()->AddAction(actionAddress, action);
}

// to Actions ->
double RealSurface::GetActionCurrentNormalizedValue(string GUID, string actionName, string widgetName)
{
    return GetZone()->GetActionCurrentNormalizedValue(GUID, GetName(), actionName, widgetName);
}

void RealSurface::UpdateAction(string GUID, string actionName, string widgetName)
{
    GetZone()->UpdateAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::ForceUpdateAction(string GUID, string actionName, string widgetName)
{
    GetZone()->ForceUpdateAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::CycleAction(string GUID, string actionName, string widgetName)
{
    GetZone()->CycleAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::DoAction(string GUID, string actionName, string widgetName, double value)
{
    GetZone()->DoAction(value, GUID, GetName(), actionName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Zone::OnTrackSelection(MediaTrack* track)
{
    for(auto* surface : realSurfaces_)
        DoAction(1.0, GetTrackGUIDAsString(track), surface->GetName(), TrackOnSelection, TrackOnSelection);
}

void Zone::TrackListChanged()
{
    vector<RealSurfaceChannel*> channels;
    
    for(auto* surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            channels.push_back(channel);
    
    int currentOffset = trackOffset_;
    bool shouldRefreshLayout = false;
    
    for(int i = 0; i < channels.size() && currentOffset < GetNumTracks(); i++)
    {
        if(channels[i]->GetIsMovable() == false)
        {
            if(GetTrackFromGUID(channels[i]->GetGUID()) == nullptr) // track has been removed
            {
                channels[i]->SetIsMovable(true); // unlock this, since there is no longer a track to lock to
                shouldRefreshLayout = true;
                break;
            }
            else
            {
                currentOffset++; // track exists, move on
            }
        }
        
        else if(channels[i]->GetGUID() == GetNextVisibleTrackGUID(currentOffset))
        {
            currentOffset++; // track exists and positions are in synch
        }
        else
        {
            shouldRefreshLayout = true;
            break;
        }
    }

    if(shouldRefreshLayout)
        RefreshLayout();
}

void Zone::AdjustTrackBank(int stride)
{
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ < 1 - numBankableChannels_ + GetNumLockedTracks())
        trackOffset_ = 1 - numBankableChannels_ + GetNumLockedTracks();
    
    if(trackOffset_ >  GetNumTracks() - 1)
        trackOffset_ = GetNumTracks() - 1;
    
    // Jump over any pinned channels and invisible tracks
    vector<string> pinnedChannels;
    for(auto surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            if(channel->GetIsMovable() == false)
                pinnedChannels.push_back(channel->GetGUID());
    
    bool skipThisChannel = false;
    
    while(trackOffset_ >= 0 && trackOffset_ < GetNumTracks())
    {
        string trackGUID = GetTrackGUIDAsString(trackOffset_);
        
        for(auto pinnedChannel : pinnedChannels)
            if(pinnedChannel == trackGUID)
            {
                skipThisChannel = true;
                previousTrackOffset < trackOffset_ ? trackOffset_++ : trackOffset_--;
                break;
            }
        
        if( ! IsTrackVisible(CSurf_TrackFromID(trackOffset_)))
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

    RefreshLayout();
}

void Zone::RefreshLayout()
{
    vector<string> pinnedChannelLayout;
    vector<string> pinnedChannels;
    vector<string> movableChannelLayout;
    vector<string> channelLayout;
    
    // Layout locked channel GUIDs
    for(auto surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            if(channel->GetIsMovable() == false)
            {
                pinnedChannelLayout.push_back(channel->GetGUID());
                pinnedChannels.push_back(channel->GetGUID());
            }
            else
                pinnedChannelLayout.push_back("");
    
    // Layout channel GUIDs
    int offset = trackOffset_;
    
    for(int i = 0; i < pinnedChannelLayout.size(); i++)
    {
        if(offset < 0)
        {
            movableChannelLayout.push_back("");
            offset++;
        }
        else if(offset >= GetNumTracks())
        {
            movableChannelLayout.push_back("");
        }
        else
        {
            movableChannelLayout.push_back(GetNextVisibleTrackGUID(offset));
            offset++;
        }
    }
    
    // Remove the locked GUIDs
    for(int i = 0; i < pinnedChannels.size(); i++)
    {
        auto iter = find(movableChannelLayout.begin(), movableChannelLayout.end(), pinnedChannels[i]);
        if(iter != movableChannelLayout.end())
        {
            movableChannelLayout.erase(iter);
        }
    }
    
    // Merge the layouts
    offset = 0;
    for(int i = 0; i < pinnedChannelLayout.size(); i++)
    {
        if(pinnedChannelLayout[i] != "")
            channelLayout.push_back(pinnedChannelLayout[i]);
        else
            channelLayout.push_back(movableChannelLayout[offset++]);
    }
    
    // Apply new layout
    offset = 0;
    for(auto* surface : realSurfaces_)
        for(auto* channel : surface->GetBankableChannels())
            channel->SetGUID(channelLayout[offset++]);
    
    for(auto* surface : realSurfaces_)
        surface->ForceUpdateWidgets();
}

void Zone::MapFXToWidgets(MediaTrack *track, RealSurface* surface)
{
    char fxName[BUFSZ];
    char fxGUID[BUFSZ];
    char fxParamName[BUFSZ];
    
    DeleteFXWindows();
    surface->UnmapFXFromWidgets(track);
    
    string trackGUID = GetTrackGUIDAsString(track);
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
        
        if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
        {
            FXTemplate* map = fxTemplates_[surface->GetName()][fxName];
            
            for(auto mapEntry : map->GetTemplateEntries())
            {
                if(mapEntry.paramName == GainReductionDB)
                    surface->SetWidgetFXGUID(mapEntry.widgetName, trackGUID + fxGUID);
                else
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); DAW::TrackFX_GetParamName(track, i, j++, fxParamName, sizeof(fxParamName)))
                        if(mapEntry.paramName == fxParamName)
                            surface->SetWidgetFXGUID(mapEntry.widgetName, trackGUID + fxGUID);
            }
            
            AddFXWindow(FXWindow(track, fxGUID));
        }
    }
    
    OpenFXWindows();
    
    surface->ForceUpdateWidgets();
}

void Zone::MapFXActions(string trackGUID, RealSurface* surface)
{
    MediaTrack* track = GetTrackFromGUID(trackGUID);
    if(track == nullptr)
        return;
    
    Layout* layout = GetLayout();
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    char fxGUID[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
        {
            FXTemplate* map = fxTemplates_[surface->GetName()][fxName];
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            string actionBaseAddress = trackGUID + fxGUID + GetName() + surface->GetName();
            
            for(auto mapEntry : map->GetTemplateEntries())
            {
                if(mapEntry.paramName == GainReductionDB)
                    GetLayout()->AddAction(actionBaseAddress + mapEntry.widgetName, new TrackGainReductionMeter_Action(layout, trackGUID, fxGUID));
                else
                {
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                    {
                        DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                        
                        if(mapEntry.paramName == fxParamName)
                            GetLayout()->AddAction(actionBaseAddress + mapEntry.widgetName, new TrackFX_Action(layout, trackGUID, fxGUID, j));
                    }
                }
            }
        }
    }
}

void Zone::TrackFXListChanged(MediaTrack* track)
{
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];

    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(GetLayout()->GetManager()->GetVSTMonitor())
        {
            DAW::ShowConsoleMsg(("\n\n" + string(fxName) + "\n").c_str());
            
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                DAW::ShowConsoleMsg((string(fxParamName) + "\n").c_str());
            }
        }
    }
    
    for(auto* surface : realSurfaces_)
        MapFXActions(GetTrackGUIDAsString(track), surface);
}

void Zone::AddAction(string actionAddress, Action* action)
{
    GetLayout()->AddAction(actionAddress, action);
}

void Zone::MapRealSurfaceActions(RealSurface* surface)
{
    Layout* layout = GetLayout();
    string actionBaseAddress = DefaultGUID + GetName() + surface->GetName();;
    string templateDirectory = actionTemplateDirectory_[surface->GetName()];
    
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/" + filename));
           
            for (string line; getline(actionTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens.size() == 2)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layout));
                    else if(tokens.size() == 3)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layout, tokens[2]));
                }
            }
        }
    }
}

void Zone::MapTrackActions(string trackGUID, RealSurface* surface)
{
    MediaTrack* track = GetTrackFromGUID(trackGUID);
    if(track == nullptr)
        return;

    Layout* layout = GetLayout();
    string actionBaseAddress = trackGUID + GetName() + surface->GetName();
    string templateDirectory = actionTemplateDirectory_[surface->GetName()];
    
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/Track/" + filename));
            
            for (string line; getline(actionTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens.size() == 2)
                    {
                        AddAction(actionBaseAddress + tokens[0], TrackActionFor(tokens[1], layout, trackGUID));
                    }
                    else if(tokens.size() == 3)
                    {
                        if(tokens[1] == "TrackTouchControlled")
                        {
                            string actionAddress = actionBaseAddress + tokens[0];
                            Action* controlledAction = TrackActionFor(tokens[2], layout, trackGUID);
                            AddAction(actionAddress, TrackActionFor(tokens[1], actionAddress, layout, trackGUID, controlledAction));
                        }
                        else
                        {
                            AddAction(actionBaseAddress + tokens[0], TrackActionFor(tokens[1], layout, trackGUID, tokens[2]));
                        }
                    }
                    else if(tokens[1] == "Cycled" && tokens.size() > 4)
                    {
                        Action* cycledAction = ActionFor(tokens[1], layout);
                        AddAction(actionBaseAddress + tokens[0], cycledAction);
                        AddAction(actionBaseAddress + tokens[2], cycledAction);
                        for(int i = 3 ; i < tokens.size(); i++)
                            cycledAction->AddAction(TrackActionFor(tokens[i], layout, trackGUID));
                    }
                }
            }
        }
    }
}

void Zone::SetPinnedTracks()
{
    char buffer[BUFSZ];
    RealSurfaceChannel* channel = nullptr;
    
    for(auto* surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetBankableChannels().size(); i++)
        {
            channel = surface->GetBankableChannels()[i];
            
            if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayout()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), buffer, sizeof(buffer)))
            {
                channel->SetGUID(buffer);
                channel->SetIsMovable(false);
            }
        }
    }
}

void Zone::PinSelectedTracks()
{
    RealSurfaceChannel* channel = nullptr;
    
    for(auto* surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetBankableChannels().size(); i++)
        {
            channel = surface->GetBankableChannels()[i];
            MediaTrack* track = GetTrackFromGUID(channel->GetGUID());
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsMovable(false);
                DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayout()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), channel->GetGUID().c_str());
                DAW::MarkProjectDirty(nullptr);
            }
        }
    }
}

void Zone::UnpinSelectedTracks()
{
    char buffer[BUFSZ];
    RealSurfaceChannel* channel = nullptr;
    
    for(auto* surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetBankableChannels().size(); i++)
        {
            channel = surface->GetBankableChannels()[i];
            MediaTrack* track = GetTrackFromGUID(channel->GetGUID());
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
            {
                channel->SetIsMovable(true);
                if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayout()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), buffer, sizeof(buffer)))
                {
                    DAW::SetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (GetLayout()->GetName() + GetName() + surface->GetName() + channel->GetSuffix()).c_str(), "");
                    DAW::MarkProjectDirty(nullptr);
                }
            }
        }
    }
}

// to Actions ->
double Zone::GetActionCurrentNormalizedValue(string GUID, string surfaceName, string actionName, string widgetName)
{
    return GetLayout()->GetActionCurrentNormalizedValue(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::UpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->UpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::ForceUpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->ForceUpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::CycleAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->CycleAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::DoAction(double value, string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->DoAction(ActionAddressFor(GUID, surfaceName, actionName), value, GetName(), surfaceName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Layout
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Layout::OnTrackSelection(MediaTrack* track)
{
    MapTrack(DAW::GetTrackGUIDAsString(track));
    
    for(auto const& [name, zone] : zones_)
        zone->OnTrackSelection(track);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfManager::InitRealSurface(RealSurface* surface)
{
    ifstream surfaceTemplateFile(surface->GetTemplateFilename());
    bool inChannel = false;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        if(line[0] != '/' && line != "") // ignore comment lines and blank lines
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
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], i));
            }
            else if(tokens.size() == 6)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4]), strToHex(tokens[5])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5])));
            }
            else if(tokens.size() == 8)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]) + i, strToHex(tokens[6]), strToHex(tokens[7])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7])));
            }
            else if(tokens.size() == 9)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]) + i, strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
            }
        }
    }
}


