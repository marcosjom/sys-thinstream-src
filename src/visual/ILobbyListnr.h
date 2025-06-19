//
//  ILobbyListnr.h
//  thinstream
//
//  Created by Marcos Ortega on 8/3/19.
//

#ifndef ILobbyListnr_h
#define ILobbyListnr_h

#include "visual/IScenesListener.h"
#include "visual/AUSceneColsStack.h"

class ILobbyListnr;

typedef enum ENLobbyPanel_ {
	//ENLobbyPanel_Behind = 0,
	ENLobbyPanel_MainLeft = 0,
	ENLobbyPanel_MainCenter,
	ENLobbyPanel_Count
} ENLobbyPanel;

typedef enum ENLobbyStack_ {
	ENLobbyStack_Home = 0,
	ENLobbyStack_History,
	ENLobbyStack_Count
} ENLobbyStack;

typedef enum ENLobbyMeetingLoadMode_ {
	ENLobbyMeetingLoadMode_Default = 0,
	ENLobbyMeetingLoadMode_Count
} ENLobbyMeetingLoadMode;

typedef enum ENLobbyActionAfter_ {
	ENLobbyActionAfter_Open = 0,	//Open the content created or modified
	ENLobbyActionAfter_PopBack,		//Pop back
	ENLobbyActionAfter_Count,
} ENLobbyActionAfter;

typedef enum ENVisualStoreProductBit_ {
	ENVisualStoreProductBit_None		= 0,
	ENVisualStoreProductBit_SubscFull	= 1,
	ENVisualStoreProductBit_SubscSign	= 2,
	ENVisualStoreProductBit_Voucher		= 4,
	ENVisualStoreProductBits_SubsAny	= (ENVisualStoreProductBit_SubscFull | ENVisualStoreProductBit_SubscSign),
	ENVisualStoreProductBits_Any		= (ENVisualStoreProductBits_SubsAny | ENVisualStoreProductBit_Voucher)
} ENVisualStoreProductBit;

typedef enum ENVisualHelp_ {
	ENVisualHelp_Count = 0
} ENVisualHelp;

typedef enum ENVisualHelpBit_ {
	ENVisualHelpBits_All = 0
} ENVisualHelpBit;

typedef struct STLobbyRoot_ {
	STTSClient*			coreClt;
	SI32				iScene;
	IScenesListener*	scenes;
	//Extras
	ILobbyListnr*		lobby;
} STLobbyRoot;

typedef struct STLobbyColRoot_ {
	STTSClient*			coreClt;
	SI32				iScene;
	IScenesListener*	scenes;
	ILobbyListnr*		lobby;
	//Extras
	ENLobbyPanel		colGrp;
} STLobbyColRoot;

class ILobbyListnr {
	public:
		virtual ~ILobbyListnr(){
			//
		}
		virtual void	lobbyFocusBehindPanel() = 0;
		virtual void	lobbyHiddeBehindPanel() = 0;
		virtual void	lobbyFocusStack(const ENLobbyStack iStack, const BOOL allowPopAllAction) = 0;
		virtual void	lobbyBack(const BOOL ignoreSubstacks) = 0;
		virtual BOOL	lobbyTopBarGetStatusDef(STTSSceneBarStatus* dst) = 0;
		virtual UI32	lobbyTopBarGetStatusRightIconMask() = 0;
		virtual void	lobbyTopBarTouched() = 0;
		virtual void	lobbyTopBarSubstatusTouched() = 0;
		virtual void	lobbyPushTermsAndConds() = 0;
		virtual void	lobbyPushPrivacy() = 0;
		virtual void	lobbyPushHome() = 0;
		//
		virtual void	lobbyExecIconCmd(const SI32 iIcon) = 0;
		virtual void	lobbyApplySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz) = 0;
};

#endif /* ILobbyListnr_h */
