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

    virtual double GetValue() { return 0.0; }
    
    virtual void SetWidgetValue(string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
    }
    
public:
    virtual double GetCurrentNormalizedValue() override
    {
        return currentValue_;
    }
    
    virtual void Update(string surfaceName, string widgetName) override
    {
        double newValue = GetValue();
        
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string surfaceName, string widgetName) override
    {
        SetWidgetValue(surfaceName, widgetName, GetValue());
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
    
    virtual double GetCurrentNormalizedValue() override
    {
        return actions_[currentIndex_]->GetCurrentNormalizedValue();
    }
    
    virtual void Add(Action* action) override
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
    
    virtual void Run(double adjustment, string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->Run(adjustment, surfaceName, widgetName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StringDisplayAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual string GetValue() { return currentValue_; }
    
    virtual void SetWidgetValue(string surfaceName, string widgetName, string value) override
    {
        GetLogicalSurface()->SetWidgetValue(surfaceName, widgetName, value);
    }
    
public:
    StringDisplayAction(LogicalSurface* logicalSurface) : Action(logicalSurface) {}
    
    virtual void Update(string surfaceName, string widgetName) override
    {
        string newValue = GetValue();
        
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string surfaceName, string widgetName) override
    {
        SetWidgetValue(surfaceName, widgetName, GetValue());
    }
};

#endif /* control_surface_base_actions_h */
