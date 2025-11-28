/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSLinuxMutex
*
*  File Name:	stiOSLinuxMutex.h
*
*  Owner:		Scot L. Brooksby
*
*	Abstract:
*		ACTION - enter description
*
*******************************************************************************/
#ifndef STIOSLINUXMUTEX_H
#define STIOSLINUXMUTEX_H

/*
 * Includes
 */
#include <pthread.h>
#include "stiOSLinuxTask.h"

/*
 * Constants
 */

/*
 * Typedefs
 */
#ifdef stiMUTEX_DEBUGGING
typedef struct SstiMutex
{
	pthread_mutex_t mutex;
	pthread_mutex_t cond_mutex;
	int m_nLockCount;
	stiTaskID m_LockedBy;
	pthread_t m_nLockedBy;
	int m_nWaiters;
	int nDestroyed;
	const char *pszName;
} SstiMutex;
#define stiMUTEX_ID		struct SstiMutex *
#define stiACTUAL_MUTEX(a)	(&(a)->mutex)
#else
#define stiMUTEX_ID		pthread_mutex_t *
#define stiACTUAL_MUTEX(a)	(a)
#endif

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

/*#ifdef stiMUTEX_DEBUGGING
#define stiOSMutexCreate() stiOSMutexCreateDebug (__FILE__, __LINE__)
stiMUTEX_ID stiOSMutexCreateDebug (
	const char *pszFileName,
	int nLineNumber);
#else*/
void stiOSRecursiveMutexCreate(pthread_mutex_t *pstMutex);

stiMUTEX_ID stiOSNamedMutexCreate (const char *pszName);
stiMUTEX_ID stiOSMutexCreate ();
/*#endif*/
stiHResult stiOSMutexDestroy (stiMUTEX_ID mutexID);
stiHResult stiOSMutexLock (stiMUTEX_ID mutexID);
stiHResult stiOSMutexUnlock (stiMUTEX_ID mutexID);

void stiOSMutexLockInfoRecord (
	stiMUTEX_ID pstMutex);

void stiOSMutexUnlockInfoRecord (
	stiMUTEX_ID pstMutex);

#endif // ifndef STIOSLINUXMUTEX_H
/* end file stiOSLinuxMutex.h */
