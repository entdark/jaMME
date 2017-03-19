LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	version.c \
	fixed.c \
	bit.c \
	timer.c \
	stream.c \
	frame.c  \
	synth.c \
	decoder.c \
	layer12.c \
	layer3.c \
	huffman.c

LOCAL_SHARED_LIBRARIES := 

LOCAL_MODULE:= libmad

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/android 

LOCAL_CFLAGS := \
    -DHAVE_CONFIG_H

# we want more accurate FPM logic
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DFPM_INTEL
else ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DFPM_64BIT
else
    LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DFPM_DEFAULT
endif

include $(BUILD_STATIC_LIBRARY)
