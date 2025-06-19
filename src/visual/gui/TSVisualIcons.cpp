//
//  TSVisualIcons.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/gui/TSVisualIcons.h"
#include "visual/TSColors.h"
#include "visual/TSFonts.h"

TSVisualIcons::TSVisualIcons(const SI32 iScene, const float marginH) : AUEscenaContenedorLimitado() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualIcons")
	_iScene		= iScene;
	_marginH	= marginH;
	NBArray_init(&_sprites, sizeof(AUEscenaSprite*), NULL);
	NBArray_init(&_texts, sizeof(STNBVisualIconsText), NULL);
}


TSVisualIcons::~TSVisualIcons(){
	//Icons
	{
		SI32 i; for(i = 0; i < _sprites.use; i++){
			AUEscenaSprite* s = NBArray_itmValueAtIndex(&_sprites, AUEscenaSprite*, i);
			if(s != NULL){
				s->liberar(NB_RETENEDOR_THIS);
				s = NULL;
			}
		}
		NBArray_empty(&_sprites);
		NBArray_release(&_sprites);
	}
	//Text
	{
		SI32 i; for(i = 0; i < _texts.use; i++){
			STNBVisualIconsText* t = NBArray_itmPtrAtIndex(&_texts, STNBVisualIconsText, i);
			if(t->text != NULL){
				t->text->liberar(NB_RETENEDOR_THIS);
				t->text = NULL;
			}
		}
		NBArray_empty(&_texts);
		NBArray_release(&_texts);
	}
}

//

BOOL TSVisualIcons::addIcon(const char* texPath, const float scale){
	BOOL r = FALSE;
	if(texPath != NULL && (scale < -0.0001f || scale > 0.0001f)){
		AUTextura* tex = NBGestorTexturas::texturaDesdeArchivo(texPath);
		if(tex != NULL){
			AUEscenaSprite* s = new(this) AUEscenaSprite(tex);
			s->establecerEscalacion(scale);
			this->agregarObjetoEscena(s);
			NBArray_addValue(&_sprites, s);
			//
			this->privOrganize(_marginH);
			//
			r = TRUE;
		}
	}
	return r;
}

BOOL TSVisualIcons::addText(const char* text, const ENNBTextLineAlignH alignH, AUFuenteRender* font, const float maxWidthInchs){
	return this->addTextWithColor(text, alignH, font, maxWidthInchs,  NBST_P(STNBColor8, 255, 255, 255, 255));
}

BOOL TSVisualIcons::addTextWithColor(const char* text, const ENNBTextLineAlignH alignH, AUFuenteRender* font, const float maxWidthInchs, const STNBColor8 color){
	BOOL r = FALSE;
	if(!NBString_strIsEmpty(text) && font != NULL){
		STNBVisualIconsText t;
		NBMemory_setZeroSt(t, STNBVisualIconsText);
		t.maxWIdthInchs = maxWidthInchs;
		t.text = new(this) AUEscenaTexto(font);
		t.text->establecerAlineacionH(alignH);
		t.text->establecerTexto(text);
		t.text->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
		this->agregarObjetoEscena(t.text);
		NBArray_addValue(&_texts, t);
		//
		this->privOrganize(_marginH);
		//
		r = TRUE;
	}
	return r;
}

void TSVisualIcons::privOrganize(const float icosMarginH){
	float iconsWidth = 0.0f; //total
	float totalWidth = 0.0f; //total
	float iconsHeight = 0.0f; //max
	//Calculate
	{
		//Icons
		{
			SI32 i; for(i = 0; i < _sprites.use; i++){
				AUEscenaSprite* s = NBArray_itmValueAtIndex(&_sprites, AUEscenaSprite*, i);
				if(s != NULL){
					const NBCajaAABB box = s->cajaAABBLocal();
					const float scale = s->escalacion().ancho;
					if(iconsWidth != 0.0f) iconsWidth += icosMarginH;
					iconsWidth += ((box.xMax - box.xMin) * scale);
					if(iconsHeight < ((box.yMax - box.yMin) * scale)){
						iconsHeight = ((box.yMax - box.yMin) * scale);
					}
				}
			}
			totalWidth = iconsWidth;
		}
		//Texts
		{
			SI32 i; for(i = 0; i < _texts.use; i++){
				STNBVisualIconsText* t = NBArray_itmPtrAtIndex(&_texts, STNBVisualIconsText, i);
				if(t->maxWIdthInchs <= 0.0f){
					t->text->organizarTexto(999999.0f);
				} else {
					t->text->organizarTexto(NBGestorEscena::anchoPulgadasAEscena(_iScene, t->maxWIdthInchs));
				}
				{
					const NBCajaAABB box = t->text->cajaAABBLocal();
					const float width = (box.xMax - box.xMin);
					if(totalWidth < width) totalWidth = width;
				}
			}
		}
	}
	//Organize
	{
		const float yBtm = 0.0f - (iconsHeight * 0.5f); //centered
		const float xLeftBase = 0.0f - (totalWidth * 0.5f); //centered
		//Icons
		{
			const float xLeftStart = xLeftBase + ((totalWidth - iconsWidth) * 0.5f);
			float xLeft = xLeftStart;
			SI32 i; for(i = 0; i < _sprites.use; i++){
				AUEscenaSprite* s = NBArray_itmValueAtIndex(&_sprites, AUEscenaSprite*, i);
				if(s != NULL){
					const NBCajaAABB box = s->cajaAABBLocal();
					const float scale = s->escalacion().ancho;
					if(xLeft != xLeftStart) xLeft += icosMarginH;
					s->establecerTraslacion(xLeft - (box.xMin * scale), yBtm - (box.yMin * scale) + ((iconsHeight - ((box.yMax - box.yMin) * scale)) * 0.0f));
					xLeft += ((box.xMax - box.xMin) * scale);
				}
			}
		}
		//Texts
		{
			const float yTopStart = yBtm - _marginH;
			float yTop = yTopStart;
			SI32 i; for(i = 0; i < _texts.use; i++){
				STNBVisualIconsText* t = NBArray_itmPtrAtIndex(&_texts, STNBVisualIconsText, i);
				const NBCajaAABB box = t->text->cajaAABBLocal();
				const float width = (box.xMax - box.xMin);
				const float height = (box.yMax - box.yMin);
				if(yTop != yTopStart) yTop -= _marginH;
				t->text->establecerTraslacion(xLeftBase - box.xMin + ((totalWidth - width) * 0.5f), yTop - box.yMax);
				yTop -= height;
			}
		}
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualIcons, AUEscenaContenedor)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualIcons, "TSVisualIcons", AUEscenaContenedor)
AUOBJMETODOS_CLONAR_NULL(TSVisualIcons)


