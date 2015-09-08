
LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE 		:= libcurl
#LOCAL_SRC_FILES := libcurl.a
#
#include $(PREBUILT_STATIC_LIBRARY)

# TODO: Put this back when we get OpenSSL in the build
#include $(CLEAR_VARS)
#LOCAL_MODULE    := curl
#LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libcurl.a
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := mwp-jni
LOCAL_SRC_FILES := mwp-jni.cpp

LOCAL_C_INCLUDES := ../../cmake-build/android/core/
LOCAL_C_INCLUDES += ../cpp/core/
LOCAL_C_INCLUDES += ../cpp/protocols/

LOCAL_STATIC_LIBRARIES    := libmwp
#LOCAL_STATIC_LIBRARIES    += libcurl

LOCAL_LDLIBS    := -llog -lz
LOCAL_WHOLE_STATIC_LIBRARIES := libcurl

include $(BUILD_SHARED_LIBRARY)

