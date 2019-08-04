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
class TrackActionWithIntFeedbackParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;

    TrackActionWithIntFeedbackParam(WidgetActionManager* manager, vector<string> params) : TrackAction(manager)
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
    string fxParamName_ = "";
    string fxParamNameAlias_ = "";
    int fxIndex_ = 0;

    FXAction(WidgetActionManager* manager, vector<string> params) : TrackAction(manager)
    {
        fxParamName_ = params[1];
        
        if(params.size() > 2)
            fxParamNameAlias_ = params[2];
        else
            fxParamNameAlias_ = params[1];
    }
    
public:
    
    virtual string GetAlias() override { return fxParamNameAlias_; }
    
    virtual void SetIndex(int fxIndex) override { fxIndex_ = fxIndex; }
        
    virtual void RequestUpdate() override
    {
        double min, max = 0;
        
        if(MediaTrack* track = widget_->GetTrack())
            SetWidgetValue(widget_, DAW::TrackFX_GetParam(track, fxIndex_, TheManager->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), &min, &max));
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
   
    void RequestUpdate() override {}

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
    
    void RequestUpdate() override {}

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
    
    void RequestUpdate() override {}

    SurfaceAction(WidgetActionManager* manager) : Action(manager)
    {
        surface_ = widget_->GetSurface();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface* surface_ = nullptr;
    string param_ = "";
    
    void RequestUpdate() override {}

    SurfaceActionWithStringParam(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
            param_ = params[1];
        
        surface_ = widget_->GetSurface();
    }
};

#endif /* control_surface_action_contexts_h */
