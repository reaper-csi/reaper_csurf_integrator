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
    Shift_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetShift(zoneName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Option_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Option_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetOption(zoneName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Control_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Control_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetControl(zoneName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Alt_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Alt_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetAlt(zoneName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latched_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    clock_t lastPressed_ = clock();
public:
    Latched_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void SetValue(string zoneName, string surfaceName, double value) {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(value != 0)
        {
            lastPressed_ = clock();
            SetValue(zoneName, surfaceName, value);
            GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
        }
        else
        {
            if(clock() - lastPressed_ >  CLOCKS_PER_SEC / 4)
            {
                SetValue(zoneName, surfaceName, value);
                GetLayout()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedZoom_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    LatchedZoom_Action(Layout* layout) : Latched_Action(layout)  {}
    
    virtual void SetValue(string zoneName, string surfaceName, double value) override
    {
        GetLayout()->SetZoom(zoneName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedScrub_Action : public Latched_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    LatchedScrub_Action(Layout* layout) : Latched_Action(layout)  {}
    
    virtual void SetValue(string zoneName, string surfaceName, double value) override
    {
        GetLayout()->SetScrub(zoneName, surfaceName, value);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual double GetValue (string zoneName, string surfaceName, string widgetName) override { return GetLayout()->IsShowFXWindows(zoneName, surfaceName); }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->SetShowFXWindows(zoneName, surfaceName, value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextMap_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    NextMap_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->GetManager()->NextLayout();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int stride_ = 0;

public:
    TrackBank_Action(Layout* layout, string paramStr) : Double_Action(layout)
    {
        stride_ =  atol(paramStr.c_str());
    }
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->AdjustTrackBank(zoneName, surfaceName, stride_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImmobilizeSelectedTracks_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ImmobilizeSelectedTracks_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
       GetLayout()->ImmobilizeSelectedTracks();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MobilizeSelectedTracks_Action : public Double_Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MobilizeSelectedTracks_Action(Layout* layout) : Double_Action(layout)  {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        GetLayout()->MobilizeSelectedTracks();
    }
};

#endif /* control_surface_manager_actions_h */
