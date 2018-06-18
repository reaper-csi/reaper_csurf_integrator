//
//  control_surface_manager_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_manager_actions_h
#define control_surface_manager_actions_h

#include "control_surface_base_actions.h"

extern Manager* TheManager;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MediaTrack* previousTrack_ = nullptr;
    
public:
    void Do(Page* page, RealSurface* surface, MediaTrack* track) override
    {
        if(1 == DAW::CountSelectedTracks(nullptr) && previousTrack_ != track)
        {
            page->UnmapWidgetsFromFX(surface, previousTrack_);
            page->MapFXToWidgets(surface, previousTrack_ = track);
        }
        else
            page->UnmapWidgetsFromFX(surface, track);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MapTrackAndFXToWidgets  : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MediaTrack* previousTrack_ = nullptr;
    
public:
    void Do(Page* page, RealSurface* surface, MediaTrack* oldTrack) override
    {
        if(1 == DAW::CountSelectedTracks(nullptr)) //&& previousTrack_ != track)
        {
            MediaTrack* track = nullptr;
            
            for(int i = 0; i < CSurf_NumTracks(page->GetFollowMCP()); i++)
                if(DAW::GetMediaTrackInfo_Value(DAW::CSurf_TrackFromID(i, page->GetFollowMCP()), "I_SELECTED"))
                {
                    track = DAW::CSurf_TrackFromID(i, page->GetFollowMCP());
                    break;
                }
            
            if(track)
            {
                page->UnmapWidgetsFromTrackAndFX(surface, previousTrack_);
                page->MapTrackAndFXToWidgets(surface, previousTrack_ = track);
            }
        }
        else
            page->UnmapWidgetsFromTrackAndFX(surface, oldTrack);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SetShowFXWindows : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        page->SetShowFXWindows(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latched : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        
    }

    /*
private:
    clock_t lastPressed_ = clock();
public:
    
    virtual void SetValue(string zoneName, string surfaceName, double value) {}
    
    virtual void Do(double value, string zoneName, string surfaceName, string widgetName) override
    {
        if(value != 0)
        {
            lastPressed_ = clock();
            SetValue(zoneName, surfaceName, value);
            GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
        }
        else
        {
            if(clock() - lastPressed_ >  CLOCKS_PER_SEC / 4)
            {
                SetValue(zoneName, surfaceName, value);
                GetLayer()->SetWidgetValue(zoneName, surfaceName, widgetName, value);
            }
        }
    }
     */
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedZoom : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        
    }

    /*
public:
    
    virtual void SetValue(string zoneName, string surfaceName, double value) override
    {
        GetLayer()->SetZoom(zoneName, surfaceName, value);
    }
     */
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatchedScrub : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        
    }

    /*
public:
    
    virtual void SetValue(string zoneName, string surfaceName, double value) override
    {
        GetLayer()->SetScrub(zoneName, surfaceName, value);
    }
     */
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NextPage : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double value) override
    {
        TheManager->NextPage();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackBank : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    void Do(Page* page, double stride) override
    {
         page->AdjustTrackBank(stride);
    }
};

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

#endif /* control_surface_manager_actions_h */
