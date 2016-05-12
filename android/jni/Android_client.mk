
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libcurl
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

LOCAL_MODULE    := jamme


LOCAL_CFLAGS :=  $(JK3_BASE_CFLAGS)
LOCAL_CPPFLAGS := $(JK3_BASE_CPPFLAGS) -DNDEBUG -DBOTLIB -D_JK2 -D_JK2MP -DFINAL_BUILD -DUSE_CURL -DCURL_STATICLIB

LOCAL_LDLIBS := $(JK3_BASE_LDLIBS)

#LOCAL_LDLIBS +=  -lGLESv1_CM -lEGL -llog -lz
LOCAL_STATIC_LIBRARIES := curl libcurl libmad# libogg_static libvorbis_static win_utf8_io_static libFLAC_static 
LOCAL_SHARED_LIBRARIES := SDL2# SDL2_mixer touchcontrols openal

LOCAL_C_INCLUDES := $(LOCAL_PATH)/android/jni/SDL2/include

#############################################################################
# CLIENT/SERVER
#############################################################################

	

JK3_SRC = \
		../../${MPDir}/android/android-jni.cpp \
		../../${MPDir}/android/in_android.cpp \
		../../${MPDir}/android/android_main.cpp \
	\
		../../${MPDir}/qcommon/cm_draw.cpp \
		../../${MPDir}/qcommon/cm_load.cpp \
		../../${MPDir}/qcommon/cm_patch.cpp \
		../../${MPDir}/qcommon/cm_polylib.cpp \
		../../${MPDir}/qcommon/cm_randomterrain.cpp \
		../../${MPDir}/qcommon/cm_shader.cpp \
		../../${MPDir}/qcommon/cm_terrain.cpp \
		../../${MPDir}/qcommon/cm_test.cpp \
		../../${MPDir}/qcommon/cm_trace.cpp \
		../../${MPDir}/qcommon/cmd_common.cpp \
		../../${MPDir}/qcommon/cmd_pc.cpp \
		../../${MPDir}/qcommon/common.cpp \
		../../${MPDir}/qcommon/cvar.cpp \
		../../${MPDir}/qcommon/files_common.cpp \
		../../${MPDir}/qcommon/files_pc.cpp \
		../../${MPDir}/qcommon/GenericParser2.cpp \
		../../${MPDir}/qcommon/huffman.cpp \
		../../${MPDir}/qcommon/md4.cpp \
		../../${MPDir}/qcommon/msg.cpp \
		../../${MPDir}/qcommon/matcomp.cpp \
		../../${MPDir}/qcommon/net_chan.cpp \
		../../${MPDir}/qcommon/persistence.cpp \
		../../${MPDir}/qcommon/q_math.cpp \
		../../${MPDir}/qcommon/q_shared.cpp \
		../../${MPDir}/qcommon/RoffSystem.cpp \
		../../${MPDir}/qcommon/stringed_ingame.cpp \
		../../${MPDir}/qcommon/stringed_interface.cpp \
		../../${MPDir}/qcommon/vm.cpp \
		../../${MPDir}/qcommon/z_memman_pc.cpp \
		\
		../../${MPDir}/botlib/be_aas_bspq3.cpp \
		../../${MPDir}/botlib/be_aas_cluster.cpp \
		../../${MPDir}/botlib/be_aas_debug.cpp \
		../../${MPDir}/botlib/be_aas_entity.cpp \
		../../${MPDir}/botlib/be_aas_file.cpp \
		../../${MPDir}/botlib/be_aas_main.cpp \
		../../${MPDir}/botlib/be_aas_move.cpp \
		../../${MPDir}/botlib/be_aas_optimize.cpp \
		../../${MPDir}/botlib/be_aas_reach.cpp \
		../../${MPDir}/botlib/be_aas_route.cpp \
		../../${MPDir}/botlib/be_aas_routealt.cpp \
		../../${MPDir}/botlib/be_aas_sample.cpp \
		../../${MPDir}/botlib/be_ai_char.cpp \
		../../${MPDir}/botlib/be_ai_chat.cpp \
		../../${MPDir}/botlib/be_ai_gen.cpp \
		../../${MPDir}/botlib/be_ai_goal.cpp \
		../../${MPDir}/botlib/be_ai_move.cpp \
		../../${MPDir}/botlib/be_ai_weap.cpp \
		../../${MPDir}/botlib/be_ai_weight.cpp \
		../../${MPDir}/botlib/be_ea.cpp \
		../../${MPDir}/botlib/be_interface.cpp \
		../../${MPDir}/botlib/l_crc.cpp \
		../../${MPDir}/botlib/l_libvar.cpp \
		../../${MPDir}/botlib/l_log.cpp \
		../../${MPDir}/botlib/l_memory.cpp \
		../../${MPDir}/botlib/l_precomp.cpp \
		../../${MPDir}/botlib/l_script.cpp \
		../../${MPDir}/botlib/l_struct.cpp \
		\
		../../${MPDir}/icarus/BlockStream.cpp \
		../../${MPDir}/icarus/GameInterface.cpp \
		../../${MPDir}/icarus/Instance.cpp \
		../../${MPDir}/icarus/Interface.cpp \
		../../${MPDir}/icarus/Memory.cpp \
		../../${MPDir}/icarus/Q3_Interface.cpp \
		../../${MPDir}/icarus/Q3_Registers.cpp \
		../../${MPDir}/icarus/Sequence.cpp \
		../../${MPDir}/icarus/Sequencer.cpp \
		../../${MPDir}/icarus/TaskManager.cpp \
		\
		../../${MPDir}/RMG/RM_Area.cpp \
		../../${MPDir}/RMG/RM_Instance.cpp \
		../../${MPDir}/RMG/RM_Instance_BSP.cpp \
		../../${MPDir}/RMG/RM_Instance_Group.cpp \
		../../${MPDir}/RMG/RM_Instance_Random.cpp \
		../../${MPDir}/RMG/RM_Instance_Void.cpp \
		../../${MPDir}/RMG/RM_InstanceFile.cpp \
		../../${MPDir}/RMG/RM_Manager.cpp \
		../../${MPDir}/RMG/RM_Mission.cpp \
		../../${MPDir}/RMG/RM_Objective.cpp \
		../../${MPDir}/RMG/RM_Path.cpp \
		../../${MPDir}/RMG/RM_Terrain.cpp \
		\
		../../${MPDir}/server/NPCNav/gameCallbacks.cpp \
		../../${MPDir}/server/NPCNav/navigator.cpp \
		../../${MPDir}/server/sv_bot.cpp \
		../../${MPDir}/server/sv_ccmds.cpp \
		../../${MPDir}/server/sv_client.cpp \
		../../${MPDir}/server/sv_game.cpp \
		../../${MPDir}/server/sv_init.cpp \
		../../${MPDir}/server/sv_main.cpp \
		../../${MPDir}/server/sv_net_chan.cpp \
		../../${MPDir}/server/sv_snapshot.cpp \
		../../${MPDir}/server/sv_world.cpp \
		\
		../../${MPDir}/qcommon/unzip.cpp \
		\
		../../${MPDir}/sys/snapvector.cpp \
		\
		../../${MPDir}/client/cl_cgame.cpp \
		../../${MPDir}/client/cl_cin.cpp \
		../../${MPDir}/client/cl_console.cpp \
		../../${MPDir}/client/cl_curl.cpp \
		../../${MPDir}/client/cl_demos.cpp \
		../../${MPDir}/client/cl_demos_auto.cpp \
		../../${MPDir}/client/cl_demos_cut.cpp \
		../../${MPDir}/client/cl_input.cpp \
		../../${MPDir}/client/cl_keys.cpp \
		../../${MPDir}/client/cl_main.cpp \
		../../${MPDir}/client/cl_mme.cpp \
		../../${MPDir}/client/cl_net_chan.cpp \
		../../${MPDir}/client/cl_parse.cpp \
		../../${MPDir}/client/cl_scrn.cpp \
		../../${MPDir}/client/cl_ui.cpp \
		../../${MPDir}/client/FXExport.cpp \
		../../${MPDir}/client/FxPrimitives.cpp \
		../../${MPDir}/client/FxScheduler.cpp \
		../../${MPDir}/client/FxSystem.cpp \
		../../${MPDir}/client/FxTemplate.cpp \
		../../${MPDir}/client/FxUtil.cpp \
		../../${MPDir}/client/snd_ambient.cpp \
		../../${MPDir}/client/snd_dma.cpp \
		../../${MPDir}/client/snd_effects.cpp \
		../../${MPDir}/client/snd_main.cpp \
		../../${MPDir}/client/snd_mem.cpp \
		../../${MPDir}/client/snd_mix.cpp \
		../../${MPDir}/client/snd_mme.cpp \
		\
		../../${MPDir}/qcommon/cm_terrainmap.cpp \
		../../${MPDir}/qcommon/CNetProfile.cpp \
		../../${MPDir}/qcommon/exe_headers.cpp \
		../../${MPDir}/qcommon/hstring.cpp \
		\
		../../${MPDir}/sdl/sdl_input.cpp \
		../../${MPDir}/sdl/sdl_snd.cpp \
		../../${MPDir}/sys/sys_unix.cpp \
		../../${MPDir}/sys/sys_net.cpp \
		\
		../../${MPDir}/zlib/adler32.c \
		../../${MPDir}/zlib/compress.c \
		../../${MPDir}/zlib/crc32.c \
		../../${MPDir}/zlib/deflate.c \
		../../${MPDir}/zlib/gzclose.c \
		../../${MPDir}/zlib/gzlib.c \
		../../${MPDir}/zlib/gzread.c \
		../../${MPDir}/zlib/gzwrite.c \
		../../${MPDir}/zlib/infback.c \
		../../${MPDir}/zlib/inffast.c \
		../../${MPDir}/zlib/inflate.c \
		../../${MPDir}/zlib/inftrees.c \
		../../${MPDir}/zlib/ioapi.c \
		../../${MPDir}/zlib/trees.c \
		../../${MPDir}/zlib/uncompr.c \
		../../${MPDir}/zlib/zutil.c \
		
LOCAL_SRC_FILES += $(JK3_SRC) $(JK3MP_ANDROID_SRC)

include $(BUILD_SHARED_LIBRARY)








