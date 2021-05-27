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
        
        if(MediaTrack* selectedTrack = context->GetSurface()->GetPage()->GetTrackNavigationManager()->GetSelectedTrack())
            context->GetSurface()->MapSelectedTrackFXMenuSlotToWidgets(context->GetSlotIndex());
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
            context->GetSurface()->UnmapSelectedTrackSendsFromWidgets();
        else if(value)
            context->GetSurface()->MapSelectedTrackSendsToWidgets();
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
            context->GetSurface()->UnmapSelectedTrackSendsFromWidgets();
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
            context->GetSurface()->UnmapSelectedTrackReceivesFromWidgets();
        else if(value)
            context->GetSurface()->MapSelectedTrackReceivesToWidgets();
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
            context->GetSurface()->UnmapSelectedTrackReceivesFromWidgets();
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
            context->GetSurface()->UnmapSelectedTrackFXFromWidgets();
        else if(value)
            context->GetSurface()->MapSelectedTrackFXToWidgets();
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
            context->GetSurface()->UnmapSelectedTrackFXFromWidgets();
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
            context->GetSurface()->UnmapSelectedTrackFXFromMenu();
        else if(value)
            context->GetSurface()->MapSelectedTrackFXToMenu();
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
            context->GetSurface()->UnmapSelectedTrackFXFromMenu();
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
        if(value == 0.0 && context->GetWidget()->GetName() != "OnTrackSelection")
            return; // ignore button releases except for OnTrackSelection -- 0 means Track deselection
        
        context->GetSurface()->GoZone(context->GetStringParam(), value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoSubZone : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual string GetName() override { return "GoZone"; }
    
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
               DAW::SetOnlyTrackSelected(trackToSelect);
        }
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
        
        TheManager->AdjustFXMenuSlotBank(context->GetPage(), context->GetIntParam());
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

#endif /* control_surface_manager_actions_h */
