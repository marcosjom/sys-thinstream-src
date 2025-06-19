//
//  TSFonts.cpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/19.
//

#include "visual/TSVisualPch.h"
#include "visual/TSFonts.h"
#include "nb/core/NBString.h"
#include "NBGestorEscena.h"
#include "NBGestorTexturas.h"
#include "NBMngrFonts.h"

#define TS_COMMON_CHARS_NUMERIC_ONLY	"0123456789"
#define TS_COMMON_CHARS_TIME_ONLY		"0123456789ahmpsAHMPS:"
#define TS_COMMON_CHARS_ALPHA_LOWER		"0123456789abcdefghijklmnopqrstuvwxyz.,:;()[]-_!?%$#@/\\"
#define TS_COMMON_CHARS_ALPHA_ALL		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,:;()[]-_!?%$#@/\\"

static STTSFontDef const _fontDefs[] = {
	{
		ENTSFont_TitleHuge
		, "Rajdhani"
		, "Bold"
		, TRUE
		, FALSE
		, 20.0f
		, FALSE
		, "WELCOME"
	}
	, {
		ENTSFont_DigitsHuge
		, "JetBrains Mono"
		, "Bold"
		, TRUE
		, FALSE
		, 14.0f
		, FALSE
		, TS_COMMON_CHARS_NUMERIC_ONLY
	}
	, {
		ENTSFont_Btn
		, "Inter Medium" 
		, "Regular"
		, TRUE
		, FALSE
		, 8.0f
		, FALSE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
	, {
		ENTSFont_ContentTitleHuge
		, "Inter Medium" 
		, "Regular"
		, TRUE
		, FALSE
		, 14.0f
		, FALSE
		, TS_COMMON_CHARS_NUMERIC_ONLY
	}
	, {
		ENTSFont_ContentTitleBig
		, "Inter Medium" 
		, "Regular"
		, TRUE
		, FALSE
		, 10.0f
		, FALSE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
	, {
		ENTSFont_ContentTitle
		, "Inter Medium" 
		, "Regular"
		, TRUE
		, FALSE
		, 8.0f
		, FALSE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
	, {
		ENTSFont_ContentHuge
		, "Inter" 
		, "Regular"
		, TRUE
		, FALSE
		, 18.0f
		, FALSE
		, TS_COMMON_CHARS_NUMERIC_ONLY
	}
	, {
		ENTSFont_ContentMid
		, "Inter" 
		, "Regular"
		, FALSE
		, FALSE
		, 7.0f
		, FALSE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
	, {
		ENTSFont_ContentMidBold
		, "Inter Medium" 
		, "Bold"
		, TRUE
		, FALSE
		, 7.0f
		, FALSE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
	, {
		ENTSFont_ContentSmall
		, "Inter" 
		, "Regular"
		, FALSE
		, FALSE
		, 6.0f
		, FALSE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
	, {
		ENTSFont_TimeSmall
		, "JetBrains Mono"
		, "Regular"
		, FALSE
		, FALSE
		, 6.0f
		, FALSE
		, TS_COMMON_CHARS_TIME_ONLY
	}
	, {
		ENTSFont_OverPdfFont
		, "Inter" 
		, "Regular"
		, FALSE
		, FALSE
		, 24.0f
		, TRUE
		, TS_COMMON_CHARS_ALPHA_ALL
	}
};

static float _fontsSizeAdjustment = 0.0f; 
static AUFuenteRender* _fontsObjs[ENTSFont_Count];

//

void TSFonts::init(const SI32 iScene){
	NBASSERT((sizeof(_fontsObjs) / sizeof(_fontsObjs[0])) == ENTSFont_Count)
	NBASSERT((sizeof(_fontDefs) / sizeof(_fontDefs[0])) == ENTSFont_Count)
	NBMemory_setZero(_fontsObjs);
	TSFonts::reInit(iScene);
}

void TSFonts::finish(){
	//Fonts
	{
		SI32 i; for(i = 0; i < ENTSFont_Count; i++){
			if(_fontsObjs[i] != NULL){
				_fontsObjs[i]->liberar(NB_RETENEDOR_NULL);
				_fontsObjs[i] = NULL;
			}
		}
	}
}

//


void TSFonts::reInit(const SI32 iScene){ //To force font reloads
	//Fonts sizes
	{
		const NBCajaAABB scnBox	= NBGestorEscena::cajaProyeccionGrupo(iScene, ENGestorEscenaGrupo_GUI);
		const float widthInch	= NBGestorEscena::anchoEscenaAPulgadas(iScene, (scnBox.xMax - scnBox.xMin));
		const float heightInch	= NBGestorEscena::altoEscenaAPulgadas(iScene, (scnBox.yMax - scnBox.yMin));
		const float diagInch	= sqrtf((widthInch * widthInch) + (heightInch * heightInch));
		PRINTF_INFO("Device window inches w(%f) h(%f), diag(%f) box(%d, %d).\n", widthInch, heightInch, diagInch, (SI32)(scnBox.xMax - scnBox.xMin), (SI32)(scnBox.yMax - scnBox.yMin));
#		if defined(__ANDROID__) || defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
		if(heightInch < 5.0f){ //iPhone SE is 4.87 x 2.31
			_fontsSizeAdjustment				= 0.0f;
		} else if(heightInch < 7.0f){ //iPhone 6S is 6.23 x 3.07
			_fontsSizeAdjustment				= 2.0f;
		} else { //Big tablet
			_fontsSizeAdjustment				= 3.0f;
		}
#		else //Ausume OS with resizable windows (bigger fonts)
		_fontsSizeAdjustment				= 4.0f;
#		endif
	}
	//Fonts
	{
		SI32 i; for(i = 0; i < ENTSFont_Count; i++){
			NBASSERT(_fontDefs[i].font == i)
			if(_fontsObjs[i] != NULL){
				_fontsObjs[i]->liberar(NB_RETENEDOR_NULL);
				_fontsObjs[i] = NULL;
			}
		}
	}
}

//

/*float TSFonts::pointsSz(const ENTSFont fnt){
	float r = 0.0f;
	if(fnt >= 0 && fnt < ENTSFont_Count){
		r = _fontDefs[fnt].sizeBase + (_fontDefs[fnt].allowSizeAdjust ? _fontsSizeAdjustment : 0.0f);
	}
	return r;
}*/

/*const char* TSFonts::preloadAlphabeth(const ENTSFont fnt){
	const char* r = NULL;
	if(fnt >= 0 && fnt < ENTSFont_Count){
		r = _fontDefs[fnt].charsToPreload;
	}
	return r;
}*/

AUFuenteRender* TSFonts::font(const ENTSFont fnt){
	AUFuenteRender* r = NULL;
	if(fnt >= 0 && fnt < ENTSFont_Count){
		//Load
		if(_fontsObjs[fnt] == NULL){
			const STTSFontDef* fntDef = &_fontDefs[fnt];
			if(fntDef->sizeInPixels){
				const float szPxs = fntDef->sizeBase;
				NBASSERT(fntDef->font == fnt)
				NBASSERT(szPxs > 0.0f)
				{
					if(NBString_strIsEmpty(fntDef->subfamily)){
						AUFuenteTextura* fntObj = NBMngrFonts::fontTexturesInPixels(fntDef->family, fntDef->isBold, fntDef->isItalic, szPxs);
						if(fntObj != NULL){
							if(!NBString_strIsEmpty(fntDef->charsToPreload)){
								const UI32 preloaded = fntObj->preloadCharShapes(fntDef->charsToPreload);
								if(preloaded > 0){
									PRINTF_INFO("TSFonts %d glyphs preloaded for font(%d)SzPxs(%f).\n", preloaded, fnt, szPxs);
								}
							}
							fntObj->retener(NB_RETENEDOR_NULL);
						}
						_fontsObjs[fnt] = fntObj;
					} else {
						AUFuenteTextura* fntObj = NBMngrFonts::fontTexturesInPixels(fntDef->family, fntDef->subfamily, szPxs);
						if(fntObj != NULL){
							if(!NBString_strIsEmpty(fntDef->charsToPreload)){
								const UI32 preloaded = fntObj->preloadCharShapes(fntDef->charsToPreload);
								if(preloaded > 0){
									PRINTF_INFO("TSFonts %d glyphs preloaded for font(%d)SzPxs(%f).\n", preloaded, fnt, szPxs);
								}
							}
							fntObj->retener(NB_RETENEDOR_NULL);
						}
						_fontsObjs[fnt] = fntObj;
					}
				}
			} else {
				const float szPts = fntDef->sizeBase + _fontsSizeAdjustment;
				NBASSERT(fntDef->font == fnt)
				NBASSERT(szPts > 0.0f)
				{
					if(NBString_strIsEmpty(fntDef->subfamily)){
						AUFuenteTextura* fntObj = NBMngrFonts::fontTextures(fntDef->family, fntDef->isBold, fntDef->isItalic, szPts);
						if(fntObj != NULL){
							if(!NBString_strIsEmpty(fntDef->charsToPreload)){
								const UI32 preloaded = fntObj->preloadCharShapes(fntDef->charsToPreload);
								if(preloaded > 0){
									PRINTF_INFO("TSFonts %d glyphs preloaded for font(%d)Sz(%f).\n", preloaded, fnt, szPts);
								}
							}
							fntObj->retener(NB_RETENEDOR_NULL);
						}
						_fontsObjs[fnt] = fntObj;
					} else {
						AUFuenteTextura* fntObj = NBMngrFonts::fontTextures(fntDef->family, fntDef->subfamily, szPts);
						if(fntObj != NULL){
							if(!NBString_strIsEmpty(fntDef->charsToPreload)){
								const UI32 preloaded = fntObj->preloadCharShapes(fntDef->charsToPreload);
								if(preloaded > 0){
									PRINTF_INFO("TSFonts %d glyphs preloaded for font(%d)Sz(%f).\n", preloaded, fnt, szPts);
								}
							}
							fntObj->retener(NB_RETENEDOR_NULL);
						}
						_fontsObjs[fnt] = fntObj;
					}
				}
			}
		}
		//Return
		r = _fontsObjs[fnt];
	}
	return r;	
}

const char* TSFonts::fontFamilyName(const ENTSFont fnt){
	const char* r = NULL;
	if(fnt >= 0 && fnt < ENTSFont_Count){
		r = _fontDefs[fnt].family;
	}
	return r;
}

