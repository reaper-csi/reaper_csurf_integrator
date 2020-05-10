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
    ToggleLearnMode(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    TogglePin(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params) {}
       
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetTrack())
            GetPage()->GetTrackNavigationManager()->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackSends  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackSends(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    ToggleMapSelectedTrackFX(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    ToggleMapSelectedTrackFXMenu(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    ToggleMapFocusedFX(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    GoFXSlot(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(name, widget, zone, params) {}
    
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
    MapSelectedTrackSendsToWidgets(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    MapSelectedTrackFXToWidgets(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    MapSelectedTrackFXToMenu(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    MapFocusedFXToWidgets(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    SelectTrackRelative(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(name, widget, zone, params) {}

    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            int trackIndex = 0;
            
            for(int i = 0; i <= GetPage()->GetTrackNavigationManager()->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(GetPage()->GetTrackNavigationManager()->GetTrackFromId(i), "I_SELECTED"))
                {
                    trackIndex = i;
                    break;
                }
            
            trackIndex += param_;
            
            if(trackIndex < 0)
                trackIndex = 0;
            
            if(trackIndex > GetPage()->GetTrackNavigationManager()->GetNumTracks())
                trackIndex = GetPage()->GetTrackNavigationManager()->GetNumTracks();
            
            DAW::SetOnlyTrackSelected(GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackIndex));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    ToggleScrollLink(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(name, widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->GetTrackNavigationManager()->ToggleScrollLink(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ForceScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ForceScrollLink(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        GetPage()->GetTrackNavigationManager()->ForceScrollLink();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleVCAMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleVCAMode(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetTrackNavigationManager()->GetVCAMode());
    }
    
    void Do(double value, Widget* sender) override
    {
        if(value == 0.0) return; // ignore button releases
        
        GetPage()->GetTrackNavigationManager()->ToggleVCAMode();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeDisplayModes : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeDisplayModes(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    GoNextPage(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    GoPage(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithStringParam(name, widget, zone, params) {}
    
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
    GoZone(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithStringParam(name, widget, zone, params) {}

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
    ClearAllSolo(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}
    
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
    TrackBank(string name, Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(name, widget, zone, params) {}
    
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
    SetShift(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    SetOption(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    SetControl(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
    SetAlt(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) {}

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
