//
//  TSVisualStatus.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSVisualStatus_h
#define TSVisualStatus_h

#include "nb/core/NBCallback.h"
#include "AUEscenaContenedor.h"
#include "client/TSClient.h"

typedef struct STTSVisualStatusModePropsBase_ {
	BOOL		bgIsSolid;
	BOOL		borderSolid;
	BOOL		animColor;
	float		velRot;
	float		scale;
	float		rot;
} STTSVisualStatusModePropsBase;

typedef struct STTSVisualStatusModeProps_ {
	BOOL		isSet;
	AUTextura*	tex;
	const char*	text;
	STTSVisualStatusModePropsBase base;
} STTSVisualStatusModeProps;

typedef enum ENTSVisualStatusMode_ {
	//Mini
	ENTSVisualStatusMode_LoadMini = 0,
	ENTSVisualStatusMode_DecryptMini,
	ENTSVisualStatusMode_ErrorMini,
	//With text
	ENTSVisualStatusMode_NoAccess,		//User have no access to file
	ENTSVisualStatusMode_Downloading,	//Currently downloading
	ENTSVisualStatusMode_Decrypting,	//Currently decrypting
	ENTSVisualStatusMode_Loading,		//Currently loading decrypted content
	//
	ENTSVisualStatusMode_Count
} ENTSVisualStatusMode;

class TSVisualStatus: public AUEscenaContenedorLimitado, public AUEscenaListaItemI, public NBAnimador {
	public:
		TSVisualStatus(STTSClient* client, const float anchoUsar, const float altoUsar);
		TSVisualStatus(STTSClient* client, const float anchoUsar, const float altoUsar, const STNBColor8 prefColor);
		virtual				~TSVisualStatus();
		//
		void				setMargins(const float marginH, const float marginV);
		//
		ENTSVisualStatusMode getMode() const;
		void				setMode(const ENTSVisualStatusMode mode);
		//
		STNBColor8			getPrefColor() const;
		void				setPrefColor(const STNBColor8 prefColor);
		//AUEscenaListaItemI
		AUEscenaObjeto*		itemObjetoEscena();
		STListaItemDatos	refDatos();
		void				establecerRefDatos(const SI32 type, const SI32 value);
		void				organizarContenido(const float width, const float height, const bool notifyHeightChange, AUAnimadorObjetoEscena* animator, const float secsAnim);
		//
		void				tickAnimacion(float segsTranscurridos);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		STTSClient*				_client;
		STListaItemDatos		_refDatos;
		float					_anchoUsar;
		float					_altoUsar;
		float					_marginH;
		float					_marginV;
		//
		AUFuenteRender*			_fntNames;
		STNBColor8				_prefColor;
		//objects
		struct {
			ENTSVisualStatusMode mode;
			STNBColor8			color;
			float				rot;
			float				alphaRel;
			BOOL				alphaInc;
			AUEscenaSprite*		bg;
			AUEscenaFigura*		border;
			AUEscenaSprite*		ico;
			AUEscenaTexto*		text;
		} _objs;
		//
		void					privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
		void					privSetMode(const ENTSVisualStatusMode iMode);
		static const STTSVisualStatusModeProps privGetProps(STTSClient* coreContext, const ENTSVisualStatusMode iMode);
		static const STTSVisualStatusModePropsBase privGetPropsBase(const ENTSVisualStatusMode iMode);
};

#endif /* TSVisualStatus_h */
