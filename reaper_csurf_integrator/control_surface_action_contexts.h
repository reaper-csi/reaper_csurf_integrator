//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    void ExecuteAction(double value)
    {
        action_->Do(page_, isInverted_ == false ? value : 1.0 - value);
    }
    
public:
    GlobalContext(Page* page, ControlSurface* surface, Widget* widget, Action* action) : ActionContext(page, surface, widget, action) {}
    
    virtual void RequestActionUpdate() override
    {
        action_->RequestUpdate(page_, this, widget_);
    }
    
    virtual void DoAction( double value) override
    {
        ExecuteAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    void ExecuteAction(double value)
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->Do(page_, widget_, track, isInverted_ == false ? value : 1.0 - value);
    }
    
protected:
    string trackGUID_ = "";
    
public:
    TrackContext(Page* page, ControlSurface* surface, Widget* widget, Action* action) : ActionContext(page, surface, widget, action) {}
    
    virtual void  SetTrack(string trackGUID) override
    {
        trackGUID_ = trackGUID;
    }
    
    virtual void RequestActionUpdate() override
    {
        if(shouldExecute_ && DAW::GetCurrentNumberOfMilliseconds() > delayStartTime_ + delayAmount_)
        {
            shouldExecute_ = false;
            ExecuteAction(valueForDelayedExecution_);
        }
        
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->RequestUpdate(page_, this, widget_, track);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(delayAmount_ == 0)
            ExecuteAction(value);
        else
        {
            if(value == 0.0)
            {
               shouldExecute_ = false;
            }
            else
            {
                valueForDelayedExecution_ = value;
                delayStartTime_ = DAW::GetCurrentNumberOfMilliseconds();
                shouldExecute_ = true;
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    void ExecuteAction(double value)
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->Do(page_, widget_, track, sendsOffset, isInverted_ == false ? value : 1.0 - value);
            }
        }
    }
    
public:
    TrackSendContext(Page* page, ControlSurface* surface, Widget* widget, Action* action) : TrackContext(page, surface, widget, action) {}
    
    virtual void RequestActionUpdate() override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset < 0)
               widget_->Reset();
            else
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;

                action_->RequestUpdate(page_, this, widget_, track, sendsOffset);
            }
        }
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContextWithIntParam : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
    void ExecuteAction()
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->Do(page_, surface_, track, param_);
    }
    
public:
    TrackContextWithIntParam(Page* page, ControlSurface* surface, Widget* widget, Action* action, int param) : TrackContext(page, surface, widget, action), param_(param) {}
    
    virtual void RequestActionUpdate() override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->RequestUpdate(page_, this, widget_, track, param_);
        else
            widget_->Reset();
    }
    
    virtual void DoAction( double value) override
    {
        ExecuteAction();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string fxParamNameAlias_ = "";
    string fxParamName_ = "";
    int fxIndex_ = 0;
    
    void ExecuteAction(double value)
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            if(shouldToggle_)
                action_->DoToggle(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
        }
    }

public:
    FXContext(Page* page, ControlSurface* surface, Widget* widget, Action* action, string fxParamName) : TrackContext(page, surface, widget, action), fxParamName_(fxParamName) {}
    
    virtual void SetAlias(string alias) override { fxParamNameAlias_ = alias; }
    
    virtual string GetAlias() override { return fxParamNameAlias_; }
    
    virtual void SetIndex(int index) override { fxIndex_ = index; }
        
    virtual void RequestActionUpdate() override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()) )
            action_->RequestUpdate(this, widget_, track, fxIndex_, page_->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_));
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperActionContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    
    void ExecuteAction()
    {
        action_->Do(page_, commandId_);
    }

    
public:
    ReaperActionContext(Page* page, ControlSurface* surface, Widget* widget, Action* action, string commandStr) : ActionContext(page, surface, widget, action)
    {
        commandId_ =  atol(commandStr.c_str());
        
        if(commandId_ == 0) // unsuccessful conversion to number
        {
        
        commandId_ = DAW::NamedCommandLookup(commandStr.c_str()); // look up by string
        
        if(commandId_ == 0) // can't find it
            commandId_ = 65535; // no-op
        }
    }
    
    virtual void RequestActionUpdate() override
    {
        action_->RequestUpdate(page_, this, widget_, commandId_);
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithIntParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
    void ExecuteAction()
    {
        action_->Do(page_, param_);
    }
    
public:
    GlobalContextWithIntParam(Page* page, ControlSurface* surface, Widget* widget, Action* action, int param) : ActionContext(page, surface, widget, action), param_(param) {}
    
    virtual void RequestActionUpdate() override
    {
        action_->RequestUpdate(page_, this, widget_, param_);
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithStringParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string param_ = "";
    
    void ExecuteAction()
    {
        action_->Do(page_, param_);
    }
    
public:
    GlobalContextWithStringParam(Page* page, ControlSurface* surface, Widget* widget, Action* action, string param) : ActionContext(page, surface, widget, action), param_(param) {}
    
    virtual void RequestActionUpdate() override
    {
        action_->RequestUpdate(page_, this, widget_, param_);
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlledContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* touchAction_ = nullptr;
    
    void ExecuteAction(double value)
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            if(page_->GetTouchState(track, 0))
                touchAction_->Do(page_, widget_, track, isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(page_, widget_, track, isInverted_ == false ? value : 1.0 - value);
        }
    }

public:
    TrackTouchControlledContext(Page* page, ControlSurface* surface, Widget* widget, Action* action, Action* touchAction) : TrackContext(page, surface, widget, action), touchAction_(touchAction) {}
    
    virtual void RequestActionUpdate() override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            if(page_->GetTouchState(track, 0))
                touchAction_->RequestUpdate(page_, this, widget_, track);
            else
                action_->RequestUpdate(page_, this, widget_, track);
        }
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendTouchControlledContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* touchAction_ = nullptr;
    
     void ExecuteAction(double value)
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(page_->GetTouchState(track, 0))
                    touchAction_->Do(page_, widget_, track, isInverted_ == false ? value : 1.0 - value);
                else
                    action_->Do(page_, widget_, track, isInverted_ == false ? value : 1.0 - value);
            }
        }
    }
    
public:
    TrackSendTouchControlledContext(Page* page, ControlSurface* surface, Widget* widget, Action* action, Action* touchAction) : TrackContext(page, surface, widget, action), touchAction_(touchAction) {}
    
    virtual void RequestActionUpdate() override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset < 0)
                widget_->Reset();
            else
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(page_->GetTouchState(track, 0))
                    touchAction_->RequestUpdate(page_, this, widget_, track, sendsOffset);
                else
                    action_->RequestUpdate(page_, this, widget_, track, sendsOffset);
            }
       }
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackCycleContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Widget* cyclerWidget_ = nullptr;
    int index = 0;
    vector<ActionContext*> actionContexts_;
    
    void ExecuteAction(double value)
    {
        if(widget_ && widget_ == cyclerWidget_)
        {
            if(value)
                index = index < actionContexts_.size() - 1 ? index + 1 : 0;
        }
        else if(actionContexts_[index])
        {
            if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
                actionContexts_[index]->DoAction(value);
        }
    }
    
public:
    TrackCycleContext(Page* page, ControlSurface* surface, Widget* widget, Action* action, vector<string> params) : TrackContext(page, surface, widget, action)
    {
        for(int i = 2; i < params.size(); i++)
        {
            istringstream iss(params[i]);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);

            if(ActionContext* context = TheManager->GetActionContext(page_, surface_, widget_, tokens))
            {
                context->SetTrack(trackGUID_);
                actionContexts_.push_back(context);
            }
        }
    }
    
    virtual void  SetTrack(string trackGUID) override
    {
        trackGUID_ = trackGUID;
        for(auto context : actionContexts_)
            context->SetTrack(trackGUID_);
    }
    
    virtual void SetCyclerWidget(Widget* cyclerWidget) override { cyclerWidget_ = cyclerWidget; }
    
    virtual void RequestActionUpdate() override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            actionContexts_[index]->RequestActionUpdate();
        else
            widget_->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PageSurfaceContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    void ExecuteAction(double value)
    {
        action_->Do(page_, surface_, isInverted_ == false ? value : 1.0 - value);
    }
    
public:
    PageSurfaceContext(Page* page, ControlSurface* surface, Widget* widget, Action* action) : ActionContext(page, surface, widget, action) {}
    
    virtual void DoAction() override
    {
        action_->Do(page_, surface_);
    }
    
    virtual void DoAction(MediaTrack* track) override
    {
        action_->Do(page_, surface_, track);
    }
    
    virtual void DoAction(MediaTrack* track, int fxIndex) override
    {
        action_->Do(page_, surface_, track, fxIndex);
    }
    
    virtual void DoAction(double value) override
    {
        ExecuteAction(value);
    }
};

#endif /* control_surface_action_contexts_h */
