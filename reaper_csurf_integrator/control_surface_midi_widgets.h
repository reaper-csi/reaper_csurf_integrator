//
//  control_surface_midi_widgets.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_midi_widgets_h
#define control_surface_midi_widgets_h

#include "control_surface_integrator.h"
#include "handy_reaper_functions.h"
#include "WDL/ptrlist.h"

extern Manager* TheManager;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Latch_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Latch_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]) + to_string(midiPressMessage_->midi_message[2]), this);
        surface->AddWidgetToMessageMap(to_string(midiReleaseMessage_->midi_message[0]) + to_string(midiReleaseMessage_->midi_message[1]) + to_string(midiReleaseMessage_->midi_message[2]), this);
    }
    
    void SetValue(int displayMode, double value) override
    {
        if(value == 0.0)
            SendMidiMessage(midiReleaseMessage_->midi_message[0], midiReleaseMessage_->midi_message[1], midiReleaseMessage_->midi_message[2]);
        else
            SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], midiPressMessage_->midi_message[2]);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        ClearCache();
        TheManager->DoAction(this, midiMessage->midi_message[2] == 0 ? 0.0 : 1.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14Bit_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 0.0;
    //db = 20*log10(X/0xFFFF)
    
public:
    Fader14Bit_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, double minDB, double maxDB, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release), minDB_(minDB), maxDB_(maxDB)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]), this);
    }
    
    virtual void SetValue(int displayMode, double volume) override
    {
        int volint = volume * 16383.0;
        SendMidiMessage(midiPressMessage_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        TheManager->DoAction(this, int14ToNormalized(midiMessage->midi_message[2], midiMessage->midi_message[1]));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader7Bit_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Fader7Bit_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]), this);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], value * 127.0);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        TheManager->DoAction(this, midiMessage->midi_message[2] / 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 0.0;
    
public:
    VUMeter_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, double minDB, double maxDB, MIDI_event_ex_t* press) : Midi_Widget(surface, role, name, wantsFeedback, press), minDB_(minDB), maxDB_(maxDB) {}
    
    void SetValue(int displayMode, double value) override
    {
        int midiValue = 0;
        
        if(value < minDB_)
            midiValue = 0;
        else if(value > maxDB_)
            midiValue = 127;
        else
            midiValue = ((value - minDB_) / (maxDB_ - minDB_)) * 127;
        
        SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], midiValue);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GainReductionMeter_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 0.0;
    
public:
    GainReductionMeter_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, double minDB, double maxDB, MIDI_event_ex_t* press) : Midi_Widget(surface, role, name, wantsFeedback, press), minDB_(minDB), maxDB_(maxDB) {}

    void SetValue(int displayMode, double value) override
    {
        SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], fabs(1.0 - value) * 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCUVUMeter_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int channel_ = 0;
    
public:
    MCUVUMeter_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, int channel) : Midi_Widget(surface, role, name, wantsFeedback), channel_(channel) {}
    
    void SetValue(int param, double value) override
    {
        //D0 yx    : update VU meter, y=channel, x=0..d=volume, e=clip on, f=clip off
        
        int midiValue = 0;
        
        double minDB = TheManager->GetVUMinDB();
        double maxDB = TheManager->GetVUMaxDB();
        
        if(value < minDB)
            midiValue = 0x00;
        else if(value > maxDB)
            midiValue = 0x0d;
        else
            midiValue = ((value - minDB) / (maxDB - minDB)) * 0x0d;
        
        SendMidiMessage(0xd0, (channel_ << 4) | midiValue, 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCUDisplay_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int offset_ = 0;
    int displayType_ = 0x14;
    int displayRow_ = 0x12;
    int channel_ = 0;
    string lastStringSent_ = "";
    
public:
    MCUDisplay_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, int displayUpperLower, int displayType, int displayRow, int channel) : Midi_Widget(surface, role, name, wantsFeedback), offset_(displayUpperLower * 56), displayType_(displayType), displayRow_(displayRow), channel_(channel) {}
    
    void SetValue(string displayText) override
    {
        if( displayText == lastStringSent_)
            return;
        
        lastStringSent_ = displayText;
        
        if(channel_ > 7) // GAW TDB -- this is a hack to prevent Fader 9 (Master) on MCU from displaying on lower row of channel 1
            return;
        
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
class MCU_TimeDisplay_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MCU_TimeDisplay_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback) : Midi_Widget(surface, role, name, wantsFeedback, new MIDI_event_ex_t(0x00, 0x00, 0x00), new MIDI_event_ex_t(0x00, 0x00, 0x00)) {}
    
    void SetValue(int mode, double value) override
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







/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PushButton_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PushButton_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]) + to_string(midiPressMessage_->midi_message[2]), this);
    }
    
    void SetValue(int displayMode, double value) override
    {
        if(value != 0)
            SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], midiPressMessage_->midi_message[2]);
        else
            SendMidiMessage(midiReleaseMessage_->midi_message[0], midiReleaseMessage_->midi_message[1], midiReleaseMessage_->midi_message[2]);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        if(midiPressMessage_->IsEqualTo(midiMessage))
            TheManager->DoAction(this, 1.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ToggleButton_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    ToggleButton_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]) + to_string(midiPressMessage_->midi_message[2]), this);
    }
    
    void SetValue(int displayMode, double value) override
    {
        if(value != 0)
            SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], midiPressMessage_->midi_message[2]);
        else
            SendMidiMessage(midiReleaseMessage_->midi_message[0], midiReleaseMessage_->midi_message[1], midiReleaseMessage_->midi_message[2]);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        if(midiPressMessage_->IsEqualTo(midiMessage))
            TheManager->DoAction(this, ! lastMessageSent_->midi_message[2]);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PushButtonWithResendOnRelease_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PushButtonWithResendOnRelease_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]) + to_string(midiPressMessage_->midi_message[2]), this);
        surface->AddWidgetToMessageMap(to_string(midiReleaseMessage_->midi_message[0]) + to_string(midiReleaseMessage_->midi_message[1]) + to_string(midiReleaseMessage_->midi_message[2]), this);
    }
    
    void SetValue(int displayMode, double value) override
    {
        if(value != 0)
            SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1], midiPressMessage_->midi_message[2]);
        else
            SendMidiMessage(midiReleaseMessage_->midi_message[0], midiReleaseMessage_->midi_message[1], midiReleaseMessage_->midi_message[2]);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        if(midiPressMessage_->IsEqualTo(midiMessage))
            TheManager->DoAction(this, 1.0);
        else if(midiReleaseMessage_->IsEqualTo(midiMessage))
            ClearCache();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PushButtonWithRelease_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PushButtonWithRelease_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]) + to_string(midiPressMessage_->midi_message[2]), this);
        surface->AddWidgetToMessageMap(to_string(midiReleaseMessage_->midi_message[0]) + to_string(midiReleaseMessage_->midi_message[1]) + to_string(midiReleaseMessage_->midi_message[2]), this);
    }
    
    void SetValue(int displayMode, double value) override
    {
        SendMidiMessage(midiPressMessage_->midi_message[0],midiPressMessage_->midi_message[1], value);
    }

    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        if(midiPressMessage_->IsEqualTo(midiMessage))
            TheManager->DoAction(this, 1.0);
        else if(midiReleaseMessage_->IsEqualTo(midiMessage))
            TheManager->DoAction(this, 0.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14BitDB_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 0.0;
    int zeroDBFaderPosition_ = 0;
    
public:
    Fader14BitDB_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, double minDB, double maxDB, MIDI_event_ex_t* press, MIDI_event_ex_t* release, int byte1ZeroDB, int byte2ZeroDB) : Midi_Widget(surface, role, name, wantsFeedback, press, release), minDB_(minDB), maxDB_(maxDB), zeroDBFaderPosition_(byte1ZeroDB | (byte2ZeroDB << 7))

    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]), this);
    }
    
    virtual void SetValue(int displayMode, double volume) override
    {
        int volint = 0;
        
        if(volume < minDB_)
            volint = 0;
        else if(volume > maxDB_)
            volint = 16383.0;
        else
        {
            if(volume >= 0.0)
                volint = volume / maxDB_ * (16383 - zeroDBFaderPosition_) + zeroDBFaderPosition_;
            else
                volint = zeroDBFaderPosition_ - (volume / minDB_ * zeroDBFaderPosition_);
        }

        SendMidiMessage(midiPressMessage_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        double faderPosition = midiMessage->midi_message[1] | (midiMessage->midi_message[2] << 7);
        
        double dbValue = 0.0;
        
        if(faderPosition >= zeroDBFaderPosition_)
            dbValue = ((faderPosition - zeroDBFaderPosition_) / (16383.0 - zeroDBFaderPosition_)) * maxDB_;
        else
            dbValue = ((zeroDBFaderPosition_ - faderPosition) / zeroDBFaderPosition_) * minDB_;

        TheManager->DoAction(this, dbValue);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encoder_Midi_Widget : public Midi_Widget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double lastNormalizedValue_ = 0.0;
    
public:
    Encoder_Midi_Widget(Midi_RealSurface* surface, string role, string name, bool wantsFeedback, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_Widget(surface, role, name, wantsFeedback, press, release)
    {
        surface->AddWidgetToMessageMap(to_string(midiPressMessage_->midi_message[0]) + to_string(midiPressMessage_->midi_message[1]), this);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        lastNormalizedValue_ = value;
        
        int valueInt = value * 127;
        
        int val = (1+((valueInt*11)>>7)) | (displayMode << 4); // display modes -- 0x00 = line (e.g. pan), 0x01 = boost/cut (e.g. eq), 0x02 = fill from right (e.g. level), 0x03 = center fill (e.g. Q)
        
        SendMidiMessage(midiPressMessage_->midi_message[0], midiPressMessage_->midi_message[1] + 0x20, val);
    }

    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        double value = (midiMessage->midi_message[2] & 0x3f) / 63.0;
        
        if (midiMessage->midi_message[2] & 0x40)
            value = -value;

        TheManager->DoAction(this, value + lastNormalizedValue_);
    }
};

#endif /* control_surface_midi_widgets_h */
