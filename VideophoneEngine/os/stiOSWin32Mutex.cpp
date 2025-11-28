/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSWin32Mutex
*
*	File Name:		stiOSWin32Mutex.c
*
*	Owner:			Scot L. Brooksby
*
*	Abstract:
*		ACTION - enter description
*
*******************************************************************************/

/*
* Includes
*/
#include <stdlib.h> /* for malloc () */
#include "stiOS.h"
#include "stiOSWin32Mutex.h"
#include "stiTrace.h"

/*
* Constants
*/

/*
* Typedefs
*/

/*
* Forward Declarations
*/

/*
* Globals
*/

/*
* Locals
*/

/*
* Function Declarations
*/

void stiOSRecursiveMutexCreate(pthread_mutex_t *pstMutex)
{
	pthread_mutexattr_t stAttribute;
	pthread_mutexattr_init(&stAttribute);
	pthread_mutexattr_settype(&stAttribute, PTHREAD_MUTEX_RECURSIVE_NP);

	/* Initialize the mutex as a recursive mutex */
	pthread_mutex_init(pstMutex, &stAttribute);

	pthread_mutexattr_destroy(&stAttribute);
}

/* stiOSMutexCreate - see stiOSMutex.h for documentation */
stiMUTEX_ID stiOSMutexCreate()
{
	return stiOSNamedMutexCreate(NULL);
}


stiMUTEX_ID stiOSNamedMutexCreate(const char *pszName)
{
	int result = pthread_win32_process_attach_np();

	pthread_mutex_t *pstMutex;

	pstMutex = (pthread_mutex_t *)malloc(sizeof (pthread_mutex_t));

	if (NULL != pstMutex)
	{
		stiOSRecursiveMutexCreate(stiACTUAL_MUTEX(pstMutex));
	} /* end if */

	return ((stiMUTEX_ID)pstMutex);
} /* end stiOSMutexCreate */


/* stiOSMutexDestroy - see stiOSMutex.h for documentation */
stiHResult stiOSMutexDestroy(
	stiMUTEX_ID pstMutex)
{
	stiHResult	eResult = stiRESULT_ERROR;

	if (NULL != pstMutex)
	{
		if (!pthread_mutex_destroy(stiACTUAL_MUTEX(pstMutex)))
		{
			DEALLOCATE(pstMutex);

			eResult = stiRESULT_SUCCESS;
		}
	} /* end if */

	return (eResult);
} /* end stiOSMutexCreate */

extern bool g_bReturnedFromConvertFrame;
extern bool g_bFinishedDecodingInEncode;
extern bool g_bFinishedDecodingInDecode;
extern void PrintConvertFrameParameters();
extern void PrintDecodeInEncodeInfo();
extern void PrintDecodeInDecodeInfo();


/* stiOSMutexLock - see stiOSMutex.h for documentation */
stiHResult stiOSMutexLock(stiMUTEX_ID pstMutex)
{
	stiHResult	eResult = stiRESULT_ERROR;

	if (NULL != pstMutex)
	{
#ifdef stiMUTEX_DEBUGGING
		int nResult = -1;
		struct timespec sTime;
		struct timeval sTimeNow;
		SstiMutexInformation mutexInformation;
		mutexInformation.m_nThreadID = GetCurrentThreadId();
		mutexInformation.m_nFrames = CaptureStackBackTrace(1, StackTraceCount, mutexInformation.m_pBackTrace, NULL);
		while(true)
		{
			//-----------We wait 5 seconds before we decide that the MUTEX is failing to unlock
			gettimeofday (&sTimeNow, NULL);
			sTime.tv_sec = sTimeNow.tv_sec + 5;
			sTime.tv_nsec = sTimeNow.tv_usec * 1000;
			nResult = pthread_mutex_timedlock(pstMutex, &sTime);
			//-----------------------------------------------------------------------------------
			
			if (nResult != 0)//If the nResult != 0 then it failed to get the mutex.
			{
				pthread_mutex_lock(&cs_mutex);
				if(!MutexTable[pstMutex].empty())
				{
					SstiMutexInformation LockedMutex = MutexTable[pstMutex].top();
					stiTrace("<%d> waiting for mutex [%p] held by <%d>\n", mutexInformation.m_nThreadID, LockedMutex.m_nThreadID);
					//----------Get Symbol Information----------
					SYMBOL_INFO*	pSymbol;
					HANDLE			hProcess;
					hProcess = GetCurrentProcess();
					SymInitialize(hProcess, NULL, TRUE);
					pSymbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO)+256 * sizeof(char), 1);
					pSymbol->MaxNameLen = 255;
					pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
					IMAGEHLP_LINE *line = (IMAGEHLP_LINE *)malloc(sizeof(IMAGEHLP_LINE));
					line->SizeOfStruct = sizeof(IMAGEHLP_LINE);
					//------------------------------------------
					//----------Output Locking Call Stack-------
					stiTrace(" Locking Call Stack\n");
					for (int i = 0; i < mutexInformation.m_nFrames; i++)
					{
						SymFromAddr(hProcess, (DWORD64)(mutexInformation.m_pBackTrace[i]), 0, pSymbol);
						DWORD dwDisplacement;
						SymGetLineFromAddr(hProcess, (DWORD)(mutexInformation.m_pBackTrace[i]), &dwDisplacement, line);
						stiTrace("  %s in %s at %d\n", pSymbol->Name, line->FileName, line->LineNumber);
					}
					//------------------------------------------
					//----------Output Locked Call Stack-------
					stiTrace(" Locked Call Stack\n");
					for (int i = 0; i < LockedMutex.m_nFrames; i++)
					{
						SymFromAddr(hProcess, (DWORD64)(LockedMutex.m_pBackTrace[i]), 0, pSymbol);
						DWORD dwDisplacement;
						SymGetLineFromAddr(hProcess, (DWORD)(LockedMutex.m_pBackTrace[i]), &dwDisplacement, line);
						stiTrace("  %s in %s at %d\n", pSymbol->Name, line->FileName, line->LineNumber);
					}
					//-----------------------------------------
					delete line;
					delete pSymbol;
				}
				else
				{
					stiTrace("<%d> waiting for mutex [%p] that could not be found\n", mutexInformation.m_nThreadID, pstMutex);
				}
				pthread_mutex_unlock(&cs_mutex);
				
			}
			else
			{
				break; // Exit while loop
			}
		}

		if (nResult == 0)
		{
			pthread_mutex_lock(&cs_mutex);
			MutexTable[pstMutex].push(mutexInformation);
			pthread_mutex_unlock(&cs_mutex);
			eResult = stiRESULT_SUCCESS;
		} /* end if */
#else
		int nResult = pthread_mutex_lock(pstMutex);

		if (nResult == 0)
		{
			eResult = stiRESULT_SUCCESS;
		} /* end if */
#endif
	} /* end if */

	return (eResult);
} /* end stiOSMutexLock */


/* stiOSMutexUnlock - see stiOSMutex.h for documentation */
stiHResult stiOSMutexUnlock(stiMUTEX_ID pstMutex)
{
	stiHResult	eResult = stiRESULT_ERROR;
	if (NULL != pstMutex)
	{
		
#ifdef stiMUTEX_DEBUGGING
		pthread_mutex_lock(&cs_mutex);
		MutexTable[pstMutex].pop();
		if (MutexTable[pstMutex].empty())
		{
			DebugMutexMap::iterator it = MutexTable.find(pstMutex);
			if (it != MutexTable.end())
			{
				MutexTable.erase(it);
			}
		}
		pthread_mutex_unlock(&cs_mutex);
#endif
		if (!pthread_mutex_unlock(pstMutex))
		{
			eResult = stiRESULT_SUCCESS;
		}
	} /* end if */

	return (eResult);
} /* end stiOSMutexUnlock */


/* end file stiOSWin32Mutex.c */
