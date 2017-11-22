//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_base_actions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Flip_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Flip_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->ToggleFlipped(GetName());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Shift_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Shift_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Option_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Option_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Control_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Control_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Alt_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Alt_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->SetAlt(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latched_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    clock_t lastPressed_ = clock();
public:
    
    Latched_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void SetValue(double value) {}
    
    virtual void RunAction(double value) override
    {
        if(value != 0)
        {
            lastPressed_ = clock();
            SetValue(value);
            GetManager()->GetCurrentLogicalSurface()->SetWidgetValue("", GetName(), value);
        }
        else
        {
            if(clock() - lastPressed_ >  CLOCKS_PER_SEC / 4)
            {
                SetValue(value);
                GetManager()->GetCurrentLogicalSurface()->SetWidgetValue("", GetName(), value);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedZoom_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    LatchedZoom_Action(string name, CSurfManager* manager, Interactor* interactor) : Latched_Action(name, manager, interactor)  {}
    
    virtual void SetValue(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->SetZoom(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedScrub_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    LatchedScrub_Action(string name, CSurfManager* manager, Interactor* interactor) : Latched_Action(name, manager, interactor)  {}
    
    virtual void SetValue(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->SetScrub(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMap_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    NextMap_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->NextLogicalSurface();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int stride_ = 0;

public:
    
    TrackBank_Action(string name, CSurfManager* manager, Interactor* interactor, int stride) : DoubleAction(name, manager, interactor), stride_(stride)   {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->AdjustTrackBank(stride_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImmobilizeSelectedTracks_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    ImmobilizeSelectedTracks_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->ImmobilizeSelectedTracks();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MobilizeSelectedTracks_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    MobilizeSelectedTracks_Action(string name, CSurfManager* manager, Interactor* interactor) : DoubleAction(name, manager, interactor)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->GetCurrentLogicalSurface()->MobilizeSelectedTracks();
    }
};

#endif /* control_surface_manager_actions_h */
