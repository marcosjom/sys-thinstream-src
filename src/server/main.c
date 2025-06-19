//
//  main.c
//  thinstream-server
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifdef _WIN32
#	define _CRT_SECURE_NO_WARNINGS
//#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>		//for CRITICAL_SECTION
#	include <stdio.h>
#endif

#include "nb/NBFrameworkPch.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBThread.h"
#include "nb/net/NBSocket.h"
#include "nb/crypto/NBSha1.h"
#include "nb/crypto/NBPKey.h"
#include "nb/crypto/NBX509.h"
#include "nb/crypto/NBPkcs12.h"
#include "nb/crypto/NBKeychain.h"
#if defined(__APPLE__)
#	include "nb/crypto/NBKeychainApple.h"
#	include <unistd.h>
#endif
#include "core/TSContext.h"
#include "core/config/TSCfg.h"
#include "core/server/TSServer.h"
//
#ifdef _WIN32
//
#else
#	include <signal.h>
#   include <unistd.h>    //for usleep()
#endif
//
#include <stdlib.h>	//for srand()

//Interruption received flag (interruptions are insecure and should not execute code, better just set the flag)
BOOL				__stopInterrupt	= FALSE;
STTSServer*			__server		= NULL;

BOOL thinstream_genCACertificate(const STTSCfg* cfg, STTSContext* context);
BOOL thinstream_extendCACertificate(const STTSCfg* cfg, STTSContext* context);
BOOL thinstream_extendCACertificate(const STTSCfg* cfg, STTSContext* context);
BOOL thinstream_genSSLCertificates(const STTSCfg* cfg, STTSContext* context);
BOOL thinstream_runServer(STTSContext* context, const SI32 msRunAndQuit);

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
	//Note: interruptions are called without considerations
	//of mutexes and threads states. Avoid non interrupt-safe methods calls.
	SI32 i; const SI32 count = (sizeof(_signalsDefs) / sizeof(_signalsDefs[0]));
	for(i = 0; i < count; i++){
		const STSignalDef* def = &_signalsDefs[i];
		if(sig == def->sig){
			if(def->action == ENSignalAction_GracefullExit){
                __stopInterrupt = TRUE;
			}
			break;
		}
	}
}
#endif

int main(int argc, const char * argv[]) {
	//Params
	BOOL runServer = FALSE, genCA = FALSE, extendCA = FALSE, genSSL = FALSE;
	SI32 msSleepBeforeRun = 0, msSleepAfterRun = 0, msRunAndQuit = 0;
	const char* cfgPath = NULL;
	//Start engine
	NBProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	NBSocket_initWSA();
	srand((int)time(NULL));
	//Apply signal handlers.
	//Ignore SIGPIPE at process level (for unix-like systems)
#	ifndef _WIN32
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
	//Load params
	{
		SI32 i; for(i = 0; i < argc; i++){
			if(NBString_strIsEqual(argv[i], "-genCA")){
				genCA = TRUE;
			} else if(NBString_strIsEqual(argv[i], "-extendCA")){
				extendCA = TRUE;
			} else if(NBString_strIsEqual(argv[i], "-genSSL")){
				genSSL = TRUE;
			} else if(NBString_strIsEqual(argv[i], "-runServer")){
				runServer = TRUE;
			} else if(NBString_strIsEqual(argv[i], "-c")){
				if((i + 1) < argc){
					cfgPath = argv[++i];
				}
			} else if(NBString_strIsEqual(argv[i], "-msSleepBeforeRun")){
				if((i + 1) < argc){
					if(NBString_strIsInteger(argv[i + 1])){
						msSleepBeforeRun = NBString_strToSI32(argv[++i]); 
					}
				}
			} else if(NBString_strIsEqual(argv[i], "-msSleepAfterRun")){
				if((i + 1) < argc){
					if(NBString_strIsInteger(argv[i + 1])){
						msSleepAfterRun = NBString_strToSI32(argv[++i]); 
					}
				}
			} else if(NBString_strIsEqual(argv[i], "-msRunAndQuit")){
				if((i + 1) < argc){
					if(NBString_strIsInteger(argv[i + 1])){
						msRunAndQuit = NBString_strToSI32(argv[++i]);
					}
				}
			}
		}
		if(msRunAndQuit > 0){
			PRINTF_INFO("-----\n");
			PRINTF_INFO("- Run will be limited to %dms.\n", msRunAndQuit);
			PRINTF_INFO("-----\n");
		}
		if(msSleepBeforeRun > 0){
			PRINTF_INFO("-----\n");
			PRINTF_INFO("Sleeping %dms before running...\n", msSleepBeforeRun);
			PRINTF_INFO("-----\n");
			NBThread_mSleep(msSleepBeforeRun);
		}
	}
#	if defined(__APPLE__) && defined(NB_CONFIG_INCLUDE_ASSERTS)
	{
		//Requires #include <unistd.h>
		char path[4096];
		getcwd(path, sizeof(path));
		PRINTF_INFO("-----\n");
		PRINTF_INFO("Current working directory: '%s'.\n", path);
		PRINTF_INFO("-----\n");
	}
#	endif
	//Execute
	{
		if(cfgPath == NULL){
			PRINTF_CONSOLE_ERROR("MAIN, must specify a config file: -c filepath.\n");
            PRINTF_CONSOLE_ERROR("MAIN, options: -genCA -extendCA -genSSL -runServer -msSleepBeforeRun [ms] -msSleepAfterRun [ms] -msRunAndQuit [ms].\n");
		} else if(cfgPath[0] == '\0'){
			PRINTF_CONSOLE_ERROR("MAIN, must specify a non-empty config file: -c filepath.\n");
		} else {
			STTSCfg	cfg;
			TSCfg_init(&cfg);
			if(!TSCfg_loadFromFilePath(&cfg, cfgPath)){
				PRINTF_CONSOLE_ERROR("MAIN, could not load config file: '%s'.\n", cfgPath);
			} else {
				PRINTF_INFO("MAIN, config file loaded: '%s'.\n", cfgPath);
				STTSContext context;
				TSContext_init(&context);
				TSContext_setNotifMode(&context, ENNBStorageNotifMode_Inmediate);
				if(!TSContext_prepare(&context, &cfg)){
					PRINTF_CONSOLE_ERROR("MAIN, TSContext_prepare failed: '%s'.\n", cfgPath);
				} else if(!TSContext_createRootFolders(&context)){
					PRINTF_CONSOLE_ERROR("MAIN, could not create root folder.\n");
				} else {
					PRINTF_INFO("MAIN, context loaded.\n");
					//Generate CA's public-certificate and private-key
					if(genCA){
						if(!thinstream_genCACertificate(&cfg, &context)){
							PRINTF_CONSOLE_ERROR("MAIN, thinstream_genCACertificate failed.\n");
						} else {
							PRINTF_INFO("MAIN, thinstream_genCACertificate completed.\n");
						}
					}
					if(extendCA){
						if(!thinstream_extendCACertificate(&cfg, &context)){
							PRINTF_CONSOLE_ERROR("MAIN, thinstream_extendCACertificate failed.\n");
						} else {
							PRINTF_INFO("MAIN, thinstream_extendCACertificate completed.\n");
						}
					}
					//Generate DRL's public-certificate and private-key
					if(genSSL){
						if(!thinstream_genSSLCertificates(&cfg, &context)){
							PRINTF_CONSOLE_ERROR("MAIN, thinstream_genSSLCertificates failed.\n");
						} else {
							PRINTF_INFO("MAIN, thinstream_genSSLCertificates completed.\n");
						}
					}
					//Run server
					if(runServer){
						thinstream_runServer(&context, msRunAndQuit);
					}
				}
				TSContext_release(&context);
			}
			TSCfg_release(&cfg);
		}
	}
	//End engine
	if(msSleepAfterRun > 0){
		PRINTF_INFO("-----\n");
		PRINTF_INFO("... sleeping %dms after running.\n", msSleepAfterRun);
		PRINTF_INFO("-----\n");
	}
	NBSocket_finishWSA();
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBProcess_release();
	//End sleep
	if(msSleepAfterRun > 0){
#       ifdef _WIN32
        Sleep((DWORD)msSleepAfterRun);
#       else
        usleep((useconds_t)msSleepAfterRun * 1000);
#       endif
	}
	printf("MAIN, returned.\n");
	return 0;
}

BOOL thinstream_genCACertificate(const STTSCfg* cfg, STTSContext* context){
	BOOL r = FALSE;
	const STNBHttpCertSrcCfg* cfgCACert = &cfg->context.caCert;
	const char* keyname		= cfgCACert->key.name;
	const UI32 keynameSz	= NBString_strLenBytes(keyname);
	const char* pass		= cfgCACert->key.pass;
	UI32 passSz				= NBString_strLenBytes(pass);
	STNBString typedPass;
	NBString_init(&typedPass);
	if(passSz <= 0){
		PRINTF_INFO("Type the key-output's password: ");
		char c = '\0';
		do {
			scanf("%c", &c);
			if(c == '\r' || c == '\n'){
				break;
			}
			NBString_concatByte(&typedPass, c);
		} while(TRUE);
		pass	= typedPass.str;
		passSz	= NBString_strLenBytes(pass);
	}
	if(cfgCACert->def.subject == NULL || cfgCACert->def.subjectSz < 1){
		PRINTF_CONSOLE_ERROR("Must specify CASubject to generate.\n");
	} else if(passSz == 0 || keynameSz == 0){
		PRINTF_CONSOLE_ERROR("A password and friendly-key-name must be provided to generate a p12 bundle.\n");
	} else {
		const char* keypath = cfgCACert->key.path;
		const char* cerpath = cfgCACert->path;
		if(cerpath == NULL || keypath == NULL || cerpath[0] == '\0' || keypath[0] == '\0'){
			PRINTF_CONSOLE_ERROR("Must specify cert/path and key/path to generate.\n");
		} else {
			STNBPKey pkey;
			STNBX509 cert;
			NBPKey_init(&pkey);
			NBX509_init(&cert);
			if(!NBX509_createSelfSigned(&cert, cfgCACert->def.bits, cfgCACert->def.daysValid, cfgCACert->def.subject, cfgCACert->def.subjectSz, &pkey)){
				PRINTF_CONSOLE_ERROR("Could not generate certificate.\n");
			} else {
				//Save public-cert
				{
					STNBString certData;
					NBString_init(&certData);
					if(!NBX509_getAsDERString(&cert, &certData)){
						PRINTF_CONSOLE_ERROR("Could not generate ca-certificate's DER.\n");
					} else {
						FILE* f = fopen(cerpath, "wb+");
						if(f == NULL){
							PRINTF_CONSOLE_ERROR("Could not write to file '%s'.\n", cerpath);
						} else {
							fwrite(certData.str, certData.length, 1, f);
							fclose(f);
						}
					}
					NBString_release(&certData);
				}
				//Save bundle (private-key and public-cert)
				{
					STNBPkcs12 bundle;
					NBPkcs12_init(&bundle);
					if(!NBPkcs12_createBundle(&bundle, keyname, &pkey, &cert, &cert, 1, pass)){
						PRINTF_CONSOLE_ERROR("Could not create p12 bundle.\n");
					} else {
						STNBString data;
						NBString_init(&data);
						if(!NBPkcs12_getAsDERString(&bundle, &data)){
							PRINTF_CONSOLE_ERROR("Could not generate clt-certificate's DER.\n");
						} else {
							FILE* f = fopen(keypath, "wb+");
							if(f != NULL){
								fwrite(data.str, data.length, 1, f);
								fclose(f);
								r = TRUE;
							}
						}
						NBString_release(&data);
					}
					NBPkcs12_release(&bundle);
				}
			}
			NBX509_release(&cert);
			NBPKey_release(&pkey);
		}
	}
	NBString_release(&typedPass);
	return r;
}

BOOL thinstream_extendCACertificate(const STTSCfg* cfg, STTSContext* context){
	BOOL r = FALSE;
	const STNBHttpCertSrcCfg* cfgCACert = &cfg->context.caCert;
	if(cfgCACert->key.pass == 0 || cfgCACert->key.path == 0){
		PRINTF_CONSOLE_ERROR("Both 'keypass' and 'keypath' are required.\n");
	} else {
		const char* pKey = cfgCACert->key.path;
		const char* pPass = cfgCACert->key.pass;
		STNBPkcs12 p12B;
		NBPkcs12_init(&p12B);
		if(!NBPkcs12_createFromDERFile(&p12B, pKey)){
			PRINTF_CONSOLE_ERROR("Could not load CA's key: '%s'.\n", pKey);
		} else {
			STNBPKey caKeyB;
			STNBX509 caCertB;
			NBPKey_init(&caKeyB);
			NBX509_init(&caCertB);
			if(!NBPkcs12_getCertAndKey(&p12B, &caKeyB, &caCertB, pPass)){
				PRINTF_CONSOLE_ERROR("Could not extract CA's key and cer: '%s'.\n", pKey);
			} else if(cfg->http.ports != NULL){
				//Build extended key and cert
				const char* keyname		= cfgCACert->key.name;
				const UI32 keynameSz	= NBString_strLenBytes(keyname);
				const char* pass		= cfgCACert->key.pass;
				UI32 passSz				= NBString_strLenBytes(pass);
				STNBString typedPass;
				NBString_init(&typedPass);
				if(passSz <= 0){
					PRINTF_INFO("Type the key-output's password: ");
					char c = '\0';
					do {
						scanf("%c", &c);
						if(c == '\r' || c == '\n'){
							break;
						}
						NBString_concatByte(&typedPass, c);
					} while(TRUE);
					pass	= typedPass.str;
					passSz	= NBString_strLenBytes(pass);
				}
				if(cfgCACert->def.subject == NULL || cfgCACert->def.subjectSz < 1){
					PRINTF_CONSOLE_ERROR("Must specify CASubject to generate.\n");
				} else if(passSz == 0 || keynameSz == 0){
					PRINTF_CONSOLE_ERROR("A password and friendly-key-name must be provided to generate a p12 bundle.\n");
				} else {
					const char* keypath = cfgCACert->key.path;
					const char* cerpath = cfgCACert->path;
					if(cerpath == NULL || keypath == NULL || cerpath[0] == '\0' || keypath[0] == '\0'){
						PRINTF_CONSOLE_ERROR("Must specify cerpath and keypath to generate.\n");
					} else {
						STNBPKey pkey;
						STNBX509 cert;
						NBPKey_init(&pkey);
						NBX509_init(&cert);
						if(!NBX509_createSelfSignedFrom(&cert, cfgCACert->def.bits, cfgCACert->def.daysValid, cfgCACert->def.subject, cfgCACert->def.subjectSz, &caCertB, &caKeyB, &pkey)){
							PRINTF_CONSOLE_ERROR("Could not generate certificate.\n");
						} else {
							//Backup current-cert and current-bundle
							{
								BOOL found = FALSE;
								UI32 seq = 0;
								STNBDate date = NBDate_getCurUTC();
								STNBString strK, strC;
								NBString_init(&strK);
								NBString_init(&strC);
								do {
									found = FALSE;
									//
									NBString_set(&strK, keypath);
									NBString_concat(&strK, ".bak.");
									NBString_concatSqlDate(&strK, date);
									if(seq > 0) NBString_concatUI32(&strK, seq);
									NBString_concat(&strK, ".p12");
									{
										FILE* f = fopen(strK.str, "rb");
										if(f != NULL){
											found = TRUE;
											fclose(f);
											f = NULL;
										}
									}
									//
									NBString_set(&strC, cerpath);
									NBString_concat(&strC, ".bak.");
									NBString_concatSqlDate(&strC, date);
									if(seq > 0) NBString_concatUI32(&strC, seq);
									NBString_concat(&strC, ".der");
									{
										FILE* f = fopen(strC.str, "rb");
										if(f != NULL){
											found = TRUE;
											fclose(f);
											f = NULL;
										}
									}
									//
									if(found) seq++;
								} while(found);
								//Copy files
								{
									//Copy key
									{
										FILE* fIn = fopen(keypath, "rb");
										if(fIn != NULL){
											FILE* fOut = fopen(strK.str, "wb+");
											if(fOut != NULL){
												BYTE buff[4096]; SI32 rr = 0;
												while(TRUE){
													rr = (SI32)fread(buff, sizeof(char), sizeof(buff), fIn);
													if(rr <= 0) break;
													fwrite(buff, sizeof(char), rr, fOut);
												}
												fclose(fIn);
												fOut = NULL;
											}
											fclose(fIn);
											fIn = NULL;
										}
									}
									//Copy cert
									{
										FILE* fIn = fopen(cerpath, "rb");
										if(fIn != NULL){
											FILE* fOut = fopen(strC.str, "wb+");
											if(fOut != NULL){
												BYTE buff[4096]; SI32 rr = 0;
												while(TRUE){
													rr = (SI32)fread(buff, sizeof(char), sizeof(buff), fIn);
													if(rr <= 0) break;
													fwrite(buff, sizeof(char), rr, fOut);
												}
												fclose(fIn);
												fOut = NULL;
											}
											fclose(fIn);
											fIn = NULL;
										}
									}
								}
								NBString_release(&strK);
								NBString_release(&strC);
							}
							//Save public-cert
							{
								STNBString certData;
								NBString_init(&certData);
								if(!NBX509_getAsDERString(&cert, &certData)){
									PRINTF_CONSOLE_ERROR("Could not generate ca-certificate's DER.\n");
								} else {
									FILE* f = fopen(cerpath, "wb+");
									if(f == NULL){
										PRINTF_CONSOLE_ERROR("Could not write to file '%s'.\n", cerpath);
									} else {
										fwrite(certData.str, certData.length, 1, f);
										fclose(f);
									}
								}
								NBString_release(&certData);
							}
							//Save bundle (private-key and public-cert)
							{
								STNBPkcs12 bundle;
								NBPkcs12_init(&bundle);
								if(!NBPkcs12_createBundle(&bundle, keyname, &pkey, &cert, &cert, 1, pass)){
									PRINTF_CONSOLE_ERROR("Could not create p12 bundle.\n");
								} else {
									STNBString data;
									NBString_init(&data);
									if(!NBPkcs12_getAsDERString(&bundle, &data)){
										PRINTF_CONSOLE_ERROR("Could not generate clt-certificate's DER.\n");
									} else {
										FILE* f = fopen(keypath, "wb+");
										if(f != NULL){
											fwrite(data.str, data.length, 1, f);
											fclose(f);
											r = TRUE;
										}
									}
									NBString_release(&data);
								}
								NBPkcs12_release(&bundle);
							}
						}
						NBX509_release(&cert);
						NBPKey_release(&pkey);
					}
				}
				NBString_release(&typedPass);
			}
			NBPKey_release(&caKeyB);
			NBX509_release(&caCertB);
		}
		NBPkcs12_release(&p12B);
	}
	return r;
}

BOOL thinstream_genSSLCertificates(const STTSCfg* cfg, STTSContext* context){
	BOOL r = FALSE;
	const STNBHttpCertSrcCfg* cfgCACert = &cfg->context.caCert;
	if(cfgCACert->key.pass == 0 || cfgCACert->key.path == 0){
		PRINTF_CONSOLE_ERROR("Both 'keypass' and 'keypath' are required.\n");
	} else {
		const char* pKey = cfgCACert->key.path;
		const char* pPass = cfgCACert->key.pass;
		STNBPkcs12 p12;
		NBPkcs12_init(&p12);
		if(!NBPkcs12_createFromDERFile(&p12, pKey)){
			PRINTF_CONSOLE_ERROR("Could not load CA's key: '%s'.\n", pKey);
		} else {
			STNBPKey caKey;
			STNBX509 caCert;
			NBPKey_init(&caKey);
			NBX509_init(&caCert);
			if(!NBPkcs12_getCertAndKey(&p12, &caKey, &caCert, pPass)){
				PRINTF_CONSOLE_ERROR("Could not extract CA's key and cer: '%s'.\n", pKey);
			} else if(cfg->http.ports != NULL){
				//Generate DRL certificates
				r = TRUE;
				{
					UI32 i; for(i = 0; i < cfg->http.portsSz; i++){
						STNBHttpPortCfg* c = &cfg->http.ports[i];
						if(!c->isDisabled){
							if(!c->ssl.isDisabled){
								const STNBHttpCertSrcCfg* cfgSslCert = &c->ssl.cert.source;
								const char* keyname		= cfgSslCert->key.name;
								const UI32 keynameSz	= NBString_strLenBytes(keyname);
								const char* pass		= cfgSslCert->key.pass;
								UI32 passSz				= NBString_strLenBytes(pass);
								STNBString typedPass;
								NBString_init(&typedPass);
								if(passSz <= 0){
									PRINTF_INFO("Type the key-output's password: ");
									char c = '\0';
									do {
										scanf("%c", &c);
										if(c == '\r' || c == '\n'){
											break;
										}
										NBString_concatByte(&typedPass, c);
									} while(TRUE);
									pass	= typedPass.str;
									passSz	= NBString_strLenBytes(pass);
								}
								if(cfgSslCert->def.subject == NULL || cfgSslCert->def.subjectSz < 1){
									PRINTF_CONSOLE_ERROR("Must specify CASubject to generate.\n");
									r = FALSE;
								} else if(passSz == 0 || keynameSz == 0){
									PRINTF_CONSOLE_ERROR("A password and friendly-key-name must be provided to generate a p12 bundle.\n");
									r = FALSE;
								} else {
									const char* keypath = cfgSslCert->key.path;
									const char* cerpath = cfgSslCert->path;
									if(keypath == NULL || cerpath == NULL || cerpath[0] == '\0' || keypath[0] == '\0'){
										PRINTF_CONSOLE_ERROR("Must specify cerpath and keypath to generate.\n");
										r = FALSE;
									} else {
										STNBPKey pkey;
										STNBX509Req certReq;
										NBPKey_init(&pkey);
										NBX509Req_init(&certReq);
										if(!NBX509Req_createSelfSigned(&certReq, cfgSslCert->def.bits, cfgSslCert->def.subject, cfgSslCert->def.subjectSz, &pkey)){
											PRINTF_CONSOLE_ERROR("Could not generate certificate.\n");
											r = FALSE;
										} else {
											STNBX509 cert;
											STNBSha1 hash;
											NBSha1_init(&hash);
											{
												const UI64 curTime = NBDatetime_getCurUTCTimestamp();
												NBSha1_feed(&hash, &curTime, sizeof(curTime));
												UI32 i; for(i = 0; i < 16; i++){
													const UI32 rnd = (rand() % 0xFFFFFFFF);
													NBSha1_feed(&hash, &rnd, sizeof(rnd));
												}
												NBSha1_feedEnd(&hash);
												NBASSERT(sizeof(hash.hash.v) == 20)
											}
											NBX509_init(&cert);
											if(!NBX509_createFromRequest(&cert, &certReq, &caCert, &caKey, cfgSslCert->def.bits, cfgSslCert->def.daysValid, hash.hash.v, sizeof(hash.hash.v))){
												PRINTF_CONSOLE_ERROR("Could not generate certificate from request.\n");
												r = FALSE;
											} else {
												//Save public-cert
												{
													STNBString certData;
													NBString_init(&certData);
													if(!NBX509_getAsDERString(&cert, &certData)){
														PRINTF_CONSOLE_ERROR("Could not generate ca-certificate's DER.\n");
													} else {
														FILE* f = fopen(cerpath, "wb+");
														if(f == NULL){
															PRINTF_CONSOLE_ERROR("Could not write to file '%s'.\n", cerpath);
														} else {
															fwrite(certData.str, certData.length, 1, f);
															fclose(f);
														}
													}
													NBString_release(&certData);
												}
												//Save bundle (private-key and public-cert)
												{
													STNBPkcs12 bundle;
													NBPkcs12_init(&bundle);
													if(!NBPkcs12_createBundle(&bundle, keyname, &pkey, &cert, &cert, 1, pass)){
														PRINTF_CONSOLE_ERROR("Could not create p12 bundle.\n");
														r = FALSE;
													} else {
														STNBString data;
														NBString_init(&data);
														if(!NBPkcs12_getAsDERString(&bundle, &data)){
															PRINTF_CONSOLE_ERROR("Could not generate clt-certificate's DER.\n");
															r = FALSE;
														} else {
															FILE* f = fopen(keypath, "wb+");
															if(f == NULL){
																r = FALSE;
															} else {
																fwrite(data.str, data.length, 1, f);
																fclose(f);
															}
														}
														NBString_release(&data);
													}
													NBPkcs12_release(&bundle);
												}
											}
											NBX509_release(&cert);
										}
										NBX509Req_release(&certReq);
										NBPKey_release(&pkey);
									}
								}
								NBString_release(&typedPass);
							}
						}
					}
				}
			}
			NBPKey_release(&caKey);
			NBX509_release(&caCert);
		}
		NBPkcs12_release(&p12);
	}
	return r;
}

BOOL thinstream_cltConnected_(void* param, const STNBX509* cert);
void thinstream_cltDisconnected_(void* param, const STNBX509* cert);
BOOL thinstream_cltRequested_(void* param, const STNBX509* cert, const ENTSRequestId reqId, const UI32 respCode, const char* respReason, const UI32 respBodyLen);

BOOL thinstream_runServer(STTSContext* context, const SI32 msRunAndQuit){
	BOOL r = FALSE;
	{
		//allocate server
		__server = NBMemory_allocType(STTSServer);
		TSServer_init(__server);
		//run server
		{
			STTSServerLstnr lstnr;
			NBMemory_setZeroSt(lstnr, STTSServerLstnr);
			lstnr.obj				= __server;
			lstnr.cltConnected		= thinstream_cltConnected_;
			lstnr.cltDisconnected	= thinstream_cltDisconnected_;
			lstnr.cltRequested		= thinstream_cltRequested_;
			if(!TSServer_prepare(__server, context, &__stopInterrupt, msRunAndQuit, &lstnr)){
				PRINTF_CONSOLE_ERROR("SERVER, Could not prepare server with config.\n");
			} else {
				//Run
				PRINTF_INFO("SERVER, prepared, executing ...\n");
				if(!TSServer_execute(__server, &__stopInterrupt)){
					PRINTF_CONSOLE_ERROR("SERVER, ... could not execute server with config.\n");
				} else {
					PRINTF_INFO("SERVER, ... cycle ended.\n");
				}
			}
		}
		//release server
		TSServer_release(__server);
		NBMemory_free(__server);
		__server = NULL;
	}
	PRINTF_INFO("SERVER, ended.\n");
	return r;
}


BOOL thinstream_cltConnected_(void* param, const STNBX509* cert){
	BOOL r = TRUE;
	//STTSSyncServer* srvr = (STTSSyncServer*)param;
	//PRINTF_INFO("thinstream_cltConnected_ cert(%s)\n", (NBX509_isCreated(cert) ? "YES" : "NO"));
	return r;
}

void thinstream_cltDisconnected_(void* param, const STNBX509* cert){
	//STTSSyncServer* srvr = (STTSSyncServer*)param;
	//PRINTF_INFO("thinstream_cltDisconnected_ cert(%s)\n", (NBX509_isCreated(cert) ? "YES" : "NO"));
}

BOOL thinstream_cltRequested_(void* param, const STNBX509* cert, const ENTSRequestId reqId, const UI32 respCode, const char* respReason, const UI32 respBodyLen){
	BOOL r = TRUE;
	//STTSSyncServer* srvr = (STTSSyncServer*)param;
	//PRINTF_INFO("thinstream_cltRequested_ cert(%s) respCode(%d) respReason('%s') respBodyLen(%d)\n", (NBX509_isCreated(cert) ? "YES" : "NO"), respCode, respReason, respBodyLen);
	return r;
}





