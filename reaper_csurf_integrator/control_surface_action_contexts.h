//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NoAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NoAction() {}
    NoAction(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    virtual ~NoAction() {}
    
    virtual void RequestUpdate() override {  ClearWidget(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ReaperAction() {}
    ReaperAction(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    virtual void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::GetToggleCommandState(GetCommandId()));
    }
    
    virtual void Do(double value) override
    {
        if(value != 0)
            DAW::SendCommandMessage(GetCommandId());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    FXAction() {}
    FXAction(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    virtual string GetDisplayName() override { return GetFxParamDisplayName(); }

    virtual string GetAlias() override
    {
        return GetFxParamDisplayName();
    }

    virtual double GetCurrentValue() override
    {
        double min = 0.0;
        double max = 0.0;
        double retVal = 0.0;
        
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
            retVal = DAW::TrackFX_GetParam(track, GetZone()->GetSlotIndex(), GetParamIndex(), &min, &max);
        
        return retVal;
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
        {
            if(GetShouldUseDisplayStyle())
                UpdateWidgetValue(GetDisplayStyle(), GetCurrentValue());
            else
                UpdateWidgetValue(GetCurrentValue());
        }
        else
             ClearWidget();
    }
};

#endif /* control_surface_action_contexts_h */
