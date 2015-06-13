TOP_DIR := $(call my-dir)
LOCAL_PATH := $(call my-dir)
MPDir = codemp

JK3_BASE_CFLAGS =  -O1 -DHAVE_GLES -DFINAL_BUILD -fexceptions  -Wall -Wno-write-strings -Wno-comment   -fno-caller-saves -fno-tree-vectorize -Wno-unused-but-set-variable
JK3_BASE_CPPFLAGS =  -fvisibility-inlines-hidden -Wno-invalid-offsetof 

JK3_BASE_LDLIBS = -llog

include $(TOP_DIR)/SDL2/Android.mk
include $(TOP_DIR)/libmad/Android.mk

include $(TOP_DIR)/Android_client.mk
include $(TOP_DIR)/Android_game.mk
include $(TOP_DIR)/Android_cgame.mk
include $(TOP_DIR)/Android_ui.mk
include $(TOP_DIR)/Android_gles.mk


