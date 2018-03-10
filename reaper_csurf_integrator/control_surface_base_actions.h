//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_base_actions_h
#define control_surface_base_actions_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Double_Action : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    double currentValue_ = 999999.99;
    
    virtual double GetValue(string zoneName, string surfaceName, string widgetName) { return 0.0; }

    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, double value) override
    {
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }

    Double_Action(Layer* layer) : Action(layer)  {}
    
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
class Cycled_Action : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<Action*> actions_;
    int currentIndex_ = 0;
    
public:
    Cycled_Action(Layer* layer) : Action(layer) {}
    
    ~Cycled_Action() {}
    
    virtual double GetCurrentNormalizedValue(string zoneName, string surfaceName, string widgetName) override { return actions_[currentIndex_]->GetCurrentNormalizedValue(zoneName, surfaceName, widgetName); }
    
    virtual void AddAction(Action* action) override
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class String_Action : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual string GetValue(string zoneName, string surfaceName, string widgetName) { return currentValue_; }
    
    virtual void SetWidgetValue(string zoneName, string surfaceName, string widgetName, string value) override
    {
        GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
    }
    
    String_Action(Layer* layer) : Action(layer) {}

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

#endif /* control_surface_base_actions_h */
