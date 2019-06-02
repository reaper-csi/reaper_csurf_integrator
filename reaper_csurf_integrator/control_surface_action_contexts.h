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
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(this, GetWidget());
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
    TrackContext(Action* action) : ActionContext(action) {}
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->RequestUpdate(this, GetWidget(), track);
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
class TrackSlotCycleContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string customSlotName_ = "";
    
    vector<ActionContext*> actionContexts_;
    
    int GetCurrentIndex()
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            return GetWidget()->GetSurface()->GetPage()->GetTrackSlotIndex(customSlotName_, track);
        else
            return 0;
    }
    
public:
    TrackSlotCycleContext(Action* action, string customModifierName) : TrackContext(action), customSlotName_(customModifierName) {}
    
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
        
        if(actionContexts_.size() > 0 && GetCurrentIndex() < actionContexts_.size())
            actionContexts_[GetCurrentIndex()]->DoAction(value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendContext : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackSendContext(Action* action) : TrackContext(action) {}
    
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

                action_->RequestUpdate(this, GetWidget(), track, sendsOffset);
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
class TrackContextWithIntFeedbackParam : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
public:
    TrackContextWithIntFeedbackParam(Action* action, vector<string> params) : TrackContext(action)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->RequestUpdate(this, GetWidget(), track, param_);
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContextWithStringAndIntParams : public TrackContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string stringParam_ = "";
    int intParam_ = 0;
    
public:
    TrackContextWithStringAndIntParams(Action* action, vector<string> params) : TrackContext(action)
    {
        if(params.size() > 2)
        {
            stringParam_ = params[1];
            intParam_ = atol(params[2].c_str());
        }
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            action_->RequestUpdate(this, GetWidget(), track);
        else
            GetWidget()->Reset();
    }
    
    virtual void DoAction(double value) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            if(GetWidgetActionContextManager() != nullptr)
                action_->Do(GetWidget()->GetSurface()->GetPage(), GetWidget(), track, GetWidgetActionContextManager(), stringParam_, intParam_);
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
    FXContext(Action* action, vector<string> params) : TrackContext(action)
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
    ReaperActionContext(Action* action, vector<string> params) : ActionContext(action)
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
        action_->RequestUpdate(this, GetWidget(), commandId_);
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
    GlobalContextWithIntParam(Action* action, vector<string> params) : ActionContext(action)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(this, GetWidget(), param_);
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
    GlobalContextWithStringParam(Action* action, vector<string> params) : ActionContext(action)
    {
        if(params.size() > 1)
            param_ = params[1];
    }
    
    virtual void RequestUpdate() override
    {
        action_->RequestUpdate(this, GetWidget(), param_);
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), param_);
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
    GlobalContextWith2StringParams(Action* action, vector<string> params) : ActionContext(action)
    {
        if(params.size() > 2)
        {
            param1_ = params[1];
            param2_ = params[2];
        }
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface()->GetPage(), param1_, param2_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SurfaceContextWithStringParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string param_ = "";
    
public:
    SurfaceContextWithStringParam(Action* action, vector<string> params) : ActionContext(action)
    {
        if(params.size() > 1)
            param_ = params[1];
    }
    
    virtual void DoAction(double value) override
    {
        action_->Do(GetWidget()->GetSurface(), param_);
    }
    
    virtual void DoAction(MediaTrack* track) override
    {
        action_->Do(GetWidget()->GetSurface(), param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackPageSurfaceContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackPageSurfaceContext(Action* action) : ActionContext(action) {}
    
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
