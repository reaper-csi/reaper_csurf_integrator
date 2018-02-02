//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_Reaper_actions.h"
#include "control_surface_manager_actions.h"

#include "WDL/lineparse.h"
#include "WDL/projectcontext.h"

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
void RealSurface::MapRealSurfaceActions()
{
    // GAW TBD -- this will be in .axt files
    
    LogicalSurface* logicalSurface = GetSurfaceGroup()->GetLogicalSurface();
    string actionBaseAddress = RealControlSurface + GetSurfaceGroup()->GetName() + GetName();;
    
    // GAW TBD for Mix and Control
    AddAction(actionBaseAddress + "NudgeLeft", CSurfManager::Action("TrackBank", logicalSurface, -1));
    AddAction(actionBaseAddress + "NudgeRight", CSurfManager::Action("TrackBank", logicalSurface, 1));
    AddAction(actionBaseAddress + "BankLeft", CSurfManager::Action("TrackBank", logicalSurface, -8));
    AddAction(actionBaseAddress + "BankRight", CSurfManager::Action("TrackBank", logicalSurface, 8));
    
    AddAction(actionBaseAddress + "Rewind", CSurfManager::Action("Rewind", logicalSurface));
    AddAction(actionBaseAddress + "FastForward", CSurfManager::Action("FastForward", logicalSurface));
    AddAction(actionBaseAddress + "Stop", CSurfManager::Action("Stop", logicalSurface));
    AddAction(actionBaseAddress + "Play", CSurfManager::Action("Play", logicalSurface));
    AddAction(actionBaseAddress + "Record", CSurfManager::Action("Record", logicalSurface));
    
    // GAW TBD for Control only
    AddAction(actionBaseAddress + "NextMap", CSurfManager::Action("NextMap", logicalSurface));
    AddAction(actionBaseAddress + "LockTracks", CSurfManager::Action("ImmobilizeSelectedTracks", logicalSurface));
    AddAction(actionBaseAddress + "UnlockTracks", CSurfManager::Action("MobilizeSelectedTracks", logicalSurface));
    
    AddAction(actionBaseAddress + "Shift",  CSurfManager::Action("Shift", logicalSurface));
    AddAction(actionBaseAddress + "Option", CSurfManager::Action("Option", logicalSurface));
    AddAction(actionBaseAddress + "Control", CSurfManager::Action("Control", logicalSurface));
    AddAction(actionBaseAddress + "Alt", CSurfManager::Action("Alt", logicalSurface));
    
    AddAction(actionBaseAddress + "Read", CSurfManager::Action("TrackAutoMode", logicalSurface, 1));
    AddAction(actionBaseAddress + "Write", CSurfManager::Action("TrackAutoMode", logicalSurface, 3));
    AddAction(actionBaseAddress + "Trim", CSurfManager::Action("TrackAutoMode", logicalSurface, 0.0));
    AddAction(actionBaseAddress + "Touch", CSurfManager::Action("TrackAutoMode", logicalSurface, 2));
    AddAction(actionBaseAddress + "Latch", CSurfManager::Action("TrackAutoMode", logicalSurface, 4));
    AddAction(actionBaseAddress + "Group", CSurfManager::Action("TrackAutoMode", logicalSurface, 5));
    
    AddAction(actionBaseAddress + "ShiftRead", CSurfManager::Action("GlobalAutoMode", logicalSurface, 1));
    AddAction(actionBaseAddress + "ShiftWrite", CSurfManager::Action("GlobalAutoMode", logicalSurface, 3));
    AddAction(actionBaseAddress + "ShiftTrim", CSurfManager::Action("GlobalAutoMode", logicalSurface, 0.0));
    AddAction(actionBaseAddress + "ShiftTouch", CSurfManager::Action("GlobalAutoMode", logicalSurface, 2));
    AddAction(actionBaseAddress + "ShiftLatch", CSurfManager::Action("GlobalAutoMode", logicalSurface, 4));
    AddAction(actionBaseAddress + "ShiftGroup", CSurfManager::Action("GlobalAutoMode", logicalSurface, 5));
    
    AddAction(actionBaseAddress + "Save", CSurfManager::Action("Reaper", logicalSurface, 40026));
    AddAction(actionBaseAddress + "ShiftSave", CSurfManager::Action("Reaper", logicalSurface, 40022));
    AddAction(actionBaseAddress + "Undo", CSurfManager::Action("Reaper", logicalSurface, 40029));
    AddAction(actionBaseAddress + "ShiftUndo", CSurfManager::Action("Reaper", logicalSurface, 40030));
   

    //logicalSurfaceInteractor_->AddAction(new Enter_Action(Enter, logicalSurfaceInteractor_));
    //logicalSurfaceInteractor_->AddAction(new Cancel_Action(Cancel, logicalSurfaceInteractor_));
    
    AddAction(actionBaseAddress + "Marker", CSurfManager::Action("Reaper", logicalSurface, 40172));
    AddAction(actionBaseAddress + "ShiftMarker", CSurfManager::Action("Reaper", logicalSurface, 40157));
    AddAction(actionBaseAddress + "OptionMarker", CSurfManager::Action("Reaper", logicalSurface, 40174));
    AddAction(actionBaseAddress + "Nudge", CSurfManager::Action("Reaper", logicalSurface, 40173));
    AddAction(actionBaseAddress + "Cycle", CSurfManager::Action("CycleTimeline", logicalSurface));
    AddAction(actionBaseAddress + "Click", CSurfManager::Action("Reaper", logicalSurface, 40364));
    
    //AddAction(actionBaseAddress + Up, new RepeatingArrow_Action(logicalSurface, 0, 0.3));
    //AddAction(actionBaseAddress + Down, new RepeatingArrow_Action(logicalSurface, 1, 0.3));
    //AddAction(actionBaseAddress + Left, new RepeatingArrow_Action(logicalSurface, 2, 0.3));
    //AddAction(actionBaseAddress + Right, new RepeatingArrow_Action(logicalSurface, 3, 0.3));
    
    AddAction(actionBaseAddress + "Zoom", CSurfManager::Action("LatchedZoom", logicalSurface));
    AddAction(actionBaseAddress + "Scrub", CSurfManager::Action("LatchedScrub", logicalSurface));
    
    // GAW TBD for Console 1 only
    AddAction(actionBaseAddress + "DisplayFX",  CSurfManager::Action("SetShowFXWindows", logicalSurface));
}

void RealSurface::InitFXMaps()
{
    // GAW TBD -- this will be in .fxt files

    FXTemplate* fxTemplate = new FXTemplate("VST: ReaComp (Cockos)");

    fxTemplate->AddEntry("Threshold", "Thresh");
    fxTemplate->AddEntry("Character", "Gain");
    fxTemplate->AddEntry("Attack", "Attack");
    fxTemplate->AddEntry("Release", "Release");
    fxTemplate->AddEntry("Ratio", "Ratio");
    fxTemplate->AddEntry("Compressor", "Bypass");
    fxTemplate->AddEntry("Parallel", "Wet");
    fxTemplate->AddEntry("CompressorMeter", GainReduction_dB);
    
    AddFXTemplate(fxTemplate);
    
    fxTemplate = new FXTemplate("VST: UAD Fairchild 660 (Universal Audio, Inc.)");

    fxTemplate->AddEntry("Threshold", "Thresh");
    fxTemplate->AddEntry("Character", "Output");
    fxTemplate->AddEntry("Drive", "Meter");
    fxTemplate->AddEntry("Attack", "Headroom");
    fxTemplate->AddEntry("Release", "Input");
    fxTemplate->AddEntry("Ratio", "Time Const");
    fxTemplate->AddEntry("Compressor", "Bypass");
    fxTemplate->AddEntry("Parallel", "Wet");
    
    AddFXTemplate(fxTemplate);
    
    fxTemplate = new FXTemplate("VST: UAD Teletronix LA-2A Silver (Universal Audio, Inc.)");
    
    fxTemplate->AddEntry("Threshold", "Peak Reduct");
    fxTemplate->AddEntry("Character", "Gain");
    fxTemplate->AddEntry("Drive", "Meter");
    fxTemplate->AddEntry("Attack", "Emphasis");
    fxTemplate->AddEntry("Ratio", "Comp/Limit");
    fxTemplate->AddEntry("Compressor", "Bypass");
    fxTemplate->AddEntry("Parallel", "Wet");
    
    AddFXTemplate(fxTemplate);
    
    fxTemplate = new FXTemplate("VST: UAD Harrison 32C (Universal Audio, Inc.)");
    
    fxTemplate->AddEntry("LoCurve", "LowPeak");
    //fxMap->AddEntry(HiCurve, "");
    fxTemplate->AddEntry("HiGain", "HiGain");
    fxTemplate->AddEntry("HiFrequency", "HiFreq");
    fxTemplate->AddEntry("HiMidGain", "HiMidGain");
    fxTemplate->AddEntry("HiMidFrequency", "HiMidFreq");
    fxTemplate->AddEntry("HiMidQ", "LowPass");
    fxTemplate->AddEntry("LoMidGain", "LoMidGain");
    fxTemplate->AddEntry("LoMidFrequency", "LoMidFreq");
    fxTemplate->AddEntry("LoMidQ", "HiPass");
    fxTemplate->AddEntry("LoGain", "LowGain");
    fxTemplate->AddEntry("LoFrequency", "LowFreq");
    fxTemplate->AddEntry("Equalizer", "Bypass");
    
    AddFXTemplate(fxTemplate);
    
    fxTemplate = new FXTemplate("VST: UAD Pultec EQP-1A (Universal Audio, Inc.)");
    
    //fxMap->AddEntry(LoCurve, "");
    //fxMap->AddEntry(HiCurve, "");
    fxTemplate->AddEntry("HiGain", "HF Atten");
    fxTemplate->AddEntry("HiFrequency", "HF Atten Freq");
    fxTemplate->AddEntry("HiMidGain", "HF Boost");
    fxTemplate->AddEntry("HiMidFrequency", "High Freq");
    fxTemplate->AddEntry("HiMidQ", "HF Q");
    fxTemplate->AddEntry("LoMidGain", "LF Atten");
    fxTemplate->AddEntry("LoMidFrequency", "Low Freq");
    //fxMap->AddEntry(LoMidQ, "");
    fxTemplate->AddEntry("LoGain", "LF Boost");
    fxTemplate->AddEntry("LoFrequency", "Low Freq");
    fxTemplate->AddEntry("Equalizer", "Bypass");
    
    AddFXTemplate(fxTemplate);
    
    fxTemplate = new FXTemplate("VST: UAD Pultec MEQ-5 (Universal Audio, Inc.)");
    
    //fxMap->AddEntry(LoCurve, "");
    //fxMap->AddEntry(HiCurve, "");
    fxTemplate->AddEntry("HiGain", "HM Peak");
    fxTemplate->AddEntry("HiFrequency", "HM Freq");
    fxTemplate->AddEntry("HiMidGain", "Mid Dip");
    fxTemplate->AddEntry("HiMidFrequency", "Mid Freq");
    //fxMap->AddEntry(HiMidQ, "");
    fxTemplate->AddEntry("LoMidGain", "LM Peak");
    fxTemplate->AddEntry("LoMidFrequency", "LM Freq");
    //fxMap->AddEntry(LoMidQ, "");
    //fxMap->AddEntry(LoGain, "");
    //fxMap->AddEntry(LoFrequency, "");
    fxTemplate->AddEntry("Equalizer", "Bypass");
    
    AddFXTemplate(fxTemplate);
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

void RealSurface::MapTrackActions(string trackGUID)
{
    LogicalSurface* logicalSurface = GetSurfaceGroup()->GetLogicalSurface();
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
    string actionBaseAddress = trackGUID + GetSurfaceGroup()->GetName() + GetName();
    
    // GAW TBD -- this will be in .axt files

    if(GetName() == "Console1")
    {
        AddAction(actionBaseAddress + "ChannelFader", CSurfManager::Action("TrackVolume", logicalSurface, track));
        AddAction(actionBaseAddress + "ChannelRotary", CSurfManager::Action("TrackPan", logicalSurface, track, 0x00));
        AddAction(actionBaseAddress + "ChannelMute", CSurfManager::Action("TrackMute", logicalSurface, track));
        AddAction(actionBaseAddress + "ChannelSolo", CSurfManager::Action("TrackSolo", logicalSurface, track));

        AddAction(actionBaseAddress + "ChannelInputMeterLeft", CSurfManager::Action("TrackOutputMeter", logicalSurface, track, 0));
        AddAction(actionBaseAddress + "ChannelInputMeterRight", CSurfManager::Action("TrackOutputMeter", logicalSurface, track, 1));
        
        AddAction(actionBaseAddress + "ChannelOutputMeterLeft", CSurfManager::Action("TrackOutputMeter", logicalSurface, track, 0));
        AddAction(actionBaseAddress + "ChannelOutputMeterRight", CSurfManager::Action("TrackOutputMeter", logicalSurface, track, 1));
        
        AddAction(actionBaseAddress + "TrackOnSelection", CSurfManager::Action("MapTrackAndFXToWidgets", logicalSurface, track));
    }
    else
    {
        AddAction(actionBaseAddress + "ChannelDisplay", CSurfManager::Action("TrackNameDisplay", logicalSurface, track));
        
        // GAW TBD
        AddAction(actionBaseAddress + "ChannelDisplay", new TrackTouchControlled_Action(logicalSurface, track, new TrackVolumeDisplay_Action(logicalSurface, track), actionBaseAddress + "ChannelDisplay"));
        
        AddAction(actionBaseAddress + "ChannelFader", CSurfManager::Action("TrackVolume", logicalSurface, track));
        AddAction(actionBaseAddress + "ChannelFaderTouch", CSurfManager::Action("TrackTouch", logicalSurface, track));
        
        // GAW TBD
        CycledAction* cycledAction = new CycledAction(logicalSurface);
        cycledAction->AddAction(new TrackPan_Action(logicalSurface, track, 0x00));
        cycledAction->AddAction(new TrackPanWidth_Action(logicalSurface, track, 0x30));
        AddAction(actionBaseAddress + "ChannelRotary", cycledAction);
        AddAction(actionBaseAddress + "ChannelRotaryPush", cycledAction);
        
        AddAction(actionBaseAddress + "ChannelSelect", CSurfManager::Action("TrackUniqueSelect", logicalSurface, track));
        AddAction(actionBaseAddress + "ShiftChannelSelect", CSurfManager::Action("TrackRangeSelect", logicalSurface, track));
        AddAction(actionBaseAddress + "ControlChannelSelect", CSurfManager::Action("TrackSelect", logicalSurface, track));
        
        AddAction(actionBaseAddress + "ChannelRecordArm", CSurfManager::Action("TrackRecordArm", logicalSurface, track));
        AddAction(actionBaseAddress + "ChannelMute", CSurfManager::Action("TrackMute", logicalSurface, track));
        AddAction(actionBaseAddress + "ChannelSolo", CSurfManager::Action("TrackSolo", logicalSurface, track));
    }
}

void RealSurface::MapFXActions(string trackGUID)
{
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
    LogicalSurface* logicalSurface = GetSurfaceGroup()->GetLogicalSurface();
    
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    char fxGUID[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(fxTemplates_.count(fxName) > 0)
        {
            FXTemplate* map = fxTemplates_[fxName];
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            string actionBaseAddress = trackGUID + fxGUID + GetSurfaceGroup()->GetName() + GetName();

            for(auto mapEntry : map->GetTemplateEntries())
            {
                if(mapEntry.paramName == GainReduction_dB)
                    AddAction(actionBaseAddress + mapEntry.widgetName, new TrackGainReductionMeter_Action(logicalSurface, track, fxGUID));
                else
                {
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                    {
                        DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                        
                        if(mapEntry.paramName == fxParamName)
                            AddAction(actionBaseAddress + mapEntry.widgetName, new TrackFX_Action(logicalSurface, track, fxGUID, j));
                    }
                }
            }
        }
        
        if(GetSurfaceGroup()->GetLogicalSurface()->GetManager()->GetVSTMonitor() && GetSurfaceGroup()->GetLogicalSurface()->GetManager()->GetIsInitialized())
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

std::hash<std::string> hash_fn;

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
Action* CSurfManager::Action(string name, LogicalSurface* logicalSurface)
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
    else if(name == "CycleTimeline")  return new CycleTimeline_Action(logicalSurface);

    return new class Action(logicalSurface);
}

Action* CSurfManager::Action(string name, LogicalSurface* logicalSurface, double param)
{
    if(name == "TrackBank")  return new TrackBank_Action(logicalSurface, param);
    else if(name == "TrackAutoMode")  return new TrackAutoMode_Action(logicalSurface, param);
    else if(name == "GlobalAutoMode")  return new GlobalAutoMode_Action(logicalSurface, param);
    else if(name == "Reaper")  return new Reaper_Action(logicalSurface, param);
    
    return new class Action(logicalSurface);
}

Action* CSurfManager::Action(string name, LogicalSurface* logicalSurface, MediaTrack* track)
{
    if(name == "TrackVolume")  return new TrackVolume_Action(logicalSurface, track);
    else if(name == "TrackTouch")  return new TrackTouch_Action(logicalSurface, track);
    else if(name == "TrackMute")  return new TrackMute_Action(logicalSurface, track);
    else if(name == "TrackSolo")  return new TrackSolo_Action(logicalSurface, track);
    else if(name == "TrackUniqueSelect")  return new TrackUniqueSelect_Action(logicalSurface, track);
    else if(name == "TrackRangeSelect")  return new TrackRangeSelect_Action(logicalSurface, track);
    else if(name == "TrackSelect")  return new TrackSelect_Action(logicalSurface, track);
    else if(name == "TrackRecordArm")  return new TrackRecordArm_Action(logicalSurface, track);
    else if(name == "TrackNameDisplay")  return new TrackNameDisplay_Action(logicalSurface, track);
    else if(name == "MapTrackAndFXToWidgets")  return new MapTrackAndFXToWidgets_Action(logicalSurface, track);
    
    return new class Action(logicalSurface);
}

Action* CSurfManager::Action(string name, LogicalSurface* logicalSurface, MediaTrack* track, double param)
{
    if(name == "TrackPan")  return new TrackPan_Action(logicalSurface, track, param);
    else if(name == "TrackOutputMeter")  return new TrackOutputMeter_Action(logicalSurface, track, param);
    
    return new class Action(logicalSurface);
}

void CSurfManager::InitRealSurfaces()
{
    // GAW TBD -- this will be in CSI.ini

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GAW TBD Hack an ini file so that testers can config MIDI IO for their local surfaces
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    const char *ptr = DAW::GetResourcePath();
    char localBuf[4096];
    
    strcpy( localBuf, ptr );
    
    strcpy( localBuf + strlen(localBuf), "/CSI/CSI.ini" );
    
    FILE *filePtr;
    
    filePtr = fopen(localBuf, "r" );
    
    if(! filePtr)
    {
        char errorBuf[4096];
        strcpy(errorBuf, "Can't locate ");
        strcpy(&errorBuf[13], localBuf);
        DAW::ShowConsoleMsg(errorBuf);
        return;
    }
    
    while (fgets(localBuf, sizeof(localBuf), filePtr))
        if(localBuf[0] == '/')
            continue;
        else
            break;
    
    char *p;
    char optionName[512];
    char optionValue[512];
    
    
    
    
    // Midi monitoring //////////////////////////
    bool midiInMonitor = false;
    
    p = strtok (localBuf,"=");
    
    strcpy( optionName, p );
    
    if(!strcmp(optionName, "MidiInMonitor"))
        if(p != NULL)
        {
            p = strtok(NULL, "=");
            
            if(p != NULL)
            {
                strcpy( optionValue, p );
                if(!strcmp(optionValue, "On\n"))
                    midiInMonitor = true;
                
            }
        }
    
    fgets(localBuf, sizeof(localBuf), filePtr);
    
    bool midiOutMonitor = false;
    
    p = strtok (localBuf,"=");
    
    strcpy( optionName, p );
    
    if(!strcmp(optionName, "MidiOutMonitor"))
        if(p != NULL)
        {
            p = strtok(NULL, "=");
            
            if(p != NULL)
            {
                strcpy( optionValue, p );
                if(!strcmp(optionValue, "On\n"))
                    midiOutMonitor = true;
                
            }
        }
    
    // VST monitoring //////////////////////////
    fgets(localBuf, sizeof(localBuf), filePtr);
    
    bool VSTMonitor = false;
    
    p = strtok (localBuf,"=");
    
    strcpy( optionName, p );
    
    if(!strcmp(optionName, "VSTMonitor"))
        if(p != NULL)
        {
            p = strtok(NULL, "=");
            
            if(p != NULL)
            {
                strcpy( optionValue, p );
                if(!strcmp(optionValue, "On\n"))
                    VSTMonitor = true;
                
            }
        }
    
    while (fgets(localBuf, sizeof(localBuf), filePtr))
    {
        if(localBuf[0] == '/')
            continue;
        
        int index = 0;
        char name[512];
        char numBankableChannelsString[512];
        char channelInString[512];
        char channelOutString[512];
        
        p = strtok (localBuf," ");
        
        strcpy( name, p );
        index++;
        
        while (p != NULL)
        {
            p = strtok(NULL, " ");
            
            if( p != NULL && index == 1)
            {
                strcpy(numBankableChannelsString, p);
                index++;
            }
            else if(p != NULL && index == 2)
            {
                strcpy(channelInString, p);
                index++;
            }
            else if(p != NULL && index == 3)
            {
                strcpy(channelOutString, p);
                index++;
            }
        }
        
        int numBankableChannels = atoi(numBankableChannelsString);
        
        int channelIn = atoi(channelInString);
        channelIn--; // MIDI channels are 0  based
        
        int channelOut = atoi(channelOutString);
        channelOut--; // MIDI channels are 0  based
        
        AddRealSurface(new MidiCSurf(name, numBankableChannels, GetMidiIOManager()->GetMidiInputForChannel(channelIn), GetMidiIOManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
    }
    
    VSTMonitor_ = VSTMonitor;
    
    fclose ( filePtr );
}

void CSurfManager::InitRealSurface(RealSurface* surface)
{
    
    // GAW TBD -- this will be in .rst files
    
    RealSurfaceChannel* channel = nullptr;
    
    if(surface->GetName() == "Console1")
    {
        surface->AddWidget(new PushButtonWithLatch_MidiWidget(RealControlSurface, surface, "DisplayFX", new MIDI_event_ex_t(0xb0, 0x66, 0x7f), new MIDI_event_ex_t(0xb0, 0x66, 0x00)));

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
        
        
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "Order",             new MIDI_event_ex_t(0xb0, 0x0e, 0x7f), new MIDI_event_ex_t(0xb0, 0x0e, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "ExternalSidechain", new MIDI_event_ex_t(0xb0, 0x11, 0x7f), new MIDI_event_ex_t(0xb0, 0x11, 0x00)));

        // Input
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "FiltersToCompressor",   new MIDI_event_ex_t(0xb0, 0x3d, 0x7f), new MIDI_event_ex_t(0xb0, 0x3d, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "PhaseInvert",           new MIDI_event_ex_t(0xb0, 0x6c, 0x7f), new MIDI_event_ex_t(0xb0, 0x6c, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "Preset",                new MIDI_event_ex_t(0xb0, 0x3a, 0x7f), new MIDI_event_ex_t(0xb0, 0x3a, 0x00)));

        //surface->AddWidget(new Fader8Bit_CSurfWidget("HiCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x69, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("LoCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x67, 0x7f)));
    
        
        // Shape
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "Shape",     new MIDI_event_ex_t(0xb0, 0x35, 0x7f), new MIDI_event_ex_t(0xb0, 0x35, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "HardGate",  new MIDI_event_ex_t(0xb0, 0x3b, 0x7f), new MIDI_event_ex_t(0xb0, 0x3b, 0x00)));

        //surface->AddWidget(new Fader8Bit_CSurfWidget("Gate", surface, channel,           new MIDI_event_ex_t(0xb0, 0x36, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("GateRelease", surface, channel,    new MIDI_event_ex_t(0xb0, 0x38, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("Sustain", surface, channel,        new MIDI_event_ex_t(0xb0, 0x37, 0x7f)));
        //surface->AddWidget(new Fader8Bit_CSurfWidget("Punch", surface, channel,          new MIDI_event_ex_t(0xb0, 0x39, 0x7f)l));

        //surface->AddWidget(new VUMeter_CSurfWidget(GateMeter, surface, channel, new  MIDI_event_ex_t(0xb0, 0x72, 0x7f)));
        
        // EQ
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "Equalizer",    new MIDI_event_ex_t(0xb0, 0x50, 0x7f), new MIDI_event_ex_t(0xb0, 0x50, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "LoCurve",      new MIDI_event_ex_t(0xb0, 0x5d, 0x7f), new MIDI_event_ex_t(0xb0, 0x5d, 0x00)));
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "HiCurve",      new MIDI_event_ex_t(0xb0, 0x41, 0x7f), new MIDI_event_ex_t(0xb0, 0x41, 0x00)));

        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "HiGain",       new MIDI_event_ex_t(0xb0, 0x52, 0x7f), new MIDI_event_ex_t(0xb0, 0x52, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "HiFrequency",  new MIDI_event_ex_t(0xb0, 0x53, 0x7f), new MIDI_event_ex_t(0xb0, 0x53, 0x00)));
        
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "HiMidGain",         new MIDI_event_ex_t(0xb0, 0x55, 0x7f), new MIDI_event_ex_t(0xb0, 0x55, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "HiMidFrequency",    new MIDI_event_ex_t(0xb0, 0x56, 0x7f), new MIDI_event_ex_t(0xb0, 0x56, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "HiMidQ",            new MIDI_event_ex_t(0xb0, 0x57, 0x7f), new MIDI_event_ex_t(0xb0, 0x57, 0x00)));
        
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "LoMidGain",         new MIDI_event_ex_t(0xb0, 0x58, 0x7f), new MIDI_event_ex_t(0xb0, 0x58, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "LoMidFrequency",    new MIDI_event_ex_t(0xb0, 0x59, 0x7f), new MIDI_event_ex_t(0xb0, 0x59, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "LoMidQ",            new MIDI_event_ex_t(0xb0, 0x5a, 0x7f), new MIDI_event_ex_t(0xb0, 0x5a, 0x00)));
        
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "LoGain",        new MIDI_event_ex_t(0xb0, 0x5b, 0x7f), new MIDI_event_ex_t(0xb0, 0x5b, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "LoFrequency",   new MIDI_event_ex_t(0xb0, 0x5c, 0x7f), new MIDI_event_ex_t(0xb0, 0x5c, 0x00)));

        // Compressor
        surface->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "Compressor", 1,   new MIDI_event_ex_t(0xb0, 0x2e, 0x7f), new MIDI_event_ex_t(0xb0, 0x2e, 0x00)));

        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Threshold",        new MIDI_event_ex_t(0xb0, 0x2f, 0x7f), new MIDI_event_ex_t(0xb0, 0x2f, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Release",          new MIDI_event_ex_t(0xb0, 0x30, 0x7f), new MIDI_event_ex_t(0xb0, 0x30, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Ratio",            new MIDI_event_ex_t(0xb0, 0x31, 0x7f), new MIDI_event_ex_t(0xb0, 0x31, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Parallel",         new MIDI_event_ex_t(0xb0, 0x32, 0x7f), new MIDI_event_ex_t(0xb0, 0x32, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Attack",           new MIDI_event_ex_t(0xb0, 0x33, 0x7f), new MIDI_event_ex_t(0xb0, 0x33, 0x00)));
        
        surface->AddWidget(new GainReductionMeter_MidiWidget("", surface, "CompressorMeter", 0.0, -20.0, new  MIDI_event_ex_t(0xb0, 0x73, 0x7f), new  MIDI_event_ex_t(0xb0, 0x73, 0x00)));

        // Output

        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Drive",            new MIDI_event_ex_t(0xb0, 0x0f, 0x7f), new MIDI_event_ex_t(0xb0, 0x0f, 0x00)));
        surface->AddWidget(new Fader7Bit_MidiWidget("", surface, "Character",        new MIDI_event_ex_t(0xb0, 0x12, 0x7f), new MIDI_event_ex_t(0xb0, 0x12, 0x00)));

        // Channel
        channel = new RealSurfaceChannel( "", surface);
        surface->AddChannel(channel);

        //channel->AddWidget(new Fader8Bit_CSurfWidget("InputGain", surface, channel,  new MIDI_event_ex_t(0xb0, 0x6b, 0x7f)));
        
        channel->AddWidget(new VUMeter_MidiWidget("", surface, "ChannelInputMeterLeft", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x6e, 0x7f),     new MIDI_event_ex_t(0xb0, 0x6e, 0x00)));
        channel->AddWidget(new VUMeter_MidiWidget("", surface, "ChannelInputMeterRight", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x6f, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x6f, 0x00)));

        channel->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "ChannelSolo",   new MIDI_event_ex_t(0xb0, 0x0d, 0x7f), new MIDI_event_ex_t(0xb0, 0x0d, 0x00)));
        channel->AddWidget(new PushButtonWithLatch_MidiWidget("", surface, "ChannelMute",   new MIDI_event_ex_t(0xb0, 0x0c, 0x7f), new MIDI_event_ex_t(0xb0, 0x0c, 0x00)));

        channel->AddWidget(new Fader7Bit_MidiWidget("", surface, "ChannelFader",      new MIDI_event_ex_t(0xb0, 0x07, 0x7f),  new MIDI_event_ex_t(0xb0, 0x07, 0x00)));
        channel->AddWidget(new Fader7Bit_MidiWidget("", surface, "ChannelRotary",     new MIDI_event_ex_t(0xb0, 0x0a, 0x7f), new MIDI_event_ex_t(0xb0, 0x0a, 0x00)));
        

        channel->AddWidget(new VUMeter_MidiWidget("", surface, "ChannelOutputMeterLeft", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x70, 0x7f),     new MIDI_event_ex_t(0xb0, 0x70, 0x00)));
        channel->AddWidget(new VUMeter_MidiWidget("", surface, "ChannelOutputMeterRight", -60.0, 6.0, new MIDI_event_ex_t(0xb0, 0x71, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x71, 0x00)));
    }
    else
    {
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Track",       new MIDI_event_ex_t(0x90, 0x28, 0x7f), new MIDI_event_ex_t(0x90, 0x28, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Send",        new MIDI_event_ex_t(0x90, 0x29, 0x7f), new MIDI_event_ex_t(0x90, 0x29, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Pan",         new MIDI_event_ex_t(0x90, 0x2a, 0x7f), new MIDI_event_ex_t(0x90, 0x2a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Plugin",      new MIDI_event_ex_t(0x90, 0x2b, 0x7f), new MIDI_event_ex_t(0x90, 0x2b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "EQ",          new MIDI_event_ex_t(0x90, 0x2c, 0x7f), new MIDI_event_ex_t(0x90, 0x2c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Instrument",  new MIDI_event_ex_t(0x90, 0x2d, 0x7f), new MIDI_event_ex_t(0x90, 0x2d, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "nameValue",   new MIDI_event_ex_t(0x90, 0x34, 0x7f), new MIDI_event_ex_t(0x90, 0x34, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "smpteBeats",  new MIDI_event_ex_t(0x90, 0x35, 0x7f), new MIDI_event_ex_t(0x90, 0x35, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "NextMap",     new MIDI_event_ex_t(0x90, 0x36, 0x7f), new MIDI_event_ex_t(0x90, 0x36, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "F2",          new MIDI_event_ex_t(0x90, 0x37, 0x7f), new MIDI_event_ex_t(0x90, 0x37, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "F3",          new MIDI_event_ex_t(0x90, 0x38, 0x7f), new MIDI_event_ex_t(0x90, 0x38, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "F4",          new MIDI_event_ex_t(0x90, 0x39, 0x7f), new MIDI_event_ex_t(0x90, 0x39, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "F5",          new MIDI_event_ex_t(0x90, 0x3a, 0x7f), new MIDI_event_ex_t(0x90, 0x3a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "F6",          new MIDI_event_ex_t(0x90, 0x3b, 0x7f), new MIDI_event_ex_t(0x90, 0x3b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "UnlockTracks",  new MIDI_event_ex_t(0x90, 0x3c, 0x7f), new MIDI_event_ex_t(0x90, 0x3c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "LockTracks",    new MIDI_event_ex_t(0x90, 0x3d, 0x7f), new MIDI_event_ex_t(0x90, 0x3d, 0x00)));

        surface->AddWidget(new PushButtonWithRelease_MidiWidget(RealControlSurface, surface, "Shift",     new MIDI_event_ex_t(0x90, 0x46, 0x7f), new MIDI_event_ex_t(0x90, 0x46, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(RealControlSurface, surface, "Option",    new MIDI_event_ex_t(0x90, 0x47, 0x7f), new MIDI_event_ex_t(0x90, 0x47, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(RealControlSurface, surface, "Control",   new MIDI_event_ex_t(0x90, 0x48, 0x7f), new MIDI_event_ex_t(0x90, 0x48, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(RealControlSurface, surface, "Alt",       new MIDI_event_ex_t(0x90, 0x49, 0x7f), new MIDI_event_ex_t(0x90, 0x49, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Read",          new MIDI_event_ex_t(0x90, 0x4a, 0x7f), new MIDI_event_ex_t(0x90, 0x4a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Write",         new MIDI_event_ex_t(0x90, 0x4b, 0x7f), new MIDI_event_ex_t(0x90, 0x4b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Trim",          new MIDI_event_ex_t(0x90, 0x4c, 0x7f), new MIDI_event_ex_t(0x90, 0x4c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Touch",         new MIDI_event_ex_t(0x90, 0x4d, 0x7f), new MIDI_event_ex_t(0x90, 0x4d, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Latch",         new MIDI_event_ex_t(0x90, 0x4e, 0x7f), new MIDI_event_ex_t(0x90, 0x4e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Group",         new MIDI_event_ex_t(0x90, 0x4f, 0x7f), new MIDI_event_ex_t(0x90, 0x4f, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Save",          new MIDI_event_ex_t(0x90, 0x50, 0x7f), new MIDI_event_ex_t(0x90, 0x50, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Undo",          new MIDI_event_ex_t(0x90, 0x51, 0x7f), new MIDI_event_ex_t(0x90, 0x51, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Cancel",        new MIDI_event_ex_t(0x90, 0x52, 0x7f), new MIDI_event_ex_t(0x90, 0x52, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Enter",         new MIDI_event_ex_t(0x90, 0x53, 0x7f), new MIDI_event_ex_t(0x90, 0x53, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Cycle",         new MIDI_event_ex_t(0x90, 0x56, 0x7f), new MIDI_event_ex_t(0x90, 0x56, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Drop",          new MIDI_event_ex_t(0x90, 0x57, 0x7f), new MIDI_event_ex_t(0x90, 0x57, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Replace",       new MIDI_event_ex_t(0x90, 0x58, 0x7f), new MIDI_event_ex_t(0x90, 0x58, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Click",         new MIDI_event_ex_t(0x90, 0x59, 0x7f), new MIDI_event_ex_t(0x90, 0x59, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Solo",          new MIDI_event_ex_t(0x90, 0x5a, 0x7f), new MIDI_event_ex_t(0x90, 0x5a, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Up",            new MIDI_event_ex_t(0x90, 0x60, 0x7f), new MIDI_event_ex_t(0x90, 0x60, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Down",          new MIDI_event_ex_t(0x90, 0x61, 0x7f), new MIDI_event_ex_t(0x90, 0x61, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Left",          new MIDI_event_ex_t(0x90, 0x62, 0x7f), new MIDI_event_ex_t(0x90, 0x62, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Right",         new MIDI_event_ex_t(0x90, 0x63, 0x7f), new MIDI_event_ex_t(0x90, 0x63, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Zoom",          new MIDI_event_ex_t(0x90, 0x64, 0x7f), new MIDI_event_ex_t(0x90, 0x64, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Scrub",         new MIDI_event_ex_t(0x90, 0x65, 0x7f), new MIDI_event_ex_t(0x90, 0x65, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "BankLeft",      new MIDI_event_ex_t(0x90, 0x2e, 0x7f), new MIDI_event_ex_t(0x90, 0x2e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "BankRight",     new MIDI_event_ex_t(0x90, 0x2f, 0x7f), new MIDI_event_ex_t(0x90, 0x2f, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "NudgeLeft",     new MIDI_event_ex_t(0x90, 0x30, 0x7f), new MIDI_event_ex_t(0x90, 0x30, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "NudgeRight",    new MIDI_event_ex_t(0x90, 0x31, 0x7f), new MIDI_event_ex_t(0x90, 0x31, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Marker",        new MIDI_event_ex_t(0x90, 0x54, 0x7f), new MIDI_event_ex_t(0x90, 0x54, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Nudge",         new MIDI_event_ex_t(0x90, 0x55, 0x7f), new MIDI_event_ex_t(0x90, 0x55, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Rewind",        new MIDI_event_ex_t(0x90, 0x5b, 0x7f), new MIDI_event_ex_t(0x90, 0x5b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "FastForward",   new MIDI_event_ex_t(0x90, 0x5c, 0x7f), new MIDI_event_ex_t(0x90, 0x5c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Stop",          new MIDI_event_ex_t(0x90, 0x5d, 0x7f), new MIDI_event_ex_t(0x90, 0x5d, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Play",          new MIDI_event_ex_t(0x90, 0x5e, 0x7f), new MIDI_event_ex_t(0x90, 0x5e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(RealControlSurface, surface, "Record",        new MIDI_event_ex_t(0x90, 0x5f, 0x7f), new MIDI_event_ex_t(0x90, 0x5f, 0x00)));
        
        for(int i = 0; i < surface->GetNumBankableChannels(); ++i)
        {
            channel = new RealSurfaceChannel(to_string(i + 1), surface);
            surface->AddChannel(channel);
            
            channel->AddWidget(new PushButtonWithRelease_MidiWidget("", surface, "ChannelFaderTouch", new MIDI_event_ex_t(0x90, 0x68 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x68 + i, 0x00)));
            
            channel->AddWidget(new PushButtonCycler_MidiWidget("", surface, "ChannelRotaryPush", new MIDI_event_ex_t(0x90, 0x20 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x20 + i, 0x00)));
            channel->AddWidget(new Encoder_MidiWidget("", surface, "ChannelRotary", new MIDI_event_ex_t(0xb0, 0x10 + i, 0x7f), new MIDI_event_ex_t(0xb0, 0x10 + i, 0x00)));

            channel->AddWidget(new Display_MidiWidget("", surface, "ChannelDisplay", i));
            channel->AddWidget(new Fader14Bit_MidiWidget("", surface, "ChannelFader", -72.0, 12.0, new MIDI_event_ex_t(0xe0 + i, 0x7f, 0x7f), new MIDI_event_ex_t(0xe0 + i, 0x00, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "ChannelRecordArm",   new MIDI_event_ex_t(0x90, 0x00 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x00 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "ChannelSolo",        new MIDI_event_ex_t(0x90, 0x08 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x08 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "ChannelMute",        new MIDI_event_ex_t(0x90, 0x10 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x10 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "ChannelSelect",      new MIDI_event_ex_t(0x90, 0x18 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x18 + i, 0x00)));
        }
    }
}
