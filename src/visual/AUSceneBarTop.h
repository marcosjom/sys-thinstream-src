//
//  AUGpMenuFinPartida.h
//  game-Refranero
//
//  Created by Marcos Ortega on 14/03/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#ifndef AUSceneBarTop_h
#define AUSceneBarTop_h

#include "IScenesListener.h"
#include "AUEscenaListaItemI.h"
#include "visual/ILobbyListnr.h"
#include "visual/AUSceneContentI.h" //for STTSSceneBarStatus

typedef enum ENSceneBarTopSide_ {
	ENSceneBarTopSide_Left = 0,
	ENSceneBarTopSide_Center,
	ENSceneBarTopSide_Right,
	ENSceneBarTopSide_Count
} ENSceneBarTopSide;

typedef enum ENSceneBarTopIcon_ {
	ENSceneBarTopIcon_Back = 0,
	ENSceneBarTopIcon_Home,
	//ENSceneBarTopIcon_Settings,
	ENSceneBarTopIcon_Count
} ENSceneBarTopIcon;

typedef enum ENSceneSubbarIcon_ {
	ENSceneSubbarIcon_Sync = 0,
	ENSceneSubbarIcon_Count
} ENSceneSubbarIcon;

//Right side icons

typedef enum ENSceneBarIcon_ {
	ENSceneBarIcon_Add = 0,
	ENSceneBarIcon_Info,
	ENSceneBarIcon_Doc,
	ENSceneBarIcon_Count
} ENSceneBarIcon;

typedef enum ENSceneBarIconBit_ {
	ENSceneBarIconBit_Add		= (0x1 << ENSceneBarIcon_Add),
	ENSceneBarIconBit_Info		= (0x1 << ENSceneBarIcon_Info),
	ENSceneBarIconBit_Doc		= (0x1 << ENSceneBarIcon_Doc),
	ENSceneBarIconBit_All		= (
								   ENSceneBarIconBit_Add
								   | ENSceneBarIconBit_Info
								   | ENSceneBarIconBit_Doc
								   )
} ENSceneBarIconBit;

typedef struct STSceneBarTopIcon_ {
	STNBPoint			posOut;
	STNBPoint			posIn;
	ENSceneBarTopSide	side;
	AUEscenaSprite*		icon;
	STNBColor8			color;
	float				alpha;
	float				xLeftTouch;
} STSceneBarTopIcon;

typedef struct STSceneBarTopTitle_ {
	STNBPoint			posOut;
	STNBPoint			posIn;
	STNBString			strText;
	AUEscenaTexto*		text;
	STNBColor8			color;
	float				alpha;
} STSceneBarTopTitle;

//

class AUSceneBarTop: public AUEscenaContenedorLimitado, public NBAnimador, public IEscuchadorTouchEscenaObjeto, public AUEscenaListaItemI {
	public:
		AUSceneBarTop(const STLobbyRoot* root, const float width, const float altoExtra);
		virtual				~AUSceneBarTop();
		//
		STNBAABox			bgBox() const;
		float				extraHeight() const;
		void				setExtraHeight(const float extraHeight);
		//Status
		UI32				getStatusRightIconMask() const;
		BOOL				getStatusDef(STTSSceneBarStatus* dst) const;
		void				setStatusDef(const STTSSceneBarStatus statusDef);
		//Substatus
		BOOL				isSubstatusVisible() const;
		void				setSubstatusVisible(const BOOL visible);
		void				setSubstatus(const ENSceneSubbarIcon icon, const char* text, const STNBColor8 bgColor, const STNBColor8 fgColor);
		//
		AUEscenaObjeto*		itemObjetoEscena();
		STListaItemDatos	refDatos();
		void				establecerRefDatos(const SI32 tipo, const SI32 valor);
		void				organizarContenido(const float width, const float height, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
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
		NBMargenes			_mrgns;
		float				_extraHeight;	//Barra de telefono
		float				_width;
		//
		AUEscenaFigura*		_bg;
		AUEscenaFigura*		_bg2;
		AUEscenaFigura*		_shadow;
		STNBAABox			_bgBox;
		//
		ENSceneBarTopIcon	_iconIdxBySide[ENSceneBarTopSide_Count];
		STSceneBarTopIcon	_icons[ENSceneBarTopIcon_Count];
		STSceneBarTopTitle	_title[2];
		SI32				_titleIdx;
		STTSSceneBarStatus	_curStatusDef;
		struct {
			float			xLeft;
			float			xRight;
		} _left;
		struct {
			float			xLeft;
			float			xRight;
			float			yTop;
			float			height;
			STSceneBarTopIcon icons[ENSceneBarIcon_Count];
		} _right;
		struct {
			STGTouch*		first;
			ENSceneBarTopSide side;
			ENSceneBarIcon	iconRight;
		} _touch;
		//Substatus
		struct {
			AUEscenaContenedorLimitado* lyr;
			AUEscenaSprite*	bg;
			STNBAABox		bgBox;
			AUEscenaTexto*	text;
			STNBString		textValue;
			STNBColor8		bgColor;
			STNBColor8		fgColor;
			float			alphaRel;
			float			rot;
			BOOL			show;
			ENSceneSubbarIcon icon;
			AUEscenaSprite*	icons[ENSceneSubbarIcon_Count];
		} _substatus;
		//
		void				privOrganizarContenido(const float width, const bool animar, const bool forzarPosicion);
		//Right icons
		void				privRightCalculateIconsPos();
		//Title
		void				privCropTitles();
		void				privCropTitle(const UI32 iTitle);
};

#endif
