//
//  control_surface_base_actions.h
//  reaper_csurf_integrator
//
//

#ifndef control_surface_action_contexts_h
#define control_surface_action_contexts_h

#include "control_surface_integrator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithIntParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;
    
    ActionWithIntParam(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
public:
    virtual string GetParamNumAsString() override
    {
        return to_string(param_);
    }
    
    virtual int GetParamNum() override
    {
        return param_;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionWithStringParam : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    string param_ = "";
    
    ActionWithStringParam(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params)
    {
        if(params.size() > 1)
            param_ = params[1];
    }
    
public:
    virtual string GetParamNumAsString() override
    {
        return param_;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ReaperAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
private:
    int commandId_ = 0;
    string commandStr_ = "";
    
public:
    ReaperAction(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params)
    {
        if(params.size() > 1)
        {
            commandStr_ = params[1];
            
            commandId_ =  atol(commandStr_.c_str());
            
            if(commandId_ == 0) // unsuccessful conversion to number
            {
                commandId_ = DAW::NamedCommandLookup(commandStr_.c_str()); // look up by string
                
                if(commandId_ == 0) // can't find it
                    commandId_ = 65535; // no-op
            }
        }
    }
    
    virtual string GetParamNumAsString() override
    {
        return commandStr_;
    }
    
    virtual void RequestUpdate() override
    {
        UpdateWidgetValue(DAW::GetToggleCommandState(commandId_));
    }
    
    virtual void Do(double value, Widget* sender) override
    {
        if(value != 0)
            DAW::SendCommandMessage(commandId_);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackAction : public Action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    TrackAction(string name, Widget* widget, Zone* zone, vector<string> params) : Action(name, widget, zone, params) { }

public:
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            RequestTrackUpdate(track);
        else
             GetWidget()->Clear();
    }
    
    virtual void DoAction(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
            Action::DoAction(value, sender);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int paramIndex_ = 0;
    bool shouldUseLocalIndex_ = false;
    
    TrackSendAction(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params)
    {
        if(params.size() > 1)
        {
            if(isdigit(params[1][0]))
            {
                shouldUseLocalIndex_ = true;
                paramIndex_ = atol(params[1].c_str());
            }
        }
    }

public:
    virtual string GetParamNumAsString() override
    {
        if(shouldUseLocalIndex_)
            return to_string(paramIndex_);
        else
            return to_string(GetSlotIndex());
    }
    
    virtual int GetParamNum() override
    {
        if(shouldUseLocalIndex_)
            return paramIndex_;
        else
            return GetSlotIndex();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackActionWithIntParam : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int param_ = 0;

    TrackActionWithIntParam(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params)
    {
        if(params.size() > 1)
            param_= atol(params[1].c_str());
    }
    
public:
    virtual string GetParamNumAsString() override
    {
        return to_string(param_);
    }
    
    virtual int GetParamNum() override
    {
        return param_;
    }

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FXAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int fxParamIndex_ = 0;
    string fxParamDisplayName_ = "";

    FXAction(string name, Widget* widget, Zone* zone, vector<string> params) : TrackAction(name, widget, zone, params)
    {
        if(params.size() > 1)
            fxParamIndex_ = atol(params[1].c_str());
        
        if(params.size() > 2)
            fxParamDisplayName_ = params[2];
        
        if(params.size() > 3 && params[3] != "[" && params[3] != "{")
        {
            shouldUseDisplayStyle_ = true;
            displayStyle_ = atol(params[3].c_str());
        }
    }
    
public:
    
    virtual string GetDisplayName() override { return fxParamDisplayName_; }
    
    virtual string GetParamNumAsString() override
    {
        return to_string(fxParamIndex_);
    }
    
    virtual int GetParamNum() override
    {
        return fxParamIndex_;
    }
    
    virtual string GetAlias() override
    {
        return fxParamDisplayName_;
    }

    virtual double GetCurrentValue() override
    {
        double min = 0.0;
        double max = 0.0;
        double retVal = 0.0;
        
        if(MediaTrack* track = GetWidget()->GetTrack())
            retVal = DAW::TrackFX_GetParam(track, GetSlotIndex(), fxParamIndex_, &min, &max);
        
        return retVal;
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetWidget()->GetTrack())
        {
            if(shouldUseDisplayStyle_)
                UpdateWidgetValue(displayStyle_, GetCurrentValue());
            else
                UpdateWidgetValue(GetCurrentValue());
        }
        else
             GetWidget()->Clear();
    }
};

#endif /* control_surface_action_contexts_h */
