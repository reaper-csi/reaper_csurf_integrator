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
protected:
    int param_ = 0;
    
    ActionWithIntParam(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params)
    {
        if(params.size() > 0)
            param_= atol(params[0].c_str());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string param_ = "";
    
    ActionWithStringParam(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params)
    {
        if(params.size() > 0)
            param_ = params[0];
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    string commandStr_ = "";
    
public:
    ReaperAction(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params)
    {
        if(params.size() > 0)
        {
            commandStr_ = params[0];
            
            commandId_ =  atol(commandStr_.c_str());
            
            if(commandId_ == 0) // unsuccessful conversion to number
            {
                commandId_ = DAW::NamedCommandLookup(commandStr_.c_str()); // look up by string
                
                if(commandId_ == 0) // can't find it
                    commandId_ = 65535; // no-op
            }
        }
    }

    virtual void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::GetToggleCommandState(commandId_));
    }
    
    virtual void Do(double value, Widget* sender) override
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
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            RequestTrackUpdate(track);
        else
             ClearWidget();
    }
    
    virtual void DoAction(double value, Widget* sender) override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            Action::DoAction(value, sender);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int paramIndex_ = 0;
    
    TrackSendAction(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params)
    {
        if(params.size() > 0)
        {
            if(isdigit(params[0][0])) // C++ 11 says empty strings can be queried without catastrophe :)
                paramIndex_ = atol(params[0].c_str());
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackActionWithIntParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;

    TrackActionWithIntParam(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params)
    {
        if(params.size() > 0)
            param_= atol(params[0].c_str());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int fxParamIndex_ = 0;
    string fxParamDisplayName_ = "";

    FXAction(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params)
    {
        if(params.size() > 0)
            fxParamIndex_ = atol(params[0].c_str());
        
        if(params.size() > 1)
            fxParamDisplayName_ = params[1];
        
        if(params.size() > 2 && params[2] != "[" && params[2] != "{")
        {
            shouldUseDisplayStyle_ = true;
            displayStyle_ = atol(params[2].c_str());
        }
    }
    
public:
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
        
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            retVal = DAW::TrackFX_GetParam(track, zone_->GetSlotIndex(), fxParamIndex_, &min, &max);
        
        return retVal;
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
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
