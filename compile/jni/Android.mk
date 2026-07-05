LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := shield
LOCAL_MODULE_FILENAME := libshield
LOCAL_SRC_FILES := main.cpp
# Menambahkan -fexceptions di ujung baris ini
LOCAL_CPPFLAGS := -Os -ffunction-sections -fdata-sections -std=c++17 -fvisibility=hidden -fexceptions
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)