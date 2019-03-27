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
        action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(widget->GetSurface()->GetPage(), isInverted_ == false ? value : 1.0 - value);
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
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
            action_->Do(widget->GetSurface()->GetPage(), widget, track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendContext(Widget* widget, Action* action) : TrackContext(widget, action) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = widget_->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;

            if(maxOffset < 0)
               widget_->Reset();
            else
            {
                int sendsOffset = widget_->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;

                action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track, sendsOffset);
            }
        }
        else
            widget_->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset = widget->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                action_->Do(widget->GetSurface()->GetPage(), widget, track, sendsOffset, isInverted_ == false ? value : 1.0 - value);
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
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track, param_);
        else
            widget_->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
            action_->Do(widget->GetSurface()->GetPage(), widget->GetSurface(), track, param_);
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
        if(MediaTrack* track = widget_->GetTrack())
            action_->RequestUpdate(this, widget_, track, fxIndex_, widget_->GetSurface()->GetPage()->GetFXParamIndex(track, widget_, fxIndex_, fxParamName_));
        else
            widget_->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
        {
            if(shouldToggle_)
                action_->DoToggle(track, fxIndex_, widget->GetSurface()->GetPage()->GetFXParamIndex(track, widget, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(track, fxIndex_, widget->GetSurface()->GetPage()->GetFXParamIndex(track, widget, fxIndex_, fxParamName_), isInverted_ == false ? value : 1.0 - value);
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
        action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, commandId_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(widget->GetSurface()->GetPage(), commandId_);
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
        action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, param_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(widget->GetSurface()->GetPage(), param_);
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
        action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, param_);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(widget->GetSurface()->GetPage(), param_);
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
        if(MediaTrack* track = widget_->GetTrack())
        {
            if(widget_->GetSurface()->GetPage()->GetTouchState(track, 0))
                touchAction_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track);
            else
                action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track);
        }
        else
            widget_->Reset();
    }
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
        {
            if(widget->GetSurface()->GetPage()->GetTouchState(track, 0))
                touchAction_->Do(widget->GetSurface()->GetPage(), widget, track, isInverted_ == false ? value : 1.0 - value);
            else
                action_->Do(widget->GetSurface()->GetPage(), widget, track, isInverted_ == false ? value : 1.0 - value);
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
        if(MediaTrack* track = widget_->GetTrack())
        {
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset < 0)
                widget_->Reset();
            else
            {
                int sendsOffset = widget_->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(widget_->GetSurface()->GetPage()->GetTouchState(track, 0))
                    touchAction_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track, sendsOffset);
                else
                    action_->RequestUpdate(widget_->GetSurface()->GetPage(), this, widget_, track, sendsOffset);
            }
       }
        else
            widget_->Reset();
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
        {
            
            int maxOffset = DAW::GetTrackNumSends(track, 0) - 1;
            
            if(maxOffset > -1)
            {
                int sendsOffset =widget->GetSurface()->GetPage()->GetSendsOffset();
                
                if(sendsOffset > maxOffset)
                    sendsOffset = maxOffset;
                
                if(widget->GetSurface()->GetPage()->GetTouchState(track, 0))
                    touchAction_->Do(widget->GetSurface()->GetPage(), widget, track, isInverted_ == false ? value : 1.0 - value);
                else
                    action_->Do(widget->GetSurface()->GetPage(), widget, track, isInverted_ == false ? value : 1.0 - value);
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
    
    virtual void DoAction(Widget* widget) override
    {
        action_->Do(widget->GetSurface()->GetPage(), widget->GetSurface());
    }
    
    virtual void DoAction(Widget* widget, MediaTrack* track) override
    {
        action_->Do(widget->GetSurface()->GetPage(), widget->GetSurface(), track);
    }
    
    virtual void DoAction(Widget* widget, MediaTrack* track, int fxIndex) override
    {
        action_->Do(widget->GetSurface()->GetPage(), widget->GetSurface(), track, fxIndex);
    }
    
    virtual void DoAction(Widget* widget, double value) override
    {
        action_->Do(widget->GetSurface()->GetPage(), widget->GetSurface(), isInverted_ == false ? value : 1.0 - value);
    }
};

#endif /* control_surface_action_contexts_h */
