//
//  DRColumnOrchestor.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/gui/TSColumnOrchestor.h"
#include "visual/TSColors.h"
#include "visual/TSFonts.h"

DRColumnOrchestor::DRColumnOrchestor(const SI32 iScene) : AUObjeto() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "DRColumnOrchestor")
	_iScene	= iScene;
	NBArray_init(&_itms, sizeof(STTSColumnItm*), NULL);
	NBArray_init(&_steps, sizeof(STTSOrchestorStep), NULL);
	_anim	= new(this) AUAnimadorObjetoEscena();
}


DRColumnOrchestor::~DRColumnOrchestor(){
	if(_anim != NULL) _anim->liberar(NB_RETENEDOR_THIS); _anim = NULL;
	{
		SI32 i; for(i = 0; i < _itms.use; i++){
			STTSColumnItm* itm = NBArray_itmValueAtIndex(&_itms, STTSColumnItm*, i);
			if(itm != NULL){
				if(itm->obj != NULL){
					itm->obj->liberar(NB_RETENEDOR_THIS);
					itm->obj = NULL;
				}
				NBMemory_free(itm);
			}
		}
		NBArray_empty(&_itms);
		NBArray_release(&_itms);
	}
	{
		SI32 i; for(i = 0; i < _steps.use; i++){
			STTSOrchestorStep* step = NBArray_itmPtrAtIndex(&_steps, STTSOrchestorStep, i);
			NBString_release(&step->name);
			NBArray_release(&step->itms);
		}
		NBArray_empty(&_steps);
		NBArray_release(&_steps);
	}
}

//

STTSColumnItm* DRColumnOrchestor::privAddOrGetItm(AUEscenaObjeto* obj){
	STTSColumnItm* r = NULL;
	if(obj != NULL){
		//Search current
		if(r == NULL){
			SI32 i; for(i = 0; i < _itms.use; i++){
				STTSColumnItm* itm = NBArray_itmValueAtIndex(&_itms, STTSColumnItm*, i);
				if(itm != NULL){
					if(itm->obj == obj){
						r = itm;
						break;
					}
				}
			}
		}
		//Add new
		if(r == NULL){
			STTSColumnItm* itm = NBMemory_allocType(STTSColumnItm);
			NBMemory_setZeroSt(*itm, STTSColumnItm);
			itm->obj = obj; if(obj != NULL) obj->retener(NB_RETENEDOR_THIS);
			NBArray_addValue(&_itms, itm);
			//
			r = itm;
		}
	} NBASSERT(r != NULL)
	return r;
}

UI32 DRColumnOrchestor::getStepsCount() const {
	return _steps.use;
}

BOOL DRColumnOrchestor::openNewStep(const char* name){
	BOOL r = FALSE;
	{
		//Search current
		SI32 i; for(i = 0; i < _steps.use; i++){
			STTSOrchestorStep* step = NBArray_itmPtrAtIndex(&_steps, STTSOrchestorStep, i);
			if(NBString_strIsEqual(step->name.str, name)){
				break;
			}
		}
		if(i == _steps.use){
			STTSOrchestorStep step;
			NBMemory_setZeroSt(step, STTSOrchestorStep);
			NBString_initWithStr(&step.name, name);
			NBArray_init(&step.itms, sizeof(STTSColumnStepItm), NULL);
			NBArray_addValue(&_steps, step);
			//
			r = TRUE;
		}
	}
	return r;
}

BOOL DRColumnOrchestor::addItmToCurrentStep(AUEscenaObjeto* obj, AUEscenaListaItemI* itf, const NBMargenes margins, const ENDRColumnItmAlignV alignV){
	BOOL r = FALSE;
	if(_steps.use > 0){
		STTSOrchestorStep* step = NBArray_itmPtrAtIndex(&_steps, STTSOrchestorStep, _steps.use - 1);
		STTSColumnItm* itm = this->privAddOrGetItm(obj);
		if(itm != NULL){
			STTSColumnStepItm sitm;
			NBMemory_setZeroSt(sitm, STTSColumnStepItm);
			sitm.itm		= itm;
			sitm.itf		= itf;
			sitm.margins	= margins;
			sitm.alignV		= alignV;
			NBArray_addValue(&step->itms, sitm);
			//
			r = TRUE;
		}
	}
	return r;
}


BOOL DRColumnOrchestor::organizeStepAtIdx(const UI32 idx, const float width, const float availHeight, const BOOL anim, const float animSecs, NBCajaAABB* dstBox){
	BOOL r = FALSE;
	if(idx >= 0 && idx < _steps.use){
		STTSOrchestorStep* step = NBArray_itmPtrAtIndex(&_steps, STTSOrchestorStep, idx);
		//Organize
		{
			//Kill current animations
			_anim->quitarAnimaciones();
			//Mark all as not-used
			{
				SI32 i; for(i = 0; i < _itms.use; i++){
					STTSColumnItm* itm = NBArray_itmValueAtIndex(&_itms, STTSColumnItm*, i);
					itm->used = FALSE;
				}
			}
			//Organize items
			{
				//Calculate heights of top, center and bottom
				UI32 stretchItemsCount = 0;
				float heightTotal = 0.0f;
				float heights[ENDRColumnItmAlignV_Count];
				{
					SI32 iAlgn; for(iAlgn = 0; iAlgn < ENDRColumnItmAlignV_Count; iAlgn++){
						heights[iAlgn] = 0;
						{
							SI32 i; for(i = 0; i < step->itms.use; i++){
								STTSColumnStepItm* sitm = NBArray_itmPtrAtIndex(&step->itms, STTSColumnStepItm, i);
								if(sitm != NULL){
									if(sitm->itm != NULL){
										if(sitm->itm->obj != NULL){
											if(sitm->alignV == iAlgn){
												if(sitm->alignV == ENDRColumnItmAlignV_Stretch){
													stretchItemsCount++;
													heights[iAlgn] += sitm->margins.top + sitm->margins.bottom;
												} else {
													//Organize
													if(sitm->itf != NULL){
														sitm->itf->organizarContenido(width - sitm->margins.left - sitm->margins.right, 0.0f, FALSE, NULL, 0.0f);
													}
													//Calculate
													{
														const NBCajaAABB box = sitm->itm->obj->cajaAABBLocal();
														const float height = (box.yMax - box.yMin);
														if(height != 0.0f){
															heights[iAlgn] += sitm->margins.top + (box.yMax - box.yMin) + sitm->margins.bottom;
														}
													}
												}
											}
										}
									}
								}
							}
						}
						//
						heightTotal += heights[iAlgn];
					}
				}
				//Move objects
				{
					const BOOL allFitInAvailHeight = (heightTotal <= availHeight ? TRUE : FALSE);
					float yTop, yTopStart, yCenterTop, yCenterTopStart, yBtm, yBtmStart;
					if(allFitInAvailHeight){
						yTopStart		= 0.0f;
						yTop			= yTopStart;
						yBtmStart		= -availHeight;
						yBtm			= yBtmStart;
						if(stretchItemsCount <= 0){
							//Aligned to the center of the remaining space
							const float remainHeight = ((yTopStart - yBtmStart) - (heights[ENDRColumnItmAlignV_Top] + heights[ENDRColumnItmAlignV_Btm]));
							yCenterTopStart	= yTopStart - heights[ENDRColumnItmAlignV_Top] - (remainHeight * 0.5f);
						} else {
							//Aligned to top to let space for streching below
							yCenterTopStart = yTopStart - heights[ENDRColumnItmAlignV_Top];
						}
						yCenterTop		= yCenterTopStart;
					} else {
						//All aligned as top
						yTopStart		= 0.0f;
						yTop			= yTopStart;
						yCenterTopStart	= yTopStart - heights[ENDRColumnItmAlignV_Top];
						yCenterTop		= yCenterTopStart;
						yBtmStart		= yCenterTopStart - heights[ENDRColumnItmAlignV_Center] - heights[ENDRColumnItmAlignV_Btm];
						yBtm			= yBtmStart;
					}
					{
						SI32 i; for(i = 0; i < step->itms.use; i++){
							STTSColumnStepItm* sitm = NBArray_itmPtrAtIndex(&step->itms, STTSColumnStepItm, i);
							if(sitm != NULL){
								if(sitm->itm != NULL){
									if(sitm->itm->obj != NULL){
										const NBCajaAABB box = sitm->itm->obj->cajaAABBLocal();
										const float wObj = (box.xMax - box.xMin);
										const float hObj = (box.yMax - box.yMin);
										STNBPoint pos;
										pos.x = 0.0f - box.xMin + ((width - wObj) * 0.5f); //centered
										pos.y = 0.0f;
										switch (sitm->alignV) {
											case ENDRColumnItmAlignV_Top:
												//Already organized
												yTop -= sitm->margins.top;
												pos.y = yTop - box.yMax;
												yTop -= hObj + sitm->margins.bottom;
												break;
											case ENDRColumnItmAlignV_Center:
												//Already organized
												yCenterTop -= sitm->margins.top;
												pos.y = yCenterTop - box.yMax;
												yCenterTop -= hObj + sitm->margins.bottom;
												break;
											case ENDRColumnItmAlignV_Btm:
												//Already organized
												//Already organized
												yBtm += sitm->margins.bottom;
												pos.y = yBtm - box.yMin;
												yBtm += hObj + sitm->margins.top;
												break;
											case ENDRColumnItmAlignV_Stretch:
												//Not organized yet
												break;
											default:
												break;
										}
										//Positionate object
										{
											if(!anim || animSecs <= 0.0f || !sitm->itm->firstOrganized || !sitm->itm->obj->visible()){
												sitm->itm->obj->establecerTraslacion(pos.x, pos.y);
											} else {
												_anim->animarPosicion(sitm->itm->obj, pos.x, pos.y, animSecs);
											}
										}
										//Show object
										{
											if(!anim || animSecs <= 0.0f || !sitm->itm->firstOrganized){
												sitm->itm->obj->establecerVisible(TRUE);
												sitm->itm->obj->establecerMultiplicadorAlpha8(255);
											} else {
												if(!sitm->itm->obj->visible()){
													sitm->itm->obj->establecerMultiplicadorAlpha8(255);
													_anim->animarVisible(sitm->itm->obj, true, animSecs);
												} else {
													const NBColor8 color = sitm->itm->obj->multiplicadorColor8Func();
													_anim->animarColorMult(sitm->itm->obj, color.r, color.g, color.b, 255, animSecs);
												}
											}
										}
										sitm->itm->firstOrganized = TRUE;
									}
									sitm->itm->used = TRUE;
								}
							}
						}
					}
					//
					if(dstBox != NULL){
						dstBox->xMin = 0.0f;
						dstBox->xMax = width;
						dstBox->yMax = 0.0f;
						dstBox->yMin = yBtmStart;
					}
				}
				//
				r = TRUE;
			}
			//Hide unused items
			{
				SI32 i; for(i = 0; i < _itms.use; i++){
					STTSColumnItm* itm = NBArray_itmValueAtIndex(&_itms, STTSColumnItm*, i);
					if(!itm->used){
						if(itm->obj != NULL){
							//Hide object
							if(!anim || animSecs <= 0.0f || !itm->firstOrganized || itm->obj->multiplicadorAlpha8() <= 0){
								itm->obj->establecerVisible(FALSE);
							} else if(itm->obj->visible()){
								_anim->animarVisible(itm->obj, false, animSecs);
							}
						}
					}
				}
			}
		}
	}
	return r;
}

BOOL DRColumnOrchestor::organizeStep(const char* name, const float width, const float availHeight, const BOOL anim, const float animSecs, NBCajaAABB* dstBox){
	BOOL r = FALSE;
	//Search step
	{
		SI32 i; for(i = 0; i < _steps.use; i++){
			STTSOrchestorStep* step = NBArray_itmPtrAtIndex(&_steps, STTSOrchestorStep, i);
			if(NBString_strIsEqual(step->name.str, name)){
				r = this->organizeStepAtIdx(i, width, availHeight, anim, animSecs, dstBox);
				break;
			}
		}
	}
	return r;
}

//

AUOBJMETODOS_CLASESID_UNICLASE(DRColumnOrchestor)
AUOBJMETODOS_CLASESNOMBRES_UNICLASE(DRColumnOrchestor, "DRColumnOrchestor")
AUOBJMETODOS_CLONAR_NULL(DRColumnOrchestor)


