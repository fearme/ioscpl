
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE 		:= libcurl
LOCAL_SRC_FILES := libcurl.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := mobilewebprint
LOCAL_SRC_FILES := mobilewebprint-jni.cpp
LOCAL_C_INCLUDES := ../../deliveries/include
LOCAL_C_INCLUDES += ../cpp/core/
LOCAL_C_INCLUDES += ../cpp/aprotocols/

LOCAL_STATIC_LIBRARIES    := libmwp
LOCAL_STATIC_LIBRARIES    += libcurl

LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)

