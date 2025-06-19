//
//  TSVisualTag.hpp
//  thinstream
//
//  Created by Marcos Ortega on 10/3/19.
//

#ifndef TSVisualTag_hpp
#define TSVisualTag_hpp

#include "AUEscenaContenedor.h"
#include "NBColor.h"

class TSVisualTag: public AUEscenaContenedorLimitado {
	public:
		TSVisualTag(const SI32 iScene, AUTextura* texIcoLft, AUFuenteRender* fnt, const char* tag, AUTextura* texIcoRght, const STNBColor8 bgColor, const STNBColor8 fgColor, const float widthMin, const float heightMin);
		virtual				~TSVisualTag();
		//
		void		updateShadow(AUTextura* texShadow, const STNBColor8 shadowColor);
		void		updateShadowColor(const STNBColor8 shadowColor);
		void		updateTag(const char* tag);
		void		updateTagAndColors(const char* tag, const STNBColor8 bgColor, const STNBColor8 fgColor);
		void		updateColorBg(const STNBColor8 bgColor);
		BOOL		updateIcons(AUTextura* texIcoLft, AUTextura* texIcoRght);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		//
		SI32				_iScene;
		//
		float				_widthMin;
		float				_heightMin;
		STNBColor8			_bgColor;
		STNBColor8			_fgColor;
		//
		AUEscenaSpriteElastico*	_bg;
		AUEscenaSpriteElastico*	_bgShadow;
		AUEscenaSprite*		_icoLft;
		AUFuenteRender*		_txtFnt;
		AUEscenaTexto*		_txt;
		AUEscenaSprite*		_icoRght;
		//
		void				privOrganizarContenido();
};

#endif /* TSVisualTag_hpp */
