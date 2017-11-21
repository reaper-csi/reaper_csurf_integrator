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

const string Flip = "Flip";

const string TrackDisplay = "TrackDisplay";
const string TrackTouched = "TrackTouched";
const string TrackSolo = "TrackSolo";
const string TrackRecordArm = "TrackRecordArm";
const string TrackSelect = "TrackSelect";
const string TrackMute = "TrackMute";

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
// Action
////////////////////////////////////////////////////////////////////////////////////////////////////////

DAW* Action::GetDAW()
{
    return GetManager()->GetDAW();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////
string CSurfWidget::GetName()
{
    return name_;
}

void CSurfWidget::Update()
{
    GetSurface()->GetManager()->Update(GetGUID(), GetName());
}

void CSurfWidget::ForceUpdate()
{
    GetSurface()->GetManager()->ForceUpdate(GetGUID(), GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// LogicalCSurf
////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogicalSurface::Initialize()
{
    InitializeFXMaps();
    
    InitializeSurfaces();
    InitializeActions();
    BuildInteractors();
    BuildCSurfWidgets();
}

// GAW TBD Temp BS for second map, just for illustrating map switching
void LogicalSurface::Initialize2()
{
    InitializeFXMaps();

    InitializeSurfaces();
    InitializeActions();
    BuildInteractors2();
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

void LogicalSurface::InitializeActions()
{
    AddAction(new TrackBank_Action(ChannelLeft, GetManager(), this, -1));
    AddAction(new TrackBank_Action(ChannelRight, GetManager(), this, 1));
    AddAction(new TrackBank_Action(BankLeft, GetManager(), this, -20));
    AddAction(new TrackBank_Action(BankRight, GetManager(), this, 20));
    
    AddAction(new Flip_Action(Flip, GetManager(), this));
    
    AddAction(new NextMap_Action(NextMap, GetManager(), this));
    AddAction(new ImmobilizeSelectedTracks_Action(LockTracks, GetManager(), this));
    AddAction(new MobilizeSelectedTracks_Action(UnlockTracks, GetManager(), this));
    
    AddAction(new Shift_Action(Shift, GetManager(), this));
    AddAction(new Option_Action(Option, GetManager(), this));
    AddAction(new Control_Action(Control, GetManager(), this));
    AddAction(new Alt_Action(Alt, GetManager(), this));
    
    AddAction(new TrackAutoMode_Action(Read, GetManager(), this, 1));
    AddAction(new TrackAutoMode_Action(Write, GetManager(), this, 3));
    AddAction(new TrackAutoMode_Action(Trim, GetManager(), this, 0));
    AddAction(new TrackAutoMode_Action(Touch, GetManager(), this, 2));
    AddAction(new TrackAutoMode_Action(Latch, GetManager(), this, 4));
    AddAction(new TrackAutoMode_Action(Group, GetManager(), this, 5));

    AddAction(new GlobalAutoMode_Action(Shift + Delimiter + Read, GetManager(), this, 1));
    AddAction(new GlobalAutoMode_Action(Shift + Delimiter + Write, GetManager(), this, 3));
    AddAction(new GlobalAutoMode_Action(Shift + Delimiter + Trim, GetManager(), this, 0));
    AddAction(new GlobalAutoMode_Action(Shift + Delimiter + Touch, GetManager(), this, 2));
    AddAction(new GlobalAutoMode_Action(Shift + Delimiter + Latch, GetManager(), this, 4));
    AddAction(new GlobalAutoMode_Action(Shift + Delimiter + Group, GetManager(), this, 5));
    
    AddAction(new Save_Action(Save, GetManager(), this));
    AddAction(new SaveAs_Action( Shift + Delimiter + Save, GetManager(), this));
    AddAction(new Undo_Action(Undo, GetManager(), this));
    AddAction(new Redo_Action(Shift + Delimiter + Undo, GetManager(), this));
    //AddAction(new Enter_Action(Enter, Manager(), this));
    //AddAction(new Cancel_Action(Cancel, Manager(), this));

    AddAction(new PreviousMarker_Action(Marker, GetManager(), this));
    AddAction(new InsertMarker_Action(Shift + Delimiter + Marker, GetManager(), this));
    AddAction(new InsertMarkerRegion_Action(Option + Delimiter + Marker, GetManager(), this));
    AddAction(new NextMarker_Action(Nudge, GetManager(), this));
    AddAction(new CycleTimeline_Action(Cycle, GetManager(), this));
    AddAction(new Metronome_Action(Click, GetManager(), this));

    AddAction(new Rewind_Action(Rewind, GetManager(), this));
    AddAction(new FastForward_Action(FastForward, GetManager(), this));
    AddAction(new Stop_Action(Stop, GetManager(), this));
    AddAction(new Play_Action(Play, GetManager(), this));
    AddAction(new Record_Action(Record, GetManager(), this));
    
    AddAction(new RepeatingArrow_Action(Up,  GetManager(), this, 0, 0.3));
    AddAction(new RepeatingArrow_Action(Down,  GetManager(), this, 1, 0.3));
    AddAction(new RepeatingArrow_Action(Left,  GetManager(), this, 2, 0.3));
    AddAction(new RepeatingArrow_Action(Right,  GetManager(), this, 3, 0.3));
    AddAction(new LatchedZoom_Action(Zoom,  GetManager(), this));
    AddAction(new LatchedScrub_Action(Scrub,  GetManager(), this));    
}

void LogicalSurface::BuildInteractors()
{
    Interactor* interactor = nullptr;
    
    for(int i = 0; i < GetDAW()->GetNumTracks() + 1; ++i) // +1 is for ReaperMasterTrack
    {
        interactor = new Interactor(GetManager(), GetDAW()->GetTrackGUIDAsString(i));
        
        interactor->AddAction(new TrackName_DisplayAction(TrackDisplay, GetManager(), interactor));
        
        Action* faderTouchStateControlledAction = new TouchStateControlled_Action(TrackTouched, GetManager(), interactor, new TrackVolume_DisplayAction(TrackDisplay, GetManager(), interactor));
        interactor->AddAction(faderTouchStateControlledAction);
        interactor->AddAliasedAction(faderTouchStateControlledAction);

        interactor->AddAction(new TrackVolume_Action(TrackVolume, GetManager(), interactor));
        
        Interactor_CycleAction* cycleAction = new Interactor_CycleAction(TrackPan, GetManager(), interactor);
        cycleAction->AddAction(new TrackPan_Action(TrackPan, GetManager(), interactor, 0x00));
        cycleAction->AddAction(new TrackPanWidth_Action(TrackPan, GetManager(), interactor, 0x30));
        interactor->AddAction(cycleAction);
        
        interactor->AddAction(new TrackUniqueSelect_Action(TrackSelect, GetManager(), interactor));
        interactor->AddAction(new TrackSelectionSelect_Action(Shift + Delimiter  + TrackSelect, GetManager(), interactor));
        interactor->AddAction(new TrackSelect_Action(Control + Delimiter  + TrackSelect, GetManager(), interactor));
        
        interactor->AddAction(new VUMeter_Action(TrackOutMeterLeft, GetManager(), interactor, 0));
        interactor->AddAction(new VUMeter_Action(TrackOutMeterRight, GetManager(), interactor, 1));
        
        interactor->AddAction(new GainReductionMeter_Action(CompressorMeter, GetManager(), interactor));

        if(i == 0)
        {
            // The Mute, Solo, and RecArm switches have no meaning for Master, they can be used for something else
        }
        else
        {
            interactor->AddAction(new TrackRecordArm_Action(TrackRecordArm, GetManager(), interactor));
            interactor->AddAction(new TrackMute_Action(TrackMute, GetManager(), interactor));
            interactor->AddAction(new TrackSolo_Action(TrackSolo, GetManager(), interactor));
        }
        
        AddInteractor(interactor);
    }
}

// GAW TBD Temp BS for second map, just for illustrating map switching
void LogicalSurface::BuildInteractors2()
{
    Interactor* interactor = nullptr;
    
    for(int i = 0; i < GetManager()->GetDAW()->GetNumTracks() + 1; ++i) // +1 is for ReaperMasterTrack
    {
        interactor = new Interactor(GetManager(), GetManager()->GetDAW()->GetTrackGUIDAsString(i));
        
        interactor->AddAction(new TrackName_DisplayAction(TrackDisplay, GetManager(), interactor));

        Action* faderTouchStateControlledAction = new TouchStateControlled_Action(TrackTouched, GetManager(), interactor, new TrackVolume_DisplayAction(TrackDisplay, GetManager(), interactor));
        interactor->AddAction(faderTouchStateControlledAction);
        interactor->AddAliasedAction(faderTouchStateControlledAction);
        
        interactor->AddAction(new TrackVolume_Action(TrackVolume, GetManager(), interactor));
        
        Interactor_CycleAction* cycleAction = new Interactor_CycleAction(TrackPan, GetManager(), interactor);
        cycleAction->AddAction(new TrackPan_Action(TrackPan, GetManager(), interactor, 0x00));
        cycleAction->AddAction(new TrackPanWidth_Action(TrackPan, GetManager(), interactor, 0x30));
        interactor->AddAction(cycleAction);
        
        interactor->AddAction(new TrackSelect_Action(TrackSelect, GetManager(), interactor));

        if(i == 0)
        {
            // The Mute, Solo, and RecArm switches have no meaning for Master, they can be used for something else
        }
        else
        {
            // For this map Record Arm is disabled
            // interactor->AddAction(new TrackRecordArm_Action(TrackRecordArm, GetManager(), interactor));

            
            interactor->AddAction(new TrackMute_Action(TrackMute, GetManager(), interactor));
            interactor->AddAction(new TrackSolo_Action(TrackSolo, GetManager(), interactor));
        }
        
        AddInteractor(interactor);
    }
}

void LogicalSurface::BuildCSurfWidgets()
{
    CSurfChannel* channel = nullptr;
    
    int currentChannel = 0;
    
    int totalNumChannels = 0;
    
    for(auto & surface : surfaces_)
    {
        totalNumChannels += surface->GetNumChannels();
        
        if(surface->GetName() == "Console1")
        {
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

     
            channel = new UniquelySelectedCSurfChannel(surface, 0, "");

            surface->AddWidget(new PushButton_CSurfWidget(DisplayFX, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x66, 0x7f), new MIDI_event_ex_t(0xb0, 0x66, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(SendsMode, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x68, 0x7f), new MIDI_event_ex_t(0xb0, 0x68, 0x00)));

            
            channel->AddWidget(new PushButton_CSurfWidget("Order", surface, channel, "",             new MIDI_event_ex_t(0xb0, 0x0e, 0x7f), new MIDI_event_ex_t(0xb0, 0x0e, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget("ExternalSidechain", surface, channel, "", new MIDI_event_ex_t(0xb0, 0x11, 0x7f), new MIDI_event_ex_t(0xb0, 0x11, 0x00)));

            // Input
            channel->AddWidget(new PushButton_CSurfWidget("FiltersToCompressor", surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x3d, 0x7f), new MIDI_event_ex_t(0xb0, 0x3d, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget("PhaseInvert", surface, channel, "",           new MIDI_event_ex_t(0xb0, 0x6c, 0x7f), new MIDI_event_ex_t(0xb0, 0x6c, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget("Preset", surface, channel, "",                new MIDI_event_ex_t(0xb0, 0x3a, 0x7f), new MIDI_event_ex_t(0xb0, 0x3a, 0x00)));

            //channel->AddWidget(new Fader8Bit_CSurfWidget("InputGain", surface, channel,  new MIDI_event_ex_t(0xb0, 0x6b, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("HiCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x69, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("LoCut", surface, channel,      new MIDI_event_ex_t(0xb0, 0x67, 0x7f)));
            
            //channel->AddWidget(new VUMeter_CSurfWidget(TrackInMeterLeft, surface, channel, new  MIDI_event_ex_t(0xb0, 0x6e, 0x7f)));
            //channel->AddWidget(new VUMeter_CSurfWidget(TrackInMeterRight, surface, channel, new  MIDI_event_ex_t(0xb0, 0x6f, 0x7f)));

            // Shape
            channel->AddWidget(new PushButton_CSurfWidget("Shape", surface, channel, "",     new MIDI_event_ex_t(0xb0, 0x35, 0x7f), new MIDI_event_ex_t(0xb0, 0x35, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget("HardGate", surface, channel, "",  new MIDI_event_ex_t(0xb0, 0x3b, 0x7f), new MIDI_event_ex_t(0xb0, 0x3b, 0x00)));

            //channel->AddWidget(new Fader8Bit_CSurfWidget("Gate", surface, channel,           new MIDI_event_ex_t(0xb0, 0x36, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("GateRelease", surface, channel,    new MIDI_event_ex_t(0xb0, 0x38, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("Sustain", surface, channel,        new MIDI_event_ex_t(0xb0, 0x37, 0x7f)));
            //channel->AddWidget(new Fader8Bit_CSurfWidget("Punch", surface, channel,          new MIDI_event_ex_t(0xb0, 0x39, 0x7f)l));

            //channel->AddWidget(new VUMeter_CSurfWidget(GateMeter, surface, channel, new  MIDI_event_ex_t(0xb0, 0x72, 0x7f)));
            
            // EQ
            channel->AddWidget(new PushButton_CSurfWidget(Equalizer, surface, channel, "",  new MIDI_event_ex_t(0xb0, 0x50, 0x7f), new MIDI_event_ex_t(0xb0, 0x50, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget(LoCurve, surface, channel, "",    new MIDI_event_ex_t(0xb0, 0x5d, 0x7f), new MIDI_event_ex_t(0xb0, 0x5d, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget(HiCurve, surface, channel, "",    new MIDI_event_ex_t(0xb0, 0x41, 0x7f), new MIDI_event_ex_t(0xb0, 0x41, 0x00)));

            channel->AddWidget(new Fader8Bit_CSurfWidget(HiGain, surface, channel, "",         new MIDI_event_ex_t(0xb0, 0x52, 0x7f), new MIDI_event_ex_t(0xb0, 0x52, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(HiFrequency, surface, channel, "",    new MIDI_event_ex_t(0xb0, 0x53, 0x7f), new MIDI_event_ex_t(0xb0, 0x53, 0x00)));
            
            channel->AddWidget(new Fader8Bit_CSurfWidget(HiMidGain, surface, channel, "",      new MIDI_event_ex_t(0xb0, 0x55, 0x7f), new MIDI_event_ex_t(0xb0, 0x55, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(HiMidFrequency, surface, channel, "", new MIDI_event_ex_t(0xb0, 0x56, 0x7f), new MIDI_event_ex_t(0xb0, 0x56, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(HiMidQ, surface, channel, "",         new MIDI_event_ex_t(0xb0, 0x57, 0x7f), new MIDI_event_ex_t(0xb0, 0x57, 0x00)));
            
            channel->AddWidget(new Fader8Bit_CSurfWidget(LoMidGain, surface, channel, "",      new MIDI_event_ex_t(0xb0, 0x58, 0x7f), new MIDI_event_ex_t(0xb0, 0x58, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(LoMidFrequency, surface, channel, "", new MIDI_event_ex_t(0xb0, 0x59, 0x7f), new MIDI_event_ex_t(0xb0, 0x59, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(LoMidQ, surface, channel, "",         new MIDI_event_ex_t(0xb0, 0x5a, 0x7f), new MIDI_event_ex_t(0xb0, 0x5a, 0x00)));
            
            channel->AddWidget(new Fader8Bit_CSurfWidget(LoGain, surface, channel, "",        new MIDI_event_ex_t(0xb0, 0x5b, 0x7f), new MIDI_event_ex_t(0xb0, 0x5b, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(LoFrequency, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x5c, 0x7f), new MIDI_event_ex_t(0xb0, 0x5c, 0x00)));

            // Compressor
            channel->AddWidget(new PushButton_CSurfWidget(Compressor, surface, channel, "", 1, new MIDI_event_ex_t(0xb0, 0x2e, 0x7f), new MIDI_event_ex_t(0xb0, 0x2e, 0x00)));

            channel->AddWidget(new Fader8Bit_CSurfWidget(Threshold, surface, channel, "",  new MIDI_event_ex_t(0xb0, 0x2f, 0x7f), new MIDI_event_ex_t(0xb0, 0x2f, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(Release, surface, channel, "",    new MIDI_event_ex_t(0xb0, 0x30, 0x7f), new MIDI_event_ex_t(0xb0, 0x30, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(Ratio, surface, channel, "",      new MIDI_event_ex_t(0xb0, 0x31, 0x7f), new MIDI_event_ex_t(0xb0, 0x31, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(Parallel, surface, channel, "",   new MIDI_event_ex_t(0xb0, 0x32, 0x7f), new MIDI_event_ex_t(0xb0, 0x32, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(Attack, surface, channel, "",     new MIDI_event_ex_t(0xb0, 0x33, 0x7f), new MIDI_event_ex_t(0xb0, 0x33, 0x00)));
            
            channel->AddWidget(new VUMeter_CSurfWidget(CompressorMeter, surface, channel, "", 0.0, -20.0, new  MIDI_event_ex_t(0xb0, 0x73, 0x7f), new  MIDI_event_ex_t(0xb0, 0x73, 0x00)));

            // Output but re-purposed for compressor

            channel->AddWidget(new Fader8Bit_CSurfWidget(Drive, surface, channel, "",     new MIDI_event_ex_t(0xb0, 0x0f, 0x7f), new MIDI_event_ex_t(0xb0, 0x0f, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(Character, surface, channel, "", new MIDI_event_ex_t(0xb0, 0x12, 0x7f), new MIDI_event_ex_t(0xb0, 0x12, 0x00)));
            
            // Output
            channel->AddWidget(new PushButton_CSurfWidget(TrackSolo, surface, channel, "",    new MIDI_event_ex_t(0xb0, 0x0d, 0x7f), new MIDI_event_ex_t(0xb0, 0x0d, 0x00)));
            channel->AddWidget(new PushButton_CSurfWidget(TrackMute, surface, channel, "",    new MIDI_event_ex_t(0xb0, 0x0c, 0x7f), new MIDI_event_ex_t(0xb0, 0x0c, 0x00)));

            channel->AddWidget(new Fader8Bit_CSurfWidget(TrackVolume, surface, channel, "",  new MIDI_event_ex_t(0xb0, 0x07, 0x7f),  new MIDI_event_ex_t(0xb0, 0x07, 0x00)));
            channel->AddWidget(new Fader8Bit_CSurfWidget(TrackPan, surface, channel, "",     new MIDI_event_ex_t(0xb0, 0x0a, 0x7f), new MIDI_event_ex_t(0xb0, 0x0a, 0x00)));
            
            channel->AddWidget(new VUMeter_CSurfWidget(TrackOutMeterLeft, surface, channel, "", -60.0, 6.0, new  MIDI_event_ex_t(0xb0, 0x70, 0x7f),     new MIDI_event_ex_t(0xb0, 0x70, 0x00)));
            channel->AddWidget(new VUMeter_CSurfWidget(TrackOutMeterRight, surface, channel, "", -60.0, 6.0, new  MIDI_event_ex_t(0xb0, 0x71, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x71, 0x00)));
            
            surface->AddChannel(channel);
        }
        else
        {
            surface->AddWidget(new PushButton_CSurfWidget("Track", surface,      new MIDI_event_ex_t(0x90, 0x28, 0x7f), new MIDI_event_ex_t(0x90, 0x28, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Send", surface,       new MIDI_event_ex_t(0x90, 0x29, 0x7f), new MIDI_event_ex_t(0x90, 0x29, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Pan", surface,        new MIDI_event_ex_t(0x90, 0x2a, 0x7f), new MIDI_event_ex_t(0x90, 0x2a, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Plugin", surface,     new MIDI_event_ex_t(0x90, 0x2b, 0x7f), new MIDI_event_ex_t(0x90, 0x2b, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("EQ", surface,         new MIDI_event_ex_t(0x90, 0x2c, 0x7f), new MIDI_event_ex_t(0x90, 0x2c, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Instrument", surface, new MIDI_event_ex_t(0x90, 0x2d, 0x7f), new MIDI_event_ex_t(0x90, 0x2d, 0x00)));

            surface->AddWidget(new PushButton_CSurfWidget("nameValue", surface,         new MIDI_event_ex_t(0x90, 0x34, 0x7f), new MIDI_event_ex_t(0x90, 0x34, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("smpteBeats", surface,        new MIDI_event_ex_t(0x90, 0x35, 0x7f), new MIDI_event_ex_t(0x90, 0x35, 0x00)));
                
            surface->AddWidget(new PushButton_CSurfWidget(NextMap, surface,             new MIDI_event_ex_t(0x90, 0x36, 0x7f), new MIDI_event_ex_t(0x90, 0x36, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("F2", surface,                new MIDI_event_ex_t(0x90, 0x37, 0x7f), new MIDI_event_ex_t(0x90, 0x37, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("F3", surface,                new MIDI_event_ex_t(0x90, 0x38, 0x7f), new MIDI_event_ex_t(0x90, 0x38, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("F4", surface,                new MIDI_event_ex_t(0x90, 0x39, 0x7f), new MIDI_event_ex_t(0x90, 0x39, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("F5", surface,                new MIDI_event_ex_t(0x90, 0x3a, 0x7f), new MIDI_event_ex_t(0x90, 0x3a, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("F6", surface,                new MIDI_event_ex_t(0x90, 0x3b, 0x7f), new MIDI_event_ex_t(0x90, 0x3b, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(UnlockTracks, surface,        new MIDI_event_ex_t(0x90, 0x3c, 0x7f), new MIDI_event_ex_t(0x90, 0x3c, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(LockTracks, surface,          new MIDI_event_ex_t(0x90, 0x3d, 0x7f), new MIDI_event_ex_t(0x90, 0x3d, 0x00)));

            surface->AddWidget(new PushButton_CSurfWidget(Shift, surface, 1,    new MIDI_event_ex_t(0x90, 0x46, 0x7f), new MIDI_event_ex_t(0x90, 0x46, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Option, surface,  1,  new MIDI_event_ex_t(0x90, 0x47, 0x7f), new MIDI_event_ex_t(0x90, 0x47, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Control, surface, 1,  new MIDI_event_ex_t(0x90, 0x48, 0x7f), new MIDI_event_ex_t(0x90, 0x48, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Alt, surface, 1,      new MIDI_event_ex_t(0x90, 0x49, 0x7f), new MIDI_event_ex_t(0x90, 0x49, 0x00)));
                
            surface->AddWidget(new PushButton_CSurfWidget(Read, surface,                new MIDI_event_ex_t(0x90, 0x4a, 0x7f), new MIDI_event_ex_t(0x90, 0x4a, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Write, surface,               new MIDI_event_ex_t(0x90, 0x4b, 0x7f), new MIDI_event_ex_t(0x90, 0x4b, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Trim, surface,                new MIDI_event_ex_t(0x90, 0x4c, 0x7f), new MIDI_event_ex_t(0x90, 0x4c, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Touch, surface,               new MIDI_event_ex_t(0x90, 0x4d, 0x7f), new MIDI_event_ex_t(0x90, 0x4d, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Latch, surface,               new MIDI_event_ex_t(0x90, 0x4e, 0x7f), new MIDI_event_ex_t(0x90, 0x4e, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Group, surface,               new MIDI_event_ex_t(0x90, 0x4f, 0x7f), new MIDI_event_ex_t(0x90, 0x4f, 0x00)));
                
            surface->AddWidget(new PushButton_CSurfWidget(Save, surface,                new MIDI_event_ex_t(0x90, 0x50, 0x7f), new MIDI_event_ex_t(0x90, 0x50, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Undo, surface,                new MIDI_event_ex_t(0x90, 0x51, 0x7f), new MIDI_event_ex_t(0x90, 0x51, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Cancel, surface,              new MIDI_event_ex_t(0x90, 0x52, 0x7f), new MIDI_event_ex_t(0x90, 0x52, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Enter, surface,               new MIDI_event_ex_t(0x90, 0x53, 0x7f), new MIDI_event_ex_t(0x90, 0x53, 0x00)));
                
            surface->AddWidget(new PushButton_CSurfWidget(Cycle, surface,               new MIDI_event_ex_t(0x90, 0x56, 0x7f), new MIDI_event_ex_t(0x90, 0x56, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Drop", surface,              new MIDI_event_ex_t(0x90, 0x57, 0x7f), new MIDI_event_ex_t(0x90, 0x57, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Replace", surface,           new MIDI_event_ex_t(0x90, 0x58, 0x7f), new MIDI_event_ex_t(0x90, 0x58, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Click, surface,               new MIDI_event_ex_t(0x90, 0x59, 0x7f), new MIDI_event_ex_t(0x90, 0x59, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget("Solo", surface,              new MIDI_event_ex_t(0x90, 0x5a, 0x7f), new MIDI_event_ex_t(0x90, 0x5a, 0x00)));

            surface->AddWidget(new PushButton_CSurfWidget(Up, surface,       new MIDI_event_ex_t(0x90, 0x60, 0x7f), new MIDI_event_ex_t(0x90, 0x60, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Down, surface,     new MIDI_event_ex_t(0x90, 0x61, 0x7f), new MIDI_event_ex_t(0x90, 0x61, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Left, surface,     new MIDI_event_ex_t(0x90, 0x62, 0x7f), new MIDI_event_ex_t(0x90, 0x62, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Right, surface,    new MIDI_event_ex_t(0x90, 0x63, 0x7f), new MIDI_event_ex_t(0x90, 0x63, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Zoom, surface,     new MIDI_event_ex_t(0x90, 0x64, 0x7f), new MIDI_event_ex_t(0x90, 0x64, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Scrub, surface,    new MIDI_event_ex_t(0x90, 0x65, 0x7f), new MIDI_event_ex_t(0x90, 0x65, 0x00)));
            
            surface->AddWidget(new PushButton_CSurfWidget(Flip, surface,                new MIDI_event_ex_t(0x90, 0x32, 0x7f), new MIDI_event_ex_t(0x90, 0x32, 0x00)));

            surface->AddWidget(new PushButton_CSurfWidget(BankLeft, surface,            new MIDI_event_ex_t(0x90, 0x2e, 0x7f), new MIDI_event_ex_t(0x90, 0x2e, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(BankRight, surface,           new MIDI_event_ex_t(0x90, 0x2f, 0x7f), new MIDI_event_ex_t(0x90, 0x2f, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(ChannelLeft, surface,         new MIDI_event_ex_t(0x90, 0x30, 0x7f), new MIDI_event_ex_t(0x90, 0x30, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(ChannelRight, surface,        new MIDI_event_ex_t(0x90, 0x31, 0x7f), new MIDI_event_ex_t(0x90, 0x31, 0x00)));

            surface->AddWidget(new PushButton_CSurfWidget(Marker, surface,              new MIDI_event_ex_t(0x90, 0x54, 0x7f), new MIDI_event_ex_t(0x90, 0x54, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Nudge, surface,               new MIDI_event_ex_t(0x90, 0x55, 0x7f), new MIDI_event_ex_t(0x90, 0x55, 0x00)));
            
            surface->AddWidget(new PushButton_CSurfWidget(Rewind, surface,              new MIDI_event_ex_t(0x90, 0x5b, 0x7f), new MIDI_event_ex_t(0x90, 0x5b, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(FastForward, surface,         new MIDI_event_ex_t(0x90, 0x5c, 0x7f), new MIDI_event_ex_t(0x90, 0x5c, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Stop, surface,                new MIDI_event_ex_t(0x90, 0x5d, 0x7f), new MIDI_event_ex_t(0x90, 0x5d, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Play, surface,                new MIDI_event_ex_t(0x90, 0x5e, 0x7f), new MIDI_event_ex_t(0x90, 0x5e, 0x00)));
            surface->AddWidget(new PushButton_CSurfWidget(Record, surface,              new MIDI_event_ex_t(0x90, 0x5f, 0x7f), new MIDI_event_ex_t(0x90, 0x5f, 0x00)));

            for(int i = 0; i < surface->GetNumChannels(); ++i)
            {
                string trackGUID = GetManager()->GetDAW()->GetTrackGUIDAsString(currentChannel++);
                
                channel = new CSurfChannel(surface, i, trackGUID);
            
                channel->AddWidget(new Display_CSurfWidget(TrackDisplay, surface, channel, trackGUID));
            
                channel->AddWidget(new Fader14Bit_CSurfWidget(TrackVolume, surface, channel, trackGUID, -72.0, 12.0,   new MIDI_event_ex_t(0xe0 + i, 0x7f, 0x7f), new MIDI_event_ex_t(0xe0 + i, 0x00, 0x00)));
                channel->AddWidget(new PushButton_CSurfWidget(TrackTouched, surface, channel, trackGUID,        new MIDI_event_ex_t(0x90, 0x68 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x68 + i, 0x00)));
                channel->AddWidget(new EncoderCycledAction_CSurfWidget(TrackPan, surface, channel, trackGUID,   new MIDI_event_ex_t(0xb0, 0x10 + i, 0x7f), new MIDI_event_ex_t(0xb0, 0x10 + i, 0x00), new MIDI_event_ex_t(0x90, 0x20 + i, 0x7f)));

                channel->AddWidget(new PushButton_CSurfWidget(TrackRecordArm, surface, channel, trackGUID,  new MIDI_event_ex_t(0x90, 0x00 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x00 + i, 0x00)));
                channel->AddWidget(new PushButton_CSurfWidget(TrackSolo, surface, channel, trackGUID,       new MIDI_event_ex_t(0x90, 0x08 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x08 + i, 0x00)));
                channel->AddWidget(new PushButton_CSurfWidget(TrackMute, surface, channel, trackGUID,       new MIDI_event_ex_t(0x90, 0x10 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x10 + i, 0x00)));
                channel->AddWidget(new PushButton_CSurfWidget(TrackSelect, surface, channel, trackGUID,     new MIDI_event_ex_t(0x90, 0x18 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x18 + i, 0x00)));
            
                surface->AddChannel(channel);
            }
        }
    }
    
    immovableTrackGUIDs_.resize(totalNumChannels);
    for(auto & trackGUID : immovableTrackGUIDs_)
        trackGUID = "";
}

void LogicalSurface::InitializeSurfaces()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GAW TBD Hack an ini file so that testers can config MIDI IO for their local surfaces
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    
    const char *ptr = GetManager()->GetDAW()->GetResourcePath();
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
        GetManager()->GetDAW()->ShowConsoleMsg(errorBuf);
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
    
    // Midi monitoring //////////////////////////
    
    
    
    while (fgets(localBuf, sizeof(localBuf), filePtr))
    {
        if(localBuf[0] == '/')
            continue;
        
        int index = 0;
        char name[512];
        char numFadersString[512];
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
                strcpy(numFadersString, p);
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
        
        int numFaders = atoi(numFadersString);
        
        int channelIn = atoi(channelInString);
        channelIn--; // MIDI channels are 0  based
        
        int channelOut = atoi(channelOutString);
        channelOut--; // MIDI channels are 0  based

        AddSurface(new MidiCSurf(manager_, this, name, numFaders, GetManager()->MidiManager()->GetMidiInputForChannel(channelIn), GetManager()->MidiManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
    }
    
    fclose ( filePtr );
}

double LogicalSurface::GetCurrentNormalizedValue(string GUID, string name)
{
    for(auto & interactor : interactors_)
        if(interactor->GetGUID() == GUID)
            return interactor->GetCurrentNormalizedValue(ModifiedNameFor(FlipNameFor(name)));
    
    return 0.0;
}

void LogicalSurface::Update(string GUID, string name)
{
    for(auto & interactor : interactors_)
        if(interactor->GetGUID() == GUID)
            interactor->Update(ModifiedNameFor(name));
}

void LogicalSurface::ForceUpdate()
{
    for(auto& surface : surfaces_)
        surface->ForceUpdate();
}

void LogicalSurface::ForceUpdate(string GUID, string name)
{
    for(auto & interactor : interactors_)
        if(interactor->GetGUID() == GUID)
            interactor->ForceUpdate(ModifiedNameFor(name));
}

void LogicalSurface::RunAction(string GUID, string name, double value)
{
    string flipName = FlipNameFor(name);
    
    if(GUID == "")
        for(auto & action : actions_[ModifiedNameFor(flipName)])
            action->RunAction(value);
    else
        for(auto & interactor : interactors_)
            if(interactor->GetGUID() == GUID)
                interactor->RunAction(ModifiedNameFor(flipName), value);
}

void LogicalSurface::CycleAction(string GUID, string name)
{
    for(auto & interactor : interactors_)
        if(interactor->GetGUID() == GUID)
            interactor->CycleAction(ModifiedNameFor(name));
}

void LogicalSurface::SetWidgetValue(string GUID, string name, double value)
{
    for(auto & surface : surfaces_)
        if(IsOKToSetWidget(name))
            surface->SetWidgetValue(GUID, FlipNameFor(UnmodifiedNameFor(name)), value);
}

void LogicalSurface::SetWidgetValue(string GUID, string name, double value, int mode)
{
    for(auto & surface : surfaces_)
        if(IsOKToSetWidget(name))
            surface->SetWidgetValue(GUID, FlipNameFor(UnmodifiedNameFor(name)), value, mode);
}

void LogicalSurface::SetWidgetValue(string GUID, string name, string value)
{
    for(auto & surface : surfaces_)
        if(IsOKToSetWidget(name))
            surface->SetWidgetValue(GUID, FlipNameFor(UnmodifiedNameFor(name)), value);
}

void LogicalSurface::AdjustTrackBank(int stride)
{
    int previousTrackOffset = trackOffset_;
    
    trackOffset_ += stride;
    
    if(trackOffset_ < 1 - NumChannels())
        trackOffset_ = 1 - NumChannels();
    
    if(trackOffset_ > GetDAW()->GetNumTracks())
        trackOffset_ = GetDAW()->GetNumTracks();
    
    if(trackOffset_ != previousTrackOffset)
        RefreshLayout();
    
    if(isSynchronized_)
        GetManager()->AdjustTrackBank(this, stride);
}

void LogicalSurface::ImmobilizeSelectedTracks()
{
    for(int i = 0; i < NumChannels(); i++)
        if(GetDAW()->GetMediaTrackInfo_Value(GetDAW()->GetTrackFromGUID(Channel(i)->GetGUID()), "I_SELECTED"))
            immovableTrackGUIDs_[i] = Channel(i)->GetGUID();
    
    if(isSynchronized_)
        GetManager()->ImmobilizeSelectedTracks(this);
}

void LogicalSurface::MobilizeSelectedTracks()
{
    for(int i = 0; i < NumChannels(); i++)
        if(GetDAW()->GetMediaTrackInfo_Value(GetDAW()->GetTrackFromGUID(Channel(i)->GetGUID()), "I_SELECTED"))
            immovableTrackGUIDs_[i] = "";
    
    if(isSynchronized_)
        GetManager()->MobilizeSelectedTracks(this);
}

void LogicalSurface::RefreshLayout()
{
    auto currentOffset = trackOffset_;
    
    for(int i = 0; i < NumChannels(); i++)
    {
        Channel(i)->SetGUID("");
        
        if(currentOffset < 0 && immovableTrackGUIDs_[i] == "")
            currentOffset++;
        else if(immovableTrackGUIDs_[i] != "")
            Channel(i)->SetGUID(immovableTrackGUIDs_[i]);
        else if( ! isInImmovableTrackGUIDS(GetDAW()->GetTrackGUIDAsString(currentOffset)))
            Channel(i)->SetGUID(GetDAW()->GetTrackGUIDAsString(currentOffset++));
        else while(isInImmovableTrackGUIDS(GetDAW()->GetTrackGUIDAsString(currentOffset++)) && currentOffset < GetDAW()->GetNumTracks())
            Channel(i)->SetGUID(GetDAW()->GetTrackGUIDAsString(currentOffset++));
    }
}

bool LogicalSurface::DidTrackListChange()
{
    if(interactors_.size() == 0)
        return false;               // We have no idea if track list changed, we have been called way too early, there's nothing to compare, just return false
    
    if(interactors_.size() != GetDAW()->GetNumTracks() + 1) // + 1 is for Master
        return true;    // list sizes disagree
    
    for(int i = 1; i < interactors_.size(); i++)                    // Start with 1 since Master is always in position 0, it doesn't move
        if(interactors_[i]->GetGUID() != GetDAW()->GetTrackGUIDAsString(i))
            return true;
    
    return false;
}

void LogicalSurface::TrackFXListChanged(MediaTrack* track)
{
 
    char trackFXName[256];
    char trackFXParameterName[256];
    int numParameters;
    
    int trackFXCount = TrackFX_GetCount(track);
    
    string trackGUID = GetDAW()->GetTrackGUIDAsString(GetDAW()->CSurf_TrackToID(track, false));
    
    for(int i = 0; i < trackFXCount; i++)
    {
        TrackFX_GetFXName(track, i, trackFXName, sizeof(trackFXName));
        
        string fxName(trackFXName);
        
        if(fxMaps_.count(trackFXName) > 0)
        {
            FXMap* map = fxMaps_[fxName];
            
            char pBuffer[256];
            memset(pBuffer, 0, sizeof(pBuffer));
            guidToString(TrackFX_GetFXGUID(track, i), pBuffer);
            string fxGUID(pBuffer);
            
            // First, dump any existing interactors for this FX GUID

            
            Interactor* interactor = new Interactor(GetManager(), fxGUID, trackGUID, i);
         
            numParameters = TrackFX_GetNumParams(track, i);


         
            for(int j = 0; j < numParameters; j++)
            {
                TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                string fxParamName(trackFXParameterName);
                
                for(auto map : map->GetMapEntries())
                    if(map.param == fxParamName)
                        interactor->AddAction(new TrackFX_Action(map.widget, GetManager(), interactor, fxParamName, j));
            }
            
            AddInteractor(interactor);
        }
        else
        {
            /*
            ShowConsoleMsg(("\n\n" + fxName + "\n").c_str());

            numParameters = TrackFX_GetNumParams(track, i);

            for(int j = 0; j < numParameters; j++)
            {
                TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                string fxParamName(trackFXParameterName);
                
                ShowConsoleMsg((fxParamName + "\n").c_str());
            }
             */
        }
    }
}

DAW* LogicalSurface::GetDAW()
{
    return GetManager()->GetDAW();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
CSurfManager::CSurfManager()
{
    midiIOManager_ = new MidiIOManager(this);
}

double CSurfManager::GetCurrentNormalizedValue(string GUID, string name)
{
    return surfaces_[currentSurfaceIndex_]->GetCurrentNormalizedValue(GUID, name);
}

void CSurfManager::Update(string GUID, string name)
{
    surfaces_[currentSurfaceIndex_]->Update(GUID, name);
}

void  CSurfManager::ForceUpdate(string GUID, string name)
{
    surfaces_[currentSurfaceIndex_]->ForceUpdate(GUID, name);
}

void CSurfManager::RunAction(string GUID, string name, double value)
{
    surfaces_[currentSurfaceIndex_]->RunAction(GUID, name, value);
}

void CSurfManager::CycleAction(string GUID, string name)
{
    surfaces_[currentSurfaceIndex_]->CycleAction(GUID, name);
}

void CSurfManager::SetWidgetValue(string GUID, string name, double value)
{
    surfaces_[currentSurfaceIndex_]->SetWidgetValue(GUID, name, value);
}

void CSurfManager::SetWidgetValue(string GUID, string name, double value, int mode)
{
    surfaces_[currentSurfaceIndex_]->SetWidgetValue(GUID, name, value, mode);
}

void CSurfManager::SetWidgetValue(string GUID, string name, string value)
{
    surfaces_[currentSurfaceIndex_]->SetWidgetValue(GUID, name, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfChannel
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfChannel::ProcessMidiMessage(const MIDI_event_ex_t* evt)
{
    for(auto & widget : widgets_)
        widget->ProcessMidiMessage(evt);
}

void CSurfChannel::Update()
{
    for(auto & widget : widgets_)
        widget->Update();
}

void CSurfChannel::ForceUpdate()
{
    for(auto & widget : widgets_)
        widget->ForceUpdate();
}

void CSurfChannel::SetWidgetValue(string GUID, string name, double value)
{
    for(auto & widget : widgets_)
        if(widget->GetGUID() == GUID && widget->GetName() == name)
            widget->SetValue(value);
}

void CSurfChannel::SetWidgetValue(string GUID, string name, double value, int mode)
{
    for(auto & widget : widgets_)
        if(widget->GetGUID() == GUID && widget->GetName() == name)
            widget->SetValue(value, mode);
}

void CSurfChannel::SetWidgetValue(string GUID, string name, string value)
{
    for(auto & widget : widgets_)
        if(widget->GetGUID() == GUID && widget->GetName() == name)
            widget->SetValue(value);
}

DAW* CSurfChannel::GetDAW()
{
    return GetSurface()->GetManager()->GetDAW();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// UniquelySelectedCSurfChannel
////////////////////////////////////////////////////////////////////////////////////////////////////////
void UniquelySelectedCSurfChannel::OnTrackSelection(MediaTrack *track)
{
    if(GetDAW()->CountSelectedTracks(nullptr) == 1)
    {
        SetGUID(GetDAW()->GetTrackGUIDAsString(GetDAW()->CSurf_TrackToID(track, false)));
        MapFX(track);
    }
    else
        SetGUID("");
}

void UniquelySelectedCSurfChannel::MapFX(MediaTrack *track)
{
    GetDAW()->SendMessage(WM_COMMAND, NamedCommandLookup("_S&M_WNCLS3"), 0);

    
    
    
    char trackFXName[256];
    char trackFXParameterName[256];
    int numParameters;
    
    int trackFXCount = TrackFX_GetCount(track);
    
    string trackGUID = GetDAW()->GetTrackGUIDAsString(GetDAW()->CSurf_TrackToID(track, false));
    
    for(int i = 0; i < trackFXCount; i++)
    {
        TrackFX_GetFXName(track, i, trackFXName, sizeof(trackFXName));
        
        string fxName(trackFXName);
        
        if(GetSurface()->GetLogicalSurface()->GetFXMaps().count(fxName) > 0)
        {
            TrackFX_Show(track, i, 3);
            
            FXMap* map = GetSurface()->GetLogicalSurface()->GetFXMaps()[fxName];
            
            char pBuffer[256];
            memset(pBuffer, 0, sizeof(pBuffer));
            guidToString(TrackFX_GetFXGUID(track, i), pBuffer);
            string fxGUID(pBuffer);
           
            numParameters = TrackFX_GetNumParams(track, i);
           
            for(int j = 0; j < numParameters; j++)
            {
                TrackFX_GetParamName(track, i, j, trackFXParameterName, sizeof(trackFXParameterName));
                string FXParamName(trackFXParameterName);
               
                for(auto map : map->GetMapEntries())
                    for(auto widget : GetWidgets())
                        if(map.widget == widget->GetName())
                            widget->SetGUID(fxGUID);
            }
        }
    }    
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealCSurf
////////////////////////////////////////////////////////////////////////////////////////////////////////
void RealCSurf::Update()
{
    for(auto & widget : widgets_)
        widget->Update();
    
    for(auto & channel : channels_)
        channel->Update();
}

void RealCSurf::ForceUpdate()
{
    for(auto & widget : widgets_)
        widget->ForceUpdate();
    
    for(auto & channel : channels_)
        channel->ForceUpdate();
}

void RealCSurf::OnTrackSelection(MediaTrack *trackid)
{
    for(auto& channel : channels_)
        channel->OnTrackSelection(trackid);
}

void RealCSurf::SetWidgetValue(string GUID, string name, double value)
{
    for(auto & widget : widgets_)
        if(widget->GetGUID() == GUID && widget->GetName() == name)
            widget->SetValue(value);
    
    for(auto & channel : channels_)
        channel->SetWidgetValue(GUID, name, value);
}

void RealCSurf::SetWidgetValue(string GUID, string name, double value, int mode)
{
    for(auto & widget : widgets_)
        if(widget->GetGUID() == GUID && widget->GetName() == name)
            widget->SetValue(value, mode);
    
    for(auto & channel : channels_)
        channel->SetWidgetValue(GUID, name, value, mode);
}

void RealCSurf::SetWidgetValue(string GUID, string name, string value)
{
    for(auto & widget : widgets_)
        if(widget->GetGUID() == GUID && widget->GetName() == name)
            widget->SetValue(value);
    
    for(auto & channel : channels_)
        channel->SetWidgetValue(GUID, name, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interactor
////////////////////////////////////////////////////////////////////////////////////////////////////////
double Interactor::GetCurrentNormalizedValue(string name)
{
    if(actions_[name].size() > 0)
        return (actions_[name])[0]->GetCurrentNormalizedValue();
    else
        return 0.0;
}

void Interactor::Update(string name)
{
    for(auto action : actions_[name])
        action->Update();
}

void Interactor::ForceUpdate(string name)
{
    for(auto action : actions_[name])
        action->ForceUpdate();
}

void Interactor::RunAction(string name, double value)
{
    for(auto action : actions_[name])
        action->RunAction(value);
}

void Interactor::CycleAction(string name)
{
    for(auto action : actions_[name])
        action->Cycle();
}

MediaTrack* Interactor::GetTrack()
{
    return GetManager()->GetDAW()->GetTrackFromGUID(trackGUID_);
}

DAW* Interactor::GetDAW()
{
    return GetManager()->GetDAW();
}
