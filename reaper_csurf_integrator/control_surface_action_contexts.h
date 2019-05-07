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
    GlobalContext(Widget* widget, Action* action) : ActionContext(widget, action) {}
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget());
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackContext(Widget* widget, Action* action) : ActionContext(widget, action) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track);
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendContext(Widget* widget, Action* action) : TrackContext(widget, action) {}
    
    // GAW TDB -- move some of this to SendsNavigationManager
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset < 0)
               GetWidget()->Reset();
            else
            {
                int sendsOffset = GetWidget()->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;

                action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track, sendsOffset);
            }
        }
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = GetWidget()->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, sendsOffset, isInverted_ == false ? value : 1.0 - value);
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
    TrackContextWithIntParam(Widget* widget, Action* action, int param) : TrackContext(widget, action), param_(param) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track, param_);
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget()->GetSurface(), track, param_);
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
    FXContext(Widget* widget, Action* action, string fxParamName) : TrackContext(widget, action), fxParamName_(fxParamName) {}
    
    virtual void SetAlias(string alias) override { fxParamNameAlias_ = alias; }
    
    virtual string GetAlias() override { return fxParamNameAlias_; }
    
    virtual void SetIndex(int index) override { fxIndex_ = index; }
        
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->RequestUpdate(this, GetWidget(), track, fxIndex_, GetWidget()->GetSurface()->GetPage()->GetFXParamIndex(track, GetWidget(), fxIndex_, fxParamName_));
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            if(shouldToggle_)
                action_->DoToggle(track, fxIndex_, GetWidget()->GetSurface()->GetPage()->GetFXParamIndex(track, GetWidget(), fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(track, fxIndex_, GetWidget()->GetSurface()->GetPage()->GetFXParamIndex(track, GetWidget(), fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
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
    ReaperActionContext(Widget* widget, Action* action, string commandStr) : ActionContext(widget, action)
    {
        commandId_ =  atol(commandStr.c_str());
        
        if(commandId_ == 0) // unsuccessful conversion to number
        {
            commandId_ = DAW::NamedCommandLookup(commandStr.c_str()); // look up by string
        
            if(commandId_ == 0) // can't find it
                commandId_ = 65535; // no-op
        }
    }
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), commandId_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithIntParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
   
public:
    GlobalContextWithIntParam(Widget* widget, Action* action, int param) : ActionContext(widget, action), param_(param) {}
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), param_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContextWithStringParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string param_ = "";
    
public:
    GlobalContextWithStringParam(Widget* widget, Action* action, string param) : ActionContext(widget, action), param_(param) {}
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), param_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackTouchControlledContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    Action* touchAction_ = nullptr;

public:
    TrackTouchControlledContext(Widget* widget, Action* action, Action* touchAction) : TrackContext(widget, action), touchAction_(touchAction) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            if(GetWidget()->GetSurface()->GetPage()->GetTouchState(track, 0))
                touchAction_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track);
            else
                action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track);
        }
        else
            GetWidget()->Reset();
    }
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            if(GetWidget()->GetSurface()->GetPage()->GetTouchState(track, 0))
                touchAction_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, isInverted_ == false ? value : 1.0 - value);
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
    TrackSendTouchControlledContext(Widget* widget, Action* action, Action* touchAction) : TrackContext(widget, action), touchAction_(touchAction) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset < 0)
                GetWidget()->Reset();
            else
            {
                int sendsOffset = GetWidget()->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(GetWidget()->GetSurface()->GetPage()->GetTouchState(track, 0))
                    touchAction_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track, sendsOffset);
                else
                    action_->RequestUpdate(GetWidget()->GetSurface()->GetPage(), this, GetWidget(), track, sendsOffset);
            }
       }
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = GetWidget()->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(GetWidget()->GetSurface()->GetPage()->GetTouchState(track, 0))
                    touchAction_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, isInverted_ == false ? value : 1.0 - value);
                else
                    action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, isInverted_ == false ? value : 1.0 - value);
            }
        }
    }
};
/*
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
                actionContexts_.push_back(context);
            }
        }
    }
    
    virtual void SetCyclerWidget(Widget* cyclerWidget) override { cyclerWidget_ = cyclerWidget; }
    
    virtual void RequestUpdate(Widget* widget) override
    {
        if(MediaTrack* track = widget->GetTrack())
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
            if(MediaTrack* track = widget->GetTrack())
                actionContexts_[index]->DoAction(widget, value);
        }
    }
};
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PageSurfaceContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PageSurfaceContext(Widget* widget, Action* action) : ActionContext(widget, action) {}
    
    virtual void DoAction() override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget()->GetSurface());
    }
    
    virtual void DoAction(MediaTrack* track) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget()->GetSurface(), track);
    }
    
    virtual void DoAction(MediaTrack* track, int fxIndex) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget()->GetSurface(), track, fxIndex);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget()->GetSurface(), isInverted_ == false ? value : 1.0 - value);
    }
};

#endif /* control_surface_action_contexts_h */
