//
//  ReportLoggingEtc.h
//  reaper_csurf_integrator
//
//  Created by Geoff Waddington on 2019-12-05.
//  Copyright © 2019 Geoff Waddington. All rights reserved.
//

#ifndef ReportLoggingEtc_h
#define ReportLoggingEtc_h

#include "reaper_plugin_functions.h"

/////////////////////////////////////////////////
class LOG
/////////////////////////////////////////////////
{
public:
    static void InitializationFailure(std::string logEntry)
    {
        // if someConfig->LogInitializationFailureToConsole == true
        ShowConsoleMsg(logEntry.c_str());
        
        // if someConfig->LogInitializationFailureToDisk == true
        // LogInitializationFailureToDisk(logEntry);

        // ... etc.
    }
    
    
    
    
};

#endif /* ReportLoggingEtc_h */
