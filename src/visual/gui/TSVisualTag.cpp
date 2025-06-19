//
//  TSVisualTag.cpp
//  thinstream
//
//  Created by Marcos Ortega on 10/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/gui/TSVisualTag.h"
//
#include "visual/TSColors.h"

TSVisualTag::TSVisualTag(const SI32 iScene, AUTextura* texIcoLft, AUFuenteRender* fnt, const char* tag, AUTextura* texIcoRght, const STNBColor8 bgColor, const STNBColor8 fgColor, const float widthMin, const float heightMin)
: AUEscenaContenedorLimitado()
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualTag")
	//
	_iScene			= iScene;
	//
	_widthMin		= widthMin;
	_heightMin		= heightMin;
	_bgColor		= bgColor;
	_fgColor		= fgColor;
	//Bg
	{
		_bg	= new(this) AUEscenaSpriteElastico(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons-circle-small.png"));
		_bg->establecerMultiplicadorColor8(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		this->agregarObjetoEscena(_bg);
	}
	//BG shadow
	{
		_bgShadow = NULL;
	}
	//Icon left
	{
		_icoLft	= NULL;
		if(texIcoLft != NULL){
			_icoLft = new(this) AUEscenaSprite(texIcoLft);
			this->agregarObjetoEscena(_icoLft);
		}
	}
	//Text
	{
		_txtFnt		= fnt; if(_txtFnt != NULL) _txtFnt->retener(NB_RETENEDOR_THIS);
		_txt		= NULL;
		if(_txtFnt != NULL && !NBString_strIsEmpty(tag)){
			_txt = new (this) AUEscenaTexto(_txtFnt);
			_txt->establecerTexto(tag);
			_txt->establecerMultiplicadorColor8(fgColor.r, fgColor.g, fgColor.b, fgColor.a);
			this->agregarObjetoEscena(_txt);
		}
	}
	//Icon right
	{
		_icoRght	= NULL;
		if(texIcoRght != NULL){
			_icoRght = new(this) AUEscenaSprite(texIcoRght);
			_icoRght->establecerMultiplicadorColor8(fgColor.r, fgColor.g, fgColor.b, fgColor.a);
			this->agregarObjetoEscena(_icoRght);
		}
	}
	//Organize
	this->privOrganizarContenido();
}

TSVisualTag::~TSVisualTag(){
	if(_bg != NULL) _bg->liberar(NB_RETENEDOR_THIS); _bg = NULL;
	if(_bgShadow != NULL) _bgShadow->liberar(NB_RETENEDOR_THIS); _bgShadow = NULL;
	if(_icoLft != NULL) _icoLft->liberar(NB_RETENEDOR_THIS); _icoLft = NULL;
	if(_txtFnt != NULL) _txtFnt->liberar(NB_RETENEDOR_THIS); _txtFnt = NULL;
	if(_txt != NULL) _txt->liberar(NB_RETENEDOR_THIS); _txt = NULL;
	if(_icoRght != NULL) _icoRght->liberar(NB_RETENEDOR_THIS); _icoRght = NULL;
}

//

void TSVisualTag::updateShadow(AUTextura* texShadow, const STNBColor8 shadowColor){
	BOOL changed = TRUE;
	if(texShadow == NULL){
		if(_bgShadow != NULL){
			AUEscenaContenedor* parent = (AUEscenaContenedor*)_bgShadow->contenedor();
			if(parent != NULL) parent->quitarObjetoEscena(_bgShadow);
			_bgShadow->liberar(NB_RETENEDOR_THIS);
			_bgShadow = NULL;
			changed = TRUE;
		}
	} else {
		if(_bgShadow == NULL){
			_bgShadow = new(this) AUEscenaSpriteElastico(texShadow);
			_bgShadow->establecerMultiplicadorColor8(shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);
			this->agregarObjetoEscenaEnIndice(_bgShadow, 0);
			changed = TRUE;
		} else {
			const NBColor8 c = _bgShadow->multiplicadorColor8Func();
			const BOOL changedTex	= (_bgShadow->textura() != texShadow);
			const BOOL changedColor	= (c.r != shadowColor.r || c.g != shadowColor.g || c.b != shadowColor.b || c.a != shadowColor.a);
			if(changedTex || changedColor){
				if(changedTex){
					_bgShadow->establecerTextura(texShadow);
				}
				if(changedColor){
					_bgShadow->establecerMultiplicadorColor8(shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);
				}
				changed = TRUE;
			}
		}
	}
	//Organize
	if(changed){
		this->privOrganizarContenido();
	}
}

void TSVisualTag::updateShadowColor(const STNBColor8 shadowColor){
	if(_bgShadow != NULL){
		const NBColor8 c = _bgShadow->multiplicadorColor8Func();
		const BOOL changedColor	= (c.r != shadowColor.r || c.g != shadowColor.g || c.b != shadowColor.b || c.a != shadowColor.a);
		if(changedColor){
			_bgShadow->establecerMultiplicadorColor8(shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);
		}
	}
}

void TSVisualTag::updateTag(const char* tag){
	//Create or remove tag
	if(NBString_strIsEmpty(tag)){
		if(_txt != NULL){
			AUEscenaContenedor* parent = (AUEscenaContenedor*)_txt->contenedor();
			if(parent != NULL) parent->quitarObjetoEscena(_txt);
			_txt->liberar(NB_RETENEDOR_THIS);
			_txt = NULL;
		}
	} else {
		if(_txt == NULL && _txtFnt != NULL){
			_txt = new (this) AUEscenaTexto(_txtFnt);
			_txt->establecerMultiplicadorColor8(_fgColor.r, _fgColor.g, _fgColor.b, _fgColor.a);
			this->agregarObjetoEscena(_txt);
		}
	}
	//Update
	if(_txt != NULL){
		_txt->establecerTexto(tag);
	}
	//Organize
	this->privOrganizarContenido();
}

void TSVisualTag::updateTagAndColors(const char* tag, const STNBColor8 bgColor, const STNBColor8 fgColor){
	_bgColor = bgColor;
	_fgColor = fgColor;
	//Create or remove tag
	if(NBString_strIsEmpty(tag)){
		if(_txt != NULL){
			AUEscenaContenedor* parent = (AUEscenaContenedor*)_txt->contenedor();
			if(parent != NULL) parent->quitarObjetoEscena(_txt);
			_txt->liberar(NB_RETENEDOR_THIS);
			_txt = NULL;
		}
	} else {
		if(_txt == NULL && _txtFnt != NULL){
			_txt = new (this) AUEscenaTexto(_txtFnt);
			this->agregarObjetoEscena(_txt);
		}
	}
	//Update
	if(_txt != NULL){
		_txt->establecerTexto(tag);
		_txt->establecerMultiplicadorColor8(fgColor.r, fgColor.g, fgColor.b, fgColor.a);
	}
	if(_icoLft != NULL){
		_icoLft->establecerMultiplicadorColor8(fgColor.r, fgColor.g, fgColor.b, fgColor.a);
	}
	if(_icoRght != NULL){
		_icoRght->establecerMultiplicadorColor8(fgColor.r, fgColor.g, fgColor.b, fgColor.a);
	}
	if(_bg != NULL){
		_bg->establecerMultiplicadorColor8(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	}
	//Organize
	this->privOrganizarContenido();
}

void TSVisualTag::updateColorBg(const STNBColor8 bgColor){
	_bgColor = bgColor;
	if(_bg != NULL){
		_bg->establecerMultiplicadorColor8(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	}
}

BOOL TSVisualTag::updateIcons(AUTextura* texIcoLft, AUTextura* texIcoRght){
	BOOL organize = FALSE;
	//Left
	{
		if(texIcoLft == NULL){
			if(_icoLft != NULL){
				AUEscenaContenedor* parent = (AUEscenaContenedor*)_icoLft->contenedor();
				if(parent != NULL) parent->quitarObjetoEscena(_icoLft);
				_icoLft->liberar(NB_RETENEDOR_THIS);
				_icoLft = NULL;
				organize = TRUE;
			}
		} else {
			if(_icoLft == NULL){
				_icoLft = new(this) AUEscenaSprite(texIcoLft);
				this->agregarObjetoEscena(_icoLft);
				organize = TRUE;
			} else if(_icoLft->textura() != texIcoLft){
				_icoLft->establecerTextura(texIcoLft);
				_icoLft->redimensionar(texIcoLft);
				organize = TRUE;
			}
		}
	}
	//Right
	{
		if(texIcoRght == NULL){
			if(_icoRght != NULL){
				AUEscenaContenedor* parent = (AUEscenaContenedor*)_icoRght->contenedor();
				if(parent != NULL) parent->quitarObjetoEscena(_icoRght);
				_icoRght->liberar(NB_RETENEDOR_THIS);
				_icoRght = NULL;
				organize = TRUE;
			}
		} else {
			if(_icoRght == NULL){
				_icoRght = new(this) AUEscenaSprite(texIcoRght);
				this->agregarObjetoEscena(_icoRght);
				organize = TRUE;
			} else if(_icoRght->textura() != texIcoRght){
				_icoRght->establecerTextura(texIcoRght);
				_icoRght->redimensionar(texIcoRght);
				organize = TRUE;
			}
		}
	}
	if(organize){
		this->privOrganizarContenido();
	}
	return organize;
}

void TSVisualTag::privOrganizarContenido(){
	const float prefixMrgnI	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.01f);
	const float prefixMrgnH	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.05f);
	const float prefixMrgnV	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.0125f);
	float widthIn = 0.0f, heightIn = 0.0f, widthInWithMrg = 0.0f, heightInWithMrg = 0.0f;
	STNBSize bgTexSz = NBST_P(STNBSize, 0.0f, 0.0f );
	if(_bg != NULL){
		AUTextura* tex = _bg->textura();
		if(tex != NULL){
			const NBTamano sz	= tex->tamanoHD();
			bgTexSz.width		= sz.ancho;
			bgTexSz.height		= sz.alto;
		}
	}
	//Calculate size
	{
		if(_icoLft != NULL){
			const NBCajaAABB box = _icoLft->cajaAABBLocal();
			if(box.xMin < box.xMax){
				if(widthIn != 0.0f) widthIn += prefixMrgnI;
				widthIn += (box.xMax - box.xMin);
			}
			if(heightIn < (box.yMax - box.yMin)){
				heightIn = (box.yMax - box.yMin);
			}
		}
		if(_txt != NULL){
			const NBCajaAABB box = _txt->cajaAABBLocal();
			if(box.xMin < box.xMax){
				if(widthIn != 0.0f) widthIn += prefixMrgnI;
				widthIn += (box.xMax - box.xMin);
			}
			if(heightIn < (box.yMax - box.yMin)){
				heightIn = (box.yMax - box.yMin);
			}
		}
		if(_icoRght != NULL){
			const NBCajaAABB box = _icoRght->cajaAABBLocal();
			if(box.xMin < box.xMax){
				if(widthIn != 0.0f) widthIn += prefixMrgnI;
				widthIn += (box.xMax - box.xMin);
			}
			if(heightIn < (box.yMax - box.yMin)){
				heightIn = (box.yMax - box.yMin);
			}
		}
		//Calculate size with margins
		if(widthIn <= 0.0f){
			widthInWithMrg = bgTexSz.width;
		} else {
			widthInWithMrg = prefixMrgnH + widthIn + prefixMrgnH;
		}
		if(heightIn <= 0.0f){
			heightInWithMrg = bgTexSz.height;
		} else {
			heightInWithMrg = prefixMrgnV + heightIn + prefixMrgnV;
		}
		if(widthInWithMrg < _widthMin){
			widthInWithMrg = _widthMin;
		}
		if(heightInWithMrg < _heightMin){
			heightInWithMrg = _heightMin;
		}
		if(widthInWithMrg < bgTexSz.width){
			widthInWithMrg = bgTexSz.width;
		}
		if(heightInWithMrg < bgTexSz.height){
			heightInWithMrg = bgTexSz.height;
		}
	}
	//Organize
	{
		STNBSize shadowTexSz		= bgTexSz;
		STNBSize shdwDeltaSz		= NBST_P(STNBSize, 0.0f, 0.0f );
		const float xLeftStart		= ((widthInWithMrg - widthIn) * 0.5f);
		//const float xRghtStart	= widthInWithMrg - ((widthInWithMrg - widthIn) * 0.5f);
		//const float yTopStart		= -((heightInWithMrg - heightIn) * 0.5f);
		//const float yBtmStart		= -heightInWithMrg + ((heightInWithMrg - heightIn) * 0.5f);
		{
			float xLeftt	= 0.0f;
			if(_icoLft != NULL){
				const NBCajaAABB box = _icoLft->cajaAABBLocal();
				if(xLeftt != 0.0f) xLeftt += prefixMrgnI;
				_icoLft->establecerTraslacion(xLeftStart + xLeftt - box.xMin, 0.0f - box.yMax - ((heightInWithMrg - (box.yMax - box.yMin)) * 0.5f));
				xLeftt += (box.xMax - box.xMin);
			}
			if(_txt != NULL){
				const NBCajaAABB box = _txt->cajaAABBLocal();
				if(xLeftt != 0.0f) xLeftt += prefixMrgnI;
				_txt->establecerTraslacion(xLeftStart + xLeftt - box.xMin, 0.0f - box.yMax - ((heightInWithMrg - (box.yMax - box.yMin)) * 0.5f));
				xLeftt += (box.xMax - box.xMin);
			}
			if(_icoRght != NULL){
				const NBCajaAABB box = _icoRght->cajaAABBLocal();
				if(xLeftt != 0.0f) xLeftt += prefixMrgnI;
				_icoRght->establecerTraslacion(xLeftStart + xLeftt - box.xMin, 0.0f - box.yMax - ((heightInWithMrg - (box.yMax - box.yMin)) * 0.5f));
				xLeftt += (box.xMax - box.xMin);
			}
			if(_bg != NULL){
				_bg->redimensionar(0.0f, 0.0f, widthInWithMrg, -heightInWithMrg);
			}
			if(_bgShadow != NULL){
				AUTextura* tex			= _bgShadow->textura();
				if(tex != NULL){
					const NBTamano sz	= tex->tamanoHD();
					shadowTexSz.width	= sz.ancho;
					shadowTexSz.height	= sz.alto;
				}
				{
					shdwDeltaSz.width	= shadowTexSz.width - bgTexSz.width;
					shdwDeltaSz.height	= shadowTexSz.height - bgTexSz.height;
					_bgShadow->redimensionar(-shdwDeltaSz.width * 0.5f, shdwDeltaSz.height * 0.5f, widthInWithMrg + shdwDeltaSz.width, -heightInWithMrg - shdwDeltaSz.height);
				}
			}
		}
		//Set limits
		this->establecerLimites(-shdwDeltaSz.width * 0.5f, widthInWithMrg + shdwDeltaSz.width, -heightInWithMrg - shdwDeltaSz.height, shdwDeltaSz.height * 0.5f);
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualTag, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualTag, "TSVisualTag", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(TSVisualTag)


