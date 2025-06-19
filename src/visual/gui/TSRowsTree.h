//
//  DRRowsTree.h
//  thinstream
//
//  Created by Marcos Ortega on 4/14/19.
//

#ifndef DRRowsTree_h
#define DRRowsTree_h

#include "AUIButton.h"
#include "AUEscenaObjeto.h"
#include "AUTextura.h"
#include "nb/core/NBString.h"

struct STTSRowsTree_;

typedef struct STTSRowsTreeSideDef_ {
	AUTextura*			ico;
	const char*			text;
	AUEscenaObjeto*		obj;
	AUEscenaListaItemI*	itf;
	BOOL				colorIsExplicit;	//Optional explicit color
	STNBColor8			colorExplicit;		//Optional explicit color
} STTSRowsTreeSideDef;

typedef struct STTSRowsTreeNode_ {
	STNBString				valueStr;
	UI32					valueUInt;
	struct STTSRowsTreeNode_*	parent;
	struct STTSRowsTree_*	children;
	//
	AUIButton*				btn;
	//
	AUTextura*				leftIco;
	AUEscenaObjeto*			leftObj;
	AUEscenaListaItemI* 	leftObjItf;
	//
	AUEscenaObjeto*			rightObj;
	AUEscenaListaItemI* 	rightObjItf;
	AUTextura*				rightIco;
} STTSRowsTreeNode;

typedef struct STTSRowsTree_ {
	STNBArray				nodes; //STTSRowsTreeNode*
	STNBArray				pend; //STTSRowsTreeNode*
} STTSRowsTree;

typedef struct STTSRowProps_ {
	AUFuenteRender*	fntLeft;
	AUFuenteRender*	fntRight;
	//
	STNBColor8		colorBgN;
	STNBColor8		colorBgH;
	STNBColor8		colorBgT;
	//
	STNBColor8		colorLeft;
	STNBColor8		colorRight;
	//
	NBMargenes		margins;
	float			marginI;
} STTSRowProps;

void DRRowProps_setDefaultsForList(STTSRowProps* obj, const SI32 iScene);

//
void DRRowsTree_init(STTSRowsTree* obj);
void DRRowsTree_release(STTSRowsTree* obj);

void DRRowsTree_fillStart(STTSRowsTree* obj);
void DRRowsTree_fillEnd(STTSRowsTree* obj);
STTSRowsTreeNode* DRRowsTree_addPendRowByValueStr(STTSRowsTree* obj, const char* valueStr);
STTSRowsTreeNode* DRRowsTree_addPendRowByValueUInt(STTSRowsTree* obj, const UI32 valueUInt);
STTSRowsTreeNode* DRRowsTree_addPendRowByValueStrAndUInt(STTSRowsTree* obj, const char* valueStr, const UI32 valueUInt);
STTSRowsTreeNode* DRRowsTree_addNewRow(STTSRowsTree* obj, const STTSRowProps* props, const char* leftTxt, const char* rightTxt, AUEscenaObjeto* rightObj, AUEscenaListaItemI* rightObjItf, AUTextura* rightIco, const char* valStr, const SI32 valUInt);
STTSRowsTreeNode* DRRowsTree_addNewRowSides(STTSRowsTree* obj, const STTSRowProps* props, const STTSRowsTreeSideDef* left, const STTSRowsTreeSideDef* right, const char* valStr, const SI32 valUInt);

void DRRowsTreeNode_fillStart(STTSRowsTreeNode* obj);
void DRRowsTreeNode_fillEnd(STTSRowsTreeNode* obj);
void DRRowsTreeNode_updateRow(STTSRowsTreeNode* obj, const char* leftTxt, const char* rightTxt, AUEscenaObjeto* rightObj, AUEscenaListaItemI* rightObjItf, AUTextura* rightIco);
void DRRowsTreeNode_updateRowSides(STTSRowsTreeNode* obj, const STTSRowsTreeSideDef* left, const STTSRowsTreeSideDef* right);
STTSRowsTreeNode* DRRowsTreeNode_addPendRowByValueStr(STTSRowsTreeNode* obj, const char* valueStr);
STTSRowsTreeNode* DRRowsTreeNode_addPendRowByValueUInt(STTSRowsTreeNode* obj, const UI32 valueUInt);
STTSRowsTreeNode* DRRowsTreeNode_addPendRowByValueStrAndUInt(STTSRowsTreeNode* obj, const char* valueStr, const UI32 valueUInt);
STTSRowsTreeNode* DRRowsTreeNode_addNewRow(STTSRowsTreeNode* obj, const STTSRowProps* props, const char* leftTxt, const char* rightTxt, AUEscenaObjeto* rightObj, AUEscenaListaItemI* rightObjItf, AUTextura* rightIco, const char* valStr, const SI32 valUInt);
STTSRowsTreeNode* DRRowsTreeNode_addNewRowSides(STTSRowsTreeNode* obj, const STTSRowProps* props, const STTSRowsTreeSideDef* left, const STTSRowsTreeSideDef* right, const char* valStr, const SI32 valUInt);


void DRRowsTree_organizeToWidth(STTSRowsTree* obj, const float width);

STTSRowsTreeNode* DRRowsTree_getNodeByValueStr(STTSRowsTree* obj, const char* valueStr);
STTSRowsTreeNode* DRRowsTree_getNodeByValueUInt(STTSRowsTree* obj, const UI32 valueUInt);
STTSRowsTreeNode* DRRowsTree_getNodeByValueStrAndUInt(STTSRowsTree* obj, const char* valueStr, const UI32 valueUInt);
STTSRowsTreeNode* DRRowsTree_getNodeByBtn(STTSRowsTree* obj, const AUIButton* btn);

#endif /* DRRowsTree_h */
