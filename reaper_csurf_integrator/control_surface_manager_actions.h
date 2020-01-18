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
    ToggleLearnMode(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
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
    TogglePin(string name, WidgetActionManager* manager, vector<string> params) : TrackAction(name, manager, params) {}
       
    void Do(double value, WidgetActionManager* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(MediaTrack* track = GetWidget()->GetTrack())
            GetPage()->GetTrackNavigationManager()->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackSends  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackSends(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetSurface()->GetShouldMapSends());
    }
    
    void Do(double value, WidgetActionManager* sender) override
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
    ToggleMapSelectedTrackFX(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFX());
    }
    
    void Do(double value, WidgetActionManager* sender) override
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
    ToggleMapSelectedTrackFXMenu(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFXMenus());
    }
    
    void Do(double value, WidgetActionManager* sender) override
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
    ToggleMapFocusedFX(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetSurface()->GetFXActivationManager()->GetShouldMapFocusedFX());
    }
    
    void Do(double value, WidgetActionManager* sender) override
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
    GoFXSlot(string name, WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
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
    MapSelectedTrackSendsToWidgets(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
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
    MapSelectedTrackFXToWidgets(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
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
    MapSelectedTrackFXToMenu(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
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
    MapFocusedFXToWidgets(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
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
    SelectTrackRelative(string name, WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            int trackIndex = 0;
            
            for(int i = 0; i < GetPage()->GetTrackNavigationManager()->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(GetPage()->GetTrackNavigationManager()->GetTrackFromId(i), "I_SELECTED"))
                {
                    trackIndex = i;
                    break;
                }
            
            trackIndex += param_;
            
            if(trackIndex < 0)
                trackIndex = 0;
            
            if(trackIndex > GetPage()->GetTrackNavigationManager()->GetNumTracks() - 1)
                trackIndex = GetPage()->GetTrackNavigationManager()->GetNumTracks() - 1;
            
            DAW::SetOnlyTrackSelected(GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackIndex));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetSurface()->GetFXActivationManager()->GetShowFXWindows());
    }
    
    void Do(double value, WidgetActionManager* sender) override
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
    ToggleScrollLink(string name, WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(name, manager, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetPage()->GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->GetTrackNavigationManager()->ToggleScrollLink(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeDisplayModes : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeDisplayModes(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        int *tmodeptr = &__g_projectconfig_timemode2;
        if (tmodeptr && *tmodeptr>=0)
        {
            (*tmodeptr)++;
            if ((*tmodeptr)>5)
                (*tmodeptr)=0;
        }
        else
        {
            tmodeptr = &__g_projectconfig_timemode;
            
            if (tmodeptr)
            {
                (*tmodeptr)++;
                if ((*tmodeptr)>5)
                    (*tmodeptr)=0;
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoNextPage : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoNextPage(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
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
    GoPage(string name, WidgetActionManager* manager, vector<string> params) : ActionWithStringParam(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        TheManager->GoPage(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoZone(string name, WidgetActionManager* manager, vector<string> params) : ActionWithStringParam(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
    {
        if(value == 0.0) return; // ignore button releases

        GetPage()->GoZone(GetSurface(), param_, sender);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackBank(string name, WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
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
    SetShift(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetPage()->GetShift());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        GetPage()->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetOption : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetOption(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetPage()->GetOption());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        GetPage()->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetControl : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetControl(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetPage()->GetControl());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        GetPage()->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetAlt : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetAlt(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(GetWidget(), GetPage()->GetAlt());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        GetPage()->SetAlt(value);
    }
};

#endif /* control_surface_manager_actions_h */
