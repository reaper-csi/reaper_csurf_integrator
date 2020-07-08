//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_integrator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TogglePin  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
            context->GetTrackNavigationManager()->TogglePin(track);
    }


    
    TogglePin() {}
    TogglePin(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = GetZone()->GetNavigator()->GetTrack())
            GetTrackNavigationManager()->TogglePin(track);
    }
    
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleLearnMode  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetPage()->ToggleEditMode();
    }

    
    
    ToggleLearnMode() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetShouldMapSends());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->ToggleMapSends();
    }

    
    
    ToggleMapSelectedTrackSends() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFX());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleMapSelectedTrackFX();
    }

    
    
    
    
    ToggleMapSelectedTrackFX() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFXMenus());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleMapSelectedTrackFXMenu();
    }

    
    
    
    
    ToggleMapSelectedTrackFXMenu() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShouldMapFocusedFX());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleMapFocusedFX();
    }

    
    
    
    
    ToggleMapFocusedFX() {}
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
class GoFXSlot  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        int fxSlot = context->GetIntParam() - 1 < 0 ? 0 : context->GetIntParam() - 1;
        
        if(MediaTrack* selectedTrack = context->GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack())
            context->GetSurface()->GetFXActivationManager()->MapSelectedTrackFXSlotToWidgets(selectedTrack, fxSlot);
    }

    
    
    
    
    GoFXSlot() {}
    GoFXSlot(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        int fxSlot = GetIntParam() - 1 < 0 ? 0 : GetIntParam() - 1;
        
        if(MediaTrack* selectedTrack = GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack())
            GetSurface()->GetFXActivationManager()->MapSelectedTrackFXSlotToWidgets(selectedTrack, fxSlot);
    }
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        // GAW TBD
        //context->GetSurface()->GetSendsActivationManager()->MapSelectedTrackSendsToWidgets(slotIndex_);
    }
    
    
    
    
    MapSelectedTrackSendsToWidgets() {}
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
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->MapSelectedTrackFXToWidgets();
    }

    
    
    
    
    MapSelectedTrackFXToWidgets() {}
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
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->MapSelectedTrackFXToMenu();
    }

    
    
    
    
    MapSelectedTrackFXToMenu() {}
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
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->MapFocusedFXToWidgets();
    }

    
    
    
    
    MapFocusedFXToWidgets() {}
    MapFocusedFXToWidgets(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

       GetSurface()->GetFXActivationManager()->MapFocusedFXToWidgets();
    }
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectTrackRelative : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            int trackIndex = 0;
            
            for(int i = 0; i <= context->GetTrackNavigationManager()->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(context->GetTrackNavigationManager()->GetTrackFromId(i), "I_SELECTED"))
                {
                    trackIndex = i;
                    break;
                }
            
            trackIndex += context->GetIntParam();
            
            if(trackIndex < 0)
                trackIndex = 0;
            
            if(trackIndex > context->GetTrackNavigationManager()->GetNumTracks())
                trackIndex = context->GetTrackNavigationManager()->GetNumTracks();
            
            DAW::SetOnlyTrackSelected(context->GetTrackNavigationManager()->GetTrackFromId(trackIndex));
        }
    }

    
    
    
    SelectTrackRelative() {}
    SelectTrackRelative(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

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
            
            trackIndex += GetIntParam();
            
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetSurface()->GetFXActivationManager()->GetShowFXWindows());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleShowFXWindows();
    }

    
    
    
    
    SetShowFXWindows() {}
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
class ToggleScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetTrackNavigationManager()->ToggleScrollLink(GetIntParam());
    }

    
    
    
    
    ToggleScrollLink() {}
    ToggleScrollLink(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void RequestUpdate() override
    {
        UpdateWidgetValue(GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        GetTrackNavigationManager()->ToggleScrollLink(GetIntParam());
    }
    
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ForceScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetTrackNavigationManager()->ForceScrollLink();
    }

    
    
    
    
    ForceScrollLink() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetTrackNavigationManager()->GetVCAMode());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetTrackNavigationManager()->ToggleVCAMode();
    }

    
    
    
    
    ToggleVCAMode() {}
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
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->NextTimeDisplayMode();
    }

    
    
    
    CycleTimeDisplayModes() {}
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
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->NextPage();
    }

    
    
    
    
    GoNextPage() {}
    GoNextPage(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->NextPage();
    }
    
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoPage : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->GoToPage(GetStringParam());
    }

    
    
    
    GoPage() {}
    GoPage(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->GoToPage(GetStringParam());
    }
    
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0)
            return; // ignore button releases
        
        context->GetSurface()->GoZone(GetStringParam());
    }

    
    
    
    GoZone() {}
    GoZone(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0)
            return; // ignore button releases

        GetSurface()->GoZone(GetStringParam());
    }
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ClearAllSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(DAW::AnyTrackSolo(nullptr));
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        DAW::SoloAllTracks(0);
    }

    
    
    
    
    
    ClearAllSolo() {}
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
class TrackBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustTrackBank(context->GetPage(), context->GetIntParam());
    }

    
    
    
    
    TrackBank() {}
    TrackBank(Widget* widget, Zone* zone, vector<string> params) : Action(widget, zone, params) {}

    void Do(double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustTrackBank(GetPage(), GetIntParam());
    }
    
    
    
    
    
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetShift());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetShift(value);
    }

    
    
    
    
    SetShift() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetOption());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetOption(value);
    }

    
    
    
    
    
    SetOption() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetControl());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetControl(value);
    }

    
    
    
    
    SetControl() {}
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
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetAlt());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetAlt(value);
    }

    
    
    
    
    SetAlt() {}
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
