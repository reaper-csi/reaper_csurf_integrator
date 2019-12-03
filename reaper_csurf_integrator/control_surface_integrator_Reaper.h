//
//  control_surface_integrator_Reaper.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_integrator_Reaper_h
#define control_surface_integrator_Reaper_h

#include "reaper_plugin_functions.h"

using namespace std;

extern HWND g_hwnd;

const int BUFSZ = 512;

struct rgb_color
{
    int r = 0;
    int g = 0;
    int b = 0;
};

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

// substracts b<T> from a<T>
template <typename T>
void
subtract_vector(std::vector<T>& a, const std::vector<T>& b)
{
    typename std::vector<T>::iterator       it = a.begin();
    typename std::vector<T>::const_iterator it2 = b.begin();
    typename std::vector<T>::iterator       end = a.end();
    typename std::vector<T>::const_iterator end2 = b.end();
    
    while (it != end)
    {
        while (it2 != end2)
        {
            if (*it == *it2)
            {
                it = a.erase(it);
                end = a.end();
                it2 = b.begin();
            }
            else
                ++it2;
        }
        ++it;
        it2 = b.begin();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DAW
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    static void SwapBufsPrecise(midi_Input* midiInput)
    {
    #ifndef timeGetTime
            midiInput->SwapBufsPrecise(GetTickCount(), GetTickCount());
    #else
            midiInput->SwapBufsPrecise(timeGetTime(), timeGetTime());
    #endif
    }
    
    static double GetCurrentNumberOfMilliseconds()
    {
    #ifndef timeGetTime
            return GetTickCount();
    #else
            return timeGetTime();
    #endif
    }
    
    static const char* get_ini_file() { return ::get_ini_file(); }

    static DWORD GetPrivateProfileString(const char *appname, const char *keyname, const char *def, char *ret, int retsize, const char *fn) { return ::GetPrivateProfileString(appname, keyname, def, ret, retsize, fn); }

    static const char* GetResourcePath() { return ::GetResourcePath(); }
    
    static int NamedCommandLookup(const char* command_name) { return ::NamedCommandLookup(command_name);  }

    static void SendCommandMessage(WPARAM wparam) { ::SendMessage(g_hwnd, WM_COMMAND, wparam, 0); }
    
    static int GetToggleCommandState(int commandId) { return ::GetToggleCommandState(commandId); }
    
    static void ShowConsoleMsg(const char* msg) { ::ShowConsoleMsg(msg); }
    
    static midi_Input* CreateMIDIInput(int dev) {  return ::CreateMIDIInput(dev); }
    
    static midi_Output* CreateMIDIOutput(int dev, bool streamMode, int* msoffset100) {  return ::CreateMIDIOutput(dev, streamMode, msoffset100); }
   
    static void SetAutomationMode(int mode, bool onlySel) { ::SetAutomationMode(mode, onlySel); }

    static int GetGlobalAutomationOverride() { return ::GetGlobalAutomationOverride(); }

    static void SetGlobalAutomationOverride(int mode) { ::SetGlobalAutomationOverride(mode); }

    static int GetFocusedFX(int* tracknumberOut, int* itemnumberOut, int* fxnumberOut) { return ::GetFocusedFX(tracknumberOut, itemnumberOut, fxnumberOut); }

    static void TrackFX_Show(MediaTrack* track, int index, int showFlag)
    {
        if(ValidatePtr(track, "MediaTrack*"))
            ::TrackFX_Show(track, index, showFlag);
    }

    static int TrackFX_GetCount(MediaTrack* track)
    {
        if(ValidatePtr(track, "MediaTrack*"))
            return ::TrackFX_GetCount(track);
        else
            return 0;
    }
    
    static bool TrackFX_GetFXName(MediaTrack* track, int fx, char* buf, int buf_sz)
    {
        if(ValidatePtr(track, "MediaTrack*"))
            return ::TrackFX_GetFXName(track, fx, buf, buf_sz);
        else
        {
            if(buf_sz > 0)
                buf[0] = 0;
            return false;
        }
    }
    
    static bool TrackFX_GetNamedConfigParm(MediaTrack* track, int fx, const char* parmname, char* buf, int buf_sz)
    {
        if(ValidatePtr(track, "MediaTrack*"))
            return ::TrackFX_GetNamedConfigParm(track, fx, parmname, buf, buf_sz);
        else
        {
            if(buf_sz > 0)
                buf[0] = 0;
            return false;
        }
    }

    static int TrackFX_GetNumParams(MediaTrack* track, int fx)
    {
        if(ValidatePtr(track, "MediaTrack*"))
            return ::TrackFX_GetNumParams(track, fx);
        else
            return 0;
    }
    
    static bool TrackFX_GetParamName(MediaTrack* track, int fx, int param, char* buf, int buf_sz) { return ::TrackFX_GetParamName(track, fx, param, buf, buf_sz); }
    
    static bool TrackFX_GetFormattedParamValue(MediaTrack* track, int fx, int param, char* buf, int buf_sz) { return ::TrackFX_GetFormattedParamValue(track, fx, param, buf, buf_sz); }
    
    static double TrackFX_GetParam(MediaTrack* track, int fx, int param, double* minvalOut, double* maxvalOut) { return ::TrackFX_GetParam(track, fx, param, minvalOut, maxvalOut); }
    
    static bool TrackFX_SetParam(MediaTrack* track, int fx, int param, double val) { return ::TrackFX_SetParam(track, fx, param, val); }

    static bool GetTrackName(MediaTrack* track, char* buf, int buf_sz) { return ::GetTrackName(track, buf, buf_sz); }
    
    static double GetMediaTrackInfo_Value(MediaTrack* track, const char* parmname) { return ::GetMediaTrackInfo_Value(track, parmname); }

    static double GetTrackSendInfo_Value(MediaTrack* track, int category, int send_index, const char* parmname) { return ::GetTrackSendInfo_Value(track, category, send_index, parmname); }

    static void* GetSetTrackSendInfo(MediaTrack* track, int category, int send_index, const char* parmname, void* setNewValue) { return ::GetSetTrackSendInfo(track, category, send_index, parmname, setNewValue); }
    
    static void* GetSetMediaTrackInfo(MediaTrack* track, const char* parmname, void* setNewValue) { return ::GetSetMediaTrackInfo(track, parmname, setNewValue); }

    static double CSurf_OnVolumeChange(MediaTrack* track, double volume, bool relative) { return ::CSurf_OnVolumeChange(track, volume, relative); }
    
    static double CSurf_OnPanChange(MediaTrack* track, double pan, bool relative) { return ::CSurf_OnPanChange(track, pan, relative); }

    static bool CSurf_OnMuteChange(MediaTrack* track, int mute) { return ::CSurf_OnMuteChange(track, mute); }

    static bool GetTrackUIMute(MediaTrack* track, bool* muteOut) { return ::GetTrackUIMute(track, muteOut); }
    
    static bool GetTrackUIVolPan(MediaTrack* track, double* volumeOut, double* panOut) { return ::GetTrackUIVolPan(track, volumeOut, panOut); }
    
    static void CSurf_SetSurfaceVolume(MediaTrack* track, double volume, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceVolume(track, volume, ignoresurf); }
    
    static double CSurf_OnSendVolumeChange(MediaTrack* track, int sendIndex, double volume, bool relative) { return ::CSurf_OnSendVolumeChange(track, sendIndex, volume, relative); }

    static double CSurf_OnSendPanChange(MediaTrack* track, int send_index, double pan, bool relative) { return ::CSurf_OnSendPanChange(track, send_index, pan, relative); }
    
    static int GetTrackNumSends(MediaTrack* track, int category) { return ::GetTrackNumSends(track, category); }
    
    static bool GetTrackSendUIMute(MediaTrack* track, int send_index, bool* muteOut) { return ::GetTrackSendUIMute(track, send_index, muteOut); }

    static bool GetTrackSendUIVolPan(MediaTrack* track, int send_index, double* volumeOut, double* panOut) { return ::GetTrackSendUIVolPan(track, send_index, volumeOut, panOut); }

    static double Track_GetPeakInfo(MediaTrack* track, int channel) { return ::Track_GetPeakInfo(track, channel); }

    static void CSurf_SetSurfacePan(MediaTrack* track, double pan, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfacePan(track, pan, ignoresurf); }

    static void CSurf_SetSurfaceMute(MediaTrack* track, bool mute, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceMute(track, mute, ignoresurf); }

    static double CSurf_OnWidthChange(MediaTrack* track, double width, bool relative) { return ::CSurf_OnWidthChange(track, width, relative); }
    
    static bool CSurf_OnSelectedChange(MediaTrack* track, int selected) { return ::CSurf_OnSelectedChange(track, selected); }

    static void CSurf_SetSurfaceSelected(MediaTrack* track, bool selected, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceSelected(track, selected, ignoresurf); }
    
    static void SetOnlyTrackSelected(MediaTrack* track) { ::SetOnlyTrackSelected(track); }

    static int CountSelectedTracks(ReaProject* proj) { return ::CountSelectedTracks2(proj, true); }
    
    static bool CSurf_OnRecArmChange(MediaTrack* track, int recarm) { return ::CSurf_OnRecArmChange(track, recarm); }

    static void CSurf_SetSurfaceRecArm(MediaTrack* track, bool recarm, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceRecArm(track, recarm, ignoresurf); }

    static bool CSurf_OnSoloChange(MediaTrack* track, int solo) { return ::CSurf_OnSoloChange(track, solo); }

    static void CSurf_SetSurfaceSolo(MediaTrack* track, bool solo, IReaperControlSurface* ignoresurf) { ::CSurf_SetSurfaceSolo(track, solo, ignoresurf); }
    
    static void CSurf_OnArrow(int whichdir, bool wantzoom) { ::CSurf_OnArrow(whichdir, wantzoom); }

    static void CSurf_OnRew(int seekplay) { ::CSurf_OnRew(seekplay); }

    static void CSurf_OnFwd(int seekplay) { ::CSurf_OnFwd(seekplay); }
    
    static void CSurf_OnStop() { ::CSurf_OnStop(); }

    static void CSurf_OnPlay() { ::CSurf_OnPlay(); }
    
    static void CSurf_OnRecord() { ::CSurf_OnRecord(); }

    static int GetPlayState() { return ::GetPlayState(); }
    
    static int GetSetRepeatEx(ReaProject* proj, int val) { return ::GetSetRepeatEx(proj, val); }

    static MediaTrack* GetMasterTrack(ReaProject* proj) { return ::GetMasterTrack(proj); };
    
    static int CSurf_NumTracks(bool mcpView) { return ::CSurf_NumTracks(mcpView); };
    
    static MediaTrack* CSurf_TrackFromID(int idx, bool mcpView) { return ::CSurf_TrackFromID(idx, mcpView); }

    static bool IsTrackVisible(MediaTrack* track, bool mixer) { return ::IsTrackVisible(track, mixer); }

    static MediaTrack* SetMixerScroll(MediaTrack* leftmosttrack) { return ::SetMixerScroll(leftmosttrack); }
    
    // Runs the system color chooser dialog.  Returns 0 if the user cancels the dialog.
    static int GR_SelectColor(HWND hwnd, int* colorOut) { return ::GR_SelectColor(hwnd, colorOut); }

    static void ColorFromNative(int col, int* rOut, int* gOut, int* bOut) { ::ColorFromNative(col, rOut, gOut, bOut); }

    static int ColorToNative(int r, int g, int b) { return ::ColorToNative(r, g, b); }
};

#endif /* control_surface_integrator_Reaper_h */
