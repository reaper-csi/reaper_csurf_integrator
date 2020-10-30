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
    virtual string GetName() override { return "FXParam"; }
    
    virtual void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::TrackFX_SetParam(track, context->GetZone()->GetSlotIndex(), context->GetParamIndex(), value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamTouch : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXParamTouch"; }
    
    virtual void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double min, max = 0;
            
            if(value == 0)
                DAW::TrackFX_EndParamEdit(track, context->GetZone()->GetSlotIndex(), context->GetParamIndex());
            else
                DAW::TrackFX_SetParam(track, context->GetZone()->GetSlotIndex(), context->GetParamIndex(), DAW::TrackFX_GetParam(track, context->GetZone()->GetSlotIndex(), context->GetParamIndex(), &min, &max));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamRelative : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXParamRelative"; }
    
    virtual void Do(ActionContext* context, double relativeValue) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double min, max = 0;
            double value = DAW::TrackFX_GetParam(track, context->GetZone()->GetSlotIndex(), context->GetParamIndex(), &min, &max);
            value +=  relativeValue;
            
            if(value < min) value = min;
            if(value > max) value = max;
            
            DAW::TrackFX_SetParam(track, context->GetZone()->GetSlotIndex(), context->GetParamIndex(), value);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FocusedFXParam"; }
    
    void RequestUpdate(ActionContext* context) override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = context->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                double min = 0.0;
                double max = 0.0;
                double value = DAW::TrackFX_GetParam(track, fxSlotNum, fxParamNum, &min, &max);
                
                context->UpdateWidgetValue(value);
            }
        }
    }
    
    void Do(ActionContext* context, double value) override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
            if(MediaTrack* track = context->GetTrackNavigationManager()->GetTrackFromId(trackNum))
                DAW::TrackFX_SetParam(track, fxSlotNum, fxParamNum, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackVolume"; }
    
    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackUIVolPan(track, &vol, &pan);
            context->UpdateWidgetValue(volToNormalized(vol));
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SoftTakeover7BitTrackVolume : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SoftTakeover7BitTrackVolume"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
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
class SoftTakeover14BitTrackVolume : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SoftTakeover14BitTrackVolume"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
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
class TrackVolumeDB : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackVolumeDB"; }
    
    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackUIVolPan(track, &vol, &pan);
            context->UpdateWidgetValue(VAL2DB(vol));
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, DB2VAL(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPan"; }
    
    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackUIVolPan(track, &vol, &pan);
            context->UpdateWidgetValue(context->GetIntParam(), panToNormalized(pan));
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanPercent : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanPercent"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackUIVolPan(track, &vol, &pan);
            context->UpdateWidgetValue(pan * 100.0);
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, value / 100.0, false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanWidth"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(context->GetIntParam(), panToNormalized(DAW::GetMediaTrackInfo_Value(track, "D_WIDTH")));
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::CSurf_OnWidthChange(track, normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthPercent : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanWidthPercent"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "D_WIDTH") * 100.0);
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
            DAW::CSurf_OnWidthChange(track, value / 100.0, false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanLPercent : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanLPercent"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "D_DUALPANL") * 100.0);
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double panFromPercent = value / 100.0;
            DAW::GetSetMediaTrackInfo(track, "D_DUALPANL", &panFromPercent);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanRPercent : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanRPercent"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "D_DUALPANR") * 100.0);
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double panFromPercent = value / 100.0;
            DAW::GetSetMediaTrackInfo(track, "D_DUALPANR", &panFromPercent);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolume : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendVolume"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackSendUIVolPan(track, context->GetParamIndex(), &vol, &pan);
            context->UpdateWidgetValue(volToNormalized(vol));
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, context->GetParamIndex(), normalizedToVol(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "D_VOL", &volume);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolumeDB : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendVolumeDB"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackSendUIVolPan(track, context->GetParamIndex(), &vol, &pan);
            context->UpdateWidgetValue(VAL2DB(vol));
        }
        else
            context->ClearWidget();
        
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, context->GetParamIndex(), DB2VAL(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "D_VOL", &volume);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendPan : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendPan"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double vol, pan = 0.0;
            DAW::GetTrackSendUIVolPan(track, context->GetParamIndex(), &vol, &pan);
            context->UpdateWidgetValue(panToNormalized(pan));
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double pan = DAW::CSurf_OnSendPanChange(track, context->GetParamIndex(), normalizedToPan(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "D_PAN", &pan);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendMute : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendMute"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            bool mute = false;
            DAW::GetTrackSendUIMute(track, context->GetParamIndex(), &mute);
            context->UpdateWidgetValue(mute);
        }
        else
            context->ClearWidget();
        
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
        {
             bool isMuted = ! DAW::GetTrackSendInfo_Value(track, 0, context->GetParamIndex(), "B_MUTE");
            
            DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "B_MUTE", &isMuted);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendInvertPolarity : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendInvertPolarity"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetTrackSendInfo_Value(track, 0, context->GetParamIndex(), "B_PHASE"));
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
        {
            bool reversed = ! DAW::GetTrackSendInfo_Value(track, 0, context->GetParamIndex(), "B_PHASE");
            
            DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "B_PHASE", &reversed);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendPrePost : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendPrePost"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            if(DAW::GetTrackSendInfo_Value(track, 0, context->GetParamIndex(), "I_SENDMODE") == 0)
                context->UpdateWidgetValue(0);
            else
                context->UpdateWidgetValue(1);
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
        {
            int mode = DAW::GetTrackSendInfo_Value(track, 0, context->GetParamIndex(), "I_SENDMODE");
            
            if(mode == 0)
                mode = 3; // switch to post FX
            else
                mode = 0; // switch to post fader
            
            DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "I_SENDMODE", &mode);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXNameDisplay"; }

    virtual void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            int slot = context->GetSlotIndex() - 1 < 0 ? 0 : context->GetSlotIndex() - 1;
            
            char fxName[BUFSZ];
            
            DAW::TrackFX_GetFXName(track, slot, fxName, sizeof(fxName));
            
            context->UpdateWidgetValue(fxName);
        }
        else
            context->GetWidget()->Clear();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXParamNameDisplay"; }

    virtual void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(context->GetFxParamDisplayName());
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamValueDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXParamValueDisplay"; }

    virtual void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            char fxParamValue[128];
            DAW::TrackFX_GetFormattedParamValue(track, context->GetSlotIndex(), context->GetParamIndex(), fxParamValue, sizeof(fxParamValue));
            context->UpdateWidgetValue(string(fxParamValue));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParamNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FocusedFXParamNameDisplay"; }
    
    virtual void RequestUpdate(ActionContext* context) override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = context->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char fxParamName[128];
                DAW::TrackFX_GetParamName(track, fxSlotNum, fxParamNum, fxParamName, sizeof(fxParamName));
                context->UpdateWidgetValue(string(fxParamName));
            }
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParamValueDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FocusedFXParamValueDisplay"; }
    
    virtual void RequestUpdate(ActionContext* context) override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = context->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char fxParamValue[128];
                DAW::TrackFX_GetFormattedParamValue(track, fxSlotNum, fxParamNum, fxParamValue, sizeof(fxParamValue));
                context->UpdateWidgetValue(string(fxParamValue));
            }
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendNameDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            string sendTrackName = "";
            MediaTrack* destTrack = (MediaTrack *)DAW::GetSetTrackSendInfo(track, 0, context->GetParamIndex(), "P_DESTTRACK", 0);;
            if(destTrack)
                sendTrackName = (char *)DAW::GetSetMediaTrackInfo(destTrack, "P_NAME", NULL);
            context->UpdateWidgetValue(sendTrackName);
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolumeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSendVolumeDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            char trackVolume[128];
            snprintf(trackVolume, sizeof(trackVolume), "%7.2lf", VAL2DB(DAW::GetTrackSendInfo_Value(track, 0, context->GetParamIndex(), "D_VOL")));
            context->UpdateWidgetValue(string(trackVolume));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FixedTextDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FixedTextDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetStringParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FixedRGBColourDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FixedRGBColourDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackNameDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            char buf[BUFSZ];
            
            if(context->GetSurface()->GetIsEuConFXAreaFocused())
            {
                if(track == context->GetTrackNavigationManager()->GetSelectedTrack())
                    DAW::GetTrackName(track, buf, sizeof(buf));
                else
                    buf[0] = 0;
            }
            else
            {
                DAW::GetTrackName(track, buf, sizeof(buf));
            }
            
            context->UpdateWidgetValue(string(buf));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackVolumeDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            char trackVolume[128];
            snprintf(trackVolume, sizeof(trackVolume), "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(track, "D_VOL")));
            context->UpdateWidgetValue(string(trackVolume));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
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
            
            context->UpdateWidgetValue(string(trackPan));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackPanWidthDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
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
            
            context->UpdateWidgetValue(string(trackPanWidth));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "Rewind"; }

    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "FastForward"; }

    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "Play"; }

    void RequestUpdate(ActionContext* context) override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            playState = 1;
        else playState = 0;
        context->UpdateWidgetValue(playState);
    }
    
    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "Stop"; }

    void RequestUpdate(ActionContext* context) override
    {
        int stopState = DAW::GetPlayState();
        if(stopState == 0 || stopState == 2 || stopState == 6) // stopped or paused or paused whilst recording
            stopState = 1;
        else stopState = 0;
        
        context->UpdateWidgetValue(stopState);
    }
    
    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "Record"; }

    void RequestUpdate(ActionContext* context) override
    {
        int recordState = DAW::GetPlayState();
        if(recordState == 5 || recordState == 6) // recording or paused whilst recording
            recordState = 1;
        else recordState = 0;
        
        context->UpdateWidgetValue(recordState);
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        DAW::CSurf_OnRecord();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackToggleVCASpill : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackToggleVCASpill"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
            context->GetTrackNavigationManager()->ToggleVCASpill(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSelect"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
        else
            context->ClearWidget();
    }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = context->GetTrack())
        {
            DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
            context->GetPage()->OnTrackSelectionBySurface(track);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackUniqueSelect"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
        else
            context->ClearWidget();
    }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = context->GetTrack())
        {
            DAW::SetOnlyTrackSelected(track);
            context->GetPage()->OnTrackSelectionBySurface(track);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackRangeSelect"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
        else
            context->ClearWidget();
    }

    virtual void Do(ActionContext* context, double value) override
    {
        // GAW TBD  fix highest track bug 
        
        if(value == 0.0) return; // ignore button releases

        int currentlySelectedCount = 0;
        int selectedTrackIndex = 0;
        int trackIndex = 0;
        
       
        for(int i = 1; i <= context->GetTrackNavigationManager()->GetNumTracks(); i++)
        {
            MediaTrack* currentTrack = context->GetTrackNavigationManager()->GetTrackFromId(i);
           
            if(currentTrack == nullptr)
                continue;
            
            if(currentTrack == context->GetTrack())
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
            MediaTrack* currentTrack = context->GetTrackNavigationManager()->GetTrackFromId(i);
            
            if(currentTrack == nullptr)
                continue;
            
            DAW::CSurf_SetSurfaceSelected(currentTrack, DAW::CSurf_OnSelectedChange(currentTrack, 1), NULL);
        }
        
        MediaTrack* lowestTrack = context->GetTrackNavigationManager()->GetTrackFromId(lowerBound);
        
        if(lowestTrack != nullptr)
            context->GetPage()->OnTrackSelectionBySurface(lowestTrack);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackRecordArm"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_RECARM"));
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
        {
            DAW::CSurf_SetSurfaceRecArm(track, DAW::CSurf_OnRecArmChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_RECARM")), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackMute"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            bool mute = false;
            DAW::GetTrackUIMute(track, &mute);
            context->UpdateWidgetValue(mute);
        }
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
        {
            bool mute = false;
            DAW::GetTrackUIMute(track, &mute);
            DAW::CSurf_SetSurfaceMute(track, DAW::CSurf_OnMuteChange(track, ! mute), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackSolo"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(DAW::GetMediaTrackInfo_Value(track, "I_SOLO"));
        else
            context->ClearWidget();
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
        {
            DAW::CSurf_SetSurfaceSolo(track, DAW::CSurf_OnSoloChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SOLO")), NULL);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GlobalAutoMode"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(DAW::GetGlobalAutomationOverride());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        DAW::SetGlobalAutomationOverride(context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackAutoMode"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* selectedTrack = context->GetTrackNavigationManager()->GetSelectedTrack())
        {
            if(context->GetIntParam() == DAW::GetMediaTrackInfo_Value(selectedTrack, "I_AUTOMODE"))
                context->UpdateWidgetValue(1.0);
            else
                context->UpdateWidgetValue(0.0);
        }
    }
    
    virtual void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        DAW::SetAutomationMode(context->GetIntParam(), true);
    }
};



/*
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
        MediaTrack* track = GetZone()->GetNavigator()->GetTrack();
        
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
        MediaTrack* track = GetZone()->GetNavigator()->GetTrack();
        
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
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TimeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TimeDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EuConTimeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "EuConTimeDisplay"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->GetSurface()->UpdateTimeDisplay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "CycleTimeline"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(DAW::GetSetRepeatEx(nullptr, -1));
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeter : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackOutputMeter"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
            context->UpdateWidgetValue(volToNormalized(DAW::Track_GetPeakInfo(track, context->GetIntParam())));
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeterAverageLR : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackOutputMeterAverageLR"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double lrVol = (DAW::Track_GetPeakInfo(track, 0) + DAW::Track_GetPeakInfo(track, 1)) / 2.0;
            
            context->UpdateWidgetValue(volToNormalized(lrVol));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeterMaxPeakLR : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "TrackOutputMeterMaxPeakLR"; }

    void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            double lVol = DAW::Track_GetPeakInfo(track, 0);
            double rVol = DAW::Track_GetPeakInfo(track, 1);
            
            double lrVol =  lVol > rVol ? lVol : rVol;
            
            context->UpdateWidgetValue(volToNormalized(lrVol));
        }
        else
            context->ClearWidget();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXGainReductionMeter : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXGainReductionMeter"; }

    void RequestUpdate(ActionContext* context) override
    {
        char buffer[BUFSZ];
        
        if(MediaTrack* track = context->GetTrack())
        {
            if(DAW::TrackFX_GetNamedConfigParm(track, context->GetParamIndex(), "GainReduction_dB", buffer, sizeof(buffer)))
                context->UpdateWidgetValue(-atof(buffer)/20.0);
            else
                context->UpdateWidgetValue(0.0);
        }
        else
            context->ClearWidget();
    }
};

#endif /* control_surface_Reaper_actions_h */
