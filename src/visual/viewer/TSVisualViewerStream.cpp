//
//  TSVisualViewerStream.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/viewer/TSVisualViewerStream.h"
#include "visual/TSColors.h"
#include "nb/core/NBCompare.h"

TSVisualViewerStream::TSVisualViewerStream(const STLobbyColRoot* root, const STTSStreamsServiceDesc* serviceDesc, const STTSStreamDesc* strmDesc, const float anchoUsar, const float altoUsar, ITSVisualViewerStreamListener* lstnr, STNBCallback* parentCallback) : AUEscenaContenedorLimitado() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualViewerStream")
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
	}
	//gui
	{
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
			this->agregarObjetoEscena(_gui.server);
		}
		//port
		{
			const STNBColor8 colorTT = TSColors::colorDef(ENTSColor_Gray)->normal;
			STNBString str;
			NBString_init(&str);
			NBString_concat(&str, ":");
			NBString_concatUI32(&str, 0);
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
				NBString_concatUI32(&str, 0);
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
				NBString_concatUI32(&str, 0);
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
	this->privSync(serviceDesc, strmDesc);
	this->privOrganizarContenido(anchoUsar, altoUsar, false);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

TSVisualViewerStream::~TSVisualViewerStream(){
	NBGestorAnimadores::quitarAnimador(this);
	//data
	{
		{
			NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &_data.serviceDesc, sizeof(_data.serviceDesc));
		}
		{
			NBStruct_stRelease(TSStreamDesc_getSharedStructMap(), &_data.strmDesc, sizeof(_data.strmDesc));
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

void TSVisualViewerStream::recordChanged(void* param, const char* relRoot, const char* relPath){
	TSVisualViewerStream* obj = (TSVisualViewerStream*)param;
	if(obj != NULL){
		//
	}
}

void TSVisualViewerStream::sync(const STTSStreamsServiceDesc* serviceDesc, const STTSStreamDesc* strmDesc){
	this->privSync(serviceDesc, strmDesc);
	this->privOrganizarContenido(_anchoUsar, _altoUsar, TRUE);
}


void TSVisualViewerStream::privSync(const STTSStreamsServiceDesc* serviceDesc, const STTSStreamDesc* strmDesc){
	{
		NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &_data.serviceDesc, sizeof(_data.serviceDesc));
		if(serviceDesc != NULL){
			NBStruct_stClone(TSStreamsServiceDesc_getSharedStructMap(), serviceDesc, sizeof(*serviceDesc), &_data.serviceDesc, sizeof(_data.serviceDesc));
		}
	}
	{
		NBStruct_stRelease(TSStreamDesc_getSharedStructMap(), &_data.strmDesc, sizeof(_data.strmDesc));
		if(strmDesc != NULL){
			NBStruct_stClone(TSStreamDesc_getSharedStructMap(), strmDesc, sizeof(*strmDesc), &_data.strmDesc, sizeof(_data.strmDesc));
		}
	}
	//update GUI
	{
		STNBString str;
		NBString_init(&str);
		//desc
		_gui.server->establecerTexto(_data.strmDesc.uid);
		//analyze sizes
		{
			UI32 liveCount = 0, storageCount = 0;
			STNBArraySorted liveSzs, storageSzs;
			NBArraySorted_init(&liveSzs, sizeof(UI32), NBCompareUI32);
			NBArraySorted_init(&storageSzs, sizeof(UI32), NBCompareUI32);
			if(_data.strmDesc.versions != NULL && _data.strmDesc.versionsSz > 0){
				UI32 i; for(i = 0; i < _data.strmDesc.versionsSz; i++){
					const STTSStreamVerDesc* v = &_data.strmDesc.versions[i];
					if(v->live.isOnline){
						liveCount++;
						if(v->live.props.w > 0){
							const UI32 wSz = v->live.props.w; 
							if(NBArraySorted_indexOf(&liveSzs, &wSz, sizeof(wSz), NULL) < 0){
								NBArraySorted_addValue(&liveSzs, wSz);
							}
						}
					}
					if(v->storage.isOnline){
						storageCount++;
						if(v->storage.props.w > 0){
							const UI32 wSz = v->storage.props.w;
							if(NBArraySorted_indexOf(&storageSzs, &wSz, sizeof(wSz), NULL) < 0){
								NBArraySorted_addValue(&storageSzs, wSz);
							}
						}
					}
				}
			}
			//live
			{
				NBString_empty(&str);
				if(liveCount > 0){
					if(liveSzs.use > 0){
						const UI32* arrSz = NBArraySorted_dataPtr(&liveSzs, UI32);
						const UI32* sz = arrSz;
						const UI32* szAfterEnd = sz + liveSzs.use;
						NBString_concat(&str, "(");
						while(sz < szAfterEnd){
							if(sz != arrSz) NBString_concat(&str, ", "); 
							NBString_concatUI32(&str, *sz);
							sz++;
						}
						NBString_concat(&str, ")");
					}
					if(str.length > 0){
						NBString_concat(&str, " ");
					}
					NBString_concatUI32(&str, liveCount);
					NBString_concat(&str, " live");
				}
				_gui.counts.live->establecerTexto(str.str);
			}
			//storage
			{
				NBString_empty(&str);
				if(storageCount > 0){
					if(storageSzs.use > 0){
						const UI32* arrSz = NBArraySorted_dataPtr(&storageSzs, UI32);
						const UI32* sz = arrSz;
						const UI32* szAfterEnd = sz + storageSzs.use;
						NBString_concat(&str, "(");
						while(sz < szAfterEnd){
							if(sz != arrSz) NBString_concat(&str, ", "); 
							NBString_concatUI32(&str, *sz);
							sz++;
						}
						NBString_concat(&str, ")");
					}
					if(str.length > 0){
						NBString_concat(&str, " ");
					}
					NBString_concatUI32(&str, storageCount);
					NBString_concat(&str, " recs");
				}
				_gui.counts.storage->establecerTexto(str.str);
			}
			//loading
			{
				_gui.connecting.ico->setVisibleAndAwake(liveCount == 0 && storageCount == 0);
			}
			NBArraySorted_release(&liveSzs);
			NBArraySorted_release(&storageSzs);
		}
		NBString_release(&str);
	}
}

//

BOOL TSVisualViewerStream::isSearchMatch(const char** wordsLwr, const SI32 wordsLwrSz){
	return FALSE;
}

//AUEscenaListaItemI

AUEscenaObjeto* TSVisualViewerStream::itemObjetoEscena(){
	return this;
}

STListaItemDatos TSVisualViewerStream::refDatos(){
	return _refDatos;
}

void TSVisualViewerStream::establecerRefDatos(const SI32 tipo, const SI32 valor){
	_refDatos.tipo	= tipo;
	_refDatos.valor	= valor;
}

void TSVisualViewerStream::setResizedCallback(STNBCallback* parentCallback){
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

void TSVisualViewerStream::popOptionSelected(void* param, void* ref, const char* optionId){
	//
}

void TSVisualViewerStream::popFocusObtained(void* param, void* ref){
	//
}

void TSVisualViewerStream::popFocusLost(void* param, void* ref){
	//
}

//

void TSVisualViewerStream::setTouchInheritor(AUEscenaObjeto* touchInheritor){
	_touchInheritor = touchInheritor;
}

void TSVisualViewerStream::organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(ancho, altoVisible, notificarCambioAltura);
}

void TSVisualViewerStream::privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura){
	//PRINTF_INFO("TSVisualViewerStream, privOrganizarContenido(%f).\n", ancho);
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

void TSVisualViewerStream::tickAnimacion(float segsTranscurridos){
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
		//organize
		if(organize){
			this->privOrganizarContenido(_anchoUsar, _altoUsar, TRUE);
		}
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualViewerStream, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualViewerStream, "TSVisualViewerStream", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(TSVisualViewerStream)

