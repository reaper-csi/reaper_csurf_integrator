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
    GetSurface()->UpdateAction(GetGUID(), GetName());
}

void MidiWidget::ForceUpdate()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetSurface()->ForceUpdateAction(GetGUID(), GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealCSurf
////////////////////////////////////////////////////////////////////////////////////////////////////////
void RealCSurf::OnTrackSelection(MediaTrack *track)
{
    for(auto * channel : GetChannels())
        channel->OnTrackSelection(track);
}

// to Actions ->
double RealCSurf::GetCurrentNormalizedValue(string GUID, string widgetName)
{
    return GetLogicalSurface()->GetCurrentNormalizedValue(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealCSurf::UpdateAction(string GUID, string widgetName)
{
    GetLogicalSurface()->UpdateAction(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealCSurf::ForceUpdateAction(string GUID, string widgetName)
{
    GetLogicalSurface()->ForceUpdateAction(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealCSurf::CycleAction(string GUID, string widgetName)
{
    GetLogicalSurface()->CycleAction(ActionAddressFor(GUID, widgetName), GetName(), widgetName);
}

void RealCSurf::RunAction(string GUID, string widgetName, double value)
{
    GetLogicalSurface()->RunAction(ActionAddressFor(GUID, widgetName), value, GetName(), widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfChannel
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfChannel::OnTrackSelection(MediaTrack *track)
{
    if(DAW::CountSelectedTracks(nullptr) == 1)
    {
        DAW::SendMessage(WM_COMMAND, NamedCommandLookup("_S&M_WNCLS3"), 0);
        MapFX(track);
    }
}

void CSurfChannel::MapFX(MediaTrack *track)
{
    
    SetGUID(DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false)));
    
    char trackFXName[256];
    char trackFXGUID[256];
    char trackFXParamName[256];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, trackFXName, sizeof(trackFXName));
        string fxName(trackFXName);
        
        if(GetSurface()->GetLogicalSurface()->GetFXMaps().count(fxName) > 0)
        {
            DAW::TrackFX_Show(track, i, 3);
            
            FXMap* map = GetSurface()->GetLogicalSurface()->GetFXMaps()[fxName];
            
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), trackFXGUID);
            string fxGUID(trackFXGUID);
            
           /*
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, trackFXParamName, sizeof(trackFXParamName));
                string fxParamName(trackFXParamName);
                
                for(auto map : map->GetMapEntries())
                    if(map.paramName == fxParamName)
                        subChannel->AddWidgetName(map.widgetName);
            }
            
            AddSubChannel(subChannel);
            */
            
            
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// LogicalSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogicalSurface::Initialize()
{
    InitializeFXMaps();
    
    InitializeSurfaces();
    InitializeLogicalCSurfInteractor();
    BuildTrackActions();
    BuildCSurfWidgets();
}

// GAW TBD Temp BS for second map, just for illustrating map switching
void LogicalSurface::Initialize2()
{
    InitializeFXMaps();

    InitializeSurfaces();
    InitializeLogicalCSurfInteractor();
    BuildTrackActions2();
    BuildCSurfWidgets();
}

/*
        660
Meter
SC Filt
Bal
DC Thr
Mix
Power

        Harrison
 
CutEnable
EQEnable
Gain
Phase
Power
Bypass
Wet

VST: UAD Pultec EQP-1A (Universal Audio, Inc.)
Enable
Output
Bypass
Wet

VST: UAD Pultec MEQ-5 (Universal Audio, Inc.)
Enable
Output
Bypass
Wet
 
*/






void LogicalSurface::InitializeFXMaps()
{
    FXMap* fxMap = new FXMap("VST: UAD UA 1176LN Rev E (Universal Audio, Inc.)");
    
    fxMap->AddEntry(Threshold, "Input");
    fxMap->AddEntry(Character, "Output");
    fxMap->AddEntry(Drive, "Meter");
    fxMap->AddEntry(Attack, "Attack");
    fxMap->AddEntry(Release, "Release");
    fxMap->AddEntry(Ratio, "Ratio");
    fxMap->AddEntry(Compressor, "Bypass");
    fxMap->AddEntry(Parallel, "Wet");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap("VST: UAD Fairchild 660 (Universal Audio, Inc.)");
    
    fxMap->AddEntry(Threshold, "Thresh");
    fxMap->AddEntry(Character, "Output");
    fxMap->AddEntry(Drive, "Meter");
    fxMap->AddEntry(Attack, "Headroom");
    fxMap->AddEntry(Release, "Input");
    fxMap->AddEntry(Ratio, "Time Const");
    fxMap->AddEntry(Compressor, "Bypass");
    fxMap->AddEntry(Parallel, "Wet");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap("VST: UAD Teletronix LA-2A Silver (Universal Audio, Inc.)");
    
    fxMap->AddEntry(Threshold, "Peak Reduct");
    fxMap->AddEntry(Character, "Gain");
    fxMap->AddEntry(Drive, "Meter");
    fxMap->AddEntry(Attack, "Emphasis");
    fxMap->AddEntry(Ratio, "Comp/Limit");
    fxMap->AddEntry(Compressor, "Bypass");
    fxMap->AddEntry(Parallel, "Wet");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap("VST: UAD Harrison 32C (Universal Audio, Inc.)");
    
    fxMap->AddEntry(LoCurve, "LowPeak");
    //fxMap->AddEntry(HiCurve, "");
    fxMap->AddEntry(HiGain, "HiGain");
    fxMap->AddEntry(HiFrequency, "HiFreq");
    fxMap->AddEntry(HiMidGain, "HiMidGain");
    fxMap->AddEntry(HiMidFrequency, "HiMidFreq");
    fxMap->AddEntry(HiMidQ, "LowPass");
    fxMap->AddEntry(LoMidGain, "LoMidGain");
    fxMap->AddEntry(LoMidFrequency, "LoMidFreq");
    fxMap->AddEntry(LoMidQ, "HiPass");
    fxMap->AddEntry(LoGain, "LowGain");
    fxMap->AddEntry(LoFrequency, "LowFreq");
    fxMap->AddEntry(Equalizer, "Bypass");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap("VST: UAD Pultec EQP-1A (Universal Audio, Inc.)");
    
    //fxMap->AddEntry(LoCurve, "");
    //fxMap->AddEntry(HiCurve, "");
    fxMap->AddEntry(HiGain, "HF Atten");
    fxMap->AddEntry(HiFrequency, "HF Atten Freq");
    fxMap->AddEntry(HiMidGain, "HF Boost");
    fxMap->AddEntry(HiMidFrequency, "High Freq");
    fxMap->AddEntry(HiMidQ, "HF Q");
    fxMap->AddEntry(LoMidGain, "LF Atten");
    //fxMap->AddEntry(LoMidFrequency, "");
    //fxMap->AddEntry(LoMidQ, "");
    fxMap->AddEntry(LoGain, "LF Boost");
    fxMap->AddEntry(LoFrequency, "Low Freq");
    fxMap->AddEntry(Equalizer, "Bypass");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap("VST: UAD Pultec MEQ-5 (Universal Audio, Inc.)");
    
    //fxMap->AddEntry(LoCurve, "");
    //fxMap->AddEntry(HiCurve, "");
    fxMap->AddEntry(HiGain, "HM Peak");
    fxMap->AddEntry(HiFrequency, "HM Freq");
    fxMap->AddEntry(HiMidGain, "Mid Dip");
    fxMap->AddEntry(HiMidFrequency, "Mid Freq");
    //fxMap->AddEntry(HiMidQ, "");
    fxMap->AddEntry(LoMidGain, "LM Peak");
    fxMap->AddEntry(LoMidFrequency, "LM Freq");
    //fxMap->AddEntry(LoMidQ, "");
    //fxMap->AddEntry(LoGain, "");
    //fxMap->AddEntry(LoFrequency, "");
    fxMap->AddEntry(Equalizer, "Bypass");
    
    AddFXMap(fxMap);
    
}

void LogicalSurface::InitializeLogicalCSurfInteractor()
{
    for(auto * surface : surfaces_)
    {
        string surfaceName = surface->GetName();

        AddAction(LogicalCSurf + surfaceName + ChannelLeft, new TrackBank_Action(this, -1));
        AddAction(LogicalCSurf + surfaceName + ChannelRight, new TrackBank_Action(this, 1));
        AddAction(LogicalCSurf + surfaceName + BankLeft, new TrackBank_Action(this, -8));
        AddAction(LogicalCSurf + surfaceName + BankRight, new TrackBank_Action(this, 8));

        AddAction(LogicalCSurf + surfaceName + NextMap, new NextMap_Action(this));
        AddAction(LogicalCSurf + surfaceName + LockTracks, new ImmobilizeSelectedTracks_Action(this));
        AddAction(LogicalCSurf + surfaceName + UnlockTracks, new MobilizeSelectedTracks_Action(this));

        AddAction(LogicalCSurf + surfaceName + Shift, new Shift_Action(this));
        AddAction(LogicalCSurf + surfaceName + Option, new Option_Action(this));
        AddAction(LogicalCSurf + surfaceName + Control, new Control_Action(this));
        AddAction(LogicalCSurf + surfaceName + Alt, new Alt_Action(this));

        AddAction(LogicalCSurf + surfaceName + Read, new TrackAutoMode_Action(this, 1));
        AddAction(LogicalCSurf + surfaceName + Write, new TrackAutoMode_Action(this, 3));
        AddAction(LogicalCSurf + surfaceName + Trim, new TrackAutoMode_Action(this, 0));
        AddAction(LogicalCSurf + surfaceName + Touch, new TrackAutoMode_Action(this, 2));
        AddAction(LogicalCSurf + surfaceName + Latch, new TrackAutoMode_Action(this, 4));
        AddAction(LogicalCSurf + surfaceName + Group, new TrackAutoMode_Action(this, 5));
        
        AddAction(LogicalCSurf + surfaceName + Shift + Read, new GlobalAutoMode_Action(this, 1));
        AddAction(LogicalCSurf + surfaceName + Shift + Write, new GlobalAutoMode_Action(this, 3));
        AddAction(LogicalCSurf + surfaceName + Shift + Trim, new GlobalAutoMode_Action(this, 0));
        AddAction(LogicalCSurf + surfaceName + Shift + Touch, new GlobalAutoMode_Action(this, 2));
        AddAction(LogicalCSurf + surfaceName + Shift + Latch, new GlobalAutoMode_Action(this, 4));
        AddAction(LogicalCSurf + surfaceName + Shift + Group, new GlobalAutoMode_Action(this, 5));
      
        AddAction(LogicalCSurf + surfaceName + Save, new Save_Action(this));
        AddAction(LogicalCSurf + surfaceName + Shift + Save, new SaveAs_Action(this));
        AddAction(LogicalCSurf + surfaceName + Undo, new Undo_Action(this));
        AddAction(LogicalCSurf + surfaceName + Shift + Undo, new Redo_Action(this));
        
        //logicalSurfaceInteractor_->AddAction(new Enter_Action(Enter, logicalSurfaceInteractor_));
        //logicalSurfaceInteractor_->AddAction(new Cancel_Action(Cancel, logicalSurfaceInteractor_));

        AddAction(LogicalCSurf + surfaceName + Marker, new PreviousMarker_Action(this));
        AddAction(LogicalCSurf + surfaceName + Shift + Marker, new InsertMarker_Action(this));
        AddAction(LogicalCSurf + surfaceName + Option + Marker, new InsertMarkerRegion_Action(this));
        AddAction(LogicalCSurf + surfaceName + Nudge, new NextMarker_Action(this));
        AddAction(LogicalCSurf + surfaceName + Cycle, new CycleTimeline_Action(this));
        AddAction(LogicalCSurf + surfaceName + Click, new Metronome_Action(this));

        AddAction(LogicalCSurf + surfaceName + Rewind, new Rewind_Action(this));
        AddAction(LogicalCSurf + surfaceName + FastForward, new FastForward_Action(this));
        AddAction(LogicalCSurf + surfaceName + Stop, new Stop_Action(this));
        AddAction(LogicalCSurf + surfaceName + Play, new Play_Action(this));
        AddAction(LogicalCSurf + surfaceName + Record, new Record_Action(this));
        
        // GAW TBD -- timers need to be cross platform

        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Up,  logicalSurfaceInteractor_, 0, 0.3));
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Down,  logicalSurfaceInteractor_, 1, 0.3));
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Left,  logicalSurfaceInteractor_, 2, 0.3));
        //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Right, logicalSurfaceInteractor_, 3, 0.3));
        
        //logicalSurfaceInteractor_->AddAction(new LatchedZoom_Action(Zoom,  logicalSurfaceInteractor_));
        //logicalSurfaceInteractor_->AddAction(new LatchedScrub_Action(Scrub,  logicalSurfaceInteractor_));
    }
}

void LogicalSurface::BuildTrackActions()
{
    for(auto * surface : surfaces_)
    {
        string surfaceName = surface->GetName();
        
        for(int i = 0; i < DAW::GetNumTracks() + 1; ++i) // +1 is for ReaperMasterTrack
        {
            string trackNumber(to_string(i));
            string trackGUID = DAW::GetTrackGUIDAsString(i);
            MediaTrack* track = DAW::CSurf_TrackFromID(i, false);
        
            AddAction(trackGUID + surfaceName + TrackDisplay + trackNumber, new TrackName_DisplayAction(this, track));
            
            /*
            Action* faderTouchStateControlledAction = new TouchStateControlled_Action(TrackTouched, interactor, new TrackVolume_DisplayAction(TrackDisplay, interactor));
            interactor->AddAction(faderTouchStateControlledAction);
            interactor->AddAliasedAction(faderTouchStateControlledAction);
             */
            
            AddAction(trackGUID + surfaceName + Volume + trackNumber, new TrackName_DisplayAction(this, track));

            
            /*
            CycledAction* cyclicAction = new CycledAction(Pan, interactor);
            cyclicAction->Add(new TrackPan_Action(Pan, interactor, 0x00));
            cyclicAction->Add(new TrackPanWidth_Action(Pan, interactor, 0x30));
            interactor->AddAction(cyclicAction);
            */
   
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
            
            MapFX(track);
        }
    
    }
}

// GAW TBD Temp BS for second map, just for illustrating map switching
void LogicalSurface::BuildTrackActions2()
{
    for(auto * surface : surfaces_)
    {
        string surfaceName = surface->GetName();
        
        for(int i = 0; i < DAW::GetNumTracks() + 1; ++i) // +1 is for ReaperMasterTrack
        {
            string trackNumber(to_string(i));
            string trackGUID = DAW::GetTrackGUIDAsString(i);
            MediaTrack* track = DAW::CSurf_TrackFromID(i, false);
            
            AddAction(trackGUID + surfaceName + TrackDisplay + trackNumber, new TrackName_DisplayAction(this, track));
            
            /*
             Action* faderTouchStateControlledAction = new TouchStateControlled_Action(TrackTouched, interactor, new TrackVolume_DisplayAction(TrackDisplay, interactor));
             interactor->AddAction(faderTouchStateControlledAction);
             interactor->AddAliasedAction(faderTouchStateControlledAction);
             */
            
            AddAction(trackGUID + surfaceName + Volume + trackNumber, new TrackName_DisplayAction(this, track));
            
            
            /*
             CycledAction* cyclicAction = new CycledAction(Pan, interactor);
             cyclicAction->Add(new TrackPan_Action(Pan, interactor, 0x00));
             cyclicAction->Add(new TrackPanWidth_Action(Pan, interactor, 0x30));
             interactor->AddAction(cyclicAction);
             */
            
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
            
            MapFX(track);
        }
        
    }
}

void LogicalSurface::BuildCSurfWidgets()
{
    CSurfChannel* channel = nullptr;
    
    int currentChannel = 0;
    
    for(auto & surface : surfaces_)
    {
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
            
            
            channel = new CSurfChannel( "", surface, true);

            
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
            
            surface->AddChannel(channel);
        }
        else
        {
            
            channel = new CSurfChannel(LogicalCSurf, surface, false);

            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Track",       new MIDI_event_ex_t(0x90, 0x28, 0x7f), new MIDI_event_ex_t(0x90, 0x28, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Send",        new MIDI_event_ex_t(0x90, 0x29, 0x7f), new MIDI_event_ex_t(0x90, 0x29, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Pan",         new MIDI_event_ex_t(0x90, 0x2a, 0x7f), new MIDI_event_ex_t(0x90, 0x2a, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Plugin",      new MIDI_event_ex_t(0x90, 0x2b, 0x7f), new MIDI_event_ex_t(0x90, 0x2b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "EQ",          new MIDI_event_ex_t(0x90, 0x2c, 0x7f), new MIDI_event_ex_t(0x90, 0x2c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Instrument",  new MIDI_event_ex_t(0x90, 0x2d, 0x7f), new MIDI_event_ex_t(0x90, 0x2d, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "nameValue",   new MIDI_event_ex_t(0x90, 0x34, 0x7f), new MIDI_event_ex_t(0x90, 0x34, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "smpteBeats",  new MIDI_event_ex_t(0x90, 0x35, 0x7f), new MIDI_event_ex_t(0x90, 0x35, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, NextMap,       new MIDI_event_ex_t(0x90, 0x36, 0x7f), new MIDI_event_ex_t(0x90, 0x36, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "F2",          new MIDI_event_ex_t(0x90, 0x37, 0x7f), new MIDI_event_ex_t(0x90, 0x37, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "F3",          new MIDI_event_ex_t(0x90, 0x38, 0x7f), new MIDI_event_ex_t(0x90, 0x38, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "F4",          new MIDI_event_ex_t(0x90, 0x39, 0x7f), new MIDI_event_ex_t(0x90, 0x39, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "F5",          new MIDI_event_ex_t(0x90, 0x3a, 0x7f), new MIDI_event_ex_t(0x90, 0x3a, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "F6",          new MIDI_event_ex_t(0x90, 0x3b, 0x7f), new MIDI_event_ex_t(0x90, 0x3b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, UnlockTracks,  new MIDI_event_ex_t(0x90, 0x3c, 0x7f), new MIDI_event_ex_t(0x90, 0x3c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, LockTracks,    new MIDI_event_ex_t(0x90, 0x3d, 0x7f), new MIDI_event_ex_t(0x90, 0x3d, 0x00)));

            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalCSurf, surface, Shift,     new MIDI_event_ex_t(0x90, 0x46, 0x7f), new MIDI_event_ex_t(0x90, 0x46, 0x00)));
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalCSurf, surface, Option,    new MIDI_event_ex_t(0x90, 0x47, 0x7f), new MIDI_event_ex_t(0x90, 0x47, 0x00)));
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalCSurf, surface, Control,   new MIDI_event_ex_t(0x90, 0x48, 0x7f), new MIDI_event_ex_t(0x90, 0x48, 0x00)));
            channel->AddWidget(new PushButtonWithRelease_MidiWidget(LogicalCSurf, surface, Alt,       new MIDI_event_ex_t(0x90, 0x49, 0x7f), new MIDI_event_ex_t(0x90, 0x49, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Read,          new MIDI_event_ex_t(0x90, 0x4a, 0x7f), new MIDI_event_ex_t(0x90, 0x4a, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Write,         new MIDI_event_ex_t(0x90, 0x4b, 0x7f), new MIDI_event_ex_t(0x90, 0x4b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Trim,          new MIDI_event_ex_t(0x90, 0x4c, 0x7f), new MIDI_event_ex_t(0x90, 0x4c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Touch,         new MIDI_event_ex_t(0x90, 0x4d, 0x7f), new MIDI_event_ex_t(0x90, 0x4d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Latch,         new MIDI_event_ex_t(0x90, 0x4e, 0x7f), new MIDI_event_ex_t(0x90, 0x4e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Group,         new MIDI_event_ex_t(0x90, 0x4f, 0x7f), new MIDI_event_ex_t(0x90, 0x4f, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Save,          new MIDI_event_ex_t(0x90, 0x50, 0x7f), new MIDI_event_ex_t(0x90, 0x50, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Undo,          new MIDI_event_ex_t(0x90, 0x51, 0x7f), new MIDI_event_ex_t(0x90, 0x51, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Cancel,        new MIDI_event_ex_t(0x90, 0x52, 0x7f), new MIDI_event_ex_t(0x90, 0x52, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Enter,         new MIDI_event_ex_t(0x90, 0x53, 0x7f), new MIDI_event_ex_t(0x90, 0x53, 0x00)));
                
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Cycle,         new MIDI_event_ex_t(0x90, 0x56, 0x7f), new MIDI_event_ex_t(0x90, 0x56, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Drop",        new MIDI_event_ex_t(0x90, 0x57, 0x7f), new MIDI_event_ex_t(0x90, 0x57, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Replace",     new MIDI_event_ex_t(0x90, 0x58, 0x7f), new MIDI_event_ex_t(0x90, 0x58, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Click,         new MIDI_event_ex_t(0x90, 0x59, 0x7f), new MIDI_event_ex_t(0x90, 0x59, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, "Solo",        new MIDI_event_ex_t(0x90, 0x5a, 0x7f), new MIDI_event_ex_t(0x90, 0x5a, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Up,            new MIDI_event_ex_t(0x90, 0x60, 0x7f), new MIDI_event_ex_t(0x90, 0x60, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Down,          new MIDI_event_ex_t(0x90, 0x61, 0x7f), new MIDI_event_ex_t(0x90, 0x61, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Left,          new MIDI_event_ex_t(0x90, 0x62, 0x7f), new MIDI_event_ex_t(0x90, 0x62, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Right,         new MIDI_event_ex_t(0x90, 0x63, 0x7f), new MIDI_event_ex_t(0x90, 0x63, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Zoom,          new MIDI_event_ex_t(0x90, 0x64, 0x7f), new MIDI_event_ex_t(0x90, 0x64, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Scrub,         new MIDI_event_ex_t(0x90, 0x65, 0x7f), new MIDI_event_ex_t(0x90, 0x65, 0x00)));
 
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, BankLeft,      new MIDI_event_ex_t(0x90, 0x2e, 0x7f), new MIDI_event_ex_t(0x90, 0x2e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, BankRight,     new MIDI_event_ex_t(0x90, 0x2f, 0x7f), new MIDI_event_ex_t(0x90, 0x2f, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, ChannelLeft,   new MIDI_event_ex_t(0x90, 0x30, 0x7f), new MIDI_event_ex_t(0x90, 0x30, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, ChannelRight,  new MIDI_event_ex_t(0x90, 0x31, 0x7f), new MIDI_event_ex_t(0x90, 0x31, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Marker,        new MIDI_event_ex_t(0x90, 0x54, 0x7f), new MIDI_event_ex_t(0x90, 0x54, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Nudge,         new MIDI_event_ex_t(0x90, 0x55, 0x7f), new MIDI_event_ex_t(0x90, 0x55, 0x00)));

            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Rewind,        new MIDI_event_ex_t(0x90, 0x5b, 0x7f), new MIDI_event_ex_t(0x90, 0x5b, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, FastForward,   new MIDI_event_ex_t(0x90, 0x5c, 0x7f), new MIDI_event_ex_t(0x90, 0x5c, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Stop,          new MIDI_event_ex_t(0x90, 0x5d, 0x7f), new MIDI_event_ex_t(0x90, 0x5d, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Play,          new MIDI_event_ex_t(0x90, 0x5e, 0x7f), new MIDI_event_ex_t(0x90, 0x5e, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget(LogicalCSurf, surface, Record,        new MIDI_event_ex_t(0x90, 0x5f, 0x7f), new MIDI_event_ex_t(0x90, 0x5f, 0x00)));

            surface->AddChannel(channel);
            
            for(int i = 0; i < surface->GetNumBankableChannels(); ++i)
            {
                string trackGUID = DAW::GetTrackGUIDAsString(currentChannel++);
                string trackNumber(to_string(i));

                channel = new CSurfChannel(trackGUID, surface, false);
            
                channel->AddWidget(new Display_MidiWidget(trackGUID, surface, TrackDisplay + trackNumber, i));
            
                channel->AddWidget(new Fader14Bit_MidiWidget(trackGUID, surface, Volume + trackNumber, -72.0, 12.0, new MIDI_event_ex_t(0xe0 + i, 0x7f, 0x7f), new MIDI_event_ex_t(0xe0 + i, 0x00, 0x00)));
                channel->AddWidget(new PushButtonWithRelease_MidiWidget(trackGUID, surface, TrackTouched + trackNumber,        new MIDI_event_ex_t(0x90, 0x68 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x68 + i, 0x00)));
                channel->AddWidget(new EncoderCycledAction_MidiWidget(trackGUID, surface, Pan + trackNumber,        new MIDI_event_ex_t(0xb0, 0x10 + i, 0x7f), new MIDI_event_ex_t(0xb0, 0x10 + i, 0x00), new MIDI_event_ex_t(0x90, 0x20 + i, 0x7f)));

                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, RecordArm + trackNumber,   new MIDI_event_ex_t(0x90, 0x00 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x00 + i, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, Solo + trackNumber,        new MIDI_event_ex_t(0x90, 0x08 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x08 + i, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, Mute + trackNumber,        new MIDI_event_ex_t(0x90, 0x10 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x10 + i, 0x00)));
                channel->AddWidget(new PushButton_MidiWidget(trackGUID, surface, Select + trackNumber,      new MIDI_event_ex_t(0x90, 0x18 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x18 + i, 0x00)));
            
                surface->AddChannel(channel);
                numLogicalChannels_++;
            }
        }
    }
    
   numLogicalChannels_++; // add one for Master
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
        
        AddSurface(new MidiCSurf(this, bankGroup, name, numBankableChannels, GetManager()->MidiManager()->GetMidiInputForChannel(channelIn), GetManager()->MidiManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
    }
    
    VSTMonitor_ = VSTMonitor;
    
    fclose ( filePtr );
}

void LogicalSurface::SetShift(string surfaceName, bool value)
{
    for(auto* surface : surfaces_)
        surface->SetShift(value);
}

void LogicalSurface::SetOption(string surfaceName, bool value)
{
    for(auto* surface : surfaces_)
        surface->SetOption(value);
}

void LogicalSurface::SetControl(string surfaceName, bool value)
{
    for(auto* surface : surfaces_)
        surface->SetControl(value);
}

void LogicalSurface::SetAlt(string surfaceName, bool value)
{
    for(auto* surface : surfaces_)
        surface->SetAlt(value);
}

void LogicalSurface::SetZoom(string surfaceName, bool value)
{
    for(auto* surface : surfaces_)
        surface->SetZoom(value);
}

void LogicalSurface::SetScrub(string surfaceName, bool value)
{
    for(auto* surface : surfaces_)
        surface->SetScrub(value);
}

// to Widgets ->
void LogicalSurface::ForceUpdate()
{
    for(auto& surface : surfaces_)
        surface->ForceUpdateWidgets();
}

// to Actions ->
double LogicalSurface::GetCurrentNormalizedValue(string actionAddress, string surfaceName, string widgetName)
{
    /*
    if(logicalSurfaceInteractor_->GetGUID() == GUID)
        return logicalSurfaceInteractor_->GetCurrentNormalizedValue(name);
    else
        for(auto & interactor : trackInteractors_)
            if(interactor->GetGUID() == GUID)
                return interactor->GetCurrentNormalizedValue(name);
     */
    
    return 0.0;
}

void LogicalSurface::UpdateAction(string actionAddress, string surfaceName, string widgetName)
{
    /*
    if(logicalSurfaceInteractor_->GetGUID() == GUID)
        logicalSurfaceInteractor_->UpdateAction(surfaceName, name);
    else
        for(auto & interactor : trackInteractors_)
            if(interactor->GetGUID() == GUID)
                interactor->UpdateAction(surfaceName, name);
     */
}

void LogicalSurface::ForceUpdateAction(string actionAddress, string surfaceName, string widgetName)
{
    /*
    if(logicalSurfaceInteractor_->GetGUID() == GUID)
        logicalSurfaceInteractor_->ForceUpdateAction(surfaceName, name);
    else
        for(auto & interactor : trackInteractors_)
            if(interactor->GetGUID() == GUID)
                interactor->ForceUpdateAction(surfaceName, name);
     */
}

void LogicalSurface::CycleAction(string actionAddress, string surfaceName, string widgetName)
{
    /*
    if(logicalSurfaceInteractor_->GetGUID() == GUID)
        logicalSurfaceInteractor_->CycleAction(surfaceName, name);
    else
        for(auto & interactor : trackInteractors_)
            if(interactor->GetGUID() == GUID)
                interactor->CycleAction(surfaceName, name);
     */
}

void LogicalSurface::RunAction(string actionAddress, double value, string surfaceName, string widgetName)
{
    if(actions_.count(actionAddress) > 0)
        actions_[actionAddress]->Run(value, surfaceName, widgetName);
    /*
    if(logicalSurfaceInteractor_->GetGUID() == widgetName)
        logicalSurfaceInteractor_->RunAction(surfaceName, widgetName, value);
    else
        for(auto & interactor : trackInteractors_)
            if(interactor->GetGUID() == widgetName)
                interactor->RunAction(surfaceName, widgetName, value);
     */
}

// to Widgets ->
void LogicalSurface::SetWidgetValue(string surfaceName, string name, double value)
{
    //for(auto & surface : surfaces_)
        //surface->SetWidgetValue(GUID, name, value);
}

void LogicalSurface::SetWidgetValue(string surfaceName, string name, double value, int mode)
{
    //for(auto & surface : surfaces_)
        //surface->SetWidgetValue(GUID, name, value, mode);
}

void LogicalSurface::SetWidgetValue(string surfaceName, string name, string value)
{
    //for(auto & surface : surfaces_)
        //surface->SetWidgetValue(GUID, name, value);
}

void LogicalSurface::AdjustTrackBank(int stride)
{
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ < 1 - GetNumLogicalChannels())
        trackOffset_ = 1 - GetNumLogicalChannels();
    
    if(trackOffset_ > DAW::GetNumTracks() - 1)
        trackOffset_ = DAW::GetNumTracks() - 1;
    
    if(trackOffset_ != previousTrackOffset)
        RefreshLayout();
}

void LogicalSurface::ImmobilizeSelectedTracks()
{
    for(auto * surface : surfaces_)
        for(auto * channel : surface->GetChannels())
            if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(channel->GetGUID()), "I_SELECTED"))
                channel->SetIsMovable(false);
}

void LogicalSurface::MobilizeSelectedTracks()
{
    for(auto * surface : surfaces_)
        for(auto * channel : surface->GetChannels())
            if(DAW::GetMediaTrackInfo_Value(DAW::GetTrackFromGUID(channel->GetGUID()), "I_SELECTED"))
                channel->SetIsMovable(true);
}

void LogicalSurface::RefreshLayout()
{
    auto currentOffset = trackOffset_;

    vector<string> immovableTracks;
    
    for(auto* surface : surfaces_)
        for(auto* channel : surface->GetChannels())
            if(channel->GetIsMovable() == false)
                immovableTracks.push_back(channel->GetGUID());
    
    vector<string> movableTracks;
    
    for(int i = 0; i < GetNumLogicalChannels(); i++)
    {
        if(currentOffset < 0)
        {
            movableTracks.push_back("");
            currentOffset++;
        }
        else if(currentOffset >= DAW::GetNumTracks())
        {
            movableTracks.push_back("");
        }
        else if(find(immovableTracks.begin(), immovableTracks.end(), DAW::GetTrackGUIDAsString(currentOffset)) == immovableTracks.end())
        {
            movableTracks.push_back(DAW::GetTrackGUIDAsString(currentOffset++));
        }
        else
        {
            currentOffset++;
        }
    }
    
    currentOffset = 0;
    
    for(auto* surface : surfaces_)
        for(auto* channel : surface->GetChannels())
            if(channel->GetIsMovable() == true)
                channel->SetGUID(movableTracks[currentOffset++]);
    
    ForceUpdate();
}

bool LogicalSurface::DidTrackListChange()
{
    /*
    if(trackInteractors_.size() == 0)
        return false;               // We have no idea if track list changed, we have been called way too early, there's nothing to compare, just return false
    
    if(trackInteractors_.size() != DAW::GetNumTracks() + 1) // +1 is for Master
        return true; // list sizes disagree
    
    for(int i = 0; i < trackInteractors_.size(); i++)                    
        if(trackInteractors_[i]->GetGUID() != DAW::GetTrackGUIDAsString(i))
            return true;
    */
    return false;
}

void LogicalSurface::TrackFXListChanged(MediaTrack* track)
{
    MapFX(track);
}

void LogicalSurface::MapFX(MediaTrack* track)
{
    char trackFXName[256];
    char trackFXParameterName[256];
    char trackFXParameterGUID[256];
    
    string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
    
    /*
    
    // We will always find the interactor for this track, otherwise how could we add FX to it ?
    Interactor* interactor = nullptr;
    
    for(auto * anInteractor : trackInteractors_)
        if(anInteractor->GetGUID() == trackGUID)
            interactor = anInteractor;

    // Dump any existing FX subInteractors
    interactor->ClearFXSubInteractors();
 
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        // GAW TBD -- how do we know we should do this ?
        //AddAction(trackGUID + surfaceName + CompressorMeter + trackNumber, new GainReductionMeter_Action(this, track, i));

        
        
        
        DAW::TrackFX_GetFXName(track, i, trackFXName, sizeof(trackFXName));
        string fxName(trackFXName);
        
        if(fxMaps_.count(trackFXName) > 0)
        {
            FXMap* map = fxMaps_[fxName];
            
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), trackFXParameterGUID);
            string fxGUID(trackFXParameterGUID);

            SubInteractor* subInteractor = new SubInteractor(fxGUID, i, interactor);

            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                string fxParamName(trackFXParameterName);
                
                for(auto map : map->GetMapEntries())
                    if(map.paramName == fxParamName)
                        subInteractor->AddAction(new TrackFX_Action(map.widgetName, subInteractor, map.paramName, j));
            }
            
            interactor->AddFXSubInteractor(subInteractor);
        }
        else if(GetVSTMonitor() && GetManager()->GetLazyInitialized())
        {
            DAW::ShowConsoleMsg(("\n\n" + fxName + "\n").c_str());

            int numParameters = DAW::TrackFX_GetNumParams(track, i);

            for(int j = 0; j < numParameters; j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                string fxParamName(trackFXParameterName);
                
                DAW::ShowConsoleMsg((fxParamName + "\n").c_str());
            }
        }
    }
     */
}






