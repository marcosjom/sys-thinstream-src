//
//  TSDevice.c
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSNotifsList.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
//

void TSNotifsList_init(STTSNotifsList* obj){
	NBArray_initWithSz(&obj->arr, sizeof(STTSNotifsListItm), NULL, 0, 16, 0.10f);
}

void TSNotifsList_release(STTSNotifsList* obj){
	{
		SI32 i; for(i = 0; i < obj->arr.use; i++){
			STTSNotifsListItm* itm = NBArray_itmPtrAtIndex(&obj->arr, STTSNotifsListItm, i);
			NBString_release(&itm->textLocaleId);
			NBString_release(&itm->text);
			NBString_release(&itm->extraPayloadName);
			NBString_release(&itm->extraPayloadData);
		}
		NBArray_empty(&obj->arr);
		NBArray_release(&obj->arr);
	}
}

void TSNotifsList_add(STTSNotifsList* obj, const char* textLocaleId, const char* text, const char* extraPayloadName, void* extraPayloadData, const UI32 extraPayloadDataSz, const STNBStructMap* extraPayloadDataMap){
	STTSNotifsListItm itm;
	NBMemory_setZeroSt(itm, STTSNotifsListItm);
	NBString_initWithStr(&itm.textLocaleId, textLocaleId);
	NBString_initWithStr(&itm.text, text);
	NBString_initWithStr(&itm.extraPayloadName, extraPayloadName);
	NBString_initWithSz(&itm.extraPayloadData, 0, 4096, 0.10f);
	if(extraPayloadData != NULL && extraPayloadDataMap != NULL){
		NBStruct_stConcatAsJson(&itm.extraPayloadData, extraPayloadDataMap, extraPayloadData, extraPayloadDataSz);
	}
	NBArray_addValue(&obj->arr, itm);
}
