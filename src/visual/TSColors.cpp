//
//  TSColors.cpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/19.
//

#include "visual/TSVisualPch.h"
#include "visual/TSColors.h"

//Colors

static const STSceneInfoColorDef __infoColorsDefs[] = {
	//ENTSColor_MainColor
	{
		{ 158, 33, 32, 255 }
		, { 138, 13, 2, 255 }
		, { 138, 13, 2, 255 }
	}
	//ENTSColor_Black
	, {
		{ 0, 0, 0, 255 }
		, { 50, 50, 50, 255 }
		, { 50, 50, 50, 255 }
	}
	//ENTSColor_Gray
	, {
		{ 148, 148, 148, 255 }
		, { 138, 138, 138, 255 }
		, { 128, 128, 128, 255 }
	}
	//ENTSColor_GrayLight
	, {
		{ 218, 218, 218, 255 }
		, { 208, 208, 208, 255 }
		, { 198, 198, 198, 255 }
	}
	//ENTSColor_Light
	, {
		{ 245, 240, 240, 255 }
		, { 180, 180, 180, 255 }
		, { 180, 180, 180, 255 }
	}
	//ENTSColor_LightFront
	, {
		{ 240, 240, 245, 255 }
		, { 180, 180, 180, 255 }
		, { 180, 180, 180, 255 }
	}
	//ENTSColor_White
	, {
		{ 255, 255, 255, 255 }
		, { 200, 200, 200, 255 }
		, { 200, 200, 200, 255 }
	}
	//ENTSColor_Red
	, {
		{ 255, 0, 0, 255 }
		, { 205, 0, 0, 255 }
		, { 205, 0, 0, 255 }
	}
	//ENTSColor_RedLight
	, {
		{ 255, 234, 234, 255 }
		, { 225, 204, 204, 255 }
		, { 225, 204, 204, 255 }
	}
	//ENTSColor_Green
	, {
		{ 44, 96, 71, 255 }
		, { 14, 66, 41, 255 }
		, { 14, 66, 41, 255 }
	}
	//ENTSColor_GreenLight
	, {
		{ 211, 243, 205, 255 }
		, { 181, 203, 175, 255 }
		, { 181, 203, 175, 255 }
	}
	//ENTSColor_Blue
	, {
		{ 43, 62, 217, 255 }
		, { 23, 42, 197, 255 }
		, { 23, 42, 197, 255 }
	}
	//ENTSColor_Yellow
	, {
		{ 253, 208, 35, 255 }
		, { 203, 158, 35, 255 }
		, { 203, 158, 35, 255 }
	}
	//ENTSColor_BarBg
	, {
		{ 249, 249, 249, 255 }
		, { 219, 219, 219, 255 }
		, { 219, 219, 219, 255 }
	}
	//ENTSColor_BarBtnIcoSel
	, {
		{ 255, 33, 32, 255 }
		, { 200, 13, 2, 255 }
		, { 200, 13, 2, 255 }
	}
	//ENTSColor_BarBtnIcoUnsel
	, {
		{ 148, 148, 148, 255 }
		, { 138, 138, 138, 255 }
		, { 128, 128, 128, 255 }
	}
	//
	//
	//
	//ENTSColor_Count
	, {
		{ 255, 255, 255, 0 }
		, { 255, 255, 255, 0 }
		, { 255, 255, 255, 0 }
	}
};

const STSceneInfoColorDef* TSColors::colorDef(const ENTSColor color){
	const STSceneInfoColorDef* r = NULL;
	NBASSERT((sizeof(__infoColorsDefs) / sizeof(__infoColorsDefs[0])) == (ENTSColor_Count + 1))
	if(color >= 0 && color <= ENTSColor_Count){
		r = &__infoColorsDefs[color];
	}
	return r;
}
