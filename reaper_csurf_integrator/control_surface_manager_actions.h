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
    TogglePin(WidgetActionManager* manager) : TrackAction(manager) {}
       
    void Do(double value) override
    {
        if(MediaTrack* track = widget_->GetTrack())
            page_->TogglePin(track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleMapSends  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleMapSends(WidgetActionManager* manager) : SurfaceAction(manager) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetShouldMapSends());
    }
    
    void Do(double value) override
    {
        surface_->ToggleMapSends();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackSendsToWidgets  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackSendsToWidgets(WidgetActionManager* manager) : SurfaceAction(manager) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        surface_->MapSelectedTrackSendsToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapSelectedTrackFXToWidgets  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapSelectedTrackFXToWidgets(WidgetActionManager* manager) : SurfaceAction(manager) {}

    void Do(double value) override
    {
        surface_->MapSelectedTrackFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapFocusedTrackFXToWidgets  : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MapFocusedTrackFXToWidgets(WidgetActionManager* manager) : SurfaceAction(manager) {}

    void Do(double value) override
    {
        surface_->MapFocusedTrackFXToWidgets();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SelectTrackRelative : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SelectTrackRelative(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params) {}

    void Do(double value) override
    {
        if(1 == DAW::CountSelectedTracks(nullptr))
        {
            int trackIndex = 0;
            
            for(int i = 0; i < page_->GetNumTracks(); i++)
                if(DAW::GetMediaTrackInfo_Value(page_->GetTrackFromId(i), "I_SELECTED"))
                {
                    trackIndex = i;
                    break;
                }
            
            trackIndex += param_;
            
            if(trackIndex < 0)
                trackIndex = 0;
            
            if(trackIndex > page_->GetNumTracks() - 1)
                trackIndex = page_->GetNumTracks() - 1;
            
            DAW::SetOnlyTrackSelected(page_->GetTrackFromId(trackIndex));
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public SurfaceAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShowFXWindows(WidgetActionManager* manager) : SurfaceAction(manager) {}
    
    void RequestUpdate() override
    {
        SetWidgetValue(widget_, surface_->GetShowFXWindows());
    }
    
    void Do(double value) override
    {
        surface_->SetShowFXWindows(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleScrollLink : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleScrollLink(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params) {}

    void RequestUpdate() override
    {
        SetWidgetValue(widget_, page_->GetScrollLink());
    }
    
    void Do(double value) override
    {
        page_->ToggleScrollLink(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CycleTimeDisplayModes : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    CycleTimeDisplayModes(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override {}

    void Do(double value) override
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
    GoNextPage(WidgetActionManager* manager) : Action(manager) {}
    
    void RequestUpdate() override {}

    void Do(double value) override
    {
        TheManager->NextPage();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoPage : public ActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoPage(WidgetActionManager* manager, vector<string> params) : ActionWithStringParam(manager, params) {}

    void RequestUpdate() override {}
    
    void Do(double value) override
    {
        TheManager->GoPage(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GoZone : public SurfaceActionWithStringParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GoZone(WidgetActionManager* manager, vector<string> params) : SurfaceActionWithStringParam(manager, params) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        surface_->GoZone(param_);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank : public ActionWithIntParam
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    TrackBank(WidgetActionManager* manager, vector<string> params) : ActionWithIntParam(manager, params) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        TheManager->AdjustTrackBank(page_, param_);
    }
};

/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PinSelectedTracks : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        page->PinSelectedTracks();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UnpinSelectedTracks : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        page->UnpinSelectedTracks();
    }
};
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShift : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetShift(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        page_->SetShift(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetOption : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetOption(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        page_->SetOption(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetControl : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetControl(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        page_->SetControl(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetAlt : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    SetAlt(WidgetActionManager* manager) : Action(manager) {}

    void RequestUpdate() override {}

    void Do(double value) override
    {
        page_->SetAlt(value);
    }
};

#endif /* control_surface_manager_actions_h */
