//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_Reaper_actions.h"
#include "control_surface_manager_actions.h"

const string ChannelLeft = "ChannelLeft";
const string ChannelRight = "ChannelRight";
const string BankLeft = "BankLeft";
const string BankRight = "BankRight";

const string Volume = "Volume";
const string Pan = "Pan";
const string TrackDisplay = "TrackDisplay";
const string TrackTouched = "TrackTouched";
const string Solo = "Solo";
const string RecordArm = "RecordArm";
const string Select = "Select";
const string Mute = "Mute";

const string NextMap = "NextMap";
const string LockTracks = "LockTracks";
const string UnlockTracks = "UnlockTracks";

const string Read = "Read";
const string Write = "Write";
const string Trim = "Trim";
const string Touch = "Touch";
const string Latch = "Latch";
const string Group = "Group";

const string Save = "Save";
const string Undo = "Undo";
const string Cancel = "Cancel";
const string Enter = "Enter";

const string Marker = "Marker";
const string Nudge = "Nudge";
const string Cycle = "Cycle";
const string Click = "Click";

const string Rewind = "Rewind";
const string FastForward = "FastForward";
const string Stop = "Stop";
const string Play = "Play";
const string Record = "Record";

const string Up = "Up";
const string Down = "Down";
const string Left = "Left";
const string Right = "Right";
const string Zoom = "Zoom";
const string Scrub = "Scrub";

const string TrackInMeterLeft = "TrackInMeterLeft";
const string TrackInMeterRight = "TrackInMeterRight";
const string CompressorMeter = "CompressorMeter";
const string GateMeter = "GateMeter";
const string TrackOutMeterLeft = "TrackOutMeterLeft";
const string TrackOutMeterRight = "TrackOutMeterRight";

const string DisplayFX = "DisplayFX";
const string SendsMode = "SendsMode";

const string  Equalizer = "Equalizer";

const string  LoCurve = "LoCurve";
const string  HiCurve = "HiCurve";
const string  HiGain = "HiGain";
const string  HiFrequency = "HiFrequency";
const string  HiMidGain = "HiMidGain";
const string  HiMidFrequency = "HiMidFrequency";
const string  HiMidQ = "HiMidQ";
const string  LoMidGain = "LoMidGain";
const string  LoMidFrequency = "LoMidFrequency";
const string  LoMidQ = "LoMidQ";
const string  LoGain = "LoGain";
const string  LoFrequency = "LoFrequency";

const string Compressor = "Compressor";

const string Threshold = "Threshold";
const string Release = "Release";
const string Ratio = "Ratio";
const string Parallel = "Parallel";
const string Attack = "Attack";

const string Drive = "Drive";
const string Character = "Character";

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void MidiWidget::Update()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->UpdateAction(GetGUID(), GetName());
}

void MidiWidget::ForceUpdate()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->ForceUpdateAction(GetGUID(), GetName());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealCSurf
////////////////////////////////////////////////////////////////////////////////////////////////////////
// to Actions ->
double RealSurface::GetActionCurrentNormalizedValue(string GUID, string widgetName)
{
    return GetLogicalSurface()->GetActionCurrentNormalizedValue(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealSurface::UpdateAction(string GUID, string widgetName)
{
    GetLogicalSurface()->UpdateAction(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealSurface::ForceUpdateAction(string GUID, string widgetName)
{
    GetLogicalSurface()->ForceUpdateAction(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealSurface::CycleAction(string GUID, string widgetName)
{
    GetLogicalSurface()->CycleAction(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealSurface::RunAction(string GUID, string widgetName, double value)
{
    GetLogicalSurface()->RunAction(ActionAddressFor(GUID, widgetName), value, GetName(), widgetName);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// LogicalSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogicalSurface::MapWidgetsToFX(MediaTrack *track)
{
    /*
    SetGUID(DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false)));
    
    char trackFXName[256];
    char trackFXGUID[256];
    char trackFXParamName[256];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, trackFXName, sizeof(trackFXName));
        string fxName = trackFXName;
        
        if(GetSurface()->GetLogicalSurface()->GetFXMaps().count(fxName) > 0)
        {
            DAW::TrackFX_Show(track, i, 3);
            
            FXMap* map = GetSurface()->GetLogicalSurface()->GetFXMaps()[fxName];
            
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), trackFXGUID);
            string fxGUID = trackFXGUID;
            
     
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, trackFXParamName, sizeof(trackFXParamName));
                string fxParamName = trackFXParamName;
                
                for(auto map : map->GetMapEntries())
                    if(map.paramName == fxParamName)
                        subChannel->AddWidgetName(map.widgetName);
            }
            
            AddSubChannel(subChannel);
     
            
            
        }
    }
     */
}

void LogicalSurface::MapFX(MediaTrack* track)
{
    char trackFXName[256];
    char trackFXParameterName[256];
    char trackFXParameterGUID[256];
    
    string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
    
    // We will always find the TrackGUIDAssociations for this track, otherwise how could we add FX to it ?
    TrackGUIDAssociation* trackGUIDAssociation = nullptr;
     
    for(auto * aTrackGUIDAssociation : trackGUIDAssociations_)
        if(aTrackGUIDAssociation->GetGUID() == trackGUID)
            trackGUIDAssociation = aTrackGUIDAssociation;
    
    trackGUIDAssociation->ClearFXActionAddresses();
     
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, trackFXName, sizeof(trackFXName));
        string fxName = trackFXName;
     
        if(fxMaps_.count(trackFXName) > 0)
        {
            FXMap* map = fxMaps_[fxName];
            
            for(auto* surface : realSurfaces_)
            {
                string surfaceName = surface->GetName();
            
                DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), trackFXParameterGUID);
                string fxGUID = trackFXParameterGUID;
         
                for(auto map : map->GetMapEntries())
                {
                    if(map.surfaceName == surfaceName && map.paramName == CompressorMeter)
                    {
                        AddAction(trackGUID + fxGUID + map.surfaceName + map.paramName, new GainReductionMeter_Action(this, track, i));
                        trackGUIDAssociation->AddFXActionAddress(trackGUID + fxGUID + map.surfaceName + map.paramName);
                    }
                    else
                    {
                        for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                        {
                            DAW::TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                            string fxParamName = trackFXParameterName;
         
                            if(map.surfaceName == surfaceName && map.paramName == fxParamName)
                            {
                                AddAction(trackGUID + fxGUID + map.surfaceName + map.paramName, new TrackFX_Action(this, track, i, j));
                                trackGUIDAssociation->AddFXActionAddress(trackGUID + fxGUID + map.surfaceName + map.paramName);
                            }
                        }
                    }
                }
            }
        }
        else if(GetVSTMonitor() && GetManager()->GetLazyIsInitialized())
        {
            DAW::ShowConsoleMsg(("\n\n" + fxName + "\n").c_str());
     
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                string fxParamName = trackFXParameterName;
     
                DAW::ShowConsoleMsg((fxParamName + "\n").c_str());
            }
        }
    }
}

void LogicalSurface::InitializeFXMaps()
{
    for(auto* surface : realSurfaces_)
    {
        string surfaceName = surface->GetName();
    
        FXMap* fxMap = new FXMap("VST: UAD UA 1176LN Rev E (Universal Audio, Inc.)");
        
        fxMap->AddEntry(surfaceName, Threshold, "Input");
        fxMap->AddEntry(surfaceName, Character, "Output");
        fxMap->AddEntry(surfaceName, Drive, "Meter");
        fxMap->AddEntry(surfaceName, Attack, "Attack");
        fxMap->AddEntry(surfaceName, Release, "Release");
        fxMap->AddEntry(surfaceName, Ratio, "Ratio");
        fxMap->AddEntry(surfaceName, Compressor, "Bypass");
        fxMap->AddEntry(surfaceName, Parallel, "Wet");
        
        AddFXMap(fxMap);
        
        fxMap = new FXMap("VST: UAD Fairchild 660 (Universal Audio, Inc.)");
        
        fxMap->AddEntry(surfaceName, Threshold, "Thresh");
        fxMap->AddEntry(surfaceName, Character, "Output");
        fxMap->AddEntry(surfaceName, Drive, "Meter");
        fxMap->AddEntry(surfaceName, Attack, "Headroom");
        fxMap->AddEntry(surfaceName, Release, "Input");
        fxMap->AddEntry(surfaceName, Ratio, "Time Const");
        fxMap->AddEntry(surfaceName, Compressor, "Bypass");
        fxMap->AddEntry(surfaceName, Parallel, "Wet");
        
        AddFXMap(fxMap);
        
        fxMap = new FXMap("VST: UAD Teletronix LA-2A Silver (Universal Audio, Inc.)");
        
        fxMap->AddEntry(surfaceName, Threshold, "Peak Reduct");
        fxMap->AddEntry(surfaceName, Character, "Gain");
        fxMap->AddEntry(surfaceName, Drive, "Meter");
        fxMap->AddEntry(surfaceName, Attack, "Emphasis");
        fxMap->AddEntry(surfaceName, Ratio, "Comp/Limit");
        fxMap->AddEntry(surfaceName, Compressor, "Bypass");
        fxMap->AddEntry(surfaceName, Parallel, "Wet");
        
        AddFXMap(fxMap);
        
        fxMap = new FXMap("VST: UAD Harrison 32C (Universal Audio, Inc.)");
        
        fxMap->AddEntry(surfaceName, LoCurve, "LowPeak");
        //fxMap->AddEntry(HiCurve, "");
        fxMap->AddEntry(surfaceName, HiGain, "HiGain");
        fxMap->AddEntry(surfaceName, HiFrequency, "HiFreq");
        fxMap->AddEntry(surfaceName, HiMidGain, "HiMidGain");
        fxMap->AddEntry(surfaceName, HiMidFrequency, "HiMidFreq");
        fxMap->AddEntry(surfaceName, HiMidQ, "LowPass");
        fxMap->AddEntry(surfaceName, LoMidGain, "LoMidGain");
        fxMap->AddEntry(surfaceName, LoMidFrequency, "LoMidFreq");
        fxMap->AddEntry(surfaceName, LoMidQ, "HiPass");
        fxMap->AddEntry(surfaceName, LoGain, "LowGain");
        fxMap->AddEntry(surfaceName, LoFrequency, "LowFreq");
        fxMap->AddEntry(surfaceName, Equalizer, "Bypass");
        
        AddFXMap(fxMap);
        
        fxMap = new FXMap("VST: UAD Pultec EQP-1A (Universal Audio, Inc.)");
        
        //fxMap->AddEntry(LoCurve, "");
        //fxMap->AddEntry(HiCurve, "");
        fxMap->AddEntry(surfaceName, HiGain, "HF Atten");
        fxMap->AddEntry(surfaceName, HiFrequency, "HF Atten Freq");
        fxMap->AddEntry(surfaceName, HiMidGain, "HF Boost");
        fxMap->AddEntry(surfaceName, HiMidFrequency, "High Freq");
        fxMap->AddEntry(surfaceName, HiMidQ, "HF Q");
        fxMap->AddEntry(surfaceName, LoMidGain, "LF Atten");
        //fxMap->AddEntry(LoMidFrequency, "");
        //fxMap->AddEntry(LoMidQ, "");
        fxMap->AddEntry(surfaceName, LoGain, "LF Boost");
        fxMap->AddEntry(surfaceName, LoFrequency, "Low Freq");
        fxMap->AddEntry(surfaceName, Equalizer, "Bypass");
        
        AddFXMap(fxMap);
        
        fxMap = new FXMap("VST: UAD Pultec MEQ-5 (Universal Audio, Inc.)");
        
        //fxMap->AddEntry(LoCurve, "");
        //fxMap->AddEntry(HiCurve, "");
        fxMap->AddEntry(surfaceName, HiGain, "HM Peak");
        fxMap->AddEntry(surfaceName, HiFrequency, "HM Freq");
        fxMap->AddEntry(surfaceName, HiMidGain, "Mid Dip");
        fxMap->AddEntry(surfaceName, HiMidFrequency, "Mid Freq");
        //fxMap->AddEntry(HiMidQ, "");
        fxMap->AddEntry(surfaceName, LoMidGain, "LM Peak");
        fxMap->AddEntry(surfaceName, LoMidFrequency, "LM Freq");
        //fxMap->AddEntry(LoMidQ, "");
        //fxMap->AddEntry(LoGain, "");
        //fxMap->AddEntry(LoFrequency, "");
        fxMap->AddEntry(surfaceName, Equalizer, "Bypass");
        
        AddFXMap(fxMap);
    }
}

void LogicalSurface::BuildActions()
{
    actions_.clear();
    trackGUIDAssociations_.clear();
    trackGUIDs_.clear();
    
    for(int i = 0; i < DAW::GetNumTracks() + 1; ++i) // +1 is for ReaperMasterTrack
        trackGUIDs_.push_back(DAW::GetTrackGUIDAsString(i));
    
    for(auto * surface : realSurfaces_)
    {
        string surfaceName = surface->GetName();

        AddAction(LogicalControlSurface + surfaceName + ChannelLeft, new TrackBank_Action(this, -1));
        AddAction(LogicalControlSurface + surfaceName + ChannelRight, new TrackBank_Action(this, 1));
        AddAction(LogicalControlSurface + surfaceName + BankLeft, new TrackBank_Action(this, -8));
        AddAction(LogicalControlSurface + surfaceName + BankRight, new TrackBank_Action(this, 8));
        
        AddAction(LogicalControlSurface + surfaceName + NextMap, new NextMap_Action(this));
        AddAction(LogicalControlSurface + surfaceName + LockTracks, new ImmobilizeSelectedTracks_Action(this));
        AddAction(LogicalControlSurface + surfaceName + UnlockTracks, new MobilizeSelectedTracks_Action(this));
        
        AddAction(LogicalControlSurface + surfaceName + Shift, new Shift_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Option, new Option_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Control, new Control_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Alt, new Alt_Action(this));
        
        AddAction(LogicalControlSurface + surfaceName + Read, new TrackAutoMode_Action(this, 1));
        AddAction(LogicalControlSurface + surfaceName + Write, new TrackAutoMode_Action(this, 3));
        AddAction(LogicalControlSurface + surfaceName + Trim, new TrackAutoMode_Action(this, 0));
        AddAction(LogicalControlSurface + surfaceName + Touch, new TrackAutoMode_Action(this, 2));
        AddAction(LogicalControlSurface + surfaceName + Latch, new TrackAutoMode_Action(this, 4));
        AddAction(LogicalControlSurface + surfaceName + Group, new TrackAutoMode_Action(this, 5));
        
        AddAction(LogicalControlSurface + surfaceName + Shift + Read, new GlobalAutoMode_Action(this, 1));
        AddAction(LogicalControlSurface + surfaceName + Shift + Write, new GlobalAutoMode_Action(this, 3));
        AddAction(LogicalControlSurface + surfaceName + Shift + Trim, new GlobalAutoMode_Action(this, 0));
        AddAction(LogicalControlSurface + surfaceName + Shift + Touch, new GlobalAutoMode_Action(this, 2));
        AddAction(LogicalControlSurface + surfaceName + Shift + Latch, new GlobalAutoMode_Action(this, 4));
        AddAction(LogicalControlSurface + surfaceName + Shift + Group, new GlobalAutoMode_Action(this, 5));
        
        AddAction(LogicalControlSurface + surfaceName + Save, new Save_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Shift + Save, new SaveAs_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Undo, new Undo_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Shift + Undo, new Redo_Action(this));
        
        //logicalSurfaceInteractor_->AddAction(new Enter_Action(Enter, logicalSurfaceInteractor_));
        //logicalSurfaceInteractor_->AddAction(new Cancel_Action(Cancel, logicalSurfaceInteractor_));
        
        AddAction(LogicalControlSurface + surfaceName + Marker, new PreviousMarker_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Shift + Marker, new InsertMarker_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Option + Marker, new InsertMarkerRegion_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Nudge, new NextMarker_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Cycle, new CycleTimeline_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Click, new Metronome_Action(this));
        
        AddAction(LogicalControlSurface + surfaceName + Rewind, new Rewind_Action(this));
        AddAction(LogicalControlSurface + surfaceName + FastForward, new FastForward_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Stop, new Stop_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Play, new Play_Action(this));
        AddAction(LogicalControlSurface + surfaceName + Record, new Record_Action(this));
        
        // GAW TBD -- timers need to be cross platform
        
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Up,  logicalSurfaceInteractor_, 0, 0.3));
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Down,  logicalSurfaceInteractor_, 1, 0.3));
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Left,  logicalSurfaceInteractor_, 2, 0.3));
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Right, logicalSurfaceInteractor_, 3, 0.3));
        
        //logicalSurfaceInteractor_->AddAction(new LatchedZoom_Action(Zoom,  logicalSurfaceInteractor_));
        //logicalSurfaceInteractor_->AddAction(new LatchedScrub_Action(Scrub,  logicalSurfaceInteractor_));

        for(int i = 0; i < DAW::GetNumTracks() + 1; ++i) // +1 is for ReaperMasterTrack
        {
            string trackNumber(to_string(i));
            string trackGUID = DAW::GetTrackGUIDAsString(i);
            MediaTrack* track = DAW::CSurf_TrackFromID(i, false);
            
            AddAction(trackGUID + surfaceName + TrackDisplay + trackNumber, new TrackName_DisplayAction(this, track));
            AddAction(trackGUID + surfaceName + TrackTouched + trackNumber, new TouchStateControlled_Action(this, track, TrackDisplay + trackNumber, trackGUID + surfaceName + TrackDisplay + trackNumber, new TrackVolume_DisplayAction(this, track)));
            AddAction(trackGUID + surfaceName + Volume + trackNumber, new TrackVolume_Action(this, track));

            CycledAction* cyclicAction = new CycledAction(this);
            cyclicAction->Add(new TrackPan_Action(this, track, 0x00));
            cyclicAction->Add(new TrackPanWidth_Action(this, track, 0x30));
            AddAction(trackGUID + surfaceName + Pan + trackNumber, cyclicAction);

            AddAction(trackGUID + surfaceName + Select + trackNumber, new TrackUniqueSelect_Action(this, track));
            AddAction(trackGUID + surfaceName + Shift + Select + trackNumber, new TrackSelectionSelect_Action(this, track));
            AddAction(trackGUID + surfaceName + Control + Select + trackNumber, new TrackSelect_Action(this, track));
            AddAction(trackGUID + surfaceName + TrackOutMeterLeft + trackNumber, new VUMeter_Action(this, track, 0));
            AddAction(trackGUID + surfaceName + TrackOutMeterRight + trackNumber, new VUMeter_Action(this, track, 1));

            if(i == 0)
            {
                // The Mute, Solo, and RecArm switches have no meaning for Master, they can be used for something else
            }
            else
            {
                AddAction(trackGUID + surfaceName + RecordArm + trackNumber, new TrackRecordArm_Action(this, track));
                AddAction(trackGUID + surfaceName + Mute + trackNumber, new TrackMute_Action(this, track));
                AddAction(trackGUID + surfaceName + Solo + trackNumber, new TrackSolo_Action(this, track));
            }
            
            AddTrackGUIDAssociation(new TrackGUIDAssociation(trackGUID, this));
            MapFX(track);
        }
    }
    
    RefreshLayout();
}

void LogicalSurface::BuildCSurfWidgets()
{
    numLogicalChannels_ = 0;
    RealSurfaceChannel* channel = nullptr;

    for(auto & surface : realSurfaces_)
    {
        surface->ClearChannels();
        surface->ClearWidgets();
        
        if(surface->GetName() == "Console1")
        {
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
 
            surface->AddWidget(new PushButton_CSurfWidget(DisplayFX, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x66, 0x7f), new MIDI_event_ex_t(0xb0, 0x66, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(SendsMode, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x68, 0x7f), new MIDI_event_ex_t(0xb0, 0x68, 0x00)));
*/
            
            
            channel = new RealSurfaceChannel( "", surface, true);
            surface->AddChannel(channel);
            
            channel->AddWidget(new PushButton_MidiWidget("", surface, "Order",             new MIDI_event_ex_t(0xb0, 0x0e, 0x7f), new MIDI_event_ex_t(0xb0, 0x0e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "ExternalSidechain", new MIDI_event_ex_t(0xb0, 0x11, 0x7f), new MIDI_event_ex_t(0xb0, 0x11, 0x00)));

            // Input
            channel->AddWidget(new PushButton_MidiWidget("", surface, "FiltersToCompressor",   new MIDI_event_ex_t(0xb0, 0x3d, 0x7f), new MIDI_event_ex_t(0xb0, 0x3d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "PhaseInvert",           new MIDI_event_ex_t(0xb0, 0x6c, 0x7f), new MIDI_event_ex_t(0xb0, 0x6c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "Preset",                new MIDI_event_ex_t(0xb0, 0x3a, 0x7f), new MIDI_event_ex_t(0xb0, 0x3a, 0x00)));

            //channel->AddWidget(new Fader8Bit_CSurfWidget("InputGain", surface, channel,  new MIDI_event_ex_t(0xb0, 0x6b, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("HiCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x69, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("LoCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x67, 0x7f)));
            
            //channel->AddWidget(new VUMeter_CSurfWidget(TrackInMeterLeft, surface, channel, new  MIDI_event_ex_t(0xb0, 0x6e, 0x7f)));
            //channel->AddWidget(new VUMeter_CSurfWidget(TrackInMeterRight, surface, channel, new  MIDI_event_ex_t(0xb0, 0x6f, 0x7f)));

            // Shape
            channel->AddWidget(new PushButton_MidiWidget("", surface, "Shape",     new MIDI_event_ex_t(0xb0, 0x35, 0x7f), new MIDI_event_ex_t(0xb0, 0x35, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, "HardGate",  new MIDI_event_ex_t(0xb0, 0x3b, 0x7f), new MIDI_event_ex_t(0xb0, 0x3b, 0x00)));

            //channel->AddWidget(new Fader8Bit_CSurfWidget("Gate", surface, channel,           new MIDI_event_ex_t(0xb0, 0x36, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("GateRelease", surface, channel,    new MIDI_event_ex_t(0xb0, 0x38, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("Sustain", surface, channel,        new MIDI_event_ex_t(0xb0, 0x37, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("Punch", surface, channel,          new MIDI_event_ex_t(0xb0, 0x39, 0x7f)l));

            //channel->AddWidget(new VUMeter_CSurfWidget(GateMeter, surface, channel, new  MIDI_event_ex_t(0xb0, 0x72, 0x7f)));
            
            // EQ
            channel->AddWidget(new PushButton_MidiWidget("", surface, Equalizer,    new MIDI_event_ex_t(0xb0, 0x50, 0x7f), new MIDI_event_ex_t(0xb0, 0x50, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, LoCurve,      new MIDI_event_ex_t(0xb0, 0x5d, 0x7f), new MIDI_event_ex_t(0xb0, 0x5d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, HiCurve,      new MIDI_event_ex_t(0xb0, 0x41, 0x7f), new MIDI_event_ex_t(0xb0, 0x41, 0x00)));

            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, HiGain,       new MIDI_event_ex_t(0xb0, 0x52, 0x7f), new MIDI_event_ex_t(0xb0, 0x52, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, HiFrequency,  new MIDI_event_ex_t(0xb0, 0x53, 0x7f), new MIDI_event_ex_t(0xb0, 0x53, 0x00)));
            
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, HiMidGain,         new MIDI_event_ex_t(0xb0, 0x55, 0x7f), new MIDI_event_ex_t(0xb0, 0x55, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, HiMidFrequency,    new MIDI_event_ex_t(0xb0, 0x56, 0x7f), new MIDI_event_ex_t(0xb0, 0x56, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, HiMidQ,            new MIDI_event_ex_t(0xb0, 0x57, 0x7f), new MIDI_event_ex_t(0xb0, 0x57, 0x00)));
            
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, LoMidGain,         new MIDI_event_ex_t(0xb0, 0x58, 0x7f), new MIDI_event_ex_t(0xb0, 0x58, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, LoMidFrequency,    new MIDI_event_ex_t(0xb0, 0x59, 0x7f), new MIDI_event_ex_t(0xb0, 0x59, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, LoMidQ,            new MIDI_event_ex_t(0xb0, 0x5a, 0x7f), new MIDI_event_ex_t(0xb0, 0x5a, 0x00)));
            
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, LoGain,        new MIDI_event_ex_t(0xb0, 0x5b, 0x7f), new MIDI_event_ex_t(0xb0, 0x5b, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, LoFrequency,   new MIDI_event_ex_t(0xb0, 0x5c, 0x7f), new MIDI_event_ex_t(0xb0, 0x5c, 0x00)));

            // Compressor
            channel->AddWidget(new PushButton_MidiWidget("", surface, Compressor, 1,   new MIDI_event_ex_t(0xb0, 0x2e, 0x7f), new MIDI_event_ex_t(0xb0, 0x2e, 0x00)));

            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Threshold,        new MIDI_event_ex_t(0xb0, 0x2f, 0x7f), new MIDI_event_ex_t(0xb0, 0x2f, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Release,          new MIDI_event_ex_t(0xb0, 0x30, 0x7f), new MIDI_event_ex_t(0xb0, 0x30, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Ratio,            new MIDI_event_ex_t(0xb0, 0x31, 0x7f), new MIDI_event_ex_t(0xb0, 0x31, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Parallel,         new MIDI_event_ex_t(0xb0, 0x32, 0x7f), new MIDI_event_ex_t(0xb0, 0x32, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Attack,           new MIDI_event_ex_t(0xb0, 0x33, 0x7f), new MIDI_event_ex_t(0xb0, 0x33, 0x00)));
            
            channel->AddWidget(new VUMeter_MidiWidget("", surface, CompressorMeter, 0.0, -20.0, new  MIDI_event_ex_t(0xb0, 0x73, 0x7f), new  MIDI_event_ex_t(0xb0, 0x73, 0x00)));

            // Output but re-purposed for compressor

            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Drive,            new MIDI_event_ex_t(0xb0, 0x0f, 0x7f), new MIDI_event_ex_t(0xb0, 0x0f, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Character,        new MIDI_event_ex_t(0xb0, 0x12, 0x7f), new MIDI_event_ex_t(0xb0, 0x12, 0x00)));
            
            // Output
            channel->AddWidget(new PushButton_MidiWidget("", surface, Solo,       new MIDI_event_ex_t(0xb0, 0x0d, 0x7f), new MIDI_event_ex_t(0xb0, 0x0d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, Mute,       new MIDI_event_ex_t(0xb0, 0x0c, 0x7f), new MIDI_event_ex_t(0xb0, 0x0c, 0x00)));

            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Volume,      new MIDI_event_ex_t(0xb0, 0x07, 0x7f),  new MIDI_event_ex_t(0xb0, 0x07, 0x00)));
            channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Pan,         new MIDI_event_ex_t(0xb0, 0x0a, 0x7f), new MIDI_event_ex_t(0xb0, 0x0a, 0x00)));
            
            channel->AddWidget(new VUMeter_MidiWidget("", surface, TrackOutMeterLeft, -60.0, 6.0, new  MIDI_event_ex_t(0xb0, 0x70, 0x7f),     new MIDI_event_ex_t(0xb0, 0x70, 0x00)));
            channel->AddWidget(new VUMeter_MidiWidget("", surface, TrackOutMeterRight, -60.0, 6.0, new  MIDI_event_ex_t(0xb0, 0x71, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x71, 0x00)));
        }
        else
        {
            
            channel = new RealSurfaceChannel(LogicalControlSurface, surface, false);
            surface->AddChannel(channel);
            
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Track",       new MIDI_event_ex_t(0x90, 0x28, 0x7f), new MIDI_event_ex_t(0x90, 0x28, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Send",        new MIDI_event_ex_t(0x90, 0x29, 0x7f), new MIDI_event_ex_t(0x90, 0x29, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Pan",         new MIDI_event_ex_t(0x90, 0x2a, 0x7f), new MIDI_event_ex_t(0x90, 0x2a, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Plugin",      new MIDI_event_ex_t(0x90, 0x2b, 0x7f), new MIDI_event_ex_t(0x90, 0x2b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "EQ",          new MIDI_event_ex_t(0x90, 0x2c, 0x7f), new MIDI_event_ex_t(0x90, 0x2c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Instrument",  new MIDI_event_ex_t(0x90, 0x2d, 0x7f), new MIDI_event_ex_t(0x90, 0x2d, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "nameValue",   new MIDI_event_ex_t(0x90, 0x34, 0x7f), new MIDI_event_ex_t(0x90, 0x34, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "smpteBeats",  new MIDI_event_ex_t(0x90, 0x35, 0x7f), new MIDI_event_ex_t(0x90, 0x35, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, NextMap,       new MIDI_event_ex_t(0x90, 0x36, 0x7f), new MIDI_event_ex_t(0x90, 0x36, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "F2",          new MIDI_event_ex_t(0x90, 0x37, 0x7f), new MIDI_event_ex_t(0x90, 0x37, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "F3",          new MIDI_event_ex_t(0x90, 0x38, 0x7f), new MIDI_event_ex_t(0x90, 0x38, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "F4",          new MIDI_event_ex_t(0x90, 0x39, 0x7f), new MIDI_event_ex_t(0x90, 0x39, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "F5",          new MIDI_event_ex_t(0x90, 0x3a, 0x7f), new MIDI_event_ex_t(0x90, 0x3a, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "F6",          new MIDI_event_ex_t(0x90, 0x3b, 0x7f), new MIDI_event_ex_t(0x90, 0x3b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, UnlockTracks,  new MIDI_event_ex_t(0x90, 0x3c, 0x7f), new MIDI_event_ex_t(0x90, 0x3c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, LockTracks,    new MIDI_event_ex_t(0x90, 0x3d, 0x7f), new MIDI_event_ex_t(0x90, 0x3d, 0x00)));

            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalControlSurface, surface, Shift,     new MIDI_event_ex_t(0x90, 0x46, 0x7f), new MIDI_event_ex_t(0x90, 0x46, 0x00)));
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalControlSurface, surface, Option,    new MIDI_event_ex_t(0x90, 0x47, 0x7f), new MIDI_event_ex_t(0x90, 0x47, 0x00)));
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalControlSurface, surface, Control,   new MIDI_event_ex_t(0x90, 0x48, 0x7f), new MIDI_event_ex_t(0x90, 0x48, 0x00)));
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalControlSurface, surface, Alt,       new MIDI_event_ex_t(0x90, 0x49, 0x7f), new MIDI_event_ex_t(0x90, 0x49, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Read,          new MIDI_event_ex_t(0x90, 0x4a, 0x7f), new MIDI_event_ex_t(0x90, 0x4a, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Write,         new MIDI_event_ex_t(0x90, 0x4b, 0x7f), new MIDI_event_ex_t(0x90, 0x4b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Trim,          new MIDI_event_ex_t(0x90, 0x4c, 0x7f), new MIDI_event_ex_t(0x90, 0x4c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Touch,         new MIDI_event_ex_t(0x90, 0x4d, 0x7f), new MIDI_event_ex_t(0x90, 0x4d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Latch,         new MIDI_event_ex_t(0x90, 0x4e, 0x7f), new MIDI_event_ex_t(0x90, 0x4e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Group,         new MIDI_event_ex_t(0x90, 0x4f, 0x7f), new MIDI_event_ex_t(0x90, 0x4f, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Save,          new MIDI_event_ex_t(0x90, 0x50, 0x7f), new MIDI_event_ex_t(0x90, 0x50, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Undo,          new MIDI_event_ex_t(0x90, 0x51, 0x7f), new MIDI_event_ex_t(0x90, 0x51, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Cancel,        new MIDI_event_ex_t(0x90, 0x52, 0x7f), new MIDI_event_ex_t(0x90, 0x52, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Enter,         new MIDI_event_ex_t(0x90, 0x53, 0x7f), new MIDI_event_ex_t(0x90, 0x53, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Cycle,         new MIDI_event_ex_t(0x90, 0x56, 0x7f), new MIDI_event_ex_t(0x90, 0x56, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Drop",        new MIDI_event_ex_t(0x90, 0x57, 0x7f), new MIDI_event_ex_t(0x90, 0x57, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Replace",     new MIDI_event_ex_t(0x90, 0x58, 0x7f), new MIDI_event_ex_t(0x90, 0x58, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Click,         new MIDI_event_ex_t(0x90, 0x59, 0x7f), new MIDI_event_ex_t(0x90, 0x59, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, "Solo",        new MIDI_event_ex_t(0x90, 0x5a, 0x7f), new MIDI_event_ex_t(0x90, 0x5a, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Up,            new MIDI_event_ex_t(0x90, 0x60, 0x7f), new MIDI_event_ex_t(0x90, 0x60, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Down,          new MIDI_event_ex_t(0x90, 0x61, 0x7f), new MIDI_event_ex_t(0x90, 0x61, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Left,          new MIDI_event_ex_t(0x90, 0x62, 0x7f), new MIDI_event_ex_t(0x90, 0x62, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Right,         new MIDI_event_ex_t(0x90, 0x63, 0x7f), new MIDI_event_ex_t(0x90, 0x63, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Zoom,          new MIDI_event_ex_t(0x90, 0x64, 0x7f), new MIDI_event_ex_t(0x90, 0x64, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Scrub,         new MIDI_event_ex_t(0x90, 0x65, 0x7f), new MIDI_event_ex_t(0x90, 0x65, 0x00)));
 
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, BankLeft,      new MIDI_event_ex_t(0x90, 0x2e, 0x7f), new MIDI_event_ex_t(0x90, 0x2e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, BankRight,     new MIDI_event_ex_t(0x90, 0x2f, 0x7f), new MIDI_event_ex_t(0x90, 0x2f, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, ChannelLeft,   new MIDI_event_ex_t(0x90, 0x30, 0x7f), new MIDI_event_ex_t(0x90, 0x30, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, ChannelRight,  new MIDI_event_ex_t(0x90, 0x31, 0x7f), new MIDI_event_ex_t(0x90, 0x31, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Marker,        new MIDI_event_ex_t(0x90, 0x54, 0x7f), new MIDI_event_ex_t(0x90, 0x54, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Nudge,         new MIDI_event_ex_t(0x90, 0x55, 0x7f), new MIDI_event_ex_t(0x90, 0x55, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Rewind,        new MIDI_event_ex_t(0x90, 0x5b, 0x7f), new MIDI_event_ex_t(0x90, 0x5b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, FastForward,   new MIDI_event_ex_t(0x90, 0x5c, 0x7f), new MIDI_event_ex_t(0x90, 0x5c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Stop,          new MIDI_event_ex_t(0x90, 0x5d, 0x7f), new MIDI_event_ex_t(0x90, 0x5d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Play,          new MIDI_event_ex_t(0x90, 0x5e, 0x7f), new MIDI_event_ex_t(0x90, 0x5e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalControlSurface, surface, Record,        new MIDI_event_ex_t(0x90, 0x5f, 0x7f), new MIDI_event_ex_t(0x90, 0x5f, 0x00)));
            
            for(int i = 0; i < surface->GetNumBankableChannels(); ++i)
            {
                string trackGUID = DAW::GetTrackGUIDAsString(numLogicalChannels_++);
                string channelNumber = to_string(i);

                channel = new RealSurfaceChannel(trackGUID, surface, true);
                surface->AddChannel(channel);
                
                channel->AddWidget(new PushButtonWithRelease_MidiWidget(trackGUID, surface, TrackTouched + channelNumber, new MIDI_event_ex_t(0x90, 0x68 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x68 + i, 0x00)));
                
                channel->AddWidget(new EncoderCycledAction_MidiWidget(trackGUID, surface, Pan + channelNumber, new MIDI_event_ex_t(0xb0, 0x10 + i, 0x7f), new MIDI_event_ex_t(0xb0, 0x10 + i, 0x00), new MIDI_event_ex_t(0x90, 0x20 + i, 0x7f)));

                channel->AddWidget(new Display_MidiWidget(trackGUID, surface, TrackDisplay + channelNumber, i));
                channel->AddWidget(new Fader14Bit_MidiWidget(trackGUID, surface, Volume + channelNumber, -72.0, 12.0, new MIDI_event_ex_t(0xe0 + i, 0x7f, 0x7f), new MIDI_event_ex_t(0xe0 + i, 0x00, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, RecordArm + channelNumber,   new MIDI_event_ex_t(0x90, 0x00 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x00 + i, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, Solo + channelNumber,        new MIDI_event_ex_t(0x90, 0x08 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x08 + i, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, Mute + channelNumber,        new MIDI_event_ex_t(0x90, 0x10 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x10 + i, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, Select + channelNumber,      new MIDI_event_ex_t(0x90, 0x18 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x18 + i, 0x00)));
            }
        }
    }
}

void LogicalSurface::InitializeSurfaces()
{
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

        string bankGroup = "";
        
        if(name != string("Console1"))
            bankGroup = "Avid";
        
        AddRealSurface(new MidiCSurf(this, bankGroup, name, numBankableChannels, GetManager()->MidiManager()->GetMidiInputForChannel(channelIn), GetManager()->MidiManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
    }
    
    VSTMonitor_ = VSTMonitor;
    
    fclose ( filePtr );
}
