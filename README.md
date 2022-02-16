# Control Surface Integration (CSI) Project

### About

The CSI project was created to extend support for control surfaces in Reaper. Geoff Waddington developed a majority of the code in C++ while the community helped with the design and testing. After three years of labor, multiple design changes and a LOT of discussion the first working version was unveiled in 2019.

CSI is designed to be as stateless as possible, reflecting what Reaper is doing. If you hide tracks, CSI remaps controls to only what is visible. It can integrate multiple controllers simultaneously keeping them in sync to work as one large control surface. 

### Features

- Multiple controllers working together as a "single surface"
- Midi, MCU and OSC Protocol Support
- PC and Mac versions (looking to support Linux in the future) 
- Multi-Layer Zones allow dynamic configuration. For example: controls can be remapped when recording vs FX editing
- VCA and FX spill modes
- Feedback support for RGB Lighting and Text Displays
- Banking Support



### Controller Types

- any MCU controller
- any OSC Controller
- any Midi Note/CC controller

### Some Supported Controllers

The following is a limited listing of controllers configurations have been created for:

- Presonus Faderport 1/8/16
- Mackie C4
- DJ Tech Tools Midi Fighter Twister
- Novation Lauch Control XL pad mini
- BCF 2000
- ICON Platform M+/B+, QCon Pro X/EX/G2, Nano
- Behringer X Touch/EX/One/mini, FCB 1010, BCR/BCF 2000
- Softube Console 1


### Project Links
&nbsp;&nbsp;&nbsp;&nbsp;[Reaper Forums Development Thread](https://forum.cockos.com/showthread.php?t=183143)

&nbsp;&nbsp;&nbsp;&nbsp;[Reaper Forums Device Help Thread](https://forum.cockos.com/showthread.php?t=245280)

&nbsp;&nbsp;&nbsp;&nbsp;[Documentation](https://github.com/GeoffAWaddington/reaper_csurf_integrator/wiki)

&nbsp;&nbsp;&nbsp;&nbsp;[Reaper CSI Project Repository](https://github.com/reaper-csi/reaper_csurf_integrator)

&nbsp;&nbsp;&nbsp;&nbsp;[Version 1.1 Builds](https://stash.reaper.fm/v/42437/CSI%20v1_1.zip)

&nbsp;&nbsp;&nbsp;&nbsp;[Version 1.0 Builds](https://stash.reaper.fm/v/40638/CSI%20v1_0.zip)
