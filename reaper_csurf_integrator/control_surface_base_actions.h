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
    
    virtual double GetValue(string groupName, string surfaceName, string widgetName) { return 0.0; }

    virtual void SetWidgetValue(string groupName, string surfaceName, string widgetName, double value) override
    {
        GetLogicalSurface()->SetWidgetValue(groupName, surfaceName, widgetName, value);
    }

    Double_Action(LogicalSurface* logicalSurface) : Action(logicalSurface)  {}
    
public:
    ~Double_Action() {}
    
    virtual double GetCurrentNormalizedValue(string groupName, string surfaceName, string widgetName) override { return currentValue_; }
    
    virtual void Update(string groupName, string surfaceName, string widgetName) override
    {
        double newValue = GetValue(groupName, surfaceName, widgetName);
        
        if(currentValue_ != newValue)
            SetWidgetValue(groupName, surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string groupName, string surfaceName, string widgetName) override
    {
        SetWidgetValue(groupName, surfaceName, widgetName, GetValue(groupName, surfaceName, widgetName));
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
    Cycled_Action(LogicalSurface* logicalSurface) : Action(logicalSurface) {}
    
    ~Cycled_Action() {}
    
    virtual double GetCurrentNormalizedValue(string groupName, string surfaceName, string widgetName) override { return actions_[currentIndex_]->GetCurrentNormalizedValue(groupName, surfaceName, widgetName); }
    
    virtual void AddAction(Action* action) override
    {
        actions_.push_back(action);
    }
   
    virtual void Update(string groupName, string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->Update(groupName, surfaceName, widgetName);
    }
    
    virtual void ForceUpdate(string groupName, string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->ForceUpdate(groupName, surfaceName, widgetName);
    }
    
    virtual void Cycle(string groupName, string surfaceName, string widgetName) override
    {
        currentIndex_ = currentIndex_ == actions_.size() - 1 ? 0 : ++currentIndex_;
        actions_[currentIndex_]->Cycle(groupName, surfaceName, widgetName);
    }
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        actions_[currentIndex_]->Do(value, groupName, surfaceName, widgetName);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class String_Action : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual string GetValue(string groupName, string surfaceName, string widgetName) { return currentValue_; }
    
    virtual void SetWidgetValue(string groupName, string surfaceName, string widgetName, string value) override
    {
        GetLogicalSurface()->SetWidgetValue(groupName, surfaceName, widgetName, value);
    }
    
    String_Action(LogicalSurface* logicalSurface) : Action(logicalSurface) {}

public:
    
    ~String_Action() {}
    
    virtual void Update(string groupName, string surfaceName, string widgetName) override
    {
        string newValue = GetValue(groupName, surfaceName, widgetName);
        
        if(currentValue_ != newValue)
            SetWidgetValue(groupName, surfaceName, widgetName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string groupName, string surfaceName, string widgetName) override
    {
        SetWidgetValue(groupName, surfaceName, widgetName, GetValue(groupName, surfaceName, widgetName));
    }
};

#endif /* control_surface_base_actions_h */
