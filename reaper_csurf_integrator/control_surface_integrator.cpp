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

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void RealSurface::MapTrackActions(string GUID)
{
    GetLogicalSurface()->MapTrackActions(GUID);
}

// to Actions ->
double RealSurface::GetActionCurrentNormalizedValue(string GUID, string actionName, string widgetName)
{
    return GetLogicalSurface()->GetActionCurrentNormalizedValue(ActionAddressFor(GUID, actionName), GetName(), widgetName);
}

void RealSurface::UpdateAction(string GUID, string actionName, string widgetName)
{
    GetLogicalSurface()->UpdateAction(ActionAddressFor(GUID, actionName), GetName(), widgetName);
}

void RealSurface::ForceUpdateAction(string GUID, string actionName, string widgetName)
{
    GetLogicalSurface()->ForceUpdateAction(ActionAddressFor(GUID, actionName), GetName(), widgetName);
}

void RealSurface::CycleAction(string GUID, string actionName, string widgetName)
{
    GetLogicalSurface()->CycleAction(ActionAddressFor(GUID, actionName), GetName(), widgetName);
}

void RealSurface::DoAction(string GUID, string actionName, string widgetName, double value)
{
    GetLogicalSurface()->DoAction(ActionAddressFor(GUID, actionName), value, GetName(), widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// LogicalSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogicalSurface::SetImmobilizedChannels()
{
    char buffer[256];
    
    for(auto * surface : realSurfaces_)
    {
        for(int i = 0; i < surface->GetChannels().size(); i++)
        {
            if(1 == DAW::GetProjExtState(nullptr, ControlSurfaceIntegrator.c_str(), (surface->GetName() +  to_string(i - 1)).c_str(), buffer, sizeof(buffer)))
            {
                surface->GetChannels()[i]->SetGUID(buffer);
                surface->GetChannels()[i]->SetIsMovable(false);
            }
        }
    }
}

void LogicalSurface::MapTrackActions(string trackGUID)
{
    if(trackGUID == "")
        return; // Nothing to map
    
    for(string mappedTrackActionGUID : mappedTrackActionGUIDs_)
        if(mappedTrackActionGUID == trackGUID)
            return; // Already did this track
    
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
    
    for(auto * surface : realSurfaces_)
    {
        string surfaceName = surface->GetName();
        
        AddAction(trackGUID + surfaceName + TrackDisplay, new TrackName_DisplayAction(this, track));
        AddAction(trackGUID + surfaceName + TrackTouched, new TouchStateControlled_Action(this, track, TrackDisplay, trackGUID + surfaceName + TrackDisplay, new TrackVolume_DisplayAction(this, track)));
        AddAction(trackGUID + surfaceName + Volume, new TrackVolume_Action(this, track));
        
        CycledAction* cyclicAction = new CycledAction(this);
        cyclicAction->Add(new TrackPan_Action(this, track, 0x00));
        cyclicAction->Add(new TrackPanWidth_Action(this, track, 0x30));
        AddAction(trackGUID + surfaceName + Pan, cyclicAction);
        
        AddAction(trackGUID + surfaceName + Select, new TrackUniqueSelect_Action(this, track));
        AddAction(trackGUID + surfaceName + Shift + Select, new TrackRangeSelect_Action(this, track));
        AddAction(trackGUID + surfaceName + Control + Select, new TrackSelect_Action(this, track));
        
        AddAction(trackGUID + surfaceName + RecordArm, new TrackRecordArm_Action(this, track));
        AddAction(trackGUID + surfaceName + Mute, new TrackMute_Action(this, track));
        AddAction(trackGUID + surfaceName + Solo, new TrackSolo_Action(this, track));
        
        AddAction(trackGUID + surfaceName + TrackOutMeterLeft, new VUMeter_Action(this, track, 0));
        AddAction(trackGUID + surfaceName + TrackOutMeterRight, new VUMeter_Action(this, track, 1));
        
        MapFX(track);
    }
    
    mappedTrackActionGUIDs_.push_back(trackGUID);
}
void LogicalSurface::MapWidgetsToFX(MediaTrack *track)
{
    string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
    
    // Map Track to any Channels interested
    for(auto* surface : realSurfaces_)
        for(auto* channel : surface->GetChannels())
            if(channel->GetShouldMapFXTrackToChannel())
                channel->SetGUID(trackGUID);
    
    char fxName[256];
    char fxGUID[256];
    char fxParamName[256];

    for(auto* surface : realSurfaces_)
        surface->ClearFXWindows();

    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);

        for(auto* surface : realSurfaces_)
        {
            if(fxMaps_.count(surface->GetName() + fxName) > 0)
            {
                FXMap* map = fxMaps_[surface->GetName() + fxName];

                for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); DAW::TrackFX_GetParamName(track, i, j++, fxParamName, sizeof(fxParamName)))
                    for(auto map : map->GetMapEntries())
                        if(map.paramName == fxParamName)
                            surface->SetWidgetGUID(map.widgetName, trackGUID + fxGUID);
                
                surface->AddFXWindow(FXWindow(track, i));
            }
        }
    }
    
    for(auto* surface : realSurfaces_)
        OpenFXWindows(surface);
    
    ForceUpdate();
}

void LogicalSurface::MapFX(MediaTrack* track)
{
    char fxName[256];
    char fxParamName[256];
    char fxGUID[256];
    
    string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
   
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
     
        for(auto* surface : realSurfaces_)
        {
            if(fxMaps_.count(surface->GetName() + fxName) > 0)
            {
                FXMap* map = fxMaps_[surface->GetName() + fxName];
            
                DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
         
                for(auto map : map->GetMapEntries())
                {
                    if(map.paramName == ReaperGainReduction_dB)
                    {
                        AddAction(trackGUID + fxGUID + surface->GetName() + map.widgetName, new GainReductionMeter_Action(this, track, i));
                    }
                    else
                    {
                        for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                        {
                            DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
         
                            if(map.paramName == fxParamName)
                                AddAction(trackGUID + fxGUID + surface->GetName() + map.widgetName, new TrackFX_Action(this, track, i, j));
                        }
                    }
                }
            }
            
            if(GetVSTMonitor() && GetManager()->GetLazyIsInitialized())
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
}


/*
void SID_PCM_Source::SaveState(ProjectStateContext * ctx)
{
    ctx->AddLine("FILE \"%s\" %f %d %d %d %d", m_sidfn.toRawUTF8(),m_sidlen,m_sid_track, m_sid_channelmode,m_sid_sr,m_sid_render_mode);
}

int SID_PCM_Source::LoadState(const char * firstline, ProjectStateContext * ctx)
{
    LineParser lp;
    char buf[2048];
    for (;;)
    {
        if (ctx->GetLine(buf, sizeof(buf)))
            break;
        lp.parse(buf);
        if (strcmp(lp.gettoken_str(0), "FILE") == 0)
        {
            m_sidfn = String(CharPointer_UTF8(lp.gettoken_str(1)));
            m_sidlen = lp.gettoken_float(2);
            m_sid_track = lp.gettoken_int(3);
            m_sid_channelmode = lp.gettoken_int(4);
            m_sid_sr = lp.gettoken_int(5);
            m_sid_render_mode = lp.gettoken_int(6);
        }
        if (lp.gettoken_str(0)[0] == '>')
        {
            renderSID();
            return 0;
        }
    }
    return -1;
}
*/



// C++17 version ////////////////////////////////////////////////////


void SaveState(ProjectStateContext * ctx)
{
    //ctx->AddLine("FILE \"%s\" %f %d %d %d %d", m_sidfn.toRawUTF8(),m_sidlen,m_sid_track, m_sid_channelmode,m_sid_sr,m_sid_render_mode);
}

template<typename T>
inline void setFromToken(LineParser& lp, int index, T& value)
{
    if constexpr (is_same<int, T>::value)
        value = lp.gettoken_int(index);
    else if constexpr (is_same<double, T>::value)
        value = lp.gettoken_float(index);
    else if constexpr (is_same<float, T>::value)
        value = static_cast<float>(lp.gettoken_float(index));
    else if constexpr (is_same<string, T>::value)
        value = string(lp.gettoken_str(index));
    //else
        //static_assert(false, "Type not supported by LineParser");
}

template<typename... Args>
inline void setFromTokens(LineParser& lp, Args&&... args)
{
    int index = 0;
    (setFromToken(lp, ++index, args), ...);
}

int LoadState(const char * firstline, ProjectStateContext * ctx)
{
    LineParser lp;
    char buf[2048];
    for (;;)
    {
        if (ctx->GetLine(buf, sizeof(buf)))
            break;
        lp.parse(buf);
        if (strcmp(lp.gettoken_str(0), "FILE") == 0)
        {
            //setFromTokens(lp, m_sidfn, m_sidlen, m_sid_track, m_sid_channelmode, m_sid_sr, m_sid_render_mode);
        }
        if (lp.gettoken_str(0)[0] == '>')
        {
            //renderSID();
            return 0;
        }
    }
    return -1;
}


void LogicalSurface::InitFXMaps(RealSurface* surface)
{
    // param  = GainReduction_dB
    // widget = CompressorMeter
    
    string surfaceName = surface->GetName();

    FXMap* fxMap = new FXMap(surfaceName + "VST: UAD UA 1176LN Rev E (Universal Audio, Inc.)");
    
    fxMap->AddEntry(Threshold, "Input");
    fxMap->AddEntry(Character, "Output");
    fxMap->AddEntry(Drive, "Meter");
    fxMap->AddEntry(Attack, "Attack");
    fxMap->AddEntry(Release, "Release");
    fxMap->AddEntry(Ratio, "Ratio");
    fxMap->AddEntry(Compressor, "Bypass");
    fxMap->AddEntry(Parallel, "Wet");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap(surfaceName + "VST: UAD Fairchild 660 (Universal Audio, Inc.)");
    
    fxMap->AddEntry(Threshold, "Thresh");
    fxMap->AddEntry(Character, "Output");
    fxMap->AddEntry(Drive, "Meter");
    fxMap->AddEntry(Attack, "Headroom");
    fxMap->AddEntry(Release, "Input");
    fxMap->AddEntry(Ratio, "Time Const");
    fxMap->AddEntry(Compressor, "Bypass");
    fxMap->AddEntry(Parallel, "Wet");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap(surfaceName + "VST: UAD Teletronix LA-2A Silver (Universal Audio, Inc.)");
    
    fxMap->AddEntry(Threshold, "Peak Reduct");
    fxMap->AddEntry(Character, "Gain");
    fxMap->AddEntry(Drive, "Meter");
    fxMap->AddEntry(Attack, "Emphasis");
    fxMap->AddEntry(Ratio, "Comp/Limit");
    fxMap->AddEntry(Compressor, "Bypass");
    fxMap->AddEntry(Parallel, "Wet");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap(surfaceName + "VST: UAD Harrison 32C (Universal Audio, Inc.)");
    
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
    
    fxMap = new FXMap(surfaceName + "VST: UAD Pultec EQP-1A (Universal Audio, Inc.)");
    
    //fxMap->AddEntry(LoCurve, "");
    //fxMap->AddEntry(HiCurve, "");
    fxMap->AddEntry(HiGain, "HF Atten");
    fxMap->AddEntry(HiFrequency, "HF Atten Freq");
    fxMap->AddEntry(HiMidGain, "HF Boost");
    fxMap->AddEntry(HiMidFrequency, "High Freq");
    fxMap->AddEntry(HiMidQ, "HF Q");
    fxMap->AddEntry(LoMidGain, "LF Atten");
    fxMap->AddEntry(LoMidFrequency, "Low Freq");
    //fxMap->AddEntry(LoMidQ, "");
    fxMap->AddEntry(LoGain, "LF Boost");
    fxMap->AddEntry(LoFrequency, "Low Freq");
    fxMap->AddEntry(Equalizer, "Bypass");
    
    AddFXMap(fxMap);
    
    fxMap = new FXMap(surfaceName + "VST: UAD Pultec MEQ-5 (Universal Audio, Inc.)");
    
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

void LogicalSurface::MapReaperLogicalControlSurfaceActions(RealSurface* surface)
{
    string surfaceName = surface->GetName();

    AddAction(ReaperLogicalControlSurface + surfaceName + ChannelLeft, new TrackBank_Action(this, -1));
    AddAction(ReaperLogicalControlSurface + surfaceName + ChannelRight, new TrackBank_Action(this, 1));
    AddAction(ReaperLogicalControlSurface + surfaceName + BankLeft, new TrackBank_Action(this, -8));
    AddAction(ReaperLogicalControlSurface + surfaceName + BankRight, new TrackBank_Action(this, 8));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + NextMap, new NextMap_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + LockTracks, new ImmobilizeSelectedTracks_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + UnlockTracks, new MobilizeSelectedTracks_Action(this));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift, new Shift_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Option, new Option_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Control, new Control_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Alt, new Alt_Action(this));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + Read, new TrackAutoMode_Action(this, 1));
    AddAction(ReaperLogicalControlSurface + surfaceName + Write, new TrackAutoMode_Action(this, 3));
    AddAction(ReaperLogicalControlSurface + surfaceName + Trim, new TrackAutoMode_Action(this, 0));
    AddAction(ReaperLogicalControlSurface + surfaceName + Touch, new TrackAutoMode_Action(this, 2));
    AddAction(ReaperLogicalControlSurface + surfaceName + Latch, new TrackAutoMode_Action(this, 4));
    AddAction(ReaperLogicalControlSurface + surfaceName + Group, new TrackAutoMode_Action(this, 5));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Read, new GlobalAutoMode_Action(this, 1));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Write, new GlobalAutoMode_Action(this, 3));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Trim, new GlobalAutoMode_Action(this, 0));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Touch, new GlobalAutoMode_Action(this, 2));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Latch, new GlobalAutoMode_Action(this, 4));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Group, new GlobalAutoMode_Action(this, 5));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + Save, new Save_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Save, new SaveAs_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Undo, new Undo_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Undo, new Redo_Action(this));
    
    //logicalSurfaceInteractor_->AddAction(new Enter_Action(Enter, logicalSurfaceInteractor_));
    //logicalSurfaceInteractor_->AddAction(new Cancel_Action(Cancel, logicalSurfaceInteractor_));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + Marker, new PreviousMarker_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Shift + Marker, new InsertMarker_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Option + Marker, new InsertMarkerRegion_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Nudge, new NextMarker_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Cycle, new CycleTimeline_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Click, new Metronome_Action(this));
    
    AddAction(ReaperLogicalControlSurface + surfaceName + Rewind, new Rewind_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + FastForward, new FastForward_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Stop, new Stop_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Play, new Play_Action(this));
    AddAction(ReaperLogicalControlSurface + surfaceName + Record, new Record_Action(this));
    
    // GAW TBD -- timers need to be cross platform
    
    //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Up,  logicalSurfaceInteractor_, 0, 0.3));
    //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Down,  logicalSurfaceInteractor_, 1, 0.3));
    //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Left,  logicalSurfaceInteractor_, 2, 0.3));
    //logicalSurfaceInteractor_->AddAction(new RepeatingArrow_Action(Right, logicalSurfaceInteractor_, 3, 0.3));
    
    //logicalSurfaceInteractor_->AddAction(new LatchedZoom_Action(Zoom,  logicalSurfaceInteractor_));
    //logicalSurfaceInteractor_->AddAction(new LatchedScrub_Action(Scrub,  logicalSurfaceInteractor_));

    AddAction(ReaperLogicalControlSurface + surfaceName + DisplayFX, new SetShowFXWindows_Action(this));

    
    RefreshLayout();
}

void LogicalSurface::InitCSurfWidgets(RealSurface* surface)
{
    RealSurfaceChannel* channel = nullptr;

    if(surface->GetName() == "Console1")
    {
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, DisplayFX, new MIDI_event_ex_t(0xb0, 0x66, 0x7f), new MIDI_event_ex_t(0xb0, 0x66, 0x00)));

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
        
        channel = new RealSurfaceChannel( "", surface, true, true);
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

        // Output

        channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Drive,            new MIDI_event_ex_t(0xb0, 0x0f, 0x7f), new MIDI_event_ex_t(0xb0, 0x0f, 0x00)));
        channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Character,        new MIDI_event_ex_t(0xb0, 0x12, 0x7f), new MIDI_event_ex_t(0xb0, 0x12, 0x00)));

        // Channel
        channel->AddWidget(new PushButton_MidiWidget("", surface, Solo,       new MIDI_event_ex_t(0xb0, 0x0d, 0x7f), new MIDI_event_ex_t(0xb0, 0x0d, 0x00)));
        channel->AddWidget(new PushButton_MidiWidget("", surface, Mute,       new MIDI_event_ex_t(0xb0, 0x0c, 0x7f), new MIDI_event_ex_t(0xb0, 0x0c, 0x00)));

        channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Volume,      new MIDI_event_ex_t(0xb0, 0x07, 0x7f),  new MIDI_event_ex_t(0xb0, 0x07, 0x00)));
        channel->AddWidget(new Fader7Bit_MidiWidget("", surface, Pan,         new MIDI_event_ex_t(0xb0, 0x0a, 0x7f), new MIDI_event_ex_t(0xb0, 0x0a, 0x00)));
        
        channel->AddWidget(new VUMeter_MidiWidget("", surface, TrackOutMeterLeft, -60.0, 6.0, new  MIDI_event_ex_t(0xb0, 0x70, 0x7f),     new MIDI_event_ex_t(0xb0, 0x70, 0x00)));
        channel->AddWidget(new VUMeter_MidiWidget("", surface, TrackOutMeterRight, -60.0, 6.0, new  MIDI_event_ex_t(0xb0, 0x71, 0x7f),    new  MIDI_event_ex_t(0xb0, 0x71, 0x00)));
    }
    else
    {
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Track",       new MIDI_event_ex_t(0x90, 0x28, 0x7f), new MIDI_event_ex_t(0x90, 0x28, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Send",        new MIDI_event_ex_t(0x90, 0x29, 0x7f), new MIDI_event_ex_t(0x90, 0x29, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Pan",         new MIDI_event_ex_t(0x90, 0x2a, 0x7f), new MIDI_event_ex_t(0x90, 0x2a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Plugin",      new MIDI_event_ex_t(0x90, 0x2b, 0x7f), new MIDI_event_ex_t(0x90, 0x2b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "EQ",          new MIDI_event_ex_t(0x90, 0x2c, 0x7f), new MIDI_event_ex_t(0x90, 0x2c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Instrument",  new MIDI_event_ex_t(0x90, 0x2d, 0x7f), new MIDI_event_ex_t(0x90, 0x2d, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "nameValue",   new MIDI_event_ex_t(0x90, 0x34, 0x7f), new MIDI_event_ex_t(0x90, 0x34, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "smpteBeats",  new MIDI_event_ex_t(0x90, 0x35, 0x7f), new MIDI_event_ex_t(0x90, 0x35, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, NextMap,       new MIDI_event_ex_t(0x90, 0x36, 0x7f), new MIDI_event_ex_t(0x90, 0x36, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "F2",          new MIDI_event_ex_t(0x90, 0x37, 0x7f), new MIDI_event_ex_t(0x90, 0x37, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "F3",          new MIDI_event_ex_t(0x90, 0x38, 0x7f), new MIDI_event_ex_t(0x90, 0x38, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "F4",          new MIDI_event_ex_t(0x90, 0x39, 0x7f), new MIDI_event_ex_t(0x90, 0x39, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "F5",          new MIDI_event_ex_t(0x90, 0x3a, 0x7f), new MIDI_event_ex_t(0x90, 0x3a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "F6",          new MIDI_event_ex_t(0x90, 0x3b, 0x7f), new MIDI_event_ex_t(0x90, 0x3b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, UnlockTracks,  new MIDI_event_ex_t(0x90, 0x3c, 0x7f), new MIDI_event_ex_t(0x90, 0x3c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, LockTracks,    new MIDI_event_ex_t(0x90, 0x3d, 0x7f), new MIDI_event_ex_t(0x90, 0x3d, 0x00)));

        surface->AddWidget(new PushButtonWithRelease_MidiWidget(ReaperLogicalControlSurface, surface, Shift,     new MIDI_event_ex_t(0x90, 0x46, 0x7f), new MIDI_event_ex_t(0x90, 0x46, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(ReaperLogicalControlSurface, surface, Option,    new MIDI_event_ex_t(0x90, 0x47, 0x7f), new MIDI_event_ex_t(0x90, 0x47, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(ReaperLogicalControlSurface, surface, Control,   new MIDI_event_ex_t(0x90, 0x48, 0x7f), new MIDI_event_ex_t(0x90, 0x48, 0x00)));
        surface->AddWidget(new PushButtonWithRelease_MidiWidget(ReaperLogicalControlSurface, surface, Alt,       new MIDI_event_ex_t(0x90, 0x49, 0x7f), new MIDI_event_ex_t(0x90, 0x49, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Read,          new MIDI_event_ex_t(0x90, 0x4a, 0x7f), new MIDI_event_ex_t(0x90, 0x4a, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Write,         new MIDI_event_ex_t(0x90, 0x4b, 0x7f), new MIDI_event_ex_t(0x90, 0x4b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Trim,          new MIDI_event_ex_t(0x90, 0x4c, 0x7f), new MIDI_event_ex_t(0x90, 0x4c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Touch,         new MIDI_event_ex_t(0x90, 0x4d, 0x7f), new MIDI_event_ex_t(0x90, 0x4d, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Latch,         new MIDI_event_ex_t(0x90, 0x4e, 0x7f), new MIDI_event_ex_t(0x90, 0x4e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Group,         new MIDI_event_ex_t(0x90, 0x4f, 0x7f), new MIDI_event_ex_t(0x90, 0x4f, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Save,          new MIDI_event_ex_t(0x90, 0x50, 0x7f), new MIDI_event_ex_t(0x90, 0x50, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Undo,          new MIDI_event_ex_t(0x90, 0x51, 0x7f), new MIDI_event_ex_t(0x90, 0x51, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Cancel,        new MIDI_event_ex_t(0x90, 0x52, 0x7f), new MIDI_event_ex_t(0x90, 0x52, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Enter,         new MIDI_event_ex_t(0x90, 0x53, 0x7f), new MIDI_event_ex_t(0x90, 0x53, 0x00)));
        
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Cycle,         new MIDI_event_ex_t(0x90, 0x56, 0x7f), new MIDI_event_ex_t(0x90, 0x56, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Drop",        new MIDI_event_ex_t(0x90, 0x57, 0x7f), new MIDI_event_ex_t(0x90, 0x57, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Replace",     new MIDI_event_ex_t(0x90, 0x58, 0x7f), new MIDI_event_ex_t(0x90, 0x58, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Click,         new MIDI_event_ex_t(0x90, 0x59, 0x7f), new MIDI_event_ex_t(0x90, 0x59, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, "Solo",        new MIDI_event_ex_t(0x90, 0x5a, 0x7f), new MIDI_event_ex_t(0x90, 0x5a, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Up,            new MIDI_event_ex_t(0x90, 0x60, 0x7f), new MIDI_event_ex_t(0x90, 0x60, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Down,          new MIDI_event_ex_t(0x90, 0x61, 0x7f), new MIDI_event_ex_t(0x90, 0x61, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Left,          new MIDI_event_ex_t(0x90, 0x62, 0x7f), new MIDI_event_ex_t(0x90, 0x62, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Right,         new MIDI_event_ex_t(0x90, 0x63, 0x7f), new MIDI_event_ex_t(0x90, 0x63, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Zoom,          new MIDI_event_ex_t(0x90, 0x64, 0x7f), new MIDI_event_ex_t(0x90, 0x64, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Scrub,         new MIDI_event_ex_t(0x90, 0x65, 0x7f), new MIDI_event_ex_t(0x90, 0x65, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, BankLeft,      new MIDI_event_ex_t(0x90, 0x2e, 0x7f), new MIDI_event_ex_t(0x90, 0x2e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, BankRight,     new MIDI_event_ex_t(0x90, 0x2f, 0x7f), new MIDI_event_ex_t(0x90, 0x2f, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, ChannelLeft,   new MIDI_event_ex_t(0x90, 0x30, 0x7f), new MIDI_event_ex_t(0x90, 0x30, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, ChannelRight,  new MIDI_event_ex_t(0x90, 0x31, 0x7f), new MIDI_event_ex_t(0x90, 0x31, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Marker,        new MIDI_event_ex_t(0x90, 0x54, 0x7f), new MIDI_event_ex_t(0x90, 0x54, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Nudge,         new MIDI_event_ex_t(0x90, 0x55, 0x7f), new MIDI_event_ex_t(0x90, 0x55, 0x00)));

        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Rewind,        new MIDI_event_ex_t(0x90, 0x5b, 0x7f), new MIDI_event_ex_t(0x90, 0x5b, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, FastForward,   new MIDI_event_ex_t(0x90, 0x5c, 0x7f), new MIDI_event_ex_t(0x90, 0x5c, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Stop,          new MIDI_event_ex_t(0x90, 0x5d, 0x7f), new MIDI_event_ex_t(0x90, 0x5d, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Play,          new MIDI_event_ex_t(0x90, 0x5e, 0x7f), new MIDI_event_ex_t(0x90, 0x5e, 0x00)));
        surface->AddWidget(new PushButton_MidiWidget(ReaperLogicalControlSurface, surface, Record,        new MIDI_event_ex_t(0x90, 0x5f, 0x7f), new MIDI_event_ex_t(0x90, 0x5f, 0x00)));
        
        for(int i = 0; i < surface->GetNumBankableChannels(); ++i)
        {
            string channelNumber = to_string(i + 1);;

            channel = new RealSurfaceChannel("", surface, true);
            surface->AddChannel(channel);
            
            channel->AddWidget(new PushButtonWithRelease_MidiWidget("", surface, TrackTouched, TrackTouched + channelNumber, new MIDI_event_ex_t(0x90, 0x68 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x68 + i, 0x00)));
            
            channel->AddWidget(new EncoderCycledAction_MidiWidget("", surface, Pan, Pan + channelNumber, new MIDI_event_ex_t(0xb0, 0x10 + i, 0x7f), new MIDI_event_ex_t(0xb0, 0x10 + i, 0x00), new MIDI_event_ex_t(0x90, 0x20 + i, 0x7f)));

            channel->AddWidget(new Display_MidiWidget("", surface, TrackDisplay, TrackDisplay + channelNumber, i));
            channel->AddWidget(new Fader14Bit_MidiWidget("", surface, Volume, Volume + channelNumber, -72.0, 12.0, new MIDI_event_ex_t(0xe0 + i, 0x7f, 0x7f), new MIDI_event_ex_t(0xe0 + i, 0x00, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, RecordArm, RecordArm + channelNumber,   new MIDI_event_ex_t(0x90, 0x00 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x00 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, Solo, Solo + channelNumber,        new MIDI_event_ex_t(0x90, 0x08 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x08 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, Mute, Mute + channelNumber,        new MIDI_event_ex_t(0x90, 0x10 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x10 + i, 0x00)));
            channel->AddWidget(new PushButton_MidiWidget("", surface, Select, Select + channelNumber,      new MIDI_event_ex_t(0x90, 0x18 + i, 0x7f), new MIDI_event_ex_t(0x90, 0x18 + i, 0x00)));
        }
    }
}

void LogicalSurface::InitRealSurfaces()
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

        AddRealSurface(new MidiCSurf(this, name, numBankableChannels, GetManager()->MidiManager()->GetMidiInputForChannel(channelIn), GetManager()->MidiManager()->GetMidiOutputForChannel(channelOut), midiInMonitor, midiOutMonitor));
    }
    
    VSTMonitor_ = VSTMonitor;
    
    fclose ( filePtr );
}
