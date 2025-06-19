//
//  TSVisualHomeSourceRow.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/home/TSVisualHomeSourceRow.h"
#include "visual/TSColors.h"

TSVisualHomeSourceRow::TSVisualHomeSourceRow(const STLobbyColRoot* root, STTSClientRequesterDeviceRef src, const float anchoUsar, const float altoUsar, ITSVisualHomeSourceRowListener* lstnr, STNBCallback* parentCallback) : AUEscenaContenedorLimitado() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualHomeSourceRow")
	_root				= *root;
	//
	NBMemory_setZero(_refDatos);
	//Set new ref
	if(parentCallback == NULL){
		NBCallback_init(&_prntResizedCallbck);
	} else {
		NBCallback_initAsRefOf(&_prntResizedCallbck, parentCallback);
	}
	_touchInheritor		= NULL;
	_anchoUsar			= 0.0f; NBASSERT(anchoUsar > 0.0f)
	_altoUsar			= 0.0f;
	_yTopLast			= 0; //integer to allow an error-margin
	_lstnr				= lstnr;
	//data
	{
		NBMemory_setZero(_data);
		TSClientRequesterDevice_set(&_data.src, &src);
		if(TSClientRequesterDevice_isSet(_data.src)){
			//add listener
			STTSClientRequesterDeviceLstnr lstnr;
			NBMemory_setZeroSt(lstnr, STTSClientRequesterDeviceLstnr);
			lstnr.param = this;
			lstnr.itf.requesterSourceConnStateChanged = requesterSourceConnStateChanged;
			lstnr.itf.requesterSourceDescChanged = requesterSourceDescChanged;
			TSClientRequesterDevice_addLstnr(_data.src, &lstnr);
		}
	}
	//gui
	{
		STTSClientRequesterDeviceOpq* srcOpq = (STTSClientRequesterDeviceOpq*)src.opaque;
		//AUTextura* bgTex = NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#btn");
		//AUTextura* icoGT = NBGestorTexturas::texturaDesdeArchivo("thinstream/icons-gt.png");
		//AUFuenteRender* fntBtn = TSFonts::font(ENTSFont_ContentMid);
		NBMemory_setZero(_gui);
		//margins
		{
			_gui.margins.left = _gui.margins.right = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.04f);
			_gui.margins.top = _gui.margins.bottom = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.04f);
			_gui.marginI = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.05f);
		}
		//server
		{
			const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_Black)->normal;
			_gui.server		= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentTitle));
			_gui.server->establecerMultiplicadorColor8(colorTT.r, colorTT.g, colorTT.b, colorTT.a);
			_gui.server->establecerAlineacionH(ENNBTextLineAlignH_Left);
			_gui.server->establecerTexto(srcOpq->cfg.server.server);
			this->agregarObjetoEscena(_gui.server);
		}
		//port
		{
			const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_Gray)->normal;
			STNBString str;
			NBString_init(&str);
			NBString_concat(&str, ":");
			NBString_concatUI32(&str, srcOpq->cfg.server.port);
			_gui.port		= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentSmall));
			_gui.port->establecerMultiplicadorColor8(colorTT.r, colorTT.g, colorTT.b, colorTT.a);
			_gui.port->establecerAlineacionH(ENNBTextLineAlignH_Left);
			_gui.port->establecerTexto(str.str);
			this->agregarObjetoEscena(_gui.port);
			NBString_release(&str);
		}
		//connecting
		{
			AUTextura* tex = NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner"); //moreOptions
			AUEscenaSprite* ico = new AUEscenaSprite(tex);
			this->agregarObjetoEscena(ico);
			_gui.connecting.ico = ico;
		}
		//counts
		{
			//live
			{
				const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_Gray)->normal;
				STNBString str;
				NBString_init(&str);
				NBString_concat(&str, ":");
				NBString_concatUI32(&str, srcOpq->cfg.server.port);
				_gui.counts.live		= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentSmall));
				_gui.counts.live->establecerMultiplicadorColor8(colorTT.r, colorTT.g, colorTT.b, colorTT.a);
				_gui.counts.live->establecerAlineacionH(ENNBTextLineAlignH_Right);
				_gui.counts.live->establecerTexto(str.str);
				this->agregarObjetoEscena(_gui.counts.live);
				NBString_release(&str);
			}
			//storage
			{
				const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_Gray)->normal;
				STNBString str;
				NBString_init(&str);
				NBString_concat(&str, ":");
				NBString_concatUI32(&str, srcOpq->cfg.server.port);
				_gui.counts.storage		= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentSmall));
				_gui.counts.storage->establecerMultiplicadorColor8(colorTT.r, colorTT.g, colorTT.b, colorTT.a);
				_gui.counts.storage->establecerAlineacionH(ENNBTextLineAlignH_Right);
				_gui.counts.storage->establecerTexto(str.str);
				this->agregarObjetoEscena(_gui.counts.storage);
				NBString_release(&str);
			}
		}
	}
	//Organizar
	this->privSync();
	this->privOrganizarContenido(anchoUsar, altoUsar, false);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

TSVisualHomeSourceRow::~TSVisualHomeSourceRow(){
	NBGestorAnimadores::quitarAnimador(this);
	//data
	{
		if(TSClientRequesterDevice_isSet(_data.src)){
			//remove listener
			TSClientRequesterDevice_removeLstnr(_data.src, this);
			//
			TSClientRequesterDevice_release(&_data.src);
			TSClientRequesterDevice_null(&_data.src);
		}
	}
	//
	NBCallback_release(&_prntResizedCallbck);
	//Gui
	{
		if(_gui.server != NULL) _gui.server->liberar(NB_RETENEDOR_THIS); _gui.server = NULL;
		if(_gui.port != NULL) _gui.port->liberar(NB_RETENEDOR_THIS); _gui.port = NULL;
		//connecting
		{
			if(_gui.connecting.ico != NULL) _gui.connecting.ico->liberar(NB_RETENEDOR_THIS); _gui.connecting.ico = NULL;
		}
		//counts
		{
			if(_gui.counts.live != NULL) _gui.counts.live->liberar(NB_RETENEDOR_THIS); _gui.counts.live = NULL;
			if(_gui.counts.storage != NULL) _gui.counts.storage->liberar(NB_RETENEDOR_THIS); _gui.counts.storage = NULL;
		}
	}
}

//

void TSVisualHomeSourceRow::recordChanged(void* param, const char* relRoot, const char* relPath){
	TSVisualHomeSourceRow* obj = (TSVisualHomeSourceRow*)param;
	if(obj != NULL){
		//
	}
}

void TSVisualHomeSourceRow::requesterSourceConnStateChanged(void* param, const STTSClientRequesterDeviceRef source, const ENTSClientRequesterDeviceConnState state){
	TSVisualHomeSourceRow* obj = (TSVisualHomeSourceRow*)param;
	obj->_data.isConnDirty = TRUE;
}

void TSVisualHomeSourceRow::requesterSourceDescChanged(void* param, const STTSClientRequesterDeviceRef source){
	TSVisualHomeSourceRow* obj = (TSVisualHomeSourceRow*)param;
	obj->_data.isDescDirty = TRUE;
}

void TSVisualHomeSourceRow::privSync(){
	_data.isConnDirty = FALSE;
	_data.isDescDirty = FALSE;
	//
	if(TSClientRequesterDevice_isSet(_data.src)){
		BOOL isConnected = FALSE;
		UI32 streamsLiveCount = 0, streamsStorageCount = 0;
		STTSClientRequesterDeviceOpq* srcOpq = (STTSClientRequesterDeviceOpq*)_data.src.opaque;
		//Analyze current state
		NBObject_lock(srcOpq);
		{
			isConnected = (srcOpq->connState == ENTSClientRequesterDeviceConnState_Connected);
			switch(srcOpq->cfg.type){
				case ENTSClientRequesterDeviceType_Local:
					//should be one stream with multiple versions
					NBASSERT(srcOpq->remoteDesc.streamsGrpsSz == 1 && srcOpq->remoteDesc.streamsGrps[0].streamsSz == 1)
					streamsLiveCount++; 
					break;
				case ENTSClientRequesterDeviceType_Native:
					{
						const STTSStreamsServiceDesc* remoteDesc = &srcOpq->remoteDesc;
						if(remoteDesc->streamsGrps != NULL && remoteDesc->streamsGrpsSz > 0){
							UI32 i; for(i = 0; i < remoteDesc->streamsGrpsSz; i++){
								const STTSStreamsGrpDesc* grp = &remoteDesc->streamsGrps[i];
								if(grp->streams != NULL && grp->streamsSz > 0){
									UI32 i; for(i = 0; i < grp->streamsSz; i++){
										const STTSStreamDesc* stream = &grp->streams[i];
										if(stream->versions != NULL && stream->versionsSz > 0){
											UI32 vLiveCount = 0, vStorageCount = 0;
											UI32 i; for(i = 0;  i < stream->versionsSz; i++){
												const STTSStreamVerDesc* ver = &stream->versions[i];
												if(ver->live.isOnline){
													vLiveCount++;
												}
												if(ver->storage.isOnline){
													vStorageCount++;
												}
											}
											if(vLiveCount > 0){
												streamsLiveCount++;
											}
											if(vStorageCount > 0){
												streamsStorageCount++;
											}
										}
									}
								}
							}
						}
					}
					break;
				default:
					NBASSERT(FALSE)
					break;
			}
			
		}
		NBObject_unlock(srcOpq);
		//apply to GUI
		{
			STNBString str, strNum;
			NBString_init(&str);
			NBString_init(&strNum);
			{
				//connecting
				{
					_gui.connecting.ico->establecerVisible(!isConnected);
				}
				//live
				{
					NBString_empty(&str);
					//
					if(streamsLiveCount > 0){
						NBString_empty(&strNum);
						NBString_concatUI32(&strNum, streamsLiveCount);
						NBString_set(&str, TSClient_getStr(_root.coreClt, "guiStreamLiveCount", "[count] live"));
						NBString_replace(&str, "[count]", strNum.str);
					}
					//
					_gui.counts.live->establecerTexto(str.str);
					_gui.counts.live->establecerVisible(isConnected);
				}
				//storage
				{
					NBString_empty(&str);
					//
					if(streamsStorageCount > 0){
						NBString_empty(&strNum);
						NBString_concatUI32(&strNum, streamsStorageCount);
						NBString_set(&str, TSClient_getStr(_root.coreClt, "guiStreamLiveCount", "[count] recs"));
						NBString_replace(&str, "[count]", strNum.str);
					}
					//
					_gui.counts.storage->establecerTexto(str.str);
					_gui.counts.storage->establecerVisible(isConnected);
				}
			}
			NBString_release(&strNum);
			NBString_release(&str);
		}
	}
}

//

BOOL TSVisualHomeSourceRow::isSameSource(STTSClientRequesterDeviceRef src) const {
	return TSClientRequesterDevice_isSame(_data.src, src);
}

STTSClientRequesterDeviceRef TSVisualHomeSourceRow::getSourceRef() const {
	return _data.src;
}

BOOL TSVisualHomeSourceRow::isSearchMatch(const char** wordsLwr, const SI32 wordsLwrSz){
	return FALSE;
}

//AUEscenaListaItemI

AUEscenaObjeto* TSVisualHomeSourceRow::itemObjetoEscena(){
	return this;
}

STListaItemDatos TSVisualHomeSourceRow::refDatos(){
	return _refDatos;
}

void TSVisualHomeSourceRow::establecerRefDatos(const SI32 tipo, const SI32 valor){
	_refDatos.tipo	= tipo;
	_refDatos.valor	= valor;
}

void TSVisualHomeSourceRow::setResizedCallback(STNBCallback* parentCallback){
	//Release
	NBCallback_release(&_prntResizedCallbck);
	//Set new ref
	if(parentCallback == NULL){
		NBCallback_init(&_prntResizedCallbck);
	} else {
		NBCallback_initAsRefOf(&_prntResizedCallbck, parentCallback);
	}
}

//IPopListener

void TSVisualHomeSourceRow::popOptionSelected(void* param, void* ref, const char* optionId){
	//
}

void TSVisualHomeSourceRow::popFocusObtained(void* param, void* ref){
	//
}

void TSVisualHomeSourceRow::popFocusLost(void* param, void* ref){
	//
}

//

void TSVisualHomeSourceRow::setTouchInheritor(AUEscenaObjeto* touchInheritor){
	_touchInheritor = touchInheritor;
}

void TSVisualHomeSourceRow::organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(ancho, altoVisible, notificarCambioAltura);
}

void TSVisualHomeSourceRow::privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura){
	//PRINTF_INFO("TSVisualHomeSourceRow, privOrganizarContenido(%f).\n", ancho);
	if(ancho > 0.0f){
		UI32 prevRowCount = 0;
		AUEscenaSpriteElastico* prevRowBg = NULL;
		AUTextura* texRowSingle	= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#rowSingle");
		AUTextura* texRowTop	= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#rowTop");
		AUTextura* texRowMid	= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#rowMid");
		AUTextura* texRowBtm	= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#rowBtm");
		const float marginH		= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.10f);
		const float marginV		= NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.10f);
		const float marginVWithBtn = NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.07f);
		const float lineHeight	= NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.005f);
		const float yTopStart	= -marginV; 
		float yTop				= yTopStart;
		float xLeft = 0.0f, xRight = ancho;
		//fullname
		{
			_gui.server->organizarTexto(xRight - xLeft - marginH - marginH - marginH - marginH);
			_gui.port->organizarTexto(xRight - xLeft - marginH - marginH - marginH - marginH);
			{
				const NBCajaAABB serverBox	= _gui.server->cajaAABBLocal();
				const float serverLeft		= xLeft + marginH + marginH - serverBox.xMin + ((ancho - (serverBox.xMax - serverBox.xMin)) * 0.0f);
				if(yTop != yTopStart) yTop -= marginV;
				_gui.server->establecerTraslacion(serverLeft, yTop - serverBox.yMax);
				{
					const NBCajaAABB portBox = _gui.port->cajaAABBLocal();
					_gui.port->establecerTraslacion(serverLeft - portBox.xMin + (serverBox.xMax - serverBox.xMin) + (marginH * 0.5f), yTop - portBox.yMax - (((serverBox.yMax - serverBox.yMin) - (portBox.yMax - portBox.yMin)) * 1.0f));
				}
				yTop -= (serverBox.yMax - serverBox.yMin) + (marginV * 0.5f);
			}
			//xLeft += marginH;
			//xRight -= marginH;
		}
		//connecting
		{
			float yTop2 = yTopStart;
			{
				const NBCajaAABB box = _gui.connecting.ico->cajaAABBLocal();
				if(yTop2 != yTopStart) yTop2 -= marginV * 0.5f;
				_gui.connecting.ico->establecerTraslacion(xRight - marginH - marginH - box.xMax, yTop2 - box.yMax);
				yTop2 -= (box.yMax - box.yMin);
			}
			if(yTop > yTop2){
				//set as new-bottom
				yTop = yTop2;
			} else {
				//center vertically?
			}
		}
		//counts
		{
			float yTop2 = yTopStart;
			_gui.counts.live->organizarTexto(xRight - xLeft - marginH - marginH - marginH - marginH);
			_gui.counts.storage->organizarTexto(xRight - xLeft - marginH - marginH - marginH - marginH);
			{
				const NBCajaAABB box = _gui.counts.live->cajaAABBLocal();
				if(yTop2 != yTopStart) yTop2 -= marginV * 0.5f;
				_gui.counts.live->establecerTraslacion(xRight - marginH - marginH - box.xMax, yTop2 - box.yMax);
				yTop2 -= (box.yMax - box.yMin);
			}
			{
				const NBCajaAABB box = _gui.counts.storage->cajaAABBLocal();
				if(yTop2 != yTopStart) yTop2 -= marginV * 0.5f;
				_gui.counts.storage->establecerTraslacion(xRight - marginH - marginH - box.xMax, yTop2 - box.yMax);
				yTop2 -= (box.yMax - box.yMin);
			}
			if(yTop > yTop2){
				//set as new-bottom
				yTop = yTop2;
			} else {
				//center vertically?
			}
		}
		//Set limits
		{
			//if(yTop != yTopStart) yTop -= marginV;
			this->establecerLimites(0.0f, ancho, yTop, 0.0f);
		}
		//
		_anchoUsar		= ancho;
		_altoUsar		= alto;
		if(_yTopLast != yTop){
			_yTopLast	= yTop;
			if(notificarCambioAltura){
				//PRINTF_INFO("Notifying callback.\n");
				NBCallback_call(&_prntResizedCallbck);
			}
		}
	}
}

void TSVisualHomeSourceRow::tickAnimacion(float segsTranscurridos){
	//Anim
	if(this->idEscena >= 0){
		BOOL organize = FALSE;
		//connecting
		{
			if(_gui.connecting.ico->visible()){
				const SI32 angStep = (360 / 12);
				_gui.connecting.rot -= (360.0f * segsTranscurridos);
				while(_gui.connecting.rot < 0.0f){
					_gui.connecting.rot += 360.0f;
				}
				_gui.connecting.ico->establecerRotacionNormalizada(((SI32)_gui.connecting.rot / angStep) * angStep);
			}
		}
		//sync
		if(_data.isConnDirty || _data.isDescDirty){
			_data.isConnDirty = FALSE;
			_data.isDescDirty = FALSE;
			this->privSync();
			organize = TRUE;
		}
		//organize
		if(organize){
			this->privOrganizarContenido(_anchoUsar, _altoUsar, TRUE);
		}
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualHomeSourceRow, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualHomeSourceRow, "TSVisualHomeSourceRow", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(TSVisualHomeSourceRow)

