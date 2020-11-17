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
        widget_->DoPressAction(midiMessage->IsEqualTo(press_) ? 1 : 0);
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
        widget_->DoTouch(midiMessage->IsEqualTo(press_) ? 1 : 0);
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
        widget_->DoPressAction(1); // Doesn't matter what value was sent, just do it
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
            widget_->DoRelativeAction(accelerationIndicesForIncrement_[val], delta);
        
        else if(accelerationIndicesForDecrement_.count(val) > 0)
            widget_->DoRelativeAction(accelerationIndicesForDecrement_[val], delta);
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
            widget_->DoRelativeAction(accelerationIndicesForIncrement_[val], 0.001);
        
        else if(accelerationIndicesForDecrement_.count(val) > 0)
            widget_->DoRelativeAction(accelerationIndicesForDecrement_[val], -0.001);
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

        widget_->DoRelativeAction(delta);
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
        
        widget_->DoRelativeAction(delta);
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
        
        widget_->DoRelativeAction(delta);
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
    
    virtual void ForceValue(double value) override
    {
        mustForce_ = true;
        SetValue(value);
        mustForce_ = false;
    }
    
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
    
    virtual void ForceRGBValue(int r, int g, int b) override
    {
        lastR = r;
        lastG = g;
        lastB = b;
        
        struct
        {
            MIDI_event_ex_t evt;
            char data[64];
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
    
    virtual void SetRGBValue(int r, int g, int b) override
    {
        if(r == lastR && g == lastG && b == lastB)
            return;
        
        ForceRGBValue(r, g, b);
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

    virtual void SetRGBValue(int r, int g, int b) override
    {
        if(r == lastR_ && g == lastG_ && b == lastB_)
            return;
        
        ForceRGBValue(r, g, b);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14Bit_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    virtual ~Fader14Bit_Midi_FeedbackProcessor() {}
    Fader14Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void ForceValue(double value) override
    {
        mustForce_ = true;
        SetValue(value);
        mustForce_ = false;
    }
    
    virtual void SetValue(double value) override
    {
        int volint = value * 16383.0;
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void ForceValue(int displayMode, double value) override
    {
        mustForce_ = true;
        SetValue(displayMode, value);
        mustForce_ = false;
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
    Fader7Bit_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void ForceValue(double value) override
    {
        mustForce_ = true;
        SetValue(value);
        mustForce_ = false;
    }

    virtual void SetValue(double value) override
    {
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], value * 127.0);
    }
    
    virtual void ForceValue(int displayMode, double value) override
    {
        mustForce_ = true;
        SetValue(displayMode, value);
        mustForce_ = false;
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
    Encoder_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void ForceValue(double value) override
    {
        mustForce_ = true;
        SetValue(value);
        mustForce_ = false;
    }

    virtual void SetValue(double value) override
    {
        int displayMode = 0;
        
        int valueInt = value * 127;
        
        int val = (1+((valueInt*11)>>7)) | (displayMode << 4); // display modes -- 0x00 = line (e.g. pan), 0x01 = boost/cut (e.g. eq), 0x02 = fill from right (e.g. level), 0x03 = center fill (e.g. Q)
        
        // GAW TBD -- fix this -- something like if (! display && value == centre)
        //if(displayMode) // Should light up lower middle light
        //val |= 0x40;
        
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1] + 0x20, val);
    }
    
    virtual void ForceValue(int displayMode, double value) override
    {
        mustForce_ = true;
        SetValue(displayMode, value);
        mustForce_ = false;
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
    VUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void ForceValue(double value) override
    {
        mustForce_ = true;
        SetValue(value);
        mustForce_ = false;
    }

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
    GainReductionMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, MIDI_event_ex_t* feedback1) : Midi_FeedbackProcessor(surface, widget, feedback1) { }
    
    virtual void ForceValue(double value) override
    {
        mustForce_ = true;
        SetValue(value);
        mustForce_ = false;
    }
    
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
    QConProXMasterVUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget) : Midi_FeedbackProcessor(surface, widget) { }
    
    virtual void ForceValue(int param, double value) override
    {
        mustForce_ = true;
        SetValue(param, value);
        mustForce_ = false;
    }
    
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
    int lastMidiValue_ = 0;
    bool isClipOn_ = false;

public:
    virtual ~MCUVUMeter_Midi_FeedbackProcessor() {}
    MCUVUMeter_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int displayType, int channelNumber) : Midi_FeedbackProcessor(surface, widget), displayType_(displayType), channelNumber_(channelNumber) {}
    
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
        if(shouldRefresh_)
        {
            double now = DAW::GetCurrentNumberOfMilliseconds();
            
            if( now > lastRefreshed_ + refreshInterval_) // time to refresh
                lastRefreshed_ = now;
            else
                return;
        }
        else if( ! mustForce_ )
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FPDisplay_Midi_FeedbackProcessor : public Midi_FeedbackProcessor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int displayType_ = 0x02;
    int channel_ = 0;
    string lastStringSent_ = " ";
    
public:
    virtual ~FPDisplay_Midi_FeedbackProcessor() {}
    FPDisplay_Midi_FeedbackProcessor(Midi_ControlSurface* surface, Widget* widget, int displayType, int channel) : Midi_FeedbackProcessor(surface, widget), displayType_(displayType), channel_(channel) { }
    
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
        else if( ! mustForce_)
        {
            if(displayText == lastStringSent_) // no changes since last send
                return;
        }
        
        lastStringSent_ = displayText;

        const char* text = displayText.c_str();
    
        struct
        {
            MIDI_event_ex_t evt;
            char data[512];
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
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x00;                    // yy line number 0-3
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x0000000;               // zz alignment flag 0000000=centre, see manual for other setups.
        
        int length = strlen(text);
        
        if (length > 200) length = 200;
        
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
        if(shouldRefresh_)
        {
            double now = DAW::GetCurrentNumberOfMilliseconds();
            
            if( now > lastRefreshed_ + refreshInterval_) // time to refresh
                lastRefreshed_ = now;
            else
                return;
        }
        else if( ! mustForce_)
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



// Color maps are stored in Blue Green Red format
uint8_t colorMap7[128][3] = { {0, 0, 0},    // 0
    {255, 0, 0},    // 1 - Blue
    {255, 21, 0},    // 2 - Blue (Green Rising)
    {255, 34, 0},
    {255, 46, 0},
    {255, 59, 0},
    {255, 68, 0},
    {255, 80, 0},
    {255, 93, 0},
    {255, 106, 0},
    {255, 119, 0},
    {255, 127, 0},
    {255, 140, 0},
    {255, 153, 0},
    {255, 165, 0},
    {255, 178, 0},
    {255, 191, 0},
    {255, 199, 0},
    {255, 212, 0},
    {255, 225, 0},
    {255, 238, 0},
    {255, 250, 0},    // 21 - End of Blue's Reign
    
    {250, 255, 0}, // 22 - Green (Blue Fading)
    {237, 255, 0},
    {225, 255, 0},
    {212, 255, 0},
    {199, 255, 0},
    {191, 255, 0},
    {178, 255, 0},
    {165, 255, 0},
    {153, 255, 0},
    {140, 255, 0},
    {127, 255, 0},
    {119, 255, 0},
    {106, 255, 0},
    {93, 255, 0},
    {80, 255, 0},
    {67, 255, 0},
    {59, 255, 0},
    {46, 255, 0},
    {33, 255, 0},
    {21, 255, 0},
    {8, 255, 0},
    {0, 255, 0},    // 43 - Green
    
    {0, 255, 12},    // 44 - Green/ Red Rising
    {0, 255, 25},
    {0, 255, 38},
    {0, 255, 51},
    {0, 255, 63},
    {0, 255, 72},
    {0, 255, 84},
    {0, 255, 97},
    {0, 255, 110},
    {0, 255, 123},
    {0, 255, 131},
    {0, 255, 144},
    {0, 255, 157},
    {0, 255, 170},
    {0, 255, 182},
    {0, 255, 191},
    {0, 255, 203},
    {0, 255, 216},
    {0, 255, 229},
    {0, 255, 242},
    {0, 255, 255},    // 64 - Green + Red (Yellow)
    
    {0, 246, 255},    // 65 - Red, Green Fading
    {0, 233, 255},
    {0, 220, 255},
    {0, 208, 255},
    {0, 195, 255},
    {0, 187, 255},
    {0, 174, 255},
    {0, 161, 255},
    {0, 148, 255},
    {0, 135, 255},
    {0, 127, 255},
    {0, 114, 255},
    {0, 102, 255},
    {0, 89, 255},
    {0, 76, 255},
    {0, 63, 255},
    {0, 55, 255},
    {0, 42, 255},
    {0, 29, 255},
    {0, 16, 255},
    {0, 4, 255},    // 85 - End Red/Green Fading
    
    {4, 0, 255},    // 86 - Red/ Blue Rising
    {16, 0, 255},
    {29, 0, 255},
    {42, 0, 255},
    {55, 0, 255},
    {63, 0, 255},
    {76, 0, 255},
    {89, 0, 255},
    {102, 0, 255},
    {114, 0, 255},
    {127, 0, 255},
    {135, 0, 255},
    {148, 0, 255},
    {161, 0, 255},
    {174, 0, 255},
    {186, 0, 255},
    {195, 0, 255},
    {208, 0, 255},
    {221, 0, 255},
    {233, 0, 255},
    {246, 0, 255},
    {255, 0, 255},    // 107 - Blue + Red
    
    {255, 0, 242},    // 108 - Blue/ Red Fading
    {255, 0, 229},
    {255, 0, 216},
    {255, 0, 204},
    {255, 0, 191},
    {255, 0, 182},
    {255, 0, 169},
    {255, 0, 157},
    {255, 0, 144},
    {255, 0, 131},
    {255, 0, 123},
    {255, 0, 110},
    {255, 0, 97},
    {255, 0, 85},
    {255, 0, 72},
    {255, 0, 63},
    {255, 0, 50},
    {255, 0, 38},
    {255, 0, 25},    // 126 - Blue-ish
    {225, 240, 240}    // 127 - White ?
};

int GetColorIntFromRGB(int r, int g, int b)
{
    if(b == 0 && g == 0 && r == 0)
        return 0;
    else if(b > 224 && g > 239 && r > 239)
        return 127;
    else if(b == 255 && r == 0)
    {
        for(int i = 1; i < 22; i++)
            if(g > colorMap7[i - 1][1] && g <= colorMap7[i][1])
                return i;
    }
    else if(g == 255 && r == 0)
    {
        for(int i = 22; i < 44; i++)
            if(b < colorMap7[i - 1][0] && b >= colorMap7[i][0])
                return i;
    }
    else if(b == 0 && g == 255)
    {
        for(int i = 44; i < 65; i++)
            if(r > colorMap7[i - 1][2] && r <= colorMap7[i][2])
                return i;
    }
    else if(b == 0 && r == 255)
    {
        for(int i = 65; i < 86; i++)
            if(g < colorMap7[i - 1][1] && g >= colorMap7[i][1])
                return i;
    }
    else if(g == 0 && r == 255)
    {
        for(int i = 86; i < 108; i++)
            if(b > colorMap7[i - 1][0] && b <= colorMap7[i][0])
                return i;
    }
    else if(b == 255 && g == 0)
    {
        for(int i = 108; i < 127; i++)
            if(r < colorMap7[i - 1][2] && r >= colorMap7[i][2])
                return i;
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
        
        int rgbVal = GetColorIntFromRGB(r, g, b);
        
        SendMidiMessage(midiFeedbackMessage1_->midi_message[0], midiFeedbackMessage1_->midi_message[1], rgbVal);
    }

    virtual void SetRGBValue(int r, int g, int b) override
    {
        if(r == lastR && g == lastG && b == lastB)
            return;
        
        ForceRGBValue(r, g, b);
    }
};

#endif /* control_surface_midi_widgets_h */
