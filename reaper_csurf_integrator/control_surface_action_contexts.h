//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    
public:
    ReaperAction(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
        {
            string commandStr = params[1];
            
            commandId_ =  atol(commandStr.c_str());
            
            if(commandId_ == 0) // unsuccessful conversion to number
            {
                commandId_ = DAW::NamedCommandLookup(commandStr.c_str()); // look up by string
                
                if(commandId_ == 0) // can't find it
                    commandId_ = 65535; // no-op
            }
        }
    }
    
    virtual void RequestUpdate() override
    {
        SetWidgetValue(widget_, DAW::GetToggleCommandState(commandId_));
    }
    
    virtual void Do(double value) override
    {
        DAW::SendCommandMessage(commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    TrackAction(WidgetActionManager* manager) : Action(manager) { }

public:
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            RequestTrackUpdate(track);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            Action::DoAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int sendIndex_ = 0;
    
    TrackSendAction(WidgetActionManager* manager) : TrackAction(manager) {}

public:
    virtual void SetIndex(int sendIndex) override { sendIndex_ = sendIndex; }

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            RequestTrackUpdate(track);
        else
            widget_->Reset();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackActionWithIntParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;

    TrackActionWithIntParam(WidgetActionManager* manager, vector<string> params) : TrackAction(manager)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int fxParamIndex_ = 0;
    string fxParamDisplayName_ = "";
    int fxIndex_ = 0;

    FXAction(WidgetActionManager* manager, vector<string> params) : TrackAction(manager)
    {
        if(params.size() > 1)
            fxParamIndex_ = atol(params[1].c_str());
        
        if(params.size() > 2)
            fxParamDisplayName_ = params[2];
    }
    
public:
    
    virtual string GetDisplayName() override { return fxParamDisplayName_; }
    
    virtual void SetIndex(int fxIndex) override { fxIndex_ = fxIndex; }
        
    virtual void RequestUpdate() override
    {
        double min, max = 0;
        
        if(MediaTrack* track = widget_->GetTrack())
            SetWidgetValue(widget_, DAW::TrackFX_GetParam(track, fxIndex_, fxParamIndex_, &min, &max));
        else
            widget_->Reset();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithIntParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;

    ActionWithIntParam(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string param_ = "";

    ActionWithStringParam(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
            param_ = params[1];
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface* surface_ = nullptr;

    SurfaceAction(WidgetActionManager* manager) : Action(manager)
    {
        surface_ = widget_->GetSurface();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceActionWithStringParam : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface* surface_ = nullptr;
    
    SurfaceActionWithStringParam(WidgetActionManager* manager, vector<string> params) : ActionWithStringParam(manager, params)
    {
        surface_ = widget_->GetSurface();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceActionWithIntParam : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface* surface_ = nullptr;
    int param_ = 0;
    
    SurfaceActionWithIntParam(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params)
    {
        surface_ = widget_->GetSurface();
    }
};

#endif /* control_surface_action_contexts_h */
