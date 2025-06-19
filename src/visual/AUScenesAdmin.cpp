//
//  AUScenesAdmin.cpp
//  thinstream
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUScenesAdmin.h"

#include "nb/2d/NBPng.h"
#include "nb/2d/NBJpeg.h"
#include "nb/compress/NBZlib.h"
#include "nb/compress/NBZDeflate.h"
#include "logic/TSClientRoot.h"
#include "visual/AUScenesAdmin.h"
#include "visual/AUSceneIntro.h"
#include "visual/AUSceneLobby.h"
#include "visual/AUScenesTransition.h"
#include "visual/TSFonts.h"
#include "core/logic/TSDevice.h"

#define TS_PDF_RENDERER_THREADS_AMOUNT		3		//amount of threads dedicated to rendering PDF docs
#define TS_NOTIFS_AUTH_VERIFY_WAIT			5.0f
#define TS_NOTIFS_AUTH_TOKEN_VERIFY_WAIT	10.0f
#define TS_NOTIFS_AUTH_TOKEN_SEND_WAIT		10.0f
#define TS_CONTACTS_REQ_WAIT				15.0f	//Wait betwwen request
#define TS_CONTACTS_REQ_WAIT_FOR_RETRY		5.0f	//Wait for retry after error

const char* __texsToPreload[] = {
	"thinstream/icons.png"
	, "thinstream/icons-gt.png"
	, "thinstream/icons-circle-small.png"
	, "thinstream/icons-circle-big.png"
	, "thinstream/icons_circle90.png"
	, "thinstream/icon-big.png"
	, "thinstream/iconDone80.png"
	, "thinstream/iconDone80White.png"
	, "thinstream/mesh.png"
	, "thinstream/meshPage.png"
	, "thinstream/scrolllbar.png"
	, "thinstream/scrolllbarMed.png"
	, "thinstream/quickTipDown.png"
	, "thinstream/iconsSmallCircleBack.png"
	, "thinstream/bar-btm-sel-mask-192.png"
};

//

AUScenesAdmin::AUScenesAdmin(AUAppI* app, STTSClient* coreClt, const SI32 iScene, const char* prefijoPaquetes, const STAppScenasPkgLoadDef* paquetesCargar, const SI32 conteoPaquetesCargar)
: AUAppEscenasAdmin(iScene, prefijoPaquetes, paquetesCargar, conteoPaquetesCargar)
, _scenesStack(this) {
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::AUScenesAdmin")
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUScenesAdmin")
	//NBASSERT(NBGestorRefranero::gestorInicializado());
	//-----------------------------------
	//SIMULAR SOLO EXISTENCIA DE PAQUETES
	//-----------------------------------
	//NBGestorArchivos::establecerSimularSoloRutasHaciaPaquetes(true);
	//Reset data
	/*{
		PRINTF_WARNING("----------------\n");
		PRINTF_WARNING("----------------\n");
		PRINTF_WARNING("-- Reseting DATA\n");
		NBFilesystem_deleteFolderAtRoot(NBGestorArchivos::getFilesystem(), ENNBFilesystemRoot_Lib, "thinstream");
		PRINTF_WARNING("----------------\n");
		PRINTF_WARNING("----------------\n");
	}*/
	//init font sizes
	{
		TSFonts::init(iScene);
		{
			STNBString countryISO;
			NBString_init(&countryISO);
			{
				NBMngrOSTelephony::getCarrierCountryISO(&countryISO);
				PRINTF_INFO("AUScenesAdmin, initial default country ISO for contacts: '%s'.\n", countryISO.str);
			}
			NBString_release(&countryISO);
		}
	}
	//
	_app				= app;
	_coreClt			= coreClt; NBASSERT(coreClt != NULL)
	_animator			= new(this) AUAnimadorObjetoEscena();
	//Limits
	{
		NBMemory_setZero(_lastBox);
		NBMemory_setZero(_lastBoxUsable);
		this->privCalculateLimits();
	}
	//Preloaded textures
	{
		NBArray_init(&_texsPreload, sizeof(AUTextura*), NULL);
		{
			SI32 i; const SI32 sz = (sizeof(__texsToPreload) / sizeof(__texsToPreload[0]));
			for(i = 0; i < sz; i++){
				const char* path = __texsToPreload[i];
				if(!NBString_strIsEmpty(path)){
					AUTextura* tex = NBGestorTexturas::texturaDesdeArchivo(path);
					if(tex == NULL){
						PRINTF_CONSOLE_ERROR("AUScenesAdmin, texture could not be preloaded: '%s'.\n", path);
					} else {
						tex->retener(NB_RETENEDOR_THIS);
						NBArray_addValue(&_texsPreload, tex);
						PRINTF_INFO("AUScenesAdmin, texture preloaded: '%s'.\n", path);
					}
				}
			}
		}
	}
	//Top layer
	{
		NBColor8 color; NBCOLOR_ESTABLECER(color, 255, 255, 255, 255);
		_topLayer = new AUEscenaContenedor();
		NBGestorEscena::agregarObjetoCapa(_iScene, ENGestorEscenaGrupo_Cortina, _topLayer, color);
		NBGestorEscena::setTopItf(_iScene, this);
	}
	//Popups
	{
		NBMemory_setZero(_pops);
		NBArray_init(&_pops.array, sizeof(STScenesPop*), NULL);
		NBThreadMutex_init(&_pops.mutex);
	}
	//Portrait enforcers
	{
		NBMemory_setZero(_orientation);
		_orientation.mask		= ENAppOrientationBits_All;
		_orientation.prefered	= ENAppOrientationBit_Portrait;
		_orientation.toApplyOnce = (ENAppOrientationBit)0;
		_orientation.allowAutoRotate = TRUE;
	}
	//Requests progress popups
	{
		NBArray_init(&_reqsProg.pops, sizeof(STScenesRequestPop*), NULL);
		NBThreadMutex_init(&_reqsProg.mutex);
	}
	//Quick-tip
	{
		//Layer
		{
			_quickTip.layer = new AUEscenaContenedor();
			_quickTip.layer->establecerVisible(false);
			_quickTip.layer->establecerEscuchadorTouches(this, this);
			_topLayer->agregarObjetoEscena(_quickTip.layer);
		}
		//Bg
		{
			_quickTip.bg				= new AUEscenaSpriteElastico(NBGestorTexturas::texturaDesdeArchivo("thinstream/quickTipDown.png"));
			_quickTip.bg->establecerMultiplicadorColor8(0, 0, 0, 255);
			_quickTip.layer->agregarObjetoEscena(_quickTip.bg);
		}
		//Text layer
		{
			{
				_quickTip.txtLayer = new AUEscenaContenedor();
				_quickTip.layer->agregarObjetoEscena(_quickTip.txtLayer);
			}
			{
				_quickTip.icoLeft		= new AUEscenaSprite();
				_quickTip.txtLayer->agregarObjetoEscena(_quickTip.icoLeft);
			}
			{
				_quickTip.icoRight		= new AUEscenaSprite();
				_quickTip.txtLayer->agregarObjetoEscena(_quickTip.icoRight);
			}
			{
				AUFuenteRender* fnt		= TSFonts::font(ENTSFont_ContentMid);
				_quickTip.txt			= new AUEscenaTexto(fnt);
				_quickTip.txt->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_FromBottom);
				_quickTip.txt->establecerMultiplicadorColor8(255, 255, 255, 255);
				_quickTip.txt->establecerTexto("quick-tip-text");
				_quickTip.txtLayer->agregarObjetoEscena(_quickTip.txt);
			}
		}
	}
	//
	NBMemory_setZero(_loadTransBoxFocus);
	_loadAction			= ENSceneAction_ReplaceAll;
	_loadTransCur		= NULL;
	_loadTransSecsDur	= 1.0f;
	_loadInfoSequence	= false;
	_loadInfoSequenceMode = ENSceneInfoMode_Count;
	_loadLobby			= false;
	_loadPrevScene		= false;
	//
	NBGestorIdioma::establecerPrioridadIdioma(0, ENIdioma_ES);
	{
		//Try to load device
		if(!TSClient_isDeviceKeySet(_coreClt)){
			TSClient_loadCurrentDeviceKey(_coreClt);
		}
		//
		this->reloadInitialContent(ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
	}
	//CurrentIds
	{
		NBString_init(&_lastIds.deviceId);
		//Listen client-root
		{
			STNBStorageCacheItmLstnr mthds;
			NBMemory_setZeroSt(mthds, STNBStorageCacheItmLstnr);
			mthds.retain		= AUObjeto_retain_;
			mthds.release		= AUObjeto_release_;
			mthds.recordChanged	= AUScenesAdmin::recordChanged;
			TSClient_addRecordListener(_coreClt, "client/_root/", "_current", this, &mthds);
		}
		//Load client-root
		{
			STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(_coreClt, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
			if(ref.data != NULL){
				STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
				NBString_set(&_lastIds.deviceId, priv->deviceId);
			}
			TSClient_returnStorageRecordFromRead(_coreClt, &ref);
		}
	}
	//Upload
	{
		NBMemory_setZero(_upload);
		_upload.sequence = 0;
		NBString_init(&_upload.filepath);
		NBString_init(&_upload.fileData);
		NBString_init(&_upload.usrData);
		//Background rotating image
		{
			_upload.pend.isActive	= FALSE;
			_upload.pend.popRef		= NULL;
            _upload.pend.file       = NBFile_alloc(NULL);
			NBString_init(&_upload.pend.fileData);
			NBString_init(&_upload.pend.usrData);
			NBBitmap_init(&_upload.pend.rotBmp);
		}
	}
	//Error log
	/*{
		NBMemory_setZero(_errLog);
	}*/
	//Notifs
	/*{
		NBMemory_setZero(_notifs);
		//Auth
		{
			_notifs.auth.stateWaitSecs	= 0.0f;
			_notifs.auth.state			= ENAppNotifAuthState_Count;
		}
		NBString_init(&_notifs.tokenCur);
		_notifs.tokenCurWaitSecs = 0.0f;
		//
		NBString_init(&_notifs.tokenSent);
		NBString_init(&_notifs.tokenSending);
	}*/
	//
	this->reanudarAnimacion();
	_app->addAppStateListener(this);
	_app->addAppOpenUrlListener(this);
	NBGestorAnimadores::agregarAnimador(this, this);
	NBGestorTeclas::agregarEscuchadorVisual(this);
	//Add error log request
	/*{
		const UI32 logLen = TSClient_logPendLength(_coreClt);
		if(logLen > 0){
			//Create new
			{
				ENDRErrorLogSendMode errLogMode = ENDRErrorLogSendMode_Ask;
				{
					STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(_coreClt, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
					if(ref.data != NULL){
						STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
						errLogMode = priv->errLogMode;
					}
					TSClient_returnStorageRecordFromRead(_coreClt, &ref);
				}
				switch (errLogMode) {
					case ENDRErrorLogSendMode_Ask:
						this->privLogErrAskStart();
						break;
					case ENDRErrorLogSendMode_Allways:
						this->privLogErrSendStart();
						break;
					default:
						TSClient_logPendClear(_coreClt);
						break;
				}
			}
		}
	}*/
	//Test log confirmation
	/*{
		if(_errLog.popRef == NULL){
			this->privLogErrAskStart();
		}
	}*/
	//Test Conn
	/*{
		const char* strLoc = TSClient_getStr(_coreClt, "reqHelloTitle", "Saying Hello");
		this->addRequestWithPop(ENReqPopHideMode_Auto, strLoc, ENTSRequestId_Hello, NULL, NULL, 0, NULL, 5, NULL, NULL, NULL);
	}*/
	//Test PdfKit
	/*{
		void* docRef = NBMngrPdfKit::docOpenFromPath(NBGestorArchivos::rutaHaciaRecursoEnPaquete("sample.pdf"));
		if(docRef == NULL){
			this->popShowInfo(ENScenePopMode_Error, ENNBTextLineAlignH_Center, "Error", "Could not parse loaded PDF data.\n");
		} else {
			BOOL errFnd = FALSE;
			const UI32 pagesCount = NBMngrPdfKit::docGetPagesCount(docRef);
			PRINTF_INFO("PdfDoc has %d pages.\n", pagesCount);
			{
				UI32 i; for(i = 0 ; i < pagesCount; i++){
					void* pageRef = NBMngrPdfKit::docGetPageAtIdx(docRef, i);
					if(pageRef == NULL){
						this->popShowInfo(ENScenePopMode_Error, ENNBTextLineAlignH_Center, "Error", "Could not obtain PDF page.");
						errFnd = TRUE;
					} else {
						const STNBRect rect = NBMngrPdfKit::pageGetBox(pageRef);
						PRINTF_INFO("PdfPage %d of %d rect(%f, %f)-(+%f, +%f).\n", (i + 1), pagesCount, rect.x, rect.y, rect.width, rect.height);
						{
							STNBBitmap bmp;
							NBBitmap_init(&bmp);
							if(!NBMngrPdfKit::pageRenderArea(pageRef, rect, NBST_P(STNBSizeI, (SI32)(rect.width), (SI32)(rect.height / 2.0f) ), &bmp)){
								PRINTF_CONSOLE_ERROR("PdfPage %d of %d cluld not be drawed rect(%f, %f)-(+%f, +%f).\n", (i + 1), pagesCount, rect.x, rect.y, rect.width, rect.height);
								errFnd = TRUE;
							} else {
								const STNBBitmapProps props = NBBitmap_getProps(&bmp);
								PRINTF_INFO("PdfPage %d of %d rendered(+%d, +%d).\n", (i + 1), pagesCount, props.size.width, props.size.height);
							}
							NBBitmap_release(&bmp);
						}
	 					//Release page
						NBMngrPdfKit::pageRelease(pageRef);
					}
				}
			}
			if(!errFnd){
				this->popShowInfo(ENScenePopMode_Sucess, ENNBTextLineAlignH_Center, "PDF opened", "Pdf file loaded and opened.\n");
			}
			NBMngrPdfKit::docRelease(docRef);
		}
	}*/
	//Orientations
	{
		NBMngrOSTools::setCanAutorotate(_orientation.allowAutoRotate);
		NBMngrOSTools::setOrientationsMask(_orientation.mask);
		NBMngrOSTools::setOrientationPrefered(_orientation.prefered);
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

AUScenesAdmin::~AUScenesAdmin(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::~AUScenesAdmin")
	NBGestorEscena::quitarObjetoCapa(_iScene, _topLayer);
	TSClient_removeRequestsByListenerParam(_coreClt, this);
	_app->removeAppOpenUrlListener(this);
	_app->removeAppStateListener(this);
	//Contacts
	{
		//Primary
		{
			//
		}
		//Secondary
		{
			//
		}
	}
	//Error log
	/*{
		if(_errLog.popRef != NULL){
			this->popRelease(_errLog.popRef);
			_errLog.popRef =  NULL;
		}
	}*/
	//Preloaded textures
	{
		SI32 i; for(i = 0; i < _texsPreload.use; i++){
			AUTextura* tex = NBArray_itmValueAtIndex(&_texsPreload, AUTextura*, i);
			if(tex != NULL) tex->liberar(NB_RETENEDOR_THIS); tex = NULL;
		}
		NBArray_empty(&_texsPreload);
	}
	//Top
	if(_topLayer != NULL) _topLayer->liberar(NB_RETENEDOR_THIS); _topLayer = NULL;
	//Requests progress popups
	{
		NBThreadMutex_lock(&_reqsProg.mutex);
		{
			SI32 i; for(i = 0 ; i < _reqsProg.pops.use; i++){
				STScenesRequestPop* itm = NBArray_itmValueAtIndex(&_reqsProg.pops, STScenesRequestPop*, i);
				AUScenesAdmin::privRequestPopRelease(itm);
				NBMemory_free(itm);
				itm = NULL;
			}
			NBArray_empty(&_reqsProg.pops);
			NBArray_release(&_reqsProg.pops);
		}
		NBThreadMutex_unlock(&_reqsProg.mutex);
		NBThreadMutex_release(&_reqsProg.mutex);
	}
	//CurrentIds
	{
		TSClient_removeRecordListener(_coreClt, "client/_root/", "_current", this);
		NBString_release(&_lastIds.deviceId);
	}
	//Popups
	{
		NBThreadMutex_lock(&_pops.mutex);
		{
			SI32 i; for(i = 0 ; i < _pops.array.use; i++){
				STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
				AUScenesAdmin::privPopRelease(itm);
				NBMemory_free(itm);
			}
			NBArray_empty(&_pops.array);
			NBArray_release(&_pops.array);
		}
		NBThreadMutex_unlock(&_pops.mutex);
		NBThreadMutex_release(&_pops.mutex);
	}
	//Quick-tip
	{
		if(_quickTip.layer != NULL) _quickTip.layer->liberar(NB_RETENEDOR_THIS); _quickTip.layer = NULL;
		if(_quickTip.bg != NULL) _quickTip.bg->liberar(NB_RETENEDOR_THIS); _quickTip.bg = NULL;
		if(_quickTip.txtLayer != NULL) _quickTip.txtLayer->liberar(NB_RETENEDOR_THIS); _quickTip.txtLayer = NULL;
		if(_quickTip.txt != NULL) _quickTip.txt->liberar(NB_RETENEDOR_THIS); _quickTip.txt = NULL;
		if(_quickTip.icoLeft != NULL) _quickTip.icoLeft->liberar(NB_RETENEDOR_THIS); _quickTip.icoLeft = NULL;
		if(_quickTip.icoRight != NULL) _quickTip.icoRight->liberar(NB_RETENEDOR_THIS); _quickTip.icoRight = NULL;
	}
	//Popup
	{
		if(_animator != NULL) _animator->liberar(NB_RETENEDOR_THIS); _animator = NULL;
	}
	//Upload
	{
		//backgrund rotating job
		{
			while(_upload.pend.isActive){
				NBThread_mSleep(20);
			}
			if(_upload.pend.popRef != NULL){
				this->popRelease(_upload.pend.popRef);
				_upload.pend.popRef = NULL;
			}
			NBFile_release(&_upload.pend.file);
			NBString_release(&_upload.pend.fileData);
			NBString_release(&_upload.pend.usrData);
			NBBitmap_release(&_upload.pend.rotBmp);
		}
		_upload.sequence = 0;
		NBString_release(&_upload.filepath);
		NBString_release(&_upload.fileData);
		NBString_release(&_upload.usrData);
	}
	//Notifs
	/*{
		NBString_release(&_notifs.tokenCur);
		NBString_release(&_notifs.tokenSent);
		NBString_release(&_notifs.tokenSending);
	}*/
	_coreClt = NULL;
	NBGestorTeclas::quitarEscuchadorVisual(this);
	NBGestorAnimadores::quitarAnimador(this);
	if(_loadTransCur != NULL) _loadTransCur->liberar(NB_RETENEDOR_THIS); _loadTransCur = NULL;
	_scenesStack.vaciar();
	//
	{
		TSFonts::finish();
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

//

/*void AUScenesAdmin::privLogErrAskStart(){
	//Hide/release previous
	if(_errLog.popRef != NULL){
		this->popHide(_errLog.popRef);
		this->popRelease(_errLog.popRef);
		_errLog.popRef =  NULL;
	}
	//Create new
	{
		const char* strLoc0 = TSClient_getStr(_coreClt, "errLogSendTitle", "Send error log");
		const char* strLoc1 = TSClient_getStr(_coreClt, "errLogSendText", "Anonymous error information is available to be send to the server. This information could help to improve this app, do you want to send it?");
		const char* strLoc2 = TSClient_getStr(_coreClt, "errLogIgnoreBtn", "Hide");
		_errLog.popRef = this->popCreate(ENScenePopMode_TextOnly, ENNBTextLineAlignH_Center, strLoc0, strLoc1, this, this, this, strLoc2, ENScenePopBtnColor_Clear);
		{
			const char* strLoc5 = TSClient_getStr(_coreClt, "errLogNeverBtn", "Never send");
			this->popAddOption(_errLog.popRef, "never", strLoc5, ENScenePopBtnColor_Clear, ENIMsgboxBtnAction_AutohideAndNotify);
		}
		//{
		//	const char* strLoc4 = TSClient_getStr(_coreClt, "errLogSendBtn", "Send log");
		//	this->popAddOption(_errLog.popRef, "send", strLoc4, ENScenePopBtnColor_Clear, ENIMsgboxBtnAction_AutohideAndNotify);
		//}
		{
			const char* strLoc3 = TSClient_getStr(_coreClt, "errLogAllwaysBtn", "Allways send");
			this->popAddOption(_errLog.popRef, "allways", strLoc3, ENScenePopBtnColor_Main, ENIMsgboxBtnAction_AutohideAndNotify);
		}
		this->popShow(_errLog.popRef);
	}
}*/

/*void AUScenesAdmin::privLogErrSendStart(){
	const UI32 logLen = TSClient_logPendLength(_coreClt);
	if(logLen > 0){
		STNBString str;
		NBString_initWithSz(&str, logLen + 1, 4096, 0.10f);
		TSClient_logPendGet(_coreClt, &str);
		if(str.length > 0){
			STTSDeviceLogNewReq req;
			NBMemory_setZeroSt(req, STTSDeviceLogNewReq);
			req.time	= NBDatetime_getCurUTCTimestamp();
			NBString_strFreeAndNewBufferBytes(&req.log, str.str, str.length);
			{
				STNBString str;
				NBString_init(&str);
				NBStruct_stConcatAsJson(&str, TSDeviceLogNewReq_getSharedStructMap(), &req, sizeof(req));
				TSClient_addRequest(_coreClt, ENTSRequestId_Device_Log_New, NULL, str.str, str.length, 0, AUScenesAdmin::privLogErrReqCallbackFunc, this);
				NBString_release(&str);
			}
			NBStruct_stRelease(TSDeviceLogNewReq_getSharedStructMap(), &req, sizeof(req));
		}
		NBString_release(&str);
	}
}*/

/*void AUScenesAdmin::privLogErrReqCallbackFunc(const STTSClientChannelRequest* req, void* lParam){
	AUScenesAdmin* obj = (AUScenesAdmin*)lParam;
	NBASSERT(obj != NULL)
	if(obj != NULL){
		NBASSERT(req->reqId == ENTSRequestId_Device_Log_New)
		if(req->reqId == ENTSRequestId_Device_Log_New){
			UI32 statusCode = 0;
			if(req->response != NULL){
				if(req->response->header.statusLine != NULL){
					statusCode = req->response->header.statusLine->statusCode;
				}
			}
			if(statusCode >= 200 && statusCode < 300){
				//
			}
			PRINTF_INFO("ENTSRequestId_Device_Log_New request ended with code(%d).\n", statusCode);
		}
	}
}*/

//

void AUScenesAdmin::recordChanged(void* param, const char* relRoot, const char* relPath){
	NBASSERT(param != NULL)
	//PRINTF_INFO("AUScenesAdmin, record updated: '%s' '%s'.\n", relRoot, relPath);
	if(param != NULL){
		AUScenesAdmin* obj = (AUScenesAdmin*)param;
		if(NBString_strIsLike(relRoot, "client/_root/") && NBString_strIsLike(relPath, "_current")){
			BOOL deviceIdChanged = FALSE;
			{
				STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(obj->_coreClt, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
				if(ref.data != NULL){
					STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
					{
						STNBString str;
						NBString_init(&str);
						NBStruct_stConcatAsJson(&str, TSClientRoot_getSharedStructMap(), priv, sizeof(*priv));
						//PRINTF_INFO("AUScenesAdmin, record value: '%s' '%s':\n'%s'\n", relRoot, relPath, str.str);
						NBString_release(&str);
					}
					if(!(NBString_strIsEmpty(obj->_lastIds.deviceId.str) && NBString_strIsEmpty(priv->deviceId))){
						if(!NBString_isEqual(&obj->_lastIds.deviceId, priv->deviceId)){
							NBString_set(&obj->_lastIds.deviceId, priv->deviceId);
							deviceIdChanged = TRUE;
						}
					}
				}
				TSClient_returnStorageRecordFromRead(obj->_coreClt, &ref);
			}
			//Process
			if(deviceIdChanged){
				PRINTF_INFO("DeviceId changed.\n");
			}
		}
	}
}

//

void AUScenesAdmin::appStateOnCreate(AUAppI* app){
	//
}

void AUScenesAdmin::appStateOnDestroy(AUAppI* app){	//applicationWillTerminate
	//
}

void AUScenesAdmin::appStateOnStart(AUAppI* app){	//applicationDidBecomeActive
	//App opened by a notification?
	{
		NBMngrNotifs::lock();
		const STAppNotif* launchNotif = NBMngrNotifs::lockedGetLaunchNotif();
		if(launchNotif != NULL){
			if(launchNotif->data != NULL){
				STNBUrl url;
				NBUrl_init(&url);
				if(NBUrl_parse(&url, launchNotif->data->str())){
					this->appOpenUrl(NULL, &url, NULL, 0);
				}
				NBUrl_release(&url);
			}
		}
		NBMngrNotifs::lockedClearLaunchNotif();
		NBMngrNotifs::unlock();
		NBMngrNotifs::clearRcvdNotifs();
	}
}


void AUScenesAdmin::appStateOnStop(AUAppI* app){	//applicationWillResignActive
	//
}

void AUScenesAdmin::appStateOnResume(AUAppI* app){	//applicationWillEnterForeground
	//
}

void AUScenesAdmin::appStateOnPause(AUAppI* app){	//applicationDidEnterBackground
	//
}

//Energy saving

float AUScenesAdmin::getSecsWithSameContent(){
	return _app->getSecsWithSameContent();
}

//

SI64 AUScenesAdmin::privRotateImageMethod_(STNBThread* t, void* param){
	SI64 r = 0;
	//
	AUScenesAdmin* obj = (AUScenesAdmin*)param;
	if(obj != NULL){
		NBASSERT(obj->_upload.pend.isActive)
		//Execute
		{
			STNBFileRef file = obj->_upload.pend.file;
			BYTE header[32];
			NBMemory_setZero(header);
			{
				NBFile_lock(file);
				NBFile_read(file, &header, sizeof(header));
				NBFile_seek(file, 0, ENNBFileRelative_Start);
				NBFile_unlock(file);
			}
			//Load
			NBBitmap_empty(&obj->_upload.pend.rotBmp);
			{
				STNBBitmap bmp;
				NBBitmap_init(&bmp);
				{
					BOOL loaded = FALSE;
					if(NBPng_dataStartsAsPng(header, sizeof(header))){
						NBFile_lock(file);
						if(!NBPng_loadFromFile(file, TRUE, &bmp, NULL)){
							PRINTF_CONSOLE_ERROR("AUIOSToolImagePickerDelegate, NBPng_loadFromFile failed.\n");
						} else {
							loaded = TRUE;
						}
						NBFile_unlock(file);
					} else {
						NBFile_lock(file);
						if(!NBJpeg_loadFromFile(file, TRUE, &bmp)){
							PRINTF_CONSOLE_ERROR("AUIOSToolImagePickerDelegate, NBJpeg_loadFromFile failed.\n");
						} else {
							loaded = TRUE;
						}
						NBFile_unlock(file);
					}
					//Rotate
					if(loaded){
						const STNBBitmapProps props = NBBitmap_getProps(&bmp);
						STNBBitmap bmp2;
						NBBitmap_init(&bmp2);
						if(!NBBitmap_createWithBitmapRotatedRight90(&bmp2, props.color, &bmp, NBST_P(STNBColor8, 255, 255, 255, 255 ), (obj->_upload.pend.rotFromIntended / 90))){
							PRINTF_CONSOLE_ERROR("AUIOSToolImagePickerDelegate, NBJpeg_loadFromFile failed.\n");
						} else {
							NBBitmap_swapData(&obj->_upload.pend.rotBmp, &bmp2);
						}
						NBBitmap_release(&bmp2);
					}
				}
				NBBitmap_release(&bmp);
			}
			//End
			NBFile_close(file);
			obj->_upload.pend.isActive = FALSE;
		}
		//Release
		obj->liberar(NB_RETENEDOR_NULL);
	}
	//Release thread
	if(t != NULL){
		NBThread_release(t);
		NBMemory_free(t);
		t = NULL;
	}
	return r;
}

//

UI32 AUScenesAdmin::getLastOpenUrlPathSeq(STNBString* dstFilePath, STNBString* dstFileData, STNBString* dstUsrData, const BOOL clearValue){
	{
		if(dstFilePath != NULL){
			NBString_set(dstFilePath, _upload.filepath.str);
		}
		if(dstFileData != NULL){
			if(clearValue){
				NBString_swapContent(dstFileData, &_upload.fileData);
			} else {
				NBString_setBytes(dstFileData, _upload.fileData.str, _upload.fileData.length);
			}
		}
		if(dstUsrData != NULL){
			NBString_set(dstUsrData, _upload.usrData.str);
		}
	}
	if(clearValue){
		NBString_empty(&_upload.filepath);
		NBString_empty(&_upload.fileData);
		NBString_empty(&_upload.usrData);
	}
	return _upload.sequence;
}

bool AUScenesAdmin::appOpenUrl(AUAppI* app, const STNBUrl* url, const void* usrData, const UI32 usrDataSz){
	bool r = false;
	{
		const char* filepath = &url->str.str[url->iPath];
		this->uploadStartFromPath(filepath, usrData, usrDataSz);
	}
	return r;
}

bool AUScenesAdmin::appOpenUrlImage(AUAppI* app, const STNBUrl* url, const SI32 rotDegFromIntended, const void* usrData, const UI32 usrDataSz){
	bool r = false;
	{
		BOOL consumed = FALSE;
		//Rotate in backgroud
		if(!consumed){
			if(!_upload.pend.isActive && rotDegFromIntended != 0){
				const char* filepath = &url->str.str[url->iPath];
				if(NBString_strStartsWith(filepath, "file://")){
					filepath = &filepath[7];
				}
				//Close prev file
				{
					NBBitmap_empty(&_upload.pend.rotBmp);
					NBFile_close(_upload.pend.file);
					NBFile_release(&_upload.pend.file);
				}
				//Usr data
				{
					NBString_empty(&_upload.pend.usrData);
					if(usrData != NULL && usrDataSz > 0){
						NBString_setBytes(&_upload.pend.usrData, (const char*)usrData, usrDataSz);
					}
				}
				//Open new file
                _upload.pend.file = NBFile_alloc(NULL);
				if(!NBFile_open(_upload.pend.file, filepath, ENNBFileMode_Read)){
					PRINTF_CONSOLE_ERROR("appOpenUrlImage, NBFile_open failed.\n");
				} else {
					STNBThread* t = NBMemory_allocType(STNBThread);
					NBThread_init(t);
					NBThread_setIsJoinable(t, FALSE); //Release thread resources after exit
					//
					this->retener(NB_RETENEDOR_THIS);
					_upload.pend.isActive = TRUE;
					_upload.pend.rotFromIntended = rotDegFromIntended;
					//
					if(!NBThread_start(t, AUScenesAdmin::privRotateImageMethod_, this, NULL)){
						_upload.pend.isActive = FALSE;
						this->liberar(NB_RETENEDOR_THIS);
					} else {
						t = NULL;
						consumed = TRUE;
						NBString_empty(&_upload.filepath);
						NBString_empty(&_upload.fileData);
						NBString_empty(&_upload.usrData);
						{
							const char* strLoc0 = TSClient_getStr(_coreClt, "imgProcessingTitle", "Processing image...");
							const char* strLoc1 = TSClient_getStr(_coreClt, "imgProcessingHideBtn", "Hide");
							if(_upload.pend.popRef != NULL){
								this->popHide(_upload.pend.popRef);
								this->popRelease(_upload.pend.popRef);
								_upload.pend.popRef = NULL;
							}
							{
								_upload.pend.popRef = this->popCreate(ENScenePopMode_Working, ENNBTextLineAlignH_Center, NULL, strLoc0, this, this, this, strLoc1, ENScenePopBtnColor_Clear);
								this->popShow(_upload.pend.popRef);
							}
						}
					}
					//Release (if not consumed)
					if(t != NULL){
						NBFile_close(_upload.pend.file);
						NBThread_release(t);
						NBMemory_free(t);
						t = NULL;
					}
				}
			}
		}
		//Open (if not consumed)
		if(!consumed){
			const char* filepath = &url->str.str[url->iPath];
			this->uploadStartFromPath(filepath, usrData, usrDataSz);
			consumed = TRUE;
		}
	}
	return r;
}

bool AUScenesAdmin::appOpenFileData(AUAppI* app, const void* data, const UI32 dataSz, const void* usrData, const UI32 usrDataSz){
	bool r = false;
	{
		//File path
		NBString_empty(&_upload.filepath);
		//File data
		NBString_setBytes(&_upload.fileData, (const char*)data, dataSz);
		//Usr data
		{
			NBString_empty(&_upload.usrData);
			if(usrData != NULL && usrDataSz > 0){
				NBString_setBytes(&_upload.usrData, (const char*)usrData, usrDataSz);
			}
		}
		_upload.sequence++;
	}
	return r;
}

bool AUScenesAdmin::appOpenFileImageData(AUAppI* app, const void* data, const UI32 dataSz, const SI32 rotDegFromIntended, const void* usrData, const UI32 usrDataSz){
	bool r = false;
	{
		BOOL consumed = FALSE;
		//Rotate in backgroud
		if(!consumed){
			if(!_upload.pend.isActive && rotDegFromIntended != 0){
				//Close prev file
				{
					NBBitmap_empty(&_upload.pend.rotBmp);
					NBFile_close(_upload.pend.file);
					NBFile_release(&_upload.pend.file);
				}
				//Usr data
				{
					NBString_empty(&_upload.pend.usrData);
					if(usrData != NULL && usrDataSz > 0){
						NBString_setBytes(&_upload.pend.usrData, (const char*)usrData, usrDataSz);
					}
				}
				//Open new file
                _upload.pend.file = NBFile_alloc(NULL);
				NBString_setBytes(&_upload.pend.fileData, (const char*)data, dataSz);
				if(!NBFile_openAsDataRng(_upload.pend.file, _upload.pend.fileData.str, _upload.pend.fileData.length)){
					PRINTF_CONSOLE_ERROR("appOpenFileImageData, NBFile_open failed.\n");
				} else {
					STNBThread* t = NBMemory_allocType(STNBThread);
					NBThread_init(t);
					NBThread_setIsJoinable(t, FALSE); //Release thread resources after exit
					//
					this->retener(NB_RETENEDOR_THIS);
					_upload.pend.isActive = TRUE;
					_upload.pend.rotFromIntended = rotDegFromIntended;
					//
					if(!NBThread_start(t, AUScenesAdmin::privRotateImageMethod_, this, NULL)){
						_upload.pend.isActive = FALSE;
						this->liberar(NB_RETENEDOR_THIS);
					} else {
						t = NULL;
						consumed = TRUE;
						NBString_empty(&_upload.filepath);
						NBString_empty(&_upload.fileData);
						NBString_empty(&_upload.usrData);
						{
							const char* strLoc0 = TSClient_getStr(_coreClt, "imgProcessingTitle", "Processing image...");
							const char* strLoc1 = TSClient_getStr(_coreClt, "imgProcessingHideBtn", "Hide");
							if(_upload.pend.popRef != NULL){
								this->popHide(_upload.pend.popRef);
								this->popRelease(_upload.pend.popRef);
								_upload.pend.popRef = NULL;
							}
							{
								_upload.pend.popRef = this->popCreate(ENScenePopMode_Working, ENNBTextLineAlignH_Center, NULL, strLoc0, this, this, this, strLoc1, ENScenePopBtnColor_Clear);
								this->popShow(_upload.pend.popRef);
							}
						}
					}
					//Release (if not consumed)
					if(t != NULL){
						NBFile_close(_upload.pend.file);
						NBThread_release(t);
						NBMemory_free(t);
						t = NULL;
					}
				}
			}
		}
		//Open (if not consumed)
		if(!consumed){
			//File path
			NBString_empty(&_upload.filepath);
			//File data
			NBString_setBytes(&_upload.fileData, (const char*)data, dataSz);
			//Usr data
			{
				NBString_empty(&_upload.usrData);
				if(usrData != NULL && usrDataSz > 0){
					NBString_setBytes(&_upload.usrData, (const char*)usrData, usrDataSz);
				}
			}
			_upload.sequence++;
		}
	}
	return r;
}

bool AUScenesAdmin::appOpenBitmap(AUAppI* app, const STNBBitmap* bmp, const SI32 rotDegFromIntended, const void* usrData, const UI32 usrDataSz){
	bool r = false;
	{
		//File path
		NBString_empty(&_upload.filepath);
		//File data
		{
			NBString_empty(&_upload.fileData);
			{
                STNBFileRef file = NBFile_alloc(NULL);
				if(!NBFile_openAsString(file, &_upload.fileData)){
					//
				} else {
					NBFile_lock(file);
					NBJpeg_saveToFile(bmp, file, 90, 10);
					NBFile_unlock(file);
				}
				NBFile_release(&file);
			}
		}
		//Usr data
		{
			NBString_empty(&_upload.usrData);
			if(usrData != NULL && usrDataSz > 0){
				NBString_setBytes(&_upload.usrData, (const char*)usrData, usrDataSz);
			}
		}
		_upload.sequence++;
	}
	return r;
}

void AUScenesAdmin::uploadStartFromPath(const char* filepath, const void* usrData, const UI32 usrDataSz){
	//File path
	NBString_set(&_upload.filepath, filepath);
	//File data
	NBString_empty(&_upload.fileData);
	//Usr data
	{
		NBString_empty(&_upload.usrData);
		if(usrData != NULL && usrDataSz > 0){
			NBString_setBytes(&_upload.usrData, (const char*)usrData, usrDataSz);
		}
	}
	_upload.sequence++;
}

//Top Itf

BOOL AUScenesAdmin::topShowQuickTipTextAtViewPort(const STNBPointI viewPortPos, const char* text, AUTextura* iconLeft, AUTextura* iconRight, const float secsShow, const ENEscenaTipAim aimDir){
	BOOL r = FALSE;
	{
		AUTextura* tex = _quickTip.bg->textura();
		const STNBAABox scnBox	= _lastBox;
		const float marginI		= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.10f);
		const float marginIco	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.05f);
		const float marginTop	= tex->meshFirstPortionSzHD(ENMallaCorteSentido_Vertical);
		const float marginBtm	= tex->meshLastPortionSzHD(ENMallaCorteSentido_Vertical);
		const float maxWidth	= (scnBox.xMax - scnBox.xMin) * 0.80f;
		//Build content
		{
			if(iconLeft != NULL){
				_quickTip.icoLeft->establecerTextura(iconLeft);
				_quickTip.icoLeft->redimensionar(iconLeft);
				_quickTip.icoLeft->establecerVisible(true);
			} else {
				_quickTip.icoLeft->establecerTextura(NULL);
				_quickTip.icoLeft->establecerVisible(false);
			}
			//
			if(iconRight != NULL){
				_quickTip.icoRight->establecerTextura(iconRight);
				_quickTip.icoRight->redimensionar(iconRight);
				_quickTip.icoRight->establecerVisible(true);
			} else {
				_quickTip.icoRight->establecerTextura(NULL);
				_quickTip.icoRight->establecerVisible(false);
			}
		}
		const NBPunto pos = NBGestorEscena::coordenadaPuertoAGrupo(_iScene, ENGestorEscenaGrupo_Cortina, viewPortPos.x, viewPortPos.y);
		switch(aimDir) {
			case ENEscenaTipAim_Up:
				{
					//Internal organization
					{
						float yTop = 0.0f;
						if(_quickTip.icoLeft->visible()){
							const NBCajaAABB box = _quickTip.icoLeft->cajaAABBLocal();
							if(yTop != 0.0f) yTop -= marginIco;
							_quickTip.icoLeft->establecerTraslacion(-box.xMin - ((box.xMax - box.xMin) * 0.5f), yTop - box.yMax);
							yTop -= (box.yMax - box.yMin);
						}
						if(_quickTip.txt->visible()){
							AUFuenteRender* fnt = TSFonts::font(ENTSFont_ContentMid);
							_quickTip.txt->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_FromTop);
							_quickTip.txt->establecerFuenteRender(fnt);
							_quickTip.txt->establecerTexto(text, maxWidth);
							{
								const NBCajaAABB box = _quickTip.txt->cajaAABBLocalCalculada();
								if(yTop != 0.0f) yTop -= marginIco;
								_quickTip.txt->establecerTraslacion(-box.xMin - ((box.xMax - box.xMin) * 0.5f), yTop - box.yMax);
								yTop -= (box.yMax - box.yMin);
							}
							
						}
						if(_quickTip.icoRight->visible()){
							const NBCajaAABB box = _quickTip.icoRight->cajaAABBLocal();
							if(yTop != 0.0f) yTop -= marginIco;
							_quickTip.icoRight->establecerTraslacion(-box.xMin - ((box.xMax - box.xMin) * 0.5f), yTop - box.yMax);
							yTop -= (box.yMax - box.yMin);
						}
					}
					//Global organization
					{
						const NBCajaAABB box	= _quickTip.txtLayer->cajaAABBLocalCalculada();
						const float height		= (box.yMax - box.yMin);
						const float width		= (box.xMax - box.xMin);
						_quickTip.txtLayer->establecerTraslacion(-box.xMin - (width * 0.5f), -box.yMax - marginI - marginBtm);
						_quickTip.bg->redimensionar(-marginI - (width * 0.5f), -(marginTop + marginI + height + marginI + marginBtm), (marginI + width + marginI), (marginTop + marginI + height + marginI + marginBtm));
					}
				}
				break;
			case ENEscenaTipAim_Left:
				{
					//Internal organization
					{
						float xLeft = 0.0f;
						if(_quickTip.icoLeft->visible()){
							const NBCajaAABB box = _quickTip.icoLeft->cajaAABBLocal();
							if(xLeft != 0.0f) xLeft += marginIco;
							_quickTip.icoLeft->establecerTraslacion(xLeft - box.xMin, -box.yMax + ((box.yMax - box.yMin) * 0.5f));
							xLeft += (box.xMax - box.xMin);
						}
						if(_quickTip.txt->visible()){
							AUFuenteRender* fnt = TSFonts::font(ENTSFont_ContentMid);
							_quickTip.txt->establecerAlineaciones(ENNBTextLineAlignH_Left, ENNBTextAlignV_Center);
							_quickTip.txt->establecerFuenteRender(fnt);
							_quickTip.txt->establecerTexto(text, maxWidth);
							{
								const NBCajaAABB box = _quickTip.txt->cajaAABBLocalCalculada();
								if(xLeft != 0.0f) xLeft += marginIco;
								_quickTip.txt->establecerTraslacion(xLeft - box.xMin, -box.yMax + ((box.yMax - box.yMin) * 0.5f));
								xLeft += (box.xMax - box.xMin);
							}
							
						}
						if(_quickTip.icoRight->visible()){
							const NBCajaAABB box = _quickTip.icoRight->cajaAABBLocal();
							if(xLeft != 0.0f) xLeft += marginIco;
							_quickTip.icoRight->establecerTraslacion(xLeft - box.xMin, -box.yMax + ((box.yMax - box.yMin) * 0.5f));
							xLeft += (box.xMax - box.xMin);
						}
					}
					//Global organization
					{
						const NBCajaAABB box	= _quickTip.txtLayer->cajaAABBLocalCalculada();
						const float height		= (box.yMax - box.yMin);
						const float width		= (box.xMax - box.xMin);
						_quickTip.txtLayer->establecerTraslacion(-box.xMin + marginBtm + marginI, -box.yMax + (height * 0.5f));
						_quickTip.bg->establecerRotacion(-90.0f);
						_quickTip.bg->redimensionar(-marginI - (height * 0.5f), marginTop + marginI + width + marginI + marginBtm, marginI + height + marginI, -(marginTop + marginI + width + marginI + marginBtm));
					}
				}
				break;
			case ENEscenaTipAim_Right:
				{
					//Internal organization
					{
						float xLeft = 0.0f;
						if(_quickTip.icoRight->visible()){
							const NBCajaAABB box = _quickTip.icoRight->cajaAABBLocal();
							if(xLeft != 0.0f) xLeft += marginIco;
							_quickTip.icoRight->establecerTraslacion(xLeft - box.xMin, -box.yMax + ((box.yMax - box.yMin) * 0.5f));
							xLeft += (box.xMax - box.xMin);
						}
						if(_quickTip.txt->visible()){
							AUFuenteRender* fnt = TSFonts::font(ENTSFont_ContentMid);
							_quickTip.txt->establecerAlineaciones(ENNBTextLineAlignH_Right, ENNBTextAlignV_Center);
							_quickTip.txt->establecerFuenteRender(fnt);
							_quickTip.txt->establecerTexto(text, maxWidth);
							{
								const NBCajaAABB box = _quickTip.txt->cajaAABBLocalCalculada();
								if(xLeft != 0.0f) xLeft += marginIco;
								_quickTip.txt->establecerTraslacion(xLeft - box.xMin, -box.yMax + ((box.yMax - box.yMin) * 0.5f));
								xLeft += (box.xMax - box.xMin);
							}
						}
						if(_quickTip.icoLeft->visible()){
							const NBCajaAABB box = _quickTip.icoLeft->cajaAABBLocal();
							if(xLeft != 0.0f) xLeft += marginIco;
							_quickTip.icoLeft->establecerTraslacion(xLeft - box.xMin, -box.yMax + ((box.yMax - box.yMin) * 0.5f));
							xLeft += (box.xMax - box.xMin);
						}
					}
					//Global organization
					{
						const NBCajaAABB box	= _quickTip.txtLayer->cajaAABBLocalCalculada();
						const float height		= (box.yMax - box.yMin);
						const float width		= (box.xMax - box.xMin);
						_quickTip.txtLayer->establecerTraslacion(-box.xMax - marginBtm - marginI, -box.yMax + (height * 0.5f));
						_quickTip.bg->establecerRotacion(90.0f);
						_quickTip.bg->redimensionar(-marginI - (height * 0.5f), marginTop + marginI + width + marginI + marginBtm, marginI + height + marginI, -(marginTop + marginI + width + marginI + marginBtm));
					}
				}
				break;
			 default: //ENEscenaTipAim_Down
				{
					//Internal organization
					{
						float yTop = 0.0f;
						if(_quickTip.icoRight->visible()){
							const NBCajaAABB box = _quickTip.icoRight->cajaAABBLocal();
							if(yTop != 0.0f) yTop -= marginIco;
							_quickTip.icoRight->establecerTraslacion(-box.xMin - ((box.xMax - box.xMin) * 0.5f), yTop - box.yMax);
							yTop -= (box.yMax - box.yMin);
						}
						if(_quickTip.txt->visible()){
							AUFuenteRender* fnt = TSFonts::font(ENTSFont_ContentMid);
							_quickTip.txt->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_FromBottom);
							_quickTip.txt->establecerFuenteRender(fnt);
							_quickTip.txt->establecerTexto(text, maxWidth);
							{
								const NBCajaAABB box = _quickTip.txt->cajaAABBLocalCalculada();
								if(yTop != 0.0f) yTop -= marginIco;
								_quickTip.txt->establecerTraslacion(-box.xMin - ((box.xMax - box.xMin) * 0.5f), yTop - box.yMax);
			 					yTop -= (box.yMax - box.yMin);
							}
						}
						if(_quickTip.icoLeft->visible()){
							const NBCajaAABB box = _quickTip.icoLeft->cajaAABBLocal();
							if(yTop != 0.0f) yTop -= marginIco;
							_quickTip.icoLeft->establecerTraslacion(-box.xMin - ((box.xMax - box.xMin) * 0.5f), yTop - box.yMax);
			 				yTop -= (box.yMax - box.yMin);
						}
					}
					//Global organization
					{
						const NBCajaAABB box	= _quickTip.txtLayer->cajaAABBLocal();
						const float height		= (box.yMax - box.yMin);
						const float width		= (box.xMax - box.xMin);
						_quickTip.txtLayer->establecerTraslacion(-box.xMin - (width * 0.5f), -box.yMin + marginI + marginBtm);
						_quickTip.bg->redimensionar(-marginI - (width * 0.5f), marginTop + marginI + height + marginI + marginBtm, marginI + width + marginI, -(marginTop + marginI + height + marginI + marginBtm));
					}
				}
				break;
		}
		//Animate
		{
			_animator->quitarAnimacionesDeObjeto(_quickTip.layer);
			//
			_quickTip.layer->establecerTraslacion(pos.x, pos.y);
			_quickTip.layer->establecerVisible(false);
			_quickTip.layer->establecerMultiplicadorAlpha8(255);
			//
			_animator->animarVisible(_quickTip.layer, true, 0.25f, ENAnimPropVelocidad_Desacelerada, 0.0f);
			_animator->animarVisible(_quickTip.layer, false, 0.50f, ENAnimPropVelocidad_Lineal, secsShow);
		}
	}
	return r;
}

BOOL AUScenesAdmin::topAddObject(AUEscenaObjeto* object){
	BOOL r = FALSE;
	if(_topLayer != NULL){
		if(object != NULL){
			_topLayer->agregarObjetoEscena(object);
		}
		r = TRUE;
	}
	return r;
}

//

/*void AUScenesAdmin::appProcesarNotificacionLocal(const char* grp, const SI32 localId, const char* objTip, const SI32 objId){
 AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::procesarNotificacionLocal")
 if(_scenesStack.conteo > 0){
 AUAppEscena* escn = (AUAppEscena*)_scenesStack.elem(_scenesStack.conteo - 1);
 escn->appProcesarNotificacionLocal(grp, localId, objTip, objId);
 PRINTF_WARNING("Notificacion, CONSUMIENDO por '%s'.\n", escn->nombreUltimaClase());
 } else {
 PRINTF_WARNING("Notificacion, IGNORANDO, no hay escena en la pila.\n")
 }
 AU_GESTOR_PILA_LLAMADAS_POP_3
 }*/

//

bool AUScenesAdmin::escenasTieneAccionesPendiente(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasTieneAccionesPendiente")
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return ((_loadInfoSequence || _loadLobby || _loadPrevScene) && _loadTransCur == NULL);
}

void AUScenesAdmin::escenasEjecutaAccionesPendientes(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasEjecutaAccionesPendientes")
	if((_loadInfoSequence || _loadLobby || _loadPrevScene) && _loadTransCur == NULL){
		//PRINTF_INFO("Iniciando carga de escenas pendientes.\n");
		_loadActionCur = _loadAction;
		if(_loadTransType == ENSceneTransitionType_White){
			//-------------------------------------
			//Iniciar animacion de carga de recurso
			//-------------------------------------
			const STNBColor8 color = NBST_P(STNBColor8, 255, 255, 255, 255 );
			_loadTransCur	= new(this) AUAppTransicionConColorSolido(_iScene, this, _loadTransSecsDur, color, ENGestorTexturaModo_cargaInmediata);
			_loadTransCur->tickTransicion(0.0f); //Inicializar
		} else if(_loadTransType == ENSceneTransitionType_Black){
			//-------------------------------------
			//Iniciar animacion de carga de recurso
			//-------------------------------------
			const STNBColor8 color = NBST_P(STNBColor8, 0, 0, 0, 255 );
			_loadTransCur	= new(this) AUAppTransicionConColorSolido(_iScene, this, _loadTransSecsDur, color, ENGestorTexturaModo_cargaInmediata);
			_loadTransCur->tickTransicion(0.0f); //Inicializar
		} else if(_loadTransType == ENSceneTransitionType_Cortina){
			//-------------------------------------
			//Iniciar animacion de carga de recurso
			//-------------------------------------
			NBColor8 colorArriba, colorAbajo;
			NBCOLOR_ESTABLECER(colorArriba, 215, 125, 30, 255)
			NBCOLOR_ESTABLECER(colorAbajo, 242, 208, 49, 255)
			_loadTransCur	= new(this) AUAppTransicionConCortina(_iScene, this, ENGestorTexturaModo_cargaInmediata, (_scenesStack.conteo > 0)/*animarEntrada*/,  colorAbajo, colorAbajo, colorArriba, colorArriba);
			_loadTransCur->tickTransicion(0.0f); //Inicializar
		} else if(_loadTransType == ENSceneTransitionType_Logo){
			//-------------------------------------
			//Iniciar animacion de carga de recurso
			//-------------------------------------
			AUScenesTransition* transicion = new(this) AUScenesTransition(_iScene, this, (_scenesStack.conteo > 0)/*animarEntrada*/, _loadTransSecsDur);
			transicion->tickTransicion(0.0f); //Inicializar
			_loadTransCur	= transicion;
		} else if(_loadTransType == ENSceneTransitionType_Fondo){
			//-------------------------------------
			//Iniciar animacion de carga de recurso
			//-------------------------------------
			_loadTransCur	= new(this) AUAppTransicionConFondo(_iScene, this, ENGestorTexturaModo_cargaInmediata, _fondoRuta.str(), _fondoCapa, _fondoSprite, NULL, "thinstream/escena/fondos/objCosturaP.png");
			_loadTransCur->tickTransicion(0.0f); //Inicializar
		} else if(_loadTransType == ENSceneTransitionType_Captura){
			//-------------------------------------
			//Iniciar animacion de carga de recurso
			//-------------------------------------
			_loadTransCur = new(this) AUAppTransicionConCaptura(_iScene, this, ENGestorTexturaModo_cargaInmediata);
			_loadTransCur->tickTransicion(0.0f); //Inicializar
		} else {
			//-------------------------------------
			//Realizar inmediatamente la carga de recurso
			//-------------------------------------
			//Liberar escena actual
			this->escenasLiberar();
			//Cargar escena y precargar sus recursos
			NBGestorTexturas::modoCargaTexturasPush(ENGestorTexturaModo_cargaInmediata);
			this->escenasCargar();
			NBGestorTexturas::modoCargaTexturasPop();
			//Liberar los recursos que quedaron sin uso
			this->escenasLiberarRecursosSinUso();
			//Cargar recursos pendientes
			while(NBGestorTexturas::texPendienteOrganizarConteo()!=0){ NBGestorTexturas::texPendienteOrganizarProcesar(9999); }
#			ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
			while(NBGestorSonidos::conteoBufferesPendientesDeCargar()!=0){ NBGestorSonidos::cargarBufferesPendientesDeCargar(9999); }
#			endif
			//Finalizar proceso
			NBGestorTexturas::generarMipMapsDeTexturas();
			//Colocar nueva escena en primer plano
			this->escenasColocarEnPrimerPlano();
#			ifdef CONFIG_NB_GESTOR_MEMORIA_IMPLEMENTAR_GRUPOS_ZONAS_MEMORIA
			NBGestorMemoria::liberarZonasSinUso();
#			endif
		}
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

bool AUScenesAdmin::escenasEnProcesoDeCarga(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasEnProcesoDeCarga")
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return (_loadTransCur != NULL);
}

void AUScenesAdmin::escenasLiberarRecursosSinUso(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasLiberarRecursosSinUso")
	NBGestorEscena::liberarRecursosCacheRenderEscenas();
	UI16 conteoLiberados;
	do {
		conteoLiberados = 0;
		conteoLiberados += NBGestorCuerpos::liberarPlantillasSinReferencias();
		conteoLiberados += NBGestorAnimaciones::liberarAnimacionesSinReferencias();
		conteoLiberados += NBGestorTexturas::liberarTexturasSinReferencias();
#		ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
		conteoLiberados += NBGestorSonidos::liberarBufferesSinReferencias();
#		endif
		//conteoLiberados +=  //PEDIENTE: aplicar autoliberaciones
	} while(conteoLiberados != 0);
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

void AUScenesAdmin::privResetLoadingState(){
	_loadInfoSequence	= false;
	_loadInfoSequenceMode = ENSceneInfoMode_Count;
	_loadLobby			= false;
	_loadPrevScene		= false;
}

void AUScenesAdmin::setViewPortTransitionBoxFocus(const STNBAABox box){
	_loadTransBoxFocus = box;
}

UI32 AUScenesAdmin::scenesStackSize(void){
	return _scenesStack.conteo;
}

void AUScenesAdmin::loadInfoSequence(const ENSceneAction action, const ENSceneInfoMode mode, const ENSceneTransitionType transitionType, const float transitionSecsDur){
	this->privResetLoadingState();
	_loadInfoSequence	= true;
	_loadInfoSequenceMode = mode;
	_loadAction			= action;
	_loadTransType		= transitionType;
	_loadTransSecsDur	= transitionSecsDur;
}

void AUScenesAdmin::loadLobby(const ENSceneAction action, const ENSceneTransitionType transitionType, const float transitionSecsDur){
	this->privResetLoadingState();
	_loadLobby			= true;
	_loadAction			= action;
	_loadTransType		= transitionType;
	_loadTransSecsDur	= transitionSecsDur;
}

void AUScenesAdmin::loadPreviousScene(const ENSceneTransitionType transitionType, const float transitionSecsDur){
	this->privResetLoadingState();
	if(_scenesStack.conteo > 0){
		_loadPrevScene		= true;
		_loadAction			= ENSceneAction_Pop;
		_loadTransType		= transitionType;
		_loadTransSecsDur	= transitionSecsDur;
	}
}

void AUScenesAdmin::reloadInitialContent(const ENSceneTransitionType transitionType, const float transitionSecsDur){
	if(_coreClt != NULL){
		/*if(!TSClient_isDeviceKeySet(_coreClt)){
			PRINTF_INFO("Starting with no-deviceId.\n");
			this->loadInfoSequence(ENSceneAction_ReplaceAll, ENSceneInfoMode_Register, transitionType, transitionSecsDur);
		} else {*/
			PRINTF_INFO("Starting with deviceId.\n");
			this->loadLobby(ENSceneAction_ReplaceAll, transitionType, transitionSecsDur);
		//}
	}
}

//

void AUScenesAdmin::escenasColocarEnPrimerPlano(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasColocarEnPrimerPlano")
	UI16 i; const UI16 conteo = _scenesStack.conteo;
	for(i = 0; i < conteo; i++){
		AUAppEscena* escena = (AUAppEscena*)_scenesStack.elemento[i];
		if((i + 1) < conteo){
			escena->escenaQuitarDePrimerPlano();
		} else {
			escena->escenaColocarEnPrimerPlano();
		}
	}
	//Apply new orientation state
	{
		this->privSyncOrientState();
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

void AUScenesAdmin::escenasQuitarDePrimerPlano(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasQuitarDePrimerPlano")
	UI16 i; const UI16 conteo = _scenesStack.conteo;
	for(i = 0; i < conteo; i++){
		AUAppEscena* escena = (AUAppEscena*)_scenesStack.elemento[i];
		escena->escenaQuitarDePrimerPlano();
	}
	NBGestorEscena::establecerColorFondo(_iScene, 0.0f, 0.0f, 0.0f, 1.0f);
	//Apply new orientation state
	{
		this->privSyncOrientState();
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

bool AUScenesAdmin::escenasAnimandoSalida(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasAnimandoSalida")
	bool r = false;
	if(_scenesStack.conteo > 0){
		AUAppEscena* escena = (AUAppEscena*)_scenesStack.elemento[_scenesStack.conteo - 1];
		r = (r || escena->escenaEstaAnimandoSalida());
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return r;
}

void AUScenesAdmin::escenasAnimarSalida(){
	if(_scenesStack.conteo > 0){
		AUAppEscena* escena = (AUAppEscena*)_scenesStack.elemento[_scenesStack.conteo - 1];
		escena->escenaAnimarSalida();
	}
}

void AUScenesAdmin::escenasAnimarEntrada(){
	if(_scenesStack.conteo > 0){
		AUAppEscena* escena = (AUAppEscena*)_scenesStack.elemento[_scenesStack.conteo - 1];
		escena->escenaAnimarEntrada();
	}
}

void AUScenesAdmin::escenasLiberar(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasLiberar")
	while(_scenesStack.conteo > 0){
		AUAppEscena* escena = (AUAppEscena*)_scenesStack.elemento[_scenesStack.conteo - 1];
		NBASSERT(escena->conteoReferencias() >= 1)
		escena->escenaQuitarDePrimerPlano();
		if(_loadActionCur == ENSceneAction_ReplaceAll || _loadActionCur == ENSceneAction_ReplaceTop || _loadActionCur == ENSceneAction_Pop){
			//Remove scene
			_scenesStack.quitarElementoEnIndice(_scenesStack.conteo - 1);
			if(_loadActionCur == ENSceneAction_ReplaceTop || _loadActionCur == ENSceneAction_Pop){
				break; //only once
			}
		} else {
			break; //only unload
		}
	}
	NBGestorEscena::establecerColorFondo(_iScene, 0.0f, 0.0f, 0.0f, 1.0f);
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

void AUScenesAdmin::escenasCargar(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::escenasCargar")
	CICLOS_CPU_TIPO cicloInicioCreacionEscena; CICLOS_CPU_HILO(cicloInicioCreacionEscena)
	if(_loadInfoSequence){
		AUSceneIntro* escenaIntro = new(this) AUSceneIntro(_coreClt, _iScene, this, _loadInfoSequenceMode); NB_DEFINE_NOMBRE_PUNTERO(escenaIntro, "AUScenesAdmin::escenaIntro")
		_scenesStack.agregarElemento(escenaIntro);
		escenaIntro->liberar(NB_RETENEDOR_THIS);
		_loadInfoSequence = false;
		_loadInfoSequenceMode = ENSceneInfoMode_Count;
		PRINTF_INFO("Intro cargado (iScene %d).\n", _iScene);
	} else if(_loadLobby){
		AUSceneLobby* scene = new(this) AUSceneLobby(_coreClt, _iScene, this); NB_DEFINE_NOMBRE_PUNTERO(scene, "AUScenesAdmin::scene")
		_scenesStack.agregarElemento(scene);
		scene->liberar(NB_RETENEDOR_THIS);
		_loadLobby = false;
		PRINTF_INFO("Lobby cargado (iScene %d).\n", _iScene);
	} else if(_loadPrevScene){
		_loadPrevScene = false;
		PRINTF_INFO("Last scene cargada (iScene %d).\n", _iScene);
	}
	CICLOS_CPU_TIPO cicloFin; CICLOS_CPU_HILO(cicloFin)
	CICLOS_CPU_TIPO ciclosCpuPorSeg; CICLOS_CPU_POR_SEGUNDO(ciclosCpuPorSeg);
	PRINTF_INFO("TIEMPO DE CREACION DE ESCENA: %.2f segs\n", (float)(cicloFin-cicloInicioCreacionEscena)/(float)ciclosCpuPorSeg);
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

SI32 AUScenesAdmin::privLoadPendingScene(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::privLoadPendingScene")
	if(NBGestorTexturas::texPendienteOrganizarConteo()!=0){
		NBGestorTexturas::texPendienteOrganizarProcesar(1);
	}
#	ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
	else if(NBGestorSonidos::conteoBufferesPendientesDeCargar()!=0){
		NBGestorSonidos::cargarBufferesPendientesDeCargar(1);
	}
#	endif
	//Actualizar la presentacion
	SI32 cantidadRestante = 0;
	cantidadRestante += NBGestorTexturas::texPendienteOrganizarConteo();
#	ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
	cantidadRestante += NBGestorSonidos::conteoBufferesPendientesDeCargar();
#	endif
	//if(_cargaConteoRecursos!=0){
	//	privInlineIconosMotorMarcarProgresoCarga(((float)_cargaConteoRecursos-(float)cantidadRestante)/(float)_cargaConteoRecursos);
	//}
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return  cantidadRestante;
}

//ViewPort
void AUScenesAdmin::puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after){
	const BOOL resChanged = (before.dpi.width != after.dpi.width || before.dpi.height != after.dpi.height || before.ppi.width != after.ppi.width || before.ppi.height != after.ppi.height);
	//Calculate limits
	this->privCalculateLimits();
	//Purge animations
	//Todo: remove (this makes tooltips not visible if trigered by the first 'tick' after returning)
	if(resChanged){ //To avoid calls made by a focus-returned or refresh actions.
		_animator->purgarAnimaciones();
	}
	//Organize
	this->privOrganizeAll(false);
	//Call parent method
	AUAppEscenasAdmin::puertoDeVisionModificado(iEscena, before, after);
	//Reload scene
	if(resChanged){
		TSFonts::reInit(_iScene);
		this->reloadInitialContent(ENSceneTransitionType_Ninguna, 0.0f);
	}
}

//

void AUScenesAdmin::privCalculateLimits(){
	const NBCajaAABB box = NBGestorEscena::cajaProyeccionGrupo(_iScene, ENGestorEscenaGrupo_GUI);
	NBMargenes marginsScn;
	NBMemory_setZeroSt(marginsScn, NBMargenes);
	//full box
	{
		_lastBox.xMin = box.xMin;
		_lastBox.xMax = box.xMax;
		_lastBox.yMin = box.yMin;
		_lastBox.yMax = box.yMax;
	}
	//internal content box
	{
		_lastBoxUsable = _lastBox;
		//Remove top bar
		{
			const float padPxs	= NBMngrOSTools::getWindowTopPaddingPxs();
			marginsScn.top		= NBGestorEscena::altoPuertoAGrupo(_iScene, ENGestorEscenaGrupo_GUI, padPxs);
		}
		//Remove btm bar
		{
			const float padPxs	= NBMngrOSTools::getWindowBtmPaddingPxs();
			marginsScn.bottom	= NBGestorEscena::altoPuertoAGrupo(_iScene, ENGestorEscenaGrupo_GUI, padPxs);
		}
		//Remove bottom space if big screen
		/*{
			NBTamano scnSize;		NBCAJAAABB_TAMANO(scnSize, _lastBox)
			const NBTamano inchSz	= NBGestorEscena::tamanoEscenaAPulgadas(_iScene, scnSize);
			const float inchDiag	= sqrtf((inchSz.ancho * inchSz.ancho) + (inchSz.alto * inchSz.alto));
			if(inchDiag >= 4.5f){
				//iPhone XR: 6.1 inch
				//iPhone 7: 4.7 inch
				//iPhone SE: 4.0 inch
				_lastBoxUsable.yMin += NBGestorEscena::altoPulgadasAEscena(_iScene, 0.15f);
			}
		}*/
	}
	//Set scene margins (area no usable)
	{
		_lastBoxUsable.yMax	-= marginsScn.top;
		_lastBoxUsable.yMin	+= marginsScn.bottom;
		NBGestorEscena::setSceneMarginsPx(_iScene, marginsScn);
	}
}

void AUScenesAdmin::privOrganizeAll(const bool animar){
	//
}

//Pop up

void AUScenesAdmin::popShowInfo(const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content){
	void* popRef = this->popCreate(mode, alignH, title, content, NULL, NULL, NULL, NULL, ENScenePopBtnColor_Clear);
	this->popShow(popRef);
	this->popRelease(popRef);
}

void* AUScenesAdmin::popCreate(const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content, IPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam, const char* hideBtnTxt, const ENScenePopBtnColor hideBtnColor, const ENIMsgboxSize sizeH, const ENIMsgboxSize sizeV){
	STScenesPop* itm = NBMemory_allocType(STScenesPop);
	NBMemory_setZeroSt(*itm, STScenesPop);
	{
		const STNBColor8 mainColor	= TSColors::colorDef(ENTSColor_MainColor)->normal;
		const STNBColor8 warnColor	= NBST_P(STNBColor8, 255, 165, 0, 255 );
		const STNBColor8 lightColor	= NBST_P(STNBColor8, 235, 235, 235, 255 );
		const STNBColor8 whiteColor	= NBST_P(STNBColor8, 255, 255, 255, 255 );
		const STNBColor8 blackColor	= NBST_P(STNBColor8, 0, 0, 0, 255 );
		const float marginI			= NBGestorEscena::altoPulgadasAEscena(_iScene, 0.05f);
		const float btnMarginV		= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.07f); //Internal margin (top & bottom)
		//NBMargenes boxMargins;	boxMargins.left = boxMargins.right = 0.0f; boxMargins.top	= boxMargins.bottom = marginI;
		NBMargenes optsMrgnIn, optsMrgnOut;
		{
			const float mrgnOptsIn		= NBGestorEscena::altoPulgadasAEscena(_iScene, 0.10f); //0.10
			const float mrgnOptsOut		= NBGestorEscena::altoPulgadasAEscena(_iScene, 0.05f); //0.10
			NBMemory_setZero(optsMrgnIn);
			NBMemory_setZero(optsMrgnOut);
			switch(sizeH) {
				case ENIMsgboxSize_AllWindow:
					optsMrgnIn.left		= optsMrgnIn.right = mrgnOptsIn;
					optsMrgnOut.left	= optsMrgnOut.right = 0.0f;
					break;
				case ENIMsgboxSize_AllUsable:
					optsMrgnIn.left		= optsMrgnIn.right = mrgnOptsIn;
					optsMrgnOut.left	= optsMrgnOut.right = 0.0f;
					break;
				default:
					//ENIMsgboxSize_Auto, ENIMsgboxSize_Modal
					optsMrgnIn.left		= optsMrgnIn.right = mrgnOptsIn;
					optsMrgnOut.left	= optsMrgnOut.right = mrgnOptsOut;
					break;
			}
			switch(sizeV) {
				case ENIMsgboxSize_AllWindow:
					optsMrgnIn.top		= (_lastBox.yMax - _lastBoxUsable.yMax) + mrgnOptsIn * 0.75f;
					optsMrgnIn.bottom	= (_lastBoxUsable.yMin - _lastBox.yMin) + mrgnOptsIn * 0.75f;
					optsMrgnOut.top		= 0.0f;
					optsMrgnOut.bottom	= 0.0f;
					break;
				case ENIMsgboxSize_AllUsable:
					optsMrgnIn.top		= optsMrgnIn.bottom = mrgnOptsIn * 0.75f;
					optsMrgnOut.top		= (_lastBox.yMax - _lastBoxUsable.yMax) + 0.0f;
					optsMrgnOut.bottom	= (_lastBoxUsable.yMin - _lastBox.yMin) + 0.0f;
					break;
				default:
					//ENIMsgboxSize_Auto, ENIMsgboxSize_Modal
					optsMrgnIn.top		= optsMrgnIn.bottom = mrgnOptsIn * 0.75f;
					optsMrgnOut.top		= (_lastBox.yMax - _lastBoxUsable.yMax) + mrgnOptsOut;
					optsMrgnOut.bottom	= (_lastBoxUsable.yMin - _lastBox.yMin) + mrgnOptsOut;
					break;
			}
		}
		AUFuenteRender* fntTittle	= TSFonts::font(ENTSFont_ContentTitle);
		AUFuenteRender* fntContent	= TSFonts::font(ENTSFont_ContentMid);
		AUFuenteRender* fntBtns		= TSFonts::font(ENTSFont_Btn);
		AUTextura* bgTex			= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#btn");
		AUTextura* scrollTex		= NBGestorTexturas::texturaDesdeArchivoPNG("thinstream/scrolllbarMed.png");
		STNBColor8 scrollColor		= TSColors::colorDef(ENTSColor_MainColor)->normal; scrollColor.a = TS_COLOR_SCROLL_BAR_ALPHA8_TEXT;
		//
		NBString_initWithStr(&itm->title, title);
		//
		itm->onFocus		= FALSE;
		itm->hideBtnAdded	= FALSE;
		itm->sizeH			= sizeH;
		itm->sizeV			= sizeV;
		itm->msgBox			= new(this) AUIMsgBox(fntTittle, fntContent, fntBtns, bgTex, scrollTex, scrollColor, marginI, optsMrgnIn);
		itm->msgBox->setHideWhenGlassTouched(FALSE);
		itm->msgBox->setTitleAlignment(ENNBTextLineAlignH_Center);
		itm->msgBox->setContentAlignment(ENNBTextLineAlignH_Center);
		itm->msgBox->setMarginsOuter(optsMrgnOut);
		itm->msgBox->setTitleColor(mainColor);
		//itm->msgBox->setCloseIcoTexture(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#btnCancel"));
		itm->msgBox->setDefaultButtonsMargins(0.0f, 0.0f, btnMarginV, btnMarginV, btnMarginV);
		if(hideBtnColor >= 0 && hideBtnColor < ENScenePopBtnColor_Count){
			const char* strLoc = TSClient_getStr(_coreClt, "hideBtnText", "Hide");
			if(hideBtnColor == ENScenePopBtnColor_Main){
				itm->msgBox->addButton("hide", ENIMsgboxBtnAction_AutohideAndNotify, NBString_strIsEmpty(hideBtnTxt) ? strLoc : hideBtnTxt, NULL, bgTex, mainColor, whiteColor);
			} else {
				itm->msgBox->addButton("hide", ENIMsgboxBtnAction_AutohideAndNotify, NBString_strIsEmpty(hideBtnTxt) ? strLoc : hideBtnTxt, NULL, bgTex, lightColor, blackColor);
			}
			itm->hideBtnAdded = TRUE;
		}
		itm->msgBox->setMsgBoxListener(this);
		itm->msgBox->updateContent(alignH, itm->title.str, content);
		_topLayer->agregarObjetoEscena(itm->msgBox);
		//Textures
		itm->texs.working	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner"); itm->texs.working->retener(NB_RETENEDOR_THIS);
		itm->texs.sucess	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#btnOk"); itm->texs.sucess->retener(NB_RETENEDOR_THIS);
		itm->texs.warn		= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#warning"); itm->texs.warn->retener(NB_RETENEDOR_THIS);
		itm->texs.error		= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#warning"); itm->texs.error->retener(NB_RETENEDOR_THIS);
		//
		itm->mode			= mode;
		itm->admin			= this;
		itm->icon			= new(this) AUEscenaSprite();
		itm->icon->establecerEscalacion(2.0f);
		if(itm->mode == ENScenePopMode_Error){
			itm->icon->establecerMultiplicadorColor8(mainColor.r, mainColor.g, mainColor.b, mainColor.a);
			itm->icon->establecerTextura(itm->texs.error);
			itm->icon->redimensionar(itm->texs.error);
			itm->msgBox->setCustomContentTop(itm->icon);
		} else if(itm->mode == ENScenePopMode_Warn){
			itm->icon->establecerMultiplicadorColor8(warnColor.r, warnColor.g, warnColor.b, warnColor.a);
			itm->icon->establecerTextura(itm->texs.warn);
			itm->icon->redimensionar(itm->texs.warn);
			itm->msgBox->setCustomContentTop(itm->icon);
		} else if(itm->mode == ENScenePopMode_Sucess){
			itm->icon->establecerMultiplicadorColor8(mainColor.r, mainColor.g, mainColor.b, mainColor.a);
			itm->icon->establecerTextura(itm->texs.sucess);
			itm->icon->redimensionar(itm->texs.sucess);
			itm->msgBox->setCustomContentTop(itm->icon);
		} else if(itm->mode == ENScenePopMode_Working){
			itm->icon->establecerMultiplicadorColor8(whiteColor.r, whiteColor.g, whiteColor.b, whiteColor.a);
			itm->icon->establecerTextura(itm->texs.working);
			itm->icon->redimensionar(itm->texs.working);
			itm->msgBox->setCustomContentTop(itm->icon);
		} else {
			itm->msgBox->setCustomContentTop(NULL);
		}
		//Listener
		itm->lstnr		= lstnr;
		itm->lstnObj	= lstnObj; if(itm->lstnObj != NULL) itm->lstnObj->retener(NB_RETENEDOR_THIS);
		itm->lstnrParam	= lstnrParam;
		//
		itm->retainCount = 1;
	}
	//Add to array
	{
		NBThreadMutex_lock(&_pops.mutex);
		NBArray_addValue(&_pops.array, itm);
		NBThreadMutex_unlock(&_pops.mutex);
	}
	//
	return itm;
}

STNBSize AUScenesAdmin::popGetContentSizeForSceneLyr(void* ref, const SI32 iScene, const ENGestorEscenaGrupo iGrp, STNBSize* dstBoxSz, STNBPoint* dstRelPos, NBMargenes* dstUnusableInnerContentSz){
	STNBSize r = NBST_P(STNBSize, 0, 0 );
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBMargenes marginsInn = itm->msgBox->getMarginsInner();
			NBMargenes marginsOut = itm->msgBox->getMarginsOuter();
			r = itm->msgBox->getContentSizeForSceneLyr(iScene, iGrp, dstBoxSz, dstRelPos, &marginsInn, &marginsOut, dstUnusableInnerContentSz, ENIMsgboxContentMskBit_Title | ENIMsgboxContentMskBit_Buttons | ENIMsgboxContentMskBit_MarginInt, itm->sizeH, itm->sizeV);
			break;
		}
	} NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
	return r;
}

void AUScenesAdmin::popSetCustomContentTop(void* ref, AUEscenaObjeto* content){
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			itm->msgBox->setCustomContentTop(content);
			break;
		}
	} NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
}

void AUScenesAdmin::popSetCustomContentBtm(void* ref, AUEscenaObjeto* content){
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			itm->msgBox->setCustomContentBtm(content);
			break;
		}
	} NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
}

void AUScenesAdmin::popAddOption(void* ref, const char* optionId, const char* text, const ENScenePopBtnColor color, const ENIMsgboxBtnAction action){
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBASSERT(itm->retainCount > 0);
			const STNBColor8 mainColor	= TSColors::colorDef(ENTSColor_MainColor)->normal;
			const STNBColor8 lightColor	= NBST_P(STNBColor8, 235, 235, 235, 255 );
			const STNBColor8 whiteColor	= NBST_P(STNBColor8, 255, 255, 255, 255 );
			const STNBColor8 blackColor	= NBST_P(STNBColor8, 0, 0, 0, 255 );
			AUTextura* bgTex			= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#btn");
			if(color == ENScenePopBtnColor_Main){
				itm->msgBox->addButtonAtTop(optionId, action, text, NULL, bgTex, mainColor, whiteColor);
			} else {
				itm->msgBox->addButtonAtTop(optionId, action, text, NULL, bgTex, lightColor, blackColor);
			}
			break;
		}
	} NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
}

void AUScenesAdmin::popUpdate(void* ref, const ENScenePopMode mode, const ENNBTextLineAlignH alignH, const char* title, const char* content){
	NBThreadMutex_lock(&_pops.mutex);
	const STNBColor8 mainColor	= TSColors::colorDef(ENTSColor_MainColor)->normal;
	const STNBColor8 warnColor	= NBST_P(STNBColor8, 255, 165, 0, 255 );
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBASSERT(itm->retainCount > 0);
			itm->mode = mode;
			//Update icons
			if(itm->mode == ENScenePopMode_Error){
				itm->icon->establecerMultiplicadorColor8(mainColor.r, mainColor.g, mainColor.b, mainColor.a);
				itm->icon->establecerRotacion(0.0f);
				itm->icon->establecerTextura(itm->texs.error);
				itm->icon->redimensionar(itm->texs.error);
				itm->msgBox->setCustomContentTop(itm->icon);
			} else if(itm->mode == ENScenePopMode_Warn){
				itm->icon->establecerMultiplicadorColor8(warnColor.r, warnColor.g, warnColor.b, warnColor.a);
				itm->icon->establecerRotacion(0.0f);
				itm->icon->establecerTextura(itm->texs.warn);
				itm->icon->redimensionar(itm->texs.warn);
				itm->msgBox->setCustomContentTop(itm->icon);
			} else if(itm->mode == ENScenePopMode_Sucess){
				itm->icon->establecerMultiplicadorColor8(mainColor.r, mainColor.g, mainColor.b, mainColor.a);
				itm->icon->establecerRotacion(0.0f);
				itm->icon->establecerTextura(itm->texs.sucess);
				itm->icon->redimensionar(itm->texs.sucess);
				itm->msgBox->setCustomContentTop(itm->icon);
			} else if(itm->mode == ENScenePopMode_Working){
				itm->icon->establecerMultiplicadorColor8(mainColor.r, mainColor.g, mainColor.b, mainColor.a);
				itm->icon->establecerTextura(itm->texs.working);
				itm->icon->redimensionar(itm->texs.working);
				itm->msgBox->setCustomContentTop(itm->icon);
			} else {
				itm->icon->establecerRotacion(0.0f);
				itm->msgBox->setCustomContentTop(NULL);
			}
			//Update content visibility
			{
				BOOL doSilentMode = FALSE;
				//Determine if silent mode is required
				if(itm->mode == ENScenePopMode_Working){
					//None extra buttons added
					if(itm->msgBox->getButtonsCount() == (itm->hideBtnAdded ? 1 : 0)){
						//No custom content at btm (workin-icon is top)
						if(!itm->msgBox->isCustomContentBtm()){
							if(TS_POPUPS_IN_SILENT_MODE){
								doSilentMode = TRUE;
							}
						}
					}
				}
				itm->msgBox->setTitleVisible(!doSilentMode);
				itm->msgBox->setContentTextVisible(!doSilentMode);
			}
			itm->msgBox->updateContent(alignH, title, content);
			break;
		}
	} NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
}

void AUScenesAdmin::popShow(void* ref){
	this->popShowAtRelY(ref, AUIMsgBox::getPreferedRelPosVertical(_iScene, 0.0f));
}

void AUScenesAdmin::popShowAtRelY(void* ref, const float relY){
	STScenesPop* itmFnd = NULL;
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBASSERT(itm->retainCount > 0);
			itmFnd = itm;
			break;
		}
	}
	NBASSERT(itmFnd != NULL)
	NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
	//Show (unlocked)
	if(itmFnd != NULL){
		//Update content visibility
		{
			BOOL doSilentMode = FALSE;
			//Determine if silent mode is required
			if(itmFnd->mode == ENScenePopMode_Working){
				//None extra buttons added
				if(itmFnd->msgBox->getButtonsCount() == (itmFnd->hideBtnAdded ? 1 : 0)){
					//No custom content at btm (workin-icon is top)
					if(!itmFnd->msgBox->isCustomContentBtm()){
						if(TS_POPUPS_IN_SILENT_MODE){
							doSilentMode = TRUE;
						}
					}
				}
			}
			itmFnd->msgBox->setTitleVisible(!doSilentMode);
			itmFnd->msgBox->setContentTextVisible(!doSilentMode);
		}
		itmFnd->msgBox->show(relY);
	}
}

void AUScenesAdmin::popHide(void* ref){
	STScenesPop* itmFnd = NULL;
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBASSERT(itm->retainCount > 0);
			itmFnd = itm;
			break;
		}
	}
	NBASSERT(itmFnd != NULL)
	NBASSERT(i < _pops.array.use) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
	//Hide (unlocked)
	if(itmFnd != NULL){
		itmFnd->msgBox->hide();
	}
}

void AUScenesAdmin::popRetain(void* ref){
	BOOL found = FALSE;
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBASSERT(itm->retainCount > 0);
			itm->retainCount++;
			found = TRUE;
			break;
		}
	} NBASSERT(found) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
}

void AUScenesAdmin::popRelease(void* ref){
	BOOL found = FALSE;
	NBThreadMutex_lock(&_pops.mutex);
	SI32 i; for(i = 0 ; i < _pops.array.use; i++){
		STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
		if(itm == ref){
			NBASSERT(itm->retainCount > 0);
			itm->retainCount--;
			if(itm->retainCount == 0 && !itm->onFocus){
				AUScenesAdmin::privPopRelease(itm);
				NBMemory_free(itm);
				NBArray_removeItemAtIndex(&_pops.array, i);
				//PRINTF_INFO("Pop removed while not visible, %d pops remaining.\n", _pops.array.use);
			}
			found = TRUE;
			break;
		}
	} NBASSERT(found) //Should be found
	NBThreadMutex_unlock(&_pops.mutex);
}

SI32 AUScenesAdmin::popVisiblesCount(){
	return _pops.visibleCount;
}

//Request with pop

UI64 AUScenesAdmin::addRequestWithPop(const ENReqPopHideMode hideMode, const char* title, const ENTSRequestId reqId, const char** paramsPairsAndNull, const void* stBody, const UI32 stBodySz, const STNBStructMap* stBodyMap, const UI64 secsWaitToRequest, const float secsWaitToShowPop, IReqPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam){
	UI64 r = 0;
	STNBString body;
	NBString_initWithSz(&body, 4096, 1024 * 256, 0.10f);
	if(stBody != NULL && stBodyMap != NULL){
		if(!NBStruct_stConcatAsJson(&body, stBodyMap, stBody, stBodySz)){
			PRINTF_CONSOLE_ERROR("Coudl not build JSON for request.\n");
		}
	}
	//Add request
	{
		r = this->addRequestWithPop(hideMode, title, reqId, paramsPairsAndNull, body.str, body.length, secsWaitToRequest, secsWaitToShowPop, lstnr, lstnObj, lstnrParam);
	}
	NBString_release(&body);
	return r;
}

UI64 AUScenesAdmin::addRequestWithPop(const ENReqPopHideMode hideMode, const char* title, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWaitToRequest, const float secsWaitToShowPop, IReqPopListener* lstnr, AUObjeto* lstnObj, void* lstnrParam){
	UI64 r = 0;
	STScenesRequestPop* itm = NBMemory_allocType(STScenesRequestPop);
	NBMemory_setZeroSt(*itm, STScenesRequestPop);
	{
		itm->requesting 	= TRUE;
		itm->onFocus		= FALSE;
		itm->secsCur		= 0.0f;
		itm->secsWaitDur	= secsWaitToShowPop;
		itm->reqId			= 0;
		//Listener
		itm->lstnr			= lstnr;
		itm->lstnObj		= lstnObj; if(itm->lstnObj != NULL) itm->lstnObj->retener(NB_RETENEDOR_THIS);
		itm->lstnrParam		= lstnrParam;
		//
		itm->admin			= this;
		itm->hideMode		= hideMode;
		{
			const char* strLoc = TSClient_getStr(_coreClt, "reqBussyText", "processing...");
			itm->popRef		= this->popCreate(ENScenePopMode_Working, ENNBTextLineAlignH_Center, title, strLoc, this, this, itm, NULL, ENScenePopBtnColor_Clear);
		}
	}
	//Add to array
	{
		NBThreadMutex_lock(&_reqsProg.mutex);
		NBArray_addValue(&_reqsProg.pops, itm);
		if(0 == (r = itm->reqId = TSClient_addRequest(_coreClt, reqId, paramsPairsAndNull, body, bodySz, secsWaitToRequest, AUScenesAdmin::reqCallbackFunc, itm))){
			PRINTF_INFO("Request could not be queued.\n"); NBASSERT(FALSE);
			NBArray_removeItemAtIndex(&_reqsProg.pops, _reqsProg.pops.use - 1);
			AUScenesAdmin::privRequestPopRelease(itm);
			NBMemory_free(itm);
			itm = NULL;
		}
		NBThreadMutex_unlock(&_reqsProg.mutex);
	}
	//Show (after added)
	if(itm->secsCur >= itm->secsWaitDur){
		this->popShow(itm->popRef);
	}
	return r;
}

void AUScenesAdmin::privPopRelease(STScenesPop* itm){
	NBASSERT(!itm->onFocus)
	NBASSERT(itm->retainCount == 0)
	//Remove from scene
	if(itm->msgBox != NULL){
		AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->msgBox->contenedor();
		if(parent != NULL) parent->quitarObjetoEscena(itm->msgBox);
	}
	//Release
	NBString_release(&itm->title);
	if(itm->icon != NULL) itm->icon->liberar(NB_RETENEDOR_NULL); itm->icon = NULL;
	if(itm->msgBox != NULL) itm->msgBox->liberar(NB_RETENEDOR_NULL); itm->msgBox = NULL;
	{
		if(itm->texs.working != NULL) itm->texs.working->liberar(NB_RETENEDOR_NULL); itm->texs.working = NULL;
		if(itm->texs.sucess != NULL) itm->texs.sucess->liberar(NB_RETENEDOR_NULL); itm->texs.sucess = NULL;
		if(itm->texs.warn != NULL) itm->texs.warn->liberar(NB_RETENEDOR_NULL); itm->texs.warn = NULL;
		if(itm->texs.error != NULL) itm->texs.error->liberar(NB_RETENEDOR_NULL); itm->texs.error = NULL;
	}
	//Listener
	{
		if(itm->lstnObj != NULL){
			itm->lstnObj->liberar(NB_RETENEDOR_NULL);
			itm->lstnObj = NULL;
		}
		itm->lstnr		= NULL;
		itm->lstnrParam	= NULL;
	}
}

void AUScenesAdmin::privRequestPopRelease(STScenesRequestPop* itm){
	NBASSERT(!itm->requesting)
	NBASSERT(!itm->onFocus)
	//Popup
	if(itm->popRef != NULL){
		itm->admin->popRelease(itm->popRef);
		itm->popRef = NULL;
	}
	//Listener
	{
		if(itm->lstnObj != NULL){
			itm->lstnObj->liberar(NB_RETENEDOR_NULL);
			itm->lstnObj = NULL;
		}
		itm->lstnr		= NULL;
		itm->lstnrParam	= NULL;
	}
}

//

/*void AUScenesAdmin::reqNotifTokenCallbackFunc(const STTSClientChannelRequest* req, void* lParam){
	AUScenesAdmin* obj = (AUScenesAdmin*)lParam;
	NBASSERT(obj != NULL)
	if(obj != NULL){
		NBASSERT(req->reqId == ENTSRequestId_Device_NotifToken_Set)
		NBASSERT(req->uid == obj->_notifs.tokenSendingReqId)
		if(req->reqId == ENTSRequestId_Device_NotifToken_Set && req->uid == obj->_notifs.tokenSendingReqId){
			UI32 statusCode = 0;
			if(req->response != NULL){
				if(req->response->header.statusLine != NULL){
					statusCode = req->response->header.statusLine->statusCode;
				}
			}
			if(statusCode >= 200 && statusCode < 300){
				NBString_setBytes(&obj->_notifs.tokenSent, obj->_notifs.tokenSending.str, obj->_notifs.tokenSending.length);
			}
			obj->_notifs.tokenSendingReqId = 0;
			obj->_notifs.tokenSendingWaitSecs = TS_NOTIFS_AUTH_TOKEN_SEND_WAIT;
			if(statusCode != 200){
				PRINTF_INFO("AUScenesAdmin, ENTSRequestId_Device_NotifToken_Set request ended with code(%d).\n", statusCode);
			}
		}
	}
}*/

void AUScenesAdmin::reqCallbackFunc(const STTSClientChannelRequest* req, void* lParam){
	STScenesRequestPop* itmFnd = NULL;
	BOOL itmRelease = FALSE, isError = TRUE;
	//
	if(req->response != NULL){
		if(req->response->isCompleted){
			if(req->response->header.statusLine != NULL){
				if(req->response->header.statusLine->statusCode >= 200 && req->response->header.statusLine->statusCode < 300){
					isError = FALSE;
				}
			}
		}
	}
	//Look for request
	{
		STScenesRequestPop* itm = (STScenesRequestPop*)lParam;
		AUScenesAdmin* admin = itm->admin;
		NBThreadMutex_lock(&admin->_reqsProg.mutex);
		SI32 i; for(i = 0 ; i < admin->_reqsProg.pops.use; i++){
			STScenesRequestPop* itm = NBArray_itmValueAtIndex(&admin->_reqsProg.pops, STScenesRequestPop*, i);
			if(lParam == itm){
				itmFnd = itm;
				itm->requesting = FALSE;
				if(!itm->requesting && !itm->onFocus && !isError){
					//Remove from array
					itmRelease = TRUE;
					NBArray_removeItemAtIndex(&admin->_reqsProg.pops, i);
					//PRINTF_INFO("AUScenesAdmin, request-pop removed after request completed, %d reques-pops remaining.\n", admin->_reqsProg.pops.use);
				}
				//PRINTF_INFO("AUScenesAdmin, request completed.\n");
				break;
			}
		} NBASSERT(itmFnd != NULL) //Should be found
		NBThreadMutex_unlock(&admin->_reqsProg.mutex);
	}
	//Notify and release
	NBASSERT(itmFnd != NULL)
	if(itmFnd != NULL){
		BOOL success = FALSE;
		//Notify
		if(itmFnd->lstnr != NULL){
			itmFnd->lstnr->reqPopEnded(itmFnd->reqId, itmFnd->lstnrParam, req);
		}
		//Change state
		if(itmRelease){
			//Release (already hidden)
			AUScenesAdmin::privRequestPopRelease(itmFnd);
			NBMemory_free(itmFnd);
			itmFnd = NULL;
		} else {
			//Update
			BOOL show = (isError && !itmFnd->onFocus);
			NBASSERT(itmFnd->popRef != NULL)
			if(req->response == NULL){
				const char* strLoc = TSClient_getStr(itmFnd->admin->_coreClt, "reqNoConn", "Check your internet connection and try again.");
				itmFnd->admin->popUpdate(itmFnd->popRef, ENScenePopMode_Error, ENNBTextLineAlignH_Center, NULL, strLoc);
				//PRINTF_INFO("AUScenesAdmin, Request with no response (no conn).\n");
			} else if(!req->response->isCompleted){
				const char* strLoc = TSClient_getStr(itmFnd->admin->_coreClt, "reqNoConn", "Check your internet connection and try again.");
				itmFnd->admin->popUpdate(itmFnd->popRef, ENScenePopMode_Error, ENNBTextLineAlignH_Center, NULL, strLoc);
				//PRINTF_INFO("AUScenesAdmin, Request not completed.\n");
			} else if(req->response->header.statusLine != NULL){
				const SI32 statusCode = req->response->header.statusLine->statusCode;
				const char* reason = &req->response->header.strs.str[req->response->header.statusLine->reasonPhrase];
				//PRINTF_INFO("AUScenesAdmin, request returned code: %d ('%s').\n", statusCode, reason);
				const char* strLoc = TSClient_getStr(itmFnd->admin->_coreClt, "reqErrTextPrefix", "Error: ");
				STNBString str;
				NBString_init(&str);
				NBString_concat(&str, strLoc);
				NBString_concat(&str, "'");
				NBString_concatSI32(&str, statusCode);
				NBString_concat(&str, "', '");
				NBString_concat(&str, reason);
				NBString_concat(&str, "'");
				if(statusCode >= 200 && statusCode < 300){
					const char* strLoc = TSClient_getStr(itmFnd->admin->_coreClt, "reqSuccessTextShort", "Success");
					itmFnd->admin->popUpdate(itmFnd->popRef, ENScenePopMode_Sucess, ENNBTextLineAlignH_Center, NULL, strLoc);
					success = TRUE;
				} else {
					itmFnd->admin->popUpdate(itmFnd->popRef, ENScenePopMode_Error, ENNBTextLineAlignH_Center, NULL, str.str);
				}
				NBString_release(&str);
				//Autohide
				if(success && itmFnd->onFocus && itmFnd->hideMode == ENReqPopHideMode_Auto){
					itmFnd->admin->popHide(itmFnd->popRef);
					show = FALSE;
				}
			}
			if(show){
				itmFnd->admin->popShow(itmFnd->popRef);
			}
		}
	}
}

void AUScenesAdmin::msgboxCloseSelected(AUIMsgBox* obj){
	//
}

void AUScenesAdmin::msgboxOptionSelected(AUIMsgBox* obj, const char* optionId){
	const STScenesPop* itmFnd = NULL;
	STScenesPop* itmNtf = NULL;
	{
		NBThreadMutex_lock(&_pops.mutex);
		if(_pops.array.use > 0){
			SI32 i; for(i = (_pops.array.use - 1) ; i >= 0; i--){
				STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
				NBASSERT(itm->retainCount >= 0)
				if(itm->msgBox == obj){
					itmFnd = itmNtf = itm;
					//Do not notify if no-one is retainig
					if(itm->retainCount <= 0){
						PRINTF_INFO("Pop option selected '%s', but ignoring, %d pops in list.\n", optionId, _pops.array.use);
						itmNtf = NULL;
					} else {
						//PRINTF_INFO("Pop option selected '%s', notifying, %d pops in list.\n", optionId, _pops.array.use);
						//Retain for notification
						itm->retainCount++;
					}
					break;
				}
			}
		}
		NBThreadMutex_unlock(&_pops.mutex);
	} NBASSERT(itmFnd != NULL)
	//Notify (unlocked)
	if(itmNtf != NULL){
		//Notify parent
		if(itmNtf->lstnr != NULL){
			itmNtf->lstnr->popOptionSelected(itmNtf->lstnrParam, itmNtf, optionId);
		}
		//Release from notification
		this->popRelease(itmNtf);
	}
}

void AUScenesAdmin::msgboxFocusObtained(AUIMsgBox* obj){
	STScenesPop* itmFnd = NULL;
	STScenesPop* itmNtf = NULL;
	{
		NBThreadMutex_lock(&_pops.mutex);
		SI32 i; for(i = (_pops.array.use - 1) ; i >= 0; i--){
			STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
			NBASSERT(itm->retainCount >= 0)
			if(itm->msgBox == obj){
				itmFnd = itmNtf = itm;
				itmFnd->onFocus = TRUE;
				//Do not notify if no-one is retainig
				if(itm->retainCount <= 0){
					itmNtf = NULL;
				} else {
					//Retain for notification
					itm->retainCount++;
				}
				break;
			}
		}
		NBThreadMutex_unlock(&_pops.mutex);
	} NBASSERT(itmNtf != NULL)
	//Notify (unlocked)
	if(itmNtf != NULL){
		//Notify parent
		if(itmNtf->lstnr != NULL){
			itmNtf->lstnr->popFocusObtained(itmNtf->lstnrParam, itmNtf);
		}
		//Release from notification
		this->popRelease(itmNtf);
	}
	//
	_pops.visibleCount++;
	//Apply new orientation state (popups blocks auto-rotation)
	this->privSyncOrientState();
}

void AUScenesAdmin::msgboxFocusLost(AUIMsgBox* obj){
	STScenesPop* itmFnd = NULL;
	STScenesPop* itmNtf = NULL;
	{
		NBThreadMutex_lock(&_pops.mutex);
		SI32 i; for(i = (_pops.array.use - 1) ; i >= 0; i--){
			STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
			if(itm->msgBox == obj){
				itmFnd = itmNtf = itm;
				itmFnd->onFocus = FALSE;
				//Do not notify if no-one is retainig
				if(itmFnd->retainCount <= 0){
					itmNtf = NULL;
					//Release
					AUScenesAdmin::privPopRelease(itm);
					NBMemory_free(itm);
					NBArray_removeItemAtIndex(&_pops.array, i);
					//PRINTF_INFO("Pop removed after hidding, %d pops remaining.\n", _pops.array.use);
				} else {
					//Retain for notification
					itm->retainCount++;
				}
				break;
			}
		}
		NBThreadMutex_unlock(&_pops.mutex);
	} NBASSERT(itmFnd != NULL)
	//Notify (unlocked)
	if(itmNtf != NULL){
		//Notify parent
		if(itmNtf->lstnr != NULL){
			itmNtf->lstnr->popFocusLost(itmNtf->lstnrParam, itmNtf);
		}
		//Release from notification
		this->popRelease(itmNtf);
	}
	//
	_pops.visibleCount--;
	if(_pops.visibleCount < 0){
		_pops.visibleCount = 0;
	}
	//Apply new orientation state (popups blocks auto-rotation)
	this->privSyncOrientState();
}

//

void AUScenesAdmin::popOptionSelected(void* param, void* ref, const char* optionId){
	/*if(ref == _errLog.popRef){
		ENDRErrorLogSendMode errLogMode = ENDRErrorLogSendMode_Ask;
		//Determine option
		if(NBString_strIsEqual(optionId, "never")){
			errLogMode = ENDRErrorLogSendMode_Never;
		} else if(NBString_strIsEqual(optionId, "send")){
			errLogMode = ENDRErrorLogSendMode_Ask;
		} else if(NBString_strIsEqual(optionId, "allways")){
			errLogMode = ENDRErrorLogSendMode_Allways;
		}
		//Save sendMode
		{
			STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(_coreClt, "client/_root/", "_current", TRUE, NULL, TSClientRoot_getSharedStructMap());
			if(ref.data != NULL){
				STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
				if(priv->errLogMode != errLogMode){
					priv->errLogMode = errLogMode;
					ref.privModified = TRUE;
				}
			}
			TSClient_returnStorageRecordFromWrite(_coreClt, &ref);
		}
		//Send
		if(errLogMode == ENDRErrorLogSendMode_Ask || errLogMode == ENDRErrorLogSendMode_Allways){
			this->privLogErrSendStart();
		} else {
			TSClient_logPendClear(_coreClt);
		}
	} else*/ if(ref == _upload.pend.popRef){
		//
	} else {
		STScenesRequestPop* itmNtf = NULL;
		{
			NBThreadMutex_lock(&_reqsProg.mutex);
			SI32 i; for(i = (_reqsProg.pops.use - 1) ; i >= 0; i--){
				STScenesRequestPop* itm = NBArray_itmValueAtIndex(&_reqsProg.pops, STScenesRequestPop*, i);
				if(itm == param){
					NBASSERT(itm->popRef == ref)
					itmNtf = itm;
					break;
				}
			}
			NBThreadMutex_unlock(&_reqsProg.mutex);
		} NBASSERT(itmNtf != NULL)
		//Notify (unlocked)
		if(itmNtf != NULL){
			//Notify parent
			if(itmNtf->lstnr != NULL){
				//Nothing
			}
		}
	}
}

void AUScenesAdmin::popFocusObtained(void* param, void* ref){
	/*if(ref == _errLog.popRef){
		//
	} else*/ if(ref == _upload.pend.popRef){
		//
	} else {
		STScenesRequestPop* itmNtf = NULL;
		{
			NBThreadMutex_lock(&_reqsProg.mutex);
			SI32 i; for(i = (_reqsProg.pops.use - 1) ; i >= 0; i--){
				STScenesRequestPop* itm = NBArray_itmValueAtIndex(&_reqsProg.pops, STScenesRequestPop*, i);
				if(itm == param){
					NBASSERT(itm->popRef == ref)
					itm->onFocus = TRUE;
					itmNtf = itm;
					break;
				}
			}
			NBThreadMutex_unlock(&_reqsProg.mutex);
		} NBASSERT(itmNtf != NULL)
		//Notify (unlocked)
		if(itmNtf != NULL){
			//Notify parent
			if(itmNtf->lstnr != NULL){
				//Nothing
			}
		}
	}
}

void AUScenesAdmin::popFocusLost(void* param, void* ref){
	/*if(ref == _errLog.popRef){
		this->popRelease(_errLog.popRef);
		_errLog.popRef = NULL;
	} else*/ if(ref == _upload.pend.popRef){
		this->popRelease(_upload.pend.popRef);
		_upload.pend.popRef = NULL;
	} else {
		STScenesRequestPop* itmNtf = NULL;
		BOOL itmRelease = FALSE;
		{
			NBThreadMutex_lock(&_reqsProg.mutex);
			SI32 i; for(i = (_reqsProg.pops.use - 1) ; i >= 0; i--){
				STScenesRequestPop* itm = NBArray_itmValueAtIndex(&_reqsProg.pops, STScenesRequestPop*, i);
				if(itm == param){
					NBASSERT(itm->popRef == ref)
					itmNtf = itm;
					itm->onFocus = FALSE;
					if(!itm->onFocus && !itm->requesting){
						NBArray_removeItemAtIndex(&_reqsProg.pops, i);
						//PRINTF_INFO("Request-pop removed after hidding, %d reques-pops remaining.\n", _reqsProg.pops.use);
						itmRelease = TRUE;
					}
					break;
				}
			}
			NBThreadMutex_unlock(&_reqsProg.mutex);
		} NBASSERT(itmNtf != NULL)
		//Notify (unlocked)
		if(itmNtf != NULL){
			//Notify parent
			/*if(itmNtf->lstnr != NULL){
			 itmNtf->lstnr->reqPopHidden(itmNtf->reqId, itmNtf->lstnrParam);
			 }*/
			//Release
			if(itmRelease){
				AUScenesAdmin::privRequestPopRelease(itmNtf);
				NBMemory_free(itmNtf);
				itmNtf = NULL;
			}
		}
	}
}

//

bool AUScenesAdmin::tickProcesoCarga(float segsTranscurridos){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::tickProcesoCarga")
	bool r = false;
	if(_loadTransCur != NULL){
		r = _loadTransCur->tickTransicion(segsTranscurridos);
		if(!_loadTransCur->ejecutandoTransicion()){
			_loadTransCur->liberar(NB_RETENEDOR_THIS);
			_loadTransCur = NULL;
		}
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return r;
}

//

void AUScenesAdmin::tickAnimacion(float segsTranscurridos){
	//Animate pop up icons
	{
		NBThreadMutex_lock(&_pops.mutex);
		SI32 i; for(i = (_pops.array.use - 1) ; i >= 0; i--){
			STScenesPop* itm = NBArray_itmValueAtIndex(&_pops.array, STScenesPop*, i);
			if(itm->mode == ENScenePopMode_Working){
				const SI32 angStep = (360 / 12);
				itm->iconRot -= (360.0f * segsTranscurridos);
				while(itm->iconRot < 0.0f){
					itm->iconRot += 360.0f;
				}
				itm->icon->establecerRotacionNormalizada(((SI32)itm->iconRot / angStep) * angStep);
			}
		}
		NBThreadMutex_unlock(&_pops.mutex);
	}
	//Notify client requests
	{
		TSClient_notifyAll(this->_coreClt);
	}
	//Notifications
	/*{
		//Detect authorization state
		{
			_notifs.auth.stateWaitSecs -= segsTranscurridos;
			if(_notifs.auth.stateWaitSecs <= 0.0f){
				const ENAppNotifAuthState curState = NBMngrNotifs::getAuthStatus(ENAppNotifAuthQueryMode_UpdateCache);
				if(curState == ENAppNotifAuthState_Authorized || curState == ENAppNotifAuthState_Denied){
					_notifs.auth.stateWaitSecs = TS_NOTIFS_AUTH_VERIFY_WAIT;
					if(_notifs.auth.state != curState){
						_notifs.auth.state = curState;
						PRINTF_INFO("Notifs auth detected (%s).\n", (curState == ENAppNotifAuthState_Authorized ? "authorized" : "denied"));
					}
				}
			}
		}
		//Detect remote notifications token change
		if(_notifs.auth.state == ENAppNotifAuthState_Authorized){
			_notifs.tokenCurWaitSecs -= segsTranscurridos;
			if(_notifs.tokenCurWaitSecs <= 0.0f){
				_notifs.tokenCurWaitSecs = TS_NOTIFS_AUTH_TOKEN_VERIFY_WAIT;
				{
					ENAppNotifTokenQueryMode mode = ENAppNotifTokenQueryMode_CacheAndRequestIfNecesary;
					STNBString token;
					NBString_init(&token);
					//Allways force new token
					if(!_notifs.firstTokenRequested){
						mode = ENAppNotifTokenQueryMode_ForceNewRequest;
						_notifs.firstTokenRequested = TRUE;
					}
					//Get token
					NBMngrNotifs::remoteGetToken(mode, &token);
					if(token.length > 0 && !NBString_isEqualBytes(&_notifs.tokenCur, token.str, token.length)){
						NBString_setBytes(&_notifs.tokenCur, token.str, token.length);
						//Force token send (if changed)
						if(!NBString_isEqualBytes(&_notifs.tokenCur, _notifs.tokenSent.str, _notifs.tokenSent.length)){
							PRINTF_INFO("Notifs token changed (%d bytes), will report to server: '%s'.\n", token.length, token.str);
							_notifs.tokenSendingWaitSecs = 0.0f;
						} else {
							PRINTF_INFO("Notifs token changed (%d bytes), already reported to server.\n", token.length);
						}
					}
					//
					NBString_release(&token);
				}
			}
		}
		//Send new token to server
		if(_notifs.tokenSendingReqId == 0){
			_notifs.tokenSendingWaitSecs -= segsTranscurridos;
			if(_notifs.tokenSendingWaitSecs <= 0){
				_notifs.tokenSendingWaitSecs = TS_NOTIFS_AUTH_TOKEN_SEND_WAIT;
				if(_notifs.tokenCur.length > 0){
					if(TSClient_isDeviceKeySet(_coreClt)){
						if(!NBString_isEqualBytes(&_notifs.tokenCur, _notifs.tokenSent.str, _notifs.tokenSent.length)){
							{
								STTSNotifToken token;
								NBMemory_setZeroSt(token, STTSNotifToken);
								token.type = ENTSNotifTokenType_Count;
#								if defined(__APPLE__) && TARGET_OS_IPHONE
								token.type = ENTSNotifTokenType_iOS;
#								elif defined(__ANDROID__)
								token.type = ENTSNotifTokenType_Android;
#								endif
								if(token.type == ENTSNotifTokenType_Count){
									PRINTF_CONSOLE_ERROR("AUScenesAdmin, could not determine the notifToken type.\n")
								} else {
									if(token.type == ENTSNotifTokenType_iOS){
										//Send in hex format
										STNBString tokenHex;
										NBString_init(&tokenHex);
										NBString_concatBytesHex(&tokenHex, _notifs.tokenCur.str, _notifs.tokenCur.length);
										NBString_strFreeAndNewBuffer(&token.token, tokenHex.str);
										NBString_release(&tokenHex);
									} else {
										//Send plain
										NBString_strFreeAndNewBufferBytes(&token.token, _notifs.tokenCur.str, _notifs.tokenCur.length);
									}
									NBString_strFreeAndNewBuffer(&token.lang, TSClient_getLocalePreferedLangAtIdx(_coreClt, 0, "en"));
									{
										STNBString str;
										NBString_init(&str);
										NBStruct_stConcatAsJson(&str, TSNotifToken_getSharedStructMap(), &token, sizeof(token));
										NBString_setBytes(&_notifs.tokenSending, _notifs.tokenCur.str, _notifs.tokenCur.length);
										_notifs.tokenSendingReqId = TSClient_addRequest(_coreClt, ENTSRequestId_Device_NotifToken_Set, NULL, str.str, str.length, 0, AUScenesAdmin::reqNotifTokenCallbackFunc, this);
										PRINTF_INFO("AUScenesAdmin, sending new token detected (%s).\n", token.token);
										NBString_release(&str);
									}
								}
								NBStruct_stRelease(TSNotifToken_getSharedStructMap(), &token, sizeof(token));
							}
						}
					}
				}
			}
		}
	}*/
	//Show request pop ups (after wait time)
	{
		STNBArray pops;
		NBArray_init(&pops, sizeof(void*), NULL);
		//Find popups to show (locked)
		{
			NBThreadMutex_lock(&_reqsProg.mutex);
			SI32 i; for(i = (_reqsProg.pops.use - 1) ; i >= 0; i--){
				STScenesRequestPop* itm = NBArray_itmValueAtIndex(&_reqsProg.pops, STScenesRequestPop*, i);
				const BOOL wasBelowWait = (itm->secsCur < itm->secsWaitDur ? TRUE : FALSE);
				itm->secsCur += segsTranscurridos;
				if(wasBelowWait && itm->secsCur >= itm->secsWaitDur){
					if(itm->requesting && !itm->onFocus){
						if(itm->popRef != NULL){
							this->popRetain(itm->popRef);
							NBArray_addValue(&pops, itm->popRef);
						}
					}
				}
			}
			NBThreadMutex_unlock(&_reqsProg.mutex);
		}
		//Show popups (unlocked)
		{
			SI32 i; for(i = 0; i < pops.use; i++){
				void* popRef = NBArray_itmValueAtIndex(&pops, void*, i);
				this->popShow(popRef);
				this->popRelease(popRef);
			}
		}
		NBArray_release(&pops);
	}
	//Consume rotated bitmap
	if(!_upload.pend.isActive){
		const STNBBitmapProps bmpProps = NBBitmap_getProps(&_upload.pend.rotBmp);
		if(bmpProps.size.width > 0 && bmpProps.size.height > 0){
			if(_upload.pend.popRef != NULL){
				this->popHide(_upload.pend.popRef);
				this->popRelease(_upload.pend.popRef);
				_upload.pend.popRef = NULL;
			}
			this->appOpenBitmap(NULL, &_upload.pend.rotBmp, 0, _upload.pend.usrData.str, _upload.pend.usrData.length);
			NBBitmap_empty(&_upload.pend.rotBmp);
		}
	}
	//Force orientation
	{
		this->privSyncOrientState();
	}
}

void AUScenesAdmin::privSyncOrientState(){
	UI32				mask = _orientation.mask;
	ENAppOrientationBit prefered = _orientation.prefered;
	ENAppOrientationBit toApplyOnce = _orientation.toApplyOnce;
	BOOL				allowAutoRotate = _orientation.allowAutoRotate;
	//Load values
	{
		//Aplly current scene prefs
		{
			if(_scenesStack.conteo > 0){
				AUAppEscena* escn = (AUAppEscena*)_scenesStack.elem(_scenesStack.conteo - 1);
				escn->escenaGetOrientations(&mask, &prefered, &toApplyOnce, &allowAutoRotate);
			}
		}
		//Force portrait mode
		{
			const SI32 enforcersCount = TSClient_getAppPortraitEnforcersCount(_coreClt);
			if(enforcersCount > 0){
				mask = ENAppOrientationBit_Portrait;
			}
		}
		//Force non-autorotate because of pop-up message
		if(_pops.visibleCount > 0){
			allowAutoRotate = FALSE;
		}
	}
	//Apply
	{
		if(_orientation.allowAutoRotate != allowAutoRotate){
			_orientation.allowAutoRotate = allowAutoRotate;
			NBMngrOSTools::setCanAutorotate(_orientation.allowAutoRotate);
		}
		if(_orientation.mask != (mask & ENAppOrientationBits_All) && (mask & ENAppOrientationBits_All) != 0){
			_orientation.mask = (mask & ENAppOrientationBits_All);
			NBMngrOSTools::setOrientationsMask(_orientation.mask);
		}
		if(_orientation.prefered != (mask & prefered) && (mask & prefered) != 0){
			_orientation.prefered = (ENAppOrientationBit)(mask & prefered);
			NBMngrOSTools::setOrientationPrefered(_orientation.prefered);
		}
		if(_orientation.toApplyOnce != (mask & toApplyOnce) && (mask & toApplyOnce) != 0){
			_orientation.toApplyOnce = (ENAppOrientationBit)(mask & toApplyOnce);
		}
	}
	//Apply the desired rotation once
	/*if((_orientation.toApplyOnce & ENAppOrientationBits_All) != 0){
		if(_orientation.allowAutoRotate && (_orientation.mask & _orientation.toApplyOnce) != 0){
			if((_orientation.toApplyOnce & ENAppOrientationBit_Portrait) != 0){
				NBMngrOSTools::setOrientation(ENAppOrientationBit_Portrait);
			} else if((_orientation.toApplyOnce & ENAppOrientationBit_LandscapeLeftBtn) != 0){
				NBMngrOSTools::setOrientation(ENAppOrientationBit_LandscapeLeftBtn);
			} else if((_orientation.toApplyOnce & ENAppOrientationBit_LandscapeRightBtn) != 0){
				NBMngrOSTools::setOrientation(ENAppOrientationBit_LandscapeRightBtn);
			} else if((_orientation.toApplyOnce & ENAppOrientationBit_PortraitInverted) != 0){
				NBMngrOSTools::setOrientation(ENAppOrientationBit_PortraitInverted);
			}
			_orientation.toApplyOnce = (ENAppOrientationBit)0;
		}
	}*/
}

//TECLADO
void AUScenesAdmin::tecladoVisualAltoModificado(const float altoPixeles){
	/*const NBCajaAABB cajaGUI = _lastBox;
	const NBRectangulo puertoVision = NBGestorEscena::puertoDeVision(_iScene);
	const float keybHeight = altoPixeles * ((cajaGUI.yMax - cajaGUI.yMin) / puertoVision.alto);
	this->privOrganizeAll(true);*/
}

//TOUCHES
void AUScenesAdmin::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	//
}

void AUScenesAdmin::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	
}

void AUScenesAdmin::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(objeto == _quickTip.layer){
		_animator->quitarAnimacionesDeObjeto(_quickTip.layer);
		_quickTip.layer->establecerVisible(false);
	}
}

//TECLAS
bool AUScenesAdmin::teclaPresionada(SI32 codigoTecla){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::teclaPresionada")
	bool consumida = false;
	if(_scenesStack.conteo > 0){
		AUAppEscena* escn = (AUAppEscena*)_scenesStack.elem(_scenesStack.conteo - 1);
		consumida = escn->teclaPresionada(codigoTecla);
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return consumida;
}

bool AUScenesAdmin::teclaLevantada(SI32 codigoTecla){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::teclaLevantada")
	bool consumida = false;
	if(_scenesStack.conteo > 0){
		AUAppEscena* escn = (AUAppEscena*)_scenesStack.elem(_scenesStack.conteo - 1);
		consumida = escn->teclaLevantada(codigoTecla);
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return consumida;
}

bool AUScenesAdmin::teclaEspecialPresionada(SI32 codigoTecla){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::teclaLevantada")
	bool consumida = false;
	if(_scenesStack.conteo > 0){
		AUAppEscena* escn = (AUAppEscena*)_scenesStack.elem(_scenesStack.conteo - 1);
		consumida = escn->teclaEspecialPresionada(codigoTecla);
	}
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return consumida;
}

// EVENTOS

bool AUScenesAdmin::permitidoGirarPantalla(){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::permitidoGirarPantalla")
	bool permitido = true;
	/*if(_scenesStack.conteo > 0){
	 AUAppEscena* escn = (AUAppEscena*)_scenesStack.elem(_scenesStack.conteo - 1);
	 permitido = escn->escenaPermitidoGirarPantalla(codigoTecla);
	 }*/
	AU_GESTOR_PILA_LLAMADAS_POP_3
	return permitido;
}

#ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
void AUScenesAdmin::sndGrupoCambioHabilitado(const ENAudioGrupo grupoAudio, const bool nuevoHabilitado){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::sndGrupoCambioHabilitado")
	PRINTF_INFO("sndGrupoCambioHabilitado(%d, %d).\n", (SI32)grupoAudio, (SI32)nuevoHabilitado);
	AU_GESTOR_PILA_LLAMADAS_POP_3
}
#endif

void AUScenesAdmin::anguloXYDispositivoCambiado(float angulo){
	AU_GESTOR_PILA_LLAMADAS_PUSH_3("AUScenesAdmin::anguloXYDispositivoCambiado")
	//if(_gameplayActual != NULL) _gameplayActual->anguloXYDispositivoCambiado(angulo);
	//_gamePortada
	AU_GESTOR_PILA_LLAMADAS_POP_3
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(AUScenesAdmin, AUAppEscenasAdmin)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(AUScenesAdmin, "AUScenesAdmin", AUAppEscenasAdmin)
AUOBJMETODOS_CLONAR_NULL(AUScenesAdmin)
