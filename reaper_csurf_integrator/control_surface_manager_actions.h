//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_integrator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TogglePin  : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TogglePin(Widget* widget, Zone* zone, vector<string> params) : TrackAction(widget, zone, params) {}

    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = zone_->GetNavigator()->GetTrack())
            GetTrackNavigationManager()->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleLearnMode  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleLearnMode(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        GetPage()->ToggleEditMode();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackSends  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackSends(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetShouldMapSends());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->ToggleMapSends();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFX  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackFX(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFX());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->GetFXActivationManager()->ToggleMapSelectedTrackFX();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFXMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackFXMenu(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFXMenus());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->GetFXActivationManager()->ToggleMapSelectedTrackFXMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapFocusedFX  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapFocusedFX(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShouldMapFocusedFX());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->GetFXActivationManager()->ToggleMapFocusedFX();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoFXSlot  : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoFXSlot(Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        int fxSlot = intParam_ - 1 < 0 ? 0 : intParam_ - 1;
        
        if(MediaTrack* selectedTrack = GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack())
            GetSurface()->GetFXActivationManager()->MapSelectedTrackFXSlotToWidgets(selectedTrack, fxSlot);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackSendsToWidgets(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        // GAW TBD
        //GetSurface()->GetSendsActivationManager()->MapSelectedTrackSendsToWidgets(slotIndex_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToWidgets(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->GetFXActivationManager()->MapSelectedTrackFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToMenu(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetSurface()->GetFXActivationManager()->MapSelectedTrackFXToMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapFocusedFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapFocusedFXToWidgets(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

       GetSurface()->GetFXActivationManager()->MapFocusedFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectTrackRelative : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectTrackRelative(Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(widget, zone, params) {}

    void Do(double value) override
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
            
            trackIndex += intParam_;
            
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
    SetShowFXWindows(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShowFXWindows());
    }
    
    void Do(double value) override
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
    ToggleScrollLink(Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetTrackNavigationManager()->ToggleScrollLink(intParam_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ForceScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ForceScrollLink(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void Do(double value) override
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
    ToggleVCAMode(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(GetTrackNavigationManager()->GetVCAMode());
    }
    
    void Do(double value) override
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
    CycleTimeDisplayModes(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
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
    GoNextPage(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
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
    GoPage(Widget* widget, Zone* zone, vector<string> params) : ActionWithStringParam(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->GoToPage(stringParam_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    string zonetype_ = "";
    
public:
    GoZone(Widget* widget, Zone* zone, vector<string> params) : ActionWithStringParam(widget, zone, params)
    {
        if(params.size() > 1)
            zonetype_ = params[1];
    }

    void Do(double value) override
    {
        if(value == 0.0)
            return; // ignore button releases

        GetSurface()->GoZone(stringParam_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ClearAllSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ClearAllSolo(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}
    
    void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::AnyTrackSolo(nullptr));
    }

    void Do(double value) override
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
    TrackBank(Widget* widget, Zone* zone, vector<string> params) : ActionWithIntParam(widget, zone, params) {}
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustTrackBank(GetPage(), intParam_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShift(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetShift());
    }

    void Do(double value) override
    {
        GetPage()->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetOption : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetOption(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetOption());
    }

    void Do(double value) override
    {
        GetPage()->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetControl : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetControl(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetControl());
    }

    void Do(double value) override
    {
        GetPage()->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetAlt : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetAlt(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetPage()->GetAlt());
    }

    void Do(double value) override
    {
        GetPage()->SetAlt(value);
    }
};

#endif /* control_surface_manager_actions_h */
