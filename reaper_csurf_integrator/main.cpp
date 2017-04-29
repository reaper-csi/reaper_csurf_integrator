#define REAPERAPI_IMPLEMENT
#define REAPERAPI_DECL

#include "reaper_plugin_functions.h"

extern reaper_csurf_reg_t csurf_integrator_reg;

extern "C"
{
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *reaper_plugin_info)
{
  if (reaper_plugin_info)
  {
	// load Reaper API functions
	if (REAPERAPI_LoadAPI(reaper_plugin_info->GetFunc) > 0)
	{
		return 0;
	}
      
      reaper_plugin_info->Register("csurf",&csurf_integrator_reg);
      
      //reaper_plugin_info->Register("projectconfig", &csurf_eucon_projectconfig); implement later for per project settings 

      
      
      
      
	// plugin registered
	return 1;
  }
  else
  {
    return 0;
  }
}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
};
