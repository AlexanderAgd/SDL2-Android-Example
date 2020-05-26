LOCAL_PATH := $(call my-dir)

# $(info $$var is [${var}])

include $(CLEAR_VARS)
LOCAL_MODULE := hidapi
LOCAL_SRC_FILES := $(LOCAL_PATH)/lib/$(APP_ABI)/libhidapi.so
#LOCAL_EXPORT_C_INCLUDES :=
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SDL2_image
LOCAL_SRC_FILES := $(LOCAL_PATH)/lib/$(APP_ABI)/libSDL2_image.so
#LOCAL_EXPORT_C_INCLUDES :=
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SDL2
LOCAL_SRC_FILES := $(LOCAL_PATH)/lib/$(APP_ABI)/libSDL2.so
#LOCAL_EXPORT_C_INCLUDES :=
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
# DO NOT CHANGE "main" - libmain.so  object is "Executable"
LOCAL_MODULE := main

#LOCAL_CFLAGS :=
#LOCAL_LDFLAGS := 
#LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -Wl,--rpath=.
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
       /home/user/ANDROID/SDL2/SDL2-2.0.12/include \
       /home/user/ANDROID/SDL2/SDL2_image-2.0.5

# Add your <name>.c files here
LOCAL_SRC_FILES := $(LOCAL_PATH)/parallax4.c

LOCAL_SHARED_LIBRARIES := hidapi SDL2 SDL2_image
include $(BUILD_SHARED_LIBRARY)
