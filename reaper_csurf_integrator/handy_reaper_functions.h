//
//  handy_reaper_functions.h
//  reaper_csurf_integrator
//
//

#ifndef handy_reaper_functions_h
#define handy_reaper_functions_h

#include "reaper_plugin_functions.h"
#include "WDL/db2val.h"

//slope = (output_end - output_start) / (input_end - input_start)
//output = output_start + slope * (input - input_start)


static double ucharToNormalized(unsigned char val)
{
    return val/127.0;
}

static unsigned char normalizedToUchar(double val)
{
    return val * 127.0;
}

static double int14ToNormalized(unsigned char msb, unsigned char lsb)
{
    int val=lsb | (msb<<7);
    return ((double)val)/16383.0;
}

static int normalizedToInt14(double val)
{
    return val * 16383.0;
}

static double normalizedToVol(double val)
{
    double pos=val*1000.0;
    pos=SLIDER2DB(pos);
    return DB2VAL(pos);
}

static double volToNormalized(double vol)
{
    double d=(DB2SLIDER(VAL2DB(vol))/1000.0);
    if (d<0.0)d=0.0;
    else if (d>1.0)d=1.0;
    
    return d;
}

static double normalizedToPan(double val)
{
    //slope = (pan_end - pan_start) / (norm_end - norm_start)
    //output = pan_start + slope * (val - norm_start)
    
    //double slope = (1.0 - (-1.0)) / (1.0 - 0.0)
    //double output = -1.0 + slope * (val - 0.0)
    
    return 2.0 * val - 1.0;
}

static double panToNormalized(double val)
{
    //slope = (norm_end - norm_start) / (pan_end - pan_start)
    //output = norm_start + slope * (val - pan_start)
    
    //double slope = (1.0 - 0.0)) / (1.0 - (-1.0)
    //double output = 1.0 + slope * (val - (-1.0))
    
    return 0.5 * (val + 1.0);
}

#endif /* handy_reaper_functions_h */
