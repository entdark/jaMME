cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InjaMME)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InjaMME)

set(MPSharedDefines ${SharedDefines})

#    Add Game Project
if(BuildGame)
	add_subdirectory("${MPDir}/game")
endif(BuildGame)

#    Add CGame Project
if(BuildCGame)
	add_subdirectory("${MPDir}/cgame")
endif(BuildCGame)

#    Add UI Project
if(BuildUI)
	add_subdirectory("${MPDir}/ui")
endif(BuildUI)

#	 Add Renderer Project
if(BuildEngine)
	add_subdirectory("${MPDir}/rd-vanilla")
endif(BuildEngine)


#        Engine/Executable (jamme.exe)

if(BuildEngine)

	#    Botlib
	# the files could arguably just be put into the engine without having a library for it.
	set(MPBotlibDefines ${SharedDefines} "BOTLIB")
	set(MPBotlibIncludeDirectories "${MPDir}")
	set(MPBotlibHeaderFiles
		"${MPDir}/botlib/aasfile.h"
		"${MPDir}/botlib/be_aas.h"
		"${MPDir}/botlib/be_aas_bsp.h"
		"${MPDir}/botlib/be_aas_cluster.h"
		"${MPDir}/botlib/be_aas_debug.h"
		"${MPDir}/botlib/be_aas_def.h"
		"${MPDir}/botlib/be_aas_entity.h"
		"${MPDir}/botlib/be_aas_file.h"
		"${MPDir}/botlib/be_aas_funcs.h"
		"${MPDir}/botlib/be_aas_main.h"
		"${MPDir}/botlib/be_aas_move.h"
		"${MPDir}/botlib/be_aas_optimize.h"
		"${MPDir}/botlib/be_aas_reach.h"
		"${MPDir}/botlib/be_aas_route.h"
		"${MPDir}/botlib/be_aas_routealt.h"
		"${MPDir}/botlib/be_aas_sample.h"
		"${MPDir}/botlib/be_ai_char.h"
		"${MPDir}/botlib/be_ai_chat.h"
		"${MPDir}/botlib/be_ai_gen.h"
		"${MPDir}/botlib/be_ai_goal.h"
		"${MPDir}/botlib/be_ai_move.h"
		"${MPDir}/botlib/be_ai_weap.h"
		"${MPDir}/botlib/be_ai_weight.h"
		"${MPDir}/botlib/be_ea.h"
		"${MPDir}/botlib/be_interface.h"
		"${MPDir}/botlib/botlib.h"
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/botlib/l_crc.h"
		"${MPDir}/botlib/l_libvar.h"
		"${MPDir}/botlib/l_log.h"
		"${MPDir}/botlib/l_memory.h"
		"${MPDir}/botlib/l_precomp.h"
		"${MPDir}/botlib/l_script.h"
		"${MPDir}/botlib/l_struct.h"
		"${MPDir}/botlib/l_utils.h"
		"${MPDir}/qcommon/q_shared.h"
		"${MPDir}/qcommon/qcommon.h"
		"${MPDir}/qcommon/qfiles.h"
		"${MPDir}/server/server.h"
		"${MPDir}/game/surfaceflags.h"
		"${MPDir}/qcommon/tags.h"
		)
	source_group("Header Files" FILES ${MPBotlibHeaderFiles})
	set(MPBotlibSourceFiles
		"${MPDir}/botlib/be_aas_bspq3.cpp"
		"${MPDir}/botlib/be_aas_cluster.cpp"
		"${MPDir}/botlib/be_aas_debug.cpp"
		"${MPDir}/botlib/be_aas_entity.cpp"
		"${MPDir}/botlib/be_aas_file.cpp"
		"${MPDir}/botlib/be_aas_main.cpp"
		"${MPDir}/botlib/be_aas_move.cpp"
		"${MPDir}/botlib/be_aas_optimize.cpp"
		"${MPDir}/botlib/be_aas_reach.cpp"
		"${MPDir}/botlib/be_aas_route.cpp"
		"${MPDir}/botlib/be_aas_routealt.cpp"
		"${MPDir}/botlib/be_aas_sample.cpp"
		"${MPDir}/botlib/be_ai_char.cpp"
		"${MPDir}/botlib/be_ai_chat.cpp"
		"${MPDir}/botlib/be_ai_gen.cpp"
		"${MPDir}/botlib/be_ai_goal.cpp"
		"${MPDir}/botlib/be_ai_move.cpp"
		"${MPDir}/botlib/be_ai_weap.cpp"
		"${MPDir}/botlib/be_ai_weight.cpp"
		"${MPDir}/botlib/be_ea.cpp"
		"${MPDir}/botlib/be_interface.cpp"
		"${MPDir}/botlib/l_crc.cpp"
		"${MPDir}/botlib/l_libvar.cpp"
		"${MPDir}/botlib/l_log.cpp"
		"${MPDir}/botlib/l_memory.cpp"
		"${MPDir}/botlib/l_precomp.cpp"
		"${MPDir}/botlib/l_script.cpp"
		"${MPDir}/botlib/l_struct.cpp"
		)
	source_group("Source Files" FILES ${MPBotlibSourceFiles})
	
#	add_library(${MPBotLib} STATIC ${MPBotlibHeaderFiles} ${MPBotlibSourceFiles})
#	set_target_properties(${MPBotLib} PROPERTIES COMPILE_DEFINITIONS "${MPBotlibDefines}")
#	set_target_properties(${MPBotLib} PROPERTIES INCLUDE_DIRECTORIES "${MPBotlibIncludeDirectories}")
	
	#    Common files/libraries/defines of both Engine and Dedicated Server
	
	# libraries: Botlib
	set(MPEngineAndDedLibraries ${MPBotLib})
	# Platform-specific libraries
	if(WIN32)
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} "winmm" "wsock32")
	else(WIN32)
		message("You might need to add some libraries to codemp/CMakeLists.txt or you could get linker errors on non-Windows-OSs.")
	endif(WIN32)
	# Include directories
	set(MPEngineAndDedIncludeDirectories ${MPDir}) # codemp folder, since includes are not always relative in the files
	
	if(UseInternalZlib)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} "${MPDir}/zlib")
		find_library(ZlibLibrary NAMES zlib PATHS "${MPDir}/libs/win32/lib")
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${ZlibLibrary})
	else(UseInternalZlib)
		find_package(ZLIB REQUIRED)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} ${ZLIB_INCLUDE_DIR})
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${ZLIB_LIBRARY})
	endif(UseInternalZlib)

	file(GLOB_RECURSE J_SRC "${MPDir}/jpeg-8c/*.c" "${MPDir}/jpeg-8c/*.h")
	source_group("jpeg-8c" FILES ${J_SRC})
	
	set(MPEngineAndDedCgameFiles
		"${MPDir}/cgame/cg_public.h"
		)
	source_group("cgame" FILES ${MPEngineAndDedCgameFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCgameFiles})
	
	set(MPEngineAndDedClientFiles
		"${MPDir}/client/client.h"
		"${MPDir}/client/keys.h"
		"${MPDir}/client/snd_public.h"
		)
	source_group("client" FILES ${MPEngineAndDedClientFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedClientFiles})
	
	set(MPEngineAndDedGameFiles
		"${MPDir}/game/anims.h"
		"${MPDir}/game/bg_public.h"
		"${MPDir}/game/bg_weapons.h"
		"${MPDir}/botlib/botlib.h"
		"${MPDir}/game/g_public.h"
		"${MPDir}/game/surfaceflags.h"
		)
	source_group("game" FILES ${MPEngineAndDedGameFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedGameFiles})
	
	set(MPEngineAndDedCommonFiles
		"${MPDir}/qcommon/q_shared.h"
		"${MPDir}/qcommon/chash.h"
		"${MPDir}/qcommon/cm_draw.cpp"
		"${MPDir}/qcommon/cm_draw.h"
		"${MPDir}/qcommon/cm_landscape.h"
		"${MPDir}/qcommon/cm_load.cpp"
		"${MPDir}/qcommon/cm_local.h"
		"${MPDir}/qcommon/cm_patch.cpp"
		"${MPDir}/qcommon/cm_patch.h"
		"${MPDir}/qcommon/cm_polylib.cpp"
		"${MPDir}/qcommon/cm_polylib.h"
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/cm_randomterrain.cpp"
		"${MPDir}/qcommon/cm_randomterrain.h"
		"${MPDir}/qcommon/cm_shader.cpp"
		"${MPDir}/qcommon/cm_terrain.cpp"
		"${MPDir}/qcommon/cm_terrainmap.h"
		"${MPDir}/qcommon/cm_test.cpp"
		"${MPDir}/qcommon/cm_trace.cpp"
		"${MPDir}/qcommon/cmd_common.cpp"
		"${MPDir}/qcommon/cmd_pc.cpp"
		"${MPDir}/qcommon/common.cpp"
		"${MPDir}/qcommon/cvar.cpp"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/qcommon/files_common.cpp"
		"${MPDir}/qcommon/files_pc.cpp"
		"${MPDir}/qcommon/game_version.h"
		"${MPDir}/qcommon/GenericParser2.cpp"
		"${MPDir}/qcommon/GenericParser2.h"
		"${MPDir}/qcommon/huffman.cpp"
		"${MPDir}/qcommon/md4.cpp"
		"${MPDir}/qcommon/MiniHeap.h"
		"${MPDir}/qcommon/msg.cpp"
		"${MPDir}/qcommon/matcomp.h"
		"${MPDir}/qcommon/matcomp.cpp"
		"${MPDir}/qcommon/net_chan.cpp"
		"${MPDir}/qcommon/q_math.cpp"
		"${MPDir}/qcommon/q_shared.cpp"
		"${MPDir}/qcommon/qcommon.h"
		"${MPDir}/qcommon/qfiles.h"
		"${MPDir}/qcommon/RoffSystem.cpp"
		"${MPDir}/qcommon/RoffSystem.h"
		"${MPDir}/qcommon/sstring.h"
		"${MPDir}/qcommon/stringed_ingame.cpp"
		"${MPDir}/qcommon/stringed_ingame.h"
		"${MPDir}/qcommon/stringed_interface.cpp"
		"${MPDir}/qcommon/stringed_interface.h"
		"${MPDir}/qcommon/tags.h"
		"${MPDir}/qcommon/timing.h"
		"${MPDir}/qcommon/vm.cpp"
		"${MPDir}/qcommon/z_memman_pc.cpp"
		)
	if(WIN32)
#		set(MPEngineAndDedCommonFiles
#			${MPEngineAndDedCommonFiles}
#			"${MPDir}/qcommon/vm_x86.cpp")
	endif(WIN32)
	source_group("common" FILES ${MPEngineAndDedCommonFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCommonFiles})
	
	set(MPEngineAndDedG2Files
		"${MPDir}/ghoul2/G2.h"
		"${MPDir}/ghoul2/G2_gore.h"
		"${MPDir}/ghoul2/ghoul2_shared.h"
		)
	source_group("ghoul2" FILES ${MPEngineAndDedG2Files})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedG2Files})
	
	set(MPEngineAndDedIcarusFiles
		"${MPDir}/icarus/BlockStream.cpp"
		"${MPDir}/icarus/blockstream.h"
		"${MPDir}/icarus/GameInterface.cpp"
		"${MPDir}/icarus/GameInterface.h"
		"${MPDir}/icarus/icarus.h"
		"${MPDir}/icarus/Instance.cpp"
		"${MPDir}/icarus/instance.h"
		"${MPDir}/icarus/Interface.cpp"
		"${MPDir}/icarus/interface.h"
		"${MPDir}/icarus/interpreter.h"
		"${MPDir}/icarus/Memory.cpp"
		"${MPDir}/icarus/Q3_Interface.cpp"
		"${MPDir}/icarus/Q3_Interface.h"
		"${MPDir}/icarus/Q3_Registers.cpp"
		"${MPDir}/icarus/Q3_Registers.h"
		"${MPDir}/icarus/Sequence.cpp"
		"${MPDir}/icarus/sequence.h"
		"${MPDir}/icarus/Sequencer.cpp"
		"${MPDir}/icarus/sequencer.h"
		"${MPDir}/icarus/TaskManager.cpp"
		"${MPDir}/icarus/taskmanager.h"
		"${MPDir}/icarus/tokenizer.h"
		)
	source_group("icarus" FILES ${MPEngineAndDedIcarusFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedIcarusFiles})
	
	set(MPEngineAndDedRmgFiles
		"${MPDir}/RMG/RM_Area.cpp"
		"${MPDir}/RMG/RM_Area.h"
		"${MPDir}/RMG/RM_Headers.h"
		"${MPDir}/RMG/RM_Instance.cpp"
		"${MPDir}/RMG/RM_Instance.h"
		"${MPDir}/RMG/RM_Instance_BSP.cpp"
		"${MPDir}/RMG/RM_Instance_BSP.h"
		"${MPDir}/RMG/RM_Instance_Group.cpp"
		"${MPDir}/RMG/RM_Instance_Group.h"
		"${MPDir}/RMG/RM_Instance_Random.cpp"
		"${MPDir}/RMG/RM_Instance_Random.h"
		"${MPDir}/RMG/RM_Instance_Void.cpp"
		"${MPDir}/RMG/RM_Instance_Void.h"
		"${MPDir}/RMG/RM_InstanceFile.cpp"
		"${MPDir}/RMG/RM_InstanceFile.h"
		"${MPDir}/RMG/RM_Manager.cpp"
		"${MPDir}/RMG/RM_Manager.h"
		"${MPDir}/RMG/RM_Mission.cpp"
		"${MPDir}/RMG/RM_Mission.h"
		"${MPDir}/RMG/RM_Objective.cpp"
		"${MPDir}/RMG/RM_Objective.h"
		"${MPDir}/RMG/RM_Path.cpp"
		"${MPDir}/RMG/RM_Path.h"
		"${MPDir}/RMG/RM_Terrain.cpp"
		"${MPDir}/RMG/RM_Terrain.h"
		)
	source_group("rmg" FILES ${MPEngineAndDedRmgFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedRmgFiles})
	
	set(MPEngineAndDedServerFiles
		"${MPDir}/server/NPCNav/gameCallbacks.cpp"
		"${MPDir}/server/NPCNav/navigator.cpp"
		"${MPDir}/server/NPCNav/navigator.h"
		"${MPDir}/server/server.h"
		"${MPDir}/server/sv_bot.cpp"
		"${MPDir}/server/sv_ccmds.cpp"
		"${MPDir}/server/sv_client.cpp"
		"${MPDir}/server/sv_game.cpp"
		"${MPDir}/server/sv_init.cpp"
		"${MPDir}/server/sv_main.cpp"
		"${MPDir}/server/sv_net_chan.cpp"
		"${MPDir}/server/sv_snapshot.cpp"
		"${MPDir}/server/sv_world.cpp"
		)
	source_group("server" FILES ${MPEngineAndDedServerFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedServerFiles})
	
	set(MPEngineAndDedUiFiles
		"${MPDir}/ui/keycodes.h"
		"${MPDir}/ui/ui_public.h"
		)
	source_group("ui" FILES ${MPEngineAndDedUiFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedUiFiles})
	
	# TODO: replace the following with proper library? Update it?
	set(MPEngineAndDedMinizipFiles
		"${MPDir}/qcommon/unzip.cpp"
		"${MPDir}/qcommon/unzip.h"
		"${MPDir}/zlib/ioapi.c"
		"${MPDir}/zlib/ioapi.h"
		)
	source_group("minizip" FILES ${MPEngineAndDedMinizipFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedMinizipFiles})
	
	if(WIN32)
		set(MPEngineAndDedWin32Files
			"${MPDir}/win32/win_local.h"
			"${MPDir}/win32/win_net.cpp"
			"${MPDir}/win32/win_shared.cpp"
			"${MPDir}/sys/snapvector.cpp"
			)
		source_group("win32" FILES ${MPEngineAndDedWin32Files})
		set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedWin32Files})
	else(WIN32)
		set(MPEngineAndDedSysFiles
			"${MPDir}/sys/snapvector.cpp"
			)
		set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedSysFiles})
		source_group("sys" FILES ${MPEngineAndDedSysFiles})
	endif(WIN32)

	set(MPEngineLibraries ${MPEngineAndDedLibraries})
	set(MPEngineIncludeDirectories ${MPEngineAndDedIncludeDirectories})
	set(MPEngineFiles ${MPEngineAndDedFiles})
	set(MPEngineDefines ${MPSharedDefines} "USE_CURL" "USE_CURL" "CURL_STATICLIB" "_WINSOCK_H")
	
	#    Dependencies
	
	#library stuff
	if(WIN32)
		if(MSVC14)
			set(MPEngineLibraries ${MPEngineLibraries} legacy_stdio_definitions.lib)
		endif(MSVC144)
		set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/win32/lib/libmad.lib)
		set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/win32/lib/libFLAC_static.lib)
		set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/win32/lib/libogg_static.lib)
		set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/win32/lib/libvorbis_static.lib)
		set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/win32/lib/libcurl.lib)
		set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/win32/lib/win_utf8_io_static.lib)
	endif(WIN32)
	if(UNIX)
		if(APPLE)
			set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/macosx/lib/libmad.a)
			set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/macosx/lib/libFLAC.a)
			set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/macosx/lib/libogg.a)
			set(MPEngineLibraries ${MPEngineLibraries} ${MPDir}/libs/macosx/lib/libvorbis.a)
			set(MPEngineLibraries ${MPEngineLibraries} curl)
		else(APPLE)
			set(MPEngineLibraries ${MPEngineLibraries} mad)
			set(MPEngineLibraries ${MPEngineLibraries} FLAC)
			set(MPEngineLibraries ${MPEngineLibraries} ogg)
			set(MPEngineLibraries ${MPEngineLibraries} vorbis)
			set(MPEngineLibraries ${MPEngineLibraries} curl)
		endif(APPLE)
	endif(UNIX)
	
	if(NOT WIN32)
		find_package(SDL2 REQUIRED)
		set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${SDL2_INCLUDE_DIR})
		set(MPEngineLibraries ${MPEngineLibraries} ${SDL2_LIBRARY})
	endif(NOT WIN32)

	set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${MPDir}/libs/win32/include)

	#    Source Files
	
	set(MPEngineDefines ${MPSharedDefines} ${MPBotlibDefines})
	set(MPEngineFiles ${MPEngineFiles} ${MPBotlibHeaderFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPBotlibSourceFiles})
	# TODO: Allow for use of external library
	file(GLOB_RECURSE MPEngineJpegFiles "${MPDir}/jpeg-8c/*.c" "${MPDir}/jpeg-8c/*.h")
	source_group("jpeg-8c" FILES ${MPEngineJpegFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineJpegFiles})
	
	set(MPEngineClientFiles
		"${MPDir}/client/cl_cgame.cpp"
		"${MPDir}/client/cl_cin.cpp"
		"${MPDir}/client/cl_console.cpp"
		"${MPDir}/client/cl_curl.cpp"
		"${MPDir}/client/cl_demos.cpp"
		"${MPDir}/client/cl_demos_auto.cpp"
		"${MPDir}/client/cl_demos_cut.cpp"
		"${MPDir}/client/cl_input.cpp"
		"${MPDir}/client/cl_keys.cpp"
		"${MPDir}/client/cl_main.cpp"
		"${MPDir}/client/cl_mme.cpp"
		"${MPDir}/client/cl_net_chan.cpp"
		"${MPDir}/client/cl_parse.cpp"
		"${MPDir}/client/cl_scrn.cpp"
		"${MPDir}/client/cl_ui.cpp"
		"${MPDir}/client/FXExport.cpp"
		"${MPDir}/client/FXExport.h"
		"${MPDir}/client/FxPrimitives.cpp"
		"${MPDir}/client/FxPrimitives.h"
		"${MPDir}/client/FxScheduler.cpp"
		"${MPDir}/client/FxScheduler.h"
		"${MPDir}/client/FxSystem.cpp"
		"${MPDir}/client/FxSystem.h"
		"${MPDir}/client/FxTemplate.cpp"
		"${MPDir}/client/FxUtil.cpp"
		"${MPDir}/client/FxUtil.h"
		"${MPDir}/client/snd_ambient.cpp"
		"${MPDir}/client/snd_ambient.h"
		"${MPDir}/client/snd_dma.cpp"
		"${MPDir}/client/snd_effects.cpp"
		"${MPDir}/client/snd_local.h"
		"${MPDir}/client/snd_main.cpp"
		"${MPDir}/client/snd_mem.cpp"
		"${MPDir}/client/snd_mix.cpp"
		"${MPDir}/client/snd_mme.cpp"
		)
	source_group("client" FILES ${MPEngineClientFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineClientFiles})
	
	set(MPEngineCommonFiles
		"${MPDir}/qcommon/cm_terrainmap.cpp"
		"${MPDir}/qcommon/CNetProfile.cpp"
		"${MPDir}/qcommon/exe_headers.cpp"
		"${MPDir}/qcommon/exe_headers.h"
		"${MPDir}/qcommon/files.h"
		"${MPDir}/qcommon/hstring.cpp"
		"${MPDir}/qcommon/hstring.h"
		"${MPDir}/qcommon/INetProfile.h"
		"${MPDir}/qcommon/platform.h"
		)
	source_group("common" FILES ${MPEngineCommonFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineCommonFiles})
	
	set(MPEngineRatlFiles
		"${MPDir}/Ratl/bits_vs.h"
		"${MPDir}/Ratl/ratl_common.h"
		"${MPDir}/Ratl/vector_vs.h"
		)
	source_group("ratl" FILES ${MPEngineRatlFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineRatlFiles})
	
	set(MPEngineRavlFiles
		"${MPDir}/Ravl/CVec.h"
		)
	source_group("ravl" FILES ${MPEngineRavlFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineRavlFiles})
		
	if(WIN32)
		set(MPEngineWin32Files
			"${MPDir}/win32/glw_win.h"
			"${MPDir}/win32/icon.ico"
			"${MPDir}/win32/resource.h"
			"${MPDir}/win32/win_input.cpp"
			"${MPDir}/win32/win_main.cpp"
			"${MPDir}/win32/win_snd.cpp"
			"${MPDir}/win32/win_syscon.cpp"
			"${MPDir}/win32/win_wndproc.cpp"
			"${MPDir}/win32/jamme.rc"
			)
		source_group("win32" FILES ${MPEngineWin32Files})
		set(MPEngineFiles ${MPEngineFiles} ${MPEngineWin32Files})
	else(WIN32)
		set(MPEngineSysFiles
			"${MPDir}/sdl/sdl_input.cpp"
			"${MPDir}/sdl/sdl_qgl.h"
			"${MPDir}/sdl/sdl_snd.cpp"
			"${MPDir}/sys/sys_loadlib.h"
			"${MPDir}/sys/sys_local.h"
			"${MPDir}/sys/sys_main.cpp"
			"${MPDir}/sys/sys_net.cpp"
			"${MPDir}/sys/sys_unix.cpp"
			"${MPDir}/sys/unix_local.h"
			)
		source_group("sys" FILES ${MPEngineSysFiles})
		set(MPEngineFiles ${MPEngineFiles} ${MPEngineSysFiles})
	endif(WIN32)

	# todo: windows resources
	
	if(MakeApplicationBundles)
		set(MACOSX_BUNDLE_BUNDLE_NAME ${jaMMEEngine})
		set(MACOSX_BUNDLE_DISPLAY_NAME ${jaMMEEngine})
		set(MACOSX_BUNDLE_ICON_FILE jaMME.icns)
		set_source_files_properties(${MPDir}/macosx/jaMME.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		set(MPEngineFiles ${MPEngineFiles} ${MPDir}/macosx/jaMME.icns)
		set_source_files_properties(${MPDir}/macosx/English.lproj/InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION Resources/English.lproj)
		set(MPEngineFiles ${MPEngineFiles} ${MPDir}/macosx/English.lproj/InfoPlist.strings)
	endif(MakeApplicationBundles)

	if(WIN32)
		add_executable(${jaMMEEngine} WIN32 ${MPEngineFiles})
	else(WIN32)
		if(MakeApplicationBundles)
			add_executable(${jaMMEEngine} MACOSX_BUNDLE ${MPEngineFiles})
			set_target_properties(${jaMMEEngine} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${MPDir}/macosx/jaMMEInfo.plist")
		else(MakeApplicationBundles)
			add_executable(${jaMMEEngine} ${MPEngineFiles})
		endif(MakeApplicationBundles)
	endif(WIN32)
	if(MakeApplicationBundles)
		install(TARGETS ${jaMMEEngine} BUNDLE DESTINATION "${jaMMEInstallDir}")
	else(MakeApplicationBundles)
		install(TARGETS ${jaMMEEngine} RUNTIME DESTINATION "${jaMMEInstallDir}")
	endif(MakeApplicationBundles)
	set_target_properties(${jaMMEEngine} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${jaMMEEngine} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPEngineDefines};${DebugDefines}")
	set_target_properties(${jaMMEEngine} PROPERTIES INCLUDE_DIRECTORIES "${MPEngineIncludeDirectories}")
	target_link_libraries(${jaMMEEngine} ${MPEngineLibraries})
endif(BuildEngine)
