//
//  control_surface_Reaper_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_Reaper_actions_h
#define control_surface_Reaper_actions_h

#include "control_surface_base_actions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackDoubleAction : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    MediaTrack* track_;
    
public:
    TrackDoubleAction(LogicalSurface* logicalSurface, MediaTrack* track) : DoubleAction(logicalSurface), track_(track) {}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackStringDisplayAction : public StringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    MediaTrack* track_;
    
public:
    TrackStringDisplayAction(LogicalSurface* logicalSurface, MediaTrack* track) : StringDisplayAction(logicalSurface), track_(track) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackFX_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int fxIndex_ = 0;
    int paramIndex_ = 0;
    
public:
    TrackFX_Action(LogicalSurface* logicalSurface, MediaTrack* track, int fxIndex, int paramIndex) : TrackDoubleAction(logicalSurface, track), fxIndex_(fxIndex), paramIndex_(paramIndex) {}
    
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
    }
    
    virtual double GetValue(string surfaceName, string widgetName) override
    {
        double min = 0;
        double max = 0;
        return DAW::TrackFX_GetParam(track_, fxIndex_, paramIndex_, &min, &max);
    }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::TrackFX_SetParam(track_, fxIndex_, paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
 
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, volToNormalized(value));
    }
    
    virtual double GetCurrentNormalizedValue() override { return volToNormalized(currentValue_); }

    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "D_VOL"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
         DAW::CSurf_SetSurfaceVolume(track_, DAW::CSurf_OnVolumeChange(track_, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
public:
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
    TrackPan_Action(LogicalSurface* logicalSurface, MediaTrack* track, int displayMode) : TrackDoubleAction(logicalSurface, track), displayMode_(displayMode) {}

    virtual double GetCurrentNormalizedValue() override { return panToNormalized(currentValue_); }

    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "D_PAN"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfacePan(track_, DAW::CSurf_OnPanChange(track_, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
protected:
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
public:
    TrackPanWidth_Action(LogicalSurface* logicalSurface, MediaTrack* track, int displayMode) : TrackDoubleAction(logicalSurface, track), displayMode_(displayMode) {}
    
    virtual double GetCurrentNormalizedValue() override { return panToNormalized(currentValue_); }

    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "D_WIDTH"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnWidthChange(track_, normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackName_DisplayAction : public TrackStringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackName_DisplayAction(LogicalSurface* logicalSurface, MediaTrack* track) : TrackStringDisplayAction(logicalSurface, track) {}
    
    virtual string GetValue(string surfaceName, string widgetName) override
    {
        if(DAW::GetMediaTrackInfo_Value(track_, "IP_TRACKNUMBER") == -1)
            return "Master";
        else
            return (char *)DAW::GetSetMediaTrackInfo(track_, "P_NAME", NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_DisplayAction : public TrackStringDisplayAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_DisplayAction(LogicalSurface* logicalSurface, MediaTrack* track) : TrackStringDisplayAction(logicalSurface, track) {}
    
    virtual string GetValue(string surfaceName, string widgetName) override
    {
        char buffer[128];
        sprintf(buffer, "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(track_, "D_VOL")));
        return string(buffer);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnRew(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FastForward_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface) {}
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnFwd(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Play_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            return 1;
        else return 0;
    }

    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnPlay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Stop_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override
    {
        int playState = DAW::GetPlayState();
        if(playState == 0 || playState == 2 || playState == 6) // stopped or paused or paused whilst recording
            return 1;
        else
            return 0;
    }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnStop();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Record_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override
    {
        int playState = DAW::GetPlayState();
        
        if(playState == 5 || playState == 6) // recording or paused whilst recording
            return 1;
        else
            return 0;
    }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnRecord();
    }
};

/*
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
            DAW::CSurf_OnArrow(direction_, GetInteractor()->GetLogicalSurface()->GetSurface(surfaceName)->IsZoom());
        }
    }
    
    virtual void RunAction(string surfaceName, double value) override
    {
        DAW::CSurf_OnArrow(direction_, GetInteractor()->GetLogicalSurface()->GetSurface(surfaceName)->IsZoom());
        pressed_ = value;
    }
};
 */

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelect_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceSelected(track_, DAW::CSurf_OnSelectedChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED")), NULL);
        GetLogicalSurface()->GetManager()->OnTrackSelection(track_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::SetOnlyTrackSelected(track_);
        GetLogicalSurface()->GetManager()->OnTrackSelection(track_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRangeSelect_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        int selectedTrackNum = DAW::CSurf_TrackToID(track_, false);
        int otherSelectedTrackNum = 0;

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            for(int i = 0; i < DAW::GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(DAW::CSurf_TrackFromID(i, false), "I_SELECTED"))
                {
                    otherSelectedTrackNum = i;
                    break;
                }

            int lowerBound = selectedTrackNum < otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            int upperBound = selectedTrackNum > otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            
            for(int i = lowerBound; i <= upperBound; i++)
            {
                DAW::CSurf_SetSurfaceSelected(track_, DAW::CSurf_OnSelectedChange(DAW::CSurf_TrackFromID(i, false), 1), NULL);
                GetLogicalSurface()->GetManager()->OnTrackSelection(DAW::CSurf_TrackFromID(i, false));
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRecordArm_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
   
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_RECARM"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceRecArm(track_, DAW::CSurf_OnRecArmChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "B_MUTE"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
         DAW::CSurf_SetSurfaceMute(track_, DAW::CSurf_OnMuteChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo_Action(LogicalSurface* logicalSurface, MediaTrack* track) : TrackDoubleAction(logicalSurface, track) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SOLO"); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceSolo(track_, DAW::CSurf_OnSoloChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TouchStateControlled_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string controlledActionWidgetName_ = "";
    string controlledActionAddress_ = "";
    Action* controlledAction_= nullptr;
    bool currentlyTouched_ = false;
    bool lastTouched_ = true;
    
public:
    TouchStateControlled_Action(LogicalSurface* logicalSurface, MediaTrack* track, string controlledActionwidgetName, string controlledActionAddress, Action* controlledAction) : TrackDoubleAction(logicalSurface, track), controlledActionWidgetName_(controlledActionwidgetName), controlledActionAddress_(controlledActionAddress), controlledAction_(controlledAction) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return currentlyTouched_; }

    virtual void Update(string surfaceName, string widgetName) override
    {
        if(currentlyTouched_)
            controlledAction_->Update(surfaceName, controlledActionWidgetName_);

        if(lastTouched_ != currentlyTouched_)
            if((lastTouched_ = currentlyTouched_) == false)
                GetLogicalSurface()->ForceUpdateAction(controlledActionAddress_, surfaceName, controlledActionWidgetName_);
    }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        currentlyTouched_ = value == 0 ? false : true;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int autoMode_ = 0;
    
public:
    GlobalAutoMode_Action(LogicalSurface* logicalSurface, int autoMode) : DoubleAction(logicalSurface), autoMode_(autoMode) {}
    
    virtual double GetValue (string surfaceName, string widgetName) override { return DAW::GetGlobalAutomationOverride(); }
    
    virtual void ForceUpdate(string surfaceName, string widgetName) override
    {
        if(GetValue(surfaceName, widgetName) == autoMode_)
            SetWidgetValue(surfaceName, widgetName, autoMode_);
    }
    
    virtual void Update(string surfaceName, string widgetName) override
    {
        double newValue = GetValue(surfaceName, widgetName);
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, widgetName, (currentValue_ = newValue) == autoMode_ ? 1 : 0);
    }

    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::SetGlobalAutomationOverride(autoMode_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode_Action : public GlobalAutoMode_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode_Action(LogicalSurface* logicalSurface, int autoMode) : GlobalAutoMode_Action(logicalSurface, autoMode) {}
    
    virtual double GetValue (string surfaceName, string widgetName) override
    {
        for(int i = 0; i < DAW::GetNumTracks(); i++)
        {
            MediaTrack *track = DAW::CSurf_TrackFromID(i, false);
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
                return DAW::GetMediaTrackInfo_Value(track, "I_AUTOMODE");
        }
        
        return 0.00;
    }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::SetAutomationMode(autoMode_, true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Reaper_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_= 0;
    
public:
    Reaper_Action(LogicalSurface* logicalSurface, int commandId) : DoubleAction(logicalSurface), commandId_(commandId)  {}
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::SendMessage(WM_COMMAND, commandId_, 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return DAW::GetSetRepeatEx(nullptr, -1); }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Cancel_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Cancel_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Enter_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Enter_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channel_ = 0;
    
protected:
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
            GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
        else
            GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, GetLogicalSurface()->GetManager()->GetVUMinDB());
    }
    
public:
    VUMeter_Action(LogicalSurface* logicalSurface, MediaTrack* track, int channel) : TrackDoubleAction(logicalSurface, track), channel_(channel) {}
    
    virtual double GetValue(string surfaceName, string widgetName) override { return VAL2DB(DAW::Track_GetPeakInfo(track_, channel_)); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GainReductionMeter_Action : public TrackDoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int fxIndex_ = 0;
    
protected:
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
            GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
        else
            GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, 0.0);
    }
    
public:
    GainReductionMeter_Action(LogicalSurface* logicalSurface, MediaTrack* track, int fxIndex) : TrackDoubleAction(logicalSurface, track), fxIndex_(fxIndex)  {}
    
    virtual double GetValue(string surfaceName, string widgetName) override
    {
        char buffer[256];
        // GAW TBD "* 3.0" is bogus should be scaled value based on GR widget range, we should query surfaceName, widgetName for that range
        if(DAW::TrackFX_GetNamedConfigParm(track_, fxIndex_, ReaperGainReduction_dB.c_str(), buffer, sizeof(buffer)))
           return atof(buffer) * 3.0;
        else
            return 0.0;
    }
};
#endif /* control_surface_Reaper_actions_h */
