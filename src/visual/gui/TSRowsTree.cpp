//
//  DRRowsTree.c
//  thinstream
//
//  Created by Marcos Ortega on 4/14/19.
//

#include "visual/TSVisualPch.h"
#include "visual/gui/TSRowsTree.h"
//
#include "visual/TSColors.h"
#include "visual/TSFonts.h"

//Node

typedef enum ENDRRowsTreeNodeBtnPart_ {
	ENDRRowsTreeNodeBtnPart_LeftIcon = 0,
	ENDRRowsTreeNodeBtnPart_LeftObj,
	ENDRRowsTreeNodeBtnPart_LeftText,
	ENDRRowsTreeNodeBtnPart_RightText,
	ENDRRowsTreeNodeBtnPart_RightObj,
	ENDRRowsTreeNodeBtnPart_RightIcon,
	ENDRRowsTreeNodeBtnPart_Count
} ENDRRowsTreeNodeBtnPart;

void DRRowsTreeNode_init(STTSRowsTreeNode* obj){
	obj->parent			= NULL;
	//
	obj->btn			= NULL;
	//
	obj->leftIco		= NULL;
	obj->leftObj		= NULL;
	obj->leftObjItf		= NULL;
	//
	obj->rightObj		= NULL;
	obj->rightObjItf	= NULL;
	obj->rightIco		= NULL;
	//
	NBString_init(&obj->valueStr);
	obj->valueUInt		= 0;
	obj->children		= NULL;
}

void DRRowsTreeNode_release(STTSRowsTreeNode* obj){
	obj->parent		= NULL;
	//
	if(obj->btn != NULL)		obj->btn->liberar(NB_RETENEDOR_NULL); obj->btn = NULL;
	//Left
	obj->leftObjItf = NULL;
	if(obj->leftObj != NULL)	obj->leftObj->liberar(NB_RETENEDOR_NULL); obj->leftObj = NULL;
	if(obj->leftIco != NULL)	obj->leftIco->liberar(NB_RETENEDOR_NULL); obj->leftIco = NULL;
	//Right
	obj->rightObjItf = NULL;
	if(obj->rightObj != NULL)	obj->rightObj->liberar(NB_RETENEDOR_NULL); obj->rightObj = NULL;
	if(obj->rightIco != NULL)	obj->rightIco->liberar(NB_RETENEDOR_NULL); obj->rightIco = NULL;
	//
	NBString_release(&obj->valueStr);
	obj->valueUInt		= 0;
	if(obj->children != NULL){
		DRRowsTree_release(obj->children);
		NBMemory_free(obj->children);
		obj->children = NULL;
	}
}

//

void DRRowsTree_init(STTSRowsTree* obj){
	NBArray_init(&obj->nodes, sizeof(STTSRowsTreeNode*), NULL);
	NBArray_init(&obj->pend, sizeof(STTSRowsTreeNode*), NULL);
}

void DRRowsTree_release(STTSRowsTree* obj){
	{
		SI32 i; for(i = 0; i < obj->pend.use; i++){
			STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
			if(n != NULL){
				DRRowsTreeNode_release(n);
				NBMemory_free(n);
				n = NULL;
			}
		}
		NBArray_empty(&obj->pend);
		NBArray_release(&obj->pend);
	}
	{
		SI32 i; for(i = 0; i < obj->nodes.use; i++){
			STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->nodes, STTSRowsTreeNode*, i);
			if(n != NULL){
				DRRowsTreeNode_release(n);
				NBMemory_free(n);
				n = NULL;
			}
		}
		NBArray_empty(&obj->nodes);
		NBArray_release(&obj->nodes);
	}
}

//

void DRRowsTree_fillStart(STTSRowsTree* obj){
	//Move all rows to pend
	NBASSERT(obj->pend.use == 0)
	if(obj->pend.use == 0 && obj->nodes.use > 0){
		//Swap nodes to pend
		NBArray_addItems(&obj->pend, NBArray_data(&obj->nodes), sizeof(STTSRowsTreeNode*), obj->nodes.use);
		NBArray_empty(&obj->nodes);
	}
}

void DRRowsTree_fillEnd(STTSRowsTree* obj){
	//Release all not consumed rows
	{
		SI32 i; for(i = 0; i < obj->pend.use; i++){
			STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
			if(n != NULL){
				DRRowsTreeNode_release(n);
				NBMemory_free(n);
				n = NULL;
			}
		}
		NBArray_empty(&obj->pend);
	}
}

//

STTSRowsTreeNode* DRRowsTree_addPendRowByValueStr(STTSRowsTree* obj, const char* valueStr){
	STTSRowsTreeNode* r = NULL;
	SI32 i; for(i = 0; i < obj->pend.use; i++){
		STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
		NBASSERT(n->btn != NULL)
		if(NBString_isEqual(&n->valueStr, valueStr)){
			//Move from "pend" to "nodes"
			n->parent = NULL;
			NBArray_addValue(&obj->nodes, n);
			NBArray_removeItemAtIndex(&obj->pend, i);
			r = n;
			break;
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTree_addPendRowByValueUInt(STTSRowsTree* obj, const UI32 valueUInt){
	STTSRowsTreeNode* r = NULL;
	SI32 i; for(i = 0; i < obj->pend.use; i++){
		STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
		NBASSERT(n->btn != NULL)
		if(n->valueUInt == valueUInt){
			//Move from "pend" to "nodes"
			n->parent = NULL;
			NBArray_addValue(&obj->nodes, n);
			NBArray_removeItemAtIndex(&obj->pend, i);
			r = n;
			break;
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTree_addPendRowByValueStrAndUInt(STTSRowsTree* obj, const char* valueStr, const UI32 valueUInt){
	STTSRowsTreeNode* r = NULL;
	SI32 i; for(i = 0; i < obj->pend.use; i++){
		STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
		NBASSERT(n->btn != NULL)
		if(n->valueUInt == valueUInt){
			if(NBString_isEqual(&n->valueStr, valueStr)){
				//Move from "pend" to "nodes"
				n->parent = NULL;
				NBArray_addValue(&obj->nodes, n);
				NBArray_removeItemAtIndex(&obj->pend, i);
				r = n;
				break;
			}
		}
	}
	return r;
}

void DRRowProps_setDefaultsForList(STTSRowProps* obj, const SI32 iScene){
	NBMemory_setZeroSt(*obj, STTSRowProps);
	obj->fntLeft	= TSFonts::font(ENTSFont_ContentTitle);
	obj->fntRight	= TSFonts::font(ENTSFont_ContentTitle);
	//
	obj->colorBgN	= TSColors::colorDef(ENTSColor_White)->normal;
	obj->colorBgH	= TSColors::colorDef(ENTSColor_Light)->normal;
	obj->colorBgT	= TSColors::colorDef(ENTSColor_LightFront)->normal;
	//
	obj->colorLeft	= TSColors::colorDef(ENTSColor_Gray)->normal;
	obj->colorRight	= TSColors::colorDef(ENTSColor_Black)->normal;
	//
	obj->margins.left	= obj->margins.right = NBGestorEscena::anchoPulgadasAEscena(iScene, 0.07f);
	obj->margins.top	= obj->margins.bottom = obj->margins.left;
	obj->marginI		= NBGestorEscena::anchoPulgadasAEscena(iScene, 0.10f);
}

STTSRowsTreeNode* DRRowsTree_addNewRow(STTSRowsTree* obj, const STTSRowProps* props, const char* leftTxt, const char* rightTxt, AUEscenaObjeto* rightObj, AUEscenaListaItemI* rightObjItf, AUTextura* rightIco, const char* valStr, const SI32 valUInt){
	STTSRowsTreeNode* r = NULL;
	{
		NBColor8 colorN; NBCOLOR_ESTABLECER(colorN, props->colorBgN.r, props->colorBgN.g, props->colorBgN.b, props->colorBgN.a)
		NBColor8 colorH; NBCOLOR_ESTABLECER(colorH, props->colorBgH.r, props->colorBgH.g, props->colorBgH.b, props->colorBgH.a)
		NBColor8 colorT; NBCOLOR_ESTABLECER(colorT, props->colorBgT.r, props->colorBgT.g, props->colorBgT.b, props->colorBgT.a)
		//
		NBColor8 colorNL; NBCOLOR_ESTABLECER(colorNL, props->colorLeft.r, props->colorLeft.g, props->colorLeft.b, props->colorLeft.a)
		NBColor8 colorHL; NBCOLOR_ESTABLECER(colorHL, colorNL.r * 90 / 100, colorNL.g * 90 / 100, colorNL.b * 90 / 100, colorNL.a)
		NBColor8 colorTL; NBCOLOR_ESTABLECER(colorTL, colorNL.r * 80 / 100, colorNL.g * 80 / 100, colorNL.b * 80 / 100, colorNL.a)
		//
		NBColor8 colorNR; NBCOLOR_ESTABLECER(colorNR, props->colorRight.r, props->colorRight.g, props->colorRight.b, props->colorRight.a)
		NBColor8 colorHR; NBCOLOR_ESTABLECER(colorHR, colorNR.r * 90 / 100, colorNR.g * 90 / 100, colorNR.b * 90 / 100, colorNR.a)
		NBColor8 colorTR; NBCOLOR_ESTABLECER(colorTR, colorNR.r * 80 / 100, colorNR.g * 80 / 100, colorNR.b * 80 / 100, colorNR.a)
		//
		{
			STTSRowsTreeNode* itm = NBMemory_allocType(STTSRowsTreeNode);
			DRRowsTreeNode_init(itm);
			//General
			{
				NBString_set(&itm->valueStr, valStr);
				itm->valueUInt		= valUInt;
				itm->parent			= NULL;
			}
			//Btn
			if(leftTxt != NULL || rightTxt != NULL || rightObj != NULL){
				AUIButton* btnRef = new AUIButton();
				btnRef->establecerMargenes(props->margins, props->marginI);
				btnRef->establecerFondo(true, NULL);
				btnRef->establecerFondoColores(colorN, colorH, colorT);
				btnRef->establecerSegsRetrasarOnTouch(0.20f);
				if(rightIco != NULL){
					btnRef->agregarIcono(rightIco, ENIBotonItemAlign_Right, 0.5f, 0, 0, colorNL, colorHL, colorTL);
					itm->rightIco = rightIco;
					itm->rightIco->retener(NB_RETENEDOR_NULL);
				}
				if(leftTxt == NULL || (rightTxt == NULL && rightObj == NULL)){
					if(leftTxt != NULL){
						btnRef->agregarTextoMultilinea(props->fntLeft, leftTxt, ENIBotonItemAlign_Center, 0.5f, ENDRRowsTreeNodeBtnPart_LeftText, 0, colorNL, colorHL, colorTL);
					} else {
						if(rightObj != NULL){
							btnRef->agregarObjeto(rightObj, ENIBotonItemAlign_Center, 0.5f, ENDRRowsTreeNodeBtnPart_RightObj, 0, colorN, colorH, colorT);
						}
						if(rightTxt != NULL){
							btnRef->agregarTextoMultilinea(props->fntRight, rightTxt, ENIBotonItemAlign_Center, 0.5f, ENDRRowsTreeNodeBtnPart_RightText, 0, colorNR, colorHR, colorTR);
						}
					}
				} else {
					btnRef->agregarTextoMultilinea(props->fntLeft, leftTxt, ENIBotonItemAlign_Left, 0.5f, ENDRRowsTreeNodeBtnPart_LeftText, 0, colorNL, colorHL, colorTL);
					if(rightObj != NULL){
						btnRef->agregarObjeto(rightObj, ENIBotonItemAlign_Right, 0.5f, ENDRRowsTreeNodeBtnPart_RightObj, 0, colorN, colorH, colorT);
					}
					if(rightTxt != NULL){
						btnRef->agregarTextoMultilinea(props->fntRight, rightTxt, ENIBotonItemAlign_Right, 0.5f, ENDRRowsTreeNodeBtnPart_RightIcon, 0, colorNR, colorHR, colorTR);
					}
				}
				btnRef->organizarContenido(false);
				itm->btn			= btnRef; //retained here by creation
				itm->rightObj		= rightObj; if(itm->rightObj != NULL) itm->rightObj->retener(NB_RETENEDOR_NULL);
				itm->rightObjItf	= (rightObj != NULL ? rightObjItf : NULL);
			}
			NBASSERT(itm->btn != NULL)
			NBArray_addValue(&obj->nodes, itm);
			r = itm;
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTree_addNewRowSides(STTSRowsTree* obj, const STTSRowProps* props, const STTSRowsTreeSideDef* left, const STTSRowsTreeSideDef* right, const char* valStr, const SI32 valUInt){
	STTSRowsTreeNode* r = NULL;
	{
		NBColor8 colorN; NBCOLOR_ESTABLECER(colorN, props->colorBgN.r, props->colorBgN.g, props->colorBgN.b, props->colorBgN.a)
		NBColor8 colorH; NBCOLOR_ESTABLECER(colorH, props->colorBgH.r, props->colorBgH.g, props->colorBgH.b, props->colorBgH.a)
		NBColor8 colorT; NBCOLOR_ESTABLECER(colorT, props->colorBgT.r, props->colorBgT.g, props->colorBgT.b, props->colorBgT.a)
		//
		NBColor8 colorNL; NBCOLOR_ESTABLECER(colorNL, props->colorLeft.r, props->colorLeft.g, props->colorLeft.b, props->colorLeft.a)
		NBColor8 colorHL; NBCOLOR_ESTABLECER(colorHL, colorNL.r * 90 / 100, colorNL.g * 90 / 100, colorNL.b * 90 / 100, colorNL.a)
		NBColor8 colorTL; NBCOLOR_ESTABLECER(colorTL, colorNL.r * 80 / 100, colorNL.g * 80 / 100, colorNL.b * 80 / 100, colorNL.a)
		//
		NBColor8 colorNR; NBCOLOR_ESTABLECER(colorNR, props->colorRight.r, props->colorRight.g, props->colorRight.b, props->colorRight.a)
		NBColor8 colorHR; NBCOLOR_ESTABLECER(colorHR, colorNR.r * 90 / 100, colorNR.g * 90 / 100, colorNR.b * 90 / 100, colorNR.a)
		NBColor8 colorTR; NBCOLOR_ESTABLECER(colorTR, colorNR.r * 80 / 100, colorNR.g * 80 / 100, colorNR.b * 80 / 100, colorNR.a)
		//
		{
			STTSRowsTreeNode* itm = NBMemory_allocType(STTSRowsTreeNode);
			DRRowsTreeNode_init(itm);
			//General
			{
				NBString_set(&itm->valueStr, valStr);
				itm->valueUInt		= valUInt;
				itm->parent			= NULL;
			}
			//Btn
			{
				SI32 leftElems = 0, rightElems = 0;
				AUIButton* btnRef = new AUIButton();
				btnRef->establecerMargenes(props->margins, props->marginI);
				btnRef->establecerFondo(true, NULL);
				btnRef->establecerFondoColores(colorN, colorH, colorT);
				btnRef->establecerSegsRetrasarOnTouch(0.20f);
				if(left != NULL){
					if(left->ico != NULL) leftElems++;
					if(left->obj != NULL) leftElems++;
					if(!NBString_strIsEmpty(left->text)) leftElems++;
				}
				if(right != NULL){
					if(right->ico != NULL) rightElems++;
					if(right->obj != NULL) rightElems++;
					if(!NBString_strIsEmpty(right->text)) rightElems++;
				}
				if(left != NULL){
					const ENIBotonItemAlign align = (rightElems == 0 ? ENIBotonItemAlign_Center : ENIBotonItemAlign_Left);
					if(left->ico != NULL){
						btnRef->agregarIcono(left->ico, align, 0.5f, ENDRRowsTreeNodeBtnPart_LeftIcon, 0, colorNL, colorHL, colorTL);
						itm->leftIco = left->ico;
						itm->leftIco->retener(NB_RETENEDOR_NULL);
					}
					if(left->obj != NULL){
						btnRef->agregarObjeto(left->obj, align, 0.5f, ENDRRowsTreeNodeBtnPart_LeftObj, 0, colorN, colorH, colorT);
						itm->leftObj	= left->obj;
						itm->leftObj->retener(NB_RETENEDOR_NULL);
						itm->leftObjItf	= left->itf;
					}
					if(!NBString_strIsEmpty(left->text)){
						btnRef->agregarTextoMultilinea(props->fntLeft, left->text, align, 0.5f, ENDRRowsTreeNodeBtnPart_LeftText, 0, colorNL, colorHL, colorTL);
					}
				}
				if(right != NULL){
					const ENIBotonItemAlign align = (leftElems == 0 ? ENIBotonItemAlign_Center : ENIBotonItemAlign_Right);
					if(right->obj != NULL){
						btnRef->agregarObjeto(right->obj, align, 0.5f, ENDRRowsTreeNodeBtnPart_RightObj, 0, colorN, colorH, colorT);
						itm->rightObj		= right->obj;
						itm->rightObj->retener(NB_RETENEDOR_NULL);
						itm->rightObjItf	= right->itf;
					}
					if(!NBString_strIsEmpty(right->text)){
						btnRef->agregarTextoMultilinea(props->fntRight, right->text, align, 0.5f, ENDRRowsTreeNodeBtnPart_RightText, 0, colorNL, colorHL, colorTL);
					}
					if(right->ico != NULL){
						btnRef->agregarIcono(right->ico, align, 0.5f, ENDRRowsTreeNodeBtnPart_RightIcon, 0, colorNL, colorHL, colorTL);
						itm->rightIco = right->ico;
						itm->rightIco->retener(NB_RETENEDOR_NULL);
					}
				}
				btnRef->organizarContenido(false);
				itm->btn = btnRef; //retained here by creation
			}
			NBASSERT(itm->btn != NULL)
			NBArray_addValue(&obj->nodes, itm);
			r = itm;
		}
	}
	return r;
}

//

void DRRowsTreeNode_updateRow(STTSRowsTreeNode* obj, const char* leftTxt, const char* rightTxt, AUEscenaObjeto* rightObj, AUEscenaListaItemI* rightObjItf, AUTextura* rightIco){
	if(obj->btn != NULL){
		obj->btn->actualizarTextosMultilineaPorTipo(ENDRRowsTreeNodeBtnPart_LeftText, (NBString_strIsEmpty(leftTxt) ? "" : leftTxt));
		obj->btn->actualizarTextosMultilineaPorTipo(ENDRRowsTreeNodeBtnPart_RightText, (NBString_strIsEmpty(rightTxt) ? "" : rightTxt));
		if(rightObj != NULL){
			obj->btn->actualizarObjetoPorTipo(ENDRRowsTreeNodeBtnPart_RightObj, rightObj);
			if(rightObj != NULL) rightObj->retener(NB_RETENEDOR_NULL);
			if(obj->rightObj != NULL) obj->rightObj->liberar(NB_RETENEDOR_NULL);
			obj->rightObj		= rightObj;
			obj->rightObjItf	= rightObjItf;
		}
		if(rightIco != NULL){
			obj->btn->actualizarIconosPorTipo(ENDRRowsTreeNodeBtnPart_RightIcon, rightIco);
			if(rightIco != NULL) rightIco->retener(NB_RETENEDOR_NULL);
			if(obj->rightIco != NULL) obj->rightIco->liberar(NB_RETENEDOR_NULL);
			obj->rightIco = rightIco;
		}
	}
}

void DRRowsTreeNode_updateRowSides(STTSRowsTreeNode* obj, const STTSRowsTreeSideDef* left, const STTSRowsTreeSideDef* right){
	if(obj->btn != NULL){
		if(left != NULL){
			if(left->ico != NULL && left->ico != obj->leftIco){
				obj->btn->actualizarIconosPorTipo(ENDRRowsTreeNodeBtnPart_LeftIcon, left->ico);
				if(left->ico != NULL) left->ico->retener(NB_RETENEDOR_NULL);
				if(obj->leftIco != NULL) obj->leftIco->liberar(NB_RETENEDOR_NULL);
				obj->leftIco = left->ico;
			}
			if(left->obj != NULL && (left->obj != obj->leftObj || left->itf != obj->leftObjItf)){
				if(left->obj != obj->leftObj){
					obj->btn->actualizarObjetoPorTipo(ENDRRowsTreeNodeBtnPart_LeftObj, left->obj);
					if(left->obj != NULL) left->obj->retener(NB_RETENEDOR_NULL);
					if(obj->leftObj != NULL) obj->leftObj->liberar(NB_RETENEDOR_NULL);
					obj->leftObj	= left->obj;
				}
				obj->leftObjItf	= left->itf;
			}
			obj->btn->actualizarTextosMultilineaPorTipo(ENDRRowsTreeNodeBtnPart_LeftText, (NBString_strIsEmpty(left->text) ? "" : left->text));
		}
		if(right != NULL){
			obj->btn->actualizarTextosMultilineaPorTipo(ENDRRowsTreeNodeBtnPart_RightText, (NBString_strIsEmpty(right->text) ? "" : right->text));
			if(right->obj != NULL && (right->obj != obj->rightObj || right->itf != obj->rightObjItf)){
				if(right->obj != obj->rightObj){
					obj->btn->actualizarObjetoPorTipo(ENDRRowsTreeNodeBtnPart_RightObj, right->obj);
					if(right->obj != NULL) right->obj->retener(NB_RETENEDOR_NULL);
					if(obj->rightObj != NULL) obj->rightObj->liberar(NB_RETENEDOR_NULL);
					obj->rightObj	= right->obj;
				}
				obj->rightObjItf	= right->itf;
			}
			if(right->ico != NULL && right->ico != obj->leftIco){
				obj->btn->actualizarIconosPorTipo(ENDRRowsTreeNodeBtnPart_RightIcon, right->ico);
				if(right->ico != NULL) right->ico->retener(NB_RETENEDOR_NULL);
				if(obj->leftIco != NULL) obj->leftIco->liberar(NB_RETENEDOR_NULL);
				obj->leftIco = right->ico;
			}
		}
	}
}


void DRRowsTreeNode_fillStart(STTSRowsTreeNode* obj){
	if(obj != NULL){
		if(obj->children != NULL){
			DRRowsTree_fillStart(obj->children);
		}
	}
}

void DRRowsTreeNode_fillEnd(STTSRowsTreeNode* obj){
	if(obj != NULL){
		if(obj->children != NULL){
			DRRowsTree_fillEnd(obj->children);
		}
	}
}


STTSRowsTreeNode* DRRowsTreeNode_addPendRowByValueStr(STTSRowsTreeNode* obj, const char* valueStr){
	STTSRowsTreeNode* r = NULL;
	if(obj != NULL){
		if(obj->children != NULL){
			r = DRRowsTree_addPendRowByValueStr(obj->children, valueStr);
			if(r != NULL){
				r->parent = obj;
			}
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTreeNode_addPendRowByValueUInt(STTSRowsTreeNode* obj, const UI32 valueUInt){
	STTSRowsTreeNode* r = NULL;
	if(obj != NULL){
		if(obj->children != NULL){
			r = DRRowsTree_addPendRowByValueUInt(obj->children, valueUInt);
			if(r != NULL){
				r->parent = obj;
			}
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTreeNode_addPendRowByValueStrAndUInt(STTSRowsTreeNode* obj, const char* valueStr, const UI32 valueUInt){
	STTSRowsTreeNode* r = NULL;
	if(obj != NULL){
		if(obj->children != NULL){
			r = DRRowsTree_addPendRowByValueStrAndUInt(obj->children, valueStr, valueUInt);
			if(r != NULL){
				r->parent = obj;
			}
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTreeNode_addNewRow(STTSRowsTreeNode* obj, const STTSRowProps* props, const char* leftTxt, const char* rightTxt, AUEscenaObjeto* rightObj, AUEscenaListaItemI* rightObjItf, AUTextura* rightIco, const char* valStr, const SI32 valUInt){
	STTSRowsTreeNode* r = NULL;
	if(obj != NULL){
		//Create children array
		if(obj->children == NULL){
			obj->children = NBMemory_allocType(STTSRowsTree);
			DRRowsTree_init(obj->children);
		}
		//Add child row
		if(obj->children != NULL){
			r = DRRowsTree_addNewRow(obj->children, props, leftTxt, rightTxt, rightObj, rightObjItf, rightIco, valStr, valUInt);
			if(r != NULL){
				r->parent = obj;
			}
		}
	}
	return r;
}


STTSRowsTreeNode* DRRowsTreeNode_addNewRowSides(STTSRowsTreeNode* obj, const STTSRowProps* props, const STTSRowsTreeSideDef* left, const STTSRowsTreeSideDef* right, const char* valStr, const SI32 valUInt){
	STTSRowsTreeNode* r = NULL;
	if(obj != NULL){
		//Create children array
		if(obj->children == NULL){
			obj->children = NBMemory_allocType(STTSRowsTree);
			DRRowsTree_init(obj->children);
		}
		//Add child row
		if(obj->children != NULL){
			r = DRRowsTree_addNewRowSides(obj->children, props, left, right, valStr, valUInt);
			if(r != NULL){
				r->parent = obj;
			}
		}
	}
	return r;
}

//

void DRRowsTree_organizeToWidth(STTSRowsTree* obj, const float width){
	NBASSERT(obj->pend.use == 0) //If fails "DRRowsTree_fillEnd" was not called after "DRRowsTree_fillStart"
	{
		SI32 i; for(i = 0; i < obj->nodes.use; i++){
			STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->nodes, STTSRowsTreeNode*, i);
			const float marginI			= n->btn->margenI();
			const NBMargenes margins	= n->btn->margenes();
			const float leftStart		= 0.0f + margins.left;
			const float rightStart		= width - margins.right;
			float left = leftStart, right = rightStart;
			if(n->leftIco != NULL){
				const NBTamano sz = n->leftIco->tamanoHD();
				if(left != leftStart) left += marginI;
				left += sz.ancho;
			}
			if(n->rightIco != NULL){
				const NBTamano sz = n->rightIco->tamanoHD();
				if(right != rightStart) right -= marginI;
				right -= sz.ancho;
			}
			if(n->leftObj != NULL && n->leftObjItf == NULL){
				const NBCajaAABB box = n->leftObj->cajaAABBLocal();
				if(left != leftStart) left += marginI;
				left += (box.xMax - box.xMin);
			}
			if(n->rightObj != NULL && n->rightObjItf == NULL){
				const NBCajaAABB box = n->rightObj->cajaAABBLocal();
				if(right != rightStart) right -= marginI;
				right -= (box.xMax - box.xMin);
			}
			//Objects
			{
				if(left != leftStart) left += marginI;
				if(right != rightStart) right -= marginI;
				if(n->leftObj != NULL && n->leftObjItf != NULL && n->rightObj != NULL && n->rightObjItf != NULL){
					//Organize half half
					const float widthForObj	= (right - left) - marginI;
					n->leftObjItf->organizarContenido(widthForObj, 0.0f, FALSE, NULL, 0.0f);
					n->rightObjItf->organizarContenido(widthForObj, 0.0f, FALSE, NULL, 0.0f);
				} else if(n->leftObj != NULL && n->leftObjItf != NULL){
					//All for left
					const float widthForObj	= (right - left);
					n->leftObjItf->organizarContenido(widthForObj, 0.0f, FALSE, NULL, 0.0f);
				} else if(n->rightObj != NULL && n->rightObjItf != NULL){
					//All for right
					const float widthForObj	= (right - left);
					n->rightObjItf->organizarContenido(widthForObj, 0.0f, FALSE, NULL, 0.0f);
				}
			}
			//Children nodes
			if(n->children != NULL){
				DRRowsTree_organizeToWidth(n->children, width);
			}
		}
	}
}

//

STTSRowsTreeNode* DRRowsTree_getNodeByValueStr(STTSRowsTree* obj, const char* valueStr){
	STTSRowsTreeNode* r = NULL;
	SI32 i; for(i = 0; i < obj->pend.use; i++){
		STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
		NBASSERT(n->btn != NULL)
		if(NBString_isEqual(&n->valueStr, valueStr)){
			r = n;
			break;
		}
	}
	return r;
}

STTSRowsTreeNode* DRRowsTree_getNodeByValueUInt(STTSRowsTree* obj, const UI32 valueUInt){
	STTSRowsTreeNode* r = NULL;
	SI32 i; for(i = 0; i < obj->pend.use; i++){
		STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
		NBASSERT(n->btn != NULL)
		if(n->valueUInt == valueUInt){
			r = n;
			break;
		}
	}
	return r;

}

STTSRowsTreeNode* DRRowsTree_getNodeByValueStrAndUInt(STTSRowsTree* obj, const char* valueStr, const UI32 valueUInt){
	STTSRowsTreeNode* r = NULL;
	SI32 i; for(i = 0; i < obj->pend.use; i++){
		STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->pend, STTSRowsTreeNode*, i);
		NBASSERT(n->btn != NULL)
		if(n->valueUInt == valueUInt){
			if(NBString_isEqual(&n->valueStr, valueStr)){
				r = n;
				break;
			}
		}
	}
	return r;
}


STTSRowsTreeNode* DRRowsTree_getNodeByBtn(STTSRowsTree* obj, const AUIButton* btn){
	STTSRowsTreeNode* r = NULL;
	{
		SI32 i; for(i = 0; i < obj->nodes.use; i++){
			STTSRowsTreeNode* n = NBArray_itmValueAtIndex(&obj->nodes, STTSRowsTreeNode*, i);
			NBASSERT(n->btn != NULL)
			if(n->btn == btn){
				r = n;
				break;
			}
			//Children nodes
			if(n->children != NULL){
				r = DRRowsTree_getNodeByBtn(n->children, btn);
				if(r != NULL){
					break;
				}
			}
		}
	}
	return r;
	
}
