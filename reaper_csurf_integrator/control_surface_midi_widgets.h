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
// CSIMessageGenerators
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
        widget_->QueueAction(midiMessage->IsEqualTo(press_) ? 1 : 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Touch_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MIDI_event_ex_t* press_;
    MIDI_event_ex_t* release_;
    
public:
    virtual ~Touch_Midi_CSIMessageGenerator() {}
    
    Touch_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : Midi_CSIMessageGenerator(widget), press_(press), release_(release)
    {
        surface->AddCSIMessageGenerator(press->midi_message[0] * 0x10000 + press->midi_message[1] * 0x100 + press->midi_message[2], this);
        surface->AddCSIMessageGenerator(release->midi_message[0] * 0x10000 + release->midi_message[1] * 0x100 + release->midi_message[2], this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        widget_->QueueTouch(midiMessage->IsEqualTo(press_) ? 1 : 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Toggle_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MIDI_event_ex_t* press_;
    
public:
    virtual ~Toggle_Midi_CSIMessageGenerator() {}
    
    Toggle_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* press) : Midi_CSIMessageGenerator(widget), press_(press)
    {
        surface->AddCSIMessageGenerator(press->midi_message[0] * 0x10000 + press->midi_message[1] * 0x100 + press->midi_message[2], this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        widget_->Toggle();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class AnyPress_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MIDI_event_ex_t* press_;
    MIDI_event_ex_t* release_;
    
public:
    virtual ~AnyPress_Midi_CSIMessageGenerator() {}
    AnyPress_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* press) : Midi_CSIMessageGenerator(widget), press_(press)
    {
        surface->AddCSIMessageGenerator(press->midi_message[0] * 0x10000 + press->midi_message[1] * 0x100, this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        widget_->QueueAction(1); // Doesn't matter what value was sent, just do it
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
        widget_->QueueAction(int14ToNormalized(midiMessage->midi_message[2], midiMessage->midi_message[1]));
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
        widget_->QueueAction(midiMessage->midi_message[2] / 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class AcceleratedEncoder_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<int, int> accelerationIndicesForIncrement_;
    map<int, int> accelerationIndicesForDecrement_;

public:
    virtual ~AcceleratedEncoder_Midi_CSIMessageGenerator() {}
    AcceleratedEncoder_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message, vector<string> params) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000 + message->midi_message[1] * 0x100, this);
        
        auto openSquareBrace = find(params.begin(), params.end(), "[");
        auto closeCurlyBrace = find(params.begin(), params.end(), "]");
        
        if(openSquareBrace != params.end() && closeCurlyBrace != params.end())
        {
            vector<int> incValues;
            vector<int> decValues;

            bool inDec = false;
            
            for(auto it = openSquareBrace + 1; it != closeCurlyBrace; ++it)
            {
                string strVal = *(it);
                
                if(strVal == "<")
                    inDec = true;
                else if(strVal == ">")
                    inDec = false;
                else if(regex_match(strVal, regex("[0-9A-Fa-f]+[-][0-9A-Fa-f]+")))
                {
                    istringstream range(strVal);
                    vector<string> range_tokens;
                    string range_token;
                    
                    while (getline(range, range_token, '-'))
                        range_tokens.push_back(range_token);
                    
                    if(range_tokens.size() == 2)
                    {
                        int firstVal = strtol(range_tokens[0].c_str(), nullptr, 16);;
                        int lastVal = strtol(range_tokens[1].c_str(), nullptr, 16);

                        if(firstVal < lastVal)
                        {
                            if(inDec == false)
                            {
                                for(int i = firstVal; i <= lastVal; i++)
                                    incValues.push_back(i);
                            }
                            else
                            {
                                for(int i = firstVal; i <= lastVal; i++)
                                    decValues.push_back(i);
                            }
                        }
                        else
                        {
                            if(inDec == false)
                            {
                                for(int i = firstVal; i >= lastVal; i--)
                                    incValues.push_back(i);
                            }
                            else
                            {
                                for(int i = firstVal; i >= lastVal; i--)
                                    decValues.push_back(i);
                            }
                        }
                    }
                }
                else if(inDec == false)
                    incValues.push_back(strtol(strVal.c_str(), nullptr, 16));
                else
                    decValues.push_back(strtol(strVal.c_str(), nullptr, 16));
            }
            
            if(incValues.size() > 0)
            {
                if(incValues.size() == 1)
                    accelerationIndicesForIncrement_[incValues[0]] = 0;
                else
                    for(int i = 0; i < incValues.size(); i++)
                        accelerationIndicesForIncrement_[incValues[i]] = i;
            }
            
            if(decValues.size() > 0)
            {
                if(decValues.size() == 1)
                    accelerationIndicesForDecrement_[decValues[0]] = 0;
                else
                    for(int i = 0; i < decValues.size(); i++)
                        accelerationIndicesForDecrement_[decValues[i]] = i;
            }
        }
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        int val = midiMessage->midi_message[2];

        double delta = (midiMessage->midi_message[2] & 0x3f) / 63.0;
        
        if (midiMessage->midi_message[2] & 0x40)
            delta = -delta;
        
        delta = delta / 2.0;

        if(accelerationIndicesForIncrement_.count(val) > 0)
            widget_->QueueRelativeAction(accelerationIndicesForIncrement_[val], delta);
        
        else if(accelerationIndicesForDecrement_.count(val) > 0)
            widget_->QueueRelativeAction(accelerationIndicesForDecrement_[val], delta);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MFT_AcceleratedEncoder_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<int, int> accelerationIndicesForIncrement_;
    map<int, int> accelerationIndicesForDecrement_;
    
public:
    virtual ~MFT_AcceleratedEncoder_Midi_CSIMessageGenerator() {}
    MFT_AcceleratedEncoder_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message, vector<string> params) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000 + message->midi_message[1] * 0x100, this);
    
        accelerationIndicesForDecrement_[0x3f] = 0;
        accelerationIndicesForDecrement_[0x3e] = 1;
        accelerationIndicesForDecrement_[0x3d] = 2;
        accelerationIndicesForDecrement_[0x3c] = 3;
        accelerationIndicesForDecrement_[0x3b] = 4;
        accelerationIndicesForDecrement_[0x3a] = 5;
        accelerationIndicesForDecrement_[0x39] = 6;
        accelerationIndicesForDecrement_[0x38] = 7;
        accelerationIndicesForDecrement_[0x36] = 8;
        accelerationIndicesForDecrement_[0x33] = 9;
        accelerationIndicesForDecrement_[0x2f] = 10;

        accelerationIndicesForIncrement_[0x41] = 0;
        accelerationIndicesForIncrement_[0x42] = 1;
        accelerationIndicesForIncrement_[0x43] = 2;
        accelerationIndicesForIncrement_[0x44] = 3;
        accelerationIndicesForIncrement_[0x45] = 4;
        accelerationIndicesForIncrement_[0x46] = 5;
        accelerationIndicesForIncrement_[0x47] = 6;
        accelerationIndicesForIncrement_[0x48] = 7;
        accelerationIndicesForIncrement_[0x4a] = 8;
        accelerationIndicesForIncrement_[0x4d] = 9;
        accelerationIndicesForIncrement_[0x51] = 10;
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        int val = midiMessage->midi_message[2];
        
        if(accelerationIndicesForIncrement_.count(val) > 0)
            widget_->QueueRelativeAction(accelerationIndicesForIncrement_[val], 0.001);
        
        else if(accelerationIndicesForDecrement_.count(val) > 0)
            widget_->QueueRelativeAction(accelerationIndicesForDecrement_[val], -0.001);
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
        double delta = (midiMessage->midi_message[2] & 0x3f) / 63.0;
        
        if (midiMessage->midi_message[2] & 0x40)
            delta = -delta;
        
        delta = delta / 2.0;

        widget_->QueueRelativeAction(delta);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EncoderPlain_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~EncoderPlain_Midi_CSIMessageGenerator() {}
    EncoderPlain_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000 + message->midi_message[1] * 0x100, this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        double delta = 1.0 / 64.0;
        
        if (midiMessage->midi_message[2] & 0x40)
            delta = -delta;
        
        widget_->QueueRelativeAction(delta);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EncoderPlainReverse_Midi_CSIMessageGenerator : public Midi_CSIMessageGenerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~EncoderPlainReverse_Midi_CSIMessageGenerator() {}
    EncoderPlainReverse_Midi_CSIMessageGenerator(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* message) : Midi_CSIMessageGenerator(widget)
    {
        surface->AddCSIMessageGenerator(message->midi_message[0] * 0x10000 + message->midi_message[1] * 0x100, this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        double delta = 1.0 / 64.0;
        
        if (! (midiMessage->midi_message[2] & 0x40))
            delta = -delta;
        
        widget_->QueueRelativeAction(delta);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FeedbackProcessors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TwoState_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~TwoState_Midi_FeedbackProcessor() {}
    TwoState_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1, MIDI_event_ex_t* feedback2) : Midi_FeedbackProcessor(surface, widget, feedback1, feedback2) { }
    
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

    virtual void ForceValue(double value) override
    {
        if(value == 0.0)
        {
            if(midiFeedbackMessage2_)
                ForceMidiMessage(midiFeedbackMessage2_->midi_message[0], midiFeedbackMessage2_->midi_message[1], midiFeedbackMessage2_->midi_message[2]);
            else if(midiFeedbackMessage1_)
                ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], 0x00);
        }
        else if(midiFeedbackMessage1_)
            ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], midiFeedbackMessage1_->midi_message[2]);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NovationLaunchpadMiniRGB7Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int lastR = 0;
    int lastG = 0;
    int lastB = 0;
    
public:
    virtual ~NovationLaunchpadMiniRGB7Bit_Midi_FeedbackProcessor() {}
    NovationLaunchpadMiniRGB7Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetRGBValue(int r, int g, int b) override
    {
        if(r == lastR && g == lastG && b == lastB)
            return;
        
        ForceRGBValue(r, g, b);
    }

    virtual void ForceRGBValue(int r, int g, int b) override
    {
        lastR = r;
        lastG = g;
        lastB = b;
        
        struct
        {
            MIDI_event_ex_t evt;
            char data[64]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset = 0;
        midiSysExData.evt.size = 0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x20;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x29;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x0d;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x03;
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x03;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = midiFeedbackMessage1_->midi_message[1] ;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = r / 2; // only 127 bit max for this device
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = g / 2;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = b / 2;
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FaderportRGB7Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int lastR_ = 0;
    int lastG_ = 0;
    int lastB_ = 0;
    
public:
    virtual ~FaderportRGB7Bit_Midi_FeedbackProcessor() {}
    FaderportRGB7Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetRGBValue(int r, int g, int b) override
    {
        if(r == lastR_ && g == lastG_ && b == lastB_)
            return;
        
        ForceRGBValue(r, g, b);
    }

    virtual void ForceRGBValue(int r, int g, int b) override
    {
        lastR_ = r;
        lastG_ = g;
        lastB_ = b;
        
        SendMidiMessage(0x90, midiFeedbackMessage1_->midi_message[1], 0x7f);
        SendMidiMessage(0x91, midiFeedbackMessage1_->midi_message[1], r / 2);  // only 127 bit allowed in Midi byte 3
        SendMidiMessage(0x92, midiFeedbackMessage1_->midi_message[1], g / 2);
        SendMidiMessage(0x93, midiFeedbackMessage1_->midi_message[1], b / 2);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader14Bit_Midi_FeedbackProcessor() {}
    Fader14Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        int volint = value * 16383.0;
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void ForceValue(double value) override
    {
        int volint = value * 16383.0;
        ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        SetValue(value);
    }
    
    virtual void ForceValue(int displayMode, double value) override
    {
        ForceValue(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader7Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader7Bit_Midi_FeedbackProcessor() {}
    Fader7Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], value * 127.0);
    }
    
    virtual void ForceValue(double value) override
    {
        ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], value * 127.0);
    }
    
    virtual void SetValue(int displayMode, double value) override
    {
        SetValue(value);
    }

    virtual void ForceValue(int displayMode, double value) override
    {
        ForceValue(value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encoder_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Encoder_Midi_FeedbackProcessor() {}
    Encoder_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        SetValue(0, value);
    }

    virtual void ForceValue(double value) override
    {
        ForceValue(0, value);
    }

    virtual void SetValue(int displayMode, double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1] + 0x20, GetMidiValue(displayMode, value));
    }

    virtual void ForceValue(int displayMode, double value) override
    {
        ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1] + 0x20, GetMidiValue(displayMode, value));
    }
    
    int GetMidiValue(int displayMode, double value)
    {
        int valueInt = value * 127;
        
        int val = (1+((valueInt*11)>>7)) | (displayMode << 4); // display modes -- 0x00 = line (e.g. pan), 0x01 = boost/cut (e.g. eq), 0x02 = fill from right (e.g. level), 0x03 = center fill (e.g. Q)
        
        //if(displayMode) // Should light up lower middle light
        //val |= 0x40;

        return val;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~VUMeter_Midi_FeedbackProcessor() {}
    VUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], GetMidiValue(value));
    }

    virtual void ForceValue(double value) override
    {
        ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], GetMidiValue(value));
    }
    
    double GetMidiValue(double value)
    {
        double dB = VAL2DB(normalizedToVol(value)) + 2.5;
        
        double midiVal = 0;
        
        if(dB < 0)
            midiVal = pow(10.0, dB / 48) * 96;
        else
            midiVal = pow(10.0, dB / 60) * 96;

        return midiVal;
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
    GainReductionMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void SetValue(double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], fabs(1.0 - value) * 127.0);
    }

    virtual void ForceValue(double value) override
    {
        ForceMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], fabs(1.0 - value) * 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class QConProXMasterVUMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 24.0;
    int param_ = 0;
    
public:
    virtual ~QConProXMasterVUMeter_Midi_FeedbackProcessor() {}
    QConProXMasterVUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int param) : Midi_FeedbackProcessor(surface, widget), param_(param) { }
    
    virtual void SetValue(double value) override
    {
        //Master Channel:
        //Master Level 1 : 0xd1, 0x0L
        //L = 0x0 – 0xD = Meter level 0% thru 100% (does not affect peak indicator)
        
        //Master Level 2 : 0xd1, 0x1L
        //L = 0x0 – 0xD = Meter level 0% thru 100% (does not affect peak indicator)
        
        int midiValue = value * 0x0f;
        
        if(midiValue > 0x0d)
            midiValue = 0x0d;
        
        SendMidiMessage(0xd1, (param_ << 4) | midiValue, 0);
    }

    virtual void ForceValue(double value) override
    {
        //Master Channel:
        //Master Level 1 : 0xd1, 0x0L
        //L = 0x0 – 0xD = Meter level 0% thru 100% (does not affect peak indicator)
        
        //Master Level 2 : 0xd1, 0x1L
        //L = 0x0 – 0xD = Meter level 0% thru 100% (does not affect peak indicator)
        
        int midiValue = value * 0x0f;
        
        if(midiValue > 0x0d)
            midiValue = 0x0d;
        
        ForceMidiMessage(0xd1, (param_ << 4) | midiValue, 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCUVUMeter_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayType_ = 0x14;
    int channelNumber_ = 0;
    int lastMidiValue_ = 0;
    bool isClipOn_ = false;

public:
    virtual ~MCUVUMeter_Midi_FeedbackProcessor() {}
    MCUVUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int displayType, int channelNumber) : Midi_FeedbackProcessor(surface, widget), displayType_(displayType), channelNumber_(channelNumber) {}
    
    virtual void SetValue(double value) override
    {
        SendMidiMessage(0xd0, (channelNumber_ << 4) | GetMidiValue(value), 0);
    }

    virtual void ForceValue(double value) override
    {
        ForceMidiMessage(0xd0, (channelNumber_ << 4) | GetMidiValue(value), 0);
    }
    
    int GetMidiValue(double value)
    {
        //D0 yx    : update VU meter, y=channel, x=0..d=volume, e=clip on, f=clip off
        int midiValue = value * 0x0f;
        if(midiValue > 0x0d)
            midiValue = 0x0d;

        return midiValue;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCUDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int offset_ = 0;
    int displayType_ = 0x10;
    int displayRow_ = 0x12;
    int channel_ = 0;
    string lastStringSent_ = "";

public:
    virtual ~MCUDisplay_Midi_FeedbackProcessor() {}
    MCUDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int displayUpperLower, int displayType, int displayRow, int channel) : Midi_FeedbackProcessor(surface, widget), offset_(displayUpperLower * 56), displayType_(displayType), displayRow_(displayRow), channel_(channel) { }
    
    virtual void ClearCache() override
    {
        lastStringSent_ = " ";
    }
    
    virtual void SetValue(string displayText) override
    {
        if(displayText != lastStringSent_) // changes since last send
            ForceValue(displayText);
    }

    virtual void ForceValue(string displayText) override
    {
        lastStringSent_ = displayText;
        
        if(displayText == "" || displayText == "-150.00")
            displayText = "       ";

        int pad = 7;
        const char* text = displayText.c_str();
        
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_Text_MaxCharactersDictionary
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<int, map<int, int>> maxCharacters;

public:
    SCE24_Text_MaxCharactersDictionary()
    {
        maxCharacters[0][0] = 16;
        maxCharacters[0][1] = 16;
        maxCharacters[0][2] = 16;
        maxCharacters[0][3] = 9;
        maxCharacters[0][4] = 0;
        maxCharacters[0][5] = 0;
        maxCharacters[0][6] = 0;
        maxCharacters[0][7] = 0;
        maxCharacters[0][8] = 0;
        maxCharacters[0][9] = 0;
        
        maxCharacters[1][0] = 16;
        maxCharacters[1][1] = 16;
        maxCharacters[1][2] = 7;
        maxCharacters[1][3] = 0;
        maxCharacters[1][4] = 3;
        maxCharacters[1][5] = 0;
        maxCharacters[1][6] = 3;
        maxCharacters[1][7] = 19;
        maxCharacters[1][8] = 19;
        maxCharacters[1][9] = 0;
        
        maxCharacters[2][0] = 16;
        maxCharacters[2][1] = 16;
        maxCharacters[2][2] = 7;
        maxCharacters[2][3] = 5;
        maxCharacters[2][4] = 5;
        maxCharacters[2][5] = 5;
        maxCharacters[2][6] = 0;
        maxCharacters[2][7] = 10;
        maxCharacters[2][8] = 0;
        maxCharacters[2][9] = 0;
        
        maxCharacters[3][0] = 16;
        maxCharacters[3][1] = 16;
        maxCharacters[3][2] = 16;
        maxCharacters[3][3] = 16;
        maxCharacters[3][4] = 16;
        maxCharacters[3][5] = 0;
        maxCharacters[3][6] = 0;
        maxCharacters[3][7] = 0;
        maxCharacters[3][8] = 0;
        maxCharacters[3][9] = 0;
        
        maxCharacters[4][0] = 16;
        maxCharacters[4][1] = 16;
        maxCharacters[4][2] = 0;
        maxCharacters[4][3] = 14;
        maxCharacters[4][4] = 14;
        maxCharacters[4][5] = 0;
        maxCharacters[4][6] = 0;
        maxCharacters[4][7] = 0;
        maxCharacters[4][8] = 0;
        maxCharacters[4][9] = 0;
        
        maxCharacters[5][0] = 16;
        maxCharacters[5][1] = 0;
        maxCharacters[5][2] = 14;
        maxCharacters[5][3] = 14;
        maxCharacters[5][4] = 14;
        maxCharacters[5][5] = 0;
        maxCharacters[5][6] = 0;
        maxCharacters[5][7] = 0;
        maxCharacters[5][8] = 0;
        maxCharacters[5][9] = 0;
        
        maxCharacters[6][0] = 16;
        maxCharacters[6][1] = 0;
        maxCharacters[6][2] = 14;
        maxCharacters[6][3] = 14;
        maxCharacters[6][4] = 14;
        maxCharacters[6][5] = 14;
        maxCharacters[6][6] = 0;
        maxCharacters[6][7] = 0;
        maxCharacters[6][8] = 0;
        maxCharacters[6][9] = 0;
        
        maxCharacters[7][0] = 16;
        maxCharacters[7][1] = 11;
        maxCharacters[7][2] = 7;
        maxCharacters[7][3] = 11;
        maxCharacters[7][4] = 7;
        maxCharacters[7][5] = 11;
        maxCharacters[7][6] = 7;
        maxCharacters[7][7] = 11;
        maxCharacters[7][8] = 7;
        maxCharacters[7][9] = 0;
        
        maxCharacters[8][0] = 12;
        maxCharacters[8][1] = 0;
        maxCharacters[8][2] = 0;
        maxCharacters[8][3] = 0;
        maxCharacters[8][4] = 0;
        maxCharacters[8][5] = 0;
        maxCharacters[8][6] = 0;
        maxCharacters[8][7] = 0;
        maxCharacters[8][8] = 0;
        maxCharacters[8][9] = 0;
        
        maxCharacters[9][0] = 14;
        maxCharacters[9][1] = 14;
        maxCharacters[9][2] = 0;
        maxCharacters[9][3] = 0;
        maxCharacters[9][4] = 0;
        maxCharacters[9][5] = 0;
        maxCharacters[9][6] = 0;
        maxCharacters[9][7] = 0;
        maxCharacters[9][8] = 0;
        maxCharacters[9][9] = 0;
    }
    
    int GetMaxCharacters(int displayType, int itemNumber)
    {
        displayType--;
        displayType = displayType < 0 ? 0 : displayType;
        displayType = displayType > 7 ? 7 : displayType;

        itemNumber--;
        itemNumber = itemNumber < 0 ? 0 : itemNumber;
        itemNumber = itemNumber > 9 ? 9 : itemNumber;

        return maxCharacters[displayType][itemNumber];
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_OLEDButtonText_MaxCharactersDictionary
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    map<int, int> maxCharacters;
    
public:
    SCE24_OLEDButtonText_MaxCharactersDictionary()
    {
        maxCharacters[0] = 6;
        maxCharacters[1] = 7;
        maxCharacters[2] = 7;
        maxCharacters[3] = 9;
    };
    
    int GetMaxCharacters(int displayType)
    {
        displayType--;
        displayType = displayType < 0 ? 0 : displayType;
        displayType = displayType > 3 ? 3 : displayType;
        
        return maxCharacters[displayType];
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int cellNumber_ = 01;
    int displayType_ = 0;
    int itemNumber_ = 00;
    int itemStyle_ = 01;
    
    string text_ = "";
    int maxCharacters_ = 0;
    int colorBreakpoint_ = 0;
    
    rgb_color foregroundColor_ = { 0x7f, 0x7f, 0x7f };
    rgb_color backgroundColor_ { 0, 0, 0 };
    
    rgb_color currentColor_ = { 0, 0, 0 } ;

    SCE24_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber) : Midi_FeedbackProcessor(surface, widget), cellNumber_(cellNumber) {}
    
    SCE24_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber, int itemNumber) : Midi_FeedbackProcessor(surface, widget), cellNumber_(cellNumber), itemNumber_(itemNumber) {}

    virtual void SetCurrentColor(double value) override
    {
        currentColor_ = value == 0 ? backgroundColor_ : foregroundColor_;
        
        ForceValue();
    }
    
    virtual void SetProperties(vector<vector<string>> properties) override
    {
        for(auto property : properties)
        {
            if(property.size() == 0)
                continue;

            if(property[0] == "RingStyle" && property.size() > 1)
            {
                itemStyle_ = stoi(property[1]);
                ForceValue();
            }
            else if(property[0] == "BarStyle" && property.size() > 2)
            {
                displayType_ = stoi(property[1]);
                itemStyle_ = stoi(property[2]);
                ForceValue();
            }
            else if((property[0] == "RingColor" || property[0] == "BarColor") && property.size() > 4)
            {
                colorBreakpoint_ = stoi(property[1]);
                foregroundColor_.r = stoi(property[2]);
                foregroundColor_.g = stoi(property[3]);
                foregroundColor_.b = stoi(property[4]);
                ForceValue();
            }
            else if((property[0] == "Color" || property[0] == "ColorOn") && property.size() > 4)
            {
                displayType_ = stoi(property[1]);
                foregroundColor_.r = stoi(property[2]);
                foregroundColor_.g = stoi(property[3]);
                foregroundColor_.b = stoi(property[4]);
                ForceValue();
            }
            else if((property[0] == "BackgroundColor" || property[0] == "ColorOff") && property.size() > 4)
            {
                displayType_ = stoi(property[1]);
                backgroundColor_.r = stoi(property[2]);
                backgroundColor_.g = stoi(property[3]);
                backgroundColor_.b = stoi(property[4]);
                
                int oldMaxCharacters = maxCharacters_;
                maxCharacters_ = 0;
                ForceValue();
                maxCharacters_ = oldMaxCharacters;
            }
            else if(property[0] == "Text" && property.size() > 8)
            {
                displayType_ = stoi(property[1]);
                text_ = property[2];
                
                backgroundColor_.r = stoi(property[6]);
                backgroundColor_.g = stoi(property[7]);
                backgroundColor_.b = stoi(property[8]);

                maxCharacters_ = 0;
                ForceValue();
                
                foregroundColor_.r = stoi(property[3]);
                foregroundColor_.g = stoi(property[4]);
                foregroundColor_.b = stoi(property[5]);
                
                maxCharacters_ = GetMaxCharacters();
            }
            else if(property[0] == "NoBlink" && property.size() == 1)
            {
                itemStyle_ = 01;
            }
            else if(property[0] == "Blink" && property.size() == 1)
            {
                itemStyle_ = 02;
            }
        }
        
        ForceValue();
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_Text_Midi_FeedbackProcessor : public SCE24_Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    SCE24_Text_MaxCharactersDictionary maxChars;
    
public:
    virtual ~SCE24_Text_Midi_FeedbackProcessor() {}
    SCE24_Text_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber, int itemNumber) : SCE24_Midi_FeedbackProcessor(surface, widget, cellNumber, itemNumber) { }
    
    virtual int GetMaxCharacters() override
    {
        return maxChars.GetMaxCharacters(displayType_, itemNumber_);
    }
   
    virtual void ClearCache() override
    {
        lastStringValue_ = " ";
    }
    
    virtual void SetValue(string displayText) override
    {
        if(displayText != lastStringValue_) // changes since last send
            ForceValue(displayText);
    }

    virtual void ForceValue(string value) override
    {
        lastStringValue_ = value;
        text_ = value;
        ForceValue();
    }
    
    virtual void ForceValue() override
    {
        string displayText = text_;

        if(displayText == "")
        {
            for(int i = 0; i < maxCharacters_; i++)
                displayText += " ";
        }
        
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x38;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Controller type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = cellNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_;    // from .zon
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Text Type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // not used
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = maxCharacters_;  // from .zon, 00 == background, no text will be sent
        
        if(maxCharacters_ != 0)
        {
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.r / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.g / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.b / 2;
        }
        else
        {
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = backgroundColor_.r / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = backgroundColor_.g / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = backgroundColor_.b / 2;
        }
        
        if(maxCharacters_ != 0)
        {
            const char* text = displayText.c_str();
            
            int textLength = strlen(text);
            
            if(textLength > maxCharacters_)
                textLength = maxCharacters_;
            
            int cnt = 0;
            
            while (cnt < textLength)
            {
                midiSysExData.evt.midi_message[midiSysExData.evt.size++] = *text++;
                cnt++;
            }
        }

        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_Bar_Midi_FeedbackProcessor : public SCE24_Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~SCE24_Bar_Midi_FeedbackProcessor() {}
    SCE24_Bar_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber, int itemNumber) : SCE24_Midi_FeedbackProcessor(surface, widget, cellNumber, itemNumber) { }
       
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
    }
    
    virtual void SetValue(double value) override
    {
        if(int(value * 100.00) != (int)lastDoubleValue_) // changes since last send
        {
            lastDoubleValue_ = int(value * 100.00);
            SendMidiMessage(0xB0 | itemNumber_, cellNumber_, (int)lastDoubleValue_);
        }
    }
    
    virtual void ForceValue() override
    {
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x38;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Controller type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = cellNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_;    // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x03;            // Bar
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemStyle_;      // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = colorBreakpoint_;// from app
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.r / 2;    // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.g / 2;    // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.b / 2;    // from app
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_Ring_Midi_FeedbackProcessor : public SCE24_Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~SCE24_Ring_Midi_FeedbackProcessor() {}
    SCE24_Ring_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber) : SCE24_Midi_FeedbackProcessor(surface, widget, cellNumber) { }
       
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
    }
    
    virtual void SetValue(double value) override
    {
        if(int(value * 100.00) != (int)lastDoubleValue_) // changes since last send
        {
            lastDoubleValue_ = int(value * 100.00);
            SendMidiMessage(0xB0, cellNumber_, (int)lastDoubleValue_);
        }
    }
    
    virtual void ForceValue() override
    {
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x38;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Controller type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = cellNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x7f;            // Ring
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemStyle_;      // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = colorBreakpoint_;// from app
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.r / 2;    // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.g / 2;    // from app
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.b / 2;    // from app
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_OLEDButton_Midi_FeedbackProcessor : public SCE24_Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    SCE24_OLEDButtonText_MaxCharactersDictionary maxChars;

public:
    virtual ~SCE24_OLEDButton_Midi_FeedbackProcessor() {}
    SCE24_OLEDButton_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber, int itemNumber) : SCE24_Midi_FeedbackProcessor(surface, widget, cellNumber, itemNumber) {}
    
    virtual int GetMaxCharacters() override
    {
        return maxChars.GetMaxCharacters(displayType_);
    }
    
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0;
        lastStringValue_ = "";
    }
    
    virtual void ForceClear() override
    {
        ForceValue(0.0);
    }
    
    virtual void SetValue(double value) override
    {
        if(value != lastDoubleValue_) // changes since last send
            ForceValue(value);
    }
    
    void ForceValue(double value) override
    {
        lastDoubleValue_ = value;
        SetCurrentColor(value); // This will cause a Force()
    }
    
    virtual void SetValue(string value) override
    {
        if(value != lastStringValue_) // changes since last send
            ForceValue(value);
    }
    
    void ForceValue(string value) override
    {
        lastStringValue_ = value;
        text_ = value;
        ForceValue();
    }

    void ForceValue() override
    {
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x38;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Controller type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = cellNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_;    // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemNumber_;     // from .zon
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // ItemType
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemStyle_;      // from .zon
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = maxCharacters_;  // from .zon, 00 == background, no text will be sent
        
        if(maxCharacters_ != 0)
        {
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.r / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.g / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = foregroundColor_.b / 2;
        }
        else
        {
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = backgroundColor_.r / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = backgroundColor_.g / 2;
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = backgroundColor_.b / 2;
        }
        
        if(maxCharacters_ != 0)
        {
            const char* text = text_.c_str();
            
            int textLength = strlen(text);
            
            if(textLength > maxCharacters_)
                textLength = maxCharacters_;
            
            int cnt = 0;
            
            while (cnt < textLength)
            {
                midiSysExData.evt.midi_message[midiSysExData.evt.size++] = *text++;
                cnt++;
            }
        }
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_LEDButton_Midi_FeedbackProcessor : public SCE24_Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~SCE24_LEDButton_Midi_FeedbackProcessor() {}
    SCE24_LEDButton_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber) : SCE24_Midi_FeedbackProcessor(surface, widget, cellNumber) { }
    
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
    }
    
    virtual void SetValue(double value) override
    {
        if(value != lastDoubleValue_) // changes since last send
            ForceValue(value);
    }
    
    virtual void ForceValue(double value) override
    {
        lastDoubleValue_ = value;
        SetCurrentColor(value);  // This will cause a Force()
    }
    
    virtual void ForceValue() override
    {
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x38;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Controller type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = cellNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = itemStyle_;      // from .zon
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = currentColor_.r / 2;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = currentColor_.g / 2;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = currentColor_.b / 2;

        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SCE24_Background_Midi_FeedbackProcessor : public  SCE24_Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~SCE24_Background_Midi_FeedbackProcessor() {}
    SCE24_Background_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int cellNumber) : SCE24_Midi_FeedbackProcessor(surface, widget, cellNumber) { }
    
    virtual void ClearCache() override
    {
        lastDoubleValue_ = 0.0;
    }
    
    virtual void SetValue(double value) override
    {
        if(value != lastDoubleValue_) // changes since last send
            ForceValue(value);
    }
    
    virtual void ForceValue(double value) override
    {
        lastDoubleValue_ = value;
        SetCurrentColor(value);  // This will cause a Force()
    }
    
    virtual void ForceValue() override
    {
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        } midiSysExData;
        
        midiSysExData.evt.frame_offset=0;
        midiSysExData.evt.size=0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x02;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x38;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;            // Controller type
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = cellNumber_;     // from .mst
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_;    // from .zon
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // indicates background colour
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;            // unused
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = currentColor_.r / 2;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = currentColor_.g / 2;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = currentColor_.b / 2;

        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FPDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayType_ = 0x02;
    int displayRow_ = 0x00;
    int channel_ = 0;
    string lastStringSent_ = " ";
    
public:
    virtual ~FPDisplay_Midi_FeedbackProcessor() {}
    FPDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int displayType, int channel, int displayRow) : Midi_FeedbackProcessor(surface, widget), displayType_(displayType), channel_(channel), displayRow_(displayRow) { }
    
    virtual void ClearCache() override
    {
        lastStringSent_ = " ";
    }
    
    virtual void SetValue(string displayText) override
    {
        if(displayText != lastStringSent_) // changes since last send
            ForceValue(displayText);
    }
    
    virtual void ForceValue(string displayText) override
    {
        lastStringSent_ = displayText;

        if(displayText == "")
            displayText = "                            ";
        
        const char* text = displayText.c_str();
    
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
        }
        midiSysExData;
        
        midiSysExData.evt.frame_offset = 0;
        midiSysExData.evt.size = 0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF0;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x01;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x06;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayType_; // Faderport8=0x02, Faderport16=0x16
        
        // <SysExHdr> 12, xx, yy, zz, tx,tx,tx,... F7
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x12;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = channel_;                // xx channel_ id
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = displayRow_;             // yy line number 0-3
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x0000000;               // zz alignment flag 0000000=centre, see manual for other setups.
        
        int length = strlen(text);
        
        if (length > 30)
            length = 30;
        
        int count = 0;
        
        while (count < length)
        {
            midiSysExData.evt.midi_message[midiSysExData.evt.size++] = *text++;                // tx text in ASCII format
            count++;
        }
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0xF7;
        
        SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class QConLiteDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int offset_ = 0;
    int displayType_ = 0x14;
    int displayRow_ = 0x12;
    int channel_ = 0;
    string lastStringSent_ = "";
    
public:
    virtual ~QConLiteDisplay_Midi_FeedbackProcessor() {}
    QConLiteDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int displayUpperLower, int displayType, int displayRow, int channel) : Midi_FeedbackProcessor(surface, widget), offset_(displayUpperLower * 28), displayType_(displayType), displayRow_(displayRow), channel_(channel) { }
    
    virtual void ClearCache() override
    {
        lastStringSent_ = " ";
    }
    
    virtual void SetValue(string displayText) override
    {
        if(displayText != lastStringSent_) // changes since last send
            ForceValue(displayText);
    }
    
    virtual void ForceValue(string displayText) override
    {
        lastStringSent_ = displayText;
        
        if(displayText == "")
            displayText = "       ";
        
        int pad = 7;
        const char* text = displayText.c_str();
        
        struct
        {
            MIDI_event_ex_t evt;
            char data[512]{};
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

char m_mackie_lasttime[10];
int m_mackie_lasttime_mode;
DWORD m_mcu_timedisp_lastforce, m_mcu_meter_lastrun;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MCU_TimeDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    MCU_TimeDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget) : Midi_FeedbackProcessor(surface, widget) {}
    
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
        
        int *tmodeptr = TheManager->GetTimeMode2Ptr();
        
        int tmode=0;
        
        if (tmodeptr && (*tmodeptr)>=0) tmode = *tmodeptr;
        else
        {
            tmodeptr = TheManager->GetTimeModePtr();
            if (tmodeptr)
                tmode=*tmodeptr;
        }
        
        if (tmode==3) // seconds
        {
            double *toptr = TheManager->GetTimeOffsPtr();
            
            if (toptr) pp+=*toptr;
            char buf[64];
            snprintf(buf, sizeof(buf),"%d %02d",(int)pp, ((int)(pp*100.0))%100);
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
            
            int *measptr = TheManager->GetMeasOffsPtr();
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
            double *toptr = TheManager->GetTimeOffsPtr();
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


int GetColorIntFromRGB(int r, int g, int b)
{
    if (b == 0 && g == 0 && r == 0)
        return 0;
    else if (b > 224 && g > 239 && r > 239)
        return 127;
    else if (b == 255 && r == 0)
    {
        return g / 12.16 + 1;
    }
    else if (g == 255 && r == 0)
    {
        return ((254 + 1) - b) / 12.16 + 22;
    }
    else if (b == 0 && g == 255)
    {
        return r / 12.16 + 43;
    }
    else if (b == 0 && r == 255)
    {
        return((254 + 1) - g) / 12.16 + 64;
    }
    else if (g == 0 && r == 255)
    {
        return b / 12.16 + 85;
    }
    else if (b == 255 && g == 0)
    {
        return((254 + 1) - r) / 12.16 + 106;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MFT_RGB_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int lastR = 0;
    int lastG = 0;
    int lastB = 0;
    
public:
    virtual ~MFT_RGB_Midi_FeedbackProcessor() {}
    MFT_RGB_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void ForceRGBValue(int r, int g, int b) override
    {
        lastR = r;
        lastG = g;
        lastB = b;
        
        if((r == 177 || r == 181) && g == 31) // this sets the different MFT modes
            SendMidiMessage(r, g, b);
        else
            SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], GetColorIntFromRGB(r, g, b));
    }

    virtual void SetRGBValue(int r, int g, int b) override
    {
        if(r == lastR && g == lastG && b == lastB)
            return;
        
        ForceRGBValue(r, g, b);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GP_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    vector<string> tokens_;
    vector<MIDI_event_ex_t> onMessages_;
    vector<MIDI_event_ex_t> offMessages_;
    vector<MIDI_event_ex_t> lastMessages_;

    static int strToHex(string valueStr)
    {
        return strtol(valueStr.c_str(), nullptr, 16);
    }

public:
    virtual ~GP_Midi_FeedbackProcessor() {}
    GP_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, vector<string> tokens) : Midi_FeedbackProcessor(surface, widget), tokens_(tokens) {

        if (tokens_[0] != "FB_GP_Midi")
            return;

        vector<string> params = tokens_;
        params.erase(params.begin());
        ParseData(params);
    }

    void ParseData(vector<string> params)
    {
        if (params.size() == 0)
            return;

        for (vector<string>::iterator it = params.begin(); it != params.end(); ) {

            if (!regex_match(*it, regex("([0-9A-Fa-f]{2})"))) {                 // Make sure 2 digit hex value
                if (!(*it == ",")) {
                    if (regex_match(*it, regex("([0-9A-Fa-f]{2},)"))) {          // Handle misplaced comma

                        it = params.insert(it, (*it).substr(0, 2));
                        ++it;
                        *it = ",";

                    }
                    else if (regex_match(*it, regex("(,[0-9A-Fa-f]{2})"))) {     // Handle misplaced comma
                        it = params.insert(it, ",");
                        ++it;
                        *it = (*it).substr(1, 2);
                    }
                    else
                    {
                        it = params.erase(it);                                  // Invalid Element so remove it
                        continue;
                    }
                }
            }
            ++it;
        }

        offMessages_.clear();
        onMessages_.clear();

        bool isOffMessage = true;
        for (int i = 0; i < params.size(); ) {
            if (params[i] == ",") {
                isOffMessage = false;
                i++;
            }
            else
            {
                if (i + 2 < params.size()) {                         //Make sure we have a 3 byte Midi token

                    if (isOffMessage) {
                        offMessages_.push_back(MIDI_event_ex_t(strToHex(params[i]), strToHex(params[i + 1]), strToHex(params[i + 2])));
                    }
                    else {
                        onMessages_.push_back(MIDI_event_ex_t(strToHex(params[i]), strToHex(params[i + 1]), strToHex(params[i + 2])));
                    }
                }
                i = i + 3;
            }
        }
    }

    virtual void SetValue(double value) override
    {
        if (value == 0.0)
        {
            bool eql = false;
            if (offMessages_.size() == lastMessages_.size()) {
                eql = std::equal(offMessages_.begin(), offMessages_.end(), lastMessages_.begin());
            }
            if (!eql || lastMessages_.size() == 0) {
                ForceValue(value);
            }
        }
        else {
            bool eql = false;
            if (onMessages_.size() == lastMessages_.size()) {
                eql = std::equal(onMessages_.begin(), onMessages_.end(), lastMessages_.begin());
            }
            if (!eql || lastMessages_.size() == 0) {
                ForceValue(value);
            }
        }
    }



    virtual void ForceValue(double value) override
    {
        if (value == 0.0)
        {
            for (int i = 0; i < offMessages_.size(); i++) {
                SendMidiMessage(offMessages_[i].midi_message[0], offMessages_[i].midi_message[1], offMessages_[i].midi_message[2]);
            }
            lastMessages_ = offMessages_;
        }
        else {
            for (int i = 0; i < onMessages_.size(); i++) {
                SendMidiMessage(onMessages_[i].midi_message[0], onMessages_[i].midi_message[1], onMessages_[i].midi_message[2]);
            }
            lastMessages_ = onMessages_;
        }
    }

    virtual void SetDataValue(vector<string> params) override {
        ParseData(params);
    }
};

bool operator==(const MIDI_event_ex_t& lhs, const MIDI_event_ex_t& rhs) {
    if (lhs.midi_message[0] == rhs.midi_message[0] && lhs.midi_message[1] == rhs.midi_message[1] && lhs.midi_message[2] == rhs.midi_message[2] && lhs.midi_message[3] == rhs.midi_message[3])
        return true;
    return false;
}

bool operator!=(const MIDI_event_ex_t& lhs, const MIDI_event_ex_t& rhs) {
    if (lhs == rhs)
        return false;
    return true;
}

#endif /* control_surface_midi_widgets_h */


