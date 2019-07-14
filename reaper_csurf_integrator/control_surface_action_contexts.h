//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

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
            Do(isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int sendIndex_ = 0;
    MediaTrack* sendTrack_ = nullptr;

public:
    TrackSendAction(WidgetActionManager* manager) : TrackAction(manager) {}
    
    virtual void SetIndex(int sendIndex) override { sendIndex_ = sendIndex; }
    virtual void SetTrack(MediaTrack* sendTrack) override { sendTrack_ = sendTrack; }

    virtual void RequestUpdate() override
    {
        if(sendTrack_)
            RequestTrackUpdate(sendTrack_);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(sendTrack_)
            Do(isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackActionWithIntFeedbackParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;
    
public:
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
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            if(shouldToggle_)
                DoToggle(isInverted_ == false ? value : 1.0 - value);
            else
                Do(isInverted_ == false ? value : 1.0 - value);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GlobalAction(WidgetActionManager* manager) : Action(manager) {}
       
    virtual void DoAction(double value) override
    {
        Do(isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalActionWithIntParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;
   
public:
    GlobalActionWithIntParam(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
        
    virtual void DoAction(double value) override
    {
        Do(param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string param_ = "";
    
public:
    GlobalActionWithStringParam(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
            param_ = params[1];
    }
    
    virtual void DoAction(double value) override
    {
        Do(param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface* surface_ = nullptr;
    
public:
    SurfaceAction(WidgetActionManager* manager) : Action(manager)
    {
        surface_ = widget_->GetSurface();
    }
    
    virtual void DoAction(double value) override
    {
        Do(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    ControlSurface* surface_ = nullptr;
    string param_ = "";
    
public:
    SurfaceActionWithStringParam(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 1)
            param_ = params[1];
        
        surface_ = widget_->GetSurface();
    }
    
    virtual void DoAction(double value) override
    {
        Do(value);
    }
};

#endif /* control_surface_action_contexts_h */
