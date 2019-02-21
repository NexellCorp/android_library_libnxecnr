LOCAL_PATH := $(call my-dir)

#libnxecnr
include $(CLEAR_VARS)

LOCAL_MODULE :=libnxecnr
LOCAL_SRC_FILES := \
	nx-ecnr.c
LOCAL_C_INCLUDES += \
	system/core/include \
	$(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES += \
	libcutils \
	libutils

include $(BUILD_SHARED_LIBRARY)

#test-ecnr
include $(CLEAR_VARS)

COMMONFLAGS += -DAPI_LEVEL=$(PLATFORM_SDK_VERSION)
CXXFLAGS += $(COMMONFLAGS) $(CXXWARNS) -Os

# Compiler Flags
LOCAL_CFLAGS := $(CXXFLAGS) -fpermissive -w

LOCAL_MODULE := test-ecnr
LOCAL_SRC_FILES := \
	test-ecnr.c
LOCAL_C_INCLUDES += \
	system/core/include \
	frameworks/wilhelm/include \
	$(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES += \
	libcutils \
	libutils \
	libnxecnr \
	libmedia \
	libOpenSLES

include $(BUILD_EXECUTABLE)
