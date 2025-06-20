//
//  AUSceneIntro.hpp
//  thinstream-test
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifndef AUSceneIntro_hpp
#define AUSceneIntro_hpp

#include "IScenesListener.h"
#include "AUIButton.h"
#include "nb/2d/NBBitmap.h"
#include "core/client/TSClient.h"
#include "core/logic/TSDevice.h"
#include "visual/TSLogoMetrics.h"
#include "visual/TSColors.h"
#include "visual/AUSceneIntroDefs.h"


#define THINSTREAM_BUILD_ID		1			//Update this value every compilation-to-share
#define THINSTREAM_BUILD_DATE	__DATE__	//Update this value every compilation-to-share

typedef struct STSceneInfoMetrics_ {
	float		formWidth;	//form width
	float		boxWidth;	//other content width (<= formWidth)
	float		sepH;		//Horizontal separation
	float		sepV;		//Vertical separation
	float		marginH;	//Scene margin width
	float		marginV;	//Scene margin height
} STSceneInfoMetrics;

//Content

typedef enum ENSceneInfoContent_ {
	ENSceneInfoContent_NotifsAuth,			//Request for notifications authorization
	//
	ENSceneInfoContent_TermsAccept,				//Terms and conditions
	//
	ENSceneInfoContent_Conteo
} ENSceneInfoContent;

//Content buttons def

typedef enum ENSceneInfoBtnVisibility_ {
	ENSceneInfoBtnVisibility_Allways = 0,
	ENSceneInfoBtnVisibility_Hidden,
	ENSceneInfoBtnVisibility_Count
} ENSceneInfoBtnVisibility;

typedef struct STSceneInfoBtnDef_ {
	ENTSColor					bgColor;
	ENTSColor					fgColor;
	ENSceneInfoBtnVisibility	visibility;	//AllwaysVisible or Hidden
	STNBString					text;		//ButtonText
	AUTextura*					icon;		//ButtonIcon
	STNBString					optionId;	//OptionId
} STSceneInfoBtnDef;

typedef struct STSceneInfoBtn_ {
	ENTSColor					bgColor;
	ENTSColor					fgColor;
	ENSceneInfoBtnVisibility	visibility;	//AllwaysVisible or Hidden
	AUIButton*					btn;		//ButtonText
	STNBPoint					btnPos;		//Natural position
	STNBString					optionId;	//OptionId
} STSceneInfoBtn;

//Coontent base def

typedef struct STSceneInfoContentBaseDef_ {
	ENTSColor				bgColor;
	ENTSColor				backColor;		//Show "back" button
	BOOL					showTopLogo;	//Show top-logo
	BOOL					showSteps;		//Show steps
	BOOL					showAsStep;		//Show step at progress bar
	BOOL					showResetDataBtn;
	BOOL					titleCentered;	//Force the title and text to be centered.
	STNBString				shortName;		//Short name at steps
	STNBString				title;			//Big red text
	STNBString				explainTop;		//Small explanation text
	STNBString				privacyTitle;	//Privacy link name and title
	STNBString				privacyText;	//Privacy text
	ENNBTextLineAlignH		privacyAlign;
	ENNBTextAlignV			buttonsPos;
	STNBArray				buttons;		//STSceneInfoBtnDef
	STNBArray				txtBoxes;		//AUITextBox*
	STNBArray				touchables;		//AUEscenaObjeto*
} STSceneInfoContentBaseDef;

typedef struct STSceneInfoContentBase_ {
	ENTSColor				bgColor;
	ENTSColor				backColor;		//Show "back" button
	BOOL					showTopLogo;	//Show top-logo
	BOOL					showSteps;		//Show steps
	BOOL					showAsStep;		//Show step at progress bar
	BOOL					showResetDataBtn;
	BOOL					titleCentered;	//Force the title and text to be centered.
	STNBString				shortName;		//Short name at steps
	AUEscenaTexto*			title;			//Big red text
	STNBPoint				titlePos;		//Natural position
	AUEscenaTexto*			explainTop;		//Small explanation text
	STNBPoint				explainTopPos;	//Natural position
	STNBString				privacyTitle;	//Privacy link name and title
	STNBString				privacyText;	//Privacy text
	ENNBTextLineAlignH		privacyLnkAlign;
	AUEscenaTexto*			privacyLnk;			//link to privacy info
	STNBPoint				privacyLnkPos;		//Natural position
	AUEscenaFigura*			privacyLnkLine;		//Underline the link
	STNBPoint				privacyLnkLinePos;	//Natural position
	AUEscenaSprite*			privacyLnkIco;		//Info icon
	STNBPoint				privacyLnkIcoPos;	//Natural position
	ENNBTextAlignV			buttonsPos;
	STNBArray				buttons;		//STSceneInfoBtn
	STNBArray				txtBoxes;		//AUITextBox*
	STNBArray				touchables;		//AUEscenaObjeto*
} STSceneInfoContentBase;

//Content

typedef struct STSceneInfoContent_ {
	ENSceneInfoContent			uid;
	STSceneInfoContentBase		base;
	AUEscenaContenedor*			layer;
	STNBPoint					layerPos;				//NaturalPos
	NBCajaAABB					layerSceneBox;			//Scene-box (to calculate keyboard obstruction)
	AUEscenaContenedorLimitado*	formLayerLimit;			//This allows visualy move the form without lossing the layout
	STNBPoint					formLayerLimitPos;		//NaturalPos
	NBCajaAABB					formLayerLimitSceneBox;	//Scene-box (to calculate keyboard obstruction)
	AUEscenaContenedor*			formLayerGlassLayer;	//
	AUEscenaSprite*				formLayerGlass;			//To hide everything behing the form
	AUEscenaContenedor*			formLayer;
	STNBPoint					formLayerPos;			//NaturalPos
	AUEscenaContenedor*			btnsLayer;
	void*						data;
} STSceneInfoContent;


//Step indicator

typedef struct STSceneInfoStepInd_ {
	UI32				iStep;
	AUEscenaSprite*		circle;
	AUEscenaFigura*		lineToNext;
	AUEscenaTexto*		text;
} STSceneInfoStepInd;

typedef struct STSceneInfoStepsInds_ {
	AUEscenaContenedorLimitado* layer;
	STNBPoint			layerPos;	//NaturalPos
	AUEscenaContenedor* layerLines;
	BOOL				layerVisible;
	STNBArray			inds;		//STSceneInfoStepInd
} STSceneInfoStepsInds;

//Stack of steps

struct STSceneInfoStack_;

typedef struct STSceneInfoStackItm_ {
	STSceneInfoContent			content;
	struct STSceneInfoStack_*	subStack;
} STSceneInfoStackItm;

typedef struct STSceneInfoStack_ {
	SI32					iCurStep;			//Current step
	STNBArray				stepsSequence;		//ENSceneInfoContent, steps list
	STSceneInfoStepsInds	stepsIndicators;	//Steps indicators
	STNBArray				contentStack;		//STSceneInfoStackItm*
} STSceneInfoStack;

//Anims

typedef struct STSceneInfoContentAnim_ {
	STSceneInfoStackItm*	stackItm;	//Stack item
	AUAnimadorObjetoEscena* animator;	//Parts animator
	BOOL					isOutAnim;	//Animation is out? (or in)
} STSceneInfoContentAnim;

typedef enum ENSceneInfoUpdateStateType_ {
	ENSceneInfoUpdateStateType_Organize = 0,	//Update state triggered by organizinf content
	ENSceneInfoUpdateStateType_FocusGain,		//Gained focus
	ENSceneInfoUpdateStateType_FocusLost,		//Lost focus
	ENSceneInfoUpdateStateType_TextValueChanged,//Text value changed
	ENSceneInfoUpdateStateType_Count
} ENSceneInfoUpdateStateType;

typedef struct STSceneInfoNamePartIdx_ {
	SI32		idx;			//start of part in nameParts string.
	SI32		timesFound;		//how many times have been found
} STSceneInfoNamePartIdx;

class AUSceneIntro: public AUAppEscena, public NBAnimador, public IEscuchadorCambioPuertoVision, public IEscuchadorTouchEscenaObjeto, public IEscuchadorIBoton, public IEscuchadorITextBox, public IEscuchadorTecladoVisual, public IEscuchadorIMsgBox, public IReqPopListener, public IPopListener {
	public:
		AUSceneIntro(STTSClient* coreClt, SI32 iScene, IScenesListener* escuchador, const ENSceneInfoMode mode);
		virtual		~AUSceneIntro();
		//
		bool		escenaEnPrimerPlano();
		void		escenaColocarEnPrimerPlano();
		void		escenaQuitarDePrimerPlano();
		void		escenaGetOrientations(UI32* dstMask, ENAppOrientationBit* dstPrefered, ENAppOrientationBit* dstToApplyOnce, BOOL* dstAllowAutoRotate);
		bool		escenaPermitidoGirarPantalla();
		bool		escenaEstaAnimandoSalida();
		void		escenaAnimarEntrada();
		void		escenaAnimarSalida();
		//
		//void		appProcesarNotificacionLocal(const char* grp, const SI32 localId, const char* objTip, const SI32 objId);
		//
		void		puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after);
		//
		void		tickAnimacion(float segsTranscurridos);
		//
		void		botonPresionado(AUIButton* obj);
		void		botonAccionado(AUIButton* obj);
		AUEscenaObjeto*	botonHerederoParaTouch(AUIButton* obj, const NBPunto &posInicialEscena, const NBPunto &posActualEscena);
		//
		void		textboxFocoObtenido(AUITextBox* obj);
		void		textboxFocoPerdido(AUITextBox* obj, const ENNBKeybFocusLostType actionType);
		bool		textboxMantenerFocoAnteConsumidorDeTouch(AUITextBox* obj, AUEscenaObjeto* consumidorTouch);
		void		textboxContenidoModificado(AUITextBox* obj, const char* strContenido);
		//
		void		msgboxOptionSelected(AUIMsgBox* obj, const char* optionId);
		void		msgboxCloseSelected(AUIMsgBox* obj);
		void		msgboxFocusObtained(AUIMsgBox* obj);
		void		msgboxFocusLost(AUIMsgBox* obj);
		//
		void		popOptionSelected(void* param, void* ref, const char* optionId);
		void		popFocusObtained(void* param, void* ref);
		void		popFocusLost(void* param, void* ref);
		//IReqPopListener
		void		reqPopEnded(const UI64 reqId, void* param, const STTSClientChannelRequest* req);
		//Record changed
		static void	recordChanged(void* param, const char* relRoot, const char* relPath);
		//TOUCHES
		void		touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
		void		touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		void		touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		//TECLAS
		bool		teclaPresionada(SI32 codigoTecla);
		bool		teclaLevantada(SI32 codigoTecla);
		bool		teclaEspecialPresionada(SI32 codigoTecla);
		//Visual keyboard
		void		tecladoVisualAltoModificado(const float altoPixeles);
		//
		void		privContentDestroy(const ENSceneInfoContent uid, void* data);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		SI32				_iScene;
		IScenesListener* 	_listener;
		STTSClient*			_coreClt;
		AUAnimadorObjetoEscena* _animator;
		AUEscenaContenedor*	_layerContent;
		//
		bool				_atFocus;
		NBCajaAABB			_lastBox;
		NBCajaAABB			_lastBoxUsable;
		ENStatusBarStyle	_barStyle;
		//
		STNBString			_tmpFormatString;
		//Bg
		STNBColor8			_bgColor;
		AUEscenaSprite*		_bg;
		//
		AUTextura*			_texIcoErr;
		//Back
		struct {
			AUEscenaContenedor* layer;
			STNBPoint			layerPos;	//NaturalPos
			STNBColor8			layerColor;
			AUEscenaSprite*		sprite;
			AUEscenaTexto*		text;
			AUEscenaSprite*		dummy;	//to expand touch area
		} _back;
		//TopRightLogo
		struct {
			float				pathScale;	//of current loaded texture
			AUEscenaSprite* 	sprite;
			STNBPoint			spritePos;	//NaturalPos
		} _topLogo;
		//Content array
		struct {
			AUEscenaContenedor*	lyr;
			STSceneInfoStack	templatesStack; //Preloaded itms
			STSceneInfoStack*	mainStack;	//Main stack definition
			STNBArray			stacks;		//STSceneInfoStack*, stack of stack
			//
			float				yTopBeforeContent;
			float				yBtmBeforeContent;
			float				yBtmBeforeSteps;
			//
			STNBArray			anims;		//STSceneInfoContentAnim
		} _contents;
		//Reset btn
		struct {
			AUIButton*		btn;
			void*			popRef;
		} _resetData;
		//Shared-data
		struct {
			BOOL			recordChanged;		//data record was changed
		} _data;
		//Request for new device (secondary devices)
		struct {
			STNBThreadMutex			mutex;
			//Verification (step 1)
			struct {
				UI64				requestId;
			} verif;
			//Confirmation (step 2)
			struct {
				BOOL					isBuilding;		//is currently building a request (in a secondary thread)
				BOOL					isBuildingLast;	//value known form previoud tick
				void*					popBuildingRef;
				void*					popConfirmRef;
				STNBPKey*				pkey;			//Private key
				STNBX509Req*			csr;			//Csr
				UI64					requestId;
			} confirm;
		} _reqInvitation;
		void*				_popupRef;		//reference to a multi-purpose-popup
		//
		float				_keybHeight;
		float				_widthToUse;
		float				_heightToUse;
		STSceneInfoMetrics	_metrics;
		//
		void				privCalculateLimits();
		void				privOrganizeContent();
		//Content metrics
		STSceneInfoMetrics	privFormMetrics() const;
		//Camera
		float				privGetScaledSize(const NBCajaAABB fitOnBox, const NBCajaAABB objBox, const float rotation);
		STNBAABox			privGetTexturePortion(const STNBAABox scnBox, const STNBSize texSz);
		//
		void				privAddStepToSequence(STSceneInfoStack* obj, const ENSceneInfoContent stepId, STSceneInfoStack* stepsTemplates, AUFuenteRender* fntStepName);
		//
		STSceneInfoBtn*		privGetButtonByActionId(STSceneInfoStackItm* itm, const char* buttonActionId);
		void				privSetButtonVisibleByActionId(STSceneInfoStackItm* itm, const char* buttonActionId, const bool visible, const BOOL animate);
		void				privSetContentGlassVisible(STSceneInfoStackItm* itm, const bool visible, const BOOL animate);
		//Stack
		STSceneInfoStack*		privCurStack();
		STSceneInfoStackItm*	privCurStackCurItm();
		STSceneInfoContent*		privCurStackCurContent();
		void					privStackPushContent(STSceneInfoStack* obj, const ENSceneInfoContent uid);
		//
		static BOOL			isValidIMEI(const char* str, const UI32 strLen, STNBString* dstFilteredDigits);
		//Content
		void*				privContentCreate(const UI32 curStackSize, const ENSceneInfoContent uid, AUEscenaContenedor* layer, STSceneInfoContentBaseDef* dstBaseDef, const STNBSize sceneSz, const ENTSScreenOrient orient);
		void				privContentUpdateAfterResize(const ENSceneInfoContent uid, void* data);
		void				privContentOrganizeItm(STSceneInfoStackItm* itm);
		void				privContentOrganize(STSceneInfoStackItm* itm, const STNBSize availSz, const ENTSScreenOrient orient);
		void				privContentUpdateState(STSceneInfoStackItm* itm, AUEscenaObjeto* obj, const ENSceneInfoUpdateStateType type, const BOOL animate);
		void				privContentTick(STSceneInfoStackItm* itm, const float secs);
		void				privContentTouched(STSceneInfoStackItm* itm, const AUEscenaObjeto* obj, const NBPunto posActualEscena);
		void				privContentAction(STSceneInfoStackItm* itm, const char* optionId);
		void				privContentPopOptionSelected(STSceneInfoStackItm* itm, void* popRef, const char* optionId);
		void				privContentRecordChanged(STSceneInfoStackItm* itm);
		void				privContentShow(const ENSceneInfoContent uid, void* data, const float secsWait, const float secsAnim);
		void				privContentHide(const ENSceneInfoContent uid, void* data, const float secsWait, const float secsAnim);
		void				privContentShow(STSceneInfoStackItm* itm, const float secsAnim, const float secsWait);
		void				privContentHide(STSceneInfoStackItm* itm, const float secsAnim, const float secsWait);
		void				privContentExchangeFromTo(STSceneInfoStackItm* curItm, STSceneInfoStackItm* newItm);
		void				privContentPushNewStack(const ENSceneInfoContent stepId, const BOOL ignoreIfAnimating);
		void				privContentPushNewStack(const ENSceneInfoContent* stepsId, const SI32 stepsIdSz, const BOOL ignoreIfAnimating);
		void				privContentPopCurStack(const BOOL ignoreIfAnimating);
		void				privContentPopCurStackAndNext(const BOOL ignoreIfAnimating);
		void				privContentShowPrev(const UI8 steps, const BOOL ignoreIfAnimating, const BOOL ignoreIfBuildingId);
		void				privContentShowNext(const BOOL ignoreIfAnimating);
		BOOL				privContentCurrentIsLast();
		//
		const char*			privGetTmpStringByReplacing(const char* str, const char* find, const char* replace);
		const char*			privGetTmpStringByReplacing2(const char* str, const char* find, const char* replace, const char* find2, const char* replace2);
		//
		void				privConfirmFlowStart();
		void				privConfirmFlowConfirmed();
		void				privConfirmBuildingPopShow();
		void				privConfirmBuildingErrorPopShow();
		void				privConfirmReqStart(const void* pkeyDER, const UI32 pkeyDERSz, const void* csrDER, const UI32 csrDERSz);
};

#endif /* AUSceneIntro_hpp */
