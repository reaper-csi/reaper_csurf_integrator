//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_base_actions_h
#define control_surface_base_actions_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Interactor_DoubleAction : public Interactor_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    double currentValue_ = 999999.99;
    
    Interactor_DoubleAction(string name, CSurfManager* manager, Interactor* interactor) : Interactor_Action(name, manager, interactor)  {}
    
    Interactor_DoubleAction(string name, CSurfManager* manager, Interactor* interactor, double initialValue) : Interactor_Action(name, manager, interactor), currentValue_(initialValue)  {}

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LogicalCSurf_DoubleAction : public LogicalCSurf_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    double currentValue_ = 999999.99;
    
    LogicalCSurf_DoubleAction(string name, CSurfManager* manager, LogicalSurface* logicalSurface) : LogicalCSurf_Action(name, manager, logicalSurface)  {}

    LogicalCSurf_DoubleAction(string name, CSurfManager* manager, LogicalSurface* logicalSurface, double initialValue) : LogicalCSurf_Action(name, manager, logicalSurface), currentValue_(initialValue)  {}

    virtual double GetValue() { return 0.0; }
    
    virtual void SetWidgetValue(double value) override
    {
        GetManager()->SetWidgetValue("", GetName(), value);
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
class Interactor_CycleAction : public Interactor_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<Action*> actions_;
    int currentIndex_ = 0;
    
public:
    Interactor_CycleAction(string name, CSurfManager* manager, Interactor* interactor) : Interactor_Action(name, manager, interactor) {}
    
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
class Interactor_StringDisplayAction : public Interactor_Action
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
    Interactor_StringDisplayAction(string name, CSurfManager* manager, Interactor* interactor) : Interactor_Action(name, manager, interactor) {}
    
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
