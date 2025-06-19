//
//  AUScenesAdmin.hpp
//  thinstream
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifndef AUScenesAdmin_hpp
#define AUScenesAdmin_hpp

#include "AUEscenaTopItf.h"
#include "IScenesListener.h"
#include "core/client/TSClient.h"

#define POPUP_WIDTH_MAX_INCHES		3.0f

class AUScenesAdmin;

typedef struct STScenesPop_ {
	BOOL				onFocus;
	BOOL				hideBtnAdded;
	ENIMsgboxSize		sizeH;
	ENIMsgboxSize		sizeV;
	AUScenesAdmin*		admin;
	struct {
		AUTextura*		working;
		AUTextura*		sucess;
		AUTextura*		warn;
		AUTextura*		error;
	} texs;
	ENScenePopMode		mode;
	STNBString			title;
	AUEscenaSprite*		icon;
	float				iconRot;
	AUIMsgBox*			msgBox;
	//
	IPopListener*		lstnr;
	AUObjeto*			lstnObj;
	void*				lstnrParam;
	SI32				retainCount;
} STScenesPop;

typedef struct STScenesRequestPop {
	BOOL				requesting;
	BOOL				onFocus;
	float				secsCur;
	float				secsWaitDur;
	UI64				reqId;
	AUScenesAdmin*		admin;
	void*				popRef;
	ENReqPopHideMode	hideMode;
	IReqPopListener*	lstnr;
	AUObjeto*			lstnObj;
	void*				lstnrParam;
} STScenesRequestPop;

class AUScenesAdmin: public AUAppEscenasAdmin, public IScenesListener
#ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
, public IEscuchadorGestionSonidos
#endif
, public AUAppStateListener
, public AUAppOpenUrlListener
, public NBAnimador
, public IEscuchadorTouchEscenaObjeto
, public IEscuchadorTecladoVisual
, public AUEscenaTopItf
, public IEscuchadorIMsgBox
, public IPopListener
{
public:
	AUScenesAdmin(AUAppI* app, STTSClient* coreClt, const SI32 iScene, const char* prefijoPaquetes, const STAppScenasPkgLoadDef* paquetesCargar, const SI32 conteoPaquetesCargar);
	virtual ~AUScenesAdmin();
	//
	bool					escenasTieneAccionesPendiente();
	void					escenasEjecutaAccionesPendientes(); //las acciones (como carga de nivel) se acumulan como pendiente hasta que se mande a llamar a esta funcion en un modo-seguro.
	bool					tickProcesoCarga(float segsTranscurridos);
	//Top Itf
	BOOL					topShowQuickTipTextAtViewPort(const STNBPointI viewPortPos, const char* text, AUTextura* iconLeft, AUTextura* iconRight, const float secsShow, const ENEscenaTipAim aimDir);
	BOOL					topAddObject(AUEscenaObjeto* object);
	//Energy saving
	float					getSecsWithSameContent();
	//
	UI32					getLastOpenUrlPathSeq(STNBString* dstFilePath, STNBString* dstFileData, STNBString* dstUsrData, const BOOL clearValue);
	//
	void					setViewPortTransitionBoxFocus(const STNBAABox box);
	UI32					scenesStackSize(void);
	void					loadInfoSequence(const ENSceneAction action, const ENSceneInfoMode mode, const ENSceneTransitionType transitionType, const float transitionSecsDur);
	void					loadLobby(const ENSceneAction action, const ENSceneTransitionType transitionType, const float transitionSecsDur);
	void					loadPreviousScene(const ENSceneTransitionType transitionType, const float transitionSecsDur);
	void					reloadInitialContent(const ENSceneTransitionType transitionType, const float transitionSecsDur);
	//
	void					appStateOnCreate(AUAppI* app);
	void					appStateOnDestroy(AUAppI* app);	//applicationWillTerminate
	void					appStateOnStart(AUAppI* app);	//applicationDidBecomeActive
	void					appStateOnStop(AUAppI* app);	//applicationWillResignActive
	void					appStateOnResume(AUAppI* app);	//applicationWillEnterForeground
	void					appStateOnPause(AUAppI* app);	//applicationDidEnterBackground
	//
	bool					appOpenUrl(AUAppI* app, const STNBUrl* url, const void* usrData, const UI32 usrDataSz);
	bool					appOpenUrlImage(AUAppI* app, const STNBUrl* url, const SI32 rotDegFromIntended, const void* usrData, const UI32 usrDataSz);
	bool					appOpenFileData(AUAppI* app, const void* data, const UI32 dataSz, const void* usrData, const UI32 usrDataSz);
	bool					appOpenFileImageData(AUAppI* app, const void* data, const UI32 dataSz, const SI32 rotDegFromIntended, const void* usrData, const UI32 usrDataSz);
	bool					appOpenBitmap(AUAppI* app, const STNBBitmap* bmp, const SI32 rotDegFromIntended, const void* usrData, const UI32 usrDataSz);
	//void					appProcesarNotificacionLocal(const char* grp, const SI32 localId, const char* objTip, const SI32 objId);
	//
	void					escenasCargar();
	void					escenasLiberar();
	void					escenasLiberarRecursosSinUso();
	bool					escenasEnProcesoDeCarga();
	void					escenasQuitarDePrimerPlano();
	void					escenasColocarEnPrimerPlano();
	bool					escenasAnimandoSalida();
	void					escenasAnimarSalida();
	void					escenasAnimarEntrada();
	//Pops
	void					popShowInfo(const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content);
	void*					popCreate(const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content, IPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam, const char* hideBtnTxt, const ENScenePopBtnColor hideBtnColor, const ENIMsgboxSize sizeH = ENIMsgboxSize_Auto, const ENIMsgboxSize sizeV = ENIMsgboxSize_Auto);
	STNBSize				popGetContentSizeForSceneLyr(void* ref, const SI32 iScene, const ENGestorEscenaGrupo iGrp, STNBSize* dstBoxSz, STNBPoint* dstRelPos, NBMargenes* dstUnusableInnerContentSz);
	void					popSetCustomContentTop(void* ref, AUEscenaObjeto* content);
	void					popSetCustomContentBtm(void* ref, AUEscenaObjeto* content);
	void					popAddOption(void* ref, const char* optionId, const char* text, const ENScenePopBtnColor color, const ENIMsgboxBtnAction action);
	void					popUpdate(void* ref, const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content);
	void					popShow(void* ref);
	void					popShowAtRelY(void* ref, const float relY);
	void					popHide(void* ref);
	void					popRetain(void* ref);
	void					popRelease(void* ref);
	SI32					popVisiblesCount();
	//Request with pop
	UI64					addRequestWithPop(const ENReqPopHideMode hideMode, const char* title, const ENTSRequestId reqId, const char** paramsPairsAndNull, const void* stBody, const UI32 stBodySz, const STNBStructMap* stBodyMap, const UI64 secsWaitToRequest, const float secsWaitToShowPop, IReqPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam);
	UI64					addRequestWithPop(const ENReqPopHideMode hideMode, const char* title, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWaitToRequest, const float secsWaitToShowPop, IReqPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam);
	//Fil upload
	void					uploadStartFromPath(const char* filepath, const void* usrData, const UI32 usrDataSz);
	//
	void					msgboxOptionSelected(AUIMsgBox* obj, const char* optionId);
	void					msgboxCloseSelected(AUIMsgBox* obj);
	void					msgboxFocusObtained(AUIMsgBox* obj);
	void					msgboxFocusLost(AUIMsgBox* obj);
	//
	void					popOptionSelected(void* param, void* ref, const char* optionId);
	void					popFocusObtained(void* param, void* ref);
	void					popFocusLost(void* param, void* ref);
	//ViewPort
	void					puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after);
	//
	void					tickAnimacion(float segsTranscurridos);
	//TOUCHES
	void					touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
	void					touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
	void					touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
	//TECLADO
	void					tecladoVisualAltoModificado(const float altoPixeles);
	//TECLAS
	bool					teclaPresionada(SI32 codigoTecla);
	bool					teclaLevantada(SI32 codigoTecla);
	bool					teclaEspecialPresionada(SI32 codigoTecla);
	//
	bool					permitidoGirarPantalla();
#	ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
	virtual void			sndGrupoCambioHabilitado(const ENAudioGrupo grupo, const bool nuevoHabilitado);
#	endif
	void					anguloXYDispositivoCambiado(float angulo);
	//Record changed
	static void				recordChanged(void* param, const char* relRoot, const char* relPath);
	//
	AUOBJMETODOS_CLASESID_DEFINICION
	AUOBJMETODOS_CLASESNOMBRES_DEFINICION
	AUOBJMETODOS_CLONAR_DEFINICION
protected:
	AUAppI*					_app;
	STTSClient*				_coreClt;
	//Box
	STNBAABox				_lastBox;			//all window area
	STNBAABox				_lastBoxUsable;		//usable area (excluding top/bottom bars)
	//ESCENAS
	STNBAABox				_loadTransBoxFocus;
	ENSceneTransitionType 	_loadTransType;
	ENSceneAction			_loadAction;
	ENSceneAction			_loadActionCur;
	float					_loadTransSecsDur;
	AUAppTransicion*		_loadTransCur;
	bool					_loadInfoSequence;
	ENSceneInfoMode			_loadInfoSequenceMode;
	bool					_loadLobby;
	bool					_loadPrevScene;
	AUArregloMutable		_scenesStack; //AUAppEscena
	//Textures
	STNBArray				_texsPreload;	//AUTextura*, preloaded textures
	//Animator
	AUAnimadorObjetoEscena* _animator;
	//Top layer
	AUEscenaContenedor*		_topLayer;
	//CurrentIds
	struct {
		STNBString			deviceId;		//local device
	} _lastIds;
	//Error log
	/*struct {
		void*	popRef;
	} _errLog;*/
	//Requests progress popups
	struct {
		STNBArray			array; //STScenesPop*
		STNBThreadMutex		mutex;
		SI32				visibleCount;
	} _pops;
	//Portrait enforcers
	struct {
		UI32				mask;
		ENAppOrientationBit prefered;
		ENAppOrientationBit toApplyOnce;
		BOOL				allowAutoRotate;
	} _orientation;
	//Requests progress popups
	struct {
		STNBArray			pops; //STScenesRequestPop*
		STNBThreadMutex		mutex;
	} _reqsProg;
	//Quick-tip
	struct {
		AUEscenaContenedor*			layer;	//Layer
		AUEscenaSpriteElastico*		bg;		//Background
		AUEscenaContenedor*			txtLayer;
		AUEscenaSprite*				icoLeft;
		AUEscenaSprite*				icoRight;
		AUEscenaTexto*				txt;	//Text
	} _quickTip;
	//Upload
	struct {
		UI32				sequence;
		STNBString			filepath;	//empty when 'fileData' is populated
		STNBString			fileData;	//empty when 'filepath' is populated
		STNBString			usrData;
		//Background rotating image
		struct {
			BOOL			isActive;	//
			void*			popRef;
			SI32			rotFromIntended;
			STNBFileRef		file;
			STNBString		fileData;	//empty when 'filepath' is populated
			STNBString		usrData;
			STNBBitmap		rotBmp;
		} pend;
	} _upload;
	//Notifs
	/*struct {
		//Authorization
		struct {
			float			stateWaitSecs;
			ENAppNotifAuthState state;
		} auth;
		BOOL				firstTokenRequested;
		STNBString			tokenCur;
		float				tokenCurWaitSecs;
		STNBString			tokenSent;
		STNBString			tokenSending;
		UI64				tokenSendingReqId;
		float				tokenSendingWaitSecs;
	} _notifs;*/
	//
	void					privCalculateLimits();
	void					privOrganizeAll(const bool animar);
	//
	UI8						privLoadResourcesScenesTransition(SI32 indiceTransicion);
	void					privReleaseResourcesScenesTransition();
	SI32					privLoadPendingScene();
	inline void				privResetLoadingState();
	//Rotate image in background
	static SI64 			privRotateImageMethod_(STNBThread* t, void* param);
	//TSClientChannelReqCallbackFunc
	static void				privPopRelease(STScenesPop* obj);
	static void				privRequestPopRelease(STScenesRequestPop* obj);
	//static void			reqNotifTokenCallbackFunc(const STTSClientChannelRequest* req, void* lParam);
	static void				reqReceiptTokenCallbackFunc(const STTSClientChannelRequest* req, void* lParam);
	static void				reqCallbackFunc(const STTSClientChannelRequest* req, void* lParam);
	//Error log
	//void					privLogErrAskStart();
	//void					privLogErrSendStart();
	//static void			privLogErrReqCallbackFunc(const STTSClientChannelRequest* req, void* lParam);
	//
	void					privSyncOrientState();
};

#endif /* AUScenesAdmin_hpp */
