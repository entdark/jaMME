#libmad

LIB_VERSION:=libmad

LOCAL_PATH:= $(call my-dir)
LIB_ROOT_REL:= ../../$(LIB_VERSION)
LIB_ROOT_ABS:= $(LOCAL_PATH)/../../$(LIB_VERSION)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -DHAVE_CONFIG_H -DFPM_DEFAULT

LOCAL_SRC_FILES := \
 $(LIB_ROOT_REL)/version.c \
 $(LIB_ROOT_REL)/fixed.c \
 $(LIB_ROOT_REL)/bit.c \
 $(LIB_ROOT_REL)/timer.c \
 $(LIB_ROOT_REL)/stream.c \
 $(LIB_ROOT_REL)/frame.c \
 $(LIB_ROOT_REL)/synth.c \
 $(LIB_ROOT_REL)/decoder.c \
 $(LIB_ROOT_REL)/layer12.c \
 $(LIB_ROOT_REL)/layer3.c \
 $(LIB_ROOT_REL)/huffman.c 
 

LOCAL_C_INCLUDES += \
 $(LIB_ROOT_ABS)/ \
 $(LIB_ROOT_ABS)/android

LOCAL_LDLIBS := 

LOCAL_MODULE := libmad

include $(BUILD_STATIC_LIBRARY)
