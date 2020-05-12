//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_integrator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleLearnMode  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleLearnMode(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        GetPage()->ToggleLearnMode();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TogglePin  : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TogglePin(Widget* widget, vector<string> params) : TrackAction(widget, params) {}
       
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetTrack())
            GetTrackNavigationManager()->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackSends  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackSends(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetSendsActivationManager()->GetShouldMapSends());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->ToggleMapSends(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFX  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackFX(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFX());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->ToggleMapSelectedFX(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFXMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackFXMenu(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFXMenus());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->ToggleMapFXMenu(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapFocusedFX  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapFocusedFX(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShouldMapFocusedFX());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->ToggleMapFocusedTrackFX(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoFXSlot  : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoFXSlot(Widget* widget, vector<string> params) : ActionWithIntParam(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        int fxIndex = param_ - 1 < 0 ? 0 : param_ - 1;
        
        GetPage()->MapSelectedTrackFXSlotToWidgets(GetSurface(), fxIndex);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackSendsToWidgets(Widget* widget, vector<string> params) : Action(widget, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->MapSelectedTrackSendsToWidgets(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToWidgets(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->MapSelectedTrackFXToWidgets(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToMenu(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->MapSelectedTrackFXToMenu(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapFocusedFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapFocusedFXToWidgets(Widget* widget, vector<string> params) : Action(widget, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->MapFocusedTrackFXToWidgets(GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectTrackRelative : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectTrackRelative(Widget* widget, vector<string> params) : ActionWithIntParam(widget, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            int trackIndex = 0;
            
            for(int i = 0; i <= GetTrackNavigationManager()->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(GetTrackNavigationManager()->GetTrackFromId(i), "I_SELECTED"))
                {
                    trackIndex = i;
                    break;
                }
            
            trackIndex += param_;
            
            if(trackIndex < 0)
                trackIndex = 0;
            
            if(trackIndex > GetTrackNavigationManager()->GetNumTracks())
                trackIndex = GetTrackNavigationManager()->GetNumTracks();
            
            DAW::SetOnlyTrackSelected(GetTrackNavigationManager()->GetTrackFromId(trackIndex));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShowFXWindows());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->GetFXActivationManager()->ToggleShowFXWindows();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleScrollLink : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleScrollLink(Widget* widget, vector<string> params) : ActionWithIntParam(widget, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetTrackNavigationManager()->ToggleScrollLink(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ForceScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ForceScrollLink(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        GetTrackNavigationManager()->ForceScrollLink();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleVCAMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleVCAMode(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetTrackNavigationManager()->GetVCAMode());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        GetTrackNavigationManager()->ToggleVCAMode();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeDisplayModes : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeDisplayModes(Widget* widget, vector<string> params) : Action(widget, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->NextTimeDisplayMode();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoNextPage : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoNextPage(Widget* widget, vector<string> params) : Action(widget, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->NextPage();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoPage : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoPage(Widget* widget, vector<string> params) : ActionWithStringParam(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->GoToPage(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoZone(Widget* widget, vector<string> params) : ActionWithStringParam(widget, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->GoZone(GetSurface(), param_, sender);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ClearAllSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ClearAllSolo(Widget* widget, vector<string> params) : Action(widget, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::AnyTrackSolo(nullptr));
    }

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        DAW::SoloAllTracks(0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackBank(Widget* widget, vector<string> params) : ActionWithIntParam(widget, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustTrackBank(GetPage(), param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShift(Widget* widget, vector<string> params) : Action(widget, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetShift());
    }

    void Do(double value, Widget* sender) override
    {
        GetPage()->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetOption : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetOption(Widget* widget, vector<string> params) : Action(widget, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetOption());
    }

    void Do(double value, Widget* sender) override
    {
        GetPage()->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetControl : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetControl(Widget* widget, vector<string> params) : Action(widget, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetControl());
    }

    void Do(double value, Widget* sender) override
    {
        GetPage()->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetAlt : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetAlt(Widget* widget, vector<string> params) : Action(widget, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetAlt());
    }

    void Do(double value, Widget* sender) override
    {
        GetPage()->SetAlt(value);
    }
};

#endif /* control_surface_manager_actions_h */
