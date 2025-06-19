//
//  AUSceneBarTop.cpp
//  app-refranero
//
//  Created by Marcos Ortega on 14/03/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneBarTop.h"
#include "visual/TSColors.h"
#include "visual/TSFonts.h"
//
#define AULIBRO_ALTO_SEPARACION_BARRA_ESTADO_H 0.10f
#define AULIBRO_ALTO_SEPARACION_BARRA_ESTADO_V 0.10f
//
#define AULIBRO_SEGS_ESPERA_PARA_BUSCAR 0.5f
//
AUSceneBarTop::AUSceneBarTop(const STLobbyRoot* root, const float width, const float extraHeight)
: AUEscenaContenedorLimitado()
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneBarTop")
	_root			= *root;
	_mrgns.left		= _mrgns.right = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, AULIBRO_ALTO_SEPARACION_BARRA_ESTADO_H);
	_mrgns.top		= _mrgns.bottom = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, AULIBRO_ALTO_SEPARACION_BARRA_ESTADO_V);
	_extraHeight	= 0.0f;
	_width			= width;
	{
		NBMemory_setZero(_curStatusDef);
		_curStatusDef.rightIconMask = ENSceneBarIconBit_All;
	}
	//Substatus
	{
		NBMemory_setZero(_substatus);
		//lyr
		{
			_substatus.lyr	= new(this) AUEscenaContenedorLimitado();
			_substatus.lyr->establecerEscuchadorTouches(this, this);
			_substatus.lyr->setVisibleAndAwake(_substatus.alphaRel > 0.0f ? TRUE : FALSE);
			this->agregarObjetoEscena(_substatus.lyr);
		}
		//Color
		{
			_substatus.bgColor	= NBST_P(STNBColor8, 255, 255, 255, 255 );
			_substatus.fgColor	= NBST_P(STNBColor8, 0, 0, 0, 255 );
			_substatus.show		= FALSE;
		}
		//Bg
		{
			_substatus.bg = new(this) AUEscenaSprite();
			_substatus.bg->establecerMultiplicadorColor8(_substatus.bgColor.r, _substatus.bgColor.g, _substatus.bgColor.b, _substatus.bgColor.a);
			_substatus.lyr->agregarObjetoEscena(_substatus.bg);
		}
		//Text
		{
			NBString_init(&_substatus.textValue);
			_substatus.text = new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentMid));
			_substatus.text->establecerMultiplicadorColor8(_substatus.fgColor.r, _substatus.fgColor.g, _substatus.fgColor.b, _substatus.fgColor.a);
			_substatus.lyr->agregarObjetoEscena(_substatus.text);
		}
		//Icons
		{
			NBMemory_setZero(_substatus.icons);
			{
				SI32 i; for(i = 0 ; i < ENSceneSubbarIcon_Count; i++){
					AUEscenaSprite* ico = NULL;
					switch (i) {
						case ENSceneSubbarIcon_Sync:
							ico = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner"));
							break;
#						ifdef NB_CONFIG_INCLUDE_ASSERTS
						default:
							NBASSERT(FALSE);
							break;
#						endif
					}
					if(ico != NULL){
						ico->setVisibleAndAwake(FALSE);
						_substatus.lyr->agregarObjetoEscena(ico);
					}
					_substatus.icons[i] = ico;
				}
				_substatus.icon = ENSceneSubbarIcon_Count;
			}
		}
	}
	//Bg shadow
	{
		_shadow		= new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
		_shadow->establecerModo(ENEscenaFiguraModo_Relleno);
		//Points
		_shadow->agregarCoordenadaLocal(0.0f, 0.0f);
		_shadow->agregarCoordenadaLocal(0.0f, 0.0f);
		_shadow->agregarCoordenadaLocal(0.0f, 0.0f);
		_shadow->agregarCoordenadaLocal(0.0f, 0.0f);
		//Colors
		_shadow->colorearVertice(0, 255, 255, 255, 25);
		_shadow->colorearVertice(1, 255, 255, 255, 25);
		_shadow->colorearVertice(2, 255, 255, 255, 0);
		_shadow->colorearVertice(3, 255, 255, 255, 0);
		//
		_shadow->establecerEscuchadorTouches(this, this);
		_shadow->establecerMultiplicadorColor8(0, 0, 0, 255);
		this->agregarObjetoEscena(_shadow);
	}
	//Bg
	{
		const STNBColor8 color = TSColors::colorDef(ENTSColor_BarBg)->normal;
		const UI8 shadowTone = (UI8)((255 * 97) / 100);
		{
			_bg = new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
			_bg->establecerEscuchadorTouches(this, this);
			_bg->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			_bg->establecerModo(ENEscenaFiguraModo_Relleno);
			//
			_bg->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg->agregarCoordenadaLocal(0.0f, 0.0f);
			//Colors
			_bg->colorearVertice(0, shadowTone, shadowTone, shadowTone, 255);
			_bg->colorearVertice(1, shadowTone, shadowTone, shadowTone, 255);
			_bg->colorearVertice(2, 255, 255, 255, 255);
			_bg->colorearVertice(3, 255, 255, 255, 255);
			//
			this->agregarObjetoEscena(_bg);
		}
		{
			_bg2 = new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
			_bg2->establecerEscuchadorTouches(this, this);
			_bg2->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			_bg2->establecerModo(ENEscenaFiguraModo_Relleno);
			//
			_bg2->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg2->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg2->agregarCoordenadaLocal(0.0f, 0.0f);
			_bg2->agregarCoordenadaLocal(0.0f, 0.0f);
			//Colors
			_bg2->colorearVertice(0, 255, 255, 255, 255);
			_bg2->colorearVertice(1, 255, 255, 255, 255);
			_bg2->colorearVertice(2, shadowTone, shadowTone, shadowTone, 255);
			_bg2->colorearVertice(3, shadowTone, shadowTone, shadowTone, 255);
			//
			this->agregarObjetoEscena(_bg2);
		}
		//
		NBMemory_setZero(_bgBox);
	}
	//Icons by side
	{
		const SI32 count = (sizeof(_iconIdxBySide) / sizeof(_iconIdxBySide[0]));
		NBMemory_setZero(_iconIdxBySide);
		SI32 i; for(i = 0; i < count; i++){
			_iconIdxBySide[i] = ENSceneBarTopIcon_Count;
		}
	}
	//Icons
	{
		const STNBColor8 color = TSColors::colorDef(ENTSColor_MainColor)->normal;
		const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
		NBMemory_setZero(_icons);
		SI32 i; for(i = 0; i < count; i++){
			STSceneBarTopIcon* d = &_icons[i];
			switch(i) {
				case ENSceneBarTopIcon_Back:
					d->icon = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#back"));
					d->side	= ENSceneBarTopSide_Left;
					break;
				case ENSceneBarTopIcon_Home:
					d->icon = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#home"));
					d->side	= ENSceneBarTopSide_Left;
					break;
				//case ENSceneBarTopIcon_Settings:
				//	d->icon = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#form"));
				//	d->side	= ENSceneBarTopSide_Left;
				//	break;
				default:
					NBASSERT(FALSE)
					break;
			}
			d->color = color;
			d->icon->establecerMultiplicadorColor8(d->color.r, d->color.g, d->color.b, d->color.a);
			d->icon->establecerMultiplicadorAlpha8(d->alpha);
			this->agregarObjetoEscena(d->icon);
		}
	}
	//Left
	{
		NBMemory_setZero(_left);
	}
	//Right
	{
		NBMemory_setZero(_right);
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_MainColor)->normal;
			SI32 i; for(i = 0; i < ENSceneBarIcon_Count; i++){
				STSceneBarTopIcon* d = &_right.icons[i];
				d->side	= ENSceneBarTopSide_Right;
				switch(i) {
					case ENSceneBarIcon_Add:
						d->icon = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#add"));
						break;
					case ENSceneBarIcon_Doc:
						d->icon = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#doc"));
						break;
					case ENSceneBarIcon_Info:
						d->icon = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#list"));
						break;
					default:
						NBASSERT(FALSE)
						break;
				}
				d->color = color;
				d->icon->establecerMultiplicadorColor8(d->color.r, d->color.g, d->color.b, d->color.a);
				d->icon->establecerMultiplicadorAlpha8(d->alpha);
				d->icon->setVisibleAndAwake(FALSE);
				this->agregarObjetoEscena(d->icon);
			}
		}
	}
	//Titles
	{
		_titleIdx = 0;
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_Black)->normal;
			const SI32 count = (sizeof(_title) / sizeof(_title[0]));
			NBMemory_setZero(_title);
			SI32 i; for(i = 0; i < count; i++){
				STSceneBarTopTitle* d = &_title[i];
				d->color	= color;
				d->alpha	= (i == _titleIdx ? 255 : 0);
				d->text		= new(this) AUEscenaTexto(TSFonts::font(ENTSFont_ContentTitle));
				d->text->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_FromTop);
				d->text->establecerMultiplicadorColor8(d->color.r, d->color.g, d->color.b, d->color.a);
				d->text->establecerMultiplicadorAlpha8(d->alpha);
				NBString_init(&d->strText);
				this->agregarObjetoEscena(d->text);
			}
		}
	}
	//Touch
	{
		_touch.first	= NULL;
		_touch.side		= ENSceneBarTopSide_Count;
		_touch.iconRight= ENSceneBarIcon_Count;
	}
	//
	this->setExtraHeight(extraHeight);
	this->privOrganizarContenido(width, false, true);
	NBGestorAnimadores::agregarAnimador(this, this);
}

AUSceneBarTop::~AUSceneBarTop(){
	NBGestorAnimadores::quitarAnimador(this);
	//
	if(_bg != NULL) _bg->liberar(NB_RETENEDOR_THIS); _bg = NULL;
	if(_bg2 != NULL) _bg2->liberar(NB_RETENEDOR_THIS); _bg2 = NULL;
	if(_shadow != NULL) _shadow->liberar(NB_RETENEDOR_THIS); _shadow = NULL;
	//Icons right
	{
		SI32 i; for(i = 0; i < ENSceneBarIcon_Count; i++){
			STSceneBarTopIcon* d = &_right.icons[i];
			if(d->icon != NULL) d->icon->liberar(NB_RETENEDOR_THIS); d->icon = NULL;
		}
	}
	//Icons
	{
		const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
		SI32 i; for(i = 0; i < count; i++){
			STSceneBarTopIcon* d = &_icons[i];
			if(d->icon != NULL) d->icon->liberar(NB_RETENEDOR_THIS); d->icon = NULL;
		}
	}
	//Titles
	{
		const SI32 count = (sizeof(_title) / sizeof(_title[0]));
		SI32 i; for(i = 0; i < count; i++){
			STSceneBarTopTitle* d = &_title[i];
			if(d->text != NULL) d->text->liberar(NB_RETENEDOR_THIS); d->text = NULL;
			NBString_release(&d->strText);
		}
	}
	//Status
	{
		NBStruct_stRelease(DRSceneBarStatus_getSharedStructMap(), &_curStatusDef, sizeof(_curStatusDef));
	}
	//Substatus
	{
		if(_substatus.lyr != NULL) _substatus.lyr->liberar(NB_RETENEDOR_THIS); _substatus.lyr = NULL;
		if(_substatus.bg != NULL) _substatus.bg->liberar(NB_RETENEDOR_THIS); _substatus.bg = NULL;
		if(_substatus.text != NULL) _substatus.text->liberar(NB_RETENEDOR_THIS); _substatus.text = NULL;
		NBString_release(&_substatus.textValue);
		//Icons
		{
			SI32 i; for(i = 0 ; i < ENSceneSubbarIcon_Count; i++){
				if(_substatus.icons[i] != NULL) _substatus.icons[i]->liberar(NB_RETENEDOR_THIS); _substatus.icons[i] = NULL;
			}
		}
	}
}

//

STNBAABox AUSceneBarTop::bgBox() const {
	return _bgBox;
}

float AUSceneBarTop::extraHeight() const{
	return _extraHeight;
}

void AUSceneBarTop::setExtraHeight(const float extraHeight){
	_extraHeight = extraHeight;
	if(extraHeight <= 0.0f){
		_mrgns.top = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, AULIBRO_ALTO_SEPARACION_BARRA_ESTADO_V);
	} else {
		_mrgns.top = NBGestorEscena::anchoPulgadasAEscena(_root.iScene, AULIBRO_ALTO_SEPARACION_BARRA_ESTADO_V / 3.0f);
	}
}

//Status

UI32 AUSceneBarTop::getStatusRightIconMask() const {
	return _curStatusDef.rightIconMask;
}

BOOL AUSceneBarTop::getStatusDef(STTSSceneBarStatus* dst) const {
	BOOL r = FALSE;
	if(dst != NULL){
		NBStruct_stRelease(DRSceneBarStatus_getSharedStructMap(), dst, sizeof(*dst));
		NBStruct_stClone(DRSceneBarStatus_getSharedStructMap(), &_curStatusDef, sizeof(_curStatusDef), dst, sizeof(*dst));
	}
	return r;
}

void AUSceneBarTop::setStatusDef(const STTSSceneBarStatus statusDef){
	const BOOL titleChanged = (!NBString_strIsEqual(_curStatusDef.title, statusDef.title));
	const BOOL rightIconMaskChanged = (_curStatusDef.rightIconMask != statusDef.rightIconMask);
	BOOL leftIconChanged = FALSE;
	//Apply
	{
		{
			NBStruct_stRelease(DRSceneBarStatus_getSharedStructMap(), &_curStatusDef, sizeof(_curStatusDef));
			NBStruct_stClone(DRSceneBarStatus_getSharedStructMap(), &statusDef, sizeof(statusDef), &_curStatusDef, sizeof(_curStatusDef));
		}
		//Left icon
		{
			if(statusDef.leftIconId >= 0 && statusDef.leftIconId <= ENSceneBarTopIcon_Count){
				if(_iconIdxBySide[ENSceneBarTopSide_Left] != (ENSceneBarTopIcon)statusDef.leftIconId){
					_iconIdxBySide[ENSceneBarTopSide_Left] = (ENSceneBarTopIcon)statusDef.leftIconId;
					leftIconChanged = TRUE;
				}
			}
		}
		if(titleChanged){
			const SI32 other = (_titleIdx == 0 ? 1: 0);
			_titleIdx = other;
			NBString_set(&_title[other].strText, statusDef.title);
			_title[other].text->establecerTexto(statusDef.title);
		}
	}
	//Update visuals
	if(rightIconMaskChanged){
		this->privOrganizarContenido(_width, FALSE, TRUE);
	} else {
		if(titleChanged){
			this->privCropTitle(_titleIdx);
		}
	}
}

//Substatus

BOOL AUSceneBarTop::isSubstatusVisible() const {
	return _substatus.show;
}

void AUSceneBarTop::setSubstatusVisible(const BOOL visible){
	_substatus.show = visible;
}

void AUSceneBarTop::setSubstatus(const ENSceneSubbarIcon icon, const char* text, const STNBColor8 bgColor, const STNBColor8 fgColor){
	BOOL organize = FALSE;
	_substatus.bgColor = bgColor;
	_substatus.fgColor = fgColor;
	if(_substatus.bg != NULL){
		_substatus.bg->establecerMultiplicadorColor8(_substatus.bgColor.r, _substatus.bgColor.g, _substatus.bgColor.b, _substatus.bgColor.a);
	}
	if(_substatus.text != NULL){
		_substatus.text->establecerMultiplicadorColor8(_substatus.fgColor.r, _substatus.fgColor.g, _substatus.fgColor.b, _substatus.fgColor.a);
	}
	if(!NBString_isEqual(&_substatus.textValue, text)){
		NBString_set(&_substatus.textValue, text);
		if(_substatus.text != NULL){
			_substatus.text->establecerTexto(text);
		}
		organize = TRUE;
	}
	if(_substatus.icon != icon){
		//Disable previous
		if(_substatus.icon < ENSceneSubbarIcon_Count){
			if(_substatus.icons[_substatus.icon] != NULL){
				_substatus.icons[_substatus.icon]->setVisibleAndAwake(FALSE);
			}
		}
		//Enable new
		if(icon < ENSceneSubbarIcon_Count){
			if(_substatus.icons[icon] != NULL){
				_substatus.icons[icon]->setVisibleAndAwake(TRUE);
			}
		}
		_substatus.icon = icon;
		organize = TRUE;
	}
	//Organize
	if(organize){
		this->privOrganizarContenido(_width, FALSE, FALSE);
	}
}

//

AUEscenaObjeto* AUSceneBarTop::itemObjetoEscena(){
	return this;
}

STListaItemDatos AUSceneBarTop::refDatos(){
	STListaItemDatos datos;
	datos.tipo = 0; datos.valor = 0;
	return datos;
}

void AUSceneBarTop::establecerRefDatos(const SI32 tipo, const SI32 valor){
	//
}

void AUSceneBarTop::organizarContenido(const float width, const float height, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(width, FALSE && true, false);
}

void AUSceneBarTop::privRightCalculateIconsPos(){
	const float marginI	= NBGestorEscena::anchoPulgadasAEscena(_root.iScene, 0.10f);
	float xRight		= _right.xRight;
	SI32 rightIconMask	= _curStatusDef.rightIconMask;
	//
	{
		SI32 i; for(i = 0; i < ENSceneBarIcon_Count; i++){
			STSceneBarTopIcon* d	= &_right.icons[i];
			const NBCajaAABB box	= d->icon->cajaAABBLocal();
			float myXRight			= xRight;
			if((rightIconMask & (0x1 << i)) == 0){
				//Not visible
				myXRight = _right.xRight;
			} else {
				//Visible
				if(xRight != _right.xRight) xRight -= marginI;
				myXRight = xRight;
				xRight -= (box.xMax - box.xMin);
			}
			//
			{
				d->posIn.x	= myXRight - box.xMax;
				d->posIn.y	= _right.yTop - box.yMax - ((_right.height - (box.yMax - box.yMin)) * 0.5f);
				d->posOut	= d->posIn;
				if(!d->icon->visible() || d->icon->multiplicadorAlpha8() <= 0){
					d->icon->establecerTraslacion(d->posOut.x, d->posOut.y);
				}
			}
			//Touch area
			d->xLeftTouch	= d->posIn.x + box.xMin;
		}
	}
	_right.xLeft = xRight;
}

void AUSceneBarTop::privOrganizarContenido(const float width, const bool animar, const bool forzarPosicion){
	float maxHeight = 0.0f;
	float baseHeight = 0.0f;
	//Left
	{
		_left.xLeft		= _mrgns.left;
		_left.xRight	= _mrgns.left;
	}
	//Calculate max height
	{
		//Icons
		{
			const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
			SI32 i; for(i = 0; i < count; i++){
				STSceneBarTopIcon* d = &_icons[i];
				if(d->icon != NULL){
					const NBCajaAABB box = d->icon->cajaAABBLocal();
					const float height = (box.yMax - box.yMin);
					if(maxHeight < height) maxHeight = height;
				}
			}
		}
		//Titles
		{
			const SI32 count = (sizeof(_title) / sizeof(_title[0]));
			SI32 i; for(i = 0; i < count; i++){
				STSceneBarTopTitle* d = &_title[i];
				if(d->text != NULL){
					AUFuenteRender* fnt = d->text->fuenteRender();
					const float height = fnt->ascendenteEscena() - fnt->descendenteEscena();
					if(maxHeight < height) maxHeight = height;
				}
			}
		}
		baseHeight = maxHeight + _mrgns.top + _mrgns.bottom;
	}
	//Icons right
	{
		_right.xLeft	= width - _mrgns.right;
		_right.xRight	= width - _mrgns.right;
		_right.yTop		= 0.0f;
		_right.height	= baseHeight;
		this->privRightCalculateIconsPos();
	}
	//Icons
	{
		const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
		SI32 i; for(i = 0; i < count; i++){
			STSceneBarTopIcon* d = &_icons[i];
			if(d->icon != NULL){
				const NBCajaAABB box = d->icon->cajaAABBLocal();
				const float relX = (d->side == ENSceneBarTopSide_Right ? 1.0f : d->side == ENSceneBarTopSide_Center ? 0.5f : 0.0f);
				d->posIn.x	= _left.xLeft - box.xMin + (((width - _mrgns.left - _mrgns.right) - (box.yMax - box.yMin)) * relX);
				d->posIn.y	= - box.yMax - ((baseHeight - (box.yMax - box.yMin)) * 0.5f);
				d->posOut.x	= d->posIn.x + (d->side == ENSceneBarTopSide_Right ? _mrgns.right : d->side == ENSceneBarTopSide_Center ? 0.0f : -_mrgns.left);
				d->posOut.y	= d->posIn.y - (d->side == ENSceneBarTopSide_Center ? _mrgns.top : 0.0f);
				if(relX == 0.0f){
					if(_left.xRight < (d->posIn.x + box.xMax)){
						_left.xRight = (d->posIn.x + box.xMax);
					}
				}
			}
		}
	}
	//Titles
	{
		const SI32 count = (sizeof(_title) / sizeof(_title[0]));
		SI32 i; for(i = 0; i < count; i++){
			STSceneBarTopTitle* d = &_title[i];
			if(d->text != NULL){
				AUFuenteRender* fnt = d->text->fuenteRender();
				const float height = fnt->ascendenteEscena() - fnt->descendenteEscena();
				d->posIn.x	= _left.xRight + ((_right.xLeft - _left.xRight) * 0.5f);
				d->posIn.y	= -((baseHeight - height) * 0.5f);
				d->posOut.x	= d->posIn.x;
				d->posOut.y	= d->posIn.y + _mrgns.top;
			}
		}
		//Crop titles
		this->privCropTitles();
	}
	//Bg
	{
		_bgBox.xMin	= 0.0f;
		_bgBox.xMax = width;
		_bgBox.yMin = -baseHeight;
		_bgBox.yMax	= _extraHeight;
		{
			const float sepPoint	= _bgBox.yMin + ((_bgBox.yMax - _bgBox.yMin) * 0.3f);
			//
			_bg->moverVerticeHacia(0, 0.0f, sepPoint);
			_bg->moverVerticeHacia(1, width, sepPoint);
			_bg->moverVerticeHacia(2, width, _bgBox.yMax);
			_bg->moverVerticeHacia(3, 0.0f, _bgBox.yMax);
			//
			_bg2->moverVerticeHacia(0, 0.0f, _bgBox.yMin);
			_bg2->moverVerticeHacia(1, width, _bgBox.yMin);
			_bg2->moverVerticeHacia(2, width, sepPoint);
			_bg2->moverVerticeHacia(3, 0.0f, sepPoint);
		}
	}
	//Substatus
	{
		const float marginV = NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.01f);
		const float marginH = NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.03f);
		const float yTopStart = 0.0f - marginV;
		float yTop = yTopStart, xLeftText = 0.0f;
		//Text
		if(_substatus.text != NULL){
			_substatus.text->organizarTexto(width - marginH - marginH);
			{
				const NBCajaAABB box = _substatus.text->cajaAABBLocal();
				const STNBPoint pos = NBST_P(STNBPoint, 0.0f - box.xMin + ((width - (box.xMax - box.xMin)) * 0.5f), yTop - box.yMax );
				_substatus.text->establecerTraslacion(pos.x, pos.y);
				yTop -= (box.yMax - box.yMin);
				xLeftText = pos.x + box.xMin;
			}
		}
		//Icon (left of text)
		if(_substatus.icon < ENSceneSubbarIcon_Count){
			AUEscenaSprite* ico = _substatus.icons[_substatus.icon];
			if(ico != NULL){
				if(yTop == yTopStart){
					ico->setVisibleAndAwake(FALSE);
				} else {
					const NBCajaAABB box = ico->cajaAABBLocal();
					const float heightTxt = (yTopStart - yTop);
					const float scale = (heightTxt / (box.yMax - box.yMin)); NBASSERT(scale >= 0.0f)
					ico->setVisibleAndAwake(TRUE);
					ico->establecerEscalacion(scale);
					ico->establecerTraslacion(xLeftText - marginH - (box.xMax * scale), yTopStart - (box.yMax * scale) - ((heightTxt - ((box.yMax - box.yMin) * scale)) * 0.5f));
				}
			}
		}
		yTop -= marginV;
		//Bg
		if(_substatus.bg != NULL){
			_substatus.bg->redimensionar(0.0f, 0.0f, width, yTop);
		}
		//
		_substatus.bgBox.xMin	= 0.0f;
		_substatus.bgBox.xMax	= width;
		_substatus.bgBox.yMax	= 0.0f;
		_substatus.bgBox.yMin	= yTop;
		//Layer
		if(_substatus.lyr != NULL){
			_substatus.lyr->establecerLimites(0.0f, width, yTop, 0.0f);
			{
				_substatus.lyr->setVisibleAndAwake(_substatus.alphaRel > 0.0f ? TRUE : FALSE);
				_substatus.lyr->establecerMultiplicadorAlpha8((UI8)(_substatus.alphaRel * 255.0f));
				_substatus.lyr->establecerTraslacion(0.0f, _bgBox.yMin - _substatus.bgBox.yMax + ((1.0f - _substatus.alphaRel) * (_substatus.bgBox.yMax - _substatus.bgBox.yMin)));
			}
		}
	}
	{
		const float yBtm = _bgBox.yMin - (_substatus.alphaRel >= 1.0f ? (_substatus.bgBox.yMax - _substatus.bgBox.yMin) : 0.0f);
		//Bg shadow
		{
			const float shadowHeight = NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.02f);
			//Vertexs-pos
			_shadow->moverVerticeHacia(0, 0.0f, yBtm + 1.0f);
			_shadow->moverVerticeHacia(1, width, yBtm + 1.0f);
			_shadow->moverVerticeHacia(2, width, yBtm + 1.0f - shadowHeight);
			_shadow->moverVerticeHacia(3, 0.0f, yBtm + 1.0f - shadowHeight);
		}
		//Limits
		{
			this->establecerLimites(0.0f, width, yBtm, _bgBox.yMax);
		}
	}
	_width = width;
}

//Title

void AUSceneBarTop::privCropTitles(){
	const SI32 count = (sizeof(_title) / sizeof(_title[0]));
	SI32 i; for(i = 0; i < count; i++){
		this->privCropTitle(i);
	}
}

void AUSceneBarTop::privCropTitle(const UI32 iTitle){
	const float widthAvail = (_right.xLeft - _left.xRight);
	const SI32 count = (sizeof(_title) / sizeof(_title[0]));
	if(iTitle >= 0 && iTitle < count){
		STSceneBarTopTitle* d = &_title[iTitle];
		if(d->text != NULL){
			float dotsWidth = 0.0f;
			//Calculate "..." width
			{
				d->text->establecerTexto("...");
				{
					const STNBTextMetrics* m = d->text->textMetrics();
					if(m->chars.use > 0){
						const STNBTextMetricsChar* frst = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, 0);
						const STNBTextMetricsChar* last = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, (m->chars.use - 1));
						dotsWidth = (last->pos.x + last->extendsRight) - (frst->pos.x - frst->extendsLeft);
						if(dotsWidth < 0.0f) dotsWidth = 0.0f;
					}
				}
			}
			//Set full title
			{
				d->text->establecerTexto(d->strText.str);
				{
					const STNBTextMetrics* m = d->text->textMetrics();
					if(m->chars.use > 3){
						const STNBTextMetricsChar* first = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, 0);
						const STNBTextMetricsChar* last = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, (m->chars.use - 1));
						const float fullWidth = (last->pos.x + last->extendsRight) - (first->pos.x - first->extendsLeft);
						if(fullWidth <= widthAvail){
							//Leave as full text
						} else {
							//Limit to visible area
							SI32 countLeft = 0, countRight = 0;
							BOOL advanceRight = TRUE;
							while((countLeft + countRight) < m->chars.use){
								if(advanceRight){
									countRight++;
								} else {
									countLeft++;
								}
								//Calculate widths
								{
									float leftW = 0.0f, rightW = 0.0f;
									if(countLeft > 0){
										const STNBTextMetricsChar* leftLimit = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, countLeft - 1);
										leftW = (leftLimit->pos.x + leftLimit->extendsRight) - (first->pos.x - first->extendsLeft);
									}
									if(countRight > 0){
										const STNBTextMetricsChar* rightLimit = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, (m->chars.use - countRight));
										rightW = (last->pos.x + last->extendsRight) - (rightLimit->pos.x - rightLimit->extendsLeft);
									}
									if((leftW + dotsWidth + rightW) > widthAvail){
										if(advanceRight){
											countRight--;
										} else {
											countLeft--;
										}
										break;
									}
								}
								advanceRight = !advanceRight;
							}
							//Build string
							{
								STNBString str;
								NBString_init(&str);
								if(countLeft > 0){
									const STNBTextMetricsChar* leftLimit = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, countLeft - 1);
									NBString_concatBytes(&str, d->strText.str, leftLimit->iByte + leftLimit->bytesLen);
								}
								NBString_concat(&str, "...");
								if(countRight > 0){
									const STNBTextMetricsChar* rightLimit = NBArray_itmPtrAtIndex(&m->chars, STNBTextMetricsChar, (m->chars.use - countRight));
									NBString_concat(&str, &d->strText.str[rightLimit->iByte]);
								}
								d->text->establecerTexto(str.str);
								NBString_release(&str);
							}
						}
					}
				}
			}
		}
	}
}

//
void AUSceneBarTop::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		//Animate icons
		{
			//Icons
			{
				const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
				SI32 i; for(i = 0; i < count; i++){
					STSceneBarTopIcon* d = &_icons[i];
					if(_iconIdxBySide[d->side] == i){
						d->alpha += 1024.0 * segsTranscurridos;
						if(d->alpha > 255.0f) d->alpha = 255.0f;
					} else {
						d->alpha -= 1024.0 * segsTranscurridos;
						if(d->alpha <= 0.0f) d->alpha = 0.0f;
					}
					d->icon->setVisibleAndAwake(d->alpha > 0.0f);
					d->icon->establecerMultiplicadorAlpha8(d->alpha);
					d->icon->establecerTraslacion(d->posOut.x + ((d->posIn.x - d->posOut.x) * (d->alpha / 255.0f)), d->posOut.y + ((d->posIn.y - d->posOut.y) * (d->alpha / 255.0f)));
				}
			}
		}
		//Right side icons
		{
			SI32 rightIconMask	= _curStatusDef.rightIconMask;
			//
			{
				SI32 i; for(i = 0; i < ENSceneBarIcon_Count; i++){
					STSceneBarTopIcon* d	= &_right.icons[i];
					if((rightIconMask & (0x1 << i)) == 0){
						//Not visible
						d->alpha -= 1024.0 * segsTranscurridos;
						if(d->alpha <= 0.0f) d->alpha = 0.0f;
					} else {
						//Visible
						d->alpha += 1024.0 * segsTranscurridos;
						if(d->alpha > 255.0f) d->alpha = 255.0f;
					}
					d->icon->setVisibleAndAwake(d->alpha > 0.0f);
					d->icon->establecerMultiplicadorAlpha8(d->alpha);
					d->icon->establecerTraslacion(d->posOut.x + ((d->posIn.x - d->posOut.x) * (d->alpha / 255.0f)), d->posOut.y + ((d->posIn.y - d->posOut.y) * (d->alpha / 255.0f)));
				}
			}
		}
		//Animate titles
		{
			const SI32 count = (sizeof(_title) / sizeof(_title[0]));
			SI32 i; for(i = 0; i < count; i++){
				STSceneBarTopTitle* d = &_title[i];
				if(_titleIdx == i){
					d->alpha += 1024.0 * segsTranscurridos;
					if(d->alpha > 255.0f) d->alpha = 255.0f;
				} else {
					d->alpha -= 1024.0 * segsTranscurridos;
					if(d->alpha <= 0.0f) d->alpha = 0.0f;
				}
				d->text->setVisibleAndAwake(d->alpha > 0.0f);
				d->text->establecerMultiplicadorAlpha8(d->alpha);
				d->text->establecerTraslacion(d->posOut.x + ((d->posIn.x - d->posOut.x) * (d->alpha / 255.0f)), d->posOut.y + ((d->posIn.y - d->posOut.y) * (d->alpha / 255.0f)));
			}
		}
		//Animate substatus
		{
			if(_substatus.show){
				_substatus.alphaRel += 4.0f * segsTranscurridos;
			} else {
				_substatus.alphaRel -= 4.0f * segsTranscurridos;
			}
			//Limit
			{
				if(_substatus.alphaRel > 1.0f) _substatus.alphaRel = 1.0f;
				if(_substatus.alphaRel < 0.0f) _substatus.alphaRel = 0.0f;
			}
			//Apply
			{
				const float yBtm = _bgBox.yMin - (_substatus.alphaRel >= 1.0f ? (_substatus.bgBox.yMax - _substatus.bgBox.yMin) : 0.0f);
				this->establecerLimites(0.0f, _width, yBtm, _bgBox.yMax);
				{
					_substatus.lyr->setVisibleAndAwake(_substatus.alphaRel > 0.0f ? TRUE : FALSE);
					_substatus.lyr->establecerMultiplicadorAlpha8((UI8)(_substatus.alphaRel * 255.0f));
					_substatus.lyr->establecerTraslacion(0.0f, _bgBox.yMin - _substatus.bgBox.yMax + ((1.0f - _substatus.alphaRel) * (_substatus.bgBox.yMax - _substatus.bgBox.yMin)));
				}
				//Bg shadow
				{
					const float shadowHeight = NBGestorEscena::altoPulgadasAEscena(_root.iScene, 0.02f);
					//Vertexs-pos
					_shadow->moverVerticeHacia(0, 0.0f, yBtm + 1.0f);
					_shadow->moverVerticeHacia(1, _width, yBtm + 1.0f);
					_shadow->moverVerticeHacia(2, _width, yBtm + 1.0f - shadowHeight);
					_shadow->moverVerticeHacia(3, 0.0f, yBtm + 1.0f - shadowHeight);
				}
			}
			//Icon
			if(_substatus.icon < ENSceneSubbarIcon_Count){
				AUEscenaSprite* ico = _substatus.icons[_substatus.icon];
				if(ico != NULL){
					const SI32 angStep = (360 / 12); 
					_substatus.rot -= (360.0f * segsTranscurridos) ;
					while(_substatus.rot < 0.0f){
						_substatus.rot += 360.0f;
					}
					ico->establecerRotacionNormalizada(((SI32)_substatus.rot / angStep) * angStep);
				}
			}
		}
	}
}

//TOUCHES

void AUSceneBarTop::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	if(_substatus.lyr == objeto){
		//
	} else if(_bg == objeto){
		if(_touch.first == NULL){
			const NBCajaAABB box = _bg->cajaAABBLocal();
			const NBPunto posLcl = _bg->coordenadaEscenaALocal(posTouchEscena.x, posTouchEscena.y);
			const float relX = (posLcl.x - box.xMin) / (box.xMax - box.xMin);
			const ENSceneBarTopSide startSide = (relX < 0.25f ? ENSceneBarTopSide_Left : relX < 0.75f ? ENSceneBarTopSide_Center : ENSceneBarTopSide_Right);
			_touch.first	= touch;
			_touch.side		= startSide;
			//Color icons
			{
				const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
				SI32 i; for(i = 0; i < count; i++){
					STSceneBarTopIcon* d = &_icons[i];
					if(d->side == startSide){
						const UI8 a = d->icon->multiplicadorAlpha8();
						d->icon->establecerMultiplicadorColor8(d->color.r * 200 / 255, d->color.g * 200 / 255, d->color.b * 200 / 255, a);
					}
					
				}
			}
			//Right sides icons
			{
				SI32 rightIconMask	= _curStatusDef.rightIconMask;
				//
				{
					NBASSERT(_touch.iconRight == ENSceneBarIcon_Count)
					SI32 i; for(i = (ENSceneBarIcon_Count - 1); i >= 0 ; i--){
						STSceneBarTopIcon* d = &_right.icons[i];
						if((rightIconMask & (0x1 << i)) != 0){
							if(d->xLeftTouch < posLcl.x){
								_touch.iconRight = (ENSceneBarIcon)i;
							}
						}
					}
					if(_touch.iconRight < ENSceneBarIcon_Count){
						STSceneBarTopIcon* d = &_right.icons[_touch.iconRight];
						const UI8 a = d->icon->multiplicadorAlpha8();
						d->icon->establecerMultiplicadorColor8(d->color.r * 200 / 255, d->color.g * 200 / 255, d->color.b * 200 / 255, a);
					}
				}
			}
			//PRINTF_INFO("Touch started at relX(%f).\n", relX);
		}
	}
}

void AUSceneBarTop::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(_bg == objeto){
		if(_touch.first == touch){
			const NBCajaAABB box = _bg->cajaAABBLocal();
			const NBPunto posLcl = _bg->coordenadaEscenaALocal(posActualEscena.x, posActualEscena.y);
			const float relX = (posLcl.x - box.xMin) / (box.xMax - box.xMin);
			//const ENSceneBarTopSide side = (relX < 0.25f ? ENSceneBarTopSide_Left : relX < 0.75f ? ENSceneBarTopSide_Center : ENSceneBarTopSide_Right);
			//(relX < 0.25f ? ENSceneBarTopSide_Left : relX < 0.75f ? ENSceneBarTopSide_Center : ENSceneBarTopSide_Right);
			//PRINTF_INFO("Touch moved to relX(%f).\n", relX);
		}
	}
}

void AUSceneBarTop::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	BOOL consumed = FALSE;
	if(_substatus.lyr == objeto){
		//
	} else if(_bg == objeto){
		if(_touch.first == touch){
			const NBCajaAABB box = _bg->cajaAABBLocal();
			const NBPunto posLcl = _bg->coordenadaEscenaALocal(posActualEscena.x, posActualEscena.y);
			const float relX = (posLcl.x - box.xMin) / (box.xMax - box.xMin);
			const ENSceneBarTopSide side = (relX < 0.25f ? ENSceneBarTopSide_Left : relX < 0.75f ? ENSceneBarTopSide_Center : ENSceneBarTopSide_Right);
			const ENSceneBarTopSide startSide = _touch.side;
			//
			_touch.first	= NULL;
			_touch.side		= ENSceneBarTopSide_Count;
			//Color icons
			{
				const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
				SI32 i; for(i = 0; i < count; i++){
					STSceneBarTopIcon* d = &_icons[i];
					if(d->side == startSide){
						const UI8 a = d->icon->multiplicadorAlpha8();
						d->icon->establecerMultiplicadorColor8(d->color.r, d->color.g, d->color.b, a);
					}
					
				}
			}
			//Right side icons
			{
				if(_touch.iconRight < ENSceneBarIcon_Count){
					STSceneBarTopIcon* d = &_right.icons[_touch.iconRight];
					const UI8 a = d->icon->multiplicadorAlpha8();
					d->icon->establecerMultiplicadorColor8(d->color.r, d->color.g, d->color.b, a);
					//Determine if touch ender over the same icon
					{
						SI32 rightIconMask	= _curStatusDef.rightIconMask;
						{
							ENSceneBarIcon curOver = ENSceneBarIcon_Count;
							SI32 i; for(i = (ENSceneBarIcon_Count - 1); i >= 0 ; i--){
								STSceneBarTopIcon* d = &_right.icons[i];
								if((rightIconMask & (0x1 << i)) != 0){
									if(d->xLeftTouch < posLcl.x){
										curOver = (ENSceneBarIcon)i;
									}
								}
							}
							{
								const BOOL isOver = (curOver == _touch.iconRight);
								_touch.iconRight = ENSceneBarIcon_Count;
								if(isOver){
									//PRINTF_INFO("Touch ended over icon: %d.\n", curOver);
									if(_root.lobby != NULL){
										_root.lobby->lobbyExecIconCmd(curOver);
									}
									consumed = TRUE;
								}
							}
						}
					}
				}
			}
			//Action
			if(side == startSide){
				const SI32 count = (sizeof(_icons) / sizeof(_icons[0]));
				SI32 i; for(i = 0; i < count; i++){
					STSceneBarTopIcon* d = &_icons[i];
					if(d->side == startSide && _iconIdxBySide[d->side] == i){
						switch(i) {
							case ENSceneBarTopIcon_Back:
								//PRINTF_INFO("ENSceneBarTopIcon_Back.\n");
								if(_root.lobby != NULL){
									_root.lobby->lobbyBack(FALSE);
								}
								break;
							case ENSceneBarTopIcon_Home:
								if(_root.lobby != NULL){
									_root.lobby->lobbyFocusStack(ENLobbyStack_Home, TRUE);
								}
								break;
							//case ENSceneBarTopIcon_Settings:
							//	//PRINTF_INFO("ENSceneBarTopIcon_Settings.\n");
							//	if(_root.lobby != NULL){
							//		_root.lobby->lobbyBack(FALSE);
							//	}
							//	break;
							default:
								NBASSERT(FALSE)
								break;
						}
						consumed = TRUE;
						break;
					}
				}
			}
			//PRINTF_INFO("Touch ended at relX(%f).\n", relX);
		}
	}
	//
	if(!touch->cancelado){
		const NBCajaAABB cajaEscena = objeto->cajaAABBEnEscena();
		if(NBCAJAAABB_INTERSECTA_PUNTO(cajaEscena, posActualEscena.x, posActualEscena.y) && !touch->cancelado){
			if(_substatus.lyr == objeto){
				if(_root.lobby != NULL){
					_root.lobby->lobbyTopBarSubstatusTouched();
				}
			} else if(_bg == objeto){
				if(!consumed){
					if(_root.lobby != NULL){
						_root.lobby->lobbyTopBarTouched();
					}
				}
			}
		}
	}
}

//

AUOBJMETODOS_CLASESID_UNICLASE(AUSceneBarTop)
AUOBJMETODOS_CLASESNOMBRES_UNICLASE(AUSceneBarTop, "AUSceneBarTop")
AUOBJMETODOS_CLONAR_NULL(AUSceneBarTop)

