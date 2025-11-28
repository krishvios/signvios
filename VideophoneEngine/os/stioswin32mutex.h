/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSWin32Mutex
*
*  File Name:	stiOSWin32Mutex.h
*
*  Owner:		Scot L. Brooksby
*
*	Abstract:
*		ACTION - enter description
*
*******************************************************************************/
#ifndef STIOSWIN32MUTEX_H
#define STIOSWIN32MUTEX_H
/*
 * Includes
 */

#include <pthread.h>
#include "stiOSWin32Task.h"
#ifdef stiMUTEX_DEBUGGING
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "Dbghelp")
#include <DbgHelp.h>
#include <unordered_map>
#include <stack>
#endif
/*
 * Constants
 */

/*
 * Typedefs
 */
#define stiMUTEX_ID		pthread_mutex_t *
#define stiACTUAL_MUTEX(a)	(a)

#ifdef stiMUTEX_DEBUGGING
static pthread_mutex_t cs_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#define StackTraceCount 3

typedef struct SstiMutexInformation
{
	int				m_nThreadID;
	void*			m_pBackTrace[StackTraceCount];
	unsigned short	m_nFrames;
} SstiMutexInformation;

typedef std::unordered_map<stiMUTEX_ID, std::stack<SstiMutexInformation>> DebugMutexMap;
static DebugMutexMap MutexTable;
#endif

void stiOSRecursiveMutexCreate(pthread_mutex_t *pstMutex);

stiMUTEX_ID stiOSNamedMutexCreate (const char *pszName);
stiMUTEX_ID stiOSMutexCreate ();

stiHResult stiOSMutexDestroy (stiMUTEX_ID mutexID);
stiHResult stiOSMutexLock (stiMUTEX_ID mutexID);
stiHResult stiOSMutexUnlock (stiMUTEX_ID mutexID);


#endif // ifndef STIOSWIN32MUTEX_H
/* end file stiOSWin32Mutex.h */
