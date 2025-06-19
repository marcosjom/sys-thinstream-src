//
//  TSVisualStatus.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/gui/TSVisualStatus.h"
#include "visual/TSColors.h"
#include "visual/TSFonts.h"

TSVisualStatus::TSVisualStatus(STTSClient* client, const float width, const float height) : AUEscenaContenedorLimitado() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualStatus")
	//
	NBMemory_setZero(_refDatos);
	_client			= client;
	_anchoUsar		= 0.0f;
	_altoUsar		= 0.0f;
	_marginH		= 0.0f;
	_marginV		= 0.0f;
	//Fonts
	{
		_fntNames 	= TSFonts::font(ENTSFont_ContentTitle);
		_fntNames->retener(NB_RETENEDOR_THIS);
	}
	//Objs
	{
		_objs.mode		= ENTSVisualStatusMode_Count;
		_objs.color		= TSColors::colorDef(ENTSColor_Gray)->normal;
		_objs.alphaRel	= 1.0f;
		_objs.alphaInc	= FALSE;
		//Bg
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_White)->normal;
			_objs.bg = new(this) AUEscenaSprite();
			_objs.bg->setVisibleAndAwake(FALSE);
			_objs.bg->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_objs.bg);
		}
		//Border
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
			_objs.border = new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
			_objs.border->establecerModo(ENEscenaFiguraModo_Borde);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->setVisibleAndAwake(FALSE);
			_objs.border->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_objs.border);
		}
		//
		{
			_objs.ico = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#add"));
			_objs.ico->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b, _objs.color.a);
			this->agregarObjetoEscena(_objs.ico);
		}
		//
		{
			_objs.text = new(this) AUEscenaTexto(_fntNames);
			_objs.text->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_Center);
			_objs.text->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b,_objs.color.a);
			this->agregarObjetoEscena(_objs.text);
		}
	}
	//
	this->privOrganizarContenido(width, height, false, NULL, 0.0f);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}


TSVisualStatus::TSVisualStatus(STTSClient* client, const float width, const float height, const STNBColor8 prefColor){
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualStatus")
	//
	NBMemory_setZero(_refDatos);
	_client			= client;
	_anchoUsar		= 0.0f;
	_altoUsar		= 0.0f;
	_marginH		= 0.0f;
	_marginV		= 0.0f;
	//Fonts
	{
		_fntNames 	= TSFonts::font(ENTSFont_ContentTitle);
		_fntNames->retener(NB_RETENEDOR_THIS);
	}
	//Objs
	{
		_objs.mode		= ENTSVisualStatusMode_Count;
		_objs.color		= prefColor;
		_objs.alphaRel	= 1.0f;
		_objs.alphaInc	= FALSE;
		//Bg
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_White)->normal;
			_objs.bg = new(this) AUEscenaSprite();
			_objs.bg->setVisibleAndAwake(FALSE);
			_objs.bg->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_objs.bg);
		}
		//Border
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
			_objs.border = new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
			_objs.border->establecerModo(ENEscenaFiguraModo_Borde);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->agregarCoordenadaLocal(0.0f, 0.0f);
			_objs.border->setVisibleAndAwake(FALSE);
			_objs.border->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_objs.border);
		}
		//
		{
			_objs.ico = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#add"));
			_objs.ico->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b, _objs.color.a);
			this->agregarObjetoEscena(_objs.ico);
		}
		//
		{
			_objs.text = new(this) AUEscenaTexto(_fntNames);
			_objs.text->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_Center);
			_objs.text->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b,_objs.color.a);
			this->agregarObjetoEscena(_objs.text);
		}
	}
	//
	this->privOrganizarContenido(width, height, false, NULL, 0.0f);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

TSVisualStatus::~TSVisualStatus(){
	NBGestorAnimadores::quitarAnimador(this);
	//Fonts
	{
		if(_fntNames != NULL) _fntNames->liberar(NB_RETENEDOR_THIS); _fntNames = NULL;
	}
	//Objs
	{
		if(_objs.bg != NULL) _objs.bg->liberar(NB_RETENEDOR_THIS); _objs.bg = NULL;
		if(_objs.border != NULL) _objs.border->liberar(NB_RETENEDOR_THIS); _objs.border = NULL;
		if(_objs.ico != NULL) _objs.ico->liberar(NB_RETENEDOR_THIS); _objs.ico = NULL;
		if(_objs.text != NULL) _objs.text->liberar(NB_RETENEDOR_THIS); _objs.text = NULL;
 	}
}

//

void TSVisualStatus::setMargins(const float marginH, const float marginV){
	_marginH = marginH;
	_marginV = marginV;
}

//

ENTSVisualStatusMode TSVisualStatus::getMode() const {
	return _objs.mode;
}

void TSVisualStatus::setMode(const ENTSVisualStatusMode mode){
	this->privSetMode(mode);
}

//

STNBColor8 TSVisualStatus::getPrefColor() const {
	return _objs.color;
}

void TSVisualStatus::setPrefColor(const STNBColor8 prefColor){
	_objs.color = prefColor;
	if(_objs.ico != NULL){
		_objs.ico->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b, _objs.color.a);
	}
	if(_objs.text != NULL){
		_objs.text->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b,_objs.color.a);
	}
}

//

const STTSVisualStatusModeProps TSVisualStatus::privGetProps(STTSClient* client, const ENTSVisualStatusMode iMode){
	STTSVisualStatusModeProps p;
	NBMemory_setZeroSt(p, STTSVisualStatusModeProps);
	if(iMode >= 0 && iMode < ENTSVisualStatusMode_Count){
		p.base	= TSVisualStatus::privGetPropsBase(iMode);
		switch (iMode) {
			//
			case ENTSVisualStatusMode_LoadMini:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner"); //moreOptions
				p.text	= NULL;
				break;
			case ENTSVisualStatusMode_DecryptMini:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner"); //lock
				p.text	= NULL;
				break;
			case ENTSVisualStatusMode_ErrorMini:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#add");
				p.text	= NULL;
				break;
			//
			case ENTSVisualStatusMode_NoAccess:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#error");
				p.text	= TSClient_getStr(client, "meetingAttachNotPermText", "No permission yet.\n\nThis file was uploaded before you joined the meeting, any other member of the meeting must authorize your access to this file.");
				break;
			case ENTSVisualStatusMode_Downloading:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner");
				p.text	= TSClient_getStr(client, "meetingAttachWaitForDownloadText", "Waiting for file download.");
				break;
			case ENTSVisualStatusMode_Decrypting:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner");
				p.text	= TSClient_getStr(client, "meetingAttachWaitForDecryptText", "Waiting for file decryption.");
				break;
			case ENTSVisualStatusMode_Loading:
				p.isSet	= TRUE;
				p.tex	= NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#spinner");
				p.text	= TSClient_getStr(client, "meetingAttachWaitForLoadText", "Waiting for file load.");
				break;
			//
			default:
				break;
		}
	}
	return p;
}

const STTSVisualStatusModePropsBase TSVisualStatus::privGetPropsBase(const ENTSVisualStatusMode iMode){
	STTSVisualStatusModePropsBase p;
	NBMemory_setZeroSt(p, STTSVisualStatusModePropsBase);
	if(iMode >= 0 && iMode < ENTSVisualStatusMode_Count){
		switch (iMode) {
			//
			case ENTSVisualStatusMode_LoadMini:
				p.bgIsSolid	= FALSE;
				p.borderSolid = FALSE;
				p.animColor	= FALSE;
				p.velRot	= -360.0f;
				p.scale		= 1.0f;
				p.rot		= 0.0f;
				break;
			case ENTSVisualStatusMode_DecryptMini:
				p.bgIsSolid	= FALSE;
				p.borderSolid = FALSE;
				p.animColor	= FALSE;
				p.velRot	= -360.0f;
				p.scale		= 1.0f;
				p.rot		= 0.0f;
				break;
			case ENTSVisualStatusMode_ErrorMini:
				p.bgIsSolid	= FALSE;
				p.borderSolid = FALSE;
				p.animColor	= FALSE;
				p.velRot	= 0.0f;
				p.scale		= 1.0f;
				p.rot		= 45.0f;
				break;
			//
			case ENTSVisualStatusMode_NoAccess:
				p.bgIsSolid	= TRUE;
				p.borderSolid = TRUE;
				p.animColor	= FALSE;
				p.velRot	= 0.0f;
				p.scale		= 2.0f;
				p.rot		= 0.0f;
				break;
			case ENTSVisualStatusMode_Downloading:
				p.bgIsSolid	= TRUE;
				p.borderSolid = TRUE;
				p.animColor	= FALSE;
				p.velRot	= -360.0f;
				p.scale		= 2.0f;
				p.rot		= 0.0f;
				break;
			case ENTSVisualStatusMode_Decrypting:
				p.bgIsSolid	= TRUE;
				p.borderSolid = TRUE;
				p.animColor	= FALSE;
				p.velRot	= -360.0f;
				p.scale		= 2.0f;
				p.rot		= 0.0f;
				break;
			case ENTSVisualStatusMode_Loading:
				p.bgIsSolid	= TRUE;
				p.borderSolid = TRUE;
				p.animColor	= FALSE;
				p.velRot	= -360.0f;
				p.scale		= 2.0f;
				p.rot		= 0.0f;
				break;
			default:
				break;
		}
	}
	return p;
}

//AUEscenaListaItemI

AUEscenaObjeto* TSVisualStatus::itemObjetoEscena(){
	return this;
}

STListaItemDatos TSVisualStatus::refDatos(){
	return _refDatos;
}

void TSVisualStatus::establecerRefDatos(const SI32 tipo, const SI32 valor){
	_refDatos.tipo	= tipo;
	_refDatos.valor	= valor;
}

void TSVisualStatus::organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(ancho, altoVisible, notificarCambioAltura, animator, secsAnim);
}

//

void TSVisualStatus::privSetMode(const ENTSVisualStatusMode iMode){
	if(_objs.mode != iMode){
		_objs.mode = iMode;
		//Apply
		{
			const STTSVisualStatusModeProps props = TSVisualStatus::privGetProps(_client, iMode);
			if(!props.isSet){
				if(_objs.bg != NULL) _objs.bg->setVisibleAndAwake(FALSE);
				if(_objs.border != NULL) _objs.border->setVisibleAndAwake(FALSE);
				if(_objs.ico != NULL) _objs.ico->setVisibleAndAwake(FALSE);
				if(_objs.text != NULL) _objs.text->setVisibleAndAwake(FALSE);
			} else {
				_objs.rot = props.base.rot;
				//Bg
				if(_objs.bg != NULL){
					_objs.bg->setVisibleAndAwake(props.base.bgIsSolid);
				}
				//Border
				if(_objs.border != NULL){
					_objs.border->setVisibleAndAwake(props.base.borderSolid);
				}
				//Icon
				if(_objs.ico != NULL){
					_objs.ico->setVisibleAndAwake(TRUE);
					_objs.ico->establecerTextura(props.tex);
					_objs.ico->establecerRotacion(props.base.rot);
					{
						NBTamano sz = props.tex->tamanoHD();
						sz.ancho	*= props.base.scale;
						sz.alto		*= -props.base.scale;
						_objs.ico->redimensionar(sz.ancho, sz.alto);
						_objs.ico->establecerEscalacion(1.0f);
					}
					if(!props.base.animColor){
						_objs.ico->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b, _objs.color.a);
					} else {
						_objs.ico->establecerMultiplicadorColor8(_objs.color.r, _objs.color.g, _objs.color.b, _objs.color.a);
						_objs.ico->establecerMultiplicadorAlpha8(_objs.alphaRel * 255.0f);
					}
				}
				//Text
				if(_objs.text != NULL){
					if(NBString_strIsEmpty(props.text)){
						_objs.text->establecerTexto("");
						_objs.text->setVisibleAndAwake(FALSE);
					} else {
						_objs.text->establecerTexto(props.text);
						_objs.text->setVisibleAndAwake(TRUE);
					}
				}
			}
		}
		//Organize
		this->privOrganizarContenido(_anchoUsar, _altoUsar, TRUE, NULL, 0.0f);
	}
}

void TSVisualStatus::privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	if(ancho > 0.0f){
		float xLeft = 0.0f, xRight = 0.0f, yTop = 0.0f, yBtm = 0.0f;
		//Icon
		if(_objs.ico != NULL){
			const NBCajaAABB box = _objs.ico->cajaAABBLocal();
			_objs.ico->establecerTraslacion(0.0f - box.xMin - ((box.xMax - box.xMin) * 0.5f), yBtm - box.yMax);
			{
				const float midSize = ((box.xMax - box.xMin) * 0.5f);
				if(xLeft > -midSize) xLeft = -midSize;
				if(xRight < midSize) xRight = midSize;
			}
			yBtm -= (box.yMax - box.yMin);
			NBASSERT(xLeft <= xRight)
			NBASSERT(yBtm <= 0.0f)
		}
		//Text
		if(_objs.text != NULL){
			const float wFortxt = (ancho * 0.90f);
			_objs.text->organizarTexto(wFortxt <= 0.0f ? 99999.0f : wFortxt);
			{
				const NBCajaAABB box = _objs.text->cajaAABBLocal();
				_objs.text->establecerTraslacion(0.0f - box.xMin - ((box.xMax - box.xMin) * 0.5f), yBtm - box.yMax);
				{
					const float midSize = ((box.xMax - box.xMin) * 0.5f);
					if(xLeft > -midSize) xLeft = -midSize;
					if(xRight < midSize) xRight = midSize;
				}
				yBtm -= (box.yMax - box.yMin);
				NBASSERT(xLeft <= xRight)
				NBASSERT(yBtm <= 0.0f)
			}
		}
		//Set limits
		{
			if(alto > -yBtm){
				const float delta = (alto - (-yBtm)); NBASSERT(delta > 0.0f)
				yTop	+= (delta * 0.5f);
				yBtm	-= (delta * 0.5f);
			}
			if(ancho > (xRight - xLeft)){
				const float delta = (ancho + (xRight - xLeft)); NBASSERT(delta > 0.0f)
				xRight	+= (delta * 0.5f);
				xLeft	-= (delta * 0.5f);
			}
			//Margins
			{
				xLeft	-= _marginH;
				xRight	+= _marginH;
				yTop	+= _marginV;
				yBtm	-= _marginV;
			}
			if(_objs.bg != NULL){
				_objs.bg->redimensionar(xLeft, yTop, (xRight - xLeft), (yBtm - yTop));
			}
			if(_objs.border != NULL){
				_objs.border->moverVerticeHacia(0, xLeft, yTop);
				_objs.border->moverVerticeHacia(1, xLeft, yBtm);
				_objs.border->moverVerticeHacia(2, xRight, yBtm);
				_objs.border->moverVerticeHacia(3, xRight, yTop);
			}
			this->establecerLimites(xLeft, xRight, yBtm, yTop);
		}
		_anchoUsar	= ancho;
		_altoUsar = alto;
	}
}

//

void TSVisualStatus::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		const STTSVisualStatusModePropsBase props = TSVisualStatus::privGetPropsBase(_objs.mode);
		if(props.velRot != 0.0f){
			_objs.ico->establecerRotacionNormalizada(_objs.ico->rotacion() + (props.velRot * segsTranscurridos));
		}
		if(!props.animColor){
			_objs.alphaRel = 1.0f;
		} else {
			if(_objs.alphaInc){
				_objs.alphaRel += 2.0f * segsTranscurridos;
				if(_objs.alphaRel >= 1.0f){
					_objs.alphaRel = 1.0f;
					_objs.alphaInc = !_objs.alphaInc;
				}
			} else {
				_objs.alphaRel -= 2.0f * segsTranscurridos;
				if(_objs.alphaRel <= 0.5f){
					_objs.alphaRel = 0.5f;
					_objs.alphaInc = !_objs.alphaInc;
				}
			}
			NBASSERT(_objs.alphaRel >= 0.0f && _objs.alphaRel <= 1.0f)
		}
		_objs.ico->establecerMultiplicadorAlpha8(_objs.alphaRel * 255.0f);
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualStatus, AUEscenaContenedor)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualStatus, "TSVisualStatus", AUEscenaContenedor)
AUOBJMETODOS_CLONAR_NULL(TSVisualStatus)


