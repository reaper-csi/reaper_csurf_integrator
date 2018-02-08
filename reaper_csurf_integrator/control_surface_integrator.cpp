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
// Reaper Actions available for mapping, this list will get added to over time
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
// MidiWidgeta available for inclusion in Real Surface Templates
////////////////////////////////////////////////////////////////////////////////////////////////////////
int strToHex(string valueStr)
{
    return strtol(valueStr.c_str(), nullptr, 16);
}

double strToDouble(string valueStr)
{
    return strtod(valueStr.c_str(), nullptr);
}

MidiWidget* WidgetFor(RealSurface* surface, string widgetClass, string name, int index)
{
    if(widgetClass == "Display") return new Display_MidiWidget(surface, name, index);
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00));
}

MidiWidget* WidgetFor(RealSurface* surface, string widgetClass, string name, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "Button") return new PushButton_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithLatch") return new PushButtonWithLatch_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithRelease") return new PushButtonWithRelease_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonCycler") return new PushButtonCycler_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Encoder") return new Encoder_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Fader7Bit") return new Fader7Bit_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Min), new MIDI_event_ex_t(byte1, byte2, byte3Max));
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

MidiWidget* WidgetFor(RealSurface* surface, string widgetClass, string name, double minDB, double maxDB, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "VUMeter") return new VUMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "GainReductionMeter") return new GainReductionMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
  
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

MidiWidget* WidgetFor(RealSurface* surface, string widgetClass, string name, double minDB, double maxDB, int byte1, int byte2Min, int byte2Max, int byte3Min, int byte3Max)
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
                    {
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], logicalSurface));
                    }
                    if(tokens.size() == 3)
                    {
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], logicalSurface, tokens[2]));
                    }
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
                    if(tokens.size() == 3)
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
    RealSurfaceChannel* channel = nullptr;

    string templateFilename = surface->GetTemplateFilename();

    
    
    
    
    
    
    if(surface->GetName() == "Console1")
    {
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "DisplayFX", strToHex("b0"), strToHex("66"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "Order", strToHex("b0"), strToHex("0e"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "ExternalSidechain", strToHex("b0"), strToHex("11"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "FiltersToCompressor", strToHex("b0"), strToHex("3d"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "PhaseInvert", strToHex("b0"), strToHex("6c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "Preset", strToHex("b0"), strToHex("3a"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "Shape", strToHex("b0"), strToHex("35"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "HardGate", strToHex("b0"), strToHex("3b"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "Equalizer", strToHex("b0"), strToHex("50"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "LoCurve", strToHex("b0"), strToHex("5d"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "HiCurve", strToHex("b0"), strToHex("41"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "HiGain", strToHex("b0"), strToHex("52"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "HiFrequency", strToHex("b0"), strToHex("53"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "HiMidGain", strToHex("b0"), strToHex("55"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "HiMidFrequency", strToHex("b0"), strToHex("56"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "HiMidQ", strToHex("b0"), strToHex("57"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "LoMidGain", strToHex("b0"), strToHex("58"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "LoMidFrequency", strToHex("b0"), strToHex("59"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "LoMidQ", strToHex("b0"), strToHex("5a"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "LoGain", strToHex("b0"), strToHex("5b"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "LoFrequency", strToHex("b0"), strToHex("5c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithLatch", "Compressor", strToHex("b0"), strToHex("5c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Threshold", strToHex("b0"), strToHex("2f"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Release", strToHex("b0"), strToHex("30"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Ratio", strToHex("b0"), strToHex("31"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Parallel", strToHex("b0"), strToHex("32"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Attack", strToHex("b0"), strToHex("33"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Drive", strToHex("b0"), strToHex("0f"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Fader7Bit", "Character", strToHex("b0"), strToHex("12"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "GainReductionMeter", "CompressorMeter", strToDouble("0.0"), strToDouble("-20.0"),  strToHex("b0"), strToHex("73"), strToHex("00"), strToHex("7f")));

        
        // Channel
        channel = new RealSurfaceChannel("", surface);
        surface->AddChannel(channel);
        
        channel->AddWidget(WidgetFor(surface, "VUMeter", "ChannelInputMeterLeft", strToDouble("-60.0"), strToDouble("6.0"), strToHex("b0"), strToHex("6e"), strToHex("00"), strToHex("7f")));
        channel->AddWidget(WidgetFor(surface, "VUMeter", "ChannelInputMeterRight", strToDouble("-60.0"), strToDouble("6.0"), strToHex("b0"), strToHex("6f"), strToHex("00"), strToHex("7f")));

        channel->AddWidget(WidgetFor(surface, "ButtonWithLatch","ChannelMute", strToHex("b0"), strToHex("0c"), strToHex("00"), strToHex("7f")));
        channel->AddWidget(WidgetFor(surface, "ButtonWithLatch","ChannelSolo", strToHex("b0"), strToHex("0d"), strToHex("00"), strToHex("7f")));
       
        channel->AddWidget(WidgetFor(surface, "Fader7Bit", "ChannelFader", strToHex("b0"), strToHex("07,"), strToHex("00"), strToHex("7f")));
        channel->AddWidget(WidgetFor(surface, "Fader7Bit", "ChannelRotary", strToHex("b0"), strToHex("0a,"), strToHex("00"), strToHex("7f")));

        channel->AddWidget(WidgetFor(surface, "VUMeter", "ChannelOutputMeterLeft", strToDouble("-60.0"), strToDouble("6.0"), strToHex("b0"), strToHex("70"), strToHex("00"), strToHex("7f")));
        channel->AddWidget(WidgetFor(surface, "VUMeter", "ChannelOutputMeterRight", strToDouble("-60.0"), strToDouble("6.0"), strToHex("b0"), strToHex("71"), strToHex("00"), strToHex("7f")));
    }
    else
    {
        surface->AddWidget(WidgetFor(surface, "Button", "Track", strToHex("90"), strToHex("28"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Send", strToHex("90"), strToHex("29"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Pan", strToHex("90"), strToHex("2a"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Plugin", strToHex("90"), strToHex("2b"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "EQ", strToHex("90"), strToHex("2c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Instrument", strToHex("90"), strToHex("2d"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "nameValue", strToHex("90"), strToHex("34"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "smpteBeats", strToHex("90"), strToHex("35"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F1", strToHex("90"), strToHex("36"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F2", strToHex("90"), strToHex("37"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F3", strToHex("90"), strToHex("38"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F4", strToHex("90"), strToHex("39"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F5", strToHex("90"), strToHex("3a"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F6", strToHex("90"), strToHex("3b"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F7", strToHex("90"), strToHex("3c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "F8", strToHex("90"), strToHex("3d"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithRelease", "Shift", strToHex("90"), strToHex("46"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithRelease", "Option", strToHex("90"), strToHex("47"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithRelease", "Control", strToHex("90"), strToHex("48"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "ButtonWithRelease", "Alt", strToHex("90"), strToHex("49"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Read", strToHex("90"), strToHex("4a"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Write", strToHex("90"), strToHex("4b"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Trim", strToHex("90"), strToHex("4c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Touch", strToHex("90"), strToHex("4d"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Latch", strToHex("90"), strToHex("4e"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Group", strToHex("90"), strToHex("4f"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Save", strToHex("90"), strToHex("50"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Undo", strToHex("90"), strToHex("51"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Cancel", strToHex("90"), strToHex("52"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Enter", strToHex("90"), strToHex("53"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Cycle", strToHex("90"), strToHex("56"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Drop", strToHex("90"), strToHex("57"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Replace", strToHex("90"), strToHex("58"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Click", strToHex("90"), strToHex("59"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Solo", strToHex("90"), strToHex("5a"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Up", strToHex("90"), strToHex("60"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Down", strToHex("90"), strToHex("61"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Left", strToHex("90"), strToHex("62"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Right", strToHex("90"), strToHex("63"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Zoom", strToHex("90"), strToHex("64"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Scrub", strToHex("90"), strToHex("65"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "BankLeft", strToHex("90"), strToHex("2e"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "BankRight", strToHex("90"), strToHex("2f"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "NudgeLeft", strToHex("90"), strToHex("30"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "NudgeRight", strToHex("90"), strToHex("31"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Marker", strToHex("90"), strToHex("54"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Nudge", strToHex("90"), strToHex("55"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Rewind", strToHex("90"), strToHex("5b"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "FastForward", strToHex("90"), strToHex("5c"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Stop", strToHex("90"), strToHex("5d"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Play", strToHex("90"), strToHex("5e"), strToHex("00"), strToHex("7f")));
        surface->AddWidget(WidgetFor(surface, "Button", "Record", strToHex("90"), strToHex("5f"), strToHex("00"), strToHex("7f")));
        
        for(int i = 0; i < surface->GetNumBankableChannels(); ++i)
        {
            channel = new RealSurfaceChannel(to_string(i + 1), surface);
            surface->AddChannel(channel);
            
            channel->AddWidget(WidgetFor(surface, "ButtonWithRelease", "ChannelFaderTouch", strToHex("90"), strToHex("68") + i, strToHex("00"), strToHex("7f")));

            channel->AddWidget(WidgetFor(surface, "ButtonCycler", "ChannelRotaryPush", strToHex("90"), strToHex("20") + i, strToHex("00"), strToHex("7f")));
            channel->AddWidget(WidgetFor(surface, "Encoder", "ChannelRotary", strToHex("b0"), strToHex("10") + i, strToHex("00"), strToHex("7f")));

            channel->AddWidget(WidgetFor(surface, "Display", "ChannelDisplay", i));
            
            channel->AddWidget(WidgetFor(surface, "Fader14Bit", "ChannelFader", strToDouble("-72.0"), strToDouble("12.0"), strToHex("e0") + i, strToHex("00"), strToHex("7f"), strToHex("00"), strToHex("7f")));
           
            channel->AddWidget(WidgetFor(surface, "Button", "ChannelRecordArm", strToHex("90"), strToHex("00") + i, strToHex("00"), strToHex("7f")));
            channel->AddWidget(WidgetFor(surface, "Button", "ChannelSolo", strToHex("90"), strToHex("08") + i, strToHex("00"), strToHex("7f")));
            channel->AddWidget(WidgetFor(surface, "Button", "ChannelMute", strToHex("90"), strToHex("10") + i, strToHex("00"), strToHex("7f")));
            channel->AddWidget(WidgetFor(surface, "Button", "ChannelSelect", strToHex("90"), strToHex("18") + i, strToHex("00"), strToHex("7f")));
        }
    }
}


/*
 surface->AddWidget(new PushButton_CSurfWidget("PagePlus", surface,   new MIDI_event_ex_t(0xb0, 0x60, 0x7f), new MIDI_event_ex_t(0xb0, 0x60, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("PageMinus", surface,  new MIDI_event_ex_t(0xb0, 0x61, 0x7f), new MIDI_event_ex_t(0xb0, 0x61, 0x00)));
 
 surface->AddWidget(new PushButton_CSurfWidget("Select1", surface,    new MIDI_event_ex_t(0xb0, 0x15, 0x7f), new MIDI_event_ex_t(0xb0, 0x15, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select2", surface,    new MIDI_event_ex_t(0xb0, 0x16, 0x7f), new MIDI_event_ex_t(0xb0, 0x16, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select3", surface,    new MIDI_event_ex_t(0xb0, 0x17, 0x7f), new MIDI_event_ex_t(0xb0, 0x17, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select4", surface,    new MIDI_event_ex_t(0xb0, 0x18, 0x7f), new MIDI_event_ex_t(0xb0, 0x18, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select5", surface,    new MIDI_event_ex_t(0xb0, 0x19, 0x7f), new MIDI_event_ex_t(0xb0, 0x19, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select6", surface,    new MIDI_event_ex_t(0xb0, 0x1a, 0x7f), new MIDI_event_ex_t(0xb0, 0x1a, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select7", surface,    new MIDI_event_ex_t(0xb0, 0x1b, 0x7f), new MIDI_event_ex_t(0xb0, 0x1b, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select8", surface,    new MIDI_event_ex_t(0xb0, 0x1c, 0x7f), new MIDI_event_ex_t(0xb0, 0x1c, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select9", surface,    new MIDI_event_ex_t(0xb0, 0x1d, 0x7f), new MIDI_event_ex_t(0xb0, 0x1d, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select10", surface,   new MIDI_event_ex_t(0xb0, 0x1e, 0x7f), new MIDI_event_ex_t(0xb0, 0x1e, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select11", surface,   new MIDI_event_ex_t(0xb0, 0x1f, 0x7f), new MIDI_event_ex_t(0xb0, 0x1f, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select12", surface,   new MIDI_event_ex_t(0xb0, 0x20, 0x7f), new MIDI_event_ex_t(0xb0, 0x20, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select13", surface,   new MIDI_event_ex_t(0xb0, 0x21, 0x7f), new MIDI_event_ex_t(0xb0, 0x21, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select14", surface,   new MIDI_event_ex_t(0xb0, 0x22, 0x7f), new MIDI_event_ex_t(0xb0, 0x22, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select15", surface,   new MIDI_event_ex_t(0xb0, 0x23, 0x7f), new MIDI_event_ex_t(0xb0, 0x23, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select16", surface,   new MIDI_event_ex_t(0xb0, 0x24, 0x7f), new MIDI_event_ex_t(0xb0, 0x24, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select17", surface,   new MIDI_event_ex_t(0xb0, 0x25, 0x7f), new MIDI_event_ex_t(0xb0, 0x25, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select18", surface,   new MIDI_event_ex_t(0xb0, 0x26, 0x7f), new MIDI_event_ex_t(0xb0, 0x26, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select19", surface,   new MIDI_event_ex_t(0xb0, 0x27, 0x7f), new MIDI_event_ex_t(0xb0, 0x27, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("Select20", surface,   new MIDI_event_ex_t(0xb0, 0x28, 0x7f), new MIDI_event_ex_t(0xb0, 0x28, 0x00)));
 
 surface->AddWidget(new PushButton_CSurfWidget("TrackGroup", surface, new MIDI_event_ex_t(0xb0, 0x7b, 0x7f), new MIDI_event_ex_t(0xb0, 0x7b, 0x00)));
 surface->AddWidget(new PushButton_CSurfWidget("TrackCopy", surface,  new MIDI_event_ex_t(0xb0, 0x78, 0x7f), new MIDI_event_ex_t(0xb0, 0x78, 0x00)));
 
 surface->AddWidget(new PushButton_CSurfWidget(SendsMode, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x68, 0x7f), new MIDI_event_ex_t(0xb0, 0x68, 0x00)));
 
 surface->AddWidget(new Fader8Bit_CSurfWidget("HiCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x69, 0x7f)));
 surface->AddWidget(new Fader8Bit_CSurfWidget("LoCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x67, 0x7f)));

 surface->AddWidget(new Fader8Bit_CSurfWidget("Gate", surface, channel,           new MIDI_event_ex_t(0xb0, 0x36, 0x7f)));
 surface->AddWidget(new Fader8Bit_CSurfWidget("GateRelease", surface, channel,    new MIDI_event_ex_t(0xb0, 0x38, 0x7f)));
 surface->AddWidget(new Fader8Bit_CSurfWidget("Sustain", surface, channel,        new MIDI_event_ex_t(0xb0, 0x37, 0x7f)));
 surface->AddWidget(new Fader8Bit_CSurfWidget("Punch", surface, channel,          new MIDI_event_ex_t(0xb0, 0x39, 0x7f)l));
 
 surface->AddWidget(new VUMeter_CSurfWidget(GateMeter, surface, channel, new  MIDI_event_ex_t(0xb0, 0x72, 0x7f)));

 channel->AddWidget(new Fader8Bit_CSurfWidget("InputGain", surface, channel,  new MIDI_event_ex_t(0xb0, 0x6b, 0x7f)));

 */


