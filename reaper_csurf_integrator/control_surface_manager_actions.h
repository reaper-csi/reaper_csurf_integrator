//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_integrator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleLearnMode  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleLearnMode(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->ToggleLearnMode();
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
        if(MediaTrack* track = widget_->GetTrack())
            page_->GetTrackNavigationManager()->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackSends  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackSends(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetShouldMapSends());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->ToggleMapSends(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFX  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackFX(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetFXActivationManager()->GetShouldMapSelectedTrackFX());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->ToggleMapSelectedFX(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFXMenu  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSelectedTrackFXMenu(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetFXActivationManager()->GetShouldMapSelectedTrackFXMenus());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->ToggleMapFXMenu(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapFocusedFX  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapFocusedFX(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetFXActivationManager()->GetShouldMapFocusedFX());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->ToggleMapFocusedTrackFX(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoFXSlot  : public SurfaceActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoFXSlot(string name, WidgetActionManager* manager, vector<string> params) : SurfaceActionWithIntParam(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
    {
        int fxIndex = param_ - 1 < 0 ? 0 : param_ - 1;
        
        page_->MapSelectedTrackFXSlotToWidgets(surface_, fxIndex);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsToWidgets  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackSendsToWidgets(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->MapSelectedTrackSendsToWidgets(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToWidgets  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToWidgets(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->MapSelectedTrackFXToWidgets(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToMenu  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToMenu(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->MapSelectedTrackFXToMenu(surface_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapFocusedFXToWidgets  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapFocusedFXToWidgets(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->MapFocusedTrackFXToWidgets(surface_);
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
        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            int trackIndex = 0;
            
            for(int i = 0; i < page_->GetTrackNavigationManager()->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(page_->GetTrackNavigationManager()->GetTrackFromId(i), "I_SELECTED"))
                {
                    trackIndex = i;
                    break;
                }
            
            trackIndex += param_;
            
            if(trackIndex < 0)
                trackIndex = 0;
            
            if(trackIndex > page_->GetTrackNavigationManager()->GetNumTracks() - 1)
                trackIndex = page_->GetTrackNavigationManager()->GetNumTracks() - 1;
            
            DAW::SetOnlyTrackSelected(page_->GetTrackNavigationManager()->GetTrackFromId(trackIndex));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows(string name, WidgetActionManager* manager, vector<string> params) : SurfaceAction(name, manager, params) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetFXActivationManager()->GetShowFXWindows());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        surface_->GetFXActivationManager()->SetShowFXWindows(value);
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
        SetWidgetValue(widget_, page_->GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(double value, WidgetActionManager* sender) override
    {
        page_->GetTrackNavigationManager()->ToggleScrollLink(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeDisplayModes : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeDisplayModes(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager) {}

    void Do(double value, WidgetActionManager* sender) override
    {
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
    GoNextPage(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager) {}

    void Do(double value, WidgetActionManager* sender) override
    {
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
        TheManager->GoPage(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public SurfaceActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoZone(string name, WidgetActionManager* manager, vector<string> params) : SurfaceActionWithStringParam(name, manager, params) {}

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->GoZone(surface_, param_, sender);
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
        TheManager->AdjustTrackBank(page_, param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShift(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, page_->GetShift());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetOption : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetOption(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, page_->GetOption());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetControl : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetControl(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, page_->GetControl());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetAlt : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetAlt(string name, WidgetActionManager* manager, vector<string> params) : Action(name, manager) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, page_->GetAlt());
    }

    void Do(double value, WidgetActionManager* sender) override
    {
        page_->SetAlt(value);
    }
};

#endif /* control_surface_manager_actions_h */
