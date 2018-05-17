//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_base_actions_h
#define control_surface_base_actions_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GlobalContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GlobalContext(Action* action, bool isInverted) : ActionContext(action, isInverted) {}
    
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
public:
    TrackContext(Action* action, bool isInverted) : ActionContext(action, isInverted) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = widget->GetTrack())
        {
            action_->RequestUpdate(page, this, widget, track);
        }
        else
        {
            widget->SetValue(0.0);
            widget->SetValue("");
        }
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        if(MediaTrack* track = widget->GetTrack())
            action_->Do(page, widget, track, isInverted_ == false ? value : 1.0 - value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackContextWithIntParam : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int param_ = 0;
    
public:
    TrackContextWithIntParam(Action* action, int param, bool isInverted) : ActionContext(action, isInverted), param_(param) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        if(MediaTrack* track = widget->GetTrack())
        {
            action_->RequestUpdate(page, this, widget, track, param_);
        }
        else
        {
            widget->SetValue(0.0);
            widget->SetValue("");
        }
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    string fxParamName_ = "";
    int fxIndex_ = 0;
public:
    FXContext(Action* action, string fxParamName, bool isInverted) : ActionContext(action, isInverted), fxParamName_(fxParamName) {}
    
    void SetIndex(int index) { fxIndex_ = index; }
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperActionContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    
public:
    ReaperActionContext(Action* action, string commandStr, bool isInverted) : ActionContext(action, isInverted)
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
    GlobalContextWithIntParam(Action* action, int param, bool isInverted) : ActionContext(action, isInverted), param_(param) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, param_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleContext(Action* action, bool isInverted) : ActionContext(action, isInverted) {}
    
    virtual void RequestActionUpdate(Page* page, Widget* widget) override
    {
        
    }
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PageSurfaceTrackContext : public ActionContext
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PageSurfaceTrackContext(Action* action, bool isInverted) : ActionContext(action, isInverted) {}
    
    virtual void DoAction(Page* page, Widget* widget, double value) override
    {
        action_->Do(page, widget->GetSurface(), widget->GetTrack());
    }
};




/*
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 class Cycled_Action : public OldAction
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 {
 private:
 vector<OldAction*> actions_;
 int currentIndex_ = 0;
 
 public:
 Cycled_Action(Layer* layer) : OldAction(layer) {}
 
 ~Cycled_Action() {}
 
 virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return actions_[currentIndex_]->GetCurrentNormalizedValue(zoneName, surfaceName, widgetName); }
 
 virtual void AddAction(OldAction* action) override
 {
 actions_.push_back(action);
 }
 
 virtual void Update(string zoneName, string surfaceName, string widgetName) override
 {
 actions_[currentIndex_]->Update(zoneName, surfaceName, widgetName);
 }
 
 virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) override
 {
 actions_[currentIndex_]->ForceUpdate(zoneName, surfaceName, widgetName);
 }
 
 virtual void Cycle(string zoneName, string surfaceName, string widgetName) override
 {
 currentIndex_ = currentIndex_ == actions_.size() - 1 ? 0 : ++currentIndex_;
 actions_[currentIndex_]->Cycle(zoneName, surfaceName, widgetName);
 }
 
 virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
 {
 actions_[currentIndex_]->Do(value, zoneName, surfaceName, widgetName);
 }
 };
 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Double_Action : public OldAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    double currentValue_ = 999999.99;
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) { return 0.0; }

    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }

    Double_Action(Layer* layer) : OldAction(layer)  {}
    
public:
    ~Double_Action() {}
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return currentValue_; }
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        double newValue = GetValue(zoneName, surfaceName, widgetName);
        
        if(currentValue_ != newValue)
            SetWidgetValue(zoneName, surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) override
    {
        SetWidgetValue(zoneName, surfaceName, widgetName, GetValue(zoneName, surfaceName, widgetName));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class String_Action : public OldAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) { return currentValue_; }
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, string value) override
    {
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }
    
    String_Action(Layer* layer) : OldAction(layer) {}

public:
    
    ~String_Action() {}
    
    virtual void Update(string zoneName, string surfaceName, string widgetName) override
    {
        string newValue = GetValue(zoneName, surfaceName, widgetName);
        
        if(currentValue_ != newValue)
            SetWidgetValue(zoneName, surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string zoneName, string surfaceName, string widgetName) override
    {
        SetWidgetValue(zoneName, surfaceName, widgetName, GetValue(zoneName, surfaceName, widgetName));
    }
};
 
 */

#endif /* control_surface_base_actions_h */
