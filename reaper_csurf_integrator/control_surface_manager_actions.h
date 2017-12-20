//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_base_actions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Shift_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Shift_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetShift(surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Option_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Option_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetOption(surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Control_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Control_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetControl(surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Alt_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Alt_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetAlt(surfaceName, value);
    }
};
/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latched_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    clock_t lastPressed_ = clock();
public:
    Latched_Action(string name, Interactor* interactor) : DoubleAction(name, interactor)  {}
    
    virtual void SetValue(string surfaceName, double value) {}
    
    virtual void RunAction(string surfaceName, double value) override
    {
        if(value != 0)
        {
            lastPressed_ = clock();
            SetValue(surfaceName, value);
            GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
        }
        else
        {
            if(clock() - lastPressed_ >  CLOCKS_PER_SEC / 4)
            {
                SetValue(surfaceName, value);
                GetInteractor()->SetWidgetValue(surfaceName, GetName(), value);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedZoom_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    LatchedZoom_Action(string name, Interactor* interactor) : Latched_Action(name, interactor)  {}
    
    virtual void SetValue(string surfaceName, double value) override
    {
        GetInteractor()->GetLogicalSurface()->SetZoom(surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedScrub_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    LatchedScrub_Action(string name, Interactor* interactor) : Latched_Action(name, interactor)  {}
    
    virtual void SetValue(string surfaceName, double value) override
    {
        GetInteractor()->GetLogicalSurface()->SetScrub(surfaceName, value);
    }
};
*/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual double GetValue (string surfaceName, string widgetName) override
    {
        return GetLogicalSurface()->IsShowFXWindows(surfaceName);
    }
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetShowOpenFXWindows(surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMap_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NextMap_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->GetManager()->NextLogicalSurface();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int stride_ = 0;

public:
    TrackBank_Action(LogicalSurface* logicalSurface, int stride) : DoubleAction(logicalSurface), stride_(stride)   {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->AdjustTrackBank(stride_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImmobilizeSelectedTracks_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ImmobilizeSelectedTracks_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
       GetLogicalSurface()->ImmobilizeSelectedTracks();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MobilizeSelectedTracks_Action : public DoubleAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MobilizeSelectedTracks_Action(LogicalSurface* logicalSurface) : DoubleAction(logicalSurface)  {}
    
    virtual void Run(double value, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->MobilizeSelectedTracks();
    }
};

#endif /* control_surface_manager_actions_h */
