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
private:
    string trackGUID_ = "";
protected:
    MediaTrack* GetTrack()
    {
        return DAW::GetTrackFromGUID(trackGUID_, false);
    }

public:
    Track_Action(Layout* layout, string trackGUID) : Action(layout), trackGUID_(trackGUID) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackDouble_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string trackGUID_ = "";
protected:
    MediaTrack* GetTrack(string zoneName)
    {
        return GetLayout()->GetZone(zoneName)->GetTrackFromGUID(trackGUID_);
    }

public:
    TrackDouble_Action(Layout* layout, string trackGUID) : Double_Action(layout), trackGUID_(trackGUID) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackString_Action : public String_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string trackGUID_ = "";
protected:
    MediaTrack* GetTrack(string zoneName)
    {
        return GetLayout()->GetZone(zoneName)->GetTrackFromGUID(trackGUID_);
    }
    
public:
    TrackString_Action(Layout* layout, string trackGUID) : String_Action(layout), trackGUID_(trackGUID) {}
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
    TrackFX_Action(Layout* layout, string trackGUID, string fxGUID, int paramIndex) : TrackDouble_Action(layout, trackGUID), fxGUID_(fxGUID), paramIndex_(paramIndex) {}
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        double min = 0;
        double max = 0;
        return DAW::TrackFX_GetParam(GetTrack(zoneName), DAW::IndexFromFXGUID(GetTrack(zoneName), fxGUID_), paramIndex_, &min, &max);
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::TrackFX_SetParam(GetTrack(zoneName), DAW::IndexFromFXGUID(GetTrack(zoneName), fxGUID_), paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackAndFXToWidgets_Action  : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapTrackAndFXToWidgets_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(1 == DAW::CountSelectedTracks(nullptr))
            GetLayout()->MapTrackAndFXToWidgets(GetTrack(zoneName), zoneName, surfaceName);
        else
            GetLayout()->UnmapWidgetsFromTrack(GetTrack(zoneName), zoneName, surfaceName);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
 
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

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_VOL"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        //double widgetMaxDB = GetLayout->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = GetLayout->GetWidgetMinDB(surfaceName, widgetName);

        //DAW::CSurf_SetSurfaceVolume(track_, DAW::CSurf_OnVolumeChange(track_, normalizedToVol(value, widgetMaxDB, widgetMinDB), false), NULL);
        DAW::CSurf_SetSurfaceVolume(GetTrack(zoneName), DAW::CSurf_OnVolumeChange(GetTrack(zoneName), normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayMode_ = 0;
    
public:
    TrackPan_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}

    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return panToNormalized(currentValue_); }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_PAN"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfacePan(GetTrack(zoneName), DAW::CSurf_OnPanChange(GetTrack(zoneName), normalizedToPan(value), false), NULL);
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
    TrackPanWidth_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}

    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return panToNormalized(currentValue_); }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_WIDTH"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_OnWidthChange(GetTrack(zoneName), normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackNameDisplay_Action(Layout* layout, string trackGUID) : TrackString_Action(layout, trackGUID) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName) == nullptr)
            return "";
           
        if(DAW::GetMediaTrackInfo_Value(GetTrack(zoneName) , "IP_TRACKNUMBER") == -1)
            return "Master";
        else
            return (char *)DAW::GetSetMediaTrackInfo(GetTrack(zoneName), "P_NAME", NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolumeDisplay_Action(Layout* layout, string trackGUID) : TrackString_Action(layout, trackGUID) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        char buffer[128];
        sprintf(buffer, "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_VOL")));
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
    TrackSelect_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceSelected(GetTrack(zoneName), DAW::CSurf_OnSelectedChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED")), NULL);
        GetLayout()->GetManager()->OnTrackSelection(GetTrack(zoneName));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::SetOnlyTrackSelected(GetTrack(zoneName));
        GetLayout()->GetManager()->OnTrackSelection(GetTrack(zoneName));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRangeSelect_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        int selectedTrackNum = GetLayout()->GetZone(zoneName)->CSurf_TrackToID(GetTrack(zoneName));
        int otherSelectedTrackNum = 0;

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            for(int i = 0; i < GetLayout()->GetZone(zoneName)->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(GetLayout()->GetZone(zoneName)->CSurf_TrackFromID(i), "I_SELECTED"))
                {
                    otherSelectedTrackNum = i;
                    break;
                }

            int lowerBound = selectedTrackNum < otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            int upperBound = selectedTrackNum > otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            
            for(int i = lowerBound; i <= upperBound; i++)
            {
                MediaTrack* track = GetLayout()->GetZone(zoneName)->CSurf_TrackFromID(i);
                
                if(GetLayout()->GetZone(zoneName)->IsTrackVisible(track))
                {
                    DAW::CSurf_SetSurfaceSelected(GetTrack(zoneName), DAW::CSurf_OnSelectedChange(track, 1), NULL);
                    GetLayout()->GetManager()->OnTrackSelection(track);
                }
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRecordArm_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
   
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_RECARM"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceRecArm(GetTrack(zoneName), DAW::CSurf_OnRecArmChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "B_MUTE"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
         DAW::CSurf_SetSurfaceMute(GetTrack(zoneName), DAW::CSurf_OnMuteChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override { return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SOLO"); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        DAW::CSurf_SetSurfaceSolo(GetTrack(zoneName), DAW::CSurf_OnSoloChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouch_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackTouch_Action(Layout* layout, string trackGUID) : TrackDouble_Action(layout, trackGUID) {}

    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetTouchState(GetTrack(zoneName), value == 0 ? false : true);
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
    
    bool IsCurrentlyTouched() { return GetLayout()->GetTouchState(GetTrack(), 0); }
    
public:
    TrackTouchControlled_Action(string actionAddress, Layout* layout, string trackGUID, Action* action) : Track_Action(layout, trackGUID), action_(action), actionAddress_(actionAddress) {}
    
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
        for(int i = 0; i < GetLayout()->GetZone(zoneName)->GetNumTracks(); i++)
        {
            MediaTrack *track = GetLayout()->GetZone(zoneName)->CSurf_TrackFromID(i);
            
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
    TrackOutputMeter_Action(Layout* layout, string trackGUID, string channelStr) : TrackDouble_Action(layout, trackGUID)
    {
        channel_ = atol(channelStr.c_str());
    }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName) != nullptr)
            return VAL2DB(DAW::Track_GetPeakInfo(GetTrack(zoneName), channel_));
        else return 0.0;
    }
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
    TrackGainReductionMeter_Action(Layout* layout, string trackGUID, string fxGUID) : TrackDouble_Action(layout, trackGUID), fxGUID_(fxGUID)  {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        char buffer[BUFSZ];
        if(GetTrack(zoneName) != nullptr)
        {
            if(DAW::TrackFX_GetNamedConfigParm(GetTrack(zoneName), DAW::IndexFromFXGUID(GetTrack(zoneName), fxGUID_), "GainReduction_dB", buffer, sizeof(buffer)))
                return atof(buffer);
            else
                return 0.0;
        }
        else
            return 0.0;
    }
};
#endif /* control_surface_Reaper_actions_h */
