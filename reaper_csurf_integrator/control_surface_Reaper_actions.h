   //
//  control_surface_Reaper_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_Reaper_actions_h
#define control_surface_Reaper_actions_h

#include "control_surface_base_actions.h"

extern Manager* TheManager;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int CommandId(string commandStr)
    {
        int commandId =  atol(commandStr.c_str());
        
        if(commandId != 0)
            return commandId; // successful conversion to number
        
        commandId = DAW::NamedCommandLookup(commandStr.c_str()); // look up by string
        
        if(commandId == 0) // can't find it
            commandId = 65535; // no-op

        return commandId;
    }
    
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        if(params.size() > 1)
            widget->SetValue(DAW::GetToggleCommandState(CommandId(params[1])));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        if(params.size() > 1)
            DAW::SendCommandMessage(CommandId(params[1]));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackFX : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{   
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        double min, max = 0;
        //manager->SetWidgetValue(widgetGUID, DAW::TrackFX_GetParam(manager->GetTrack(widgetGUID), manager->GetFXIndex(widgetGUID), manager->GetFXParamIndex(widgetGUID), &min, &max));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        //DAW::TrackFX_SetParam(manager->GetTrack(widgetGUID), manager->GetFXIndex(widgetGUID), manager->GetFXParamIndex(widgetGUID), value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        if(MediaTrack* track = page->GetTrack(widget))
            widget->SetValue(volToNormalized(DAW::GetMediaTrackInfo_Value(track, "D_VOL")));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        if(MediaTrack* track = page->GetTrack(widget))
            DAW::CSurf_SetSurfaceVolume(track, DAW::CSurf_OnVolumeChange(track, normalizedToVol(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPan : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, panToNormalized(DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "D_PAN")));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        //MediaTrack* track = manager->GetTrack(widgetGUID);
        //DAW::CSurf_SetSurfacePan(track, DAW::CSurf_OnPanChange(track, normalizedToPan(value), false), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidth : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, panToNormalized(DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "D_WIDTH")));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        //DAW::CSurf_OnWidthChange(manager->GetTrack(widgetGUID), normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        if(MediaTrack* track = page->GetTrack(widget))
        {
            string trackName = "";
            
            if(DAW::GetMediaTrackInfo_Value(track , "IP_TRACKNUMBER") == -1)
                trackName = "Master";
            else
                trackName =  (char *)DAW::GetSetMediaTrackInfo(track, "P_NAME", NULL);

            widget->SetValue(trackName);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolumeDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);manager->GetTrack(widgetGUID);
        char trackVolume[128];
        sprintf(trackVolume, "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(track, "D_VOL")));
        //manager->SetWidgetValue(widgetGUID, string(trackVolume));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        bool left = false;
        
        double panVal = 0.0; //DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "D_PAN");
        
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
        
        //manager->SetWidgetValue(widgetGUID, string(trackPan));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        bool reversed = false;
        
        double widthVal = 0.0; //DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "D_WIDTH");
        
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

        //manager->SetWidgetValue(widgetGUID, string(trackPanWidth));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        DAW::CSurf_OnRew(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastForward : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Widget* widget, Page* page, vector<string> & params, double value)
    {
        DAW::CSurf_OnFwd(1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Play : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        int playState = DAW::GetPlayState();
        if(playState == 1 || playState == 2 || playState == 5 || playState == 6) // playing or paused or recording or paused whilst recording
            playState = 1;
        else playState = 0;
        widget->SetValue(playState);
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        DAW::CSurf_OnPlay();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Stop : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        int stopState = DAW::GetPlayState();
        if(stopState == 0 || stopState == 2 || stopState == 6) // stopped or paused or paused whilst recording
            stopState = 1;
        else stopState = 0;
        
        widget->SetValue(stopState);
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        DAW::CSurf_OnStop();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Record : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        int recordState = DAW::GetPlayState();
        if(recordState == 5 || recordState == 6) // recording or paused whilst recording
            recordState = 1;
        else recordState = 0;
        
        widget->SetValue(recordState);
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        DAW::CSurf_OnRecord();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RepeatingArrow : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    /*
private:
    int direction_ = 0;
    clock_t lastRepeated = clock();
    double repeatRate_ = 0.0;
    bool pressed_ = false;
    
public:
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        if(pressed_ && clock() - lastRepeated >  CLOCKS_PER_SEC * repeatRate_)
        {
            lastRepeated = clock();
            // GAW TBD
            //DAW::CSurf_OnArrow(direction_, Getlayer->GetRealSurfaceFor(surfaceName)->IsZoom());
        }
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        // GAW TBD
        // DAW::CSurf_OnArrow(direction_, Getlayer->GetRealSurfaceFor(surfaceName)->IsZoom());
        pressed_ = value;
    }
     */
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "I_SELECTED"));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);
        DAW::CSurf_SetSurfaceSelected(track, DAW::CSurf_OnSelectedChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SELECTED")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "I_SELECTED"));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);
        DAW::SetOnlyTrackSelected(track);
        TheManager->OnTrackSelection(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "I_SELECTED"));
    }

    virtual void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);
        vector<MediaTrack*> tracks; //manager->GetSurfaceTracks(widgetGUID);
        
        int currentlySelectedCount = 0;
        int selectedTrackIndex = 0;
        int trackIndex = 0;
        
        for(int i = 0; i < tracks.size(); i++)
        {
            if(tracks[i] == track)
                trackIndex = i;
            
            if(DAW::GetMediaTrackInfo_Value(tracks[i], "I_SELECTED"))
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
            DAW::CSurf_SetSurfaceSelected(tracks[i], DAW::CSurf_OnSelectedChange(tracks[i], 1), NULL);
            TheManager->OnTrackSelection(tracks[i]);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRecordArm : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "I_RECARM"));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);
        DAW::CSurf_SetSurfaceRecArm(track, DAW::CSurf_OnRecArmChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "B_MUTE"));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);
        DAW::CSurf_SetSurfaceMute(track, DAW::CSurf_OnMuteChange(track, ! DAW::GetMediaTrackInfo_Value(track, "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(manager->GetTrack(widgetGUID), "I_SOLO"));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        MediaTrack* track = nullptr; //manager->GetTrack(widgetGUID);
        DAW::CSurf_SetSurfaceSolo(track, DAW::CSurf_OnSoloChange(track, ! DAW::GetMediaTrackInfo_Value(track, "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouch : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        //manager->SetTouchState(manager->GetTrack(widgetGUID), value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlled : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    /*
private:
    OldAction* action_= nullptr;
    string actionAddress_= nullptr;
    
    bool lastTouched_ = false;
    
    bool IsCurrentlyTouched(string zoneName)
    {
        if(GetTrack(zoneName))
            return GetLayer()->GetTouchState(GetTrack(zoneName), 0);
        else
            return false;
    }
    
public:
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        bool currentlyTouched = IsCurrentlyTouched(zoneName);
        
        if(currentlyTouched)
        {
            lastTouched_ = currentlyTouched;
            action_->Update(zoneName, surfaceName, widgetName);
        }
        else if(lastTouched_ != currentlyTouched)
        {
            lastTouched_ = currentlyTouched;
            GetLayer()->ForceUpdateAction(actionAddress_, zoneName, surfaceName, widgetName);
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
        if(IsCurrentlyTouched(zoneName))
            action_->ForceUpdate(zoneName, surfaceName, widgetName);
    }
    
    virtual void Cycle(string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched(zoneName))
            action_->Cycle(zoneName, surfaceName, widgetName);
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched(zoneName))
            action_->Do(value, zoneName, surfaceName, widgetName);
    }
    */
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAutoMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        widget->SetValue(DAW::GetGlobalAutomationOverride());
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double autoMode) override
    {
        if(params.size() > 1)
            DAW::SetGlobalAutomationOverride(atol(params[1].c_str()));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAutoMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //vector<MediaTrack*> tracks = manager->GetSurfaceTracks(widgetGUID);
        
        //for(int i = 0; i < tracks.size(); i++)
            //if(DAW::GetMediaTrackInfo_Value(tracks[i], "I_SELECTED"))
                //manager->SetWidgetValue(widgetGUID, DAW::GetMediaTrackInfo_Value(tracks[i], "I_AUTOMODE"));
    }
    
    virtual void Do(Widget* widget, Page* page, vector<string> & params, double autoMode) override
    {
        DAW::SetAutomationMode(autoMode, true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeline : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        widget->SetValue(DAW::GetSetRepeatEx(nullptr, -1));
    }
    
    void Do(Widget* widget, Page* page, vector<string> & params, double value) override
    {
        DAW::GetSetRepeatEx(nullptr, ! DAW::GetSetRepeatEx(nullptr, -1));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackOutputMeter : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        //if(DAW::GetPlayState() & 0x01) // if playing
            //manager->SetWidgetValue(widgetGUID, VAL2DB(DAW::Track_GetPeakInfo(manager->GetTrack(widgetGUID), manager->GetChannel(widgetGUID))));
        //else
            //manager->SetWidgetValue(widgetGUID, 0.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackGainReductionMeter : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(Widget* widget, Page* page, vector<string> & params) override
    {
        if(DAW::GetPlayState() & 0x01) // if playing
        {
            char buffer[BUFSZ];

            //if(DAW::TrackFX_GetNamedConfigParm(manager->GetTrack(widgetGUID), manager->GetFXIndex(widgetGUID), "GainReduction_dB", buffer, sizeof(buffer)))
               //manager->SetWidgetValue(widgetGUID, -atof(buffer)/20.0);
            //else
               //manager->SetWidgetValue(widgetGUID, 0.0);
        }
        //else
            //manager->SetWidgetValue(widgetGUID, 1.0);
    }
};



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Track_Action : public OldAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string trackGUID_ = "";
protected:
    MediaTrack* GetTrack(string zoneName)
    {
        return GetLayer()->GetZone(zoneName)->GetTrackFromGUID(trackGUID_);
    }

public:
    Track_Action(Layer* layer, string trackGUID) : OldAction(layer), trackGUID_(trackGUID) {}
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
        return GetLayer()->GetZone(zoneName)->GetTrackFromGUID(trackGUID_);
    }

public:
    TrackDouble_Action(Layer* layer, string trackGUID) : Double_Action(layer), trackGUID_(trackGUID) {}
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
        return GetLayer()->GetZone(zoneName)->GetTrackFromGUID(trackGUID_);
    }
    
public:
    TrackString_Action(Layer* layer, string trackGUID) : String_Action(layer), trackGUID_(trackGUID) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OldReaper_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_= 0;
    
public:
    OldReaper_Action(Layer* layer, string commandStr) : Double_Action(layer)
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
    TrackFX_Action(Layer* layer, string trackGUID, string fxGUID, int paramIndex) : TrackDouble_Action(layer, trackGUID), fxGUID_(fxGUID), paramIndex_(paramIndex) {}
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        double min = 0;
        double max = 0;
        if(GetTrack(zoneName))
            return DAW::TrackFX_GetParam(GetTrack(zoneName), DAW::IndexFromFXGUID(GetTrack(zoneName), fxGUID_), paramIndex_, &min, &max);
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            DAW::TrackFX_SetParam(GetTrack(zoneName), DAW::IndexFromFXGUID(GetTrack(zoneName), fxGUID_), paramIndex_, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackAndFXToWidgets_Action  : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapTrackAndFXToWidgets_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
        {
            if(1 == DAW::CountSelectedTracks(nullptr))
                GetLayer()->MapTrackAndFXToWidgets(GetTrack(zoneName), zoneName, surfaceName);
            else
                GetLayer()->UnmapWidgetsFromTrack(GetTrack(zoneName), zoneName, surfaceName);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackVolume_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackVolume_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
 
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        //double widgetMaxDB = Getlayer->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = Getlayer->GetWidgetMinDB(surfaceName, widgetName);
        
        //Getlayer->SetWidgetValue(surfaceName, widgetName, clampedAndNormalized(VAL2DB(value), widgetMaxDB, widgetMinDB));
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, volToNormalized(value));
    }
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override
    {
        //double widgetMaxDB = Getlayer->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = Getlayer->GetWidgetMinDB(surfaceName, widgetName);

        //return volToNormalized(currentValue_, widgetMaxDB, widgetMinDB);
        return volToNormalized(currentValue_);
    }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_VOL");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        //double widgetMaxDB = Getlayer->GetWidgetMaxDB(surfaceName, widgetName);
        //double widgetMinDB = Getlayer->GetWidgetMinDB(surfaceName, widgetName);

        //DAW::CSurf_SetSurfaceVolume(track_, DAW::CSurf_OnVolumeChange(track_, normalizedToVol(value, widgetMaxDB, widgetMinDB), false), NULL);
        if(GetTrack(zoneName))
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
    TrackPan_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}

    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return panToNormalized(currentValue_); }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_PAN");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
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
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, panToNormalized(value), displayMode_);
    }
    
public:
    TrackPanWidth_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}

    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return panToNormalized(currentValue_); }

    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_WIDTH");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            DAW::CSurf_OnWidthChange(GetTrack(zoneName), normalizedToPan(value), false);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackNameDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackNameDisplay_Action(Layer* layer, string trackGUID) : TrackString_Action(layer, trackGUID) {}
    
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
    TrackVolumeDisplay_Action(Layer* layer, string trackGUID) : TrackString_Action(layer, trackGUID) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        char buffer[128];
        memset(buffer, 0, sizeof(buffer));
        if(GetTrack(zoneName))
            sprintf(buffer, "%7.2lf", VAL2DB(DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_VOL")));
        return string(buffer);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPanDisplay_Action(Layer* layer, string trackGUID) : TrackString_Action(layer, trackGUID) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName) == nullptr)
            return "";
        
        bool left = false;
        
        double panVal = DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_PAN");
        
        if(panVal < 0)
        {
            left = true;
            panVal = -panVal;
        }
        
        int panIntVal = int(panVal * 100.0);
        string displayStr = "";
        
        if(left)
        {
            if(panIntVal == 100)
                displayStr += "<";
            else if(panIntVal < 100 && panIntVal > 9)
                displayStr += "< ";
            else
                displayStr += "<  ";

            displayStr += to_string(panIntVal);
        }
        else
        {
            displayStr += "   ";
            
            displayStr += to_string(panIntVal);

            if(panIntVal == 100)
                displayStr += ">";
            else if(panIntVal < 100 && panIntVal > 9)
                displayStr += " >";
            else
                displayStr += "  >";
        }
        
        if(panIntVal == 0)
            displayStr = "  <C>  ";
        
        return displayStr;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPanWidthDisplay_Action : public TrackString_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPanWidthDisplay_Action(Layer* layer, string trackGUID) : TrackString_Action(layer, trackGUID) {}
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName) == nullptr)
            return "";

        bool reversed = false;
        
        double widthVal = DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "D_WIDTH");
        
        if(widthVal < 0)
        {
            reversed = true;
            widthVal = -widthVal;
        }
        
        int widthIntVal = int(widthVal * 100.0);
        string displayStr = "";
        
        if(reversed)
            displayStr += "Rev ";

        displayStr += to_string(widthIntVal);
        
        if(widthIntVal == 0)
            displayStr = " <Mno> ";
        
        return displayStr;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rewind_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Rewind_Action(Layer* layer) : Double_Action(layer)  {}
    
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
    FastForward_Action(Layer* layer) : Double_Action(layer) {}
    
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
    Play_Action(Layer* layer) : Double_Action(layer) {}
    
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
    Stop_Action(Layer* layer) : Double_Action(layer) {}
    
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
    Record_Action(Layer* layer) : Double_Action(layer) {}
    
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
    RepeatingArrow_Action(Layer* layer, int direction, double repeatRate) : Double_Action(layer), direction_(direction), repeatRate_(repeatRate) {}

    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        if(pressed_ && clock() - lastRepeated >  CLOCKS_PER_SEC * repeatRate_)
        {
            lastRepeated = clock();
            // GAW TBD
            //DAW::CSurf_OnArrow(direction_, Getlayer->GetRealSurfaceFor(surfaceName)->IsZoom());
        }
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        // GAW TBD
        // DAW::CSurf_OnArrow(direction_, Getlayer->GetRealSurfaceFor(surfaceName)->IsZoom());
        pressed_ = value;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSelect_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
        {
            DAW::CSurf_SetSurfaceSelected(GetTrack(zoneName), DAW::CSurf_OnSelectedChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED")), NULL);
            GetLayer()->GetManager()->OnTrackSelection(GetTrack(zoneName));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackUniqueSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackUniqueSelect_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
        {
            DAW::SetOnlyTrackSelected(GetTrack(zoneName));
            GetLayer()->GetManager()->OnTrackSelection(GetTrack(zoneName));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackRangeSelect_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackRangeSelect_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SELECTED");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName) == nullptr)
            return;
        
        int selectedTrackNum = GetLayer()->GetZone(zoneName)->CSurf_TrackToID(GetTrack(zoneName));
        int otherSelectedTrackNum = 0;

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            for(int i = 0; i < GetLayer()->GetZone(zoneName)->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(GetLayer()->GetZone(zoneName)->CSurf_TrackFromID(i), "I_SELECTED"))
                {
                    otherSelectedTrackNum = i;
                    break;
                }

            int lowerBound = selectedTrackNum < otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            int upperBound = selectedTrackNum > otherSelectedTrackNum ? selectedTrackNum : otherSelectedTrackNum;
            
            for(int i = lowerBound; i <= upperBound; i++)
            {
                MediaTrack* track = GetLayer()->GetZone(zoneName)->CSurf_TrackFromID(i);
                
                if(GetLayer()->GetZone(zoneName)->IsTrackVisible(track))
                {
                    DAW::CSurf_SetSurfaceSelected(GetTrack(zoneName), DAW::CSurf_OnSelectedChange(track, 1), NULL);
                    GetLayer()->GetManager()->OnTrackSelection(track);
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
    TrackRecordArm_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
   
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_RECARM");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            DAW::CSurf_SetSurfaceRecArm(GetTrack(zoneName), DAW::CSurf_OnRecArmChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_RECARM")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackMute_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackMute_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "B_MUTE");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            DAW::CSurf_SetSurfaceMute(GetTrack(zoneName), DAW::CSurf_OnMuteChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "B_MUTE")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSolo_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSolo_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            return DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SOLO");
        else
            return 0.0;
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            DAW::CSurf_SetSurfaceSolo(GetTrack(zoneName), DAW::CSurf_OnSoloChange(GetTrack(zoneName), ! DAW::GetMediaTrackInfo_Value(GetTrack(zoneName), "I_SOLO")), NULL);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouch_Action : public TrackDouble_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackTouch_Action(Layer* layer, string trackGUID) : TrackDouble_Action(layer, trackGUID) {}

    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(GetTrack(zoneName))
            GetLayer()->SetTouchState(GetTrack(zoneName), value == 0 ? false : true);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlled_Action : public Track_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    OldAction* action_= nullptr;
    string actionAddress_= nullptr;
    
    bool lastTouched_ = false;
    
    bool IsCurrentlyTouched(string zoneName)
    {
        if(GetTrack(zoneName))
            return GetLayer()->GetTouchState(GetTrack(zoneName), 0);
        else
            return false;
    }
    
public:
    TrackTouchControlled_Action(string actionAddress, Layer* layer, string trackGUID, OldAction* action) : Track_Action(layer, trackGUID), action_(action), actionAddress_(actionAddress) {}
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        bool currentlyTouched = IsCurrentlyTouched(zoneName);
        
        if(currentlyTouched)
        {
            lastTouched_ = currentlyTouched;
            action_->Update(zoneName, surfaceName, widgetName);
        }
        else if(lastTouched_ != currentlyTouched)
        {
            lastTouched_ = currentlyTouched;
            GetLayer()->ForceUpdateAction(actionAddress_, zoneName, surfaceName, widgetName);
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
        if(IsCurrentlyTouched(zoneName))
            action_->ForceUpdate(zoneName, surfaceName, widgetName);
    }
    
    virtual void Cycle(string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched(zoneName))
            action_->Cycle(zoneName, surfaceName, widgetName);
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(IsCurrentlyTouched(zoneName))
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
    GlobalAutoMode_Action(Layer* layer, string autoModeStr) : Double_Action(layer)
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
    TrackAutoMode_Action(Layer* layer, string autoModeStr) : GlobalAutoMode_Action(layer, autoModeStr) {}
    
    virtual double GetValue (string zoneName, string surfaceName, string widgetName) override
    {
        for(int i = 0; i < GetLayer()->GetZone(zoneName)->GetNumTracks(); i++)
        {
            MediaTrack *track = GetLayer()->GetZone(zoneName)->CSurf_TrackFromID(i);
            
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
    CycleTimeline_Action(Layer* layer) : Double_Action(layer)  {}
    
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
    Cancel_Action(Layer* layer) : Double_Action(layer)  {}
    // GAW TBD
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Enter_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Enter_Action(Layer* layer) : Double_Action(layer)  {}
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
              GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName,
                                                clampedAndNormalized(value, GetLayer()->GetWidgetMaxDB(zoneName, surfaceName, widgetName), GetLayer()->GetWidgetMinDB(zoneName, surfaceName, widgetName)));
        else
            GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, 0.0);
    }
    
public:
    TrackOutputMeter_Action(Layer* layer, string trackGUID, string channelStr) : TrackDouble_Action(layer, trackGUID)
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
            //Getlayer->SetWidgetValue(surfaceName, widgetName, clampedAndNormalized(-value, Getlayer->GetWidgetMaxDB(surfaceName, widgetName), Getlayer->GetWidgetMinDB(surfaceName, widgetName)));
            GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, -value / 20.0); // GAW TBD hacked for now
        else
            GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, 1.0);
    }
    
public:
    TrackGainReductionMeter_Action(Layer* layer, string trackGUID, string fxGUID) : TrackDouble_Action(layer, trackGUID), fxGUID_(fxGUID)  {}
    
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
