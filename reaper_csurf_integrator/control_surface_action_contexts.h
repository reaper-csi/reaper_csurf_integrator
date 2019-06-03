//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    Widget* widget_ = nullptr;

public:
    TrackContext(WidgetActionContextManager* manager, Action* action) : ActionContext(manager, action)
    {
        widget_ = GetWidget();
    }

    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(this, track);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            action_->Do(widget_, track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSlotCycleContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string customSlotName_ = "";
    vector<ActionContext*> actionContexts_;
    
    int GetCurrentIndex()
    {
        if(MediaTrack* track = widget_->GetTrack())
            return page_->GetTrackSlotIndex(customSlotName_, track);
        else
            return 0;
    }
    
public:
    TrackSlotCycleContext(WidgetActionContextManager* manager, Action* action, string customModifierName) : TrackContext(manager, action), customSlotName_(customModifierName) {}
    
    virtual void AddActionContext(ActionContext* actionContext) override
    {
        actionContexts_.push_back(actionContext);
    }

    virtual void RequestUpdate() override
    {
        if(actionContexts_.size() > 0 && GetCurrentIndex() < actionContexts_.size())
            actionContexts_[GetCurrentIndex()]->RequestUpdate();
    }
    
    virtual void DoAction(double value) override
    {
        int index = GetCurrentIndex();
        
        if(actionContexts_.size() > 0 && index < actionContexts_.size())
            actionContexts_[index]->DoAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendContext(WidgetActionContextManager* manager, Action* action) : TrackContext(manager, action) {}
    
    // GAW TDB -- move some of this to SendsNavigationManager
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset < 0)
               widget_->Reset();
            else
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;

                action_->RequestUpdate(this, track, sendsOffset);
            }
        }
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->Do(widget_, track, sendsOffset, isInverted_ == false ? value : 1.0 - value);
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContextWithIntFeedbackParam : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
public:
    TrackContextWithIntFeedbackParam(WidgetActionContextManager* manager, Action* action, vector<string> params) : TrackContext(manager, action)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(this, track, param_);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            action_->Do(widget_, track, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string fxParamName_ = "";
    string fxParamNameAlias_ = "";
    int fxIndex_ = 0;

public:
    FXContext(WidgetActionContextManager* manager, Action* action, vector<string> params) : TrackContext(manager, action)
    {
        fxParamName_ = params[1];
        
        if(params.size() > 2)
            fxParamNameAlias_ = params[2];
        else
            fxParamNameAlias_ = params[1];
    }
    
    virtual string GetAlias() override { return fxParamNameAlias_; }
    
    virtual void SetIndex(int index) override { fxIndex_ = index; }
        
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(this, track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_));
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            if(shouldToggle_)
                action_->DoToggle(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
        }
    }
};

//////// The next Contexts don't use the double value, so they can safely override all flavours

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContextWithStringAndIntParams : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string stringParam_ = "";
    int intParam_ = 0;
    
public:
    TrackContextWithStringAndIntParams(WidgetActionContextManager* manager, Action* action, vector<string> params) : TrackContext(manager, action)
    {
        if(params.size() > 2)
        {
            stringParam_ = params[1];
            intParam_ = atol(params[2].c_str());
        }
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(this, track);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            if(widgetActionContextManager_ != nullptr)
                action_->Do(widget_, track, widgetActionContextManager_, stringParam_, intParam_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GlobalContext(WidgetActionContextManager* manager, Action* action) : ActionContext(manager, action) {}
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(this);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(page_, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperActionContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    
public:
    ReaperActionContext(WidgetActionContextManager* manager, Action* action, vector<string> params) : ActionContext(manager, action)
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
        action_->RequestUpdate(this, commandId_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithIntParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
   
public:
    GlobalContextWithIntParam(WidgetActionContextManager* manager, Action* action, vector<string> params) : ActionContext(manager, action)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(this, param_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(page_, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithStringParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string param_ = "";
    
public:
    GlobalContextWithStringParam(WidgetActionContextManager* manager, Action* action, vector<string> params) : ActionContext(manager, action)
    {
        if(params.size() > 1)
            param_ = params[1];
    }
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(this, param_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(page_, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWith2StringParams : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string param1_ = "";
    string param2_ = "";

public:
    GlobalContextWith2StringParams(WidgetActionContextManager* manager, Action* action, vector<string> params) : ActionContext(manager, action)
    {
        if(params.size() > 2)
        {
            param1_ = params[1];
            param2_ = params[2];
        }
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(page_, param1_, param2_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceContextWithStringParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    string param_ = "";
    
public:
    SurfaceContextWithStringParam(WidgetActionContextManager* manager, Action* action, vector<string> params) : ActionContext(manager, action)
    {
        if(params.size() > 1)
            param_ = params[1];
        
        surface_ = GetWidget()->GetSurface();
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(surface_, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPageSurfaceContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    ControlSurface* surface_ = nullptr;
    
public:
    TrackPageSurfaceContext(WidgetActionContextManager* manager, Action* action) : ActionContext(manager, action)
    {
        surface_ = GetWidget()->GetSurface();
    }
    
    virtual void DoAction(MediaTrack* track) override
    {
        action_->Do(page_, surface_, track);
    }
    
    virtual void DoAction(MediaTrack* track, int fxIndex) override
    {
        action_->Do(page_, surface_, track, fxIndex);
    }
};

#endif /* control_surface_action_contexts_h */
