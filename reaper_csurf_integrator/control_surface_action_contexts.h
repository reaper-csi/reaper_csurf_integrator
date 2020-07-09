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
    virtual void RequestUpdate(ActionContext* context) override {  context->ClearWidget(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(DAW::GetToggleCommandState(context->GetCommandId()));
    }
    
    virtual void Do(ActionContext* context, double value) override
    {
        if(value != 0)
            DAW::SendCommandMessage(context->GetCommandId());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual void RequestUpdate(ActionContext* context) override
    {
        if(MediaTrack* track = context->GetTrack())
        {
            if(context->GetShouldUseDisplayStyle())
                context->UpdateWidgetValue(context->GetDisplayStyle(), GetCurrentValue(context));
            else
                context->UpdateWidgetValue(GetCurrentValue(context));
        }
        else
            ClearWidget();
    }

    virtual double GetCurrentValue(ActionContext* context) override
    {
        double min = 0.0;
        double max = 0.0;
        double retVal = 0.0;
        
        if(MediaTrack* track = context->GetTrack())
            retVal = DAW::TrackFX_GetParam(track, context->GetSlotIndex(), context->GetParamIndex(), &min, &max);
        
        return retVal;
    }
};

#endif /* control_surface_action_contexts_h */
