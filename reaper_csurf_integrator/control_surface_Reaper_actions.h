   //
//  control_surface_Reaper_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_Reaper_actions_h
#define control_surface_Reaper_actions_h

#include "control_surface_base_actions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Track_Action : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    MediaTrack* track_;
    
public:
    Track_Action(Layout* layout, MediaTrack* track) : Action(layout), track_(track) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackDouble_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    MediaTrack* track_;
    
public:
    TrackDouble_Action(Layout* layout, MediaTrack* track) : Double_Action(layout), track_(track) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackString_Action : public String_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    MediaTrack* track_;
    
public:
    TrackString_Action(Layout* layout, MediaTrack* track) : String_Action(layout), track_(track) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Reaper_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_= 0;
    
public:
    Reaper_Action(Layout* layout, string commandStr) : Double_Action(layout)
    {
        commandId_ =  atol(commandStr.c_str());
        
        if(commandId_ != 0)
            return;
        
        commandId_ = DAW::NamedCommandLookup(commandStr.c_str());
        
        if(commandId_ == 0) // can't find it
            commandId_ = 65535; // no-op
    }
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetToggleCommandState(commandId_); }

    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::SendCommandMessage(commandId_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackFX_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string fxGUID_ = "";
    int paramIndex_ = 0;
    
public:
    TrackFX_Action(Layout* layout, MediaTrack* track, string fxGUID, int paramIndex) : TrackDouble_Action(layout, track), fxGUID_(fxGUID), paramIndex_(paramIndex) {}
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        double min = 0;
        double max = 0;
        return DAW::TrackFX_GetParam(track_, DAW::IndexFromFXGUID(track_, fxGUID_), paramIndex_, &min, &max);
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::TrackFX_SetParam(track_, DAW::IndexFromFXGUID(track_, fxGUID_), paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackAndFXToWidgets_Action  : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapTrackAndFXToWidgets_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(1 == DAW::CountSelectedTracks(nullptr))
            GetLayout()->MapTrackAndFXToWidgets(track_, zoneName, surfaceName);
        else
            GetLayout()->UnmapWidgetsFromTrack(track_, zoneName, surfaceName);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
 
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        //double widgetMaxDB = GetLayout->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = GetLayout->GetWidgetMinDB(surfaceName, widgetName);
        
        //GetLayout->SetWidgetValue(surfaceName, widgetName, clampedAndNormalized(VAL2DB(value), widgetMaxDB, widgetMinDB));
        GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, volToNormalized(value));
    }
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override
    {
        //double widgetMaxDB = GetLayout->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = GetLayout->GetWidgetMinDB(surfaceName, widgetName);

        //return volToNormalized(currentValue_, widgetMaxDB, widgetMinDB);
        return volToNormalized(currentValue_);
    }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "D_VOL"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        //double widgetMaxDB = GetLayout->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = GetLayout->GetWidgetMinDB(surfaceName, widgetName);

        //DAW::CSurf_SetSurfaceVolume(track_, DAW::CSurf_OnVolumeChange(track_, normalizedToVol(value, widgetMaxDB, widgetMinDB), false), NULL);
        DAW::CSurf_SetSurfaceVolume(track_, DAW::CSurf_OnVolumeChange(track_, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
public:
    TrackPan_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}

    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return panToNormalized(currentValue_); }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "D_PAN"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfacePan(track_, DAW::CSurf_OnPanChange(track_, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 3;
    
protected:
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
public:
    TrackPanWidth_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}

    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return panToNormalized(currentValue_); }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "D_WIDTH"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnWidthChange(track_, normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackNameDisplay_Action(Layout* layout, MediaTrack* track) : TrackString_Action(layout, track) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(DAW::GetMediaTrackInfo_Value(track_, "IP_TRACKNUMBER") == -1)
            return "Master";
        else
            return (char *)DAW::GetSetMediaTrackInfo(track_, "P_NAME", NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolumeDisplay_Action(Layout* layout, MediaTrack* track) : TrackString_Action(layout, track) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        char buffer[128];
        sprintf(buffer, "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(track_, "D_VOL")));
        return string(buffer);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnRew(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FastForward_Action(Layout* layout) : Double_Action(layout) {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnFwd(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Play_Action(Layout* layout) : Double_Action(layout) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            return 1;
        else return 0;
    }

    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnPlay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Stop_Action(Layout* layout) : Double_Action(layout) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        int playState = DAW::GetPlayState();
        if(playState == 0 || playState == 2 || playState == 6) // stopped or paused or paused whilst recording
            return 1;
        else
            return 0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnStop();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Record_Action(Layout* layout) : Double_Action(layout) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        int playState = DAW::GetPlayState();
        
        if(playState == 5 || playState == 6) // recording or paused whilst recording
            return 1;
        else
            return 0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnRecord();
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RepeatingArrow_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int direction_ = 0;
    clock_t lastRepeated = clock();
    double repeatRate_ = 0.0;
    bool pressed_ = false;
    
public:
    RepeatingArrow_Action(Layout* layout, int direction, double repeatRate) : Double_Action(layout), direction_(direction), repeatRate_(repeatRate) {}

    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        if(pressed_ && clock() - lastRepeated >  CLOCKS_PER_SEC * repeatRate_)
        {
            lastRepeated = clock();
            // GAW TBD
            //DAW::CSurf_OnArrow(direction_, GetLayout->GetRealSurfaceFor(surfaceName)->IsZoom());
        }
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        // GAW TBD
        // DAW::CSurf_OnArrow(direction_, GetLayout->GetRealSurfaceFor(surfaceName)->IsZoom());
        pressed_ = value;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelect_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceSelected(track_, DAW::CSurf_OnSelectedChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED")), NULL);
        GetLayout()->GetManager()->OnTrackSelection(track_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::SetOnlyTrackSelected(track_);
        GetLayout()->GetManager()->OnTrackSelection(track_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRangeSelect_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SELECTED"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        int selectedTrackNum = GetLayout()->GetManager()->CSurf_TrackToID(track_);
        int otherSelectedTrackNum = 0;

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            for(int i = 0; i < GetLayout()->GetManager()->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(GetLayout()->GetManager()->CSurf_TrackFromID(i), "I_SELECTED"))
                {
                    otherSelectedTrackNum = i;
                    break;
                }

            int lowerBound = selectedTrackNum < otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            int upperBound = selectedTrackNum > otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            
            for(int i = lowerBound; i <= upperBound; i++)
            {
                DAW::CSurf_SetSurfaceSelected(track_, DAW::CSurf_OnSelectedChange(GetLayout()->GetManager()->CSurf_TrackFromID(i), 1), NULL);
                GetLayout()->GetManager()->OnTrackSelection(GetLayout()->GetManager()->CSurf_TrackFromID(i));
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRecordArm_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
   
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_RECARM"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceRecArm(track_, DAW::CSurf_OnRecArmChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "B_MUTE"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
         DAW::CSurf_SetSurfaceMute(track_, DAW::CSurf_OnMuteChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(track_, "I_SOLO"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceSolo(track_, DAW::CSurf_OnSoloChange(track_, ! DAW::GetMediaTrackInfo_Value(track_, "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouch_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackTouch_Action(Layout* layout, MediaTrack* track) : TrackDouble_Action(layout, track) {}

    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetTouchState(track_, value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlled_Action : public Track_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* action_= nullptr;
    string actionAddress_= nullptr;
    
    bool lastTouched_ = false;
    
    bool IsCurrentlyTouched() { return GetLayout()->GetTouchState(track_, 0); }
    
public:
    TrackTouchControlled_Action(string actionAddress, Layout* layout, MediaTrack* track, Action* action) : Track_Action(layout, track), action_(action), actionAddress_(actionAddress) {}
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        bool currentlyTouched = IsCurrentlyTouched();
        
        if(currentlyTouched)
        {
            lastTouched_ = currentlyTouched;
            action_->Update(zoneName, surfaceName, widgetName);
        }
        else if(lastTouched_ != currentlyTouched)
        {
            lastTouched_ = currentlyTouched;
            GetLayout()->ForceUpdateAction(actionAddress_, zoneName, surfaceName, widgetName);
        }
    }

    virtual int GetDisplayMode() override
    {
        return action_->GetDisplayMode();
    }
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override
    {
        return action_->GetCurrentNormalizedValue(zoneName, surfaceName, widgetName);
    }
    
    virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched())
            action_->ForceUpdate(zoneName, surfaceName, widgetName);
    }
    
    virtual void Cycle(string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched())
            action_->Cycle(zoneName, surfaceName, widgetName);
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched())
            action_->Do(value, zoneName, surfaceName, widgetName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int autoMode_ = 0;
    
public:
    GlobalAutoMode_Action(Layout* layout, string autoModeStr) : Double_Action(layout)
    {
        autoMode_ = atol(autoModeStr.c_str());
    }

    virtual double GetValue (string zoneName, string surfaceName, string widgetName) override { return DAW::GetGlobalAutomationOverride(); }
    
    virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetValue(zoneName, surfaceName, widgetName) == autoMode_)
            SetWidgetValue(zoneName, surfaceName, widgetName, autoMode_);
    }
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        double newValue = GetValue(zoneName, surfaceName, widgetName);
        if(currentValue_ != newValue)
            SetWidgetValue(zoneName, surfaceName, widgetName, (currentValue_ = newValue) == autoMode_ ? 1 : 0);
    }

    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::SetGlobalAutomationOverride(autoMode_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode_Action : public GlobalAutoMode_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackAutoMode_Action(Layout* layout, string autoModeStr) : GlobalAutoMode_Action(layout, autoModeStr) {}
    
    virtual double GetValue (string zoneName, string surfaceName, string widgetName) override
    {
        for(int i = 0; i < GetLayout()->GetManager()->GetNumTracks(); i++)
        {
            MediaTrack *track = GetLayout()->GetManager()->CSurf_TrackFromID(i);
            
            if(DAW::GetMediaTrackInfo_Value(track, "I_SELECTED"))
                return DAW::GetMediaTrackInfo_Value(track, "I_AUTOMODE");
        }
        
        return 0.00;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::SetAutomationMode(autoMode_, true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeline_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetSetRepeatEx(nullptr, -1); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Cancel_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Cancel_Action(Layout* layout) : Double_Action(layout)  {}
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Enter_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Enter_Action(Layout* layout) : Double_Action(layout)  {}
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeter_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channel_ = 0;
    
protected:
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
              GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName,
                                                clampedAndNormalized(value, GetLayout()->GetWidgetMaxDB(zoneName, surfaceName, widgetName), GetLayout()->GetWidgetMinDB(zoneName, surfaceName, widgetName)));
        else
            GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, 0.0);
    }
    
public:
    TrackOutputMeter_Action(Layout* layout, MediaTrack* track, string channelStr) : TrackDouble_Action(layout, track)
    {
        channel_ = atol(channelStr.c_str());
    }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return VAL2DB(DAW::Track_GetPeakInfo(track_, channel_)); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackGainReductionMeter_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string fxGUID_ = "";
    
protected:
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
            //GetLayout->SetWidgetValue(surfaceName, widgetName, clampedAndNormalized(-value, GetLayout->GetWidgetMaxDB(surfaceName, widgetName), GetLayout->GetWidgetMinDB(surfaceName, widgetName)));
            GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, -value / 20.0); // GAW TBD hacked for now
        else
            GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, 1.0);
    }
    
public:
    TrackGainReductionMeter_Action(Layout* layout, MediaTrack* track, string fxGUID) : TrackDouble_Action(layout, track), fxGUID_(fxGUID)  {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        char buffer[BUFSZ];
        if(DAW::TrackFX_GetNamedConfigParm(track_, DAW::IndexFromFXGUID(track_, fxGUID_), "GainReduction_dB", buffer, sizeof(buffer)))
            return atof(buffer);
        else
            return 0.0;
    }
};
#endif /* control_surface_Reaper_actions_h */
