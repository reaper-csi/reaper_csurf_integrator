//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_base_actions_h
#define control_surface_base_actions_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DoubleAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    double currentValue_ = 999999.99;
    
    DoubleAction(LogicalSurface* logicalSurface) : Action(logicalSurface)  {}
    DoubleAction(LogicalSurface* logicalSurface, double initialValue) : Action(logicalSurface), currentValue_(initialValue)  {}

    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
    }
    
    virtual double GetValue(string surfaceName, string widgetName) { return 0.0; }
    
    virtual double GetCurrentNormalizedValue(string surfaceName, string widgetName) override { return currentValue_; }

public:
    virtual void Update(string surfaceName, string widgetName) override
    {
        double newValue = GetValue(surfaceName, widgetName);
        
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string surfaceName, string widgetName) override
    {
        SetWidgetValue(surfaceName, widgetName, GetValue(surfaceName, widgetName));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycledAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<Action*> actions_;
    int currentIndex_ = 0;
    
public:
    CycledAction(LogicalSurface* logicalSurface) : Action(logicalSurface) {}
    
    virtual double GetCurrentNormalizedValue(string surfaceName, string widgetName) override { return actions_[currentIndex_]->GetCurrentNormalizedValue(surfaceName, widgetName); }
    
    virtual void AddAction(Action* action) override
    {
        actions_.push_back(action);
    }
   
    virtual void Update(string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->Update(surfaceName, widgetName);
    }
    
    virtual void ForceUpdate(string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->ForceUpdate(surfaceName, widgetName);
    }
    
    virtual void Cycle(string surfaceName, string widgetName) override
    {
        currentIndex_ = currentIndex_ == actions_.size() - 1 ? 0 : ++currentIndex_;
        actions_[currentIndex_]->Cycle(surfaceName, widgetName);
    }
    
    virtual void Do(double value, string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->Do(value, surfaceName, widgetName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StringDisplayAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual void SetWidgetValue(string surfaceName, string widgetName, string value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
    }
    
    virtual string GetValue(string surfaceName, string widgetName) { return currentValue_; }
    
public:
    StringDisplayAction(LogicalSurface* logicalSurface) : Action(logicalSurface) {}
    
    virtual void Update(string surfaceName, string widgetName) override
    {
        string newValue = GetValue(surfaceName, widgetName);
        
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string surfaceName, string widgetName) override
    {
        SetWidgetValue(surfaceName, widgetName, GetValue(surfaceName, widgetName));
    }
};

#endif /* control_surface_base_actions_h */
