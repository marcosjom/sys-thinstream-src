//
//  TSColors.h
//  thinstream
//
//  Created by Marcos Ortega on 9/2/19.
//

#ifndef TSColors_h
#define TSColors_h

#define TS_COLOR_SCROLL_BAR_ALPHA8_TEXT		125	//Text panels
#define TS_COLOR_SCROLL_BAR_ALPHA8_COLS		200	//Visual columns

typedef enum ENTSColor_ {
	ENTSColor_MainColor = 0,
	ENTSColor_Black,
	ENTSColor_Gray,
	ENTSColor_GrayLight,
	ENTSColor_Light,
	ENTSColor_LightFront,
	ENTSColor_White,
	ENTSColor_Red,
	ENTSColor_RedLight,
	ENTSColor_Green,
	ENTSColor_GreenLight,
	ENTSColor_Blue,
	ENTSColor_Yellow,
	//Bar
	ENTSColor_BarBg,
	ENTSColor_BarBtnIcoSel,
	ENTSColor_BarBtnIcoUnsel,
	//
	ENTSColor_Count
} ENTSColor;

typedef struct STSceneInfoColorDef_ {
	STNBColor8	normal;
	STNBColor8	hover;
	STNBColor8	touched;
} STSceneInfoColorDef;


class TSColors {
	public:
		static const STSceneInfoColorDef* colorDef(const ENTSColor color);
};

#endif /* TSColors_h */
