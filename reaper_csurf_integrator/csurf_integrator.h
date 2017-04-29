//
//  csurf_integrator.h
//  reaper_csurf_integrator
//
//  Created by Geoff Waddington on 2017-04-28.
//

#ifndef csurf_integrator_h
#define csurf_integrator_h

#include "reaper_plugin.h"
#include "WDL/wdlstring.h"
#include "resource.h"

midi_Output *CreateThreadedMIDIOutput(midi_Output *output); // returns null on null

class CSurf_Integrator : public IReaperControlSurface
{
    WDL_String descspace;
    char configtmp[1024];
 
    midi_Input *m_midiin0;
    midi_Input *m_midiin1;
    midi_Input *m_midiin2;
    
    void OnMIDIEvent0(MIDI_event_t *evt);
    void OnMIDIEvent1(MIDI_event_t *evt);
    void OnMIDIEvent2(MIDI_event_t *evt);

public:
    CSurf_Integrator();
    virtual ~CSurf_Integrator();
    
    midi_Output *m_midiout0;
    midi_Output *m_midiout1;
    midi_Output *m_midiout2;
  
    void Run();
    
    const char *GetTypeString();
    
    const char *GetDescString();
    
    const char *GetConfigString(); // string of configuration data
};

#endif /* csurf_integrator_h */
