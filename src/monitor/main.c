//
//  main.c
//  thinstream-monitor
//
//  Created by Marcos Ortega on 9/11/19.
//

#include "nb/NBFrameworkPch.h"
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBNumParser.h"
#include "nb/net/NBSmtpClient.h"
#include "nb/net/NBMsExchangeClt.h"

int main(int argc, const char * argv[]) {
	return 0;
}

/*BOOL __globalExitSignal = FALSE;

#ifndef _WIN32
typedef enum ENSignalAction_ {
	ENSignalAction_Ignore = 0,
	ENSignalAction_GracefullExit,
	ENSignalAction_Count
} ENSignalAction;
#endif

#ifndef _WIN32
typedef struct STSignalDef_ {
	int				sig;
	const char*		sigName;
	ENSignalAction	action;
} STSignalDef;
#endif

#ifndef _WIN32
static STSignalDef _signalsDefs[] = {
	//sockets SIGPIPE signals (for unix-like systems)
	{ SIGPIPE, "SIGPIPE", ENSignalAction_Ignore}, //Ignore
	//termination signals: https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html
	{ SIGTERM, "SIGTERM", ENSignalAction_GracefullExit },
	{ SIGINT, "SIGINT", ENSignalAction_GracefullExit },
	{ SIGQUIT, "SIGQUIT", ENSignalAction_GracefullExit },
	{ SIGKILL, "SIGKILL", ENSignalAction_GracefullExit },
	{ SIGHUP, "SIGHUP", ENSignalAction_GracefullExit },
};
#endif

#ifndef _WIN32
void DRMain_intHandler(int sig){
	SI32 i; const SI32 count = (sizeof(_signalsDefs) / sizeof(_signalsDefs[0]));
	for(i = 0; i < count; i++){
		const STSignalDef* def = &_signalsDefs[i];
		if(sig == def->sig){
			if(def->action == ENSignalAction_GracefullExit){
				PRINTF_INFO("-------------------------------------.\n");
				PRINTF_INFO("Monitor, captured signal %s...\n", def->sigName);
				{
					__globalExitSignal = TRUE;
				}
				PRINTF_INFO("Monitor, clean-exit flag set (please wait ...).\n");
				PRINTF_INFO("-------------------------------------.\n");
			}
			break;
		}
	}
}
#endif

BOOL DRMonitor_sendEmail(const char* subject, const char* body, STNBMsExchangeClt* exClient);

int main(int argc, const char * argv[]) {
	//File to monitor
	UI32 minsPerOpenErrMsg	= 1;	//Time to wait to add next 'failed to open' msg (to avoid repetitive msgs).
	UI32 minsPerSmtpAttempt	= 1;	//Time to wait to send the next attempt to send email (after fails).
	UI32 minsPerEmail		= 1;	//Time to wait to send the next email with the accumulated content.
	const char* filepathMon	= NULL;
	PRINTF_INFO("Monitor, start-of-program.\n");
#	if defined(__APPLE__) && defined(NB_CONFIG_INCLUDE_ASSERTS)
	{
		//Requires #include <unistd.h>
		char path[4096];
		getcwd(path, sizeof(path));
		PRINTF_INFO("Monitor, current working directory: '%s'.\n", path);
	}
#	endif
	//Parameters
	{
		SI32 i; for(i = 0; i < argc; i++){
			if(NBString_strIsEqual(argv[i], "-f")){ //File to monitor
				if((i + 1) < argc){
					filepathMon = argv[i + 1];
				}
			} else if(NBString_strIsEqual(argv[i], "-w")){ //waitPerEmail (minutes)
				if((i + 1) < argc){
					BOOL succs = FALSE;
					const char* ammStr = argv[i + 1];
					const UI32 amm = NBNumParser_toUI32(ammStr, &succs);
					if(succs && amm > 0){
						minsPerEmail = amm;
					}
				}
			}
		}
	}
	//Start engine
	NBMngrProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	NBSocket_initWSA();
	srand((int)time(NULL));
#	ifndef _WIN32
	//Apply signal handlers
	{
		SI32 i; const SI32 count = (sizeof(_signalsDefs) / sizeof(_signalsDefs[0]));
		for(i = 0; i < count; i++){
			const STSignalDef* def = &_signalsDefs[i];
			if(def->action == ENSignalAction_Ignore){
				struct sigaction act;
				act.sa_handler	= SIG_IGN;
				act.sa_flags	= 0;
				sigemptyset(&act.sa_mask);
				sigaction(def->sig, &act, NULL);
			} else if(def->action == ENSignalAction_GracefullExit){
				struct sigaction act;
				act.sa_handler	= DRMain_intHandler;
				act.sa_flags	= 0;
				sigemptyset(&act.sa_mask);
				sigaction(def->sig, &act, NULL);
			}
		}
	}
#	endif
	//Run Monitor
	{
		STNBMsExchangeClt* exClient = NBMemory_allocType(STNBMsExchangeClt);
		if(exClient != NULL){
			NBMsExchangeClt_init(exClient);
			NBMsExchangeClt_configure(exClient, "exchange.omcolo.net", 443, "/EWS/Exchange.asmx", "noreply@thinstream.co", "Noreply2019", NULL, NULL);
		}
		if(NBString_strIsEmpty(filepathMon)){
			printf("USAGE '-f fileToMonitor.txt [-w <minutesPerEmail>]'.\n");
		} else {
			//Send initial email
			if(!DRMonitor_sendEmail("DRMonitor started", "DRMonitor service/daemon started. This could have been manually made by an administrator, if not, because the server operating system restarted or the service/daemon crashed.", exClient)){
				//
			}
			//Cicle
			{
				const SI32 MONITOR_MAX_LEN_LINE	= 1024 * 4;
				const SI32 MONITOR_MAX_LEN_LOG	= 1024 * 64;
				SI64 lineCurStartPos = 0, logCurStartPos = 0;
				STNBString pendStr, lineStr, logStr, logCurId, logCurTag;
				NBString_initWithSz(&pendStr, 4096 * 100, 4096 * 100, 0.10f);
				NBString_initWithSz(&lineStr, 4096 * 100, 4096 * 100, 0.10f);
				NBString_initWithSz(&logStr, 4096 * 100, 4096 * 100, 0.10f);
				NBString_initWithSz(&logCurId, 48, 48, 0.10f);
				NBString_initWithSz(&logCurTag, 48, 48, 0.10f);
				{
					STNBFileRef filemon = NBFile_alloc(NULL);
					{
						UI32 msAccum			= 0;
						const UI32 msPerWait	= 50;
						const UI32 msPerRead	= 1000;
						UI64 lastFailOpenMsg	= 0;
						UI64 lastEmailAttmp		= 0;
						UI64 lastEmailSent		= 0;
						SI64 fileCurPos			= -1;
						while(!__globalExitSignal){
							NBThread_mSleep(msPerWait);
							msAccum += msPerWait;
							{
								const UI64 curTime = NBDatetime_getCurUTCTimestamp();
								if(!__globalExitSignal && msAccum >= msPerRead){
									char dateBuff[NB_DATE_TIME_STR_BUFF_SIZE];
									NBDatetime_getCurLocalSqlStr(dateBuff, sizeof(dateBuff));
									//Open file (if necesary)
									if(!NBFile_isOpen(&filemon)){
										if(!NBFile_open(&filemon, filepathMon, ENNBFileMode_Read)){
											PRINTF_CONSOLE_ERROR("Monitor, file could not be opened.\n");
											const UI64 timeSinceLastLine = (curTime - lastFailOpenMsg);
											if(timeSinceLastLine >= (60 * minsPerOpenErrMsg)){
												lastFailOpenMsg = curTime;
												{
													NBString_concat(&pendStr, dateBuff);
													NBString_concat(&pendStr, " ");
													NBString_concat(&pendStr, "MONITOR, could not open file to monitor.");
													NBString_concat(&pendStr, "<br />\n");
													NBString_concat(&pendStr, "<br />\n");
												}
											}
										} else {
											const SI64 posBef = fileCurPos;
											//PRINTF_INFO("Monitor, file opened.\n");
											//Move to the end or continue
											{
												NBFile_lock(filemon);
												{
													//Move tot the end of the file
													NBFile_seek(filemon, 0, ENNBFileRelative_End);
													{
														const SI64 fileSz = NBFile_curPos(&filemon);
														if(fileCurPos <= 0 || fileCurPos >= fileSz){
															//Keep the current end
															fileCurPos = fileSz;
														} else {
															//Move to the last valid position
															NBFile_seek(filemon, fileCurPos, ENNBFileRelative_Start);
														}
													}
												}
												NBFile_unlock(filemon);
											}
											if(posBef != fileCurPos){
												PRINTF_INFO("Monitor, moved file positon from(%lld) to(%lld).\n", posBef, fileCurPos);
											}
										}
									}
									//Read file
									if(NBFile_isOpen(&filemon)){
										char buff[4096];
										NBFile_lock(filemon);
										{
											lineCurStartPos	= fileCurPos;
											lastFailOpenMsg	= 0;
										}
										while(TRUE){
											const SI32 read = NBFile_read(filemon, buff, sizeof(buff));
											if(read <= 0){
												//nothing to read
												break;
											} else {
												if(logCurId.length > 0){
													//Long-log mode (read untill close tag, or max size)
												} else {
													//Line mode (read one line at the time)
												}
												SI64 i; for(i = 0; i < read; i++){
													//Add char to line
													{
														if(buff[i] == '\n'){
															//Add as HTML new line
															NBString_concat(&lineStr, "<br />");
														}
														NBString_concatByte(&lineStr, buff[i]);
													}
													//Analyze line content
													if(buff[i] == '\n' || lineStr.length > MONITOR_MAX_LEN_LINE){
														//End of line found (or forced)
														BOOL addLine = (logCurId.length > 0 ? TRUE : FALSE);
														if(!addLine){ addLine = (NBString_indexOf(&lineStr, "ERROR", 0) >= 0); }
														if(!addLine){ addLine = (NBString_indexOf(&lineStr, "Error", 0) >= 0); }
														if(!addLine){ addLine = (NBString_indexOf(&lineStr, "error", 0) >= 0); }
														if(!addLine){ addLine = (NBString_indexOf(&lineStr, "Assert", 0) >= 0); }
														if(!addLine){ addLine = (NBString_indexOf(&lineStr, "assert", 0) >= 0); }
														if(!addLine){ addLine = (NBString_indexOf(&lineStr, "ServiceServer on port", 0) >= 0); }
														if(addLine){
															//Detect start of log-report
															if(logCurId.length <= 0){
																const char* tagStart	= "[start-of-logId:";
																const char* tagEnd		= "]";
																const SI32 posStart = NBString_strIndexOf(lineStr.str, tagStart, 0);
																if(posStart >= 0){
																	const SI32 posEnd = NBString_strIndexOf(lineStr.str, tagEnd, posStart);
																	if(posStart < posEnd){
																		const SI32 tagStartLen	= NBString_strLenBytes(tagStart);
																		NBString_setBytes(&logCurId, &lineStr.str[posStart + tagStartLen], posEnd - (posStart + tagStartLen));
																		NBString_empty(&logCurTag);
																		NBString_concat(&logCurTag, "[end-of-logId:");
																		NBString_concat(&logCurTag, logCurId.str);
																		NBString_concat(&logCurTag, "]");
																		logCurStartPos = lineCurStartPos;
																		PRINTF_INFO("Log started at pos(%lld) logId('%s').\n", logCurStartPos, logCurId.str);
																	}
																}
															}
															//Add line
															{
																if(logCurId.length > 0){
																	//Add to log
																	NBString_concat(&logStr, lineStr.str);
																	//Detect end of log
																	{
																		const BOOL isEndOfLog = (NBString_strIndexOf(lineStr.str, logCurTag.str, 0) >= 0 ? TRUE : FALSE);
																		if(isEndOfLog || logStr.length > MONITOR_MAX_LEN_LOG){
																			//Add to report
																			NBString_concat(&pendStr, logStr.str);
																			PRINTF_INFO("Log ended len(%lld) pos(%lld) logId('%s').\n", ((fileCurPos + i) - logCurStartPos), fileCurPos + i, logCurId.str);
																			//Reset log
																			NBString_empty(&logStr);
																			NBString_empty(&logCurId);
																			NBString_empty(&logCurTag);
																			logCurStartPos = 0;
																		}
																	}
																} else {
																	//Add to report
																	NBString_concat(&pendStr, lineStr.str);
																}
															}
														}
														NBString_empty(&lineStr);
														lineCurStartPos	= fileCurPos + i;
													}
												}
												//Set file pos
												fileCurPos = NBFile_curPos(&filemon);
											}
										}
										//Go back
										{
											//if line is incomplete
											if(lineStr.length > 0){
												if(fileCurPos > lineCurStartPos){
													fileCurPos = lineCurStartPos;
												}
											}
											//if log is incomplete
											if(logStr.length > 0){
												if(fileCurPos > logCurStartPos){
													fileCurPos = logCurStartPos;
												}
											}
										}
										NBFile_unlock(filemon);
										NBFile_close(filemon);
										//Reset
										NBFile_release(&filemon);
										filemon = NBFile_alloc(NULL);
									}
									//Send email
									if(pendStr.length > 0){
										const UI64 timeSinceLastSnd = (curTime - lastEmailSent);
										if(timeSinceLastSnd >= (60 * minsPerEmail)){
											const UI64 timeSinceLastAttmp = (curTime - lastEmailAttmp);
											if(timeSinceLastAttmp >= (60 * minsPerSmtpAttempt)){
												lastEmailAttmp = curTime;
												//Send email
												if(!DRMonitor_sendEmail("DRMonitor status", pendStr.str, exClient)){
													NBString_concat(&pendStr, dateBuff);
													NBString_concat(&pendStr, " ");
													NBString_concat(&pendStr, "MONITOR, could not send status email.");
													NBString_concat(&pendStr, "<br />\n");
													NBString_concat(&pendStr, "<br />\n");
												} else {
													NBString_empty(&pendStr);
													lastEmailSent	= curTime;
													lastEmailAttmp	= 0;
												}
											}
										}
									}
									msAccum = 0;
								}
							}
						}
					}
					NBFile_release(&filemon);
				}
				NBString_release(&lineStr);
				NBString_release(&pendStr);
				NBString_release(&logStr);
				NBString_release(&logCurId);
				NBString_release(&logCurTag);
			}
		}
		if(exClient != NULL){
			NBMsExchangeClt_release(exClient);
			NBMemory_free(exClient);
			exClient = NULL;
		}
	}
	//End engine
	NBSocket_finishWSA();
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBMngrProcess_release();
	PRINTF_INFO("Monitor, end-of-program.\n");
	return 0;
}

BOOL DRMonitor_sendEmail(const char* subject, const char* bodyHtml, STNBMsExchangeClt* exClient){
	BOOL r = FALSE;
	//
	STNBSmtpHeader hdr;
	NBSmtpHeader_init(&hdr);
	NBSmtpHeader_setFROM(&hdr, "noreply@nibsa.com.ni");
	NBSmtpHeader_setSUBJECT(&hdr, subject);
	NBSmtpHeader_addTO(&hdr, "mortegam@nibsa.com.ni");
	{
		STNBSmtpBody body;
		NBSmtpBody_init(&body);
		NBSmtpBody_setMimeType(&body, "text/html");
		NBSmtpBody_setContent(&body, bodyHtml);
		if(exClient != NULL){
			//Send using exchange
			if(!NBMsExchangeClt_mailSendSync(exClient, &hdr, &body)){
				PRINTF_CONSOLE_ERROR("Monitor, exchange could not Exchange.\n");
			}
		} else {
			//Send using SMTP
			STNBSmtpClient smpt;
			NBSmtpClient_init(&smpt);
			if(!NBSmtpClient_connect(&smpt, "mail.nibsa.com.ni", 465, TRUE, NULL)){ //587 no-ssl, 465 tls
				PRINTF_CONSOLE_ERROR("Monitor, smtp could not connect.\n");
			} else {
				if(!NBSmtpClient_sendHeader(&smpt, &hdr, "noreply@nibsa.com.ni", "KALvnnwQ5n8FpAans")){
					PRINTF_CONSOLE_ERROR("Monitor, smtp could not send header.\n");
				} else if(!NBSmtpClient_sendBody(&smpt, &body)){
					PRINTF_CONSOLE_ERROR("Monitor, smtp could not send body.\n");
				} else {
					PRINTF_INFO("Monitor, smtp body sent.\n");
					r = TRUE;
				}
				NBSmtpClient_disconnect(&smpt);
			}
			NBSmtpClient_release(&smpt);
		}
		NBSmtpBody_release(&body);
	}
	NBSmtpHeader_release(&hdr);
	return r;
}
*/
