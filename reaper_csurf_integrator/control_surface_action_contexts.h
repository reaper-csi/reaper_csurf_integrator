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
public:
    GlobalContext(Action* action) : ActionContext(action) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        action_->RequestUpdate(page, this, widget);
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string trackGUID_ = "";
    
public:
    TrackContext(Action* action) : ActionContext(action) {}
    
    virtual void  SetTrack(string trackGUID) override
    {
        trackGUID_ = trackGUID;
    }
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
            action_->RequestUpdate(page, this, widget, track);
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
            action_->Do(page, widget, track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendContext(Action* action) : TrackContext(action) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset < 0)
               widget->Reset();
            else
            {
                int sendsOffset = page->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->RequestUpdate(page, this, widget, track, sendsOffset);
            }
        }
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset > -1)
            {
                int sendsOffset = page->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->Do(page, widget, track, sendsOffset, isInverted_ == false ? value : 1.0 - value);
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContextWithIntParam : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
public:
    TrackContextWithIntParam(Action* action, int param) : TrackContext(action), param_(param) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
            action_->RequestUpdate(page, this, widget, track, param_);
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
            action_->Do(page, widget, track, value);
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
    
public:
    FXContext(Action* action, string fxParamName) : TrackContext(action), fxParamName_(fxParamName) {}
    
    virtual void SetAlias(string alias) override { fxParamNameAlias_ = alias; }
    
    virtual string GetAlias() override { return fxParamNameAlias_; }
    
    virtual void SetIndex(int index) override { fxIndex_ = index; }
    
    virtual void SetShouldtoggle() override { shouldToggle_ = true; }
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()) )
            action_->RequestUpdate(this, widget, track, fxIndex_, page->GetFXParamIndex(track, widget, fxIndex_, fxParamName_));
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            if(shouldToggle_)
                action_->DoToggle(track, fxIndex_, page->GetFXParamIndex(track, widget, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(track, fxIndex_, page->GetFXParamIndex(track, widget, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperActionContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    
public:
    ReaperActionContext(Action* action, string commandStr) : ActionContext(action)
    {
        commandId_ =  atol(commandStr.c_str());
        
        if(commandId_ == 0) // unsuccessful conversion to number
        {
        
        commandId_ = DAW::NamedCommandLookup(commandStr.c_str()); // look up by string
        
        if(commandId_ == 0) // can't find it
            commandId_ = 65535; // no-op
        }
    }
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        action_->RequestUpdate(page, this, widget, commandId_);
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithIntParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
public:
    GlobalContextWithIntParam(Action* action, int param) : ActionContext(action), param_(param) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        action_->RequestUpdate(page, this, widget, param_);
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlledContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* touchAction_ = nullptr;
public:
    TrackTouchControlledContext(Action* action, Action* touchAction) : TrackContext(action), touchAction_(touchAction) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            if(page->GetTouchState(track, 0))
                touchAction_->RequestUpdate(page, this, widget, track);
            else
                action_->RequestUpdate(page, this, widget, track);
        }
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            if(page->GetTouchState(track, 0))
                touchAction_->Do(page, widget, track, isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(page, widget, track, isInverted_ == false ? value : 1.0 - value);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendTouchControlledContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* touchAction_ = nullptr;
public:
    TrackSendTouchControlledContext(Action* action, Action* touchAction) : TrackContext(action), touchAction_(touchAction) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset < 0)
                widget->Reset();
            else
            {
                int sendsOffset = page->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(page->GetTouchState(track, 0))
                    touchAction_->RequestUpdate(page, this, widget, track, sendsOffset);
                else
                    action_->RequestUpdate(page, this, widget, track, sendsOffset);
            }
       }
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
        {
            
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = page->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(page->GetTouchState(track, 0))
                    touchAction_->Do(page, widget, track, isInverted_ == false ? value : 1.0 - value);
                else
                    action_->Do(page, widget, track, isInverted_ == false ? value : 1.0 - value);
            }
        }
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
    
public:
    TrackCycleContext(vector<string> params, Action* action) : TrackContext(action)
    {
        for(int i = 2; i < params.size(); i++)
        {
            istringstream iss(params[i]);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);

            if(ActionContext* context = TheManager->GetActionContext(tokens))
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
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
            actionContexts_[index]->RequestActionUpdate(page, widget);
        else
            widget->Reset();
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(widget && widget == cyclerWidget_)
        {
            if(value)
                index = index < actionContexts_.size() - 1 ? index + 1 : 0;
        }
        else if(actionContexts_[index])
        {
            if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page->GetFollowMCP()))
                actionContexts_[index]->DoAction(page, widget, value);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PageSurfaceContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PageSurfaceContext(Action* action) : ActionContext(action) {}
    
    virtual void DoAction(Page* page, RealSurface* surface) override
    {
        action_->Do(page, surface);
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, widget->GetSurface(), isInverted_ == false ? value : 1.0 - value);
    }
};

#endif /* control_surface_action_contexts_h */
