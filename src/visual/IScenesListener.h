//
//  AUScenesAdmin.hpp
//  thinstream
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifndef AUScenesListener_hpp
#define AUScenesListener_hpp

#include "core/client/TSClient.h"
#include "nb/core/NBStructMap.h"
#include "AUIMsgBox.h"
//
#include "visual/AUSceneIntroDefs.h"

#define TS_POPUPS_IN_SILENT_MODE			FALSE	//Hides all the titles and textContent of 'working' popups
#define TS_POPUPS_REQ_WAIT_SECS_BEFORE_REQ	0		//Wait this amount of seconds to send request (artifically added time to requests)
#define TS_POPUPS_REQ_WAIT_SECS_BEFORE_SHOW	3		//Wait this amount of seconds to show the 'requests' popus
#define TS_TRANSITION_PARTIAL_ANIM_DURATION	0.25f

typedef enum ENSceneTransitionType_ {
	ENSceneTransitionType_Ninguna = 0,
	ENSceneTransitionType_Captura,
	ENSceneTransitionType_Fondo,
	ENSceneTransitionType_Black,
	ENSceneTransitionType_White,
	ENSceneTransitionType_Cortina,
	ENSceneTransitionType_Logo
} ENSceneTransitionType;

typedef enum ENSceneAction_ {
	ENSceneAction_ReplaceAll = 0,	//The current scene stacks will be removed
	ENSceneAction_ReplaceTop,		//The current top-scene will be removed
	ENSceneAction_Pop,				//The new scene will be the precious top
	ENSceneAction_Push				//The current scene stacks will be kept
} ENSceneAction;

class IScenesListener;

typedef struct STVisualRoot_ {
	STTSClient*			coreClt;
	SI32				iScene;
	IScenesListener*	scenes;
} STVisualRoot;

//Bars
typedef enum ENSceneLobbyBar_ {
	ENSceneLobbyBar_TopTitle = 0,
	ENSceneLobbyBar_BottomCats,
	ENSceneLobbyBar_Count
} ENSceneLobbyBar;

typedef enum ENSceneLobbyBarPos_ {
	ENSceneLobbyBarPos_Top = 0,
	ENSceneLobbyBarPos_Bottom,
	ENSceneLobbyBarPos_Count
} ENSceneLobbyBarPos;

//

typedef enum ENScenePopBtnColor_ {
	ENScenePopBtnColor_Clear = 0,
	ENScenePopBtnColor_Main,
	ENScenePopBtnColor_Count
} ENScenePopBtnColor;


typedef enum ENScenePopMode_ {
	ENScenePopMode_TextOnly = 0,
	ENScenePopMode_Working,		//rotative icon
	ENScenePopMode_Sucess,		//sucess icon
	ENScenePopMode_Error,		//error icon
	ENScenePopMode_Warn,		//warning icon
	ENScenePopMode_Count
} ENScenePopMode;

typedef enum ENReqPopHideMode_ {
	ENReqPopHideMode_Manual = 0,
	ENReqPopHideMode_Auto,
	ENReqPopHideMode_Count
} ENReqPopHideMode;

class IPopListener {
public:
	virtual ~IPopListener(){
		//
	}
	//IPopListener
	virtual void	popOptionSelected(void* param, void* ref, const char* optionId) = 0;
	virtual void	popFocusObtained(void* param, void* ref) = 0;
	virtual void	popFocusLost(void* param, void* ref) = 0;
};


class IReqPopListener {
	public:
		virtual ~IReqPopListener(){
			//
		}
		//
		virtual void reqPopEnded(const UI64 reqId, void* param, const STTSClientChannelRequest* req) = 0;
};

class IScenesListener {
public:
	virtual ~IScenesListener(){
		//
	}
	//Energy saving
	virtual float	getSecsWithSameContent() = 0;
	//
	virtual UI32	getLastOpenUrlPathSeq(STNBString* dstFilePath, STNBString* dstFileData, STNBString* dstUsrData, const BOOL clearValue) = 0;
	//
	virtual void	setViewPortTransitionBoxFocus(const STNBAABox box) = 0;
	virtual UI32	scenesStackSize(void) = 0;
	virtual void	loadInfoSequence(const ENSceneAction action, const ENSceneInfoMode mode, const ENSceneTransitionType transitionType, const float transitionSecsDur) = 0;
	virtual void	loadLobby(const ENSceneAction action, const ENSceneTransitionType transitionType, const float transitionSecsDur) = 0;
	virtual void	loadPreviousScene(const ENSceneTransitionType transitionType, const float transitionSecsDur) = 0;
	virtual void	reloadInitialContent(const ENSceneTransitionType transitionType, const float transitionSecsDur) = 0;
	//Pops
	virtual void	popShowInfo(const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content) = 0;
	virtual void*	popCreate(const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content, IPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam, const char* hideBtnTxt, const ENScenePopBtnColor hideBtnColor, const ENIMsgboxSize sizeH = ENIMsgboxSize_Auto, const ENIMsgboxSize sizeV = ENIMsgboxSize_Auto) = 0;
	virtual STNBSize popGetContentSizeForSceneLyr(void* ref, const SI32 iScene, const ENGestorEscenaGrupo iGrp, STNBSize* dstBoxSz, STNBPoint* dstRelPos, NBMargenes* dstUnusableInnerContentSz) = 0;
	virtual void	popSetCustomContentTop(void* ref, AUEscenaObjeto* content) = 0;
	virtual void	popSetCustomContentBtm(void* ref, AUEscenaObjeto* content) = 0;
	virtual void	popAddOption(void* ref, const char* optionId, const char* text, const ENScenePopBtnColor color, const ENIMsgboxBtnAction action) = 0;
	virtual void	popUpdate(void* ref, const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content) = 0;
	virtual void	popShow(void* ref) = 0;
	virtual void	popShowAtRelY(void* ref, const float relY) = 0;
	virtual void	popHide(void* ref) = 0;
	virtual void	popRetain(void* ref) = 0;
	virtual void	popRelease(void* ref) = 0;
	virtual SI32	popVisiblesCount() = 0;
	//Request pops
	virtual UI64	addRequestWithPop(const ENReqPopHideMode hideMode, const char* title, const ENTSRequestId reqId, const char** paramsPairsAndNull, const void* stBody, const UI32 stBodySz, const STNBStructMap* stBodyMap, const UI64 secsWaitToRequest, const float secsWaitToShowPop, IReqPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam) = 0;
	virtual UI64	addRequestWithPop(const ENReqPopHideMode hideMode, const char* title, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWaitToRequest, const float secsWaitToShowPop, IReqPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam) = 0;
	//File upload
	virtual void	uploadStartFromPath(const char* filepath, const void* usrData, const UI32 usrDataSz) = 0;
};

#endif /* AUScenesAdmin_hpp */
