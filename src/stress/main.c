//
//  main.c
//  thinstream-stress
//
//  Created by Marcos Ortega on 9/6/19.
//

#include "nb/NBFrameworkPch.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBNumParser.h"
#include "nb/net/NBSocket.h"
#include "stress/TSStressClient.h"

int main(int argc, const char * argv[]) {
	return 0;
}

/*STTSStressClientRun* __globalRunStressSignup = NULL;

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
				PRINTF_INFO("Stress, captured signal %s...\n", def->sigName);
				{
					if(__globalRunStressSignup != NULL){
						TSStressClientRun_setStopFlag(__globalRunStressSignup);
					}
				}
				PRINTF_INFO("Stress, clean-exit flag set (please wait ...).\n");
				PRINTF_INFO("-------------------------------------.\n");
			}
			break;
		}
	}
}
#endif

int main(int argc, const char * argv[]) {
	UI32 ammThreads = 0;
	PRINTF_INFO("Stress, start-of-program.\n");
#	if defined(__APPLE__) && defined(NB_CONFIG_INCLUDE_ASSERTS)
	{
		//Requires #include <unistd.h>
		char path[4096];
		getcwd(path, sizeof(path));
		PRINTF_INFO("Stress, current working directory: '%s'.\n", path);
	}
#	endif
	//Parameters
	{
		SI32 i; for(i = 0; i < argc; i++){
			if(NBString_strIsEqual(argv[i], "-t")){
				if((i + 1) < argc){
					BOOL succs = FALSE;
					const char* ammStr = argv[i + 1];
					const UI32 amm = NBNumParser_toUI32(ammStr, &succs);
					if(succs && amm >= 0){
						ammThreads = amm;
					}
				}
			}
		}
	}
	//Start engine
	NBProcess_init();
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
	//Run test
	{
		STTSStressClientRun test;
		TSStressClientRun_init(&test);
		{
			__globalRunStressSignup = &test;
			{
				if(!TSStressClientRun_start(&test, ammThreads)){
					PRINTF_INFO("Stress, error starting TSStressClientRun.\n");
				}
				TSStressClientRun_waitForAll(&test);
			}
			__globalRunStressSignup = NULL;
		}
		TSStressClientRun_release(&test);
	}
	//End engine
	NBSocket_finishWSA();
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBProcess_release();
	PRINTF_INFO("Stress, end-of-program.\n");
	return 0;
}
*/
