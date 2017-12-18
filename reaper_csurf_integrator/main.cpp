#define REAPERAPI_IMPLEMENT
#define REAPERAPI_DECL

#include "reaper_plugin_functions.h"
#include "resource.h"

extern reaper_csurf_reg_t csurf_integrator_reg;

REAPER_PLUGIN_HINSTANCE g_hInst; // used for dialogs, if any
HWND g_hwnd;

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *reaper_plugin_info)
{
    g_hInst = hInstance;
    
    if (!reaper_plugin_info || reaper_plugin_info->caller_version != REAPER_PLUGIN_VERSION || !reaper_plugin_info->GetFunc)
        return 0;

    if (reaper_plugin_info)
    {
        g_hwnd = reaper_plugin_info->hwnd_main;

        // load Reaper API functions
        if (REAPERAPI_LoadAPI(reaper_plugin_info->GetFunc) > 0)
        {
            return 0;
        }
      
        reaper_plugin_info->Register("csurf",&csurf_integrator_reg);
      
        // plugin registered
        return 1;
    }
    else
    {
        return 0;
    }
}
    
#ifndef _WIN32 // import the resources. Note: if you do not have these files, run "php ../mac_resgen.php sample_project.rc" from this directory
#include "./WDL/swell/swell-dlggen.h"
#include "res.rc_mac_dlg"
#endif


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
};
