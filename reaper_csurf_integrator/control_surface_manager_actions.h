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
    virtual string GetName() override { return "TogglePin"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* track = context->GetTrack())
            context->GetTrackNavigationManager()->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackSends  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleMapSelectedTrackSends"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetShouldMapSends());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->ToggleMapSends();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFX  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleMapSelectedTrackFX"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFX());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleMapSelectedTrackFX();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSelectedTrackFXMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleMapSelectedTrackFXMenu"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShouldMapSelectedTrackFXMenus());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleMapSelectedTrackFXMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapFocusedFX  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleMapFocusedFX"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShouldMapFocusedFX());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleMapFocusedFX();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoFXSlot  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GoFXSlot"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        int fxSlot = context->GetIntParam() - 1 < 0 ? 0 : context->GetIntParam() - 1;
        
        if(MediaTrack* selectedTrack = context->GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack())
            context->GetSurface()->GetFXActivationManager()->MapSelectedTrackFXSlotToWidgets(selectedTrack, fxSlot);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapSelectedTrackSendsToWidgets"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->MapSelectedTrackSendsToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapSelectedTrackFXToWidgets"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->MapSelectedTrackFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapSelectedTrackFXToMenu"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->MapSelectedTrackFXToMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapFocusedFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapFocusedFXToWidgets"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->MapFocusedFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectTrackRelative : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SelectTrackRelative"; }

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
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetShowFXWindows"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetSurface()->GetFXActivationManager()->GetShowFXWindows());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetSurface()->GetFXActivationManager()->ToggleShowFXWindows();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleScrollLink"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetTrackNavigationManager()->GetScrollLink());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetTrackNavigationManager()->ToggleScrollLink(context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ForceScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ForceScrollLink"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetTrackNavigationManager()->ForceScrollLink();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleVCAMode : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleVCAMode"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetTrackNavigationManager()->GetVCAMode());
    }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetTrackNavigationManager()->ToggleVCAMode();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeDisplayModes : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "CycleTimeDisplayModes"; }

    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "GoNextPage"; }

    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "GoPage"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->GoToPage(context->GetStringParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GoZone"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0)
            return; // ignore button releases
        
        context->GetSurface()->GoZone(context->GetStringParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ClearAllSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ClearAllSolo"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(DAW::AnyTrackSolo(nullptr));
    }
    
    void Do(ActionContext* context, double value) override
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
    virtual string GetName() override { return "TrackBank"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustTrackBank(context->GetPage(), context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetShift"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetShift());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetOption : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetOption"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetOption());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetControl : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetControl"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetControl());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetAlt : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetAlt"; }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetAlt());
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetAlt(value);
    }
};

#endif /* control_surface_manager_actions_h */
