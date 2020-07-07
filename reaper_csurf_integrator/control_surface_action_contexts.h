//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithIntParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ActionWithIntParam(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ActionWithStringParam(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ReaperAction(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    virtual void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::GetToggleCommandState(commandId_));
    }
    
    virtual void Do(double value) override
    {
        if(value != 0)
            DAW::SendCommandMessage(commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    TrackAction(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

public:
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
            RequestTrackUpdate(track);
        else
             ClearWidget();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
            Action::DoAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendAction(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackActionWithIntParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackActionWithIntParam(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXAction(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}
    
    virtual string GetDisplayName() override { return fxParamDisplayName_; }

    virtual string GetAlias() override
    {
        return fxParamDisplayName_;
    }

    virtual double GetCurrentValue() override
    {
        double min = 0.0;
        double max = 0.0;
        double retVal = 0.0;
        
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
            retVal = DAW::TrackFX_GetParam(track, GetZone()->GetSlotIndex(), paramIndex_, &min, &max);
        
        return retVal;
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
        {
            if(shouldUseDisplayStyle_)
                UpdateWidgetValue(displayStyle_, GetCurrentValue());
            else
                UpdateWidgetValue(GetCurrentValue());
        }
        else
             ClearWidget();
    }
};

#endif /* control_surface_action_contexts_h */
