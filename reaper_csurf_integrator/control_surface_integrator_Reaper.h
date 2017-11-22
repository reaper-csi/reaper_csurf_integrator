//
//  control_surface_integrator_Reaper.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_integrator_Reaper_h
#define control_surface_integrator_Reaper_h

#include "reaper_plugin_functions.h"

extern HWND g_hwnd;

struct MIDI_event_ex_t : MIDI_event_t
{
    MIDI_event_ex_t() {};
    
    MIDI_event_ex_t(const unsigned char first, const unsigned char second, const unsigned char third)
    {
        size = 3;
        midi_message[0] = first;
        midi_message[1] = second;
        midi_message[2] = third;
        midi_message[3] = 0x00;
    };
    
    bool IsEqualTo(const MIDI_event_ex_t* other) const
    {
        if(this->size != other->size)
            return false;
        
        for(int i = 0; i < size; ++i)
            if(this->midi_message[i] != other->midi_message[i])
                return false;
        
        return true;
    }
};

static const string ReaperMasterTrackGUID = "ReaperMasterTrackGUID";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DAW
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    DAW() {}
    
    void SwapBufsPrecise(midi_Input* midiInput)
    {
#ifndef timeGetTime
        midiInput->SwapBufsPrecise(GetTickCount(), GetTickCount());
#else
        midiInput->SwapBufsPrecise(timeGetTime(), timeGetTime());
#endif
    }
    
    DWORD GetPrivateProfileString(const char *appname, const char *keyname, const char *def, char *ret, int retsize, const char *fn) { return ::GetPrivateProfileString(appname, keyname, def, ret, retsize, fn); }

    const char* GetResourcePath() { return ::GetResourcePath(); }

    midi_Input* CreateMIDIInput(int dev) {  return ::CreateMIDIInput(dev); }
    
    midi_Output* CreateMIDIOutput(int dev, bool streamMode, int* msoffset100) {  return ::CreateMIDIOutput(dev, streamMode, msoffset100); }
   
    void SetAutomationMode(int mode, bool onlySel) { ::SetAutomationMode(mode, onlySel); } 

    int GetGlobalAutomationOverride() { return ::GetGlobalAutomationOverride(); }

    void SetGlobalAutomationOverride(int mode) { ::SetGlobalAutomationOverride(mode); }
    
    int TrackFX_GetNumParams(MediaTrack* track, int fx) { return ::TrackFX_GetNumParams(track, fx); }
    
    bool TrackFX_GetParamName(MediaTrack* track, int fx, int param, char* buf, int buf_sz) { return ::TrackFX_GetParamName(track, fx, param, buf, buf_sz); }
    
    double TrackFX_GetParam(MediaTrack* track, int fx, int param, double* minvalOut, double* maxvalOut) { return ::TrackFX_GetParam(track, fx, param, minvalOut, maxvalOut); }
    
    bool TrackFX_SetParam(MediaTrack* track, int fx, int param, double val) { return ::TrackFX_SetParam(track, fx, param, val); }

    double GetMediaTrackInfo_Value(MediaTrack* track, const char* parmname) { return ::GetMediaTrackInfo_Value(track, parmname); }
    
    void* GetSetMediaTrackInfo(MediaTrack* tr, const char* parmname, void* setNewValue) { return ::GetSetMediaTrackInfo(tr, parmname, setNewValue); }

    double CSurf_OnVolumeChange(MediaTrack* trackid, double volume, bool relative) { return ::CSurf_OnVolumeChange(trackid, volume, relative); }

    void CSurf_SetSurfaceVolume(MediaTrack* trackid, double volume, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceVolume(trackid, volume, ignoresurf); }

    double Track_GetPeakInfo(MediaTrack* track, int channel) { return ::Track_GetPeakInfo(track, channel); }
    
    double CSurf_OnPanChange(MediaTrack* trackid, double pan, bool relative) { return ::CSurf_OnPanChange(trackid, pan, relative); }

    void CSurf_SetSurfacePan(MediaTrack* trackid, double pan, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfacePan(trackid, pan, ignoresurf); }

    double CSurf_OnWidthChange(MediaTrack* trackid, double width, bool relative) { return ::CSurf_OnWidthChange(trackid, width, relative); }
    
    bool CSurf_OnSelectedChange(MediaTrack* trackid, int selected) { return ::CSurf_OnSelectedChange(trackid, selected); }

    void CSurf_SetSurfaceSelected(MediaTrack* trackid, bool selected, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceSelected(trackid, selected, ignoresurf); }
    
    void SetOnlyTrackSelected(MediaTrack* track) { ::SetOnlyTrackSelected(track); }

    int CountSelectedTracks(ReaProject* proj) { return ::CountSelectedTracks(proj); }

    bool CSurf_OnRecArmChange(MediaTrack* trackid, int recarm) { return ::CSurf_OnRecArmChange(trackid, recarm); }

    void CSurf_SetSurfaceRecArm(MediaTrack* trackid, bool recarm, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceRecArm(trackid, recarm, ignoresurf); }

    bool CSurf_OnMuteChange(MediaTrack* trackid, int mute) { return ::CSurf_OnMuteChange(trackid, mute); }

    void CSurf_SetSurfaceMute(MediaTrack* trackid, bool mute, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceMute(trackid, mute, ignoresurf); }

    bool CSurf_OnSoloChange(MediaTrack* trackid, int solo) { return ::CSurf_OnSoloChange(trackid, solo); }

    void CSurf_SetSurfaceSolo(MediaTrack* trackid, bool solo, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceSolo(trackid, solo, ignoresurf); }
    
    void CSurf_OnArrow(int whichdir, bool wantzoom) { ::CSurf_OnArrow(whichdir, wantzoom); }

    void CSurf_OnRew(int seekplay) { ::CSurf_OnRew(seekplay); }

    void CSurf_OnFwd(int seekplay) { ::CSurf_OnFwd(seekplay); }
    
    void CSurf_OnStop() { ::CSurf_OnStop(); }

    void CSurf_OnPlay() { ::CSurf_OnPlay(); }
    
    void CSurf_OnRecord() { ::CSurf_OnRecord(); }

    int GetPlayState() { return ::GetPlayState(); }
    
    int GetSetRepeatEx(ReaProject* proj, int val) { return ::GetSetRepeatEx(proj, val); }

    void SendMessage(UINT uint, WPARAM wparam, LPARAM lparam) { ::SendMessage(g_hwnd, uint, wparam, lparam); }
    
    void ShowConsoleMsg(const char* msg) { ::ShowConsoleMsg(msg); }
    
    int GetNumTracks() { return ::GetNumTracks(); };

    MediaTrack* CSurf_TrackFromID(int idx, bool mcpView) { return ::CSurf_TrackFromID(idx, mcpView); }
    
    int CSurf_TrackToID(MediaTrack* track, bool mcpView) { return ::CSurf_TrackToID(track, mcpView);}

    string GetTrackGUIDAsString(int trackNumber)
    {
        if(trackNumber < 0 || trackNumber > GetNumTracks())
            return "";
        else if(trackNumber == 0)
            return ReaperMasterTrackGUID;
        else
        {
            char pBuffer[256];
            memset(pBuffer, 0, sizeof(pBuffer));
            guidToString(GetTrackGUID(CSurf_TrackFromID(trackNumber, false)), pBuffer);
            return pBuffer;
        }
    }

    MediaTrack *GetTrackFromGUID(string trackGUID)
    {
        for(int i = 0; i < GetNumTracks() + 1; i++) // +1 is for Reaper Master Track
            if(GetTrackGUIDAsString(i) == trackGUID)
                return CSurf_TrackFromID(i, false);
        
        return nullptr;
    }
};

#endif /* control_surface_integrator_Reaper_h */
