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
class TrackFX_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string paramName_ = "";
    int paramIndex_ = 0;
    
public:
    TrackFX_Action(string name, CSurfManager* manager, Interactor* interactor, string paramName, int paramIndex) : Interactor_DoubleAction(name, manager, interactor), paramName_(paramName), paramIndex_(paramIndex) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return currentValue_;
    }
    
    virtual void SetWidgetValue(double value) override
    {
        GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), value);
    }
    
    virtual double GetValue() override
    {
        double min = 0;
        double max = 0;
        return GetDAW()->TrackFX_GetParam(GetInteractor()->GetTrack(), GetInteractor()->GetIndex(), paramIndex_, &min, &max);
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->TrackFX_SetParam(GetInteractor()->GetTrack(), GetInteractor()->GetIndex(), paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return volToNormalized(currentValue_);
    }

    virtual void SetWidgetValue(double value) override
    {
        GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), volToNormalized(value));
    }

    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_VOL");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_SetSurfaceVolume(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnVolumeChange(GetInteractor()->GetTrack(), normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
protected:
    virtual void SetWidgetValue(double value) override
    {
        GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), panToNormalized(value), displayMode_);
    }
    
public:
    TrackPan_Action(string name, CSurfManager* manager, Interactor* interactor, int displayMode) : Interactor_DoubleAction(name, manager, interactor), displayMode_(displayMode) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return panToNormalized(currentValue_);
    }

    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_PAN");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_SetSurfacePan(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnPanChange(GetInteractor()->GetTrack(), normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
protected:
    virtual void SetWidgetValue(double value) override
    {
        GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), panToNormalized(value), displayMode_);
    }
    
public:
    TrackPanWidth_Action(string name, CSurfManager* manager, Interactor* interactor, int displayMode) : Interactor_DoubleAction(name, manager, interactor), displayMode_(displayMode) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return panToNormalized(currentValue_);
    }

    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_WIDTH");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_OnWidthChange(GetInteractor()->GetTrack(), normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackName_DisplayAction : public Interactor_StringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackName_DisplayAction(string name, CSurfManager* manager, Interactor* interactor) : Interactor_StringDisplayAction(name, manager, interactor) {}
    
    virtual string GetValue() override
    {
        if(GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "IP_TRACKNUMBER") == -1)
            return "Master";
        else
            return (char *)GetDAW()->GetSetMediaTrackInfo(GetInteractor()->GetTrack(), "P_NAME", NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_DisplayAction : public Interactor_StringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_DisplayAction(string name, CSurfManager* manager, Interactor* interactor) : Interactor_StringDisplayAction(name, manager, interactor) {}
    
    virtual string GetValue() override
    {
        double value = GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "D_VOL");
            
        double DBValue = VAL2DB(value);

        char buffer[128];
        sprintf(buffer, "%7.2lf", DBValue);
        return string(buffer);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->CSurf_OnRew(1); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FastForward_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf) {}
    
    virtual void RunAction(double value) override { GetDAW()->CSurf_OnFwd(1); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Play_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf) {}
    
    virtual void RunAction(double value) override { GetDAW()->CSurf_OnPlay(); }
    
    virtual double GetValue() override
    {
        int playState = GetDAW()->GetPlayState();
        
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            return 1;
        else return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Stop_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf) {}
    
    virtual void RunAction(double value) override { GetDAW()->CSurf_OnStop(); }
    
    virtual double GetValue() override
    {
        int playState = GetDAW()->GetPlayState();
        
        if(playState == 0 || playState == 2 || playState == 6) // stopped or paused or paused whilst recording
            return 1;
        else
            return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Record_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf) {}
    
    virtual void RunAction(double value) override { GetDAW()->CSurf_OnRecord(); }
    
    virtual double GetValue() override
    {
        int playState = GetDAW()->GetPlayState();
        
        if(playState == 5 || playState == 6) // recording or paused whilst recording
            return 1;
        else
            return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RepeatingArrow_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int direction_ = 0;
    clock_t lastRepeated = clock();
    double repeatRate_ = 0.0;
    bool pressed_ = false;
    
public:
    RepeatingArrow_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf, int direction, double repeatRate) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf), direction_(direction), repeatRate_(repeatRate) {}

    virtual void Update() override
    {
        if(pressed_ && clock() - lastRepeated >  CLOCKS_PER_SEC * repeatRate_)
        {
            lastRepeated = clock();
            GetDAW()->CSurf_OnArrow(direction_, GetSurface()->IsZoom());
        }
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_OnArrow(direction_, GetSurface()->IsZoom());
        pressed_ = value;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelect_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_SetSurfaceSelected(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnSelectedChange(GetInteractor()->GetTrack(), ! GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED")), NULL);
        GetManager()->OnTrackSelection(GetInteractor()->GetTrack());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->SetOnlyTrackSelected(GetInteractor()->GetTrack());
        GetManager()->OnTrackSelection(GetInteractor()->GetTrack());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelectionSelect_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelectionSelect_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SELECTED");
    }
    
    virtual void RunAction(double value) override
    {
        int selectedTrackNum = 0;
        int selectionTrackNumber = GetDAW()->CSurf_TrackToID(GetInteractor()->GetTrack(), false);
        
        if(GetDAW()->CountSelectedTracks(nullptr) == 1)
        {
            for(int i = 0; i < GetDAW()->GetNumTracks(); i++)
                if(GetDAW()->GetMediaTrackInfo_Value(GetDAW()->CSurf_TrackFromID(i, false), "I_SELECTED"))
                    selectedTrackNum = i;

            int lowerBound = selectionTrackNumber < selectedTrackNum ? selectionTrackNumber : selectedTrackNum;
            int upperBound = selectionTrackNumber > selectedTrackNum ? selectionTrackNumber : selectedTrackNum;
            
            for(int i = lowerBound; i <= upperBound; i++)
            {
                GetDAW()->CSurf_SetSurfaceSelected(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnSelectedChange(GetDAW()->CSurf_TrackFromID(i, false), 1), NULL);
                GetManager()->OnTrackSelection(GetDAW()->CSurf_TrackFromID(i, false));
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRecordArm_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
   
    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_RECARM");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_SetSurfaceRecArm(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnRecArmChange(GetInteractor()->GetTrack(), ! GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "B_MUTE");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_SetSurfaceMute(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnMuteChange(GetInteractor()->GetTrack(), ! GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
    virtual double GetValue() override
    {
        return GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SOLO");
    }
    
    virtual void RunAction(double value) override
    {
        GetDAW()->CSurf_SetSurfaceSolo(GetInteractor()->GetTrack(), GetDAW()->CSurf_OnSoloChange(GetInteractor()->GetTrack(), ! GetDAW()->GetMediaTrackInfo_Value(GetInteractor()->GetTrack(), "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TouchStateControlled_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* controlledAction_= nullptr;
    bool currentlyTouched_ = false;
    bool lastTouched_ = true;
    
public:
    TouchStateControlled_Action(string name, CSurfManager* manager, Interactor* interactor, Action* controlledAction) : Interactor_DoubleAction(name, manager, interactor), controlledAction_(controlledAction) {}
    
    virtual string GetAlias() override { return controlledAction_->GetName(); }

    virtual double GetValue() override { return currentlyTouched_; }

    virtual void RunAction(double value) override { currentlyTouched_ =  value == 0 ? false : true; }

    virtual void Update() override
    {
        if(currentlyTouched_)
            controlledAction_->Update();

        if(lastTouched_ != currentlyTouched_)
        {
            lastTouched_ = currentlyTouched_;
            
            if(currentlyTouched_ == false)
                GetInteractor()->ForceUpdate(GetAlias());
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int autoMode_ = 0;
    
public:
    GlobalAutoMode_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf, int autoMode) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf), autoMode_(autoMode) {}
    
    virtual double GetValue () override { return GetDAW()->GetGlobalAutomationOverride(); }
    
    virtual void RunAction(double value) override {  GetDAW()->SetGlobalAutomationOverride(autoMode_); }
    
    virtual void ForceUpdate() override
    {
        if(GetValue() == autoMode_)
            SetWidgetValue(autoMode_);
    }
    
    virtual void Update() override
    {
        double newValue = GetValue();
        
        if(currentValue_ != newValue)
        {
            currentValue_ = newValue;
            
            SetWidgetValue(currentValue_ == autoMode_ ? 1 : 0);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode_Action : public GlobalAutoMode_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf, int autoMode) : GlobalAutoMode_Action(name, manager, logicalCSurf, autoMode) {}
    
    virtual void RunAction(double value) override { GetDAW()->SetAutomationMode(autoMode_, true); }

    virtual double GetValue () override
    {
        for(int i = 0; i < GetDAW()->GetNumTracks(); i++)
        {
            MediaTrack *track = GetDAW()->CSurf_TrackFromID(i, false);
            
            if(GetDAW()->GetMediaTrackInfo_Value(track, "I_SELECTED"))
                return GetDAW()->GetMediaTrackInfo_Value(track, "I_AUTOMODE");
        }
        
        return 0.00;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PreviousMarker_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PreviousMarker_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_MARKER_PREV, 0); }
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMarker_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NextMarker_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_MARKER_NEXT, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class InsertMarker_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    InsertMarker_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_INSERT_MARKER, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class InsertMarkerRegion_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    InsertMarkerRegion_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_INSERT_MARKERRGN, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual double GetValue() override { return GetDAW()->GetSetRepeatEx(nullptr, -1); }
    
    virtual void RunAction(double value) override { GetDAW()->GetSetRepeatEx(nullptr, ! GetManager()->GetDAW()->GetSetRepeatEx(nullptr, -1)); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Metronome_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Metronome_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}

    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_METRONOME, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Save_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Save_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_FILE_SAVEPROJECT, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SaveAs_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SaveAs_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, ID_FILE_SAVEAS, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Undo_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Undo_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, IDC_EDIT_UNDO, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Redo_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Redo_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override { GetDAW()->SendMessage(WM_COMMAND, IDC_EDIT_REDO, 0); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Cancel_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Cancel_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Enter_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Enter_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}

    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channel_ = 0;
    
protected:
    virtual void SetWidgetValue(double value) override
    {
        if(GetDAW()->GetPlayState() & 0x01) // if playing
            GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), value);
        else
            GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), GetManager()->GetVUMinDB());
    }
    
public:
    VUMeter_Action(string name, CSurfManager* manager, Interactor* interactor, int channel) : Interactor_DoubleAction(name, manager, interactor), channel_(channel) {}
    
    virtual double GetValue() override
    {
        return VAL2DB(GetDAW()->Track_GetPeakInfo(GetInteractor()->GetTrack(), channel_));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GainReductionMeter_Action : public Interactor_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    virtual void SetWidgetValue(double value) override
    {
        if(GetDAW()->GetPlayState() & 0x01) // if playing
            GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), value);
        else
            GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), 0.0);
    }
    
public:
    GainReductionMeter_Action(string name, CSurfManager* manager, Interactor* interactor) : Interactor_DoubleAction(name, manager, interactor) {}
    
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
