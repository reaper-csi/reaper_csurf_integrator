//
//  control_surface_midi_widgets.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_midi_widgets_h
#define control_surface_midi_widgets_h

#include "control_surface_integrator.h"
#include "handy_reaper_functions.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PushButton_MidiWidget : public MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int reverseSense_ = 0;
    
public:
    PushButton_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : MidiWidget(GUID, surface, name, suffix, press, release)  {}
    
    PushButton_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, int reverseSense, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : MidiWidget(GUID, surface, name, suffix, press, release), reverseSense_(reverseSense) {}
    
    void SetValue(double value) override
    {
        GetRealSurface()->SendMidiMessage(value == reverseSense_ ? GetMidiReleaseMessage() : GetMidiPressMessage());
    }
    
    virtual void SetValueToZero() override
    {
        SetValue(reverseSense_ ? 1 : 0);
    }
    
    void AddToRealSurface(RealSurface* surface) override
    {
        MidiWidget::AddToRealSurface(surface);
        surface->AddWidgetToMessageMap(to_string(GetMidiPressMessage()->midi_message[0]) + to_string(GetMidiPressMessage()->midi_message[1]) + to_string(GetMidiPressMessage()->midi_message[2]), this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        GetRealSurface()->DoAction(GetGUID(), GetActionName(), GetName(), reverseSense_ ? 0 : 1);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PushButtonWithRelease_MidiWidget : public PushButton_MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    PushButtonWithRelease_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : PushButton_MidiWidget(GUID, surface, name, suffix, press, release)  {}
    
    PushButtonWithRelease_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, int reverseSense, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : PushButton_MidiWidget(GUID, surface, name, suffix,  reverseSense, press, release) {}
    
    void AddToRealSurface(RealSurface* surface) override
    {
        MidiWidget::AddToRealSurface(surface);
        surface->AddWidgetToMessageMap(to_string(GetMidiPressMessage()->midi_message[0]) + to_string(GetMidiPressMessage()->midi_message[1]) + to_string(GetMidiPressMessage()->midi_message[2]), this);
        surface->AddWidgetToMessageMap(to_string(GetMidiReleaseMessage()->midi_message[0]) + to_string(GetMidiReleaseMessage()->midi_message[1]) + to_string(GetMidiReleaseMessage()->midi_message[2]), this);
    }

    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        if(GetMidiPressMessage()->IsEqualTo(midiMessage))
            GetRealSurface()->DoAction(GetGUID(), GetActionName(), GetName(), reverseSense_ ? 0 : 1);
        else if(GetMidiReleaseMessage()->IsEqualTo(midiMessage))
            GetRealSurface()->DoAction(GetGUID(), GetActionName(), GetName(), reverseSense_ ? 1 : 0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader14Bit_MidiWidget : public MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 0.0;
    
public:
    Fader14Bit_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, double minDB, double maxDB, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : MidiWidget(GUID, surface, name, suffix, press, release), minDB_(minDB), maxDB_(maxDB) {}
    
    double GetMinDB() override { return minDB_; }
    double GetMaxDB() override { return maxDB_; }

    virtual void SetValue(double volume) override
    {
        int volint = volume * 16383.0;
        GetRealSurface()->SendMidiMessage(GetMidiPressMessage()->midi_message[0], volint&0x7f, (volint>>7)&0x7f);
    }
    
    virtual void SetValueToZero() override
    {
        SetValue(0.0);
    }
    
    void AddToRealSurface(RealSurface* surface) override
    {
        MidiWidget::AddToRealSurface(surface);
        surface->AddWidgetToMessageMap(to_string(GetMidiPressMessage()->midi_message[0]), this);
    }

    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        GetRealSurface()->DoAction(GetGUID(), GetActionName(), GetName(), int14ToNormalized(midiMessage->midi_message[2], midiMessage->midi_message[1]));        
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Fader7Bit_MidiWidget : public MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Fader7Bit_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : MidiWidget(GUID, surface, name, suffix, press, release) {}
    
    virtual void SetValue(double value) override
    {
        GetRealSurface()->SendMidiMessage(GetMidiPressMessage()->midi_message[0], GetMidiPressMessage()->midi_message[1], value * 127.0);
    }
    
    virtual void SetValue(double value, int displayMode) override
    {
        SetValue(value);
    }
    
    virtual void SetValueToZero() override
    {
        SetValue(0.0, 0x00);
    }
    
    void AddToRealSurface(RealSurface* surface) override
    {
        MidiWidget::AddToRealSurface(surface);
        surface->AddWidgetToMessageMap(to_string(GetMidiPressMessage()->midi_message[0]) + to_string(GetMidiPressMessage()->midi_message[1]), this);
    }

    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        GetRealSurface()->DoAction(GetGUID(), GetActionName(), GetName(), midiMessage->midi_message[2] / 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encoder_MidiWidget : public MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    Encoder_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : MidiWidget(GUID, surface, name, suffix, press, release) {}
    
    virtual void SetValue(double pan, int displayMode) override
    {
        unsigned char panch = pan * 127.0;
        
        int val = (1+((panch*11)>>7)) | displayMode; // display modes -- 0x00 = line (e.g. pan), 0x01 = boost/cut (e.g. eq), 0x02 = fill from right (e.g. level), 0x03 = center fill (e.g. pan width)
        
        GetRealSurface()->SendMidiMessage(GetMidiPressMessage()->midi_message[0], GetMidiPressMessage()->midi_message[1] + 0x20, val);
    }
    
    virtual void SetValueToZero() override
    {
        SetValue(0.0, 0x00);
    }
    
    void AddToRealSurface(RealSurface* surface) override
    {
        MidiWidget::AddToRealSurface(surface);
        surface->AddWidgetToMessageMap(to_string(GetMidiPressMessage()->midi_message[0]) + to_string(GetMidiPressMessage()->midi_message[1]), this);
    }
 
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        double value = (midiMessage->midi_message[2] & 0x3f) / 63.0;
        
        if (midiMessage->midi_message[2] & 0x40)
            value = -value;
        
        value += GetRealSurface()->GetActionCurrentNormalizedValue(GetGUID(), GetActionName(),  GetName());
        
        GetRealSurface()->DoAction(GetGUID(), GetActionName(), GetName(), value);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EncoderCycledAction_MidiWidget : public Encoder_MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    MIDI_event_ex_t* cycle_;
    
public:
    EncoderCycledAction_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, MIDI_event_ex_t* press, MIDI_event_ex_t* release, MIDI_event_ex_t* cycle) : Encoder_MidiWidget(GUID, surface, name, suffix, press, release), cycle_(cycle) {}
    
    void AddToRealSurface(RealSurface* surface) override
    {
        MidiWidget::AddToRealSurface(surface);
        surface->AddWidgetToMessageMap(to_string(cycle_->midi_message[0]) + to_string(cycle_->midi_message[1]) + to_string(cycle_->midi_message[2]), this);
        surface->AddWidgetToMessageMap(to_string(GetMidiPressMessage()->midi_message[0]) + to_string(GetMidiPressMessage()->midi_message[1]), this);
    }
    
    virtual void ProcessMidiMessage(const MIDI_event_ex_t* midiMessage) override
    {
        if(midiMessage->IsEqualTo(cycle_))
            GetRealSurface()->CycleAction(GetGUID(), GetActionName(), GetName());
        else
            Encoder_MidiWidget::ProcessMidiMessage(midiMessage);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VUMeter_MidiWidget : public MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    double minDB_ = 0.0;
    double maxDB_ = 0.0;
    
public:
    VUMeter_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, double minDB, double maxDB, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : MidiWidget(GUID, surface, name, suffix, press, release), minDB_(minDB), maxDB_(maxDB){}
    
    double GetMinDB() override { return minDB_; }
    double GetMaxDB() override { return maxDB_; }
    
    virtual void SetValueToZero() override
    {
        SetValue(0.0);
    }
    
    void SetValue(double value) override
    {
        GetRealSurface()->SendMidiMessage(GetMidiPressMessage()->midi_message[0], GetMidiPressMessage()->midi_message[1], value * 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GainReductionMeter_MidiWidget : public VUMeter_MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
public:
    GainReductionMeter_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, double minDB, double maxDB, MIDI_event_ex_t* press, MIDI_event_ex_t* release) : VUMeter_MidiWidget(GUID, surface, suffix, suffix, minDB, maxDB, press, release) {}

    virtual void SetValueToZero() override
    {
        SetValue(1.0);
    }
    
    void SetValue(double value) override
    {
        GetRealSurface()->SendMidiMessage(GetMidiPressMessage()->midi_message[0], GetMidiPressMessage()->midi_message[1], fabs(1.0 - value) * 127.0);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Display_MidiWidget : public MidiWidget
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
    int slotIndex_ = 0;
    
public:
    Display_MidiWidget(string GUID, RealSurface* surface, string name, string suffix, int slotIndex) : MidiWidget(GUID, surface, name, suffix, new MIDI_event_ex_t(0x00, 0x00, 0x00), new MIDI_event_ex_t(0x00, 0x00, 0x00)), slotIndex_(slotIndex) {}
    
    virtual void SetValueToZero() override
    {
        SetValue("");
    }
    
    void SetValue(string displayText) override
    {
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
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x14;
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = 0x12;
        
        midiSysExData.evt.midi_message[midiSysExData.evt.size++] = slotIndex_ * 7;
        
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
        
        GetRealSurface()->SendMidiMessage(&midiSysExData.evt);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MidiCSurf : public RealSurface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    midi_Input* midiInput_ = nullptr;
    midi_Output* midiOutput_ = nullptr;
    bool midiInMonitor_ = false;
    bool midiOutMonitor_ = false;

    void HandleMidiInput()
    {
        if(midiInput_)
        {
            DAW::SwapBufsPrecise(midiInput_);
            MIDI_eventlist* list = midiInput_->GetReadBuf();
            int bpos = 0;
            MIDI_event_t* evt;
            while ((evt = list->EnumItems(&bpos)))
                ProcessMidiMessage((MIDI_event_ex_t*)evt);
        }
    }
    
    void ProcessMidiMessage(const MIDI_event_ex_t* evt)
    {
        // At this point we don't know how much of the message comprises the key, so try all three
        if(widgetsByMessage_.count(to_string(evt->midi_message[0])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0])]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(to_string(evt->midi_message[0]) + to_string(evt->midi_message[1])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0]) + to_string(evt->midi_message[1])]->ProcessMidiMessage(evt);
        else if(widgetsByMessage_.count(to_string(evt->midi_message[0]) + to_string(evt->midi_message[1]) + to_string(evt->midi_message[2])) > 0)
            widgetsByMessage_[to_string(evt->midi_message[0]) + to_string(evt->midi_message[1]) + to_string(evt->midi_message[2])]->ProcessMidiMessage(evt);

        if(midiInMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "IN -> %s %02x  %02x  %02x \n", GetName().c_str(), evt->midi_message[0], evt->midi_message[1], evt->midi_message[2]);
            DAW::ShowConsoleMsg(buffer);
        }
    }
    
public:
    virtual ~MidiCSurf()
    {
        if (midiInput_) delete midiInput_;
        if(midiOutput_) delete midiOutput_;
    }
    
    MidiCSurf(LogicalSurface* logicalSurface, const string name, int numBankableChannels, midi_Input* midiInput, midi_Output* midiOutput, bool midiInMonitor, bool midiOutMonitor)
    : RealSurface(logicalSurface, name, numBankableChannels), midiInput_(midiInput), midiOutput_(midiOutput), midiInMonitor_(midiInMonitor), midiOutMonitor_(midiOutMonitor) {}
    
    virtual void SendMidiMessage(MIDI_event_ex_t* midiMessage) override
    {
        if(midiOutput_)
            midiOutput_->SendMsg(midiMessage, -1);
    }
    
    virtual void SendMidiMessage(int first, int second, int third) override
    {
        if(midiOutput_)
            midiOutput_->Send(first, second, third, -1);
        
        if(midiOutMonitor_)
        {
            char buffer[250];
            sprintf(buffer, "OUT -> %s %02x  %02x  %02x \n", GetName().c_str(), first, second, third);
            DAW::ShowConsoleMsg(buffer);
        }
    }
    
    virtual void RunAndUpdate() override
    {
        HandleMidiInput();
        RealSurface::UpdateWidgets();
    }
};

#endif /* control_surface_midi_widgets_h */
