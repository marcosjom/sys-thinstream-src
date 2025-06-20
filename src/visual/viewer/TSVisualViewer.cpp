//
//  TSVisualViewer.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/viewer/TSVisualViewer.h"
//
#include "visual/TSColors.h"
#include "core/logic/TSDeviceLocal.h"
#include "visual/AUSceneBarTop.h"
#include "visual/TSFonts.h"

TSVisualViewer::TSVisualViewer(const STLobbyColRoot* root, ITSVisualViewerListener* lstnr, STTSClientRequesterDeviceRef src, const char* srcRunId, const float width, const float height, const NBMargenes margins, const NBMargenes marginsRows, AUAnimadorObjetoEscena* animObjetos) : AUEscenaContenedorLimitado() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualViewer")
	_root			= *root;
	_lstnr			= lstnr;
	//
	NBMemory_setZero(_refDatos);
	NBCallback_init(&_prntResizedCallbck);
	//Child resized callback
	{
		STNBCallbackMethod m;
		NBMemory_setZeroSt(m, STNBCallbackMethod);
		m.obj			= this;
		m.retain		= AUObjeto_retain_;
		m.release		= AUObjeto_release_;
		m.callback		= TSVisualViewer::childResized;
		NBCallback_initWithMethods(&_chldResizedCallbck, &m);
		_chldResizedCount = 0;
	}
	_touchInheritor	= NULL;
	_iconWidthRef	= 0.0f;
	_anchoUsar		= 0.0f;
	_altoUsar		= 0.0f;
	_yTopLast		= 0.0f;
	_margins		= margins;
	_marginsRows	= marginsRows;
	NBMemory_setZero(_lastFilterInner);
	NBMemory_setZero(_lastFilterOuter);
	//
	if(animObjetos != NULL){
		_animObjetos	= animObjetos;
		_animObjetos->retener(NB_RETENEDOR_THIS);
	} else {
		_animObjetos	= new(this) AUAnimadorObjetoEscena();
	}
	//Fonts
	{
		_fntNames 	= TSFonts::font(ENTSFont_ContentMid);
		_fntExtras	= TSFonts::font(ENTSFont_ContentTitle);
		_fntNames->retener(NB_RETENEDOR_THIS);
		_fntExtras->retener(NB_RETENEDOR_THIS);
	}
	//data
	{
		NBMemory_setZero(_data);
		TSClientRequesterDevice_set(&_data.src, &src);
		NBString_init(&_data.srcRunId);
		if(!NBString_strIsEmpty(srcRunId)){
			NBString_set(&_data.srcRunId, srcRunId);
		}
	}
	//No authtorized message
	{
		const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
		//
		_add.layer	= new(this) AUEscenaContenedor();
		_add.layer->establecerEscuchadorTouches(this, this);
		this->agregarObjetoEscena(_add.layer);
		//
		{
			_add.ico = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#add"));
			_add.ico->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			_add.ico->establecerEscalacion(1.5f);
			_add.layer->agregarObjetoEscena(_add.ico);
		}
		//
		{
			_add.text = new(this) AUEscenaTexto(_fntNames);
			_add.text->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_Center);
			{
				const char* strLoc = TSClient_getStr(_root.coreClt, "invitationStartText", "Tap here to add an account.");
				_add.text->establecerTexto(strLoc);
			}
			_add.text->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			_add.layer->agregarObjetoEscena(_add.text);
		}
	}
	//Grps
	{
		NBArray_init(&_grps.array, sizeof(STVisualViewerGrp), NULL);
		_grps.layer	= new(this) AUEscenaContenedor();
		_grps.layer->setVisibleAndAwake(TRUE);
		_grps.layer->establecerEscuchadorTouches(this, this);
		this->agregarObjetoEscena(_grps.layer);
	}
	//Rows
	{
		NBArray_init(&_rows.array, sizeof(STVisualViewerRow), NULL);
	}
	//Buttons
	{
		const STNBColor8 txtColor = TSColors::colorDef(ENTSColor_Gray)->normal;
		const STNBColor8 icoColor = TSColors::colorDef(ENTSColor_Gray)->normal;
		//AUTextura* icoGT		= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons-gt.png");
		AUFuenteRender* fnt		= TSFonts::font(ENTSFont_ContentMid);
		const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_Light)->normal;
		NBMargenes margins; margins.left = margins.right = margins.top = margins.bottom = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.10f);
		const float marginI		= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.10f);
		NBColor8 colorN; NBCOLOR_ESTABLECER(colorN, colorTT.r, colorTT.g, colorTT.b, colorTT.a)
		NBColor8 colorH; NBCOLOR_ESTABLECER(colorH, (SI32)colorTT.r * 9 / 10, (SI32)colorTT.g * 9 / 10, (SI32)colorTT.b * 9 / 10, colorTT.a)
		NBColor8 colorT; NBCOLOR_ESTABLECER(colorT, (SI32)colorTT.r * 9 / 10, (SI32)colorTT.g * 9 / 10, (SI32)colorTT.b * 9 / 10, colorTT.a)
		NBColor8 colorTxtN; NBCOLOR_ESTABLECER(colorTxtN, txtColor.r, txtColor.g, txtColor.b, txtColor.a)
		NBColor8 colorIcoN; NBCOLOR_ESTABLECER(colorIcoN, icoColor.r, icoColor.g, icoColor.b, icoColor.a)
		NBColor8 colorIcoH; NBCOLOR_ESTABLECER(colorIcoH, icoColor.r, icoColor.g, icoColor.b, icoColor.a)
		NBColor8 colorIcoT; NBCOLOR_ESTABLECER(colorIcoT, icoColor.r, icoColor.g, icoColor.b, icoColor.a)
		//const float widthForInteral = width - _marginsRows.left /*- marginI - texIcoGT->tamanoHD().ancho*/ - _marginsRows.right;
		//
		NBMemory_setZero(_buttons);
		//Info
		{
			//Conds and terms
			{
				//AUTextura* icoTex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#info");
				const char* strLoc	= TSClient_getStr(_root.coreClt, "guiMenuTermsBtn", "Terms and conditions");
				AUIButton* btnRef = new(this) AUIButton();
				btnRef->establecerMargenes(margins, marginI);
				btnRef->establecerFondo(true, NULL);
				btnRef->establecerFondoColores(colorN, colorH, colorT);
				btnRef->establecerSegsRetrasarOnTouch(0.20f);
				//btnRef->agregarIcono(icoTex, ENIBotonItemAlign_Left, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
				btnRef->agregarTextoMultilinea(fnt, strLoc, ENIBotonItemAlign_Center, 0.5f, 0, 0, colorTxtN, colorIcoH, colorIcoT);
				//btnRef->agregarIcono(icoGT, ENIBotonItemAlign_Right, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
				btnRef->establecerEscuchadorBoton(this);
				btnRef->organizarContenido(false);
				_buttons.info.termsAndConds = btnRef;
			}
			//Privacy policy
			{
				//AUTextura* icoTex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#info");
				const char* strLoc	= TSClient_getStr(_root.coreClt, "privacyTitle", "Privacy policy");
				AUIButton* btnRef = new(this) AUIButton();
				btnRef->establecerMargenes(margins, marginI);
				btnRef->establecerFondo(true, NULL);
				btnRef->establecerFondoColores(colorN, colorH, colorT);
				btnRef->establecerSegsRetrasarOnTouch(0.20f);
				//btnRef->agregarIcono(icoTex, ENIBotonItemAlign_Left, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
				btnRef->agregarTextoMultilinea(fnt, strLoc, ENIBotonItemAlign_Center, 0.5f, 0, 0, colorTxtN, colorIcoH, colorIcoT);
				//btnRef->agregarIcono(icoGT, ENIBotonItemAlign_Right, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
				btnRef->establecerEscuchadorBoton(this);
				btnRef->organizarContenido(false);
				_buttons.info.privacy = btnRef;
			}
			//About
			{
				//AUTextura* icoTex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#info");
				const char* strLoc	= TSClient_getStr(_root.coreClt, "AboutTitle", "About");
				AUIButton* btnRef = new(this) AUIButton();
				btnRef->establecerMargenes(margins, marginI);
				btnRef->establecerFondo(true, NULL);
				btnRef->establecerFondoColores(colorN, colorH, colorT);
				btnRef->establecerSegsRetrasarOnTouch(0.20f);
				//btnRef->agregarIcono(icoTex, ENIBotonItemAlign_Left, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
				btnRef->agregarTextoMultilinea(fnt, strLoc, ENIBotonItemAlign_Center, 0.5f, 0, 0, colorTxtN, colorIcoH, colorIcoT);
				//btnRef->agregarIcono(icoGT, ENIBotonItemAlign_Right, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
				btnRef->establecerEscuchadorBoton(this);
				btnRef->organizarContenido(false);
				_buttons.info.about = btnRef;
			}
		}
	}
	//Add listener
	{
		STNBStorageCacheItmLstnr mthds;
		NBMemory_setZeroSt(mthds, STNBStorageCacheItmLstnr);
		mthds.retain		= AUObjeto_retain_;
		mthds.release		= AUObjeto_release_;
		mthds.recordChanged	= TSVisualViewer::recordChanged;
		TSClient_addRecordListener(_root.coreClt, "client/_root/", "_current", this, &mthds);
		//Add lstnr
		{
			STTSClientRequesterRef reqRef = TSClient_getStreamsRequester(_root.coreClt);
			if(TSClientRequester_isSet(reqRef)){
				STTSClientRequesterLstnr lstnr;
				NBMemory_setZeroSt(lstnr, STTSClientRequesterLstnr);
				lstnr.param = this;
				lstnr.itf.requesterSourcesUpdated = TSVisualViewer::requesterSourcesUpdated;
				TSClientRequester_addLstnr(reqRef, &lstnr);
			}
			if(TSClientRequesterDevice_isSet(_data.src)){
				STTSClientRequesterDeviceLstnr lstnr;
				NBMemory_setZeroSt(lstnr, STTSClientRequesterDeviceLstnr);
				lstnr.param = this;
				lstnr.itf.requesterSourceConnStateChanged = requesterSourceConnStateChanged;
				lstnr.itf.requesterSourceDescChanged = requesterSourceDescChanged;
				TSClientRequesterDevice_addLstnr(_data.src, &lstnr);
			}
		}
		//First sync
		_sync.isDirty = FALSE;
		this->privSyncStreams(width);
	}
	//
	this->privOrganizarContenido(width, height, false, NULL, 0.0f);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

TSVisualViewer::~TSVisualViewer(){
	NBGestorAnimadores::quitarAnimador(this);
	NBCallback_release(&_prntResizedCallbck);
	NBCallback_release(&_chldResizedCallbck);
	//Remove lstnr
	{
		TSClient_removeRecordListener(_root.coreClt, "client/_root/", "_current", this);
		//Requester
		{
			STTSClientRequesterRef reqRef = TSClient_getStreamsRequester(_root.coreClt);
			if(TSClientRequester_isSet(reqRef)){
				TSClientRequester_removeLstnr(reqRef, this);
			}
		}
		//Source
		if(TSClientRequesterDevice_isSet(_data.src)){
			TSClientRequesterDevice_removeLstnr(_data.src, this);
		}
	}
	//Data
	{
		//Source
		if(TSClientRequesterDevice_isSet(_data.src)){
			TSClientRequesterDevice_release(&_data.src);
			TSClientRequesterDevice_null(&_data.src);
		}
		NBString_release(&_data.srcRunId);
	}
	//
	if(_animObjetos != NULL) _animObjetos->liberar(NB_RETENEDOR_THIS); _animObjetos = NULL;
	//Fonts
	{
		if(_fntNames != NULL) _fntNames->liberar(NB_RETENEDOR_THIS); _fntNames = NULL;
		if(_fntExtras != NULL) _fntExtras->liberar(NB_RETENEDOR_THIS); _fntExtras = NULL;
	}
	//Auth
	{
		if(_add.layer != NULL) _add.layer->liberar(NB_RETENEDOR_THIS); _add.layer = NULL;
		if(_add.ico != NULL) _add.ico->liberar(NB_RETENEDOR_THIS); _add.ico = NULL;
		if(_add.text != NULL) _add.text->liberar(NB_RETENEDOR_THIS); _add.text = NULL;
	}
	//Rows
	{
		{
			SI32 i; for(i = 0; i < _rows.array.use; i++){
				STVisualViewerRow* itm = NBArray_itmPtrAtIndex(&_rows.array, STVisualViewerRow, i);
				//uid
				{
					if(itm->uid.runId != NULL) NBMemory_free(itm->uid.runId); itm->uid.runId = NULL;
					if(itm->uid.streamId != NULL) NBMemory_free(itm->uid.streamId); itm->uid.streamId = NULL;
				}
				if(itm->btn != NULL) itm->btn->liberar(NB_RETENEDOR_THIS); itm->btn = NULL;
				if(itm->content != NULL) itm->content->liberar(NB_RETENEDOR_THIS); itm->content = NULL;
			}
			NBArray_empty(&_rows.array);
			NBArray_release(&_rows.array);
		}
	}
	//Grps
	{
		if(_grps.layer != NULL) _grps.layer->liberar(NB_RETENEDOR_THIS); _grps.layer = NULL;
		{
			SI32 i; for(i = 0; i < _grps.array.use; i++){
				STVisualViewerGrp* itm = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, i);
				itm->obj->liberar(NB_RETENEDOR_THIS);
				itm->obj = NULL;
			}
			NBArray_empty(&_grps.array);
			NBArray_release(&_grps.array);
		}
	}
	//Buttons
	{
		//Info
		{
			if(_buttons.info.termsAndConds != NULL) _buttons.info.termsAndConds->liberar(NB_RETENEDOR_THIS); _buttons.info.termsAndConds = NULL;
			if(_buttons.info.privacy != NULL) _buttons.info.privacy->liberar(NB_RETENEDOR_THIS); _buttons.info.privacy = NULL;
			if(_buttons.info.about != NULL) _buttons.info.about->liberar(NB_RETENEDOR_THIS); _buttons.info.about = NULL;
		}
	}
}

//Child resized

void TSVisualViewer::childResized(void* param){
	TSVisualViewer* obj = (TSVisualViewer*)param;
	obj->_chldResizedCount++;
}

//AUEscenaListaItemI

AUEscenaObjeto* TSVisualViewer::itemObjetoEscena(){
	return this;
}

STListaItemDatos TSVisualViewer::refDatos(){
	return _refDatos;
}

void TSVisualViewer::establecerRefDatos(const SI32 tipo, const SI32 valor){
	_refDatos.tipo	= tipo;
	_refDatos.valor	= valor;
}

void TSVisualViewer::organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(ancho, altoVisible, notificarCambioAltura, animator, secsAnim);
}

//AUSceneContentI

void TSVisualViewer::setResizedCallback(STNBCallback* parentCallback){
	//Release
	NBCallback_release(&_prntResizedCallbck);
	//Set new ref
	if(parentCallback == NULL){
		NBCallback_init(&_prntResizedCallbck);
	} else {
		NBCallback_initAsRefOf(&_prntResizedCallbck, parentCallback);
	}
}

void TSVisualViewer::setTouchInheritor(AUEscenaObjeto* touchInheritor){
	_touchInheritor = touchInheritor;
}

BOOL TSVisualViewer::getStatusBarDef(STTSSceneBarStatus* dst){
	BOOL r = FALSE;
	if(dst != NULL){
		const char* strLoc = TSClient_getStr(_root.coreClt, "lobbyDashbTitle", "Home");
		NBString_strFreeAndNewBuffer(&dst->title, strLoc);
		//
		dst->rightIconMask |= ENSceneBarIconBit_Add;
		//
		r = TRUE;
	}
	return r;
}

BOOL TSVisualViewer::execBarBackCmd(){
	return FALSE;
}

void TSVisualViewer::execBarIconCmd(const SI32 iIcon){
	switch(iIcon){
		case ENSceneBarIcon_Add:
			_root.scenes->loadInfoSequence(ENSceneAction_Push, ENSceneInfoMode_Register, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
			break;
		default:
			break;
	}
}

void TSVisualViewer::applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz){
	if(wordsLwrSz <= 0){
		//Set all visible
		SI32 i; for(i = 0; i < _rows.array.use; i++){
			STVisualViewerRow* itm = NBArray_itmPtrAtIndex(&_rows.array, STVisualViewerRow, i);
			itm->btn->setVisibleAndAwake(TRUE);
		}
	} else {
		//Set only match visibles
		SI32 i; for(i = 0; i < _rows.array.use; i++){
			STVisualViewerRow* itm = NBArray_itmPtrAtIndex(&_rows.array, STVisualViewerRow, i);
			const BOOL isMatch = itm->content->isSearchMatch(wordsLwr, wordsLwrSz);
			itm->btn->setVisibleAndAwake(isMatch);
		}
	}
	this->privOrganizarContenido(_anchoUsar, _altoUsar, true, NULL, 0.0f);
}

void TSVisualViewer::applyAreaFilter(const STNBAABox outter, const STNBAABox inner){
	//Grps
	if(outter.xMin < outter.xMax && outter.yMin < outter.yMax && inner.xMin < inner.xMax && inner.yMin < inner.yMax){
		SI32 i; for(i = 0; i < _grps.array.use; i++){
			STVisualViewerGrp* itm = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, i);
			const NBPunto pos		= itm->obj->traslacion();
			const NBCajaAABB box	= itm->obj->cajaAABBLocal();
			STNBAABox bbox;
			bbox.xMin = box.xMin + pos.x;
			bbox.xMax = box.xMax + pos.x;
			bbox.yMin = box.yMin + pos.y;
			bbox.yMax = box.yMax + pos.y;
			if(bbox.yMin >= outter.yMax || bbox.yMax <= outter.yMin){
				itm->inViewPort = FALSE;
			} else {
				itm->inViewPort = TRUE;
				if(itm->inViewPort){
					STNBAABox inner2;
					STNBAABox outter2;
					inner2.xMin = inner.xMin - pos.x;
					inner2.xMax = inner.xMax - pos.x;
					inner2.yMin = inner.yMin - pos.y;
					inner2.yMax = inner.yMax - pos.y;
					outter2.xMin = outter.xMin - pos.x;
					outter2.xMax = outter.xMax - pos.x;
					outter2.yMin = outter.yMin - pos.y;
					outter2.yMax = outter.yMax - pos.y;
					itm->obj->applyAreaFilter(outter2, inner2);
				}
			}
			itm->obj->setVisibleAndAwake(itm->inViewPort && (itm->obj->getRowsCount() > 0));
		}
		_lastFilterInner = inner;
		_lastFilterOuter = outter;
	}
}

BOOL TSVisualViewer::applyScaledSize(const STNBSize sizeBase, const STNBSize sizeScaled, const bool notifyChange, AUAnimadorObjetoEscena* animator, const float secsAnim){
	return FALSE;
}

STNBSize TSVisualViewer::getScaledSize(){
	return NBST_P(STNBSize, _anchoUsar, _altoUsar );
}

ENDRScrollSideH TSVisualViewer::getPreferedScrollH(){
	return ENDRScrollSideH_Left;
}

ENDRScrollSideV TSVisualViewer::getPreferedScrollV(){
	return ENDRScrollSideV_Top;
}

void* TSVisualViewer::anchorCreate(const STNBPoint lclPos){
	return NULL;
}

void* TSVisualViewer::anchorToFocusClone(float* dstSecsDur){
	return NULL;
}

void* TSVisualViewer::anchorClone(const void* anchorRef){
	return NULL;
}

void TSVisualViewer::anchorToFocusClear(){
	//
}

STNBAABox TSVisualViewer::anchorGetBox(void* anchorRef, float* dstScalePref){
	STNBAABox r; NBMemory_setZeroSt(r, STNBAABox);
	NBASSERT(anchorRef == NULL)
	return r;
}

void TSVisualViewer::anchorDestroy(void* anchorRef){
	NBASSERT(anchorRef == NULL)
}

void TSVisualViewer::scrollToTop(const float secs){
	//
}

//

void TSVisualViewer::privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	_chldResizedCount = 0;
	if(ancho > 0.0f){
		const float yTopStart = -_margins.top;
		float xLeft = 0.0f /*_margins.left*/, xRight = ancho /*- _margins.right*/, yTop = yTopStart;
		//Rows
		{
			const float grpMarginBetweenGrps = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.00f);
			//Determine center of thumbnails
			/*STNBPoint centerThumbs = NBST_P(STNBPoint, 0, 0 );
			if(_rows.array.use > 0){
				...* row = NBArray_itmPtrAtIndex(&_rows.array, ..., 0);
				if(row->content != NULL){
					const NBMargenes btnMargins = row->btn->margenes();
					//centerThumbs = row->content->getCenterThumbnail();
					centerThumbs.x += btnMargins.left;
					centerThumbs.y += btnMargins.top;
				}
			}*/
			//Organize groups
			{
				SI32 i; for(i = 0; i < _grps.array.use; i++){
					STVisualViewerGrp* grp = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, i);
					//Configure grp-header
					{
						grp->obj->setTitlePos(TRUE, _marginsRows.left + _iconWidthRef + _marginsRows.left, ENNBTextLineAlignH_Left);
						//grp->obj->setTitlePos(TRUE, centerThumbs.x, ENNBTextLineAlignH_Center);
					}
					//Organize rows
					{
						grp->obj->organizarContenido((xRight - xLeft), 0.0f, false, _animObjetos, 0.0f);
						grp->obj->setVisibleAndAwake(grp->inViewPort && (grp->obj->getRowsCount() > 0));
					}
					//Traslate
					{
						const NBCajaAABB box = grp->obj->cajaAABBLocal();
						if(yTop != yTopStart) yTop -= grpMarginBetweenGrps;
						{
							const STNBPoint pos = NBST_P(STNBPoint, xLeft - box.xMin, yTop - box.yMax );
							const NBPunto curPos = grp->obj->traslacion();
							if(curPos.x != pos.x || curPos.y != pos.y){
								if(animator != NULL && secsAnim > 0.0f){
									animator->animarPosicion(grp->obj, pos.x, pos.y, secsAnim, ENAnimPropVelocidad_Desacelerada);
								} else {
									grp->obj->establecerTraslacion(pos.x, pos.y);
								}
							}
						}
						yTop -= (box.yMax - box.yMin);
					}
				}
			}
			_grps.layer->setVisibleAndAwake(_rows.array.use > 0);
		}
		//Auth
		if(_grps.array.use > 0){
			_add.layer->setVisibleAndAwake(FALSE);
		} else {
			const float grpMarginV = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.20f);
			float totalHeight = 0.0f;
			//Calculate the height
			{
				if(yTop != yTopStart) totalHeight += grpMarginV;
				//Text
				{
					_add.text->organizarTexto(ancho * 0.5f);
					const NBCajaAABB box = _add.text->cajaAABBLocal();
					if(totalHeight != 0.0f) totalHeight += grpMarginV;
					totalHeight += (box.yMax - box.yMin);
				}
				//Icon
				{
					const NBCajaAABB box = _add.ico->cajaAABBLocal();
					if(totalHeight != 0.0f) totalHeight += grpMarginV;
					totalHeight += (box.yMax - box.yMin);
				}
			}
			//Organize
			{
				BOOL setAtBottom = FALSE;
				if((-yTop + totalHeight) < alto){
					//Will fit in remainig space, align to center
					yTop -= (((yTop + alto) - totalHeight) * 0.5f);
					setAtBottom = TRUE;
				} else {
					//Wint fit, align below
				}
				//Organize
				{
					//Icon
					{
						const NBCajaAABB box = _add.ico->cajaAABBLocal();
						const float scale = _add.ico->escalacion().ancho;
						if(yTop != yTopStart) yTop -= grpMarginV;
						_add.ico->establecerTraslacion(0.0f - (box.xMin * scale) + ((ancho - ((box.xMax - box.xMin) * scale)) * 0.5f), yTop - (box.yMax * scale));
						yTop -= ((box.yMax - box.yMin) * scale);
					}
					//Text
					{
						const NBCajaAABB box = _add.text->cajaAABBLocal();
						if(yTop != yTopStart) yTop -= grpMarginV;
						_add.text->establecerTraslacion(0.0f - box.xMin + ((ancho - (box.xMax - box.xMin)) * 0.5f), yTop - box.yMax);
						yTop -= (box.yMax - box.yMin);
					}
					//Margin
					{
						yTop -= (grpMarginV * 2.0f);
					}
				}
				if(setAtBottom){
					yTop = -alto;
				}
			}
			_add.layer->setVisibleAndAwake(TRUE);
		}
		//Set limits
		{
			this->establecerLimites(0.0f, ancho, yTop, 0.0f);
		}
		_anchoUsar	= ancho;
		_altoUsar	= alto;
		//Apply area filter
		this->applyAreaFilter(_lastFilterOuter, _lastFilterInner);
		//PRINTF_INFO("_yTopLast(%f) yTop(%f).\n", _yTopLast, yTop);
		if(_yTopLast != yTop){
			_yTopLast = yTop;
			if(notificarCambioAltura){
				//PRINTF_INFO("Notifying callback.\n");
				NBCallback_call(&_prntResizedCallbck);
			}
		}
	}
}

//

void TSVisualViewer::recordChanged(void* param, const char* relRoot, const char* relPath){
	TSVisualViewer* obj = (TSVisualViewer*)param;
	if(obj != NULL){
		obj->_sync.isDirty = TRUE;
	}
}

void TSVisualViewer::requesterSourcesUpdated(void* param, STTSClientRequesterRef reqRef){
	TSVisualViewer* obj = (TSVisualViewer*)param;
	if(obj != NULL){
		obj->_sync.isDirty = TRUE;
	}
}

void TSVisualViewer::requesterSourceConnStateChanged(void* param, const STTSClientRequesterDeviceRef source, const ENTSClientRequesterDeviceConnState state){
	TSVisualViewer* obj = (TSVisualViewer*)param;
	if(obj != NULL){
		obj->_sync.isDirty = TRUE;
	}
}

void TSVisualViewer::requesterSourceDescChanged(void* param, const STTSClientRequesterDeviceRef source){
	TSVisualViewer* obj = (TSVisualViewer*)param;
	if(obj != NULL){
		obj->_sync.isDirty = TRUE;
	}
}

void TSVisualViewer::privSyncAddGrpAndBtn(const char* grpName, AUIButton* btn, STNBArray* pendGrps, STNBArray* pendRows){
	if(grpName != NULL && btn != NULL && pendGrps != NULL && pendRows != NULL){
		NBASSERT(pendGrps != &_grps.array)
		NBASSERT(pendRows != &_rows.array)
		this->privSyncAddGrpToEnd(grpName, pendGrps);
		this->privSyncAddBtn(btn, pendRows);
	}
}

void TSVisualViewer::privSyncAddGrpToEnd(const char* grpName, STNBArray* pendGrps){
	BOOL grpChanged = TRUE;
	//Determine change of grp
	if(_grps.array.use > 0){
		STVisualViewerGrp* lastGrp = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, _grps.array.use - 1);
		if(NBString_strIsEqual(lastGrp->obj->getGrpName(), grpName)){
			grpChanged = FALSE;
		} else {
			grpChanged = TRUE;
		}
	}
	if(grpChanged){
		BOOL reused = FALSE;
		//Determine if exists
		{
			NBASSERT(pendGrps != &_grps.array)
			SI32 i; for(i = 0; i < pendGrps->use; i++){
				STVisualViewerGrp* itm = NBArray_itmPtrAtIndex(pendGrps, STVisualViewerGrp, i);
				//PRINTF_INFO("Comparing grp '%s' vs '%s'.\n", grp->grpName.str, lowUtf8.str);
				if(NBString_strIsEqual(itm->obj->getGrpName(), grpName)){
					//Reuse grp
					NBArray_addValue(&_grps.array, *itm);
					//PRINTF_INFO("Grp reused(#%d).\n", _grps.array.use);
					NBArray_removeItemAtIndex(pendGrps, i);
					reused = TRUE;
					break;
				}
			}
		}
		//Create grp
		if(!reused){
			const UI32 bitsMask = 0;
			STVisualViewerGrp itm;
			itm.inViewPort	= TRUE;
			itm.obj			= new(this) TSVisualRowsGrp(_root.iScene, _anchoUsar, _altoUsar, _animObjetos, _fntNames, _fntExtras, grpName, bitsMask);
			itm.obj->setGrpName(grpName);
			itm.obj->setTitleVisible(FALSE);
			itm.obj->setLinesVisible(FALSE);
			_grps.layer->agregarObjetoEscena(itm.obj);
			NBArray_addValue(&_grps.array, itm);
			//PRINTF_INFO("Meeting grp added(#%d).\n", _grps.array.use);
		}
	}
}

void TSVisualViewer::privSyncAddBtn(AUIButton* btn, STNBArray* pendRows){
	if(btn != NULL && pendRows != NULL){
		NBASSERT(_grps.array.use > 0)
		if(_grps.array.use > 0){
			STVisualViewerGrp* grp = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, _grps.array.use - 1);
			//Add row
			BOOL reused = FALSE;
			//Determine if exists
			{
				NBASSERT(pendRows != &_rows.array)
				SI32 i; for(i = 0; i < pendRows->use; i++){
					STVisualViewerRow* row = NBArray_itmPtrAtIndex(pendRows, STVisualViewerRow, i);
					//PRINTF_INFO("Comparing row '%s' vs '%s'.\n", row->content->getUID(), c->uid);
					if(row->btn == btn){
						//Move to grp layer
						{
							grp->obj->syncAdd(row->btn, row->btn);
						}
						//Update value
						{
							//row->content->updateIfNecesary(c);
						}
						//Reuse row
						NBArray_addValue(&_rows.array, *row);
						//PRINTF_INFO("Meeting row reused(#%d).\n", _rows.array.use);
						NBArray_removeItemAtIndex(pendRows, i);
						reused = TRUE;
						break;
					}
				}
			}
			//Create meeting
			if(!reused){
				STVisualViewerRow row;
				NBMemory_setZeroSt(row, STVisualViewerRow);
				//Create button
				{
					grp->obj->syncAdd(btn, btn);
					row.btn = btn; if(btn != NULL) btn->retener(NB_RETENEDOR_THIS);
				}
				NBArray_addValue(&_rows.array, row);
				//PRINTF_INFO("Meeting row added(#%d).\n", _rows.array.use);
			}
		}
	}
}

typedef struct STTSVisualViewerStreamIdx_ {
	UI32 uid;
	TSVisualViewerStream* row;
} STTSVisualViewerStreamIdx;

BOOL NBCompare_TSVisualViewerStreamIdx(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	const STTSVisualViewerStreamIdx* d1 = (STTSVisualViewerStreamIdx*)data1;
	const STTSVisualViewerStreamIdx* d2 = (STTSVisualViewerStreamIdx*)data2;
	NBASSERT(dataSz == sizeof(*d1))
	if(dataSz == sizeof(*d1)){
		switch (mode) {
			case ENCompareMode_Equal:
				return d1->uid == d2->uid;
			case ENCompareMode_Lower:
				return d1->uid < d2->uid;
			case ENCompareMode_LowerOrEqual:
				return d1->uid <= d2->uid;
			case ENCompareMode_Greater:
				return d1->uid > d2->uid;
			case ENCompareMode_GreaterOrEqual:
				return d1->uid >= d2->uid;
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	NBASSERT(FALSE)
	return FALSE;
}


void TSVisualViewer::privSyncAddStreamOnce(const float width, STNBArray* pendRows, const STTSStreamsServiceDesc* service, const STTSStreamDesc* strmDesc){
	NBASSERT(_grps.array.use > 0)
	STVisualViewerGrp* grp = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, _grps.array.use - 1);
	BOOL reused = FALSE;
	//verify already added (multiple paths to same runId+streamId are posible)
	if(!reused){
		SI32 i; for(i = 0; i < _rows.array.use; i++){
			STVisualViewerRow* row = NBArray_itmPtrAtIndex(&_rows.array, STVisualViewerRow, i);
			//PRINTF_INFO("Comparing row '%s' vs '%s'.\n", row->content->getUID(), c->uid);
			if(NBString_strIsEqual(service->runId, row->uid.runId) && NBString_strIsEqual(strmDesc->uid, row->uid.streamId)){
				reused = TRUE;
				break;
			}
		}
	}
	//Add from pend-list
	if(!reused){
		SI32 i; for(i = 0; i < pendRows->use; i++){
			STVisualViewerRow* row = NBArray_itmPtrAtIndex(pendRows, STVisualViewerRow, i);
			//PRINTF_INFO("Comparing row '%s' vs '%s'.\n", row->content->getUID(), c->uid);
			if(NBString_strIsEqual(service->runId, row->uid.runId) && NBString_strIsEqual(strmDesc->uid, row->uid.streamId)){
				//Move to grp layer
				{
					grp->obj->syncAdd(row->btn, row->btn);
				}
				//Update value
				{
					row->content->sync(service, strmDesc);
				}
				//Reuse row
				NBArray_addValue(&_rows.array, *row);
				//PRINTF_INFO("Meeting row reused(#%d).\n", _rows.array.use);
				NBArray_removeItemAtIndex(pendRows, i);
				reused = TRUE;
				break;
			}
		}
	}
	//Add newly created
	if(!reused){
		const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_LightFront)->normal;
		const float marginI = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.01f);
		NBColor8 colorN; NBCOLOR_ESTABLECER(colorN, 255, 255, 255, 255)
		NBColor8 colorH; NBCOLOR_ESTABLECER(colorH, 255, 255, 255, 255)
		NBColor8 colorT; NBCOLOR_ESTABLECER(colorT, colorTT.r, colorTT.g, colorTT.b, colorTT.a)
		NBColor8 colorIcoN; NBCOLOR_ESTABLECER(colorIcoN, 175, 175, 175, 255)
		NBColor8 colorIcoH; NBCOLOR_ESTABLECER(colorIcoH, 125, 125, 125, 255)
		NBColor8 colorIcoT; NBCOLOR_ESTABLECER(colorIcoT, 150, 150, 150, 255)
		const float widthForInteral = width - _marginsRows.left /*- marginI - texIcoGT->tamanoHD().ancho*/ - _marginsRows.right;
		//
		STVisualViewerRow row;
		NBMemory_setZeroSt(row, STVisualViewerRow);
		//Create row
		{
			row.uid.runId		= NBString_strNewBuffer(service->runId);
			row.uid.streamId	= NBString_strNewBuffer(strmDesc->uid);
			row.content			= new(this) TSVisualViewerStream(&_root, service, strmDesc, widthForInteral, 0, NULL, &_chldResizedCallbck);
		}
		//Create button
		{
			AUIButton* btnRef = new(this) AUIButton();
			btnRef->establecerMargenes(_marginsRows, marginI);
			btnRef->establecerFondo(true, NULL);
			btnRef->establecerFondoColores(colorN, colorH, colorT);
			btnRef->establecerSegsRetrasarOnTouch(0.20f);
			btnRef->agregarObjetoResizable(row.content, row.content, NBST_P(STNBSize, 1.0f, 1.0f), ENIBotonItemAlign_Left, 0.0f, 0, 0);
			//btnRef->agregarIcono(texIcoGT, ENIBotonItemAlign_Right, 0.5f, 0, 0, colorIcoN, colorIcoH, colorIcoT);
			btnRef->establecerEscuchadorBoton(this);
			btnRef->organizarContenido(false);
			grp->obj->syncAdd(btnRef, btnRef);
			row.btn = btnRef;
		}
		NBArray_addValue(&_rows.array, row);
		//PRINTF_INFO("Meeting row added(#%d).\n", _rows.array.use);
	}
}

void TSVisualViewer::privSyncAddStreamsRecurive(const float width, STNBArray* pendRows, STTSClientRequesterDeviceRef rootSrcRef, const STTSStreamsServiceDesc* srcDesc){
	const BOOL addStreams = (
							 (!TSClientRequesterDevice_isSet(_data.src) || TSClientRequesterDevice_isSame(_data.src, rootSrcRef))
							 && (_data.srcRunId.length == 0 || NBString_strIsEqual(_data.srcRunId.str, srcDesc->runId)
								 )
							 );
	//process streams
	if(srcDesc->streamsGrps != NULL && srcDesc->streamsGrpsSz > 0){
		UI32 i; for(i = 0; i < srcDesc->streamsGrpsSz; i++){
			const STTSStreamsGrpDesc* grpDesc = &srcDesc->streamsGrps[i];
			if(grpDesc->streams != NULL && grpDesc->streamsSz > 0){
				UI32 i; for(i = 0; i < grpDesc->streamsSz; i++){
					const STTSStreamDesc* strmDesc = &grpDesc->streams[i];
					if(addStreams){
						this->privSyncAddStreamOnce(width, pendRows, srcDesc, strmDesc);
					}
				}
			}
		}
	}
	//process subs-services
	if(srcDesc->subServices != NULL && srcDesc->subServicesSz > 0){
		UI32 i; for(i = 0; i < srcDesc->subServicesSz; i++){
			const STTSStreamsServiceDesc* subSrc = &srcDesc->subServices[i];
			this->privSyncAddStreamsRecurive(width,  pendRows, rootSrcRef, subSrc);
		}
	}
}

void TSVisualViewer::privSyncStreams(const float width){
	_sync.isDirty = FALSE;
	//Load rows (invert order)
	{
		{
			//AUTextura* texIcoGT	= NBGestorTexturas::texturaDesdeArchivoPNG("thinstream/icons.png#viewItem");
			const SI32 qGrpsBefore = _grps.array.use;
			const SI32 qRowsBefore = _rows.array.use;
			STNBArray pendRows;	//STVisualViewerRow
			STNBArray pendGrps;	//STVisualViewerGrp
			//Populate pends
			{
				//Rows (existent)
				{
					NBArray_init(&pendRows, sizeof(STVisualViewerRow), NULL);
					NBArray_addItems(&pendRows, NBArray_data(&_rows.array), sizeof(STVisualViewerRow), _rows.array.use);
					NBArray_empty(&_rows.array);
				}
				//Grps (existent)
				{
					NBArray_init(&pendGrps, sizeof(STVisualViewerGrp), NULL);
					NBArray_addItems(&pendGrps, NBArray_data(&_grps.array), sizeof(STVisualViewerGrp), _grps.array.use);
					{
						SI32 i; for(i = 0; i < _grps.array.use; i++){
							STVisualViewerGrp* itm = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, i);
							TSVisualRowsGrp* grp = itm->obj;
							grp->syncStart();
						}
					}
					NBArray_empty(&_grps.array);
				}
			}
			//Add streams buttons
			{
				const char* grpName = TSClient_getStr(_root.coreClt, "guiDashOpenMeetingsTitle", "Sources");
				this->privSyncAddGrpToEnd(grpName, &pendGrps);
				{
					STTSClientRequesterRef reqRef = TSClient_getStreamsRequester(_root.coreClt);
					if(TSClientRequester_isSet(reqRef)){
						SI32 srcsSz = 0; 
						STTSClientRequesterDeviceRef* srcs = TSClientRequester_getGrpsAndLock(reqRef, &srcsSz);
						if(srcs != NULL){
							SI32 i; for(i = 0; i < srcsSz; i++){
								STTSClientRequesterDeviceRef srcRef = srcs[i];
								STTSClientRequesterDeviceOpq* srcOpq = (STTSClientRequesterDeviceOpq*)srcRef.opaque;
								const STTSStreamsServiceDesc* srcDesc = &srcOpq->remoteDesc;
								this->privSyncAddStreamsRecurive(width, &pendRows, srcRef, srcDesc);
							}
							TSClientRequester_returnGrpsAndUnlock(reqRef, srcs);
						}
					}
				}
			}
			//Add info options
			{
				const char* grpName = TSClient_getStr(_root.coreClt, "guiDashActionsInfoTitle", "Information");
				//this->privSyncAddGrpAndBtn(grpName, _buttons.info.eSign, &pendGrps, &pendRows);
				this->privSyncAddGrpAndBtn(grpName, _buttons.info.termsAndConds, &pendGrps, &pendRows);
				this->privSyncAddGrpAndBtn(grpName, _buttons.info.privacy, &pendGrps, &pendRows);
				this->privSyncAddGrpAndBtn(grpName, _buttons.info.about, &pendGrps, &pendRows);
			}
			//Release pends
			{
				//Print stats
				{
					const SI32 qGrpsAfter = _grps.array.use;
					const SI32 qRowsAfter = _rows.array.use;
					const SI32 qGrpsDelta = (qGrpsAfter - qGrpsBefore);
					const SI32 qRowsDelta = (qRowsAfter - qRowsBefore);
					//PRINTF_INFO("Synced %d meetings; started with grps(%d) rows(%d); delta grps(%s%d) rows(%s%d); deleting unused grps(%d) rows(%d).\n", meetIds.use, qGrpsBefore, qRowsBefore, (qGrpsDelta >= 0 ? "+" : ""), qGrpsDelta, (qRowsDelta >= 0 ? "+" : ""), qRowsDelta, pendGrps.use, pendRows.use);
				}
				{
					SI32 i; for(i = 0; i < _grps.array.use; i++){
						STVisualViewerGrp* itm = NBArray_itmPtrAtIndex(&_grps.array, STVisualViewerGrp, i);
						TSVisualRowsGrp* grp = itm->obj;
						grp->syncEndByRemovingUnsynced();
					}
				}
				//Rows (remove non-consumed)
				{
					SI32 i; for(i = 0; i < pendRows.use; i++){
						STVisualViewerRow* itm = NBArray_itmPtrAtIndex(&pendRows, STVisualViewerRow, i);
						//uid
						{
							if(itm->uid.runId != NULL) NBMemory_free(itm->uid.runId); itm->uid.runId = NULL;
							if(itm->uid.streamId != NULL) NBMemory_free(itm->uid.streamId); itm->uid.streamId = NULL;
						}
						if(itm->content != NULL) itm->content->liberar(NB_RETENEDOR_THIS); itm->content = NULL;
						if(itm->btn != NULL){
							AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->btn->contenedor();
							if(parent != NULL) parent->quitarObjetoEscena(itm->btn);
							itm->btn->liberar(NB_RETENEDOR_THIS);
							itm->btn = NULL;
						}
					}
					NBArray_release(&pendRows);
				}
				//Grps (remove non-consumed)
				{
					SI32 i; for(i = 0; i < pendGrps.use; i++){
						STVisualViewerGrp* itm = NBArray_itmPtrAtIndex(&pendGrps, STVisualViewerGrp, i);
						{
							AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
							if(parent != NULL) parent->quitarObjetoEscena(itm->obj);
						}
						itm->obj->liberar(NB_RETENEDOR_THIS);
						itm->obj = NULL;
					}
					NBArray_release(&pendGrps);
				}
			}
		}
	}
	PRINTF_INFO("TSVisualViewer, streams synced.\n");
}

//

void TSVisualViewer::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		BOOL organize = FALSE;
		//Sync
		if(_sync.isDirty){
			_sync.isDirty = FALSE;
			this->privSyncStreams(_anchoUsar);
			organize = TRUE;
		}
		//Organize
		if(organize || _chldResizedCount > 0){
			this->privOrganizarContenido(_anchoUsar, _altoUsar, TRUE, NULL, 0.0f);
		}
	}
}

//Buttons

void TSVisualViewer::botonPresionado(AUIButton* obj){
	//
}

void TSVisualViewer::botonAccionado(AUIButton* obj){
	SI32 i; for(i = 0; i < _rows.array.use; i++){
		STVisualViewerRow* row = NBArray_itmPtrAtIndex(&_rows.array, STVisualViewerRow, i);
		if(row->btn == obj){
			if(row->btn == _buttons.info.termsAndConds){
				PRINTF_INFO("Terms and conds.\n");
				if(_root.lobby != NULL){
					_root.lobby->lobbyPushTermsAndConds();
				}
			} else if(row->btn == _buttons.info.privacy){
				PRINTF_INFO("Privacy.\n");
				if(_root.lobby != NULL){
					_root.lobby->lobbyPushPrivacy();
				}
			} else {
				NBASSERT(row->content != NULL)
				PRINTF_INFO("Stream selected: '%s@%s'.\n", row->uid.streamId, row->uid.runId);
				/*if(_lstnr != NULL){
					_lstnr->homeStreamsSourceSelected(this, uid);
				}*/
			}
			//End-cicle
			break;
		}
	} NBASSERT(i < _rows.array.use)
}

AUEscenaObjeto* TSVisualViewer::botonHerederoParaTouch(AUIButton* obj, const NBPunto &posInicialEscena, const NBPunto &posActualEscena){
	return _touchInheritor;
}

//Touches

void TSVisualViewer::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	if(_add.layer == objeto){
		_add.layer->establecerMultiplicadorAlpha8(200);
	}
}

void TSVisualViewer::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	//Resign to touch at drag
	{
		const STNBSize deltaScn = NBST_P(STNBSize, 
			posActualEscena.x - posInicialEscena.x
			, posActualEscena.y - posInicialEscena.y );
		const STNBSize deltaInch = NBST_P(STNBSize, 
			NBGestorEscena::anchoEscenaAPulgadas(_root.iScene, deltaScn.width)
			, NBGestorEscena::anchoEscenaAPulgadas(_root.iScene, deltaScn.height)
		);
		const float dist2 = (deltaInch.width * deltaInch.width) + (deltaInch.height * deltaInch.height);
		if(dist2 > (0.10f * 0.10f)){
			objeto->liberarTouch(touch, posInicialEscena, posAnteriorEscena, posActualEscena, TRUE, _touchInheritor);
		}
	}
}

void TSVisualViewer::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(!touch->cancelado){
		const NBCajaAABB cajaEscena = objeto->cajaAABBEnEscena();
		if(NBCAJAAABB_INTERSECTA_PUNTO(cajaEscena, posActualEscena.x, posActualEscena.y) && !touch->cancelado){
			if(_add.layer == objeto){
				_root.scenes->loadInfoSequence(ENSceneAction_Push, ENSceneInfoMode_Register, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
			}
		}
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualViewer, AUEscenaContenedor)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualViewer, "TSVisualViewer", AUEscenaContenedor)
AUOBJMETODOS_CLONAR_NULL(TSVisualViewer)


