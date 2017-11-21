//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_base_actions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Flip_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Flip_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->ToggleFlipped(GetName());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Shift_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Shift_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Option_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Option_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Control_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Control_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Alt_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    Alt_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->SetAlt(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latched_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    clock_t lastPressed_ = clock();
public:
    
    Latched_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void SetValue(double value) {}
    
    virtual void RunAction(double value) override
    {
        if(value != 0)
        {
            lastPressed_ = clock();
            SetValue(value);
            GetSurface()->SetWidgetValue("", GetName(), value);
        }
        else
        {
            if(clock() - lastPressed_ >  CLOCKS_PER_SEC / 4)
            {
                SetValue(value);
                GetSurface()->SetWidgetValue("", GetName(), value);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedZoom_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    LatchedZoom_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : Latched_Action(name, manager, logicalCSurf)  {}
    
    virtual void SetValue(double value) override
    {
        GetSurface()->SetZoom(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedScrub_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    LatchedScrub_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : Latched_Action(name, manager, logicalCSurf)  {}
    
    virtual void SetValue(double value) override
    {
        GetSurface()->SetScrub(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMap_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    NextMap_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetManager()->NextLogicalSurface();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int stride_ = 0;

public:
    
    TrackBank_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf, int stride) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf), stride_(stride)   {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->AdjustTrackBank(stride_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImmobilizeSelectedTracks_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    ImmobilizeSelectedTracks_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->ImmobilizeSelectedTracks();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MobilizeSelectedTracks_Action : public LogicalCSurf_DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    
    MobilizeSelectedTracks_Action(string name, CSurfManager* manager, LogicalSurface* logicalCSurf) : LogicalCSurf_DoubleAction(name, manager, logicalCSurf)  {}
    
    virtual void RunAction(double value) override
    {
        GetSurface()->MobilizeSelectedTracks();
    }
};

#endif /* control_surface_manager_actions_h */
