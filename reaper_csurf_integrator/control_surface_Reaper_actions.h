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
    FXParam(string name, Widget* widget, Zone* zone, vector<string> params) : FXAction(name, widget, zone, params) {}
    
    virtual void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            DAW::TrackFX_SetParam(track, GetSlotIndex(), fxParamIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamRelative : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamRelative(string name, Widget* widget, Zone* zone, vector<string> params) : FXAction(name, widget, zone, params) {}
       
    virtual void Do(double relativeValue, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            double min, max = 0;
            double value = DAW::TrackFX_GetParam(track, GetSlotIndex(), fxParamIndex_, &min, &max);
            value +=  relativeValue;
            
            if(value < min) value = min;
            if(value > max) value = max;
            
            DAW::TrackFX_SetParam(track, GetSlotIndex(), fxParamIndex_, value);
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
            if(MediaTrack* track = GetWidget()->GetSurface()->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                double min = 0.0;
                double max = 0.0;
                double value = DAW::TrackFX_GetParam(track, fxSlotNum, fxParamNum, &min, &max);

                SetWidgetValue(GetWidget(), value);
            }
        }
    }
    
public:
    FocusedFXParam(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;

        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
            if(MediaTrack* track = widget_->GetSurface()->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNum))
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
        SetWidgetValue(GetWidget(), volToNormalized(vol));
    }
    
public:
    TrackVolume(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
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
        SetWidgetValue(GetWidget(), VAL2DB(vol));
    }
    
public:
    TrackVolumeDB(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
        
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
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
        SetWidgetValue(GetWidget(), param_, panToNormalized(pan));
    }
    
public:
    TrackPan(string name, Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
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
        SetWidgetValue(GetWidget(), pan * 100.0);
    }
    
public:
    TrackPanPercent(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
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
        SetWidgetValue(GetWidget(), param_, panToNormalized(DAW::GetMediaTrackInfo_Value(track, "D_WIDTH")));
    }
    
public:
    TrackPanWidth(string name, Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
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
        SetWidgetValue(GetWidget(), DAW::GetMediaTrackInfo_Value(track, "D_WIDTH") * 100.0);
    }
    
public:
    TrackPanWidthPercent(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            DAW::CSurf_OnWidthChange(track, value / 100.0, false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        char buf[BUFSZ];
        
        DAW::GetTrackName(track, buf, sizeof(buf));
        
        SetWidgetValue(GetWidget(), string(buf));
    }

public:
    TrackNameDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
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
        SetWidgetValue(GetWidget(), string(trackVolume));
    }
    
public:
    TrackVolumeDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolume : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        double vol, pan = 0.0;
        DAW::GetTrackSendUIVolPan(track, GetParamNum(), &vol, &pan);
        SetWidgetValue(GetWidget(), volToNormalized(vol));
    }

public:
    TrackSendVolume(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, GetParamNum(), normalizedToVol(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "D_VOL", &volume);
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
        DAW::GetTrackSendUIVolPan(track, GetParamNum(), &vol, &pan);
        SetWidgetValue(GetWidget(), VAL2DB(vol));
    }
    
public:
    TrackSendVolumeDB(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            double volume = DAW::CSurf_OnSendVolumeChange(track, GetParamNum(), DB2VAL(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "D_VOL", &volume);
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
        DAW::GetTrackSendUIVolPan(track, GetParamNum(), &vol, &pan);
        SetWidgetValue(GetWidget(), panToNormalized(pan));
    }
    
public:
    TrackSendPan(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            double pan = DAW::CSurf_OnSendPanChange(track, GetParamNum(), normalizedToPan(value), false);
            
            DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "D_PAN", &pan);
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
        DAW::GetTrackSendUIMute(track, GetParamNum(), &mute);
        SetWidgetValue(GetWidget(), mute);
    }

public:
    TrackSendMute(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            bool isMuted = ! DAW::GetTrackSendInfo_Value(track, 0, GetParamNum(), "B_MUTE");
            
            DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "B_MUTE", &isMuted);
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
        SetWidgetValue(GetWidget(), DAW::GetTrackSendInfo_Value(track, 0, GetParamNum(), "B_PHASE"));
    }
    
public:
    TrackSendInvertPolarity(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            bool reversed = ! DAW::GetTrackSendInfo_Value(track, 0, GetParamNum(), "B_PHASE");
            
            DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "B_PHASE", &reversed);
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
        if(DAW::GetTrackSendInfo_Value(track, 0, GetParamNum(), "I_SENDMODE") == 0)
            SetWidgetValue(GetWidget(), 0);
        else
            SetWidgetValue(GetWidget(), 1);
    }
    
public:
    TrackSendPrePost(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            int mode = DAW::GetTrackSendInfo_Value(track, 0, GetParamNum(), "I_SENDMODE");
            
            if(mode == 0)
                mode = 3; // switch to post FX
            else
                mode = 0; // switch to post fader
            
            DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "I_SENDMODE", &mode);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXNameDisplay : public TrackActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXNameDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(name, widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            int param = param_ - 1 < 0 ? 0 : param_ - 1;
            
            char fxName[BUFSZ];
            
            DAW::TrackFX_GetFXName(track, param, fxName, sizeof(fxName));

            SetWidgetValue(GetWidget(), GetWidget()->GetSurface()->GetLocalZoneAlias(fxName));
        }
        else
             GetWidget()->Clear();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamNameDisplay : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamNameDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : FXAction(name, widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            SetWidgetValue(GetWidget(), GetDisplayName());
        else
             GetWidget()->Clear();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXParamValueDisplay : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXParamValueDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : FXAction(name, widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            char fxParamValue[128];
            DAW::TrackFX_GetFormattedParamValue(track, GetSlotIndex(), fxParamIndex_, fxParamValue, sizeof(fxParamValue));
            SetWidgetValue(GetWidget(), string(fxParamValue));
        }
        else
             GetWidget()->Clear();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParamNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXParamNameDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = GetWidget()->GetSurface()->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char fxParamName[128];
                DAW::TrackFX_GetParamName(track, fxSlotNum, fxParamNum, fxParamName, sizeof(fxParamName));
                SetWidgetValue(GetWidget(), string(fxParamName));
            }
        }
        else
             GetWidget()->Clear();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocusedFXParamValueDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FocusedFXParamValueDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
    virtual void RequestUpdate() override
    {
        int trackNum = 0;
        int fxSlotNum = 0;
        int fxParamNum = 0;
        
        if(DAW::GetLastTouchedFX(&trackNum, &fxSlotNum, &fxParamNum))
        {
            if(MediaTrack* track = GetWidget()->GetSurface()->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                char fxParamValue[128];
                DAW::TrackFX_GetFormattedParamValue(track, fxSlotNum, fxParamNum, fxParamValue, sizeof(fxParamValue));
                SetWidgetValue(GetWidget(), string(fxParamValue));
           }
        }
        else
             GetWidget()->Clear();
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
        MediaTrack* destTrack = (MediaTrack *)DAW::GetSetTrackSendInfo(track, 0, GetParamNum(), "P_DESTTRACK", 0);;
        if(destTrack)
            sendTrackName = (char *)DAW::GetSetMediaTrackInfo(destTrack, "P_NAME", NULL);
        SetWidgetValue(GetWidget(), sendTrackName);
    }

public:
    TrackSendNameDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendVolumeDisplay : public TrackSendAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        char trackVolume[128];
        snprintf(trackVolume, sizeof(trackVolume), "%7.2lf", VAL2DB(DAW::GetTrackSendInfo_Value(track, 0, GetParamNum(), "D_VOL")));
        SetWidgetValue(GetWidget(), string(trackVolume));
    }

public:
    TrackSendVolumeDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackSendAction(name, widget, zone, params) {}
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
        
        SetWidgetValue(GetWidget(), string(trackPan));
    }

public:
    TrackPanDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
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
        
        SetWidgetValue(GetWidget(), string(trackPanWidth));
    }

public:
    TrackPanWidthDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }

    void Do(double value, Widget* sender) override
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
    FastForward(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }

    void Do(double value, Widget* sender) override
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
    Play(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }

    void RequestUpdate() override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            playState = 1;
        else playState = 0;
        SetWidgetValue(GetWidget(), playState);
    }
    
    void Do(double value, Widget* sender) override
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
    Stop(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }

    void RequestUpdate() override
    {
        int stopState = DAW::GetPlayState();
        if(stopState == 0 || stopState == 2 || stopState == 6) // stopped or paused or paused whilst recording
            stopState = 1;
        else stopState = 0;
        
        SetWidgetValue(GetWidget(), stopState);
    }
    
    void Do(double value, Widget* sender) override
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
    Record(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }

    void RequestUpdate() override
    {
        int recordState = DAW::GetPlayState();
        if(recordState == 5 || recordState == 6) // recording or paused whilst recording
            recordState = 1;
        else recordState = 0;
        
        SetWidgetValue(GetWidget(), recordState);
    }
    
    void Do(double value, Widget* sender) override
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
    TrackToggleVCASpill(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = widget_->GetTrack())
            widget_->GetSurface()->GetPage()->GetTrackNavigationManager()->ToggleVCASpill(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(GetWidget(), DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
public:
    TrackSelect(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
            GetWidget()->GetSurface()->GetPage()->OnTrackSelectionBySurface(track);
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
        SetWidgetValue(GetWidget(), DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
public:
    TrackUniqueSelect(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            DAW::SetOnlyTrackSelected(track);
            GetWidget()->GetSurface()->GetPage()->OnTrackSelectionBySurface(track);
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
        SetWidgetValue(GetWidget(), DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"));
    }
    
public:
    TrackRangeSelect(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}

    virtual void Do(double value, Widget* sender) override
    {
        // GAW TBD  fix highest track bug 
        
        if(value == 0.0) return; // ignore button releases

        int currentlySelectedCount = 0;
        int selectedTrackIndex = 0;
        int trackIndex = 0;
        
        Page* page = GetWidget()->GetSurface()->GetPage();
        
        for(int i = 1; i <= page->GetTrackNavigationManager()->GetNumTracks(); i++)
        {
            MediaTrack* currentTrack = page->GetTrackNavigationManager()->GetTrackFromId(i);
           
            if(currentTrack == nullptr)
                continue;
            
            if(currentTrack == GetWidget()->GetTrack())
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
            
            if(currentTrack == nullptr)
                continue;
            
            DAW::CSurf_SetSurfaceSelected(currentTrack, DAW::CSurf_OnSelectedChange(currentTrack, 1), NULL);
        }
        
        MediaTrack* lowestTrack = page->GetTrackNavigationManager()->GetTrackFromId(lowerBound);
        
        if(lowestTrack != nullptr)
            page->OnTrackSelectionBySurface(lowestTrack);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(GetWidget(), DAW::GetMediaTrackInfo_Value(track, "I_RECARM"));
    }
    
public:
    TrackRecordArm(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
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
        SetWidgetValue(GetWidget(), mute);
    }
    
public:
    TrackMute(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
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
        SetWidgetValue(GetWidget(), DAW::GetMediaTrackInfo_Value(track, "I_SOLO"));
    }
    
public:
    TrackSolo(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
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
    SetTrackTouch(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        GetWidget()->SetIsTouched(value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GlobalAutoMode(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(name, widget, zone, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), DAW::GetGlobalAutomationOverride());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::SetGlobalAutomationOverride(param_);
    }
};


// I_AUTOMODE : int * : track automation mode (0=trim/off, 1=read, 2=touch, 3=write, 4=latch)


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(name, widget, zone, params) {}

    void RequestUpdate() override
    {
        bool gotOne = false;
        
        for(int i = 0; i <= GetPage()->GetTrackNavigationManager()->GetNumTracks(); i++)
        {
            MediaTrack* track = GetPage()->GetTrackNavigationManager()->GetTrackFromId(i);
            
            if(track == nullptr)
                continue;
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED") && DAW::GetMediaTrackInfo_Value(track, "I_AUTOMODE") == param_)
            {
                gotOne = true;
                break;
            }
        }
        
        if(gotOne)
            SetWidgetValue(GetWidget(), 1.0);
        else
            SetWidgetValue(GetWidget(), 0.0);
    }
    
    virtual void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        DAW::SetAutomationMode(param_, true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TimeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TimeDisplay(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), DAW::GetSetRepeatEx(nullptr, -1));
    }
    
    void Do(double value, Widget* sender) override
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
    TrackOutputMeter(string name, Widget* widget, Zone* zone, vector<string> params) : TrackActionWithIntParam(name, widget, zone, params) {}

    void RequestTrackUpdate(MediaTrack* track) override
    {
        SetWidgetValue(GetWidget(), volToNormalized(DAW::Track_GetPeakInfo(track, param_)));
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
        
        SetWidgetValue(GetWidget(), volToNormalized(lrVol));
    }

public:
    TrackOutputMeterAverageLR(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
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
        
        SetWidgetValue(GetWidget(), volToNormalized(lrVol));
    }

public:
    TrackOutputMeterMaxPeakLR(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXGainReductionMeter : public FXAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXGainReductionMeter(string name, Widget* widget, Zone* zone, vector<string> params) : FXAction(name, widget, zone, params) {}

    void RequestUpdate() override
    {
        char buffer[BUFSZ];
        
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            if(DAW::TrackFX_GetNamedConfigParm(track, fxParamIndex_, "GainReduction_dB", buffer, sizeof(buffer)))
                SetWidgetValue(GetWidget(), -atof(buffer)/20.0);
            else
                SetWidgetValue(GetWidget(), 0.0);
        }
        else
             GetWidget()->Clear();
    }
};

#endif /* control_surface_Reaper_actions_h */
