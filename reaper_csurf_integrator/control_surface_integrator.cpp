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
Action* ActionFor(string name, LogicalSurface* logicalSurface)
{
    if(name == "SetShowFXWindows")  return new SetShowFXWindows_Action(logicalSurface);
    else if(name == "Rewind")  return new Rewind_Action(logicalSurface);
    else if(name == "FastForward")  return new FastForward_Action(logicalSurface);
    else if(name == "Stop")  return new Stop_Action(logicalSurface);
    else if(name == "Play")  return new Play_Action(logicalSurface);
    else if(name == "Record")  return new Record_Action(logicalSurface);
    else if(name == "NextMap")  return new NextMap_Action(logicalSurface);
    else if(name == "LockTracks")  return new ImmobilizeSelectedTracks_Action(logicalSurface);
    else if(name == "UnlockTracks")  return new MobilizeSelectedTracks_Action(logicalSurface);
    else if(name == "Shift")  return new Shift_Action(logicalSurface);
    else if(name == "Option")  return new Option_Action(logicalSurface);
    else if(name == "Control")  return new Control_Action(logicalSurface);
    else if(name == "Alt")  return new Alt_Action(logicalSurface);
    else if(name == "LatchedZoom")  return new LatchedZoom_Action(logicalSurface);
    else if(name == "LatchedScrub")  return new LatchedScrub_Action(logicalSurface);
    else if(name == "Cycled")  return new Cycled_Action(logicalSurface);
    else if(name == "CycleTimeline")  return new CycleTimeline_Action(logicalSurface);
    
    return new Action(logicalSurface);
}

Action* ActionFor(string name, LogicalSurface* logicalSurface, string param)
{
    if(name == "TrackBank")  return new TrackBank_Action(logicalSurface, param);
    else if(name == "TrackAutoMode")  return new TrackAutoMode_Action(logicalSurface, param);
    else if(name == "GlobalAutoMode")  return new GlobalAutoMode_Action(logicalSurface, param);
    else if(name == "Reaper")  return new Reaper_Action(logicalSurface, param);
    
    return new Action(logicalSurface);
}

Action* ActionFor(string name, LogicalSurface* logicalSurface, MediaTrack* track)
{
    if(name == "TrackVolume")  return new TrackVolume_Action(logicalSurface, track);
    else if(name == "TrackVolumeDisplay")  return new TrackVolumeDisplay_Action(logicalSurface, track);
    else if(name == "TrackTouch")  return new TrackTouch_Action(logicalSurface, track);
    else if(name == "TrackMute")  return new TrackMute_Action(logicalSurface, track);
    else if(name == "TrackSolo")  return new TrackSolo_Action(logicalSurface, track);
    else if(name == "TrackUniqueSelect")  return new TrackUniqueSelect_Action(logicalSurface, track);
    else if(name == "TrackRangeSelect")  return new TrackRangeSelect_Action(logicalSurface, track);
    else if(name == "TrackSelect")  return new TrackSelect_Action(logicalSurface, track);
    else if(name == "TrackRecordArm")  return new TrackRecordArm_Action(logicalSurface, track);
    else if(name == "TrackNameDisplay")  return new TrackNameDisplay_Action(logicalSurface, track);
    else if(name == "MapTrackAndFXToWidgets")  return new MapTrackAndFXToWidgets_Action(logicalSurface, track);
    
    return new Action(logicalSurface);
}

Action* ActionFor(string name, LogicalSurface* logicalSurface, MediaTrack* track, string param)
{
    if(name == "TrackPan")  return new TrackPan_Action(logicalSurface, track, param);
    else if(name == "TrackPanWidth")  return new TrackPanWidth_Action(logicalSurface, track, param);
    else if(name == "TrackOutputMeter")  return new TrackOutputMeter_Action(logicalSurface, track, param);
    
    return new Action(logicalSurface);
}

Action* ActionFor(string name, string actionAddress, LogicalSurface* logicalSurface, MediaTrack* track, Action* baseAction)
{
    if(name == "TrackTouchControlled")  return new TrackTouchControlled_Action(actionAddress, logicalSurface, track, baseAction);
    
    return new Action(logicalSurface);
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
    if(widgetClass == "Display") return new Display_MidiWidget(surface, name, index);
    
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
    
    realSurface_->GetSurfaceGroup()->GetLogicalSurface()->MapTrack(GUID_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
RealSurface::RealSurface(const string name, string templateFilename, int numChannels, int numBankableChannels) : name_(name),  templateFilename_(templateFilename), numChannels_(numChannels), numBankableChannels_(numBankableChannels)
{
    for(int i = 0; i < numChannels_; i++)
        channels_.push_back(new RealSurfaceChannel(to_string(i + 1), this));
}

void RealSurface::MapTrackToWidgets(MediaTrack *track)
{
    string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
    
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
    GetSurfaceGroup()->GetLogicalSurface()->AddAction(actionAddress, action);
}

// to Actions ->
double RealSurface::GetActionCurrentNormalizedValue(string GUID, string actionName, string widgetName)
{
    return GetSurfaceGroup()->GetActionCurrentNormalizedValue(GUID, GetName(), actionName, widgetName);
}

void RealSurface::UpdateAction(string GUID, string actionName, string widgetName)
{
    GetSurfaceGroup()->UpdateAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::ForceUpdateAction(string GUID, string actionName, string widgetName)
{
    GetSurfaceGroup()->ForceUpdateAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::CycleAction(string GUID, string actionName, string widgetName)
{
    GetSurfaceGroup()->CycleAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::DoAction(string GUID, string actionName, string widgetName, double value)
{
    GetSurfaceGroup()->DoAction(value, GUID, GetName(), actionName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SurfaceGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////
void SurfaceGroup::MapFXActions(string trackGUID, RealSurface* surface)
{
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
    LogicalSurface* logicalSurface = GetLogicalSurface();
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
                    GetLogicalSurface()->AddAction(actionBaseAddress + mapEntry.widgetName, new TrackGainReductionMeter_Action(logicalSurface, track, fxGUID));
                else
                {
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                    {
                        DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                        
                        if(mapEntry.paramName == fxParamName)
                            GetLogicalSurface()->AddAction(actionBaseAddress + mapEntry.widgetName, new TrackFX_Action(logicalSurface, track, fxGUID, j));
                    }
                }
            }
        }
        
        if(GetLogicalSurface()->GetManager()->GetVSTMonitor() && GetLogicalSurface()->GetManager()->GetIsInitialized())
        {
            DAW::ShowConsoleMsg(("\n\n" + string(fxName) + "\n").c_str());
            
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                DAW::ShowConsoleMsg((string(fxParamName) + "\n").c_str());
            }
        }
    }
}

void SurfaceGroup::AddAction(string actionAddress, Action* action)
{
    GetLogicalSurface()->AddAction(actionAddress, action);
}

void SurfaceGroup::MapRealSurfaceActions(RealSurface* surface)
{
    LogicalSurface* logicalSurface = GetLogicalSurface();
    string actionBaseAddress = RealControlSurface + GetName() + surface->GetName();;
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
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], logicalSurface));
                    else if(tokens.size() == 3)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], logicalSurface, tokens[2]));
                }
            }
        }
    }
}

void SurfaceGroup::MapTrackActions(string trackGUID, RealSurface* surface)
{
    LogicalSurface* logicalSurface = GetLogicalSurface();
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
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
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], logicalSurface, track));
                    }
                    else if(tokens.size() == 3)
                    {
                        if(tokens[0] == "TrackTouchControlled")
                        {
                            string actionAddress = actionBaseAddress + tokens[2];
                            Action* controlledAction = ActionFor(tokens[1], logicalSurface, track);
                            AddAction(actionAddress, ActionFor(tokens[0], actionAddress, logicalSurface, track, controlledAction));
                        }
                        else
                        {
                            AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], logicalSurface, track, tokens[2]));
                        }
                    }
                    else if(tokens[0] == "Cycled" && tokens.size() == 7)
                    {
                        Action* cycledAction = ActionFor(tokens[0], logicalSurface);
                        cycledAction->AddAction(ActionFor(tokens[3], logicalSurface, track, tokens[4]));
                        cycledAction->AddAction(ActionFor(tokens[5], logicalSurface, track, tokens[6]));
                        AddAction(actionBaseAddress + tokens[1], cycledAction);
                        AddAction(actionBaseAddress + tokens[2], cycledAction);
                    }
                }
            }
        }
    }
}

// to Actions ->
double SurfaceGroup::GetActionCurrentNormalizedValue(string GUID, string surfaceName, string actionName, string widgetName)
{
    return GetLogicalSurface()->GetActionCurrentNormalizedValue(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void SurfaceGroup::UpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLogicalSurface()->UpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void SurfaceGroup::ForceUpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLogicalSurface()->ForceUpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void SurfaceGroup::CycleAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLogicalSurface()->CycleAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void SurfaceGroup::DoAction(double value, string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLogicalSurface()->DoAction(ActionAddressFor(GUID, surfaceName, actionName), value, GetName(), surfaceName, widgetName);
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
                if(tokens[0] == "Channel")
                    inChannel = true;
                else if(tokens[0] == "ChannelEnd")
                    inChannel = false;
            }
            else if(tokens.size() == 2)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetNumChannels(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], i));
            }
            else if(tokens.size() == 6)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetNumChannels(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4]), strToHex(tokens[5])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5])));
            }
            else if(tokens.size() == 8)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetNumChannels(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]) + i, strToHex(tokens[6]), strToHex(tokens[7])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7])));
            }
             else if(tokens.size() == 9)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetNumChannels(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]) + i, strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
            }
        }
    }
}


