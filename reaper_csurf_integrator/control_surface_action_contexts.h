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
            Do(widget_, track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int sendsIndex_ = 0;
    MediaTrack* track_ = nullptr;

public:
    TrackSendAction(WidgetActionManager* manager) : TrackAction(manager) {}
    
    virtual void SetIndex(int sendsIndex) override { sendsIndex_ = sendsIndex; }
    virtual void SetTrack(MediaTrack* track) override { track_ = track; }

    virtual void RequestUpdate() override
    {
        if(track_)
            RequestSendUpdate(track_, sendsIndex_);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(track_)
            Do(widget_, track_, sendsIndex_, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackActionWithIntFeedbackParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
public:
    TrackActionWithIntFeedbackParam(WidgetActionManager* manager, vector<string> params) : TrackAction(manager)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            RequestTrackUpdateWithIntParam(track, param_);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            Do(widget_, track, value);
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
            SetWidgetValue(widget_, DAW::TrackFX_GetParam(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), &min, &max));
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            if(shouldToggle_)
                DoToggle(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                Do(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
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
        Do(page_, isInverted_ == false ? value : 1.0 - value);
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
        Do(page_, param_);
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
        Do(page_, param_);
    }
};
/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    
public:
    SurfaceAction(WidgetActionManager* manager) : Action(manager)
    {
        surface_ = GetWidget()->GetSurface();
    }
    
    virtual void DoAction(double value) override
    {
        Do(page_, surface_);
    }
};
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
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
        Do(surface_, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceActionWith2StringParams : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    string param1_ = "";
    string param2_ = "";
    
public:
    SurfaceActionWith2StringParams(WidgetActionManager* manager, vector<string> params) : Action(manager)
    {
        if(params.size() > 2)
        {
            param1_ = params[1];
            param2_ = params[2];
        }
        
        surface_ = widget_->GetSurface();
    }
    
    virtual void DoAction(double value) override
    {
        Do(surface_, param1_, param2_);
    }
};
#endif /* control_surface_action_contexts_h */
