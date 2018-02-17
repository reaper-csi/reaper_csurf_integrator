//
//  control_surface_integrator.cpp
//  reaper_control_surface_integrator
//
//

#include "control_surface_integrator.h"
#include "control_surface_midi_widgets.h"
#include "control_surface_Reaper_actions.h"
#include "control_surface_manager_actions.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reaper Actions available for mapping, this list will get added to substantially over time
////////////////////////////////////////////////////////////////////////////////////////////////////////
Action* ActionFor(string name, Layout* layout)
{
    if(name == "SetShowFXWindows")  return new SetShowFXWindows_Action(layout);
    else if(name == "Rewind")  return new Rewind_Action(layout);
    else if(name == "FastForward")  return new FastForward_Action(layout);
    else if(name == "Stop")  return new Stop_Action(layout);
    else if(name == "Play")  return new Play_Action(layout);
    else if(name == "Record")  return new Record_Action(layout);
    else if(name == "NextMap")  return new NextMap_Action(layout);
    else if(name == "ImmobilizeSelectedTracks")  return new ImmobilizeSelectedTracks_Action(layout);
    else if(name == "MobilizeSelectedTracks")  return new MobilizeSelectedTracks_Action(layout);
    else if(name == "Shift")  return new Shift_Action(layout);
    else if(name == "Option")  return new Option_Action(layout);
    else if(name == "Control")  return new Control_Action(layout);
    else if(name == "Alt")  return new Alt_Action(layout);
    else if(name == "LatchedZoom")  return new LatchedZoom_Action(layout);
    else if(name == "LatchedScrub")  return new LatchedScrub_Action(layout);
    else if(name == "Cycled")  return new Cycled_Action(layout);
    else if(name == "CycleTimeline")  return new CycleTimeline_Action(layout);
    
    return new Action(layout);
}

Action* ActionFor(string name, Layout* layout, string param)
{
    if(name == "TrackBank")  return new TrackBank_Action(layout, param);
    else if(name == "TrackAutoMode")  return new TrackAutoMode_Action(layout, param);
    else if(name == "GlobalAutoMode")  return new GlobalAutoMode_Action(layout, param);
    else if(name == "Reaper")  return new Reaper_Action(layout, param);
    
    return new Action(layout);
}

Action* ActionFor(string name, Layout* layout, MediaTrack* track)
{
    if(name == "TrackVolume")  return new TrackVolume_Action(layout, track);
    else if(name == "TrackVolumeDisplay")  return new TrackVolumeDisplay_Action(layout, track);
    else if(name == "TrackPan")  return new TrackPan_Action(layout, track);
    else if(name == "TrackPanWidth")  return new TrackPanWidth_Action(layout, track);
    else if(name == "TrackTouch")  return new TrackTouch_Action(layout, track);
    else if(name == "TrackMute")  return new TrackMute_Action(layout, track);
    else if(name == "TrackSolo")  return new TrackSolo_Action(layout, track);
    else if(name == "TrackUniqueSelect")  return new TrackUniqueSelect_Action(layout, track);
    else if(name == "TrackRangeSelect")  return new TrackRangeSelect_Action(layout, track);
    else if(name == "TrackSelect")  return new TrackSelect_Action(layout, track);
    else if(name == "TrackRecordArm")  return new TrackRecordArm_Action(layout, track);
    else if(name == "TrackNameDisplay")  return new TrackNameDisplay_Action(layout, track);
    else if(name == "MapTrackAndFXToWidgets")  return new MapTrackAndFXToWidgets_Action(layout, track);
    
    return new Action(layout);
}

Action* ActionFor(string name, Layout* layout, MediaTrack* track, string param)
{
    if(name == "TrackOutputMeter")  return new TrackOutputMeter_Action(layout, track, param);
    
    return new Action(layout);
}

Action* ActionFor(string name, string actionAddress, Layout* layout, MediaTrack* track, Action* baseAction)
{
    if(name == "TrackTouchControlled")  return new TrackTouchControlled_Action(actionAddress, layout, track, baseAction);
    
    return new Action(layout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidgeta available for inclusion in Real Surface Templates, we will add widgets as necessary
////////////////////////////////////////////////////////////////////////////////////////////////////////
int strToHex(string valueStr)
{
    return strtol(valueStr.c_str(), nullptr, 16);
}

double strToDouble(string valueStr)
{
    return strtod(valueStr.c_str(), nullptr);
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, int index)
{
    if(widgetClass == "Display") return new Display_MidiWidget(surface, name, index);
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(00, 00, 00), new MIDI_event_ex_t(00, 00, 00));
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "Button") return new PushButton_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithLatch") return new PushButtonWithLatch_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonWithRelease") return new PushButtonWithRelease_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "ButtonCycler") return new PushButtonCycler_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Encoder") return new Encoder_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "Fader7Bit") return new Fader7Bit_MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Min), new MIDI_event_ex_t(byte1, byte2, byte3Max));
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2, int byte3Min, int byte3Max)
{
    if(widgetClass == "VUMeter") return new VUMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
    else if(widgetClass == "GainReductionMeter") return new GainReductionMeter_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
  
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2, byte3Max), new MIDI_event_ex_t(byte1, byte2, byte3Min));
}

MidiWidget* WidgetFor(RealSurface* surface, string name, string widgetClass, double minDB, double maxDB, int byte1, int byte2Min, int byte2Max, int byte3Min, int byte3Max)
{
    if(widgetClass == "Fader14Bit") return new Fader14Bit_MidiWidget(surface, name, minDB, maxDB, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
    
    return new MidiWidget(surface, name, new MIDI_event_ex_t(byte1, byte2Max, byte3Max), new MIDI_event_ex_t(byte1, byte2Min, byte3Min));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// MidiWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////
void MidiWidget::AddToRealSurface(RealSurface* surface)
{
    surface->AddWidgetToNameMap(GetName(), this);
}

void MidiWidget::Update()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->UpdateAction(GetGUID(), GetActionName(), GetName());
}

void MidiWidget::ForceUpdate()
{
    // this is the turnaround point, now we head back up the chain eventually leading to Action ->
    GetRealSurface()->ForceUpdateAction(GetGUID(), GetActionName(), GetName());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurfaceChannel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RealSurfaceChannel::SetGUID(string GUID)
{
    GUID_ = GUID;
    
    for (auto widgetName : widgetNames_)
        realSurface_->SetWidgetGUID(widgetName, GUID);
    
    realSurface_->GetZone()->GetLayout()->MapTrack(GUID_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealSurface
////////////////////////////////////////////////////////////////////////////////////////////////////////
RealSurface::RealSurface(const string name, string templateFilename, int numChannels, int isBankable) : name_(name),  templateFilename_(templateFilename), isBankable_(isBankable) 
{
    for(int i = 0; i < numChannels; i++)
        channels_.push_back(new RealSurfaceChannel(to_string(i + 1), this));
}

void RealSurface::MapTrackToWidgets(MediaTrack *track)
{
    string trackGUID = DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false));
    
    for(auto* channel : channels_)
        channel->SetGUID(trackGUID);
}

void  RealSurface::UnmapWidgetsFromTrack(MediaTrack *track)
{
    for(auto* channel : channels_)
        channel->SetGUID("");
}

void RealSurface::AddAction(string actionAddress, Action* action)
{
    GetZone()->GetLayout()->AddAction(actionAddress, action);
}

// to Actions ->
double RealSurface::GetActionCurrentNormalizedValue(string GUID, string actionName, string widgetName)
{
    return GetZone()->GetActionCurrentNormalizedValue(GUID, GetName(), actionName, widgetName);
}

void RealSurface::UpdateAction(string GUID, string actionName, string widgetName)
{
    GetZone()->UpdateAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::ForceUpdateAction(string GUID, string actionName, string widgetName)
{
    GetZone()->ForceUpdateAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::CycleAction(string GUID, string actionName, string widgetName)
{
    GetZone()->CycleAction(GUID, GetName(), actionName, widgetName);
}

void RealSurface::DoAction(string GUID, string actionName, string widgetName, double value)
{
    GetZone()->DoAction(value, GUID, GetName(), actionName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Zone::MapFXActions(string trackGUID, RealSurface* surface)
{
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
    Layout* layout = GetLayout();
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];
    char fxGUID[BUFSZ];
    
    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(fxTemplates_.count(surface->GetName()) > 0 && fxTemplates_[surface->GetName()].count(fxName) > 0)
        {
            FXTemplate* map = fxTemplates_[surface->GetName()][fxName];
            DAW::guidToString(DAW::TrackFX_GetFXGUID(track, i), fxGUID);
            string actionBaseAddress = trackGUID + fxGUID + GetName() + surface->GetName();
            
            for(auto mapEntry : map->GetTemplateEntries())
            {
                if(mapEntry.paramName == GainReductionDB)
                    GetLayout()->AddAction(actionBaseAddress + mapEntry.widgetName, new TrackGainReductionMeter_Action(layout, track, fxGUID));
                else
                {
                    for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
                    {
                        DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                        
                        if(mapEntry.paramName == fxParamName)
                            GetLayout()->AddAction(actionBaseAddress + mapEntry.widgetName, new TrackFX_Action(layout, track, fxGUID, j));
                    }
                }
            }
        }
    }
}

void Zone::TrackFXListChanged(MediaTrack* track)
{
    char fxName[BUFSZ];
    char fxParamName[BUFSZ];

    for(int i = 0; i < DAW::TrackFX_GetCount(track); i++)
    {
        DAW::TrackFX_GetFXName(track, i, fxName, sizeof(fxName));
        
        if(GetLayout()->GetManager()->GetVSTMonitor())
        {
            DAW::ShowConsoleMsg(("\n\n" + string(fxName) + "\n").c_str());
            
            for(int j = 0; j < DAW::TrackFX_GetNumParams(track, i); j++)
            {
                DAW::TrackFX_GetParamName(track, i, j, fxParamName, sizeof(fxParamName));
                DAW::ShowConsoleMsg((string(fxParamName) + "\n").c_str());
            }
        }
    }
    
    for(auto* surface : realSurfaces_)
        MapFXActions(DAW::GetTrackGUIDAsString(DAW::CSurf_TrackToID(track, false)), surface);
}

void Zone::AddAction(string actionAddress, Action* action)
{
    GetLayout()->AddAction(actionAddress, action);
}

void Zone::MapRealSurfaceActions(RealSurface* surface)
{
    Layout* layout = GetLayout();
    string actionBaseAddress = DefaultGUID + GetName() + surface->GetName();;
    string templateDirectory = actionTemplateDirectory_[surface->GetName()];
    
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/" + filename));
           
            for (string line; getline(actionTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens.size() == 2)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layout));
                    else if(tokens.size() == 3)
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layout, tokens[2]));
                }
            }
        }
    }
}

void Zone::MapTrackActions(string trackGUID, RealSurface* surface)
{
    Layout* layout = GetLayout();
    MediaTrack* track = DAW::GetTrackFromGUID(trackGUID);
    string actionBaseAddress = trackGUID + GetName() + surface->GetName();
 
    string templateDirectory = actionTemplateDirectory_[surface->GetName()];
    
    for(string filename : FileSystem::GetDirectoryFilenames(templateDirectory))
    {
        if(filename.length() > 4 && filename[0] != '.' && filename[filename.length() - 4] == '.' && filename[filename.length() - 3] == 'a' && filename[filename.length() - 2] == 'x' &&filename[filename.length() - 1] == 't')
        {
            ifstream actionTemplateFile(string(templateDirectory + "/Track/" + filename));
            
            for (string line; getline(actionTemplateFile, line) ; )
            {
                if(line[0] != '/' && line != "") // ignore comment lines and blank lines
                {
                    istringstream iss(line);
                    vector<string> tokens;
                    string token;
                    while (iss >> quoted(token))
                        tokens.push_back(token);
                    
                    if(tokens.size() == 2)
                    {
                        AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layout, track));
                    }
                    else if(tokens.size() == 3)
                    {
                        if(tokens[1] == "TrackTouchControlled")
                        {
                            string actionAddress = actionBaseAddress + tokens[0];
                            Action* controlledAction = ActionFor(tokens[2], layout, track);
                            AddAction(actionAddress, ActionFor(tokens[1], actionAddress, layout, track, controlledAction));
                        }
                        else
                        {
                            AddAction(actionBaseAddress + tokens[0], ActionFor(tokens[1], layout, track, tokens[2]));
                        }
                    }
                    else if(tokens[1] == "Cycled" && tokens.size() > 4)
                    {
                        Action* cycledAction = ActionFor(tokens[1], layout);
                        AddAction(actionBaseAddress + tokens[0], cycledAction);
                        AddAction(actionBaseAddress + tokens[2], cycledAction);
                        for(int i = 3 ; i < tokens.size(); i++)
                            cycledAction->AddAction(ActionFor(tokens[i], layout, track));
                    }
                }
            }
        }
    }
}

// to Actions ->
double Zone::GetActionCurrentNormalizedValue(string GUID, string surfaceName, string actionName, string widgetName)
{
    return GetLayout()->GetActionCurrentNormalizedValue(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::UpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->UpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::ForceUpdateAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->ForceUpdateAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::CycleAction(string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->CycleAction(ActionAddressFor(GUID, surfaceName, actionName), GetName(), surfaceName, widgetName);
}

void Zone::DoAction(double value, string GUID, string surfaceName, string actionName, string widgetName)
{
    GetLayout()->DoAction(ActionAddressFor(GUID, surfaceName, actionName), value, GetName(), surfaceName, widgetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSurfManager
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfManager::InitRealSurface(RealSurface* surface)
{
    ifstream surfaceTemplateFile(surface->GetTemplateFilename());
    bool inChannel = false;
    
    for (string line; getline(surfaceTemplateFile, line) ; )
    {
        if(line[0] != '/' && line != "") // ignore comment lines and blank lines
        {
            istringstream iss(line);
            vector<string> tokens;
            string token;
            while (iss >> quoted(token))
                tokens.push_back(token);
            
            if(tokens.size() == 1)
            {
                if(tokens[0] == Channel)
                    inChannel = true;
                else if(tokens[0] == ChannelEnd)
                    inChannel = false;
            }
            else if(tokens.size() == 2)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], i));
            }
            else if(tokens.size() == 6)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]) + i, strToHex(tokens[4]), strToHex(tokens[5])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToHex(tokens[2]), strToHex(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5])));
            }
            else if(tokens.size() == 8)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]) + i, strToHex(tokens[6]), strToHex(tokens[7])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7])));
            }
            else if(tokens.size() == 9)
            {
                if(inChannel)
                    for(int i = 0; i < surface->GetChannels().size(); i++)
                        surface->GetChannels()[i]->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]) + i, strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
                else
                    surface->AddWidget(WidgetFor(surface, tokens[0], tokens[1], strToDouble(tokens[2]), strToDouble(tokens[3]), strToHex(tokens[4]), strToHex(tokens[5]), strToHex(tokens[6]), strToHex(tokens[7]), strToHex(tokens[8])));
            }
        }
    }
}


