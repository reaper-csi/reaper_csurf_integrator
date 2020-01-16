#define REAPERAPI_IMPLEMENT
#define REAPERAPI_DECL

#include "reaper_plugin_functions.h"
#include "resource.h"

gaccel_register_t acreg=
{
    {FCONTROL|FALT|FVIRTKEY, '1', 0},
    "CSI Learn Mode"
};

int g_registered_command = 0;

extern bool hookCommandProc(int command, int flag);

extern  void ShutdownMidiIO();

extern reaper_csurf_reg_t csurf_integrator_reg;

REAPER_PLUGIN_HINSTANCE g_hInst; // used for dialogs, if any
HWND g_hwnd;
reaper_plugin_info_t *g_reaper_plugin_info;

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *reaper_plugin_info)
{
    g_hInst = hInstance;
    
    if (! reaper_plugin_info)
    {
        ShutdownMidiIO();
        return 0;
    }
    
    if (reaper_plugin_info->caller_version != REAPER_PLUGIN_VERSION || !reaper_plugin_info->GetFunc)
        return 0;

    if (reaper_plugin_info)
    {
        g_hwnd = reaper_plugin_info->hwnd_main;
        g_reaper_plugin_info = reaper_plugin_info;

        // load Reaper API functions
        if (REAPERAPI_LoadAPI(reaper_plugin_info->GetFunc) > 0)
        {
            return 0;
        }
      
        reaper_plugin_info->Register("csurf",&csurf_integrator_reg);
        
        acreg.accel.cmd = g_registered_command = reaper_plugin_info->Register("command_id", (void*)"CSILearnMode");
        
        if (!g_registered_command)
            return 0; // failed getting a command id, fail!
        
        reaper_plugin_info->Register("gaccel", &acreg);
        reaper_plugin_info->Register("hookcommand", (void*)hookCommandProc);
        
      
        // plugin registered
        return 1;
    }
    else
    {
        return 0;
    }
}
    
#ifndef _WIN32 // import the resources. Note: if you do not have these files, run "php WDL/swell/mac_resgen.php res.rc" from this directory
#include "./WDL/swell/swell-dlggen.h"
#include "res.rc_mac_dlg"
#endif


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
};
