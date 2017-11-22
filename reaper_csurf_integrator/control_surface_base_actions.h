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
    
    DoubleAction(string name, CSurfManager* manager, Interactor* interactor) : Action(name, manager, interactor)  {}
    
    DoubleAction(string name, CSurfManager* manager, Interactor* interactor, double initialValue) : Action(name, manager, interactor), currentValue_(initialValue)  {}

    virtual double GetValue() { return 0.0; }
    
    virtual void SetWidgetValue(double value) override
    {
        GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), value);
    }
    
public:
    virtual double GetCurrentNormalizedValue() override
    {
        return currentValue_;
    }
    
    virtual void Update() override
    {
        double newValue = GetValue();
        
        if(currentValue_ != newValue)
            SetWidgetValue(currentValue_ = newValue);
    }
    
    virtual void ForceUpdate() override
    {
        SetWidgetValue(currentValue_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CyclicAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<Action*> actions_;
    int currentIndex_ = 0;
    
public:
    CyclicAction(string name, CSurfManager* manager, Interactor* interactor) : Action(name, manager, interactor) {}
    
    virtual double GetCurrentNormalizedValue() override
    {
        return actions_[currentIndex_]->GetCurrentNormalizedValue();
    }
    
    virtual void AddAction(Action* action) override
    {
        actions_.push_back(action);
    }
   
    virtual void Update() override
    {
        actions_[currentIndex_]->Update();
    }
    
    virtual void ForceUpdate() override
    {
        actions_[currentIndex_]->ForceUpdate();
    }
    
    virtual void Cycle() override
    {
        currentIndex_ = currentIndex_ == actions_.size() - 1 ? 0 : ++currentIndex_;
        actions_[currentIndex_]->ForceUpdate();
    }
    
    virtual void RunAction(double adjustment) override
    {
        actions_[currentIndex_]->RunAction(adjustment);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StringDisplayAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string currentValue_ = "";
    
    virtual string GetValue() { return ""; }
    
    virtual void SetWidgetValue(string value) override
    {
        GetManager()->SetWidgetValue(GetInteractor()->GetGUID(), GetName(), value);
    }
    
public:
    StringDisplayAction(string name, CSurfManager* manager, Interactor* interactor) : Action(name, manager, interactor) {}
    
    virtual void Update() override
    {
        string newValue = GetValue();
        
        if(currentValue_ != newValue)
            SetWidgetValue(currentValue_ = newValue);
    }
    
    virtual void ForceUpdate() override
    {
        SetWidgetValue(currentValue_);
    }
};

#endif /* control_surface_base_actions_h */
