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
    FXParam(Widget* widget, Zone* zone, vector<string> params) : FXAction(widget, zone, params) {}

    virtual void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::TrackFX_SetParam(track, zone_->GetSlotIndex(), paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamRelative : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamRelative(Widget* widget, Zone* zone, vector<string> params) : FXAction(widget, zone, params) {}

    virtual void Do(double relativeValue) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double min, max = 0;
            double value = DAW::TrackFX_GetParam(track, zone_->GetSlotIndex(), paramIndex_, &min, &max);
            value +=  relativeValue;
            
            if(value < min) value = min;
            if(value > max) value = max;
            
            DAW::TrackFX_SetParam(track, zone_->GetSlotIndex(), paramIndex_, value);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestUpdate() override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                double min = 0.0;
                double max = 0.0;
                double value = DAW::TrackFX_GetParam(track, fxSlotNum, fxParamNum, &min, &max);

                UpdateWidgetValue(value);
            }
        }
    }
    
public:
    FocusedFXParam(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void Do(double value) override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;

        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
            if(MediaTrack* track = GetTrackNavigationManager()->GetTrackFromId(trackNum))
                DAW::TrackFX_SetParam(track, fxSlotNum, fxParamNum, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(track, &vol, &pan);
        UpdateWidgetValue(volToNormalized(vol));
    }
    
public:
    TrackVolume(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SoftTakeover7BitTrackVolume : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SoftTakeover7BitTrackVolume(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double trackVolume, trackPan = 0.0;
            DAW::GetTrackUIVolPan(track, &trackVolume, &trackPan);
            trackVolume = volToNormalized(trackVolume);
            
            if( fabs(value - trackVolume) < 0.025) // GAW -- Magic number -- ne touche pas
                DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SoftTakeover14BitTrackVolume : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SoftTakeover14BitTrackVolume(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double trackVolume, trackPan = 0.0;
            DAW::GetTrackUIVolPan(track, &trackVolume, &trackPan);
            trackVolume = volToNormalized(trackVolume);
            
            if( fabs(value - trackVolume) < 0.0025) // GAW -- Magic number -- ne touche pas
                DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDB : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(track, &vol, &pan);
        UpdateWidgetValue(VAL2DB(vol));
    }
    
public:
    TrackVolumeDB(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params)  {}
    
    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, DB2VAL(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(track, &vol, &pan);
        UpdateWidgetValue(intParam_, panToNormalized(pan));
    }
    
public:
    TrackPan(Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanPercent : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackUIVolPan(track, &vol, &pan);
        UpdateWidgetValue(pan * 100.0);
    }
    
public:
    TrackPanPercent(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params)  {}
    
    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, value / 100.0, false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(intParam_, panToNormalized(DAW::GetMediaTrackInfo_Value(track, "D_WIDTH")));
    }
    
public:
    TrackPanWidth(Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::CSurf_OnWidthChange(track, normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthPercent : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "D_WIDTH") * 100.0);
    }
    
public:
    TrackPanWidthPercent(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            DAW::CSurf_OnWidthChange(track, value / 100.0, false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanLPercent : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "D_DUALPANL") * 100.0);
    }
    
public:
    TrackPanLPercent(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double panFromPercent = value / 100.0;
            DAW::GetSetMediaTrackInfo(track, "D_DUALPANL", &panFromPercent);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanRPercent : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "D_DUALPANR") * 100.0);
    }
    
public:
    TrackPanRPercent(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double panFromPercent = value / 100.0;
            DAW::GetSetMediaTrackInfo(track, "D_DUALPANR", &panFromPercent);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolume : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackSendUIVolPan(track, zone_->GetSlotIndex(), &vol, &pan);
        UpdateWidgetValue(volToNormalized(vol));
    }

public:
    TrackSendVolume(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, zone_->GetSlotIndex(), normalizedToVol(value), false);
            
            DAW::GetSetTrackSendInfo(zone_->GetNavigator()->GetTrack(), 0, zone_->GetSlotIndex(), "D_VOL", &volume);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolumeDB : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackSendUIVolPan(track, zone_->GetSlotIndex(), &vol, &pan);
        UpdateWidgetValue(VAL2DB(vol));
    }
    
public:
    TrackSendVolumeDB(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, zone_->GetSlotIndex(), DB2VAL(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, zone_->GetSlotIndex(), "D_VOL", &volume);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendPan : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackSendUIVolPan(track, zone_->GetSlotIndex(), &vol, &pan);
        UpdateWidgetValue(panToNormalized(pan));
    }
    
public:
    TrackSendPan(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            double pan = DAW::CSurf_OnSendPanChange(track, zone_->GetSlotIndex(), normalizedToPan(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, zone_->GetSlotIndex(), "D_PAN", &pan);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendMute : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        bool mute = false;
        DAW::GetTrackSendUIMute(track, zone_->GetSlotIndex(), &mute);
        UpdateWidgetValue(mute);
    }

public:
    TrackSendMute(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            bool isMuted = ! DAW::GetTrackSendInfo_Value(track, 0, zone_->GetSlotIndex(), "B_MUTE");
            
            DAW::GetSetTrackSendInfo(track, 0, zone_->GetSlotIndex(), "B_MUTE", &isMuted);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendInvertPolarity : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetTrackSendInfo_Value(track, 0, zone_->GetSlotIndex(), "B_PHASE"));
    }
    
public:
    TrackSendInvertPolarity(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            bool reversed = ! DAW::GetTrackSendInfo_Value(track, 0, zone_->GetSlotIndex(), "B_PHASE");
            
            DAW::GetSetTrackSendInfo(track, 0, zone_->GetSlotIndex(), "B_PHASE", &reversed);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendPrePost : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate( MediaTrack* track) override
    {
        if(DAW::GetTrackSendInfo_Value(track, 0, zone_->GetSlotIndex(), "I_SENDMODE") == 0)
            UpdateWidgetValue(0);
        else
            UpdateWidgetValue(1);
    }
    
public:
    TrackSendPrePost(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            int mode = DAW::GetTrackSendInfo_Value(track, 0, zone_->GetSlotIndex(), "I_SENDMODE");
            
            if(mode == 0)
                mode = 3; // switch to post FX
            else
                mode = 0; // switch to post fader
            
            DAW::GetSetTrackSendInfo(track, 0, zone_->GetSlotIndex(), "I_SENDMODE", &mode);
        }
    }
};


// GAW TBD -- do we even need this anymore ?
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXNameDisplay : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXNameDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(widget, zone, params) {}

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            int param = intParam_ - 1 < 0 ? 0 : intParam_ - 1;
            
            char fxName[BUFSZ];
            
            DAW::TrackFX_GetFXName(track, param, fxName, sizeof(fxName));

            //UpdateWidgetValue(GetSurface()->GetLocalZoneAlias(fxName));
        }
        else
             ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamNameDisplay : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamNameDisplay(Widget* widget, Zone* zone, vector<string> params) : FXAction(widget, zone, params) {}

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            UpdateWidgetValue(GetDisplayName());
        else
             ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamValueDisplay : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamValueDisplay(Widget* widget, Zone* zone, vector<string> params) : FXAction(widget, zone, params) {}

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            char fxParamValue[128];
            DAW::TrackFX_GetFormattedParamValue(track, zone_->GetSlotIndex(), paramIndex_, fxParamValue, sizeof(fxParamValue));
            UpdateWidgetValue(string(fxParamValue));
        }
        else
             ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParamNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXParamNameDisplay(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char fxParamName[128];
                DAW::TrackFX_GetParamName(track, fxSlotNum, fxParamNum, fxParamName, sizeof(fxParamName));
                UpdateWidgetValue(string(fxParamName));
            }
        }
        else
             ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParamValueDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXParamValueDisplay(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char fxParamValue[128];
                DAW::TrackFX_GetFormattedParamValue(track, fxSlotNum, fxParamNum, fxParamValue, sizeof(fxParamValue));
                UpdateWidgetValue(string(fxParamValue));
           }
        }
        else
             ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendNameDisplay : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        string sendTrackName = "";
        MediaTrack* destTrack = (MediaTrack *)DAW::GetSetTrackSendInfo(track, 0, zone_->GetSlotIndex(), "P_DESTTRACK", 0);;
        if(destTrack)
            sendTrackName = (char *)DAW::GetSetMediaTrackInfo(destTrack, "P_NAME", NULL);
        UpdateWidgetValue(sendTrackName);
    }

public:
    TrackSendNameDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolumeDisplay : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        char trackVolume[128];
        snprintf(trackVolume, sizeof(trackVolume), "%7.2lf", VAL2DB(DAW::GetTrackSendInfo_Value(track, 0, zone_->GetSlotIndex(), "D_VOL")));
        UpdateWidgetValue(string(trackVolume));
    }

public:
    TrackSendVolumeDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FixedTextDisplay : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestUpdate() override
    {
        UpdateWidgetValue(stringParam_);
    }
    
public:
    FixedTextDisplay(Widget* widget, Zone* zone, vector<string> params) : ActionWithStringParam(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FixedRGBColourDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestUpdate() override
    {
        UpdateWidgetValue(0);
    }
    
public:
    FixedRGBColourDisplay(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        char buf[BUFSZ];

        if(GetSurface()->GetIsEuConFXAreaFocused())
        {
            if(track == GetTrackNavigationManager()->GetSelectedTrack())
                DAW::GetTrackName(track, buf, sizeof(buf));
            else
                buf[0] = 0;
        }
        else
        {
            DAW::GetTrackName(track, buf, sizeof(buf));
        }
        
        UpdateWidgetValue(string(buf));
    }
    
public:
    TrackNameDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        char trackVolume[128];
        snprintf(trackVolume, sizeof(trackVolume), "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(track, "D_VOL")));
        UpdateWidgetValue(string(trackVolume));
    }
    
public:
    TrackVolumeDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
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
        
        UpdateWidgetValue(string(trackPan));
    }

public:
    TrackPanDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
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
        
        UpdateWidgetValue(string(trackPanWidth));
    }

public:
    TrackPanWidthDisplay(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::CSurf_OnRew(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FastForward(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::CSurf_OnFwd(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Play(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }

    void RequestUpdate() override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            playState = 1;
        else playState = 0;
        UpdateWidgetValue(playState);
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::CSurf_OnPlay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Stop(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }

    void RequestUpdate() override
    {
        int stopState = DAW::GetPlayState();
        if(stopState == 0 || stopState == 2 || stopState == 6) // stopped or paused or paused whilst recording
            stopState = 1;
        else stopState = 0;
        
        UpdateWidgetValue(stopState);
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::CSurf_OnStop();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Record(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }

    void RequestUpdate() override
    {
        int recordState = DAW::GetPlayState();
        if(recordState == 5 || recordState == 6) // recording or paused whilst recording
            recordState = 1;
        else recordState = 0;
        
        UpdateWidgetValue(recordState);
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::CSurf_OnRecord();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackToggleVCASpill : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackToggleVCASpill(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            GetTrackNavigationManager()->ToggleVCASpill(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
public:
    TrackSelect(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
            GetPage()->OnTrackSelectionBySurface(track);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
public:
    TrackUniqueSelect(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            DAW::SetOnlyTrackSelected(track);
            GetPage()->OnTrackSelectionBySurface(track);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
public:
    TrackRangeSelect(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    virtual void Do(double value) override
    {
        // GAW TBD  fix highest track bug 
        
        if(value == 0.0) return; // ignore button releases

        int currentlySelectedCount = 0;
        int selectedTrackIndex = 0;
        int trackIndex = 0;
        
       
        for(int i = 1; i <= GetTrackNavigationManager()->GetNumTracks(); i++)
        {
            MediaTrack* currentTrack = GetTrackNavigationManager()->GetTrackFromId(i);
           
            if(currentTrack == nullptr)
                continue;
            
            if(currentTrack == zone_->GetNavigator()->GetTrack())
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
            MediaTrack* currentTrack = GetTrackNavigationManager()->GetTrackFromId(i);
            
            if(currentTrack == nullptr)
                continue;
            
            DAW::CSurf_SetSurfaceSelected(currentTrack, DAW::CSurf_OnSelectedChange(currentTrack, 1), NULL);
        }
        
        MediaTrack* lowestTrack = GetTrackNavigationManager()->GetTrackFromId(lowerBound);
        
        if(lowestTrack != nullptr)
            GetPage()->OnTrackSelectionBySurface(lowestTrack);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_RECARM"));
    }
    
public:
    TrackRecordArm(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            DAW::CSurf_SetSurfaceRecArm(track, DAW::CSurf_OnRecArmChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_RECARM")), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        bool mute = false;
        DAW::GetTrackUIMute(track, &mute);
        UpdateWidgetValue(mute);
    }
    
public:
    TrackMute(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
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
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SOLO"));
    }
    
public:
    TrackSolo(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            DAW::CSurf_SetSurfaceSolo(track, DAW::CSurf_OnSoloChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SOLO")), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetFaderTouch : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetFaderTouch(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        zone_->GetNavigator()->SetIsFaderTouched(value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetRotaryTouch : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetRotaryTouch(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    void Do(double value) override
    {
        zone_->GetNavigator()->SetIsRotaryTouched(value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GlobalAutoMode(Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::GetGlobalAutomationOverride());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::SetGlobalAutomationOverride(intParam_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode(Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(widget, zone, params) {}

    void RequestUpdate() override
    {
        if(MediaTrack* selectedTrack = GetTrackNavigationManager()->GetSelectedTrack())
        {
            if(intParam_ == DAW::GetMediaTrackInfo_Value(selectedTrack, "I_AUTOMODE"))
                UpdateWidgetValue(1.0);
            else
                UpdateWidgetValue(0.0);
        }
    }
    
    virtual void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::SetAutomationMode(intParam_, true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTrackAutoMode : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        if(MediaTrack* selectedTrack = GetTrackNavigationManager()->GetSelectedTrack())
            if(track == selectedTrack)
                SetSteppedValueIndex(DAW::GetMediaTrackInfo_Value(selectedTrack, "I_AUTOMODE"));
        
        if(autoModes_.count(steppedValuesIndex_) > 0)
            UpdateWidgetValue(autoModes_[steppedValuesIndex_]);
        
        if(timeSilentlySet_ != 0 && DAW::GetCurrentNumberOfMilliseconds() - timeSilentlySet_ > TempDisplayTime)
        {
            if(displayWidget_ != nullptr)
                displayWidget_->UpdateValue("   ");
            timeSilentlySet_ = 0;
        }
    }
    
public:
    CycleTrackAutoMode(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
    
    virtual void DoAction(double value) override
    {
        if(value)
            Do(value);
    }
    
    virtual void Do(double value) override
    {
        MediaTrack* track = zone_->GetNavigator()->GetTrack();
        
        if(track != nullptr && steppedValues_.size() > 0)
        {
            if(timeSilentlySet_ != 0)
            {
                if(steppedValuesIndex_ == steppedValues_.size() - 1)
                        steppedValuesIndex_ = 0;
                else
                    steppedValuesIndex_++;
            }

            DAW::SetOnlyTrackSelected(track);
            
            int autoMode = steppedValues_[steppedValuesIndex_];
            
            DAW::SetAutomationMode(autoMode, true);
            
            if(displayWidget_ != nullptr && autoModes_.count(autoMode) > 0)
                displayWidget_->SilentSetValue(autoModes_[autoMode]);

            timeSilentlySet_ = DAW::GetCurrentNumberOfMilliseconds();
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuConCycleTrackAutoMode : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        if(MediaTrack* selectedTrack = GetTrackNavigationManager()->GetSelectedTrack())
            if(track == selectedTrack)
                SetSteppedValueIndex(DAW::GetMediaTrackInfo_Value(selectedTrack, "I_AUTOMODE"));
        
        if(autoModes_.count(steppedValuesIndex_) > 0)
            UpdateWidgetValue(autoModes_[steppedValuesIndex_]);
        
        if(timeSilentlySet_ != 0 && DAW::GetCurrentNumberOfMilliseconds() - timeSilentlySet_ > TempDisplayTime / 2)
        {
            if(displayWidget_ != nullptr)
                displayWidget_->UpdateValue("   ");
            timeSilentlySet_ = 0;
        }
    }
    
public:
    EuConCycleTrackAutoMode(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
    
    virtual void DoAction(double value) override
    {
        if(value)
            Do(value);
    }
    
    virtual void Do(double value) override
    {
        MediaTrack* track = zone_->GetNavigator()->GetTrack();
        
        if(track != nullptr && steppedValues_.size() > 0)
        {
            if(steppedValuesIndex_ == steppedValues_.size() - 1)
                steppedValuesIndex_ = 0;
            else
                steppedValuesIndex_++;
           
            DAW::SetOnlyTrackSelected(track);
            
            int autoMode = steppedValues_[steppedValuesIndex_];
            
            DAW::SetAutomationMode(autoMode, true);
            
            if(displayWidget_ != nullptr && autoModes_.count(autoMode) > 0)
                displayWidget_->SilentSetValue(autoModes_[autoMode]);
            
            timeSilentlySet_ = DAW::GetCurrentNumberOfMilliseconds();
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TimeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TimeDisplay(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuConTimeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    EuConTimeDisplay(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) { }
    
    void RequestUpdate() override
    {
        GetSurface()->UpdateTimeDisplay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::GetSetRepeatEx(nullptr, -1));
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeter : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackOutputMeter(Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(widget, zone, params) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        UpdateWidgetValue(volToNormalized(DAW::Track_GetPeakInfo(track, intParam_)));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeterAverageLR : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double lrVol = (DAW::Track_GetPeakInfo(track, 0) + DAW::Track_GetPeakInfo(track, 1)) / 2.0;
        
        UpdateWidgetValue(volToNormalized(lrVol));
    }

public:
    TrackOutputMeterAverageLR(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeterMaxPeakLR : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double lVol = DAW::Track_GetPeakInfo(track, 0);
        double rVol = DAW::Track_GetPeakInfo(track, 1);
        
        double lrVol =  lVol > rVol ? lVol : rVol;
        
        UpdateWidgetValue(volToNormalized(lrVol));
    }

public:
    TrackOutputMeterMaxPeakLR(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXGainReductionMeter : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXGainReductionMeter(Widget* widget, Zone* zone, vector<string> params) : FXAction(widget, zone, params) {}

    void RequestUpdate() override
    {
        char buffer[BUFSZ];
        
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
        {
            if(DAW::TrackFX_GetNamedConfigParm(track, paramIndex_, "GainReduction_dB", buffer, sizeof(buffer)))
                UpdateWidgetValue(-atof(buffer)/20.0);
            else
                UpdateWidgetValue(0.0);
        }
        else
             ClearWidget();
    }
};

#endif /* control_surface_Reaper_actions_h */
