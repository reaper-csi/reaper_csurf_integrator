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
class GoFXSlot  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GoFXSlot"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetPage()->MapSelectedTrackFXMenuSlotToWidgets(context->GetSurface(), context->GetSlotIndex());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoCurrentFXSlot  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GoFXSlot"; }

    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        context->GetPage()->MapSelectedTrackFXMenuSlotToWidgets(context->GetSurface(), context->GetPage()->GetTrackNavigationManager()->GetFXMenuSlot());
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
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapSelectedTrackSendsFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapSelectedTrackSendsToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapSelectedTrackSendsFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapSelectedTrackSendsFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapSelectedTrackSendsFromWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackReceivesToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapSelectedTrackReceivesToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapSelectedTrackReceivesFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapSelectedTrackReceivesToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapSelectedTrackReceivesFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapSelectedTrackReceivesFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapSelectedTrackReceivesFromWidgets(context->GetSurface());
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
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapSelectedTrackFXFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapSelectedTrackFXToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapSelectedTrackFXFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapSelectedTrackFXFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapSelectedTrackFXFromWidgets(context->GetSurface());
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
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapSelectedTrackFXFromMenu(context->GetSurface());
        else if(value)
            context->GetPage()->MapSelectedTrackFXToMenu(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapSelectedTrackFXFromMenu  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapSelectedTrackFXFromMenu"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapSelectedTrackFXFromMenu(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackSendsSlotToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapTrackSendsSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapTrackSendsSlotFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapTrackSendsSlotToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapTrackSendsSlotFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapTrackSendsSlotFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapTrackSendsSlotFromWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackReceivesSlotToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapTrackReceivesSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapTrackReceivesSlotFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapTrackReceivesSlotToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapTrackReceivesSlotFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapTrackReceivesSlotFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapTrackReceivesSlotFromWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackFXMenusSlotToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapTrackFXMenusSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapTrackFXMenusSlotFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapTrackFXMenusSlotToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapTrackFXMenusSlotFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapTrackFXMenusSlotFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapTrackFXMenusSlotFromWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsSlotToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapSelectedTrackSendsSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapSelectedTrackSendsSlotFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapSelectedTrackSendsSlotToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapSelectedTrackSendsSlotFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapSelectedTrackSendsSlotFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapSelectedTrackSendsSlotFromWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackReceivesSlotToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "MapSelectedTrackReceivesSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->UnmapSelectedTrackReceivesSlotFromWidgets(context->GetSurface());
        else if(value)
            context->GetPage()->MapSelectedTrackReceivesSlotToWidgets(context->GetSurface());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapSelectedTrackReceivesSlotFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapSelectedTrackReceivesSlotFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetPage()->UnmapSelectedTrackReceivesSlotFromWidgets(context->GetSurface());
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
        if(value == 0 && context->GetWidget()->GetName() == "OnFXFocus")
            context->GetSurface()->UnmapFocusedFXFromWidgets();
        else if(value)
            context->GetSurface()->MapFocusedFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnmapFocusedFXFromWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "UnmapFocusedFXFromWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value)
            context->GetSurface()->UnmapFocusedFXFromWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleScrollLink : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ToggleScrollLink"; }

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return context->GetTrackNavigationManager()->GetScrollLink();
    }
    
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
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

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return context->GetTrackNavigationManager()->GetVCAMode();
    }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
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
class PageNameDisplay : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "PageNameDisplay"; }
    
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(context->GetPage()->GetName());
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
        if(value == 0 && context->GetWidget()->GetName() == "OnTrackSelection")
            context->GetPage()->GoZone(context->GetSurface(), context->GetStringParam(), value);
        else if(value)
            context->GetPage()->GoZone(context->GetSurface(), context->GetStringParam(), value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoSubZone : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GoSubZone"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0)
            return; // ignore button releases
        
        context->GetSurface()->GoSubZone(context->GetZone(), context->GetStringParam(), value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ClearAllSolo : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ClearAllSolo"; }

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return DAW::AnyTrackSolo(nullptr);
    }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
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
class SelectedTrackBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SelectedTrackBank"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        if(MediaTrack* selectedTrack = context->GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack())
        {
            int trackNum = context->GetSurface()->GetPage()->GetTrackNavigationManager()->GetIdFromTrack(selectedTrack);
            
            trackNum += context->GetIntParam();
            
            if(trackNum < 1)
                trackNum = 1;
            
            if(trackNum > context->GetPage()->GetTrackNavigationManager()->GetNumTracks())
                trackNum = context->GetPage()->GetTrackNavigationManager()->GetNumTracks();
            
            if(MediaTrack* trackToSelect = context->GetPage()->GetTrackNavigationManager()->GetTrackFromId(trackNum))
            {
                DAW::SetOnlyTrackSelected(trackToSelect);
                context->GetSurface()->OnTrackSelection();
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SlotBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SlotBank"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustSendSlotBank(context->GetPage(), context->GetIntParam());
        TheManager->AdjustReceiveSlotBank(context->GetPage(), context->GetIntParam());
        TheManager->AdjustFXMenuSlotBank(context->GetPage(), context->GetSurface(), context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SendSlotBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SendSlotBank"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustSendSlotBank(context->GetPage(), context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReceiveSlotBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "ReceiveSlotBank"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustReceiveSlotBank(context->GetPage(), context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXMenuSlotBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "FXMenuSlotBank"; }
    
    void Do(ActionContext* context, double value) override
    {
        if(value == 0.0) return; // ignore button releases
        
        TheManager->AdjustFXMenuSlotBank(context->GetPage(), context->GetSurface(), context->GetIntParam());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetShift"; }

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return context->GetPage()->GetShift();
    }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
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

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return context->GetPage()->GetOption();
    }
    
    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
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

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return context->GetPage()->GetControl();
    }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
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

    virtual double GetCurrentNormalizedValue(ActionContext* context) override
    {
        return context->GetPage()->GetAlt();
    }

    void RequestUpdate(ActionContext* context) override
    {
        context->UpdateWidgetValue(GetCurrentNormalizedValue(context));
    }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetPage()->SetAlt(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastGoZone : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastGoZone"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastGoZone();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveGoZone : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveGoZone"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveGoZone();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastGoFXSlot : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastGoFXSlot"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastGoFXSlot();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveGoFXSlot : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveGoFXSlot"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveGoFXSlot();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapSelectedTrackSendsToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapSelectedTrackSendsToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapSelectedTrackSendsToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapSelectedTrackSendsToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapSelectedTrackSendsToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapSelectedTrackSendsToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapSelectedTrackReceivesToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapSelectedTrackReceivesToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapSelectedTrackReceivesToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapSelectedTrackReceivesToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapSelectedTrackReceivesToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapSelectedTrackReceivesToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapSelectedTrackFXToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapSelectedTrackFXToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapSelectedTrackFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapSelectedTrackFXToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapSelectedTrackFXToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapSelectedTrackFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapSelectedTrackFXToMenu : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapSelectedTrackFXToMenu"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapSelectedTrackFXToMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapSelectedTrackFXToMenu : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapSelectedTrackFXToMenu"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapSelectedTrackFXToMenu();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapTrackSendsSlotToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapTrackSendsSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapTrackSendsSlotToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapTrackSendsSlotToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapTrackSendsSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapTrackSendsSlotToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapTrackReceivesSlotToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapTrackReceivesSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapTrackReceivesSlotToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapTrackReceivesSlotToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapTrackReceivesSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapTrackReceivesSlotToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetBroadcastMapTrackFXMenusSlotToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetBroadcastMapTrackFXMenusSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetBroadcastMapTrackFXMenusSlotToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetReceiveMapTrackFXMenusSlotToWidgets : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "SetReceiveMapTrackFXMenusSlotToWidgets"; }
    
    void Do(ActionContext* context, double value) override
    {
        context->GetSurface()->SetReceiveMapTrackFXMenusSlotToWidgets();
    }
};


#endif /* control_surface_manager_actions_h */
