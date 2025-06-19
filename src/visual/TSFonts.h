//
//  TSFonts.h
//  thinstream
//
//  Created by Marcos Ortega on 9/2/19.
//

#ifndef TSFonts_h
#define TSFonts_h

#include "AUFuenteRender.h"

typedef enum ENTSFont_ {
	ENTSFont_TitleHuge = 0,		//For "WELCOME!"
	ENTSFont_DigitsHuge,		//Verification code input
	ENTSFont_Btn,				//For buttons
	ENTSFont_ContentTitleHuge,	//Huge title
	ENTSFont_ContentTitleBig,	//Title (big)
	ENTSFont_ContentTitle,		//Title
	ENTSFont_ContentHuge,		//Huge content
	ENTSFont_ContentMid,		//Mid-size text
	ENTSFont_ContentMidBold,	//Mid-size text
	ENTSFont_ContentSmall,		//Small-size text
	ENTSFont_TimeSmall,			//Time numbers
	ENTSFont_OverPdfFont,		//Font over PDF (non adjustanle0
	ENTSFont_Count
} ENTSFont;

typedef struct STTSFontDef_ {
	ENTSFont	font;
	const char*	family;
	const char*	subfamily;
	BOOL		isItalic;
	BOOL		isBold;
	float		sizeBase;
	BOOL		sizeInPixels;
	const char*	charsToPreload;
} STTSFontDef; 

//

class TSFonts {
	public:
		static void				init(const SI32 iScene);
		static void				finish();
		//
		static void				reInit(const SI32 iScene); //To force font reloads
		//
		//static float			pointsSz(const ENTSFont fnt);
		//static const char*	preloadAlphabeth(const ENTSFont fnt);
		static AUFuenteRender*	font(const ENTSFont fnt);
		static const char*		fontFamilyName(const ENTSFont fnt);
};

#endif /* TSFonts_h */
