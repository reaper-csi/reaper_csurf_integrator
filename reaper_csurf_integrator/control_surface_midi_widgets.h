//
//  control_surface_midi_widgets.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_midi_widgets_h
#define control_surface_midi_widgets_h

#include "control_surface_integrator.h"
#include "handy_functions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PressRelease_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MIDI_event_ex_t* press_;
    MIDI_event_ex_t* release_;

public:
    virtual ~PressRelease_Midi_CSIMessageGenerator() {}
    PressRelease_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* press) : Midi_CSIMessageGenerator(widget), press_(press)
    {
        surface->AddCSIMessageGenerator(press->midi_message[0] * 0x10000 + press->midi_message[1] * 0x100 + press->midi_message[2], this);
    }
    
    PressRelease_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_CSIMessageGenerator(widget), press_(press), release_(release)
    {
        surface->AddCSIMessageGenerator(press->midi_message[0] * 0x10000 + press->midi_message[1] * 0x100 + press->midi_message[2], this);
        surface->AddCSIMessageGenerator(release->midi_message[0] * 0x10000 + release->midi_message[1] * 0x100 + release->midi_message[2], this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        widget_->DoAction(midiMessage->IsEqualTo(press_) ? 1 : 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14Bit_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader14Bit_Midi_CSIMessageGenerator() {}
    Fader14Bit_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000, this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        widget_->DoAction(int14ToNormalized(midiMessage->midi_message[2], midiMessage->midi_message[1]));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader7Bit_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader7Bit_Midi_CSIMessageGenerator() {}
    Fader7Bit_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000 + message->midi_message[1] * 0x100, this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        widget_->DoAction(midiMessage->midi_message[2] / 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encoder_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Encoder_Midi_CSIMessageGenerator() {}
    Encoder_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000 + message->midi_message[1] * 0x100, this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        double value = (midiMessage->midi_message[2] & 0x3f) / 63.0;
        
        if (midiMessage->midi_message[2] & 0x40)
            value = -value;
        
        widget_->DoRelativeAction(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TwoState_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~TwoState_Midi_FeedbackProcessor() {}
    TwoState_Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1, MIDI_event_ex_t* feedback2) : Midi_FeedbackProcessor(surface, feedback1, feedback2) { }
    
    virtual void SetValue(double value) override
    {
        if(value == 0.0)
        {
            if(midiFeedbackMessage2_)
                SendMidiMessage(midiFeedbackMessage2_->midi_message[0], midiFeedbackMessage2_->midi_message[1], midiFeedbackMessage2_->midi_message[2]);
            else if(midiFeedbackMessage1_)
                SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], 0x00);
        }
        else if(midiFeedbackMessage1_)
            SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], midiFeedbackMessage1_->midi_message[2]);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader14Bit_Midi_FeedbackProcessor() {}
    Fader14Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        int volint = value * 16383.0;
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        int volint = value * 16383.0;
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader7Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader7Bit_Midi_FeedbackProcessor() {}
    Fader7Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], value * 127.0);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], value * 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encoder_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Encoder_Midi_FeedbackProcessor() {}
    Encoder_Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        int displayMode = 0;
        
        int valueInt = value * 127;
        
        int val = (1+((valueInt*11)>>7)) | (displayMode << 4); // display modes -- 0x00 = line (e.g. pan), 0x01 = boost/cut (e.g. eq), 0x02 = fill from right (e.g. level), 0x03 = center fill (e.g. Q)
        
        //if(displayMode) // Should light up lower middle light
        //val |= 0x40;
        
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1] + 0x20, val);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        int valueInt = value * 127;
        
        int val = (1+((valueInt*11)>>7)) | (displayMode << 4); // display modes -- 0x00 = line (e.g. pan), 0x01 = boost/cut (e.g. eq), 0x02 = fill from right (e.g. level), 0x03 = center fill (e.g. Q)
        
        //if(displayMode) // Should light up lower middle light
        //val |= 0x40;
        
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1] + 0x20, val);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~VUMeter_Midi_FeedbackProcessor() {}
    VUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        double dB = VAL2DB(normalizedToVol(value)) + 2.5;
        
        double midiVal = 0;
        
        if(dB < 0)
            midiVal = pow(10.0, dB / 48) * 96;
        else
            midiVal = pow(10.0, dB / 60) * 96;
        
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], midiVal);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GainReductionMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 24.0;
    
public:
    virtual ~GainReductionMeter_Midi_FeedbackProcessor() {}
    GainReductionMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], fabs(1.0 - value) * 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class QConProXMasterVUMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 24.0;
    
public:
    virtual ~QConProXMasterVUMeter_Midi_FeedbackProcessor() {}
    QConProXMasterVUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface) : Midi_FeedbackProcessor(surface) { }
    
    virtual void SetValue(int param, double value) override
    {
        //Master Channel:
        //Master Level 1 : 0xd1, 0x0L
        //L = 0x0 – 0xD = Meter level 0% thru 100% (does not affect peak indicator)
        
        //Master Level 2 : 0xd1, 0x1L
        //L = 0x0 – 0xD = Meter level 0% thru 100% (does not affect peak indicator)
        
        int midiValue = value * 0x0d;
        SendMidiMessage(0xd1, (param << 4) | midiValue, 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCUVUMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayType_ = 0x14;
    int channelNumber_ = 0;

public:
    virtual ~MCUVUMeter_Midi_FeedbackProcessor() {}
    MCUVUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, int displayType, int channelNumber) : Midi_FeedbackProcessor(surface), displayType_(displayType), channelNumber_(channelNumber)
    {    
        // Enable meter mode for signal LED and lower display
        struct
        {
            MIDI_event_ex_t evt;
            char data[BUFSZ];
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x66;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x20;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = channelNumber_;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x05; // signal LED and lower display
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;

        SendMidiMessage(&midiSysExData.evt);
    }
    
    virtual void SetValue(double value) override
    {
        //D0 yx    : update VU meter, y=channel, x=0..d=volume, e=clip on, f=clip off
        int midiValue = value * 0x0f;
        if(midiValue > 0x0d)
            midiValue = 0x0d;
        SendMidiMessage(0xd0, (channelNumber_ << 4) | midiValue, 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCUDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int offset_ = 0;
    int displayType_ = 0x14;
    int displayRow_ = 0x12;
    int channel_ = 0;
    string lastStringSent_ = " ";

public:
    virtual ~MCUDisplay_Midi_FeedbackProcessor() {}
    MCUDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface, int displayUpperLower, int displayType, int displayRow, int channel) : Midi_FeedbackProcessor(surface), offset_(displayUpperLower * 56), displayType_(displayType), displayRow_(displayRow), channel_(channel) { }
    
    virtual void ClearCache() override
    {
        lastStringSent_ = " ";
    }
    
    virtual void SetValue(string displayText) override
    {
        if(shouldRefresh_)
        {
            double now = DAW::GetCurrentNumberOfMilliseconds();
            
            if( now > lastRefreshed_ + refreshInterval_) // time to refresh
                lastRefreshed_ = now;
            else
                return;
        }
        else
        {
            if(displayText == lastStringSent_) // no changes since last send
                return;
        }
        
        lastStringSent_ = displayText;
        
        int pad = 7;
        const char* text = displayText.c_str();
        
        struct
        {
            MIDI_event_ex_t evt;
            char data[512];
        } midiSysExData;
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x66;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayRow_;
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = channel_ * 7 + offset_;
        
        int l = strlen(text);
        if (pad < l)
            l = pad;
        if (l > 200)
            l = 200;
        
        int cnt = 0;
        while (cnt < l)
        {
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = *text++;
            cnt++;
        }
        
        while (cnt++ < pad)
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = ' ';
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

int __g_projectconfig_timemode2, __g_projectconfig_timemode;
int __g_projectconfig_measoffs;
int __g_projectconfig_timeoffs; // double

char m_mackie_lasttime[10];
int m_mackie_lasttime_mode;
DWORD m_mcu_timedisp_lastforce, m_mcu_meter_lastrun;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCU_TimeDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MCU_TimeDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface) : Midi_FeedbackProcessor(surface) {}
    
    virtual void SetValue(double value) override
    {
        
#ifndef timeGetTime
        DWORD now = GetTickCount();
#else
        DWORD now = timeGetTime();
#endif
        
        double pp=(GetPlayState()&1) ? GetPlayPosition() : GetCursorPosition();
        unsigned char bla[10];
        
        memset(bla,0,sizeof(bla));
        
        int *tmodeptr=&__g_projectconfig_timemode2;
        
        int tmode=0;
        
        if (tmodeptr && (*tmodeptr)>=0) tmode = *tmodeptr;
        else
        {
            tmodeptr=&__g_projectconfig_timemode;
            if (tmodeptr)
                tmode=*tmodeptr;
        }
        
        if (tmode==3) // seconds
        {
            double *toptr = (double*)projectconfig_var_addr(NULL,__g_projectconfig_timeoffs);
            
            if (toptr) pp+=*toptr;
            char buf[64];
            sprintf(buf,"%d %02d",(int)pp, ((int)(pp*100.0))%100);
            if (strlen(buf)>sizeof(bla)) memcpy(bla,buf+strlen(buf)-sizeof(bla),sizeof(bla));
            else
                memcpy(bla+sizeof(bla)-strlen(buf),buf,strlen(buf));
            
        }
        else if (tmode==4) // samples
        {
            char buf[128];
            format_timestr_pos(pp,buf,sizeof(buf),4);
            if (strlen(buf)>sizeof(bla)) memcpy(bla,buf+strlen(buf)-sizeof(bla),sizeof(bla));
            else
                memcpy(bla+sizeof(bla)-strlen(buf),buf,strlen(buf));
        }
        else if (tmode==5) // frames
        {
            char buf[128];
            format_timestr_pos(pp,buf,sizeof(buf),5);
            char *p=buf;
            char *op=buf;
            int ccnt=0;
            while (*p)
            {
                if (*p == ':')
                {
                    ccnt++;
                    if (ccnt!=3)
                    {
                        p++;
                        continue;
                    }
                    *p=' ';
                }
                
                *op++=*p++;
            }
            *op=0;
            if (strlen(buf)>sizeof(bla)) memcpy(bla,buf+strlen(buf)-sizeof(bla),sizeof(bla));
            else
                memcpy(bla+sizeof(bla)-strlen(buf),buf,strlen(buf));
        }
        else if (tmode>0)
        {
            int num_measures=0;
            double beats=TimeMap2_timeToBeats(NULL,pp,&num_measures,NULL,NULL,NULL)+ 0.000000000001;
            double nbeats = floor(beats);
            
            beats -= nbeats;
            
            int fracbeats = (int) (1000.0 * beats);
            
            int *measptr = (int*)projectconfig_var_addr(NULL,__g_projectconfig_measoffs);
            int nm=num_measures+1+(measptr ? *measptr : 0);
            if (nm >= 100) bla[0]='0'+(nm/100)%10;//bars hund
            if (nm >= 10) bla[1]='0'+(nm/10)%10;//barstens
            bla[2]='0'+(nm)%10;//bars
            
            int nb=(int)nbeats+1;
            if (nb >= 10) bla[3]='0'+(nb/10)%10;//beats tens
            bla[4]='0'+(nb)%10;//beats
            
            
            bla[7]='0' + (fracbeats/100)%10;
            bla[8]='0' + (fracbeats/10)%10;
            bla[9]='0' + (fracbeats%10); // frames
        }
        else
        {
            double *toptr = (double*)projectconfig_var_addr(NULL,__g_projectconfig_timeoffs);
            if (toptr) pp+=(*toptr);
            
            int ipp=(int)pp;
            int fr=(int)((pp-ipp)*1000.0);
            
            if (ipp >= 360000) bla[0]='0'+(ipp/360000)%10;//hours hundreds
            if (ipp >= 36000) bla[1]='0'+(ipp/36000)%10;//hours tens
            if (ipp >= 3600) bla[2]='0'+(ipp/3600)%10;//hours
            
            bla[3]='0'+(ipp/600)%6;//min tens
            bla[4]='0'+(ipp/60)%10;//min
            bla[5]='0'+(ipp/10)%6;//sec tens
            bla[6]='0'+(ipp%10);//sec
            bla[7]='0' + (fr/100)%10;
            bla[8]='0' + (fr/10)%10;
            bla[9]='0' + (fr%10); // frames
        }
        
        if (m_mackie_lasttime_mode != tmode)
        {
            m_mackie_lasttime_mode=tmode;
            SendMidiMessage(0x90, 0x71, tmode==5?0x7F:0); // set smpte light
            SendMidiMessage(0x90, 0x72, m_mackie_lasttime_mode>0 && tmode<3?0x7F:0); // set beats light
            
            // Blank display on mode change
            for (int x = 0 ; x < sizeof(bla) ; x++)
                SendMidiMessage(0xB0,0x40+x,0x20);
            
        }
        
        if (memcmp(m_mackie_lasttime,bla,sizeof(bla)))
        {
            bool force=false;
            if (now > m_mcu_timedisp_lastforce)
            {
                m_mcu_timedisp_lastforce=now+2000;
                force=true;
            }
            
            for (int x =0 ; x < sizeof(bla) ; x ++)
            {
                int idx=sizeof(bla)-x-1;
                if (bla[idx]!=m_mackie_lasttime[idx]||force)
                {
                    SendMidiMessage(0xB0,0x40+x,bla[idx]);
                    m_mackie_lasttime[idx]=bla[idx];
                }
            }
        }
    }
};
#endif /* control_surface_midi_widgets_h */
