//
//  AUGpMenuFinPartida.h
//  game-Refranero
//
//  Created by Marcos Ortega on 14/03/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#ifndef AUSceneBarBtm_h
#define AUSceneBarBtm_h

#include "IScenesListener.h"
#include "AUEscenaListaItemI.h"
#include "visual/ILobbyListnr.h"
#include "visual/gui/TSVisualTag.h"

class AUSceneBarBtm : public AUEscenaContenedorLimitado, public NBAnimador, public AUEscenaListaItemI, public IEscuchadorTouchEscenaObjeto {
	public:
		AUSceneBarBtm(const STLobbyRoot* root, const float anchoParaContenido, const float altoExtra);
		virtual				~AUSceneBarBtm();
		//
		float				altoExtra() const;
		void				establecerAltoExtra(const float altoExtra);
		//
		UI32				stacksVisibleCount();
		UI32				stacksVisibleIdxToGlobalIdx(const UI32 visibleIdx);
		//
		ENLobbyStack		seleccionActual() const;
		void				establecerSeleccionActual(const ENLobbyStack iSel);
		void				updateTagNumber(const ENLobbyStack iStack, const UI32 value);
		void				updateTagText(const ENLobbyStack iStack, const char* value);
		void				updateStyle(const BOOL havePatients, const BOOL organize);
		//
		AUEscenaObjeto*		itemObjetoEscena();
		STListaItemDatos	refDatos();
		void				establecerRefDatos(const SI32 tipo, const SI32 valor);
		void				organizarContenido(const float anchoParaContenido, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
		//
		void				tickAnimacion(float segsTranscurridos);
		//TOUCHES
		void				touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
		void				touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		void				touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		STLobbyRoot			_root;
		//
		float				_anchoParaContenido;
		float				_altoExtra;	//Barra de publicidad
		float				_marginIn;	//margin between icon and text
		//Bg
		struct {
			AUEscenaFigura*	obj;
			AUEscenaFigura*	shadow;
			STNBPoint		posAnimStart;
		} _bg;
		//Touch
		struct {
			STGTouch*		first;
			ENLobbyStack	iSel;
		} _touch;
		//Sel
		struct {
			ENLobbyStack	curIdx;
			AUEscenaSprite* bg;
			float			animOutDur;
			float			animOutCur;
			float			animInDur;
			float			animInCur;
		} _sel;
		//Btns
		struct {
			AUEscenaContenedor* lyr;	//for coloring by touch
			TSVisualTag*	tag;		//tag with number
			STNBString		tagValue;	//tag value
			AUEscenaSprite*	icon;
			AUEscenaTexto*	text;		//Next to icon
			AUEscenaTexto*	textMini;	//Below icon
			STNBPoint		textMiniPosUnsel;
			STNBColor		color;		//current color in float
			STNBColor		colorAnimStart;		//current color in float
			STNBPoint		posSel;
			STNBPoint		posUnsel;
			STNBPoint		posAnimStart;
			BOOL			hidden;
		} _btns[ENLobbyStack_Count];
		//
		void				privOrganizarContenido(const float anchoParaContenido);
		void				privOrganizeTag(const ENLobbyStack iStack);
};

#endif
