//
//  TSVisualIcons.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSVisualIcons_h
#define TSVisualIcons_h

#include "nb/core/NBCallback.h"
#include "AUEscenaContenedor.h"

typedef struct STNBVisualIconsText_ {
	float			maxWIdthInchs;
	AUEscenaTexto*	text;
} STNBVisualIconsText;

class TSVisualIcons: public AUEscenaContenedorLimitado {
	public:
		TSVisualIcons(const SI32 iScene, const float marginH);
		virtual		~TSVisualIcons();
		//
		BOOL	addIcon(const char* texPath, const float scale);
		BOOL	addText(const char* text, const ENNBTextLineAlignH alignH, AUFuenteRender* font, const float maxWidthInchs);
		BOOL	addTextWithColor(const char* text, const ENNBTextLineAlignH alignH, AUFuenteRender* font, const float maxWidthInchs, const STNBColor8 color);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		SI32		_iScene;
		float		_marginH;
		STNBArray	_sprites; //AUEscenaSprite*
		STNBArray	_texts; //STNBVisualIconsText
		void		privOrganize(const float marginH);
};

#endif /* TSVisualIcons_h */
