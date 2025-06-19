
#-------------------------
# PROJECT
#-------------------------

$(eval $(call nbCall,nbInitProject))

NB_PROJECT_NAME             := thinstream

NB_PROJECT_CFLAGS           := -fPIC

NB_PROJECT_CXXFLAGS         := -fPIC -std=c++11

NB_PROJECT_INCLUDES         := \
   src \
   src/core \
   ../../../CltNicaraguaBinary/sys-nbframework/lib-nbframework-src/include \
   ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-src/include \
   ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-media-src/include \
   ../../../CltNicaraguaBinary/sys-auframework/lib-auframework-app-src/include

#-------------------------
# TARGET
#-------------------------

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := thinstream-core

NB_TARGET_PREFIX            := lib

NB_TARGET_SUFIX             := .a

NB_TARGET_TYPE              := static

NB_TARGET_DEPENDS           := nbframework

NB_TARGET_FLAGS_ENABLES     += NB_LIB_THINSTREAM

#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := core

NB_CODE_GRP_FLAGS_REQUIRED  += NB_LIB_THINSTREAM

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_SSL

NB_CODE_GRP_SRCS            := \
    src/core/client/TSClient.c \
    src/core/client/TSClientChannel.c \
    src/core/client/TSClientChannelRequest.c \
    src/core/config/TSCfg.c \
    src/core/config/TSCfgContext.c \
    src/core/config/TSCfgLocale.c \
    src/core/config/TSCfgThreads.c \
    src/core/config/TSCfgApplePushServices.c \
    src/core/config/TSCfgCypher.c \
    src/core/config/TSCfgFilesystem.c \
    src/core/config/TSCfgFirebaseCM.c \
    src/core/config/TSCfgMail.c \
    src/core/config/TSCfgOutServer.c \
    src/core/config/TSCfgRestApiClt.c \
    src/core/config/TSCfgServer.c \
    src/core/config/TSCfgTelemetry.c \
    src/core/config/TSCfgStream.c \
    src/core/config/TSCfgStreamVersion.c \
    src/core/logic/TSCertificate.c \
    src/core/logic/TSClientRoot.c \
    src/core/logic/TSDevice.c \
    src/core/logic/TSDeviceLocal.c \
    src/core/logic/TSFile.c \
    src/core/logic/TSHello.c \
    src/core/logic/TSIdentityLocal.c \
    src/core/logic/TSNotifications.c \
    src/core/logic/TSStreamDefs.c \
    src/core/logic/TSStreamUnitBuff.c \
    src/core/logic/TSStreamUnit.c \
    src/core/logic/TSStreamBuilder.c \
    src/core/logic/TSStreamsSessionMsg.c \
    src/core/logic/TSStreamsStorage.c \
    src/core/logic/TSStreamsStorageLoader.c \
    src/core/logic/TSStreamsStorageStats.c \
    src/core/logic/TSStreamStorage.c \
    src/core/logic/TSStreamStorageFiles.c \
    src/core/logic/TSStreamStorageFilesGrp.c \
    src/core/logic/TSStreamVersionStorage.c \
    src/core/logic/TSClientRequester.c \
    src/core/logic/TSClientRequesterDevice.c \
    src/core/logic/TSClientRequesterConn.c \
    src/core/logic/TSClientRequesterConnStats.c \
    src/core/server/TSApiRestClt.c \
    src/core/server/TSApplePushServicesClt.c \
    src/core/server/TSFirebaseCMClt.c \
    src/core/server/TSMailClt.c \
    src/core/server/TSNotifsList.c \
    src/core/server/TSServer.c \
    src/core/server/TSServerStreams.c \
    src/core/server/TSServerSessionReq.c \
    src/core/server/TSServerStreamPath.c \
    src/core/server/TSServerStreamReq.c \
    src/core/server/TSServerStreamReqConn.c \
    src/core/server/TSServerLogicClt.c \
    src/core/server/TSServerLstnr.c \
    src/core/server/TSServerStats.c \
    src/core/TSContext.c \
    src/core/TSThread.c \
    src/core/TSThreadStats.c \
    src/core/TSCypher.c \
    src/core/TSCypherData.c \
    src/core/TSCypherJob.c \
    src/core/TSFilesystem.c \
    src/core/TSRequestsMap.c

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))


#-------------------------
# TARGET
#-------------------------

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := thinstream-visual

NB_TARGET_PREFIX            := lib

NB_TARGET_SUFIX             := .a

NB_TARGET_TYPE              := static

NB_TARGET_DEPENDS           := auframework-app auframework-media auframework

NB_TARGET_FLAGS_ENABLES     += NB_LIB_THINSTREAM

#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := visual

NB_CODE_GRP_FLAGS_REQUIRED  +=

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_THINSTREAM

NB_CODE_GRP_SRCS            := \
    src/visual/AUSceneIntro.cpp \
    src/visual/gui/TSVisualIcons.cpp \
    src/visual/gui/TSVisualStatus.cpp \
    src/visual/AUSceneCol.cpp \
    src/visual/home/TSVisualHome.cpp \
    src/visual/TSFonts.cpp \
    src/visual/TSVisualPch.cpp \
    src/visual/AUSceneColsStack.cpp \
    src/visual/gui/TSVisualTag.cpp \
    src/visual/gui/TSRowsTree.cpp \
    src/visual/AUSceneBarBtm.cpp \
    src/visual/AUScenesAdmin.cpp \
    src/visual/AUSceneBarStatus.cpp \
    src/visual/AUScenesTransition.cpp \
    src/visual/AUSceneBarTop.cpp \
    src/visual/TSColors.cpp \
    src/visual/viewer/TSVisualViewer.cpp \
    src/visual/TSVisualRegister.cpp \
    src/visual/AUSceneLobby.cpp \
    src/visual/gui/TSVisualRowsGrp.cpp \
    src/visual/home/TSVisualHomeSourceRow.cpp \
    src/visual/TSLogoMetrics.cpp \
    src/visual/viewer/TSVisualViewerStream.cpp \
    src/visual/gui/TSColumnOrchestor.cpp \
    src/visual/AUSceneColsPanel.cpp

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))

#-------------------------
# TARGET
#-------------------------

#Specific OS files
ifeq (,$(findstring Android,$(NB_CFG_HOST)))

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := thinstream-server

NB_TARGET_PREFIX            :=

NB_TARGET_SUFIX             :=

NB_TARGET_TYPE              := exe

NB_TARGET_DEPENDS           := thinstream-core

NB_TARGET_FLAGS_ENABLES     += NB_LIB_THINSTREAM

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
else
ifeq ($(OS),Windows_NT)
  #Windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    #Linux
  endif
  ifeq ($(UNAME_S),Darwin)
    #OSX
    NB_TARGET_FRAMEWORKS    += Foundation Security
  endif
endif
endif


#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := all

NB_CODE_GRP_FLAGS_REQUIRED  +=

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_THINSTREAM

NB_CODE_GRP_SRCS            := \
    src/server/main.c

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))

endif










#-------------------------
# TARGET
#-------------------------

#Specific OS files
ifeq (,$(findstring Android,$(NB_CFG_HOST)))

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := thinstream-stress

NB_TARGET_PREFIX            :=

NB_TARGET_SUFIX             :=

NB_TARGET_TYPE              := exe

NB_TARGET_DEPENDS           := thinstream-core

NB_TARGET_FLAGS_ENABLES     += NB_LIB_THINSTREAM

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
else
ifeq ($(OS),Windows_NT)
  #Windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    #Linux
  endif
  ifeq ($(UNAME_S),Darwin)
    #OSX
    NB_TARGET_FRAMEWORKS    += Foundation Security
  endif
endif
endif


#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := all

NB_CODE_GRP_FLAGS_REQUIRED  +=

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_THINSTREAM

NB_CODE_GRP_SRCS            := \
    src/stress/main.c \
    src/stress/TSStressClient.c

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))

endif









#-------------------------
# TARGET
#-------------------------

#Specific OS files
ifeq (,$(findstring Android,$(NB_CFG_HOST)))

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := thinstream-monitor

NB_TARGET_PREFIX            :=

NB_TARGET_SUFIX             :=

NB_TARGET_TYPE              := exe

NB_TARGET_DEPENDS           := thinstream-core

NB_TARGET_FLAGS_ENABLES     += NB_LIB_THINSTREAM

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
else
ifeq ($(OS),Windows_NT)
  #Windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    #Linux
  endif
  ifeq ($(UNAME_S),Darwin)
    #OSX
    NB_TARGET_FRAMEWORKS    += Foundation Security
  endif
endif
endif


#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := all

NB_CODE_GRP_SRCS            := \
    src/monitor/main.c

NB_CODE_GRP_FLAGS_REQUIRED  +=

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_THINSTREAM

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))

endif









#-------------------------
# TARGET
#-------------------------

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := thinstream

NB_TARGET_PREFIX            := lib

NB_TARGET_SUFIX             := .so

NB_TARGET_TYPE              := shared

NB_TARGET_DEPENDS           := thinstream-visual thinstream-core

NB_TARGET_LIBS              := log GLESv1_CM OpenSLES android

NB_TARGET_FLAGS_ENABLES     += NB_LIB_THINSTREAM

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
  NB_TARGET_SHARED_LIBS     += pdfium-prebuilt icuuc-prebuilt chrome_zlib-prebuilt cpp-prebuilt
endif

#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            :=

NB_CODE_GRP_INCLUDES        += \
   projects/android-studio/app/src/main/jni/

NB_CODE_GRP_SRCS            := \
   projects/android-studio/app/src/main/jni/org_thinstream_android_AppNative.cpp

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
  $(eval $(call nbCall,nbBuildCodeGrpRules))
endif

#-------------------------
# TARGET RULES
#-------------------------

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
  $(eval $(call nbCall,nbBuildTargetRules))
endif

#-------------------------
# PROJECT RULES
#-------------------------

$(eval $(call nbCall,nbBuildProjectRules))
