//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_base_actions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Shift_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Shift_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetShift(groupName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Option_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Option_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetOption(groupName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Control_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Control_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetControl(groupName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Alt_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Alt_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetAlt(groupName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latched_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    clock_t lastPressed_ = clock();
public:
    Latched_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void SetValue(string groupName, string surfaceName, double value) {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        if(value != 0)
        {
            lastPressed_ = clock();
            SetValue(groupName, surfaceName, value);
            GetLogicalSurface()->SetWidgetValue(groupName, surfaceName, widgetName, value);
        }
        else
        {
            if(clock() - lastPressed_ >  CLOCKS_PER_SEC / 4)
            {
                SetValue(groupName, surfaceName, value);
                GetLogicalSurface()->SetWidgetValue( groupName, surfaceName, widgetName, value);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedZoom_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    LatchedZoom_Action(LogicalSurface* logicalSurface) : Latched_Action(logicalSurface)  {}
    
    virtual void SetValue(string groupName, string surfaceName, double value) override
    {
        GetLogicalSurface()->SetZoom(groupName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedScrub_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    LatchedScrub_Action(LogicalSurface* logicalSurface) : Latched_Action(logicalSurface)  {}
    
    virtual void SetValue(string groupName, string surfaceName, double value) override
    {
        GetLogicalSurface()->SetScrub(groupName, surfaceName, value);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual double GetValue (string groupName, string surfaceName, string widgetName) override { return GetLogicalSurface()->IsShowFXWindows(groupName, surfaceName); }
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->SetShowFXWindows(groupName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMap_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NextMap_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->GetManager()->NextLogicalSurface();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int stride_ = 0;

public:
    TrackBank_Action(LogicalSurface* logicalSurface, int stride) : Double_Action(logicalSurface), stride_(stride)   {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->AdjustTrackBank(groupName, surfaceName, stride_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImmobilizeSelectedTracks_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ImmobilizeSelectedTracks_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
       GetLogicalSurface()->ImmobilizeSelectedTracks();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MobilizeSelectedTracks_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MobilizeSelectedTracks_Action(LogicalSurface* logicalSurface) : Double_Action(logicalSurface)  {}
    
    virtual void Do(double value, string groupName, string surfaceName, string widgetName) override
    {
        GetLogicalSurface()->MobilizeSelectedTracks();
    }
};

#endif /* control_surface_manager_actions_h */
