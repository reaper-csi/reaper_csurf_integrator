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
    void ExecuteAction(Widget* widget, double value)
    {
        action_->Do(page_, isInverted_ == false ? value : 1.0 - value);
    }
    
public:
    GlobalContext(Page* page, ControlSurface* surface, Action* action) : ActionContext(page, surface, action) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        action_->RequestUpdate(page_, this, widget);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        ExecuteAction(widget, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string trackGUID_ = "";
    
public:
    TrackContext(Page* page, ControlSurface* surface, Action* action) : ActionContext(page, surface, action) {}
    
    virtual void  SetTrack(string trackGUID) override
    {
        trackGUID_ = trackGUID;
    }
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->RequestUpdate(page_, this, widget, track);
        else
            widget->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->Do(page_, widget, track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendContext(Page* page, ControlSurface* surface, Action* action) : TrackContext(page, surface, action) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset < 0)
               widget->Reset();
            else
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;

                action_->RequestUpdate(page_, this, widget, track, sendsOffset);
            }
        }
        else
            widget->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->Do(page_, widget, track, sendsOffset, isInverted_ == false ? value : 1.0 - value);
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
    TrackContextWithIntParam(Page* page, ControlSurface* surface, Action* action, int param) : TrackContext(page, surface, action), param_(param) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->RequestUpdate(page_, this, widget, track, param_);
        else
            widget->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            action_->Do(page_, surface_, track, param_);
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
    FXContext(Page* page, ControlSurface* surface, Action* action, string fxParamName) : TrackContext(page, surface, action), fxParamName_(fxParamName) {}
    
    virtual void SetAlias(string alias) override { fxParamNameAlias_ = alias; }
    
    virtual string GetAlias() override { return fxParamNameAlias_; }
    
    virtual void SetIndex(int index) override { fxIndex_ = index; }
        
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()) )
            action_->RequestUpdate(this, widget, track, fxIndex_, page_->GetFXParamIndex(track, widget, fxIndex_, fxParamName_));
        else
            widget->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            if(shouldToggle_)
                action_->DoToggle(track, fxIndex_, page_->GetFXParamIndex(track, widget, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(track, fxIndex_, page_->GetFXParamIndex(track, widget, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
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
    ReaperActionContext(Page* page, ControlSurface* surface, Action* action, string commandStr) : ActionContext(page, surface, action)
    {
        commandId_ =  atol(commandStr.c_str());
        
        if(commandId_ == 0) // unsuccessful conversion to number
        {
        
        commandId_ = DAW::NamedCommandLookup(commandStr.c_str()); // look up by string
        
        if(commandId_ == 0) // can't find it
            commandId_ = 65535; // no-op
        }
    }
    
    virtual void RequestUpdate(Widget* widget) override
    {
        action_->RequestUpdate(page_, this, widget, commandId_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(page_, commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithIntParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
   
public:
    GlobalContextWithIntParam(Page* page, ControlSurface* surface, Action* action, int param) : ActionContext(page, surface, action), param_(param) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        action_->RequestUpdate(page_, this, widget, param_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
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
    GlobalContextWithStringParam(Page* page, ControlSurface* surface, Action* action, string param) : ActionContext(page, surface, action), param_(param) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        action_->RequestUpdate(page_, this, widget, param_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(page_, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlledContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* touchAction_ = nullptr;

public:
    TrackTouchControlledContext(Page* page, ControlSurface* surface, Action* action, Action* touchAction) : TrackContext(page, surface, action), touchAction_(touchAction) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            if(page_->GetTouchState(track, 0))
                touchAction_->RequestUpdate(page_, this, widget, track);
            else
                action_->RequestUpdate(page_, this, widget, track);
        }
        else
            widget->Reset();
    }
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            if(page_->GetTouchState(track, 0))
                touchAction_->Do(page_, widget, track, isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(page_, widget, track, isInverted_ == false ? value : 1.0 - value);
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
    TrackSendTouchControlledContext(Page* page, ControlSurface* surface, Action* action, Action* touchAction) : TrackContext(page, surface, action), touchAction_(touchAction) {}
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset < 0)
                widget->Reset();
            else
            {
                int sendsOffset = page_->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(page_->GetTouchState(track, 0))
                    touchAction_->RequestUpdate(page_, this, widget, track, sendsOffset);
                else
                    action_->RequestUpdate(page_, this, widget, track, sendsOffset);
            }
       }
        else
            widget->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
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
                    touchAction_->Do(page_, widget, track, isInverted_ == false ? value : 1.0 - value);
                else
                    action_->Do(page_, widget, track, isInverted_ == false ? value : 1.0 - value);
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
    TrackCycleContext(Page* page, ControlSurface* surface, Action* action, vector<string> params) : TrackContext(page, surface, action)
    {
        for(int i = 2; i < params.size(); i++)
        {
            istringstream iss(params[i]);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);

            if(ActionContext* context = TheManager->GetActionContext(page_, surface_, tokens))
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
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
            actionContexts_[index]->RequestUpdate(widget);
        else
            widget->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(widget && widget == cyclerWidget_)
        {
            if(value)
                index = index < actionContexts_.size() - 1 ? index + 1 : 0;
        }
        else if(actionContexts_[index])
        {
            if(MediaTrack* track = DAW::GetTrackFromGUID(trackGUID_, page_->GetFollowMCP()))
                actionContexts_[index]->DoAction(widget, value);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PageSurfaceContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PageSurfaceContext(Page* page, ControlSurface* surface, Action* action) : ActionContext(page, surface, action) {}
    
    virtual void DoAction(Widget* widget) override
    {
        action_->Do(page_, surface_);
    }
    
    virtual void DoAction(Widget* widget, MediaTrack* track) override
    {
        action_->Do(page_, surface_, track);
    }
    
    virtual void DoAction(Widget* widget, MediaTrack* track, int fxIndex) override
    {
        action_->Do(page_, surface_, track, fxIndex);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(page_, surface_, isInverted_ == false ? value : 1.0 - value);
    }
};

#endif /* control_surface_action_contexts_h */
