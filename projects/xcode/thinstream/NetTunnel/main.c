//
//  main.c
//  NetTunnel
//
//  Created by Marcos Ortega on 10/12/19.
//

#include "nb/NBFrameworkPch.h"
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrProcess.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBThread.h"
#include "nb/net/NBSocket.h"

#ifdef _WIN32
//
#else
#    include <signal.h>
#endif

#include <stdlib.h>    //for srand()

#define	NB_LIMIT_BYTES_PER_SEC	(1024 * 500)

STNBThreadMutex	__sockMutex;
SI32			__sockThreadsRunning = 0;
STNBSocketRef	__sockListen = NB_OBJREF_NULL;
BOOL			__globalExitSignal = FALSE;

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
				PRINTF_INFO("Tunnel, captured signal %s...\n", def->sigName);
				__globalExitSignal = TRUE;
				if(NBSocket_isSet(__sockListen)){
					NBSocket_shutdown(__sockListen, NB_IO_BITS_RDWR);
					NBSocket_close(__sockListen);
				}
				PRINTF_INFO("Tunnel, clean-exit flag set (please wait ...).\n");
				PRINTF_INFO("-------------------------------------.\n");
			}
			break;
		}
	}
}
#endif

typedef struct STTunnel_ {
	BOOL			rcvIsActive;
	BOOL			sndIsActive;
	STNBThreadMutex	mutex;
	STNBSocketRef	socketIn;
	STNBSocketRef	socketOut;
} STTunnel;

void ttReleaseIfUnusedLocked_(STTunnel* tt){
	if(tt->rcvIsActive || tt->sndIsActive){
		NBThreadMutex_unlock(&tt->mutex);
	} else {
		//Socket
		if(NBSocket_isSet(tt->socketIn)){
			NBSocket_shutdown(tt->socketIn, NB_IO_BITS_RDWR);
			NBSocket_release(&tt->socketIn);
            NBSocket_null(&tt->socketIn);
		}
		//Socket
		if(NBSocket_isSet(tt->socketOut)){
			NBSocket_shutdown(tt->socketOut, NB_IO_BITS_RDWR);
			NBSocket_release(&tt->socketOut);
            NBSocket_null(&tt->socketOut);
		}
		NBThreadMutex_unlock(&tt->mutex);
		NBThreadMutex_release(&tt->mutex);
		//Tunnel
		NBMemory_free(tt);
		tt = NULL;
		//Remove thread
		{
			NBThreadMutex_lock(&__sockMutex);
			NBASSERT(__sockThreadsRunning > 0)
			__sockThreadsRunning--;
			NBThreadMutex_unlock(&__sockMutex);
		}
	}
}

void ttReleaseFromRcv(STTunnel* tt){
	NBThreadMutex_lock(&tt->mutex);
	tt->rcvIsActive = FALSE;
	ttReleaseIfUnusedLocked_(tt);
}

void ttReleaseFromSnd(STTunnel* tt){
	NBThreadMutex_lock(&tt->mutex);
	tt->sndIsActive = FALSE;
	ttReleaseIfUnusedLocked_(tt);
}

void ttReleaseIfUnused(STTunnel* tt){
	NBThreadMutex_lock(&tt->mutex);
	ttReleaseIfUnusedLocked_(tt);
}

SI64 _runRcv(STNBThread* t, void* param);
SI64 _runSnd(STNBThread* t, void* param);

int main(int argc, const char * argv[]) {
	//File to monitor
	PRINTF_INFO("Tunnel, start-of-program.\n");
	//Start engine
	NBMngrProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	NBSocket_initWSA();
	srand((int)time(NULL));
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
	{
		NBThreadMutex_init(&__sockMutex);
		__sockListen = NBSocket_alloc(NULL);
		NBSocket_setNoSIGPIPE(__sockListen, TRUE);
		NBSocket_setReuseAddr(__sockListen, TRUE); //Recommended before bind
		NBSocket_setReusePort(__sockListen, TRUE); //Recommended before bind
		{
			const SI32 portIn = 4434;
			//const char* serverOut = "127.0.0.1";
			//const SI32 portOut = 4433;
			const char* serverOut = "wowzaec2demo.streamlock.net";
			const SI32 portOut = 554;
			if(!NBSocket_bind(__sockListen, portIn)){
				PRINTF_CONSOLE_ERROR("NBSocket_bind failed.\n");
			} else if(!NBSocket_listen(__sockListen)){
				PRINTF_CONSOLE_ERROR("NBSocket_listen failed.\n");
			} else {
				PRINTF_INFO("NBSocket_listen to port: %d.\n", portIn);
				STNBSocketRef sockIn = NBSocket_alloc(NULL);
				while(NBSocket_accept(__sockListen, sockIn)){
					PRINTF_INFO("New client.\n");
					STNBSocketRef sockOut = NBSocket_alloc(NULL);
					//
					NBSocket_setNoSIGPIPE(sockOut, TRUE);
					NBSocket_setDelayEnabled(sockOut, FALSE);
					NBSocket_setCorkEnabled(sockOut, FALSE);
					//
					NBSocket_setNoSIGPIPE(sockIn, TRUE);
					NBSocket_setDelayEnabled(sockIn, FALSE);
					NBSocket_setCorkEnabled(sockIn, FALSE);
					//
					if(!NBSocket_connect(sockOut, serverOut, portOut)){
						PRINTF_CONSOLE_ERROR("NBSocket_connect failed to: '%s:%d'.\n", serverOut, portOut);
					} else {
						//PRINTF_INFO("NBSocket_connect success to:  %s:%d.\n", serverOut, portOut);
						//Create threads
						STTunnel* tt = NBMemory_allocType(STTunnel);
						NBMemory_setZeroSt(*tt, STTunnel);
						NBThreadMutex_init(&tt->mutex);
						tt->socketIn = sockIn; NBSocket_null(&sockIn);
						tt->socketOut = sockOut; NBSocket_null(&sockOut);
						//Add thread
						{
							NBThreadMutex_lock(&__sockMutex);
							NBASSERT(__sockThreadsRunning >= 0)
							__sockThreadsRunning++;
							NBThreadMutex_unlock(&__sockMutex);
						}
						{
							//Create thread
							STNBThread* tIn = NBMemory_allocType(STNBThread);
							STNBThread* tOut = NBMemory_allocType(STNBThread);
							NBThread_init(tIn);
							NBThread_init(tOut);
							{
								tt->sndIsActive = TRUE;
								NBThread_setIsJoinable(tOut, FALSE);
								if(!NBThread_start(tOut, _runSnd, tt, NULL)){
									tt->sndIsActive = FALSE;
								} else {
									tt->rcvIsActive = TRUE;
									NBThread_setIsJoinable(tIn, FALSE);
									if(!NBThread_start(tIn, _runRcv, tt, NULL)){
										tt->rcvIsActive = FALSE;
									} else {
										tt		= NULL;
										tIn		= NULL;
										tOut	= NULL;
									}
								}
							}
							//
							if(tIn != NULL){
								NBThread_release(tIn);
								NBMemory_free(tIn);
								tIn = NULL;
							}
							if(tOut != NULL){
								NBThread_release(tOut);
								NBMemory_free(tOut);
								tOut = NULL;
							}
						}
						if(tt != NULL){
							ttReleaseIfUnused(tt);
							tt = NULL;
						}
					}
					//Release sockets (if not consumed)
					{
						if(NBSocket_isSet(sockOut)){
							//PRINTF_INFO("sockOut shutdown.\n");
							NBSocket_shutdown(sockOut, NB_IO_BITS_RDWR);
							NBSocket_release(&sockOut);
                            NBSocket_null(&sockOut);
						}
						if(NBSocket_isSet(sockIn)){
							//PRINTF_INFO("sockIn shutdown.\n");
							NBSocket_shutdown(sockIn, NB_IO_BITS_RDWR);
							NBSocket_release(&sockIn);
                            NBSocket_null(&sockIn);
						}
					}
					//Reset new socket
					{
						sockIn = NBSocket_alloc(NULL);
					}
				}
				//Wait for threads
				{
					NBThreadMutex_lock(&__sockMutex);
					while(__sockThreadsRunning > 0){
						NBThreadMutex_unlock(&__sockMutex);
						NBThread_mSleep(50);
						NBThreadMutex_lock(&__sockMutex);
					}
					NBThreadMutex_unlock(&__sockMutex);
				}
				if(NBSocket_isSet(sockIn)){
					NBSocket_release(&sockIn);
                    NBSocket_null(&sockIn);
				}
			}
		}
		NBSocket_shutdown(__sockListen, NB_IO_BITS_RDWR);
		NBSocket_release(&__sockListen);
        NBSocket_null(&__sockListen);
		NBThreadMutex_release(&__sockMutex);
	}
	//End engine
	NBSocket_finishWSA();
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBMngrProcess_release();
	PRINTF_INFO("Tunnel, end-of-program.\n");
	return 0;
}

SI64 _runRcv(STNBThread* t, void* param){
	if(param != NULL){
		STTunnel* tt = (STTunnel*)param;
		NBASSERT(tt->rcvIsActive)
		//Run cicle
		UI64 bytesAccumSent = 0, bytesTotalSent = 0;
		UI64 lastSec = NBDatetime_getCurUTCTimestamp();
		STNBString str;
		NBString_init(&str);
		//PRINTF_INFO("_runRcv, start.\n");
		while(TRUE){
			char buff[4096];
			const SI32 rcvd = NBSocket_recv(tt->socketIn, buff, sizeof(buff));
			if(rcvd <= 0){
				//PRINTF_CONSOLE_ERROR("_runRcv, receiveBytes failed (%d).\n", rcvd);
				NBSocket_shutdown(tt->socketOut, NB_IO_BITS_RDWR);
				break;
			} else {
				//Wait untill limit
				UI64 curSec = NBDatetime_getCurUTCTimestamp();
				while(bytesAccumSent >= NB_LIMIT_BYTES_PER_SEC){
					if(lastSec == (curSec = NBDatetime_getCurUTCTimestamp())){
						NBThread_mSleep(50);
					} else {
						if(bytesAccumSent != 0){
							PRINTF_INFO("_runRcv, bytesSentPerSec(%llu).\n", bytesAccumSent);
						}
						bytesAccumSent = 0;
						break;
					}
				}
				if(lastSec != curSec && bytesAccumSent != 0){
					PRINTF_INFO("_runRcv, bytesSentPerSec(%llu).\n", bytesAccumSent);
				}
				NBString_setBytes(&str, buff, rcvd);
				NBString_replace(&str, "localhost", "wowzaec2demo.streamlock.net");
				NBString_replace(&str, "4434", "554");
				PRINTF_INFO("Received %d bytes: -->\n%s\n<--\n", str.length, str.str);
				if(NBSocket_send(tt->socketOut, str.str, str.length) != str.length){
					//PRINTF_CONSOLE_ERROR("_runRcv, sendBytes(%d) failed.\n", rcvd);
					NBSocket_shutdown(tt->socketIn, NB_IO_BITS_RDWR);
					break;
				} else {
					//PRINTF_INFO("_runRcv, %d bytes tunneled.\n", rcvd);
					if(__globalExitSignal){
						NBSocket_shutdown(tt->socketIn, NB_IO_BITS_RDWR);
						NBSocket_shutdown(tt->socketOut, NB_IO_BITS_RDWR);
					}
					bytesTotalSent += rcvd;
					bytesAccumSent += rcvd;
				}
				lastSec = curSec;
			}
		}
		PRINTF_INFO("_runRcv, end (%llu bytes tunneled).\n", bytesTotalSent);
		//Release Tunnel
		{
			ttReleaseFromRcv(tt);
			tt = NULL;
		}
		NBString_release(&str);
	}
	//Release thread
	if(t != NULL){
		NBThread_release(t);
		NBMemory_free(t);
		t = NULL;
	}
	return 0;
}

SI64 _runSnd(STNBThread* t, void* param){
	if(param != NULL){
		STTunnel* tt = (STTunnel*)param;
		NBASSERT(tt->sndIsActive)
		//Run cicle
		UI64 bytesAccumSent = 0, bytesTotalSent = 0;
		UI64 lastSec = NBDatetime_getCurUTCTimestamp();
		STNBString str;
		NBString_init(&str);
		//PRINTF_INFO("_runSnd, start.\n");
		while(TRUE){
			char buff[4096];
			const SI32 rcvd = NBSocket_recv(tt->socketOut, buff, sizeof(buff));
			if(rcvd <= 0){
				//PRINTF_CONSOLE_ERROR("_runSnd, receiveBytes failed (%d).\n", rcvd);
				NBSocket_shutdown(tt->socketIn, NB_IO_BITS_RDWR);
				break;
			} else {
				//Wait untill limit
				UI64 curSec = NBDatetime_getCurUTCTimestamp();
				while(bytesAccumSent >= NB_LIMIT_BYTES_PER_SEC){
					if(lastSec == (curSec = NBDatetime_getCurUTCTimestamp())){
						NBThread_mSleep(50);
					} else {
						if(bytesAccumSent != 0){
							PRINTF_INFO("_runSnd, bytesSentPerSec(%llu).\n", bytesAccumSent);
						}
						bytesAccumSent = 0;
						break;
					}
				}
				if(lastSec != curSec && bytesAccumSent != 0){
					PRINTF_INFO("_runSnd, bytesSentPerSec(%llu).\n", bytesAccumSent);
				}
				NBString_setBytes(&str, buff, rcvd);
				NBString_replace(&str, "localhost", "wowzaec2demo.streamlock.net");
				NBString_replace(&str, "4434", "554");
				PRINTF_INFO("Sending %d bytes: -->\n%s\n<--\n", str.length, str.str);
				if(NBSocket_send(tt->socketIn, str.str, str.length) != str.length){
					//PRINTF_CONSOLE_ERROR("_runSnd, sendBytes(%d) failed.\n", rcvd);
					NBSocket_shutdown(tt->socketOut, NB_IO_BITS_RDWR);
					break;
				} else {
					//PRINTF_INFO("_runSnd, %d bytes tunneled.\n", rcvd);
					if(__globalExitSignal){
						NBSocket_shutdown(tt->socketIn, NB_IO_BITS_RDWR);
						NBSocket_shutdown(tt->socketOut, NB_IO_BITS_RDWR);
					}
					bytesTotalSent += rcvd;
					bytesAccumSent += rcvd;
				}
				lastSec = curSec;
			}
		}
		PRINTF_INFO("_runSnd, end (%llu bytes tunneled).\n", bytesTotalSent);
		//Release Tunnel
		{
			ttReleaseFromSnd(tt);
			tt = NULL;
		}
		NBString_release(&str);
	}
	//Release thread
	if(t != NULL){
		NBThread_release(t);
		NBMemory_free(t);
		t = NULL;
	}
	return 0;
}
