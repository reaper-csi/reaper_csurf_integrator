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

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfManager::InitRealSurface(RealSurface* surface)
{
    
    // GAW TBD -- this will be in .rst files
    
    string templateFilename = surface->GetTemplateFilename();
    
    
    
    RealSurfaceChannel* channel = nullptr;
    
    if(surface->GetName() == "Console1")
    {
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "DisplayFX", new MIDI_event_ex_t(0xb0, 0x66, 0x7f), new MIDI_event_ex_t(0xb0, 0x66, 0x00)));

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
*/
        
        
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "Order",             new MIDI_event_ex_t(0xb0, 0x0e, 0x7f), new MIDI_event_ex_t(0xb0, 0x0e, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "ExternalSidechain", new MIDI_event_ex_t(0xb0, 0x11, 0x7f), new MIDI_event_ex_t(0xb0, 0x11, 0x00)));

        // Input
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "FiltersToCompressor",   new MIDI_event_ex_t(0xb0, 0x3d, 0x7f), new MIDI_event_ex_t(0xb0, 0x3d, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "PhaseInvert",           new MIDI_event_ex_t(0xb0, 0x6c, 0x7f), new MIDI_event_ex_t(0xb0, 0x6c, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "Preset",                new MIDI_event_ex_t(0xb0, 0x3a, 0x7f), new MIDI_event_ex_t(0xb0, 0x3a, 0x00)));

        //surface->AddWidget(new Fader8Bit_CSurfWidget("HiCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x69, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("LoCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x67, 0x7f)));
    
        
        // Shape
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "Shape",     new MIDI_event_ex_t(0xb0, 0x35, 0x7f), new MIDI_event_ex_t(0xb0, 0x35, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "HardGate",  new MIDI_event_ex_t(0xb0, 0x3b, 0x7f), new MIDI_event_ex_t(0xb0, 0x3b, 0x00)));

        //surface->AddWidget(new Fader8Bit_CSurfWidget("Gate", surface, channel,           new MIDI_event_ex_t(0xb0, 0x36, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("GateRelease", surface, channel,    new MIDI_event_ex_t(0xb0, 0x38, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("Sustain", surface, channel,        new MIDI_event_ex_t(0xb0, 0x37, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("Punch", surface, channel,          new MIDI_event_ex_t(0xb0, 0x39, 0x7f)l));

        //surface->AddWidget(new VUMeter_CSurfWidget(GateMeter, surface, channel, new  MIDI_event_ex_t(0xb0, 0x72, 0x7f)));
        
        // EQ
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "Equalizer",    new MIDI_event_ex_t(0xb0, 0x50, 0x7f), new MIDI_event_ex_t(0xb0, 0x50, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "LoCurve",      new MIDI_event_ex_t(0xb0, 0x5d, 0x7f), new MIDI_event_ex_t(0xb0, 0x5d, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "HiCurve",      new MIDI_event_ex_t(0xb0, 0x41, 0x7f), new MIDI_event_ex_t(0xb0, 0x41, 0x00)));

        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "HiGain",       new MIDI_event_ex_t(0xb0, 0x52, 0x7f), new MIDI_event_ex_t(0xb0, 0x52, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "HiFrequency",  new MIDI_event_ex_t(0xb0, 0x53, 0x7f), new MIDI_event_ex_t(0xb0, 0x53, 0x00)));
        
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "HiMidGain",         new MIDI_event_ex_t(0xb0, 0x55, 0x7f), new MIDI_event_ex_t(0xb0, 0x55, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "HiMidFrequency",    new MIDI_event_ex_t(0xb0, 0x56, 0x7f), new MIDI_event_ex_t(0xb0, 0x56, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "HiMidQ",            new MIDI_event_ex_t(0xb0, 0x57, 0x7f), new MIDI_event_ex_t(0xb0, 0x57, 0x00)));
        
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "LoMidGain",         new MIDI_event_ex_t(0xb0, 0x58, 0x7f), new MIDI_event_ex_t(0xb0, 0x58, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "LoMidFrequency",    new MIDI_event_ex_t(0xb0, 0x59, 0x7f), new MIDI_event_ex_t(0xb0, 0x59, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "LoMidQ",            new MIDI_event_ex_t(0xb0, 0x5a, 0x7f), new MIDI_event_ex_t(0xb0, 0x5a, 0x00)));
        
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "LoGain",        new MIDI_event_ex_t(0xb0, 0x5b, 0x7f), new MIDI_event_ex_t(0xb0, 0x5b, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "LoFrequency",   new MIDI_event_ex_t(0xb0, 0x5c, 0x7f), new MIDI_event_ex_t(0xb0, 0x5c, 0x00)));

        // Compressor
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "Compressor", 1,   new MIDI_event_ex_t(0xb0, 0x2e, 0x7f), new MIDI_event_ex_t(0xb0, 0x2e, 0x00)));

        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Threshold",        new MIDI_event_ex_t(0xb0, 0x2f, 0x7f), new MIDI_event_ex_t(0xb0, 0x2f, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Release",          new MIDI_event_ex_t(0xb0, 0x30, 0x7f), new MIDI_event_ex_t(0xb0, 0x30, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Ratio",            new MIDI_event_ex_t(0xb0, 0x31, 0x7f), new MIDI_event_ex_t(0xb0, 0x31, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Parallel",         new MIDI_event_ex_t(0xb0, 0x32, 0x7f), new MIDI_event_ex_t(0xb0, 0x32, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Attack",           new MIDI_event_ex_t(0xb0, 0x33, 0x7f), new MIDI_event_ex_t(0xb0, 0x33, 0x00)));
        
        surface->AddWidget(new GainReductionMeter_MidiWidget(surface, "CompressorMeter", 0.0, -20.0, new  MIDI_event_ex_t(0xb0, 0x73, 0x7f), new  MIDI_event_ex_t(0xb0, 0x73, 0x00)));

        // Output

        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Drive",            new MIDI_event_ex_t(0xb0, 0x0f, 0x7f), new MIDI_event_ex_t(0xb0, 0x0f, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget(surface, "Character",        new MIDI_event_ex_t(0xb0, 0x12, 0x7f), new MIDI_event_ex_t(0xb0, 0x12, 0x00)));

        // Channel
        channel = new RealSurfaceChannel("1", surface);
        surface->AddChannel(channel);

        //channel->AddWidget(new Fader8Bit_CSurfWidget("InputGain", surface, channel,  new MIDI_event_ex_t(0xb0, 0x6b, 0x7f)));
        
        channel->AddWidget(new VUMeter_MidiWidget(surface, "ChannelInputMeterLeft", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x6e, 0x7f),     new MIDI_event_ex_t(0xb0, 0x6e, 0x00)));
        channel->AddWidget(new VUMeter_MidiWidget(surface, "ChannelInputMeterRight", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x6f, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x6f, 0x00)));

        channel->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "ChannelSolo",   new MIDI_event_ex_t(0xb0, 0x0d, 0x7f), new MIDI_event_ex_t(0xb0, 0x0d, 0x00)));
        channel->AddWidget(new PushButtonWithLatch_MidiWidget(surface, "ChannelMute",   new MIDI_event_ex_t(0xb0, 0x0c, 0x7f), new MIDI_event_ex_t(0xb0, 0x0c, 0x00)));

        channel->AddWidget(new Fader7Bit_MidiWidget(surface, "ChannelFader",      new MIDI_event_ex_t(0xb0, 0x07, 0x7f),  new MIDI_event_ex_t(0xb0, 0x07, 0x00)));
        channel->AddWidget(new Fader7Bit_MidiWidget(surface, "ChannelRotary",     new MIDI_event_ex_t(0xb0, 0x0a, 0x7f), new MIDI_event_ex_t(0xb0, 0x0a, 0x00)));
        

        channel->AddWidget(new VUMeter_MidiWidget(surface, "ChannelOutputMeterLeft", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x70, 0x7f),     new MIDI_event_ex_t(0xb0, 0x70, 0x00)));
        channel->AddWidget(new VUMeter_MidiWidget(surface, "ChannelOutputMeterRight", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x71, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x71, 0x00)));
    }
    else
    {
        surface->AddWidget(new PushButton_MidiWidget(surface, "Track",       new MIDI_event_ex_t(0x90, 0x28, 0x7f), new MIDI_event_ex_t(0x90, 0x28, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Send",        new MIDI_event_ex_t(0x90, 0x29, 0x7f), new MIDI_event_ex_t(0x90, 0x29, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Pan",         new MIDI_event_ex_t(0x90, 0x2a, 0x7f), new MIDI_event_ex_t(0x90, 0x2a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Plugin",      new MIDI_event_ex_t(0x90, 0x2b, 0x7f), new MIDI_event_ex_t(0x90, 0x2b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "EQ",          new MIDI_event_ex_t(0x90, 0x2c, 0x7f), new MIDI_event_ex_t(0x90, 0x2c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Instrument",  new MIDI_event_ex_t(0x90, 0x2d, 0x7f), new MIDI_event_ex_t(0x90, 0x2d, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(surface, "nameValue",   new MIDI_event_ex_t(0x90, 0x34, 0x7f), new MIDI_event_ex_t(0x90, 0x34, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "smpteBeats",  new MIDI_event_ex_t(0x90, 0x35, 0x7f), new MIDI_event_ex_t(0x90, 0x35, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(surface, "NextMap",     new MIDI_event_ex_t(0x90, 0x36, 0x7f), new MIDI_event_ex_t(0x90, 0x36, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "F2",          new MIDI_event_ex_t(0x90, 0x37, 0x7f), new MIDI_event_ex_t(0x90, 0x37, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "F3",          new MIDI_event_ex_t(0x90, 0x38, 0x7f), new MIDI_event_ex_t(0x90, 0x38, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "F4",          new MIDI_event_ex_t(0x90, 0x39, 0x7f), new MIDI_event_ex_t(0x90, 0x39, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "F5",          new MIDI_event_ex_t(0x90, 0x3a, 0x7f), new MIDI_event_ex_t(0x90, 0x3a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "F6",          new MIDI_event_ex_t(0x90, 0x3b, 0x7f), new MIDI_event_ex_t(0x90, 0x3b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "UnlockTracks",  new MIDI_event_ex_t(0x90, 0x3c, 0x7f), new MIDI_event_ex_t(0x90, 0x3c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "LockTracks",    new MIDI_event_ex_t(0x90, 0x3d, 0x7f), new MIDI_event_ex_t(0x90, 0x3d, 0x00)));

        surface->AddWidget(new PushButtonWithRelease_MidiWidget(surface, "Shift",     new MIDI_event_ex_t(0x90, 0x46, 0x7f), new MIDI_event_ex_t(0x90, 0x46, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(surface, "Option",    new MIDI_event_ex_t(0x90, 0x47, 0x7f), new MIDI_event_ex_t(0x90, 0x47, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(surface, "Control",   new MIDI_event_ex_t(0x90, 0x48, 0x7f), new MIDI_event_ex_t(0x90, 0x48, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(surface, "Alt",       new MIDI_event_ex_t(0x90, 0x49, 0x7f), new MIDI_event_ex_t(0x90, 0x49, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(surface, "Read",          new MIDI_event_ex_t(0x90, 0x4a, 0x7f), new MIDI_event_ex_t(0x90, 0x4a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Write",         new MIDI_event_ex_t(0x90, 0x4b, 0x7f), new MIDI_event_ex_t(0x90, 0x4b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Trim",          new MIDI_event_ex_t(0x90, 0x4c, 0x7f), new MIDI_event_ex_t(0x90, 0x4c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Touch",         new MIDI_event_ex_t(0x90, 0x4d, 0x7f), new MIDI_event_ex_t(0x90, 0x4d, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Latch",         new MIDI_event_ex_t(0x90, 0x4e, 0x7f), new MIDI_event_ex_t(0x90, 0x4e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Group",         new MIDI_event_ex_t(0x90, 0x4f, 0x7f), new MIDI_event_ex_t(0x90, 0x4f, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(surface, "Save",          new MIDI_event_ex_t(0x90, 0x50, 0x7f), new MIDI_event_ex_t(0x90, 0x50, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Undo",          new MIDI_event_ex_t(0x90, 0x51, 0x7f), new MIDI_event_ex_t(0x90, 0x51, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Cancel",        new MIDI_event_ex_t(0x90, 0x52, 0x7f), new MIDI_event_ex_t(0x90, 0x52, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Enter",         new MIDI_event_ex_t(0x90, 0x53, 0x7f), new MIDI_event_ex_t(0x90, 0x53, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(surface, "Cycle",         new MIDI_event_ex_t(0x90, 0x56, 0x7f), new MIDI_event_ex_t(0x90, 0x56, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Drop",          new MIDI_event_ex_t(0x90, 0x57, 0x7f), new MIDI_event_ex_t(0x90, 0x57, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Replace",       new MIDI_event_ex_t(0x90, 0x58, 0x7f), new MIDI_event_ex_t(0x90, 0x58, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Click",         new MIDI_event_ex_t(0x90, 0x59, 0x7f), new MIDI_event_ex_t(0x90, 0x59, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Solo",          new MIDI_event_ex_t(0x90, 0x5a, 0x7f), new MIDI_event_ex_t(0x90, 0x5a, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(surface, "Up",            new MIDI_event_ex_t(0x90, 0x60, 0x7f), new MIDI_event_ex_t(0x90, 0x60, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Down",          new MIDI_event_ex_t(0x90, 0x61, 0x7f), new MIDI_event_ex_t(0x90, 0x61, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Left",          new MIDI_event_ex_t(0x90, 0x62, 0x7f), new MIDI_event_ex_t(0x90, 0x62, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Right",         new MIDI_event_ex_t(0x90, 0x63, 0x7f), new MIDI_event_ex_t(0x90, 0x63, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Zoom",          new MIDI_event_ex_t(0x90, 0x64, 0x7f), new MIDI_event_ex_t(0x90, 0x64, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Scrub",         new MIDI_event_ex_t(0x90, 0x65, 0x7f), new MIDI_event_ex_t(0x90, 0x65, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(surface, "BankLeft",      new MIDI_event_ex_t(0x90, 0x2e, 0x7f), new MIDI_event_ex_t(0x90, 0x2e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "BankRight",     new MIDI_event_ex_t(0x90, 0x2f, 0x7f), new MIDI_event_ex_t(0x90, 0x2f, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "NudgeLeft",     new MIDI_event_ex_t(0x90, 0x30, 0x7f), new MIDI_event_ex_t(0x90, 0x30, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "NudgeRight",    new MIDI_event_ex_t(0x90, 0x31, 0x7f), new MIDI_event_ex_t(0x90, 0x31, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(surface, "Marker",        new MIDI_event_ex_t(0x90, 0x54, 0x7f), new MIDI_event_ex_t(0x90, 0x54, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Nudge",         new MIDI_event_ex_t(0x90, 0x55, 0x7f), new MIDI_event_ex_t(0x90, 0x55, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(surface, "Rewind",        new MIDI_event_ex_t(0x90, 0x5b, 0x7f), new MIDI_event_ex_t(0x90, 0x5b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "FastForward",   new MIDI_event_ex_t(0x90, 0x5c, 0x7f), new MIDI_event_ex_t(0x90, 0x5c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Stop",          new MIDI_event_ex_t(0x90, 0x5d, 0x7f), new MIDI_event_ex_t(0x90, 0x5d, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Play",          new MIDI_event_ex_t(0x90, 0x5e, 0x7f), new MIDI_event_ex_t(0x90, 0x5e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(surface, "Record",        new MIDI_event_ex_t(0x90, 0x5f, 0x7f), new MIDI_event_ex_t(0x90, 0x5f, 0x00)));
        
        for(int i = 0; i < surface->GetNumBankableChannels(); ++i)
        {
            channel = new RealSurfaceChannel(to_string(i + 1), surface);
            surface->AddChannel(channel);
            
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(surface, "ChannelFaderTouch", new MIDI_event_ex_t(0x90, 0x68 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x68 + i, 0x00)));
            
            channel->AddWidget(new PushButtonCycler_MidiWidget(surface, "ChannelRotaryPush", new MIDI_event_ex_t(0x90, 0x20 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x20 + i, 0x00)));
            channel->AddWidget(new Encoder_MidiWidget(surface, "ChannelRotary", new MIDI_event_ex_t(0xb0, 0x10 + i, 0x7f), new MIDI_event_ex_t(0xb0, 0x10 + i, 0x00)));

            channel->AddWidget(new Display_MidiWidget(surface, "ChannelDisplay", i));
            channel->AddWidget(new Fader14Bit_MidiWidget(surface, "ChannelFader", -72.0, 12.0, new MIDI_event_ex_t(0xe0 + i, 0x7f, 0x7f), new MIDI_event_ex_t(0xe0 + i, 0x00, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(surface, "ChannelRecordArm",   new MIDI_event_ex_t(0x90, 0x00 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x00 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(surface, "ChannelSolo",        new MIDI_event_ex_t(0x90, 0x08 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x08 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(surface, "ChannelMute",        new MIDI_event_ex_t(0x90, 0x10 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x10 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(surface, "ChannelSelect",      new MIDI_event_ex_t(0x90, 0x18 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x18 + i, 0x00)));
        }
    }
}
