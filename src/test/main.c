//
//  main.c
//  thinstream-test
//
//  Created by Marcos Ortega on 7/23/18.
//

#include "nb/NBFrameworkPch.h"
//
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBProcess.h"
#include "nb/net/NBSocket.h"
//
#include "nb/core/NBString.h"
#include "nb/files/NBFile.h"
#include "nb/files/NBMp4.h"
#include "nb/net/NBRtp.h"
#include "nb/net/NBSdp.h"
#include "nb/2d/NBAvcParser.h"
#include "nb/2d/NBJpeg.h"
//
#include "nb/2d/NBAvcDecoderApple.h"
//------------------------------
//Main
//------------------------------

void TSParseNAL_(STNBAvcParser* parser, const BYTE* pay, const SI32 paySz);
void TSParseMp4_(STNBMp4Box* root, STNBFileRef f);

/*typedef struct STNBVTbl_ {
	void (*method1)(void);
	void (*method2)(void);
} STNBVTbl;

void method1_(void){
	PRINTF_INFO("method1_.\n");
}

void method2_(void){
	PRINTF_INFO("method2_.\n");
}

static const STNBVTbl vTbl = {
	method1_,
	method2_,
	. method1 = method2_
};*/

int main(int argc, const char * argv[]) {
	NBProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	//Test SDP parser (Session Description Protocol)
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	{
		//NBSdpDesc_dbgTest();
	}
#	endif
	//NBAvcBitstream test
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	{
		//NBASSERT(NBAvcBitstream_dbgTest(0xFFFFFFFE));
	}
#	endif
	//Testing NBRtpQueue_add algorythm
	/*{
		//Normal-case
		/ *const SI32 queueSz	= 16;
		const SI32 seqCicleAhead = 1;
		const UI16 seqNumNew = 216;
		const SI32 gapAheadSz = 519;
		SI32 seqCicleGap	= 0;
		UI16 seqNumGap		= 65232;
		SI32 gapRemain		= gapAheadSz;
		BOOL quickJumped	= FALSE;* /
		//Specific-case failing in production
		/ *const SI32 queueSz	= 16;
		const SI32 seqCicle = 0;
		const SI32 seqCicleAhead = 1;
		const UI16 seqNumNew = 216;
		const SI32 gapAheadSz = 519;
		SI32 seqCicleGap	= seqCicle;
		UI16 seqNumGap		= 65232;
		SI32 gapRemain		= gapAheadSz;
		BOOL quickJumped	= FALSE;* /
		//Specific-case failing (modified to end at the start of the next cycle)
		const SI32 queueSz	= 16;
		const SI32 seqCicle = 0;
		const SI32 seqCicleAhead = 1;
		const UI16 seqNumNew = 216 - (216 - queueSz + 1);
		const SI32 gapAheadSz = 519 - (216 - queueSz + 1);
		SI32 seqCicleGap	= seqCicle;
		UI16 seqNumGap		= 65232;
		SI32 gapRemain		= gapAheadSz;
		BOOL quickJumped	= FALSE;
		//
		while(gapRemain >= 0){
			//Free oldest slot (if necesary)
			//increase new slot seq
			if(seqNumGap == 0xFFFF){
				seqCicleGap++;
				seqNumGap = 0;
			} else {
				seqNumGap++;
			}
			//Reserve gap slot
			if(gapRemain > 0){
				if(gapRemain <= queueSz){
					//
				} else {
					//queue is empty, quick-jump can be done
					NBASSERT(gapRemain > queueSz)
					const UI32 jumpSz = (gapRemain - queueSz);
					const UI32 seqNumDst = ((SI32)seqNumGap) + jumpSz;
					//PRINTF_INFO("NBRtpQueue, quick-jumped %d lost-slots.\n", jumpSz);
					seqCicleGap	+= (seqNumDst / 0x10000U); //(0x10000U = 0xFFFFU + 0x1U)
					seqNumGap	= (seqNumDst % 0x10000U); //(0x10000U = 0xFFFFU + 0x1U)
					gapRemain	-= jumpSz; 
					quickJumped	= TRUE;
					NBASSERT(gapRemain == queueSz)
				}
			}
			//Next
			gapRemain--;
		}
		NBASSERT(seqCicleGap == seqCicleAhead)
#		ifdef NB_CONFIG_INCLUDE_ASSERTS
		if(seqNumGap != seqNumNew){
			PRINTF_CONSOLE_ERROR("NBRtpsQueue, quickJump failed for gapAheadSz(%d), result(%d) expected(%d).\n", gapAheadSz, seqNumGap, seqNumNew);
		}
#		endif
		NBASSERT(seqNumGap == seqNumNew)
	}*/
	//
	/*{
		(*vTbl.method1)();
		(*vTbl.method2)();
	}*/
	//Test timestampRT
	/*{
		UI32 iStart, amm, val, valLast, valRel, valRelLast;
		for(iStart = 0; iStart < 0xFFFFFFFFU; iStart++){
			val = valLast = iStart; valRel = valRelLast = 0;
			if((iStart % (1024 * 16)) == 0){
				PRINTF_INFO("Test, processing start (%u, %.2f%%).\n", iStart, ((double)iStart / (double)(0xFFFFFFFFU)) * 100.0);
			}
			for(amm = 0; amm < 0xFFFFFFFFU; amm++){
				if((amm % (1024 * 1024 * 128)) == 0){
					PRINTF_INFO("Test, processing start (%u, %.2f%%) +(%u, %.2f%%).\n", iStart, ((double)iStart / (double)(0xFFFFFFFFU)) * 100.0, amm, ((double)amm / (double)(0xFFFFFFFFU)) * 100.0);
				}
				valRel = NB_RTP_TIMESTAMP_32_DELTA(iStart, val);
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				if(valRelLast > valRel){
					PRINTF_CONSOLE_ERROR("Test, timestampRTP non compatible: startPoint(%u) curRtp(%u); prevDelta(%u) curDelta(%u).\n", iStart, val, valRelLast, valRel);
					NBASSERT(valRelLast <= valRel)
				}
#				endif
				valLast		= val;
				valRelLast	= valRel;
				val++;
			}
		}
	}*/
	//Test IPv4
	/*{
		const char* addresses[] = {
			"192.168.0.1", "192.168.0.1"
			, "192.168.0.100", "192.168.0.100"
			, "8.8.8.8", "8.8.8.8"
			, "8.8.8.8/2", "0.0.0.0"
			, "8.8.8.8\\2", "0.0.0.0"
			, "8.8.8.8hola", "0.0.0.0"
			, "h8.8.8.8hola", "0.0.0.0"
			, "1234.1234.1234.12", "0.0.0.0"
			, "1.2.3", "0.0.0.0"
			, "000.000.000", "0.0.0.0"
			, "000.000.000.000", "0.0.0.0"
			, "001.001.001.000", "1.1.1.0"
		};
		{
			char ipv4[16];
			UI32 i; const UI32 count = (sizeof(addresses) / sizeof(addresses[0]));
			for(i = 0; i < count; i += 2){
				const char* src = addresses[i];
				const char* expct = addresses[i + 1];
				const UI32 netIP = NBSocketAddr_parseIPv4(src);
				NBSocketAddr_concatIPv4(netIP, ipv4, sizeof(ipv4));
				PRINTF_INFO("NBSocketAddr, #%d) '%s' -> %u -> '%s'.\n", (i + 1), src, netIP, ipv4);
				NBASSERT(NBString_strIsEqual(expct, ipv4));
				
			}
		}
	}*/
	//Test ".264" file parsing
	/*{
		const char* filepath = "/Users/mortegam/Downloads/stream.h.264";
		//const char* filepath = "/Users/mortegam/streamOut.264";
		STNBFileRef file = NBFile_alloc(NULL);
		if(!NBFile_open(&file, filepath, ENNBFileMode_Read)){
			PRINTF_CONSOLE_ERROR("Main, NBFile_open failed for: '%s'.\n", filepath);
		} else {
			NBFile_lock(file);
			{
				BYTE buff[4096];
				SI32 read; SI32 zeroesCount = 0;
				UI32 nalsCount = 0;
				STNBString nalPay;
				STNBAvcParser parser;
				NBString_initWithSz(&nalPay, 4096, 4096, 0.10f);
				NBAvcParser_init(&parser);
				while(TRUE){
					read = NBFile_read(file, buff, sizeof(buff));
					if(read <= 0){
						break;
					}
					//consume bytes
					{
						const BYTE* b = buff;
						const BYTE* bAfterEnd = b + read;
						while(b < bAfterEnd){
							if(*b == 0x00){
								zeroesCount++;
								while(zeroesCount > 3){
									NBString_concatByte(&nalPay, 0x00);
									zeroesCount--;
								}
							} else if(*b == 0x01 && zeroesCount >= 2){
								//Flush nal
								if(nalPay.length > 0){
									//PRINTF_INFO("Main, NAL #%d, %d bytes.\n", (nalsCount + 1), nalPay.length);
									TSParseNAL_(&parser, (const BYTE*)nalPay.str, nalPay.length);
								}
								NBString_empty(&nalPay);
								zeroesCount = 0;
								nalsCount++;
							} else {
								while(zeroesCount > 0){
									NBString_concatByte(&nalPay, 0x00);
									zeroesCount--;
								}
								NBString_concatByte(&nalPay, *b);
							}
							b++;
						}
					}
				}
				NBString_release(&nalPay);
				NBAvcParser_release(&parser);
			}
			NBFile_unlock(file);
		}
		NBFile_release(&file);
	}*/
	//Test ".264" file hardware decoder
	/*{
		void* decCtx = NULL;
		STNBAvcDecoderApple dec;
		NBAvcDecoderApple_init(&dec);
		decCtx = NBAvcDecoderApple_ctxCreate(&dec); 
		if(decCtx == NULL){
			PRINTF_CONSOLE_ERROR("Main, NBAvcDecoderApple_ctxCreate failed.\n");
		} else {
			const char* filepath = "/Users/mortegam/Desktop/NIBSA_proyectos/CltMarcosOrtega/sys-thinstream/test_sample_high.264";
			STNBFileRef file = NBFile_alloc(NULL);
			if(!NBFile_open(&file, filepath, ENNBFileMode_Read)){
				PRINTF_CONSOLE_ERROR("Main, NBFile_open failed for: '%s'.\n", filepath);
			} else {
				NBFile_lock(file);
				{
					BYTE buff[4096];
					SI32 read; SI32 zeroesCount = 0;
					UI32 nalsCount = 0;
					STNBString nalPay;
					STNBAvcParser parser;
					NBString_initWithSz(&nalPay, 4096, 4096, 0.10f);
					NBAvcParser_init(&parser);
					STNBBitmap bmp; UI32 bmpSeq = 0;
					NBBitmap_init(&bmp);
					while(TRUE){
						read = NBFile_read(file, buff, sizeof(buff));
						if(read <= 0){
							break;
						}
						//consume bytes
						{
							const BYTE* b = buff;
							const BYTE* bAfterEnd = b + read;
							while(b < bAfterEnd){
								if(*b == 0x00){
									zeroesCount++;
									while(zeroesCount > 3){
										NBString_concatByte(&nalPay, 0x00);
										zeroesCount--;
									}
								} else if(*b == 0x01 && zeroesCount >= 2){
									//Flush nal
									if(nalPay.length > 0){
										//PRINTF_INFO("Main, NAL #%d, %d bytes.\n", (nalsCount + 1), nalPay.length);
										if(!NBAvcDecoderApple_ctxFeedUnit(&dec, decCtx, nalPay.str, nalPay.length)){
											PRINTF_CONSOLE_ERROR("Main, NBAvcDecoderApple_ctxFeedUnit #%d failed.\n", nalsCount);
										} else if(!NBAvcDecoderApple_ctxGetBuffer(&dec, decCtx, &bmpSeq, &bmp, TRUE)){
											PRINTF_CONSOLE_ERROR("Main, NBAvcDecoderApple_ctxGetBuffer #%d failed.\n", nalsCount);
										} else {
											STNBString path;
											NBString_init(&path);
											NBString_concat(&path, filepath);
											NBString_concat(&path, "-");
											NBString_concatUI32(&path, bmpSeq);
											NBString_concat(&path, ".jpg");
											if(!NBJpeg_saveToPath(&bmp, path.str, 80, 20)){
												PRINTF_CONSOLE_ERROR("Main, NBJpeg_saveToPath #%d failed.\n", nalsCount);
											} else {
												PRINTF_INFO("Main, NBJpeg_saveToPath #%d success '%s'.\n", nalsCount, path.str);
											}
											NBString_release(&path);
										}
									}
									NBString_empty(&nalPay);
									//concat nalu start header
									{
										while(zeroesCount > 0){
											NBString_concatByte(&nalPay, 0x00);
											zeroesCount--;
										}
										NBString_concatByte(&nalPay, *b);
										NBASSERT(zeroesCount == 0)
									}
									nalsCount++;
								} else {
									while(zeroesCount > 0){
										NBString_concatByte(&nalPay, 0x00);
										zeroesCount--;
									}
									NBString_concatByte(&nalPay, *b);
								}
								b++;
							}
						}
					}
					NBBitmap_release(&bmp);
					NBString_release(&nalPay);
					NBAvcParser_release(&parser);
				}
				NBFile_unlock(file);
			}
			NBFile_release(&file);
		}
		NBAvcDecoderApple_release(&dec);
	}*/
	//Testing mp4 file-parser
	{
		//
		//const char* filepath = "/Users/mortegam/Downloads/sample-apple-fragmented.mp4";
		//const char* filepathOut = "/Users/mortegam/Downloads/sample-apple-fragmented.2.mp4";
		//
		//const char* filepath = "/Users/mortegam/Downloads/SampleVideo_1280x720_5mb.mp4";
		//const char* filepathOut = "/Users/mortegam/Downloads/SampleVideo_1280x720_5mb.2.mp4";
		//
		const char* filepath = "/Users/mortegam/Downloads/stream.mp4";
		const char* filepathOut = "/Users/mortegam/Downloads/stream.out.mp4";
		//
        STNBFileRef file = NBFile_alloc(NULL);
		{
			STNBMp4Box root;
			NBMp4Box_init(&root);
			{
				if(!NBFile_open(file, filepath, ENNBFileMode_Read)){
					PRINTF_CONSOLE_ERROR("Main, NBFile_open failed for: '%s'.\n", filepath);
				} else {
					//load
					{
						NBFile_lock(file);
						{
							TSParseMp4_(&root, file);
						}
						NBFile_unlock(file);
					}
					//print
					{
						SI32 i;
						STNBString buffOut;
						NBString_initWithSz(&buffOut, 64 * 1024, 64 * 1024, 0.10f);
						for(i = 0; i < root.subBoxes.use; i++){
							STNBMp4BoxRef* b = NBArray_itmPtrAtIndex(&root.subBoxes, STNBMp4BoxRef, i);
							NBString_empty(&buffOut);
							NBMp4BoxRef_concat(b, 0, file, &buffOut);
							PRINTF_INFO("TSParseMp4_, print:\n%s\n", buffOut.str);
						}
						NBString_release(&buffOut);
					}
					//write
					if(!NBString_strIsEqual(filepath, filepathOut)){
                        STNBFileRef fileOut = NBFile_alloc(NULL);
						if(!NBFile_open(fileOut, filepathOut, ENNBFileMode_Write)){
							PRINTF_CONSOLE_ERROR("Main, NBFile_open failed for: '%s'.\n", filepathOut);
						} else {
							NBFile_lock(fileOut);
							{
								SI32 i; UI64 written = 0;
								STNBString buffOut;
								NBString_initWithSz(&buffOut, 64 * 1024, 64 * 1024, 0.10f);
								for(i = 0; i < root.subBoxes.use; i++){
									STNBMp4BoxRef* b = NBArray_itmPtrAtIndex(&root.subBoxes, STNBMp4BoxRef, i);
									NBString_empty(&buffOut);
									if(!NBMp4BoxRef_writeBits(b, 0, file, &buffOut)){
										PRINTF_CONSOLE_ERROR("Main, NBMp4BoxRef_writeBits failed.\n");
										break;
									} else if(NBFile_write(fileOut, buffOut.str, buffOut.length) != buffOut.length){
										PRINTF_CONSOLE_ERROR("Main, NBFile_write failed.\n");
										break;
									} else {
										written += buffOut.length;
									}
								}
								PRINTF_INFO("Main, %llu bytes written'.\n", written);
								NBString_release(&buffOut);
							}
							NBFile_unlock(fileOut);
						}
						NBFile_release(&fileOut);
					}
				}
			}
			NBMp4Box_release(&root);
		}
		NBFile_release(&file);
	}
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBProcess_release();
	return 0;
}

//
	
void TSParseMp4_(STNBMp4Box* root, STNBFileRef f){
	const UI32 depthLvl = 0; SI64 boxFilePos = 0, boxPayFilePos = 0;
	STNBMp4BoxHdr hdr;
	NBMp4BoxHdr_init(&hdr);
	do {
		NBMp4BoxHdr_release(&hdr);
		//
		NBMp4BoxHdr_init(&hdr);
		boxFilePos = NBFile_curPos(f); 
		if(!NBMp4BoxHdr_loadBitsFromFile(&hdr, f)){
			PRINTF_INFO("TSParseMp4_, end-of-file.\n");
			break;
		}
		boxPayFilePos = NBFile_curPos(f);
		{
			STNBMp4BoxRef bRef;
			NBMp4BoxRef_init(&bRef);
			if(!NBMp4BoxRef_allocBoxFromFile(&bRef, root, depthLvl, boxFilePos, (boxPayFilePos - boxFilePos), &hdr, f)){
				PRINTF_INFO("TSParseMp4_, NBMp4Box_allocRefFromFile failed for '%c%c%c%c'.\n", hdr.type.str[0], hdr.type.str[1], hdr.type.str[2], hdr.type.str[3]);
				NBMp4BoxRef_release(&bRef);
				break;
			} else {
				NBArray_addValue(&root->subBoxes, bRef);
			}
		}
	} while(TRUE);
	NBMp4BoxHdr_release(&hdr);
}

void TSParseNAL_(STNBAvcParser* parser, const BYTE* pay, const SI32 paySz){
	if(pay != NULL && paySz > 0){
		const BYTE nalFirstByte		= *(pay++);
		BOOL forbiddenZeroBit		= FALSE;
		UI8 nalRefIdc				= 0;
		UI8 nalType					= 0;
		{
			forbiddenZeroBit	= ((nalFirstByte >> 7) & 0x1);
			nalRefIdc			= ((nalFirstByte >> 5) & 0x2); //zero means 'not-image-related' NAL package
			nalType				= (nalFirstByte & 0x1F);
		}
		if(forbiddenZeroBit){
			PRINTF_CONSOLE_ERROR("Test, NAL ignoring with active forbiddenZeroBit.\n");
		} else if(nalType <= 0 || nalType >= 30){
			PRINTF_CONSOLE_ERROR("Test, NAL type(%d) is reserved.\n", nalType);
		} else {
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			{
				const STNBAvcNaluDef* nTypeDef = NBAvc_getNaluDef(nalType);
				PRINTF_INFO("Test, H.264 NAL packet type(%u, '%s', '%s').\n", nalType, nTypeDef->group, nTypeDef->desc);
			}
#			endif
			if(nalType <= 0 || nalType >= 24){
				PRINTF_CONSOLE_ERROR("Test, expected non-fragmented NAL type.\n");
			} else {
				//PRINTF_INFO("Test, processing %d bytes NAL.\n", paySz);
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				//Print
				/*{
					const char* hexChars = "0123456789abcdef";
					const BYTE* pData = (const BYTE*)hdr.payload.data;
					const BYTE* pDataAfterEnd = pData + hdr.payload.sz;
					STNBString str;
					NBString_init(&str);
					while(pData < pDataAfterEnd){
						NBString_concatByte(&str, hexChars[(*pData >> 4) & 0xF]);
						NBString_concatByte(&str, hexChars[*pData & 0xF]);
						NBString_concatByte(&str, pDataAfterEnd != pData && ((pDataAfterEnd - pData) % 32) == 0 ? '\n' : ' ');
						pData++;
					}
					PRINTF_INFO("Test, port(%d) NAL payload:\n------>\n%s\n<------\n", opq->port, str.str);
					NBString_release(&str);
				}*/
#				endif
				if(parser != NULL && 
				   (
					nalType == 7 ||	//Sequence parameter set
					nalType == 8 ||	//Picture parameter set 
					nalType == 5 ||	//IDR picture
					nalType == 1	//non-IDR picture
					)
				   ){
					if(!NBAvcParser_feedStart(parser, nalRefIdc, nalType)){
						PRINTF_CONSOLE_ERROR("Test, NBAvcParser_feedStart(%d) failed.\n", nalType);
					} else if(!NBAvcParser_feedNALU(parser, &nalFirstByte, 1, pay, paySz, FALSE)){
						PRINTF_CONSOLE_ERROR("Test, NBAvcParser_feedNALU(%d) failed.\n", nalType);
					} else if(!NBAvcParser_feedEnd(parser)){
						PRINTF_CONSOLE_ERROR("Test, NBAvcParser_feedEnd(%d) failed.\n", nalType);
					} else {
						PRINTF_INFO("Test, OK type(%d) -------------.\n", nalType);
					}
				}
			}
		}
	}
}
