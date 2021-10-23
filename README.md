# This Fork #
- The goal of this fork is to merge EternalJK/japro features into JAMME.
- All compiling and testing has been done on Linux. I can't really help out with other OS's at this point.

# Todo: #
- cg_stylePlayer 
- fix cg_strafeHelperActiveColor & center line on w turning movement styles
- make escape pause-play the demo as in game menu opens/closes (EJK behavior)
- \speedometer, \strafehelper , \styleplayer bit value config commands
- chatbox commands and emojis

# Working: # 
- cg_strafehelper (4066 for all lines, 20450 for all lines with line crosshair, configure with \strafehelper in EternalJK)
- cg_speedometer (Configure with \speedometer command in EternaJK)
- cg_racetimer 0-2
- cg_crosshairHealth 0-1
- cg_crosshairSaberStyleColor 0-1
- cg_hudColors 0-1 
- cg_drawFps 0-6
- cg_drawTimer 0-6
- cg_drawTimerMsec 0-1
- cg_lagometer 0-3
- Esc no longer exits demo, opens in game menu (EternalJK behavior)

# Contributors #
- tayst

# Credits # 
- ent for his massive amounts of work in making jaMME and helping me to port these features
- loda, bucky, eternal, alpha for their work on japro and EternalJK
- duo for his help with the rear S/AS/SD strafehelper lines

Jedi Academy Movie Maker's Edition
==================================

Jedi Academy Movie Maker's Edition (jaMME) is an engine modification of Jedi Academy for moviemaking. It's a port of q3mme with most of its features and some new ones. The modification is based on very early (May 2013th) version of OpenJK. Original source code belongs to Raven Software.

# Features #
* demo playback control (pause, rewind)
* free camera mode
* chase camera mode
* time speed animation
* capturing motion blur
* capturing output in stereo 3D
* different output types: jpg, tga, png, avi
* playing music on background to synchronize it with editing
* saving depth of field mask
* overriding players information: name, saber colours, hilts, team, model
* realistic first person view with visible body (trueview)
* recording audio to wav
* replacing world textures with your own
* replacing skybox with one solid colour (chroma key)
* capturing in any resolution
* off-screen capturing
* capturing a list of demos
* supporting mods: base (basejka, base_enhanced), ja+ (ja++), lugormod, makermod, MBII

# Contributors #
* ent
* Scooper
* redsaurus
* teh

# Installation #
Extract the archive to "GameData" folder.

# Dependencies #
* zlib for pk3 support
* libmad for mp3 support
* libogg for ogg vorbis support
* libvorbis for ogg vorbis support
* libflac for flac support

# Credits #
* q3mme crew and their q3mme mod
* Raz0r and his JA++ code for JA+ and JA++ compatibiliy, also he helped a lot
* teh and his pugmod that was a good starting point, also some jaMME features are taken from pugmod; and his jamme port for linux
* Sil and his features from SMod
* CaNaBiS and his help in explaining of how q3mme works
* Razorace and his trueview feature (taken from JA++)
* Grant R. Griffin and his "FIR filter algorithms for use in C"
* OpenJK contributors and their OpenJK project
* redsaurus for Mac support

