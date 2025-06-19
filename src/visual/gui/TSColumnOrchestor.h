//
//  DRColumnOrchestor.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef DRColumnOrchestor_h
#define DRColumnOrchestor_h

#include "nb/core/NBCallback.h"
#include "AUEscenaContenedor.h"

typedef enum ENDRColumnItmAlignV_ {
	ENDRColumnItmAlignV_Top = 0,	//Align to top
	ENDRColumnItmAlignV_Center,		//Align to center
	ENDRColumnItmAlignV_Stretch,	//Use all the remaining space
	ENDRColumnItmAlignV_Btm,		//Align to btm
	ENDRColumnItmAlignV_Count
} ENDRColumnItmAlignV;

typedef struct STTSColumnItm_ {
	AUEscenaObjeto*			obj;
	BOOL					used;	//used on an 'organize' request
	BOOL					firstOrganized;	//has been organized visible since added at scene? (animate or not?)
} STTSColumnItm;

typedef struct STTSOrchestorStep_ {
	STNBString				name;
	STNBArray				itms; //STTSColumnStepItm
} STTSOrchestorStep;

typedef struct STTSColumnStepItm_ {
	STTSColumnItm*			itm;
	AUEscenaListaItemI*		itf;
	NBMargenes				margins;
	ENDRColumnItmAlignV 	alignV;
} STTSColumnStepItm;

class DRColumnOrchestor: public AUObjeto {
	public:
		DRColumnOrchestor(const SI32 iScene);
		virtual		~DRColumnOrchestor();
		//
		UI32		getStepsCount() const;
		BOOL		openNewStep(const char* name);
		BOOL		addItmToCurrentStep(AUEscenaObjeto* obj, AUEscenaListaItemI* itf, const NBMargenes margins, const ENDRColumnItmAlignV alignV);
		//
		BOOL		organizeStepAtIdx(const UI32 idx, const float width, const float availHeight, const BOOL anim, const float animSecs, NBCajaAABB* dstBox);
		BOOL		organizeStep(const char* name, const float width, const float availHeight, const BOOL anim, const float animSecs, NBCajaAABB* dstBox);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		SI32		_iScene;
		STNBArray	_itms; //STTSColumnItm*
		STNBArray	_steps; //STTSOrchestorStep
		AUAnimadorObjetoEscena* _anim;
		//
		STTSColumnItm* privAddOrGetItm(AUEscenaObjeto* obj);
};

#endif /* DRColumnOrchestor_h */
