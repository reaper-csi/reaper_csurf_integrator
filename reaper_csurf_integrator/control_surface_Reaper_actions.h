//
//  control_surface_Reaper_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_Reaper_actions_h
#define control_surface_Reaper_actions_h

#include "control_surface_action_contexts.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParam : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParam(WidgetActionManager* manager, vector<string> params) : FXAction(manager, params) {}
    
    virtual void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            DAW::TrackFX_SetParam(track, fxIndex_, fxParamIndex_, value);
    }
    
    virtual void DoToggle(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            double min, max = 0;
            double toggledValue = ! DAW::TrackFX_GetParam(track, fxIndex_, fxParamIndex_, &min, &max);
            DAW::TrackFX_SetParam(track, fxIndex_, fxParamIndex_, toggledValue);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamRelative : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamRelative(WidgetActionManager* manager, vector<string> params) : FXAction(manager, params) {}
       
    virtual void Do(double relativeValue) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            double min, max = 0;
            double value = DAW::TrackFX_GetParam(track, fxIndex_, fxParamIndex_, &min, &max);
            value +=  relativeValue;
            
            DAW::TrackFX_SetParam(track, fxIndex_, fxParamIndex_, value);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(track, &vol, &pan);
        SetWidgetValue(widget_, volToNormalized(vol));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MasterTrackVolume : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MasterTrackVolume(WidgetActionManager* manager) : Action(manager) {}
    
    void RequestUpdate() override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(DAW::GetMasterTrack(0), &vol, &pan);
        SetWidgetValue(widget_, volToNormalized(vol));
    }
    
    void Do(double value) override
    {
        DAW::CSurf_SetSurfaceVolume(DAW::GetMasterTrack(0), DAW::CSurf_OnVolumeChange(DAW::GetMasterTrack(0), normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPan(WidgetActionManager* manager, vector<string> params) : TrackActionWithIntParam(manager, params) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(track, &vol, &pan);
        SetWidgetValue(widget_, param_, panToNormalized(pan));
    }

    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPanWidth(WidgetActionManager* manager, vector<string> params) : TrackActionWithIntParam(manager, params) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, param_, panToNormalized(DAW::GetMediaTrackInfo_Value(track, "D_WIDTH")));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            DAW::CSurf_OnWidthChange(track, normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackNameDisplay(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        string trackName = "";
        
        if(DAW::GetMediaTrackInfo_Value(track , "IP_TRACKNUMBER") == -1)
            trackName = "Master";
        else
            trackName =  (char *)DAW::GetSetMediaTrackInfo(track, "P_NAME", NULL);
        
        SetWidgetValue(widget_, trackName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolumeDisplay(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        char trackVolume[128];
        sprintf(trackVolume, "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(track, "D_VOL")));
        SetWidgetValue(widget_, string(trackVolume));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolume : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendVolume(WidgetActionManager* manager) : TrackSendAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackSendUIVolPan(track, sendIndex_, &vol, &pan);
        SetWidgetValue(widget_, volToNormalized(vol));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, sendIndex_, normalizedToVol(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, sendIndex_, "D_VOL", &volume);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendPan : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendPan(WidgetActionManager* manager) : TrackSendAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackSendUIVolPan(track, sendIndex_, &vol, &pan);
        SetWidgetValue(widget_, panToNormalized(pan));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            double pan = DAW::CSurf_OnSendPanChange(track, sendIndex_, normalizedToPan(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, sendIndex_, "D_PAN", &pan);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendMute : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendMute(WidgetActionManager* manager) : TrackSendAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        bool mute = false;
        DAW::GetTrackSendUIMute(track, sendIndex_, &mute);
        SetWidgetValue(widget_, mute);
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            bool isMuted = ! DAW::GetTrackSendInfo_Value(track, 0, sendIndex_, "B_MUTE");
            
            DAW::GetSetTrackSendInfo(track, 0, sendIndex_, "B_MUTE", &isMuted);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendInvertPolarity : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendInvertPolarity(WidgetActionManager* manager) : TrackSendAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, DAW::GetTrackSendInfo_Value(track, 0, sendIndex_, "B_PHASE"));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            bool reversed = ! DAW::GetTrackSendInfo_Value(track, 0, sendIndex_, "B_PHASE");
            
            DAW::GetSetTrackSendInfo(track, 0, sendIndex_, "B_PHASE", &reversed);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendPrePost : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendPrePost(WidgetActionManager* manager) : TrackSendAction(manager) {}
    
    void RequestTrackUpdate( MediaTrack* track) override
    {
        if(DAW::GetTrackSendInfo_Value(track, 0, sendIndex_, "I_SENDMODE") == 0)
            SetWidgetValue(widget_, 0);
        else
            SetWidgetValue(widget_, 1);
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            bool isPre = DAW::GetTrackSendInfo_Value(track, 0, sendIndex_, "I_SENDMODE") == 0 ? 0 : 1;
            
            if(isPre == 0)
                isPre = 3; // switch to post FX
            else
                isPre = 0; // switch to post fader
            
            DAW::GetSetTrackSendInfo(track, 0, sendIndex_, "I_SENDMODE", &isPre);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXNameDisplay : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXNameDisplay(WidgetActionManager* manager, vector<string> params) : TrackActionWithIntParam(manager, params) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            int param = param_ - 1 < 0 ? 0 : param_ - 1;
            
            char fxName[BUFSZ];
            
            DAW::TrackFX_GetFXName(track, param, fxName, sizeof(fxName));

            SetWidgetValue(widget_, widget_->GetSurface()->GetLocalZoneAlias(fxName));
        }
        else
            widget_->Reset();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamNameDisplay : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamNameDisplay(WidgetActionManager* manager, vector<string> params) : FXAction(manager, params) {}

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            SetWidgetValue(widget_, GetDisplayName());
        else
            widget_->Reset();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamValueDisplay : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamValueDisplay(WidgetActionManager* manager, vector<string> params) : FXAction(manager, params) {}

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            char fxParamValue[128];
            DAW::TrackFX_GetFormattedParamValue(track, fxIndex_, fxParamIndex_, fxParamValue, sizeof(fxParamValue));
            SetWidgetValue(widget_, string(fxParamValue));
        }
        else
            widget_->Reset();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendNameDisplay : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendNameDisplay(WidgetActionManager* manager) : TrackSendAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        string sendTrackName = "";
        MediaTrack* destTrack = (MediaTrack *)DAW::GetSetTrackSendInfo(track, 0, sendIndex_, "P_DESTTRACK", 0);;
        if(destTrack)
            sendTrackName = (char *)DAW::GetSetMediaTrackInfo(destTrack, "P_NAME", NULL);
        SetWidgetValue(widget_, sendTrackName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolumeDisplay : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendVolumeDisplay(WidgetActionManager* manager) : TrackSendAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        char trackVolume[128];
        sprintf(trackVolume, "%7.2lf", VAL2DB(DAW::GetTrackSendInfo_Value(track, 0, sendIndex_, "D_VOL")));
        SetWidgetValue(widget_, string(trackVolume));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPanDisplay(WidgetActionManager* manager) : TrackAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        bool left = false;
        
        double panVal = DAW::GetMediaTrackInfo_Value(track, "D_PAN");
        
        if(panVal < 0)
        {
            left = true;
            panVal = -panVal;
        }
        
        int panIntVal = int(panVal * 100.0);
        string trackPan = "";
        
        if(left)
        {
            if(panIntVal == 100)
                trackPan += "<";
            else if(panIntVal < 100 && panIntVal > 9)
                trackPan += "< ";
            else
                trackPan += "<  ";
            
            trackPan += to_string(panIntVal);
        }
        else
        {
            trackPan += "   ";
            
            trackPan += to_string(panIntVal);
            
            if(panIntVal == 100)
                trackPan += ">";
            else if(panIntVal < 100 && panIntVal > 9)
                trackPan += " >";
            else
                trackPan += "  >";
        }
        
        if(panIntVal == 0)
            trackPan = "  <C>  ";
        
        SetWidgetValue(widget_, string(trackPan));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPanWidthDisplay(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        bool reversed = false;
        
        double widthVal = DAW::GetMediaTrackInfo_Value(track, "D_WIDTH");
        
        if(widthVal < 0)
        {
            reversed = true;
            widthVal = -widthVal;
        }
        
        int widthIntVal = int(widthVal * 100.0);
        string trackPanWidth = "";
        
        if(reversed)
            trackPanWidth += "Rev ";
        
        trackPanWidth += to_string(widthIntVal);
        
        if(widthIntVal == 0)
            trackPanWidth = " <Mno> ";

        SetWidgetValue(widget_, string(trackPanWidth));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind(WidgetActionManager* manager) : Action(manager) { }

    void Do(double value) override
    {
        DAW::CSurf_OnRew(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FastForward(WidgetActionManager* manager) : Action(manager) { }

    void Do(double value) override
    {
        DAW::CSurf_OnFwd(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Play(WidgetActionManager* manager) : Action(manager) { }

    void RequestUpdate() override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            playState = 1;
        else playState = 0;
        SetWidgetValue(widget_, playState);
    }
    
    void Do(double value) override
    {
        DAW::CSurf_OnPlay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Stop(WidgetActionManager* manager) : Action(manager) { }

    void RequestUpdate() override
    {
        int stopState = DAW::GetPlayState();
        if(stopState == 0 || stopState == 2 || stopState == 6) // stopped or paused or paused whilst recording
            stopState = 1;
        else stopState = 0;
        
        SetWidgetValue(widget_, stopState);
    }
    
    void Do(double value) override
    {
        DAW::CSurf_OnStop();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Record(WidgetActionManager* manager) : Action(manager) { }

    void RequestUpdate() override
    {
        int recordState = DAW::GetPlayState();
        if(recordState == 5 || recordState == 6) // recording or paused whilst recording
            recordState = 1;
        else recordState = 0;
        
        SetWidgetValue(widget_, recordState);
    }
    
    void Do(double value) override
    {
        DAW::CSurf_OnRecord();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackFolderDive : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackFolderDive(WidgetActionManager* manager) : TrackAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double folderDepth = DAW::GetMediaTrackInfo_Value(track, "I_FOLDERDEPTH");
        
        if(folderDepth == 1)
            SetWidgetValue(widget_, 1);
        else
            SetWidgetValue(widget_, 0);
    }
    
    void Do(double value) override
    {
        /*
        if(MediaTrack* track = widget_->GetTrack())
        {
            DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
            widget_->GetSurface()->GetPage()->OnTrackSelectionBySurface(track);
        }
         */
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelect(WidgetActionManager* manager) : TrackAction(manager) {}
    
    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
            widget_->GetSurface()->GetPage()->OnTrackSelectionBySurface(track);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            DAW::SetOnlyTrackSelected(track);
            widget_->GetSurface()->GetPage()->OnTrackSelectionBySurface(track);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MasterTrackUniqueSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    MasterTrackUniqueSelect(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, DAW::GetMediaTrackInfo_Value(DAW::GetMasterTrack(0), "I_SELECTED"));
    }
    
    void Do( double value) override
    {
        DAW::SetOnlyTrackSelected(DAW::GetMasterTrack(0));
        widget_->GetSurface()->GetPage()->OnTrackSelectionBySurface(GetMasterTrack(0));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRangeSelect(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }

    virtual void Do(double value) override
    {
        int currentlySelectedCount = 0;
        int selectedTrackIndex = 0;
        int trackIndex = 0;
        
        Page* page = widget_->GetSurface()->GetPage();
        
        for(int i = 0; i < page->GetTrackNavigationManager()->GetNumTracks(); i++)
        {
           MediaTrack* currentTrack = page->GetTrackNavigationManager()->GetTrackFromId(i);
            
            if(currentTrack == widget_->GetTrack())
                trackIndex = i;
            
            if(DAW::GetMediaTrackInfo_Value(currentTrack, "I_SELECTED"))
            {
                selectedTrackIndex = i;
                currentlySelectedCount++;
            }
        }
        
        if(currentlySelectedCount != 1)
            return;
        
        int lowerBound = trackIndex < selectedTrackIndex ? trackIndex : selectedTrackIndex;
        int upperBound = trackIndex > selectedTrackIndex ? trackIndex : selectedTrackIndex;

        for(int i = lowerBound; i <= upperBound; i++)
        {
            MediaTrack* currentTrack = page->GetTrackNavigationManager()->GetTrackFromId(i);
            
            DAW::CSurf_SetSurfaceSelected(currentTrack, DAW::CSurf_OnSelectedChange(currentTrack, 1), NULL);
        }
        
        page->OnTrackSelectionBySurface(page->GetTrackNavigationManager()->GetTrackFromId(lowerBound));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRecordArm(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, DAW::GetMediaTrackInfo_Value(track, "I_RECARM"));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            DAW::CSurf_SetSurfaceRecArm(track, DAW::CSurf_OnRecArmChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_RECARM")), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        bool mute = false;
        DAW::GetTrackUIMute(track, &mute);
        SetWidgetValue(widget_, mute);
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            bool mute = false;
            DAW::GetTrackUIMute(track, &mute);
            DAW::CSurf_SetSurfaceMute(track, DAW::CSurf_OnMuteChange(track, ! mute), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, DAW::GetMediaTrackInfo_Value(track, "I_SOLO"));
    }
    
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            DAW::CSurf_SetSurfaceSolo(track, DAW::CSurf_OnSoloChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SOLO")), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetTrackTouch : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetTrackTouch(WidgetActionManager* manager) : TrackAction(manager) {}

    void Do(double value) override
    {
        widget_->SetIsTouched(value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetMasterTrackTouch : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetMasterTrackTouch(WidgetActionManager* manager) : Action(manager) {}

    void Do(double value) override
    {
        // GAW TBD -- if anyone ever asks for it :)
        //page->SetTouchState(DAW::GetMasterTrack(0), value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GlobalAutoMode(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, DAW::GetGlobalAutomationOverride());
    }
    
    void Do(double value) override
    {
        DAW::SetGlobalAutomationOverride(param_);
    }
};


// I_AUTOMODE : int * : track automation mode (0=trim/off, 1=read, 2=touch, 3=write, 4=latch)


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params) {}

    void RequestUpdate() override
    {
        bool gotOne = false;
        
        for(int i = 0; i < page_->GetTrackNavigationManager()->GetNumTracks(); i++)
        {
            MediaTrack* track = page_->GetTrackNavigationManager()->GetTrackFromId(i);
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED") && DAW::GetMediaTrackInfo_Value(track, "I_AUTOMODE") == param_)
            {
                gotOne = true;
                break;
            }
        }
        
        if(gotOne)
            SetWidgetValue(widget_, 1.0);
        else
            SetWidgetValue(widget_, 0.0);
    }
    
    virtual void Do(double value) override
    {
        DAW::SetAutomationMode(param_, true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TimeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TimeDisplay(WidgetActionManager* manager) : Action(manager) { }
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, DAW::GetSetRepeatEx(nullptr, -1));
    }
    
    void Do(double value) override
    {
        DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeter : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackOutputMeter(WidgetActionManager* manager, vector<string> params) : TrackActionWithIntParam(manager, params) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(widget_, volToNormalized(DAW::Track_GetPeakInfo(track, param_)));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeterAverageLR : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackOutputMeterAverageLR(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        double lrVol = (DAW::Track_GetPeakInfo(track, 0) + DAW::Track_GetPeakInfo(track, 1)) / 2.0;
        
        SetWidgetValue(widget_, volToNormalized(lrVol));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeterMaxPeakLR : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackOutputMeterMaxPeakLR(WidgetActionManager* manager) : TrackAction(manager) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        double lVol = DAW::Track_GetPeakInfo(track, 0);
        double rVol = DAW::Track_GetPeakInfo(track, 1);

        double lrVol =  lVol > rVol ? lVol : rVol;
        
        SetWidgetValue(widget_, volToNormalized(lrVol));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MasterTrackOutputMeter : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MasterTrackOutputMeter(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, param_, volToNormalized(DAW::Track_GetPeakInfo(DAW::GetMasterTrack(0), param_))); // param 0=left, 1=right, etc.
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXGainReductionMeter : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXGainReductionMeter(WidgetActionManager* manager, vector<string> params) : FXAction(manager, params) {}

    void RequestUpdate() override
    {
        char buffer[BUFSZ];
        
        if(MediaTrack* track = widget_->GetTrack())
        {
            if(DAW::TrackFX_GetNamedConfigParm(track, fxParamIndex_, "GainReduction_dB", buffer, sizeof(buffer)))
                SetWidgetValue(widget_, -atof(buffer)/20.0);
            else
                SetWidgetValue(widget_, 0.0);
        }
        else
            widget_->Reset();
    }
};

#endif /* control_surface_Reaper_actions_h */
