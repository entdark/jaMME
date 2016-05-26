
LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)


LOCAL_MODULE    := rd-jamme_arm


LOCAL_CFLAGS := $(JK3_BASE_CFLAGS) -DHAVE_GLES
ifeq ($(TARGET_ARCH),x86)
    LOCAL_CFLAGS := $(LOCAL_CFLAGS) -march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32 -DX86_OR_64
endif
ifeq ($(TARGET_ARCH),x86_64)
    LOCAL_CFLAGS := $(LOCAL_CFLAGS) -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -DX86_OR_64
endif
LOCAL_CPPFLAGS := $(JK3_BASE_CPPFLAGS) 

LOCAL_LDLIBS := $(JK3_BASE_LDLIBS)

LOCAL_LDLIBS +=  -lGLESv1_CM -lEGL -llog -lz
LOCAL_SHARED_LIBRARIES := SDL2 jamme #jk3mp is only needed to see the androidSwapped, massive hack

LOCAL_C_INCLUDES := $(JK3_BASE_C_INCLUDES) $(TOP_DIR)/libpng $(LOCAL_PATH)/$(MPDir)/rd-vanilla $(TOP_DIR) $(LOCAL_PATH)/android/jni/SDL2/include

JK3_SRC = \
	../../${MPDir}/rd-vanilla/G2_API.cpp \
	../../${MPDir}/rd-vanilla/G2_bolts.cpp \
	../../${MPDir}/rd-vanilla/G2_bones.cpp \
	../../${MPDir}/rd-vanilla/G2_misc.cpp \
	../../${MPDir}/rd-vanilla/G2_surfaces.cpp \
	../../${MPDir}/rd-vanilla/tr_arioche.cpp \
	../../${MPDir}/rd-vanilla/tr_backend.cpp \
	../../${MPDir}/rd-vanilla/tr_bsp.cpp \
	../../${MPDir}/rd-vanilla/tr_bloom.cpp \
	../../${MPDir}/rd-vanilla/tr_cmds.cpp \
	../../${MPDir}/rd-vanilla/tr_curve.cpp \
	../../${MPDir}/rd-vanilla/tr_font.cpp \
	../../${MPDir}/rd-vanilla/tr_framebuffer.cpp \
	../../${MPDir}/rd-vanilla/tr_ghoul2.cpp \
	../../${MPDir}/rd-vanilla/tr_image.cpp \
	../../${MPDir}/rd-vanilla/tr_init.cpp \
	../../${MPDir}/rd-vanilla/tr_light.cpp \
	../../${MPDir}/rd-vanilla/tr_main.cpp \
	../../${MPDir}/rd-vanilla/tr_marks.cpp \
	../../${MPDir}/rd-vanilla/tr_mesh.cpp \
	../../${MPDir}/rd-vanilla/tr_mme.cpp \
	../../${MPDir}/rd-vanilla/tr_mme_avi.cpp \
	../../${MPDir}/rd-vanilla/tr_mme_common.cpp \
	../../${MPDir}/rd-vanilla/tr_mme_pipe.cpp \
	../../${MPDir}/rd-vanilla/tr_mme_sse2.cpp \
	../../${MPDir}/rd-vanilla/tr_mme_stereo.cpp \
	../../${MPDir}/rd-vanilla/tr_model.cpp \
	../../${MPDir}/rd-vanilla/tr_noise.cpp \
	../../${MPDir}/rd-vanilla/tr_quicksprite.cpp \
	../../${MPDir}/rd-vanilla/tr_scene.cpp \
	../../${MPDir}/rd-vanilla/tr_shade.cpp \
	../../${MPDir}/rd-vanilla/tr_shade_calc.cpp \
	../../${MPDir}/rd-vanilla/tr_shader.cpp \
	../../${MPDir}/rd-vanilla/tr_shadows.cpp \
	../../${MPDir}/rd-vanilla/tr_sky.cpp \
	../../${MPDir}/rd-vanilla/tr_subs.cpp \
	../../${MPDir}/rd-vanilla/tr_surface.cpp \
	../../${MPDir}/rd-vanilla/tr_surfacesprites.cpp \
	../../${MPDir}/rd-vanilla/tr_terrain.cpp \
	../../${MPDir}/rd-vanilla/tr_world.cpp \
	../../${MPDir}/rd-vanilla/tr_WorldEffects.cpp \
	../../${MPDir}/qcommon/GenericParser2.cpp \
	../../${MPDir}/qcommon/matcomp.cpp \
	../../${MPDir}/qcommon/q_math.cpp \
	../../${MPDir}/qcommon/q_shared.cpp \
	../../${MPDir}/android/android_glimp.cpp \
	\
	../../${MPDir}/libpng/png.c \
	../../${MPDir}/libpng/pngerror.c \
	../../${MPDir}/libpng/pngget.c \
	../../${MPDir}/libpng/pngmem.c \
	../../${MPDir}/libpng/pngpread.c \
	../../${MPDir}/libpng/pngread.c \
	../../${MPDir}/libpng/pngrio.c \
	../../${MPDir}/libpng/pngrtran.c \
	../../${MPDir}/libpng/pngrutil.c \
	../../${MPDir}/libpng/pngset.c \
	../../${MPDir}/libpng/pngtrans.c \
	../../${MPDir}/libpng/pngwio.c \
	../../${MPDir}/libpng/pngwrite.c \
	../../${MPDir}/libpng/pngwtran.c \
	../../${MPDir}/libpng/pngwutil.c \
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
	\
	../../${MPDir}/jpeg-8c/jaricom.c \
	../../${MPDir}/jpeg-8c/jcapimin.c \
	../../${MPDir}/jpeg-8c/jcapistd.c \
	../../${MPDir}/jpeg-8c/jcarith.c \
	../../${MPDir}/jpeg-8c/jccoefct.c \
	../../${MPDir}/jpeg-8c/jccolor.c \
	../../${MPDir}/jpeg-8c/jcdctmgr.c \
	../../${MPDir}/jpeg-8c/jchuff.c \
	../../${MPDir}/jpeg-8c/jcinit.c \
	../../${MPDir}/jpeg-8c/jcmainct.c \
	../../${MPDir}/jpeg-8c/jcmarker.c \
	../../${MPDir}/jpeg-8c/jcmaster.c \
	../../${MPDir}/jpeg-8c/jcomapi.c \
	../../${MPDir}/jpeg-8c/jcparam.c \
	../../${MPDir}/jpeg-8c/jcprepct.c \
	../../${MPDir}/jpeg-8c/jcsample.c \
	../../${MPDir}/jpeg-8c/jctrans.c \
	../../${MPDir}/jpeg-8c/jdapimin.c \
	../../${MPDir}/jpeg-8c/jdapistd.c \
	../../${MPDir}/jpeg-8c/jdarith.c \
	../../${MPDir}/jpeg-8c/jdatadst.c \
	../../${MPDir}/jpeg-8c/jdatasrc.c \
	../../${MPDir}/jpeg-8c/jdcoefct.c \
	../../${MPDir}/jpeg-8c/jdcolor.c \
	../../${MPDir}/jpeg-8c/jddctmgr.c \
	../../${MPDir}/jpeg-8c/jdhuff.c \
	../../${MPDir}/jpeg-8c/jdinput.c \
	../../${MPDir}/jpeg-8c/jdmainct.c \
	../../${MPDir}/jpeg-8c/jdmarker.c \
	../../${MPDir}/jpeg-8c/jdmaster.c \
	../../${MPDir}/jpeg-8c/jdmerge.c \
	../../${MPDir}/jpeg-8c/jdpostct.c \
	../../${MPDir}/jpeg-8c/jdsample.c \
	../../${MPDir}/jpeg-8c/jdtrans.c \
	../../${MPDir}/jpeg-8c/jerror.c \
	../../${MPDir}/jpeg-8c/jfdctflt.c \
	../../${MPDir}/jpeg-8c/jfdctfst.c \
	../../${MPDir}/jpeg-8c/jfdctint.c \
	../../${MPDir}/jpeg-8c/jidctflt.c \
	../../${MPDir}/jpeg-8c/jidctfst.c \
	../../${MPDir}/jpeg-8c/jidctint.c \
	../../${MPDir}/jpeg-8c/jmemmgr.c \
	../../${MPDir}/jpeg-8c/jmemnobs.c \
	../../${MPDir}/jpeg-8c/jquant1.c \
	../../${MPDir}/jpeg-8c/jquant2.c \
	../../${MPDir}/jpeg-8c/jutils.c \
	\
	../../${MPDir}/png/rpng.cpp \
	
LOCAL_SRC_FILES += $(JK3_SRC) 


include $(BUILD_SHARED_LIBRARY)








