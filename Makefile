
LOCAL_PATH := $(call my-dir)

#-------------
# Reference to prebuilt lib
#-------------
include $(CLEAR_VARS)
LOCAL_MODULE := pdfium-prebuilt
LOCAL_SRC_FILES := ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-app-src/src/pdfium_android/$(TARGET_ARCH_ABI)/lib.unstripped/libpdfium.cr.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := icuuc-prebuilt
LOCAL_SRC_FILES := ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-app-src/src/pdfium_android/$(TARGET_ARCH_ABI)/lib.unstripped/libicuuc.cr.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := chrome_zlib-prebuilt
LOCAL_SRC_FILES := ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-app-src/src/pdfium_android/$(TARGET_ARCH_ABI)/lib.unstripped/libchrome_zlib.cr.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := cpp-prebuilt
LOCAL_SRC_FILES := ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-app-src/src/pdfium_android/$(TARGET_ARCH_ABI)/lib.unstripped/libc++.cr.so
include $(PREBUILT_SHARED_LIBRARY)

#Configure
NB_CFG_PRINT_INTERNALS := 1
NB_CFG_PRINT_INFO      := 0

#Import functions
include ../../../CltNicaraguaBinary/sys-nbframework/lib-nbframework-src/MakefileFuncs.mk

#Init workspace
$(eval $(call nbCall,nbInitWorkspace))

#Import projects
include ../../../CltNicaraguaBinary/sys-nbframework/lib-nbframework-src/MakefileProject.mk

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
  include ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-src/MakefileProject.mk
endif

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
  include ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-media-src/MakefileProject.mk
endif

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
  include ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-app-src/MakefileProject.mk
endif

#Project
include MakefileProject.mk

#Build workspace
$(eval $(call nbCall,nbBuildWorkspaceRules))


#Clean rule
clean:
	rm -r bin
	rm -r tmp

