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
    
    DoubleAction(string name, Interactor* interactor) : Action(name, interactor)  {}
    
    DoubleAction(string name, Interactor* interactor, double initialValue) : Action(name, interactor), currentValue_(initialValue)  {}

    virtual double GetValue() { return 0.0; }
    
    virtual void SetWidgetValue(string surfaceName, double value) override
    {
        GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
    }
    
public:
    virtual double GetCurrentNormalizedValue() override
    {
        return currentValue_;
    }
    
    virtual void Update(string surfaceName) override
    {
        double newValue = GetValue();
        
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string surfaceName) override
    {
        SetWidgetValue(surfaceName, GetValue());
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
    CycledAction(string name, Interactor* interactor) : Action(name, interactor) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return actions_[currentIndex_]->GetCurrentNormalizedValue();
    }
    
    virtual void Add(Action* action) override
    {
        actions_.push_back(action);
    }
   
    virtual void Update(string surfaceName) override
    {
        actions_[currentIndex_]->Update(surfaceName);
    }
    
    virtual void ForceUpdate(string surfaceName) override
    {
        actions_[currentIndex_]->ForceUpdate(surfaceName);
    }
    
    virtual void Cycle(string surfaceName) override
    {
        currentIndex_ = currentIndex_ == actions_.size() - 1 ? 0 : ++currentIndex_;
        actions_[currentIndex_]->Cycle(surfaceName);
    }
    
    virtual void Run(string surfaceName, double adjustment) override
    {
        actions_[currentIndex_]->Run(surfaceName, adjustment);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StringDisplayAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual string GetValue() { return currentValue_; }
    
    virtual void SetWidgetValue(string surfaceName, string value) override
    {
        GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
    }
    
public:
    StringDisplayAction(string name, Interactor* interactor) : Action(name, interactor) {}
    
    virtual void Update(string surfaceName) override
    {
        string newValue = GetValue();
        
        if(currentValue_ != newValue)
            SetWidgetValue(surfaceName, currentValue_ = newValue);
    }
    
    virtual void ForceUpdate(string surfaceName) override
    {
        SetWidgetValue(surfaceName, GetValue());
    }
};

#endif /* control_surface_base_actions_h */
