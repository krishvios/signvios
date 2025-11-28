CUR_DIR := $(call my-dir)
ANDROID_DIR := $(CUR_DIR)/Libraries/android
include $(call all-subdir-makefiles)
include $(ANDROID_DIR)/Android.mk
