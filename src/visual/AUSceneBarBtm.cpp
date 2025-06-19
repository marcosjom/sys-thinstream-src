//
//  AUSceneBarBtm.cpp
//  app-refranero
//
//  Created by Marcos Ortega on 14/03/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#include "visual/TSVisualPch.h"
#include "AUSceneBarBtm.h"
#include "visual/TSColors.h"
#include "visual/TSFonts.h"

#define ANCHO_BASE_PUBLICIDAD	512.0f

AUSceneBarBtm::AUSceneBarBtm(const STLobbyRoot* root, const float anchoParaContenido, const float altoExtra)
: AUEscenaContenedorLimitado()
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneBarBtm")
	_root			= *root;
	//
	_anchoParaContenido = anchoParaContenido;
	_altoExtra		= 0.0f;
	_marginIn		= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.05f);
	{
		this->establecerAltoExtra(altoExtra);
	}
	//Current touch
	{
		_touch.first	= NULL;
		_touch.iSel		= ENLobbyStack_Count;
	}
	//Bg
	{
		const STNBColor8 color = TSColors::colorDef(ENTSColor_BarBg)->normal;
		//Bg shadow
		{
			_bg.shadow		= new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
			_bg.shadow->establecerModo(ENEscenaFiguraModo_Relleno);
			//
			_bg.shadow->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg.shadow->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg.shadow->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg.shadow->agregarCoordenadaLocal(0.0f, 0.0f);
			//Colors
			_bg.shadow->colorearVertice(0, 255, 255, 255, 25);
			_bg.shadow->colorearVertice(1, 255, 255, 255, 25);
			_bg.shadow->colorearVertice(2, 255, 255, 255, 0);
			_bg.shadow->colorearVertice(3, 255, 255, 255, 0);
			//
			_bg.shadow->establecerEscuchadorTouches(this, this);
			_bg.shadow->establecerMultiplicadorColor8(0, 0, 0, 255);
			this->agregarObjetoEscena(_bg.shadow);
		}
		//Bg
		{
			_bg.obj			= new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
			_bg.obj->establecerModo(ENEscenaFiguraModo_Relleno);
			//
			_bg.obj->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg.obj->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg.obj->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg.obj->agregarCoordenadaLocal(0.0f, 0.0f);
			//Colors
			_bg.obj->colorearVertice(0, 255, 255, 255, 255);
			_bg.obj->colorearVertice(1, 255, 255, 255, 255);
			_bg.obj->colorearVertice(2, 255, 255, 255, 255);
			_bg.obj->colorearVertice(3, 255, 255, 255, 255);
			//
			_bg.obj->establecerEscuchadorTouches(this, this);
			_bg.obj->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_bg.obj);
		}
		_bg.posAnimStart.x = 0.0f;
		_bg.posAnimStart.y = 0.0f;
	}
	//Selector
	{
		_sel.curIdx = ENLobbyStack_Count;
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_BarBg)->normal;
			AUTextura* tex	= NBGestorTexturas::texturaDesdeArchivoPNG("thinstream/bar-btm-sel-mask-192.png");
			_sel.bg		= new(this) AUEscenaSprite(tex);
			_sel.bg->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_sel.bg);
		}
		_sel.animOutDur	= 0.0f;
		_sel.animOutCur	= 0.0f;
		_sel.animInDur	= 0.0f;
		_sel.animInCur	= 0.0f;
	}
	//Icons
	{
		NBMemory_setZero(_btns);
		{
			AUTextura* tagShadowTex		= NBGestorTexturas::texturaDesdeArchivoPNG("thinstream/iconsSmallCircleBack.png");
			AUFuenteRender* tagFnt		= TSFonts::font(ENTSFont_ContentSmall);
			const STNBColor8 tagBgColor	= TSColors::colorDef(ENTSColor_MainColor)->normal;
			const STNBColor8 tagFgColor = TSColors::colorDef(ENTSColor_White)->normal;
			//
			SI32 i; const STNBColor8 color = TSColors::colorDef(ENTSColor_BarBtnIcoUnsel)->normal;
			for(i = 0; i < ENLobbyStack_Count; i++){
				const char* texPath = NULL;
				const char* title = NULL;
				switch(i){
					case ENLobbyStack_Home:
						texPath = "thinstream/icons.png#home";
						title = TSClient_getStr(_root.coreClt, "mnuBtmDashboardTitle", "Home");
						break;
					case ENLobbyStack_History:
						texPath = "thinstream/icons.png#form";
						title = TSClient_getStr(_root.coreClt, "mnuBtmHistoryTitle", "Forms");
						break;
#					ifdef NB_CONFIG_INCLUDE_ASSERTS
					default:
						NBASSERT(FALSE);
						break;
#					endif
				}
				//
				{
					_btns[i].lyr = new(this) AUEscenaContenedor();
					this->agregarObjetoEscena(_btns[i].lyr);
				}
				//Icon
				{
					NBASSERT(texPath != NULL)
					if(texPath == NULL){
						_btns[i].icon = NULL;
					} else {
						AUEscenaSprite* boton = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivoPNG(texPath));
						boton->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
						boton->setVisibleAndAwake(TRUE);
						_btns[i].lyr->agregarObjetoEscena(boton);
						_btns[i].icon			= boton;
						_btns[i].color.r		= color.r;
						_btns[i].color.g		= color.g;
						_btns[i].color.b		= color.b;
						_btns[i].color.a		= color.a;
						_btns[i].colorAnimStart	= _btns[i].color;
						_btns[i].posSel.x		= 0.0f;
						_btns[i].posSel.y		= 0.0f;
						_btns[i].posUnsel.x		= 0.0f;
						_btns[i].posUnsel.y		= 0.0f;
						_btns[i].posAnimStart.x	= 0.0f;
						_btns[i].posAnimStart.y	= 0.0f;
						_btns[i].hidden			= FALSE;
					}
				}
				//Text
				{
					NBASSERT(title != NULL)
					if(title == NULL){
						_btns[i].text		= NULL;
						_btns[i].textMini	= NULL;
					} else {
						//Normal text (right to icon)
						{
							AUEscenaTexto* txt	= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentMid));
							txt->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
							txt->establecerTexto(title);
							txt->setVisibleAndAwake(FALSE);
							_btns[i].lyr->agregarObjetoEscena(txt);
							_btns[i].text = txt;
						}
						//Small text (below icon)
						{
							AUEscenaTexto* txt	= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentSmall));
							txt->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
							txt->establecerTexto(title);
							txt->setVisibleAndAwake(FALSE);
							_btns[i].lyr->agregarObjetoEscena(txt);
							_btns[i].textMini = txt;
						}
					}
				}
				//Tag
				{
					NBString_init(&_btns[i].tagValue);	//tag value
					//NBString_set(&_btns[i].tagValue, "0"); //Visual test
					_btns[i].tag = new(this) TSVisualTag(_root.iScene, NULL, tagFnt, _btns[i].tagValue.str, NULL, tagBgColor, tagFgColor, 0.0f, 0.0f);
					_btns[i].tag->updateShadow(tagShadowTex, tagFgColor);
					_btns[i].lyr->agregarObjetoEscena(_btns[i].tag);
				}
			}
		}
	}
	//
	this->privOrganizarContenido(anchoParaContenido);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

AUSceneBarBtm::~AUSceneBarBtm(){
	NBGestorAnimadores::quitarAnimador(this);
	if(_bg.obj != NULL) _bg.obj->liberar(NB_RETENEDOR_THIS); _bg.obj = NULL;
	if(_bg.shadow != NULL) _bg.shadow->liberar(NB_RETENEDOR_THIS); _bg.shadow = NULL;
	if(_sel.bg != NULL) _sel.bg->liberar(NB_RETENEDOR_THIS); _sel.bg = NULL;
	//Liberar botones
	{
		SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
			if(_btns[i].lyr != NULL){
				_btns[i].lyr->liberar(NB_RETENEDOR_THIS);
				_btns[i].lyr = NULL;
			}
			//Tag
			{
				NBString_release(&_btns[i].tagValue);	//tag value
				if(_btns[i].tag != NULL){
					_btns[i].tag->liberar(NB_RETENEDOR_THIS);
					_btns[i].tag = NULL;
				}
			}
			if(_btns[i].icon != NULL){
				_btns[i].icon->liberar(NB_RETENEDOR_THIS);
				_btns[i].icon = NULL;
			}
			if(_btns[i].text != NULL){
				_btns[i].text->liberar(NB_RETENEDOR_THIS);
				_btns[i].text = NULL;
			}
			if(_btns[i].textMini != NULL){
				_btns[i].textMini->liberar(NB_RETENEDOR_THIS);
				_btns[i].textMini = NULL;
			}
		}
	}
}

//

float AUSceneBarBtm::altoExtra() const {
	return _altoExtra;
}

void AUSceneBarBtm::establecerAltoExtra(const float altoExtra){
	_altoExtra = altoExtra;
}

//

ENLobbyStack AUSceneBarBtm::seleccionActual() const {
	return _sel.curIdx;
}

void AUSceneBarBtm::establecerSeleccionActual(const ENLobbyStack iSel){
	if(_sel.curIdx != iSel){
		//Set value
		{
			_sel.curIdx	= (iSel >= 0 && iSel < ENLobbyStack_Count ? iSel : ENLobbyStack_Count);
		}
		//Star animations
		if(_sel.animOutDur <= 0.0f){
			const float animDur = 0.25f;
			//Set start of animation state
			{
				SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
					const NBColor8 c = _btns[i].icon->multiplicadorColor8Func();
					const NBPunto p = _btns[i].icon->traslacion();
					_btns[i].colorAnimStart.r	= c.r;
					_btns[i].colorAnimStart.g	= c.g;
					_btns[i].colorAnimStart.b	= c.b;
					_btns[i].colorAnimStart.a	= c.a;
					_btns[i].posAnimStart.x		= p.x;
					_btns[i].posAnimStart.y		= p.y;
				}
				{
					const NBPunto p = _bg.obj->traslacion();
					_bg.posAnimStart.x	= p.x;
					_bg.posAnimStart.y	= p.y;
				}
			}
			if(_sel.animInDur > 0.0f){
				//Reverse current in-animation
				NBASSERT(_sel.animOutDur <= 0.0f && _sel.animOutCur <= 0.0f && _sel.animInDur > 0.0f)
				_sel.animOutCur	= _sel.animInCur;
				_sel.animOutDur	= _sel.animInDur;
				_sel.animInCur	= 0.0f;
				_sel.animInDur	= animDur;
			} else {
				//Init new animation
				NBASSERT(_sel.animOutDur <= 0.0f && _sel.animOutCur <= 0.0f && _sel.animInDur <= 0.0f && _sel.animInCur <= 0.0f)
				_sel.animOutCur	= 0.0f;
				_sel.animOutDur	= animDur;
				_sel.animInCur	= 0.0f;
				_sel.animInDur	= animDur;
			}
		}
	}
}

void AUSceneBarBtm::updateTagNumber(const ENLobbyStack iStack, const UI32 value){
	if(value == 0){
		this->updateTagText(iStack, "");
	} else {
		STNBString str;
		NBString_init(&str);
		NBString_concatUI32(&str, value);
		this->updateTagText(iStack, str.str);
		NBString_release(&str);
	}
}

void AUSceneBarBtm::updateTagText(const ENLobbyStack iStack, const char* value){
	if(iStack >= 0 && iStack <= ENLobbyStack_Count){
		NBString_set(&_btns[iStack].tagValue, value);
		_btns[iStack].tag->updateTag(value);
		this->privOrganizeTag(iStack);
	}
}

void AUSceneBarBtm::updateStyle(const BOOL havePatients, const BOOL organize){
	//Hide all
	{
		SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
			_btns[i].hidden = TRUE;
		}
	}
	//Apply
	{
		if(havePatients){
			_btns[ENLobbyStack_Home].hidden = FALSE; 
			_btns[ENLobbyStack_History].hidden = FALSE;
		}
	}
	//Apply all
	{
		SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
			_btns[i].lyr->setVisibleAndAwake(!_btns[i].hidden);
		}
	}
	//
	if(organize){
		this->privOrganizarContenido(_anchoParaContenido);
	}
}

//

AUEscenaObjeto* AUSceneBarBtm::itemObjetoEscena(){
	return this;
}


STListaItemDatos AUSceneBarBtm::refDatos(){
	STListaItemDatos datos;
	datos.tipo = 0; datos.valor = 0;
	return datos;
}

void AUSceneBarBtm::establecerRefDatos(const SI32 tipo, const SI32 valor){
	//
}

void AUSceneBarBtm::organizarContenido(const float anchoParaContenido, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(anchoParaContenido);
}

void AUSceneBarBtm::privOrganizarContenido(const float anchoParaContenido){
	const float margenHOut	= 0.0f;
	float margenSup			= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.05f);
	float margenInf			= 0.0f;
	float altoMaximo		= 0.0f;
	float maxWidthIcon		= 0.0f;
	UI32 countVisible		= 0;
	//Calculate max height and max-width with texts
	{
		SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
			if(_btns[i].lyr->visible()){
				const NBCajaAABB boxIco = _btns[i].icon->cajaAABBLocal();
				if(altoMaximo < (boxIco.yMax - boxIco.yMin)) altoMaximo = (boxIco.yMax - boxIco.yMin);
				if(_btns[i].text == NULL){
					const float ww = (boxIco.xMax - boxIco.xMin);
					if(maxWidthIcon < ww) maxWidthIcon = ww;
				} else {
					const NBCajaAABB boxTxt = _btns[i].text->cajaAABBLocal();
					const float ww = (boxIco.xMax - boxIco.xMin) + _marginIn + (boxTxt.xMax - boxTxt.xMin);
					if(maxWidthIcon < ww) maxWidthIcon = ww;
				}
				countVisible++;
			}
		}
	}
	//Ubicar y colorear iconos
	if(countVisible > 0){
		const float wPerIcon	= (anchoParaContenido - margenHOut - margenHOut) / (float)countVisible;
		const STNBColor8 colorG	= TSColors::colorDef(ENTSColor_BarBtnIcoUnsel)->normal;
		const STNBColor8 colorR	= TSColors::colorDef(ENTSColor_BarBtnIcoSel)->normal;
		const NBCajaAABB selBox	= _sel.bg->cajaAABBLocal();
		if(wPerIcon < maxWidthIcon){
			margenSup	= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.05f);
			margenInf	= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.01f);
			{
				//Icon and small text below
				SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
					if(_btns[i].lyr->visible()){
						const NBCajaAABB boxIco = _btns[i].icon->cajaAABBLocal();
						const float xCenter	= margenHOut + (wPerIcon * (0.5f + (float)i));
						float icoTxtHeight	= (boxIco.yMax - boxIco.yMin);
						float yBtm			= _altoExtra + margenInf;
						if(_btns[i].textMini != NULL){
							const NBCajaAABB boxTxt		= _btns[i].textMini->cajaAABBLocal();
							_btns[i].textMiniPosUnsel.x	= xCenter - boxTxt.xMin - ((boxTxt.xMax - boxTxt.xMin) * 0.5f);
							_btns[i].textMiniPosUnsel.y	= yBtm - boxTxt.yMin; NBASSERT(_btns[i].textMiniPosUnsel.y == _btns[i].textMiniPosUnsel.y)
							_btns[i].textMini->setVisibleAndAwake(TRUE);
							_btns[i].textMini->establecerTraslacion(_btns[i].textMiniPosUnsel.x, _btns[i].textMiniPosUnsel.y);
							if(i == _sel.curIdx){
								_btns[i].textMini->establecerMultiplicadorColor8(colorR.r, colorR.g, colorR.b, colorR.a);
							} else {
								_btns[i].textMini->establecerMultiplicadorColor8(colorG.r, colorG.g, colorG.b, colorG.a);
							}
							yBtm += (boxTxt.yMax - boxTxt.yMin);
							icoTxtHeight += (boxTxt.yMax - boxTxt.yMin);
						}
						//
						_btns[i].posUnsel.x	= xCenter - boxIco.xMin - ((boxIco.xMax - boxIco.xMin) * 0.5f);
						_btns[i].posUnsel.y	= yBtm - boxIco.yMin;
						_btns[i].posSel.x	= _btns[i].posUnsel.x;
						_btns[i].posSel.y	= _btns[i].posUnsel.y + ((selBox.yMax - selBox.yMin) * 0.15f);
						{
							if(i == _sel.curIdx){
								_btns[i].icon->establecerTraslacion(_btns[i].posSel.x, _btns[i].posSel.y);
								_btns[i].icon->establecerMultiplicadorColor8(colorR.r, colorR.g, colorR.b, colorR.a);
							} else {
								_btns[i].icon->establecerTraslacion(_btns[i].posUnsel.x, _btns[i].posUnsel.y);
								_btns[i].icon->establecerMultiplicadorColor8(colorG.r, colorG.g, colorG.b, colorG.a);
							}
							this->privOrganizeTag((ENLobbyStack)i);
						}
						if(_btns[i].text != NULL){
							_btns[i].text->setVisibleAndAwake(FALSE);
							if(i == _sel.curIdx){
								_btns[i].text->establecerMultiplicadorColor8(colorR.r, colorR.g, colorR.b, colorR.a);
							} else {
								_btns[i].text->establecerMultiplicadorColor8(colorG.r, colorG.g, colorG.b, colorG.a);
							}
						}
						//
						if(altoMaximo < icoTxtHeight) altoMaximo = icoTxtHeight;
					}
				}
			}
		} else {
			margenSup	= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.10f);
			margenInf	= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.10f);
			{
				//Icon and text at the right
				SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
					if(_btns[i].lyr->visible()){
						const NBCajaAABB boxIco = _btns[i].icon->cajaAABBLocal();
						const float xCenter	= margenHOut + (wPerIcon * (0.5f + (float)i));
						float ww = (boxIco.xMax - boxIco.xMin);
						if(_btns[i].text != NULL){
							const NBCajaAABB boxTxt = _btns[i].text->cajaAABBLocal();
							ww += _marginIn + (boxTxt.xMax - boxTxt.xMin);
						}
						_btns[i].posUnsel.x	= xCenter - boxIco.xMin - (ww * 0.5f);
						_btns[i].posUnsel.y	= _altoExtra + margenInf - boxIco.yMin + ((altoMaximo - (boxIco.yMax - boxIco.yMin)) * 0.5f);
						_btns[i].posSel.x	= _btns[i].posUnsel.x;
						_btns[i].posSel.y	= _btns[i].posUnsel.y; //+ ((selBox.yMax - selBox.yMin) * 0.25f);
						{
							if(i == _sel.curIdx){
								_btns[i].icon->establecerTraslacion(_btns[i].posSel.x, _btns[i].posSel.y);
								_btns[i].icon->establecerMultiplicadorColor8(colorR.r, colorR.g, colorR.b, colorR.a);
							} else {
								_btns[i].icon->establecerTraslacion(_btns[i].posUnsel.x, _btns[i].posUnsel.y);
								_btns[i].icon->establecerMultiplicadorColor8(colorG.r, colorG.g, colorG.b, colorG.a);
							}
							this->privOrganizeTag((ENLobbyStack)i);
						}
						if(_btns[i].text != NULL){
							const NBCajaAABB boxTxt = _btns[i].text->cajaAABBLocal();
							_btns[i].text->setVisibleAndAwake(TRUE);
							_btns[i].text->establecerTraslacion(_btns[i].posUnsel.x + boxIco.xMax + _marginIn - boxTxt.xMin, _btns[i].posUnsel.y + boxIco.yMax - (((boxIco.yMax - boxIco.yMin) - (boxTxt.yMax - boxTxt.yMin)) * 0.5f));
							if(i == _sel.curIdx){
								_btns[i].text->establecerMultiplicadorColor8(colorR.r, colorR.g, colorR.b, colorR.a);
							} else {
								_btns[i].text->establecerMultiplicadorColor8(colorG.r, colorG.g, colorG.b, colorG.a);
							}
						}
						if(_btns[i].textMini != NULL){
							_btns[i].textMini->setVisibleAndAwake(FALSE);
							if(i == _sel.curIdx){
								_btns[i].textMini->establecerMultiplicadorColor8(colorR.r, colorR.g, colorR.b, colorR.a);
							} else {
								_btns[i].textMini->establecerMultiplicadorColor8(colorG.r, colorG.g, colorG.b, colorG.a);
							}
						}
					}
				}
			}
		}
	}
	//Ubicar selector
	{
		if(_sel.curIdx >= 0 && _sel.curIdx < ENLobbyStack_Count){
			_sel.bg->setVisibleAndAwake(TRUE);
			_sel.bg->establecerMultiplicadorAlpha8(255);
			_sel.bg->establecerTraslacion(_btns[_sel.curIdx].icon->traslacion());
		} else {
			_sel.bg->setVisibleAndAwake(FALSE);
			_sel.bg->establecerTraslacion(_btns[0].icon->traslacion());
		}
	}
	//Stop animation
	{
		_sel.animOutDur	= 0.0f;
		_sel.animOutCur	= 0.0f;
		_sel.animInDur	= 0.0f;
		_sel.animInCur	= 0.0f;
	}
	//BG and shadow
	{
		const float altoFondo = _altoExtra + margenInf + altoMaximo + margenSup;
		//Bg
		{
			//Vertexs-pos
			_bg.obj->moverVerticeHacia(0, 0.0f, 0.0f);
			_bg.obj->moverVerticeHacia(1, anchoParaContenido, 0.0f);
			_bg.obj->moverVerticeHacia(2, anchoParaContenido, altoFondo);
			_bg.obj->moverVerticeHacia(3, 0.0f, altoFondo);
		}
		//Bg shadow
		{
			const float shadowHeight = NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.02f);
			//Vertexs-pos
			_bg.shadow->moverVerticeHacia(0, 0.0f, altoFondo - 1.0f);
			_bg.shadow->moverVerticeHacia(1, anchoParaContenido, altoFondo - 1.0f);
			_bg.shadow->moverVerticeHacia(2, anchoParaContenido, altoFondo - 1.0f + shadowHeight);
			_bg.shadow->moverVerticeHacia(3, 0.0f, altoFondo - 1.0f + shadowHeight);
			//Limites
			this->establecerLimites(0.0f, anchoParaContenido, 0.0f, altoFondo);
		}
		//Limits
		{
			this->establecerLimites(0.0f, anchoParaContenido, 0.0f, altoFondo);
		}
		NBASSERT(altoFondo >= 0.0f)
	}
	//PRINTF_INFO("Alto: %f.\n", _bg.obj->limites().alto);
	_anchoParaContenido = anchoParaContenido;
}

void AUSceneBarBtm::privOrganizeTag(const ENLobbyStack i){
	TSVisualTag* tag = _btns[i].tag;
	if(_btns[i].tagValue.length <= 0){
		tag->setVisibleAndAwake(FALSE);
	} else {
		tag->setVisibleAndAwake(TRUE);
		{
			const STNBPoint posIco	= _btns[i].posUnsel; //_btns[i].icon->traslacion();
			const NBCajaAABB boxIco	= _btns[i].icon->cajaAABBLocal();
			const NBCajaAABB boxTag	= tag->cajaAABBLocal();
			const float xPos = posIco.x + boxIco.xMax + _marginIn - boxTag.xMax; //+ (((boxIco.xMax - boxIco.xMin) - (boxTag.xMax - boxTag.xMin)) * 0.5f);
			const float yPos = posIco.y + boxIco.yMax - boxTag.yMax + ((boxTag.yMax - boxTag.yMin) * 0.50f);
			tag->establecerTraslacion(xPos, yPos);
		}
	}
}

//

void AUSceneBarBtm::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		if(_sel.animOutDur > 0.0f){
			_sel.animOutCur += segsTranscurridos;
			{
				float rel = _sel.animOutCur / _sel.animOutDur;
				if(rel >= 1.0f){
					_sel.animOutCur	= 0.0f;
					_sel.animOutDur	= 0.0f;
					rel = 1.0f;
				}
				{
					const float rel2 = (rel * rel);
					SI32 curIdxActive = -1;
					//Animate btns
					{
						const STNBColor8 colorG = TSColors::colorDef(ENTSColor_BarBtnIcoUnsel)->normal;
						SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
							//Apply color
							{
								const STNBColor8 c = NBST_P(STNBColor8, 
									(UI8)(_btns[i].colorAnimStart.r + ((colorG.r - _btns[i].colorAnimStart.r) * rel2))
									, (UI8)(_btns[i].colorAnimStart.g + ((colorG.g - _btns[i].colorAnimStart.g) * rel2))
									, (UI8)(_btns[i].colorAnimStart.b + ((colorG.b - _btns[i].colorAnimStart.b) * rel2))
									, (UI8)(_btns[i].colorAnimStart.a + ((colorG.a - _btns[i].colorAnimStart.a) * rel2))
								);
								_btns[i].icon->establecerMultiplicadorColor8(c.r, c.g, c.b, c.a);
								if(_btns[i].text != NULL){
									_btns[i].text->establecerMultiplicadorColor8(c.r, c.g, c.b, c.a);
								}
								if(_btns[i].textMini != NULL){
									_btns[i].textMini->establecerMultiplicadorColor8(c.r, c.g, c.b, c.a);
								}
							}
							//Apply position
							{
								const STNBPoint p = NBST_P(STNBPoint, 
									_btns[i].posAnimStart.x + ((_btns[i].posUnsel.x - _btns[i].posAnimStart.x) * rel2)
									, _btns[i].posAnimStart.y + ((_btns[i].posUnsel.y - _btns[i].posAnimStart.y) * rel2)
								);
								{
									_btns[i].icon->establecerTraslacion(p.x, p.y);
									//this->privOrganizeTag((ENLobbyStack)i); //ToDo: remove
								}
								if(_btns[i].textMini != NULL){
									_btns[i].textMini->establecerTraslacion(_btns[i].textMiniPosUnsel.x + (p.x - _btns[i].posUnsel.x), _btns[i].textMiniPosUnsel.y + (p.y - _btns[i].posUnsel.y));
								}
							}
							//Determine if this one was moving
							if(_btns[i].posAnimStart.x != _btns[i].posUnsel.x || _btns[i].posAnimStart.y != _btns[i].posUnsel.y){
								curIdxActive = i;
							}
						}
					}
					//Animate bg
					if(curIdxActive >= 0 && curIdxActive < ENLobbyStack_Count){
						const STNBPoint posEnd		= NBST_P(STNBPoint, _btns[curIdxActive].posUnsel.x, _btns[curIdxActive].posUnsel.y );
						const STNBPoint posStart	= _btns[curIdxActive].posAnimStart;
						const STNBPoint p = NBST_P(STNBPoint, 
							posStart.x + ((posEnd.x - posStart.x) * rel2)
							, posStart.y + ((posEnd.y - posStart.y) * rel2)
						);
						_sel.bg->establecerTraslacion(p.x, p.y);
					} else {
						const float yDst = _btns[0].posUnsel.y;
						const STNBPoint p = NBST_P(STNBPoint, 
							_bg.posAnimStart.x
							, _bg.posAnimStart.y + ((yDst - _bg.posAnimStart.y) * rel2)
						);
						_sel.bg->establecerTraslacion(p.x, p.y);
					}
				}
			}
		} else if(_sel.animInDur > 0.0f){
			//Set initial positiuon of circle
			if(_sel.animInCur <= 0){
				const BOOL isActive = (_sel.curIdx >= 0 && _sel.curIdx < ENLobbyStack_Count ? TRUE : FALSE);
				if(isActive){
					_sel.bg->establecerTraslacion(_btns[_sel.curIdx].posSel.x, _btns[_sel.curIdx].posSel.y);
				}
				_sel.bg->setVisibleAndAwake(isActive);
			}
			_sel.animInCur += segsTranscurridos;
			{
				float rel = _sel.animInCur / _sel.animInDur;
				if(rel >= 1.0f){
					_sel.animInCur	= 0.0f;
					_sel.animInDur	= 0.0f;
					rel = 1.0f;
				}
				{
					const float relInv = (1.0f - rel);
					const float rel2 = (relInv * relInv);
					//Animate btns
					{
						const STNBColor8 colorG = TSColors::colorDef(ENTSColor_BarBtnIcoUnsel)->normal;
						const STNBColor8 colorR = TSColors::colorDef(ENTSColor_BarBtnIcoSel)->normal;
						SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
							const STNBPoint posEnd		= (i == _sel.curIdx ? _btns[i].posUnsel : _btns[i].posUnsel);
							const STNBPoint posStart	= (i == _sel.curIdx ? _btns[i].posSel : _btns[i].posUnsel);
							const STNBColor8 colorEnd	= (i == _sel.curIdx ? colorG : colorG);
							const STNBColor8 colorStart	= (i == _sel.curIdx ? colorR : colorG);
							//Apply color
							{
								const STNBColor8 c = NBST_P(STNBColor8, 
									(UI8)(colorStart.r + ((colorEnd.r - colorStart.r) * rel2))
									, (UI8)(colorStart.g + ((colorEnd.g - colorStart.g) * rel2))
									, (UI8)(colorStart.b + ((colorEnd.b - colorStart.b) * rel2))
									, (UI8)(colorStart.a + ((colorEnd.a - colorStart.a) * rel2))
								);
								_btns[i].icon->establecerMultiplicadorColor8(c.r, c.g, c.b, c.a);
								if(_btns[i].text != NULL){
									_btns[i].text->establecerMultiplicadorColor8(c.r, c.g, c.b, c.a);
								}
								if(_btns[i].textMini != NULL){
									_btns[i].textMini->establecerMultiplicadorColor8(c.r, c.g, c.b, c.a);
								}
							}
							//Apply position
							{
								const STNBPoint p = NBST_P(STNBPoint, 
									posStart.x + ((posEnd.x - posStart.x) * rel2)
									, posStart.y + ((posEnd.y - posStart.y) * rel2)
								);
								{
									_btns[i].icon->establecerTraslacion(p.x, p.y);
									//this->privOrganizeTag((ENLobbyStack)i); //ToDo: remove
								}
								if(_btns[i].textMini != NULL){
									_btns[i].textMini->establecerTraslacion(_btns[i].textMiniPosUnsel.x + (p.x - _btns[i].posUnsel.x), _btns[i].textMiniPosUnsel.y + (p.y - _btns[i].posUnsel.y));
								}
							}
						}
					}
					//Animate bg
					if(_sel.curIdx >= 0 && _sel.curIdx < ENLobbyStack_Count){
						const STNBPoint posEnd		= NBST_P(STNBPoint, _btns[_sel.curIdx].posUnsel.x, _btns[_sel.curIdx].posUnsel.y );
						const STNBPoint posStart	= NBST_P(STNBPoint, _btns[_sel.curIdx].posSel.x, _btns[_sel.curIdx].posSel.y );
						const STNBPoint p = NBST_P(STNBPoint, 
							posStart.x + ((posEnd.x - posStart.x) * rel2)
							, posStart.y + ((posEnd.y - posStart.y) * rel2)
						);
						_sel.bg->establecerTraslacion(p.x, p.y);
					}
				}
			}
		}
	}
}

UI32 AUSceneBarBtm::stacksVisibleCount(){
	UI32 r = 0;
	SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
		if(_btns[i].lyr->visible()){
			r++;
		}
	}
	return r;
}

UI32 AUSceneBarBtm::stacksVisibleIdxToGlobalIdx(const UI32 visibleIdx){
	UI32 r = 0, iVisible = 0;
	SI32 i; for(i = 0; i < ENLobbyStack_Count; i++){
		if(_btns[i].lyr->visible()){
			if(iVisible == visibleIdx){
				r = i;
				break;
			}
			iVisible++;
		}
	}
	return r;
}

//TOUCHES

void AUSceneBarBtm::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	if(objeto == _bg.obj){
		//Only if first touch
		if(_touch.first == NULL){
			const UI32 colsCount		= this->stacksVisibleCount();
			if(colsCount > 0){
				const NBCajaAABB box	= _bg.obj->cajaAABBLocal();
				const NBPunto posLocal	= _bg.obj->coordenadaEscenaALocal(posTouchEscena.x, posTouchEscena.y);
				const SI32 wPerCol		= (box.xMax - box.xMin) / colsCount;
				const SI32 iRel			= (posLocal.x - box.xMin) / wPerCol;
				_touch.first			= touch;
				_touch.iSel				= (ENLobbyStack)this->stacksVisibleIdxToGlobalIdx(iRel < 0 ? 0 : iRel >= colsCount ? colsCount - 1 : iRel);
				NBASSERT(_touch.iSel >= 0 && _touch.iSel < ENLobbyStack_Count)
				_btns[_touch.iSel].lyr->establecerMultiplicadorColor8(200, 200, 200, 255);
			}
		}
	}
}

void AUSceneBarBtm::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(objeto == _bg.obj){
		//Only if first touch
		if(_touch.first == touch){
			const UI32 colsCount		= this->stacksVisibleCount();
			if(colsCount > 0){
				const NBCajaAABB box	= _bg.obj->cajaAABBLocal();
				const NBPunto posLocal	= _bg.obj->coordenadaEscenaALocal(posActualEscena.x, posActualEscena.y);
				const SI32 wPerCol		= (box.xMax - box.xMin) / colsCount;
				const SI32 iRel			= (posLocal.x - box.xMin) / wPerCol;
				const SI32 iSel			= this->stacksVisibleIdxToGlobalIdx(iRel < 0 ? 0 : iRel >= colsCount ? colsCount - 1 : iRel);
				NBASSERT(iSel >= 0 && iSel < ENLobbyStack_Count)
				if(_touch.iSel != iSel || posLocal.x < box.xMin || posLocal.x > box.xMax || posLocal.y < box.yMin || posLocal.y > box.yMax){
					_btns[_touch.iSel].lyr->establecerMultiplicadorColor8(255, 255, 255, 255);
					_touch.first	= NULL;
					_touch.iSel		= ENLobbyStack_Count;
				}
			}
		}
	}
}

void AUSceneBarBtm::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(objeto == _bg.obj){
		//Only if first touch
		if(_touch.first == touch){
			if(_touch.iSel >= 0 && _touch.iSel < ENLobbyStack_Count){
				_btns[_touch.iSel].lyr->establecerMultiplicadorColor8(255, 255, 255, 255);
				if(_root.lobby != NULL){
					_root.lobby->lobbyFocusStack(_touch.iSel, TRUE); //allowPopAllAction
				}
			}
			_touch.first	= NULL;
			_touch.iSel		= ENLobbyStack_Count;
		}
	}
	//
	{
		const NBCajaAABB cajaEscena = objeto->cajaAABBEnEscena();
		if(NBCAJAAABB_AREA_INTERNA_INTERSECTA_PUNTO(cajaEscena, posActualEscena.x, posActualEscena.y)){
			//
		}
	}
}

//

AUOBJMETODOS_CLASESID_UNICLASE(AUSceneBarBtm)
AUOBJMETODOS_CLASESNOMBRES_UNICLASE(AUSceneBarBtm, "AUSceneBarBtm")
AUOBJMETODOS_CLONAR_NULL(AUSceneBarBtm)

