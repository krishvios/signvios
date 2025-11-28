/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSLinuxMutex
*
*	File Name:		stiOSLinuxMutex.c
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
#include <cstdlib> /* for malloc () */
#include "stiOS.h"
#include "stiOSLinuxMutex.h"
#include "stiTrace.h"
#ifndef WIN32
#include <sys/time.h>
#endif // WIN32


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

#if defined stiMVRS_APP || DEVICE == DEV_NTOUCH_MAC || APPLICATION == APP_DHV_IOS || APPLICATION == APP_DHV_ANDROID
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif


void stiOSRecursiveMutexCreate(pthread_mutex_t *pstMutex)
{
#if defined __USE_GNU || defined WIN32 || defined __APPLE__ || APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
	pthread_mutexattr_t stAttribute;
	pthread_mutexattr_init (&stAttribute);
	pthread_mutexattr_settype (&stAttribute, PTHREAD_MUTEX_RECURSIVE_NP);

	/* Initialize the mutex as a recursive mutex */
	pthread_mutex_init (pstMutex, &stAttribute);

	pthread_mutexattr_destroy (&stAttribute);
#else
	/* Initialize the mutex as a normal mutex */
	pthread_mutex_init (pstMutex, NULL);
#endif

}

/* stiOSMutexCreate - see stiOSMutex.h for documentation */
stiMUTEX_ID stiOSMutexCreate ()
{
	return stiOSNamedMutexCreate (nullptr);
}


stiMUTEX_ID stiOSNamedMutexCreate (
	const char *pszName)
{
#ifdef WIN32
	int result = pthread_win32_process_attach_np();
#endif

#ifdef stiMUTEX_DEBUGGING
	SstiMutex *pstMutex = NULL;

	/* Allocate a pthread mutex structure */
	pstMutex = (SstiMutex *) malloc (sizeof (SstiMutex));
#else
	pthread_mutex_t *pstMutex;

	pstMutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
#endif

	if (nullptr != pstMutex)
	{
		stiOSRecursiveMutexCreate(stiACTUAL_MUTEX(pstMutex));

#ifdef stiMUTEX_DEBUGGING
		pstMutex->m_nLockCount = 0;
		pstMutex->m_nWaiters = 0;
		pstMutex->nDestroyed = 0;
		pstMutex->pszName = pszName;
		pstMutex->m_LockedBy = stiOS_TASK_INVALID_ID;

		stiOSRecursiveMutexCreate(&pstMutex->cond_mutex);
#endif // stiMUTEX_DEBUGGING
	} /* end if */

	return ((stiMUTEX_ID) pstMutex);
} /* end stiOSMutexCreate */


/* stiOSMutexDestroy - see stiOSMutex.h for documentation */
stiHResult stiOSMutexDestroy (
	stiMUTEX_ID pstMutex)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (nullptr != pstMutex, stiRESULT_ERROR);

#ifdef stiMUTEX_DEBUGGING
	pthread_mutex_lock (&pstMutex->cond_mutex);

	if (pstMutex->m_nLockCount > 0 || pstMutex->m_nWaiters > 0)
	{
		stiTrace ("Destroying mutex %p (%s), Locks: %d, Waiters: %d\n", pstMutex, pstMutex->pszName, pstMutex->m_nLockCount,
				  pstMutex->m_nWaiters);
	}

	if (!pthread_mutex_destroy(stiACTUAL_MUTEX(pstMutex)))
	{
		pthread_mutex_destroy (&pstMutex->cond_mutex);

		pstMutex->nDestroyed = 1;

		if (pstMutex->m_LockedBy != stiOS_TASK_INVALID_ID)
		{
			stiOSLinuxTaskRelease (pstMutex->m_LockedBy);
			pstMutex->m_LockedBy = stiOS_TASK_INVALID_ID;
		}

		DEALLOCATE(pstMutex);
	}
	else
	{
		pthread_mutex_unlock (&pstMutex->cond_mutex);

		stiTHROW_NOLOG (stiRESULT_ERROR);
	}
#else
	{
		int nResult = pthread_mutex_destroy(stiACTUAL_MUTEX(pstMutex));
		stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);

		DEALLOCATE(pstMutex);
	}
#endif

STI_BAIL:

	return (hResult);
} /* end stiOSMutexCreate */


#ifdef stiMUTEX_DEBUGGING
static void TraceMutexInfo (
	stiMUTEX_ID pstMutex)
{
	const char *pszSelfName = stiOSTaskName (stiOSTaskIDSelf ());

	if (pszSelfName)
	{
		stiTrace ("%s (0x%x) ", pszSelfName, pthread_self ());
	}
	else
	{
		stiTrace ("0x%x ", pthread_self ());
	}

	stiTrace ("waiting on mutex %s (%p) locked by ", pstMutex->pszName, pstMutex);

	const char *pszOtherName = stiOSTaskName(pstMutex->m_LockedBy);

	if (pszOtherName)
	{
		stiTrace ("%s (%p)\n", pszOtherName, pstMutex->m_nLockedBy);
		stiOSTaskMutexPrint (pstMutex->m_LockedBy);
	}
	else
	{
		stiTrace ("%p\n", pstMutex->m_nLockedBy);
	}
	
#ifdef stiMUTEX_DEBUGGING_BACKTRACE
	stiBackTrace ();
#endif
}

static void TraceLockFailed (
	stiMUTEX_ID pstMutex)
{
	const char *pszSelfName = stiOSTaskName (stiOSTaskIDSelf ());

	stiTrace ("*** ");

	if (pszSelfName)
	{
		stiTrace ("%s (0x%x) ", pszSelfName, pthread_self ());
	}
	else
	{
		stiTrace ("0x%x ", pthread_self ());
	}

	stiTrace ("waiting on mutex %s (%p: %x) locked by ", pstMutex->pszName, pstMutex, pstMutex->mutex);

	const char *pszOtherName = stiOSTaskName(pstMutex->m_LockedBy);

	if (pszOtherName)
	{
		stiTrace ("%s (%p) ", pszOtherName, pstMutex->m_nLockedBy);
		stiOSTaskMutexPrint (pstMutex->m_LockedBy);
	}
	else
	{
		stiTrace ("%p ", pstMutex->m_nLockedBy);
	}

	stiTrace ("failed to get a lock\n");

#ifdef stiMUTEX_DEBUGGING_BACKTRACE
	stiBackTrace ();
#endif
}
#endif


#ifdef stiMUTEX_DEBUGGING
void stiOSMutexLockInfoRecord (
	stiMUTEX_ID pstMutex)
{
	pthread_mutex_lock (&pstMutex->cond_mutex);

	++pstMutex->m_nLockCount;

	pstMutex->m_nLockedBy = pthread_self ();

	stiTaskID ThreadId = stiOSLinuxTaskIDSelf ();

	//
	// Was the thread found?
	//
	if (ThreadId != stiOS_TASK_INVALID_ID)
	{

		//
		// Yes.
		//
		// Make sure we only addref the mutex once.  Releasing
		// of the mutex is controlled by the m_nLockCount data member.
		//
		if (pstMutex->m_LockedBy == stiOS_TASK_INVALID_ID)
		{
			pstMutex->m_LockedBy = ThreadId;
			stiOSLinuxTaskAddRef (pstMutex->m_LockedBy);
		}

		//
		// Add the mutex to the list of mutexes the thread is holding.
		//
		stiOSTaskMutexAdd (ThreadId, pstMutex);
	}

	pthread_mutex_unlock (&pstMutex->cond_mutex);
}


void stiOSMutexUnlockInfoRecord (
	stiMUTEX_ID pstMutex)
{
	--pstMutex->m_nLockCount;

	stiTaskID ThreadId = stiOSTaskIDSelf ();

	if (ThreadId != stiOS_TASK_INVALID_ID)
	{
		stiOSTaskMutexRemove (ThreadId, pstMutex);
	}

	if (pstMutex->m_nLockCount == 0)
	{
		if (pstMutex->m_LockedBy != stiOS_TASK_INVALID_ID)
		{
			stiOSLinuxTaskRelease (pstMutex->m_LockedBy);
			pstMutex->m_LockedBy = stiOS_TASK_INVALID_ID;
		}
		pstMutex->m_nLockedBy = 0;
	}
	else if (pstMutex->m_nLockCount < 0)
	{
		stiTrace ("Lock count less than 0: %p\n", pstMutex);
	}
}
#endif

/* stiOSMutexLock - see stiOSMutex.h for documentation */
stiHResult stiOSMutexLock (
	stiMUTEX_ID pstMutex)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (nullptr != pstMutex, stiRESULT_ERROR);

#ifdef stiMUTEX_DEBUGGING
	int nResult;

	//
	// This algorithm tries to lock the mutex.  If it cannot it waits on a condition
	// until another process unlocks the mutex at which point it tries again.
	//
	pthread_mutex_lock (&pstMutex->cond_mutex);
	pstMutex->m_nWaiters++;
	pthread_mutex_unlock (&pstMutex->cond_mutex);

	for (;;)
	{
		pthread_mutex_lock (&pstMutex->cond_mutex);
		if (pstMutex->nDestroyed)
		{
			stiTrace ("*** Accessing destroyed mutex\n");
		}
		pthread_mutex_unlock (&pstMutex->cond_mutex);

		timeval TimeNow;
		timespec Time;

		gettimeofday (&TimeNow, NULL);
		Time.tv_sec = TimeNow.tv_sec + 5;
		Time.tv_nsec = TimeNow.tv_usec * 1000;

		nResult = pthread_mutex_timedlock (stiACTUAL_MUTEX(pstMutex), &Time);

		if (nResult != ETIMEDOUT)
		{
			pthread_mutex_lock (&pstMutex->cond_mutex);
			pstMutex->m_nWaiters--;
			pthread_mutex_unlock (&pstMutex->cond_mutex);

			if (nResult == 0)
			{
				stiOSMutexLockInfoRecord (pstMutex);
			}
			else
			{
				TraceLockFailed (pstMutex);
				TraceMutexInfo (pstMutex);

				stiTHROW_NOLOG (stiRESULT_ERROR);
			}

			break;
		}

		TraceMutexInfo (pstMutex);
	}
#else
	{
		int nResult = pthread_mutex_lock (stiACTUAL_MUTEX(pstMutex));
		stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	}
#endif

STI_BAIL:

	return (hResult);
} /* end stiOSMutexLock */


/* stiOSMutexUnlock - see stiOSMutex.h for documentation */
stiHResult stiOSMutexUnlock (
	stiMUTEX_ID pstMutex)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (nullptr != pstMutex, stiRESULT_ERROR);

#ifdef stiMUTEX_DEBUGGING
	pthread_mutex_lock (&pstMutex->cond_mutex);

	if (pstMutex->nDestroyed)
	{
		stiTrace ("*** Accessing destroyed mutex\n");
	}

	if (pthread_mutex_unlock (&pstMutex->mutex) == 0)
	{
		stiOSMutexUnlockInfoRecord (pstMutex);
		pthread_mutex_unlock (&pstMutex->cond_mutex);
	}
	else
	{
		stiTrace ("Failed to unlock %p\n", pstMutex);
		pthread_mutex_unlock (&pstMutex->cond_mutex);

		stiTHROW_NOLOG (stiRESULT_ERROR);
	}

#else
	{
		int nResult = pthread_mutex_unlock (stiACTUAL_MUTEX (pstMutex));
		stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	}
#endif

STI_BAIL:

	return (hResult);
} /* end stiOSMutexUnlock */


/* end file stiOSLinuxMutex.c */
