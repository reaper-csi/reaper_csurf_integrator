//
//  control_surface_Reaper_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_Reaper_actions_h
#define control_surface_Reaper_actions_h

#include "control_surface_base_actions.h"

#define IDC_REPEAT                      1068
#define ID_FILE_SAVEAS                  40022
#define ID_FILE_NEWPROJECT              40023
#define ID_FILE_OPENPROJECT             40025
#define ID_FILE_SAVEPROJECT             40026
#define IDC_EDIT_UNDO                   40029
#define IDC_EDIT_REDO                   40030
#define ID_MARKER_PREV                  40172
#define ID_MARKER_NEXT                  40173
#define ID_INSERT_MARKERRGN             40174
#define ID_INSERT_MARKER                40157
#define ID_LOOP_SETSTART                40222
#define ID_LOOP_SETEND                  40223
#define ID_METRONOME                    40364
#define ID_GOTO_MARKER1                 40161
#define ID_SET_MARKER1                  40657

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackFX_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string paramName_ = "";
    int paramIndex_ = 0;
    
public:
    TrackFX_Action(string name, Interactor* interactor, string paramName, int paramIndex) : DoubleAction(name, interactor), paramName_(paramName), paramIndex_(paramIndex) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return currentValue_;
    }
    
    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
    }
    
    virtual double GetValue() override
    {
        double min = 0;
        double max = 0;
        return DAW::TrackFX_GetParam(GetInteractor()->GetTrack(), GetInteractor()->GetIndex(), paramIndex_, &min, &max);
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        DAW::TrackFX_SetParam(GetInteractor()->GetTrack(), GetInteractor()->GetIndex(), paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return volToNormalized(currentValue_);
    }

    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        GetInteractor()->SetWidgetValue(surfaceName, GetName(), volToNormalized(value));
    }

    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_VOL");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
protected:
    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        GetInteractor()->SetWidgetValue(surfaceName, GetName(), panToNormalized(value), displayMode_);
    }
    
public:
    TrackPan_Action(string name, Interactor* interactor, int displayMode) : DoubleAction(name, interactor), displayMode_(displayMode) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return panToNormalized(currentValue_);
    }

    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_PAN");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
protected:
    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        GetInteractor()->SetWidgetValue(surfaceName, GetName(), panToNormalized(value), displayMode_);
    }
    
public:
    TrackPanWidth_Action(string name, Interactor* interactor, int displayMode) : DoubleAction(name, interactor), displayMode_(displayMode) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return panToNormalized(currentValue_);
    }

    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_WIDTH");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        DAW::CSurf_OnWidthChange(GetInteractor()->GetTrack(), normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackName_DisplayAction : public StringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackName_DisplayAction(string name, Interactor* interactor) : StringDisplayAction(name, interactor) {}
    
    virtual string GetValue() override
    {
        if(DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "IP_TRACKNUMBER") == -1)
            return "Master";
        else
            return (char *)DAW::GetSetMediaTrackInfo(GetInteractor()->GetTrack(), "P_NAME", NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_DisplayAction : public StringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_DisplayAction(string name, Interactor* interactor) : StringDisplayAction(name, interactor) {}
    
    virtual string GetValue() override
    {
        double value = DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_VOL");
            
        double DBValue = VAL2DB(value);

        char buffer[128];
        sprintf(buffer, "%7.2lf", DBValue);
        return string(buffer);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::CSurf_OnRew(1); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FastForward_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::CSurf_OnFwd(1); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Play_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::CSurf_OnPlay(); }
    
    virtual double GetValue() override
    {
        int playState = DAW::GetPlayState();
        
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            return 1;
        else return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Stop_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::CSurf_OnStop(); }
    
    virtual double GetValue() override
    {
        int playState = DAW::GetPlayState();
        
        if(playState == 0 || playState == 2 || playState == 6) // stopped or paused or paused whilst recording
            return 1;
        else
            return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Record_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::CSurf_OnRecord(); }
    
    virtual double GetValue() override
    {
        int playState = DAW::GetPlayState();
        
        if(playState == 5 || playState == 6) // recording or paused whilst recording
            return 1;
        else
            return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RepeatingArrow_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int direction_ = 0;
    clock_t lastRepeated = clock();
    double repeatRate_ = 0.0;
    bool pressed_ = false;
    
public:
    RepeatingArrow_Action(string name, Interactor* interactor, int direction, double repeatRate) : DoubleAction(name, interactor), direction_(direction), repeatRate_(repeatRate) {}

    virtual void UpdateAction(string surfaceName) override
    {
        if(pressed_ && clock() - lastRepeated >  CLOCKS_PER_SEC * repeatRate_)
        {
            lastRepeated = clock();
            DAW::CSurf_OnArrow(direction_, GetInteractor()->GetLogicalSurface()->IsZoom());
        }
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        DAW::CSurf_OnArrow(direction_, GetInteractor()->GetLogicalSurface()->IsZoom());
        pressed_ = value;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelect_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
        GetInteractor()->GetLogicalSurface()->GetManager()->OnTrackSelection(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::SetOnlyTrackSelected(track);
        GetInteractor()->GetLogicalSurface()->GetManager()->OnTrackSelection(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelectionSelect_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelectionSelect_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        int selectedTrackNum = 0;
        int selectionTrackNumber = DAW::CSurf_TrackToID(GetInteractor()->GetTrack(), false);
        
        if(DAW::CountSelectedTracks(nullptr) == 1)
        {
            for(int i = 0; i < DAW::GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(DAW::CSurf_TrackFromID(i, false), "I_SELECTED"))
                    selectedTrackNum = i;

            int lowerBound = selectionTrackNumber < selectedTrackNum ? selectionTrackNumber : selectedTrackNum;
            int upperBound = selectionTrackNumber > selectedTrackNum ? selectionTrackNumber : selectedTrackNum;
            
            for(int i = lowerBound; i <= upperBound; i++)
            {
                DAW::CSurf_SetSurfaceSelected(GetInteractor()->GetTrack(), DAW::CSurf_OnSelectedChange(DAW::CSurf_TrackFromID(i, false), 1), NULL);
                GetInteractor()->GetLogicalSurface()->GetManager()->OnTrackSelection(DAW::CSurf_TrackFromID(i, false));
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRecordArm_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
   
    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_RECARM");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::CSurf_SetSurfaceRecArm(track, DAW::CSurf_OnRecArmChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "B_MUTE");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::CSurf_SetSurfaceMute(track, DAW::CSurf_OnMuteChange(track, ! DAW::GetMediaTrackInfo_Value(track, "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override
    {
        return DAW::GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SOLO");
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        MediaTrack* track = GetInteractor()->GetTrack();
        DAW::CSurf_SetSurfaceSolo(track, DAW::CSurf_OnSoloChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TouchStateControlled_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* controlledAction_= nullptr;
    bool currentlyTouched_ = false;
    bool lastTouched_ = true;
    
public:
    TouchStateControlled_Action(string name, Interactor* interactor, Action* controlledAction) : DoubleAction(name, interactor), controlledAction_(controlledAction) {}
    
    virtual string GetAlias() override { return controlledAction_->GetName(); }

    virtual double GetValue() override { return currentlyTouched_; }

    virtual void RunAction(string surfaceName, double value) override { currentlyTouched_ =  value == 0 ? false : true; }

    virtual void UpdateAction(string surfaceName) override
    {
        if(currentlyTouched_)
            controlledAction_->UpdateAction(surfaceName);

        if(lastTouched_ != currentlyTouched_)
        {
            lastTouched_ = currentlyTouched_;
            
            if(currentlyTouched_ == false)
                GetInteractor()->ForceUpdateAction(surfaceName, GetAlias());
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int autoMode_ = 0;
    
public:
    GlobalAutoMode_Action(string name, Interactor* interactor, int autoMode) : DoubleAction(name, interactor), autoMode_(autoMode) {}
    
    virtual double GetValue () override { return DAW::GetGlobalAutomationOverride(); }
    
    virtual void RunAction(string surfaceName, double value) override {  DAW::SetGlobalAutomationOverride(autoMode_); }
    
    virtual void ForceUpdateAction(string surfaceName) override
    {
        if(GetValue() == autoMode_)
            SetWidgetValue(surfaceName, autoMode_);
    }
    
    virtual void UpdateAction(string surfaceName) override
    {
        double newValue = GetValue();
        
        if(currentValue_ != newValue)
        {
            currentValue_ = newValue;
            
            SetWidgetValue(surfaceName, currentValue_ == autoMode_ ? 1 : 0);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode_Action : public GlobalAutoMode_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode_Action(string name, Interactor* interactor, int autoMode) : GlobalAutoMode_Action(name, interactor, autoMode) {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SetAutomationMode(autoMode_, true); }

    virtual double GetValue () override
    {
        for(int i = 0; i < DAW::GetNumTracks(); i++)
        {
            MediaTrack *track = DAW::CSurf_TrackFromID(i, false);
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
                return DAW::GetMediaTrackInfo_Value(track, "I_AUTOMODE");
        }
        
        return 0.00;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PreviousMarker_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PreviousMarker_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_MARKER_PREV, 0); }
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMarker_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NextMarker_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_MARKER_NEXT, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class InsertMarker_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    InsertMarker_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_INSERT_MARKER, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class InsertMarkerRegion_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    InsertMarkerRegion_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_INSERT_MARKERRGN, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override { return DAW::GetSetRepeatEx(nullptr, -1); }
    
    virtual void RunAction(string surfaceName, double value) override { DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1)); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Metronome_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Metronome_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}

    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_METRONOME, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Save_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Save_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_FILE_SAVEPROJECT, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SaveAs_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SaveAs_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, ID_FILE_SAVEAS, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Undo_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Undo_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, IDC_EDIT_UNDO, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Redo_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Redo_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void RunAction(string surfaceName, double value) override { DAW::SendMessage(WM_COMMAND, IDC_EDIT_REDO, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Cancel_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Cancel_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Enter_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Enter_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}

    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channel_ = 0;
    
protected:
    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
            GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
        else
            GetInteractor()->SetWidgetValue(surfaceName, GetName(), GetInteractor()->GetLogicalSurface()->GetManager()->GetVUMinDB());
    }
    
public:
    VUMeter_Action(string name, Interactor* interactor, int channel) : DoubleAction(name, interactor), channel_(channel) {}
    
    virtual double GetValue() override
    {
        return VAL2DB(DAW::Track_GetPeakInfo(GetInteractor()->GetTrack(), channel_));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GainReductionMeter_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
            GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
        else
            GetInteractor()->SetWidgetValue(surfaceName, GetName(), 0.0);
    }
    
public:
    GainReductionMeter_Action(string name, Interactor* interactor) : DoubleAction(name, interactor) {}
    
    virtual double GetValue() override
    {
        char buffer[256];
        

       if(TrackFX_GetNamedConfigParm(GetInteractor()->GetTrack(), GetInteractor()->GetIndex(), "GainReduction_dB", buffer, sizeof(buffer)))
       {
           return atof(buffer) * 3.0;
       }
       else
       {
            return 0.0;
       }

        
         //return VAL2DB(GetDAW()->Track_GetPeakInfo(GetInteractor()->GetTrack(), 0));
    }
};
/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IndicateOver0db_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    IndicateOver0db_Action(string name, CSurfManager* manager, TrackInteractor* interactor, double initialValue) : Interactor_DoubleAction(name, manager, interactor, initialValue) {}
    
    virtual void ForceUpdate() override {}
    virtual void RunAction(double value) override {}
    
    virtual void Update() override
    {
        if( ! DAW()->GetMediaTrackInfo_Value(Interactor()->Track(), "B_MUTE"))
        {
            MediaTrack* track = Interactor()->Track();
            
            int* newValuePtr = (int *)DAW()->GetSetMediaTrackInfo(track, "I_PEAKINFO", NULL);
            
            int newValue = *newValuePtr;
            
            if(currentValue_ != newValue)
            {
                currentValue_ = newValue;
                
                if(currentValue_ > 0)
                    Manager()->SetWidgetValue(Interactor()->TrackGUID(), Name(), 0x7f);
                else
                    Manager()->SetWidgetValue(Interactor()->TrackGUID(), Name(), 0x00);
            }
        }
    }
};
*/
#endif /* control_surface_Reaper_actions_h */
