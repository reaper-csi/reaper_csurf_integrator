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
    
    ActionWithIntParam(Widget* widget, vector<string> params) : Action(widget, params)
    {
        if(params.size() > 0)
            param_= atol(params[0].c_str());
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
    
    ActionWithStringParam(Widget* widget, vector<string> params) : Action(widget, params)
    {
        if(params.size() > 0)
            param_ = params[0];
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
    ReaperAction(Widget* widget, vector<string> params) : Action(widget, params)
    {
        if(params.size() > 0)
        {
            commandStr_ = params[0];
            
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
    TrackAction(Widget* widget, vector<string> params) : Action(widget, params) {}
    TrackAction(Widget* widget, vector<string> params, Navigator* navigator) : Action(widget, params, navigator) {}

public:
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetTrack())
            RequestTrackUpdate(track);
        else
             ClearWidget();
    }
    
    virtual void DoAction(double value, Widget* sender) override
    {
        if(MediaTrack* track = GetTrack())
            Action::DoAction(value, sender);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackSendAction : public TrackAction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
protected:
    int sendIndex_ = 0;
    int paramIndex_ = 0;
    bool shouldUseLocalIndex_ = false;
    
    TrackSendAction(Widget* widget, vector<string> params) : TrackAction(widget, params)
    {
        if(params.size() > 0)
        {
            if(isdigit(params[0][0])) // C++ 11 says empty strings can be queried without catastrophe :)
            {
                shouldUseLocalIndex_ = true;
                paramIndex_ = atol(params[0].c_str());
            }
        }
    }
    
    TrackSendAction(Widget* widget, vector<string> params, Navigator* navigator, int sendIndex) : TrackAction(widget, params, navigator), sendIndex_(sendIndex)
    {
        if(params.size() > 0)
        {
            if(isdigit(params[0][0])) // C++ 11 says empty strings can be queried without catastrophe :)
                paramIndex_ = atol(params[0].c_str());
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

    TrackActionWithIntParam(Widget* widget, vector<string> params) : TrackAction(widget, params)
    {
        if(params.size() > 0)
            param_= atol(params[0].c_str());
    }
    
    TrackActionWithIntParam(Widget* widget, vector<string> params, Navigator* navigator) : TrackAction(widget, params, navigator)
    {
        if(params.size() > 0)
            param_= atol(params[0].c_str());
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
    int fxIndex_ = 0;
    int fxParamIndex_ = 0;
    string fxParamDisplayName_ = "";

    FXAction(Widget* widget, vector<string> params) : TrackAction(widget, params)
    {
        if(params.size() > 0)
            fxParamIndex_ = atol(params[0].c_str());
        
        if(params.size() > 1)
            fxParamDisplayName_ = params[1];
        
        if(params.size() > 2 && params[2] != "[" && params[2] != "{")
        {
            shouldUseDisplayStyle_ = true;
            displayStyle_ = atol(params[2].c_str());
        }
    }
    
    FXAction(Widget* widget, vector<string> params, Navigator* navigator, int index) : TrackAction(widget, params, navigator)
    {
        fxIndex_ = index;
      
        if(params.size() > 0)
            fxParamIndex_ = atol(params[0].c_str());
        
        if(params.size() > 1)
            fxParamDisplayName_ = params[1];
        
        if(params.size() > 2 && params[2] != "[" && params[2] != "{")
        {
            shouldUseDisplayStyle_ = true;
            displayStyle_ = atol(params[2].c_str());
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
        
        if(MediaTrack* track = GetTrack())
            retVal = DAW::TrackFX_GetParam(track, GetSlotIndex(), fxParamIndex_, &min, &max);
        
        return retVal;
    }
    
    virtual void RequestUpdate() override
    {
        if(MediaTrack* track = GetTrack())
        {
            if(shouldUseDisplayStyle_)
                UpdateWidgetValue(displayStyle_, GetCurrentValue());
            else
                UpdateWidgetValue(GetCurrentValue());
        }
        else
             ClearWidget();
    }
};

#endif /* control_surface_action_contexts_h */
