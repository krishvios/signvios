/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSTask
*
*  File Name:	stiOSWin32Task.cpp
*
*  Owner:		Scot L. Brooksby
*
*	Abstract:
*		Implements the OS task functions under Win32.
*
*******************************************************************************/

/*
 * Includes
 */

#include <stdlib.h>
#include <string.h>		/* for strncpy */

#include "stiOS.h"
#include "stiOSWin32Mutex.h"
#include "stiOSWin32Task.h"
#include "stiTrace.h"

/*
 * Constants
 */
#if defined(stiHOLDSERVER) || defined(stiMEDIASERVER)
	#define g_nThreadCount 12000
#else
	#define g_nThreadCount 60
#endif

/*
 * Typedefs
 */

/*
 * Forward Declarations
 */

/*
 * Globals
 */
static stiTaskID g_apsThreadInfo[g_nThreadCount];
static EstiBool g_bIsInitialized = estiFALSE;
static pthread_once_t task_init_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_ThreadInfoMutex;
static pthread_cond_t g_ThreadStartCondition;

/*
 * Locals
 */

/*
 * Function Declarations
 */
void * stiOSWin32ThreadWrapper (void * pArg);
void stiOSWin32ThreadIDFree (void * pArg);


static void TaskInit (void)
{
    stiOSRecursiveMutexCreate (&g_ThreadInfoMutex);
	pthread_cond_init (&g_ThreadStartCondition, NULL);
}


void stiOSWin32TaskAddRef (
	SstiTaskID *pTaskID)
{
#if 1
//	pthread_mutex_lock (stiACTUAL_MUTEX (pTaskID->m_LockMutex));
    pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

	++pTaskID->m_nRefCount;

//	pthread_mutex_unlock (stiACTUAL_MUTEX(pTaskID->m_LockMutex));
	pthread_mutex_unlock (&g_ThreadInfoMutex);
#endif
}


void stiOSWin32TaskRelease (
	SstiTaskID *pTaskID)
{
#if 1
	if (pTaskID == NULL)
	{
		stiTrace ("pTaskID == NULL\n");
	}

//	pthread_mutex_lock (stiACTUAL_MUTEX (pTaskID->m_LockMutex));
    pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

	--pTaskID->m_nRefCount;

	if (pTaskID->m_nRefCount == 0)
	{
//		stiOSMutexDestroy (pTaskID->m_LockMutex);

#if 0
		stiTrace ("Deleted task information: %s\n", pTaskID->m_szTaskName);
#endif
	delete pTaskID;
	}
#if 0
	else if (pTaskID->m_nRefCount < 0)
	{
		stiTrace ("TaskID ref count is less than zero! %d\n", pTaskID->m_nRefCount);
	}
#endif
//	else
//	{
//		pthread_mutex_unlock (stiACTUAL_MUTEX (pTaskID->m_LockMutex));
		pthread_mutex_unlock (&g_ThreadInfoMutex);
//	}

#endif
}

/*******************************************************************************
*; stiOSWin32TaskSpawn
*
* Description: Spawns a task.
*
* Abstract:
*
* Returns:
*	A valid stiTaskID, or stiOS_TASK_INVALID_ID on error
*/
stiTaskID stiOSWin32TaskSpawn (
	const char* pszName,	/* task name */
	int nPriority,	/* task priority */
	int nStackSize,	/* desired stack size */
    stiPTHREAD_TASK_FUNC_PTR pEntry, /* pointer to the task entry function */
	void* pArg)		/* argument to the task entry function */
{
	int nReturn = 0;

	SstiTaskID *pThreadID = (SstiTaskID *)stiOS_TASK_INVALID_ID;

	/* Allocate a new structure for this thread */
//	pThreadID = (PSstiTaskID) malloc (sizeof (*pThreadID));
	pThreadID = new SstiTaskID;

	/* Did we allocate the memory? */
	if (pThreadID != NULL)
	{
		pThreadID->m_nRefCount = 0;

//		pThreadID->m_LockMutex = stiOSMutexCreate ();

		pthread_attr_t stThreadAttributes;

		/* Yes! Set the start routine and argument */
		pThreadID->m_pTaskProc = pEntry;
		pThreadID->m_pArg = pArg;

		/* Set the task name without overfilling the buffer */
		strncpy (
			pThreadID->m_szTaskName,
			pszName,
			sizeof (pThreadID->m_szTaskName) - 1);
		pThreadID->m_szTaskName[sizeof (pThreadID->m_szTaskName) - 1] = '\0';

		/* Initialize the thread attributes */
		nReturn = pthread_attr_init (&stThreadAttributes);

		if (!nReturn)
		{
			if (nStackSize != stiOS_TASK_DEFAULT_STACK_SIZE)
			{
            /*
             * Silently enforce a minimum stack size to prevent
             * setstacksize from failing.
             */
            if (nStackSize < stiOS_TASK_MIN_STACK_SIZE) {
               nStackSize = stiOS_TASK_MIN_STACK_SIZE;
            }
			nReturn = pthread_attr_setstacksize (&stThreadAttributes, nStackSize);
			}

            if (!nReturn)
			{
                /* Now spawn the thread */
                nReturn = pthread_create (
                        &(pThreadID->m_stPThread),	/* Thread Structure */
                        &stThreadAttributes,		/* Attributes */
                        stiOSWin32ThreadWrapper,	/* Start Routine */
                        pThreadID);					/* Arguments */
            }
			if (!nReturn)
			{
		    /* Since we won't ever pthread_join() on this thread, don't leak memory */
		    nReturn = pthread_detach (pThreadID->m_stPThread);
	    }
			/* Destroy the thread attributes */
			pthread_attr_destroy (&stThreadAttributes);
		}

        if (nReturn)
		{
			delete pThreadID;
			pThreadID = (PSstiTaskID)stiOS_TASK_INVALID_ID;
		} /* end if */
		else
		{
		    /* Store the thread info structure in an array for lookups */
		    ThreadInfoStore (pThreadID);

			//
			// Signal the condition now that we have the thread stored.
			//
			pthread_cond_broadcast (&g_ThreadStartCondition);

#if 0  // Enable to use Round Robin scheduler.  This hasn't been fully tested.  (ERC)
			// Set the process scheduler method and priority.
			struct sched_param stSched;
			stSched.sched_priority = nPriority;
			int nRet = pthread_setschedparam (pThreadID->m_stPThread, SCHED_RR, &stSched);
			int nMethod;

			if (nRet != 0)
			{
				printf ("<stiOSWin32TaskSpawn> pthread_setschedparam returned %d for %s\n", nRet, pszName);
			}

			nRet = pthread_getschedparam (pThreadID->m_stPThread, &nMethod, &stSched);

			printf ("<stiOSWin32TaskSpawn> Task %s, method = %s, priority = %d\n",
				pszName,
				SCHED_RR == nMethod ? "RoundRobin" :
				SCHED_FIFO == nMethod ? "FIFO" :
				SCHED_OTHER == nMethod ? "Other" :
				"???",
				stSched.sched_priority);
#endif
		} /* end else */
	} /* end if */

	return (pThreadID);
} /* end stiOSWin32TaskSpawn */

#if APPLICATION == APP_NTOUCH_PC
//
// Usage: SetThreadName ("ThreadName");
//
#include <windows.h>
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD	dwType;		// Must be 0x1000.
   LPCSTR	szName;		// Pointer to name (in user addr space).
   DWORD	dwThreadID; // Thread ID (-1=caller thread).
   DWORD	dwFlags;	// Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadName(const char* threadName)
{
   THREADNAME_INFO info;
   info.dwType		= 0x1000;
   info.szName		= threadName;
   info.dwThreadID	= -1;  // sets current thread name
   info.dwFlags		= 0;
   __try
   {
      RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}
#endif

/*******************************************************************************
*; stiOSWin32ThreadWrapper
*
* Description: Wraps the thread start function so resources are freed
*
* Abstract:
*
* Returns:
*	void * - the return value from the thread procedure
*/
void * stiOSWin32ThreadWrapper (
	void * pArg)
{
	PSstiTaskID pTaskID = (PSstiTaskID)pArg;
	void * pReturn = NULL;

	/* Push our freeing function onto the cleanup stack */
	pthread_cleanup_push (stiOSWin32ThreadIDFree, pArg);

	//
	// Wait to make sure we are in the thread array before proceeding.
	//
    pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);
	// We do not want to contiue if stiOSWin32TaskIDSelf () == stiOS_TASK_INVALID_ID as this will cause problems.
	while (stiOSWin32TaskIDSelf () == stiOS_TASK_INVALID_ID)
	{
		pthread_cond_wait (&g_ThreadStartCondition, &g_ThreadInfoMutex);
	}

	pthread_mutex_unlock (&g_ThreadInfoMutex);

#if APPLICATION == APP_NTOUCH_PC
	SetThreadName(pTaskID->m_szTaskName);
#endif

	/* Call the task function */
	pReturn = pTaskID->m_pTaskProc (pTaskID->m_pArg);

	/* Pop our freeing function from the cleanup stack, executing it */
	pthread_cleanup_pop (1);

	return (pReturn);
} /* end stiOSWin32ThreadWrapper */

/*******************************************************************************
*; stiOSWin32ThreadIDFree
*
* Description: Frees resources allocated by stiOSWin32TaskSpawn
*
* Abstract:
*
* Returns:
*	None
*/
void stiOSWin32ThreadIDFree (
	void * pArg)
{
    stiTaskID pTaskId = (stiTaskID)pArg;

    /* Look for and remove the task info structure from the global array */
    int i;

    pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

#if 0
	stiTrace ("Release thread information for %s (refcount = %d)\n", pTaskId->m_szTaskName, pTaskId->m_nRefCount);
#endif

    /* Locate the task info structure for this thread */
    for (i = 0; i < g_nThreadCount; i++)
    {
        if (pTaskId == g_apsThreadInfo[i])
        {
            g_apsThreadInfo[i] = NULL;

			stiOSWin32TaskRelease (pTaskId);

            /* Found it, break from the loop */
            break;
        } /* end if */
    } /* end for */

#if 0
	if (i == g_nThreadCount)
	{
		stiTrace ("Could not find thread information for %s\n", pTaskId->m_szTaskName);
	}
#endif

	pthread_mutex_unlock (&g_ThreadInfoMutex);

} /* end stiOSWin32ThreadIDFree */

/*******************************************************************************
*; stiOSWin32TaskDelete
*
* Description: Forcibly stops execution of a task.
*
* Abstract:
*
* Returns:
*	EstiResult - estiOK on success, otherwise estiERROR
*/
EstiResult stiOSWin32TaskDelete (
	stiTaskID pTaskID)
{
	EstiResult eResult = estiERROR;

	/* Is the task ID valid? */
	if ((pTaskID != NULL) && (pTaskID != stiOS_TASK_INVALID_ID))
	{
		/* Yes! Cancel this task. */
		if (0 == pthread_cancel (pTaskID->m_stPThread))
		{
			eResult = estiOK;
		} /* end if */
	} /* end if */

	return (eResult);
} /* end stiOSWin32TaskDelete */


/*******************************************************************************
*; stiOSWin32TaskDelay
*
* Description: Delays a task by the specified number of ticks
*
* Abstract:
*	One tick is implemented as 1 millisecond.
*
* Returns:
*	EstiResult - estiOK on success, otherwise estiERROR
*/
EstiResult stiOSWin32TaskDelay (
	int nTicks)
{
	unsigned long ulMicroSeconds;
	struct timeval stNow;
	struct timeval stEnd;
	struct timeval stTest;

	/* Convert the delay from ticks (milliseconds) to microseconds */
	ulMicroSeconds = (unsigned long)(nTicks * TICKS_PER_SEC);

	/* Have we been asked to delay for Zero time? */
	if (ulMicroSeconds == 0L)
	{
		/* Yes! Just yield the CPU */
		sched_yield ();
	} /* end if */
	else
	{
		/*
		 * Calculate the time of the end of the delay by adding the delay to
		 * the current time.
		 */
		gettimeofday (&stNow, (struct timezone *)NULL);
		stEnd.tv_sec = stNow.tv_sec;
		stEnd.tv_usec = stNow.tv_usec;
		stEnd.tv_usec += ulMicroSeconds;

		/* Has the microsecond value wrapped? */
		if (stEnd.tv_usec > 1000000)
		{
			/* Yes! Bump up the second value to compensate */
			stEnd.tv_sec += stEnd.tv_usec / 1000000;
			stEnd.tv_usec = stEnd.tv_usec % 1000000;
		} /* end if */

		/*
		 * Wait for the time of day to reach the timeout value. Loop around if
		 * the usleep call returns because of a signal.
		 */
		while (ulMicroSeconds > 0)
		{
			/* Sleep for the necessary time */
			usleep (ulMicroSeconds);

			/* Check to see if the thread has been cancelled */
			pthread_testcancel ();

			/* Check the time */
			gettimeofday (&stNow, (struct timezone *)NULL);

			/* Has the system clock been changed?  If so, is the time that
			 * we are now set to expire longer than the original amount
			 * that was intended?  If so, set the timer to expire now.
			 */
			stTest.tv_sec = stNow.tv_sec;
			stTest.tv_usec = stNow.tv_usec;
			stTest.tv_usec += ulMicroSeconds;

			/* If the microsecond value has exceeded a second, modify. */
			if (stTest.tv_usec > 1000000)
			{
			    stTest.tv_sec += stTest.tv_usec / 1000000;
			    stTest.tv_usec = stTest.tv_usec % 1000000;
			} /* end if */

			/* Now that we have set up what the exit time would be if
			 * we started the timer now, is it less than the current
			 * end time?
			 */
			if (stTest.tv_sec < stEnd.tv_sec ||
			    (stTest.tv_sec == stEnd.tv_sec &&
			     stTest.tv_usec < stEnd.tv_usec))
			{
			    /* We've detected a change in the system clock.  Set the
			     * timer to expire now.
			     */
			     stEnd.tv_usec = stNow.tv_usec + 1;
			     stEnd.tv_sec = stNow.tv_sec - 2;
			} // end if



			/* Is the microsecond value larger than our end time? */
			if (stEnd.tv_usec > stNow.tv_usec)
			{
				/* Yes! Is the second value large as well? */
				if (stEnd.tv_sec < stNow.tv_sec)
				{
					/* Yes! We are done. Bail out of the loop */
					ulMicroSeconds = 0;
				} /* end if */
				else
				{
					/* No! Calculate a new wait time */
					ulMicroSeconds =
						(((stEnd.tv_sec - stNow.tv_sec) * 1000000)) +
						(stEnd.tv_usec - stNow.tv_usec);
				} /* end else */
			} /* end if */
			else
			{
				/* No! Has the time still expired? */
				if ((stEnd.tv_sec - 1) < stNow.tv_sec)
				{
					/* Yes! We are done. Bail out of the loop */
					ulMicroSeconds = 0;
				} /* end if */
				else
				{
					/* No! Calculate a new wait time */
					ulMicroSeconds =
						((stEnd.tv_usec + 1000000) - stNow.tv_usec) +
						(((stEnd.tv_sec - 1) - stNow.tv_sec) * 1000000);
				} /* end if */
			} /* end else */
		} /* end while */
	} /* end else */

	return (estiOK);
} /* end stiOSWin32TaskDelay */

/*******************************************************************************
*; stiOSWin32TaskName
*
* Description: Returns the name of the specified task
*
* Abstract:
*
* Returns:
*	char * - Name of specified task, otherwise NULL
*/
const char * stiOSWin32TaskName (
	stiTaskID pTaskID)
{
	const char * szName = NULL;

	/* Is the task ID valid? */
	if ((pTaskID != NULL) && (pTaskID != stiOS_TASK_INVALID_ID))
	{
		/* Yes! Return the name of this task. */
		szName = pTaskID->m_szTaskName;
	} /* end if */

	return (szName);
} /* end stiOSWin32TaskName */


/********************************************************************************
*; ThreadInfoStore
*
* Description: Stores the thread informational structure for future lookups
*
* Abstract:
*
* Returns:
*   none
*/
void ThreadInfoStore (
    stiTaskID pTaskID)
{
    int i;

    pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

    if (!g_bIsInitialized)
    {
        /* The global array hasn't yet been initialized.  Do so now */
        for (i = 0; i < g_nThreadCount; i++)
        {
            g_apsThreadInfo[i] = NULL;
        } /* end for */

        g_bIsInitialized = estiTRUE;
    } /* end if */

    /* Locate an array location that isn't in use */
    for (i = 0; i < g_nThreadCount; i++)
    {
        if (NULL == g_apsThreadInfo[i])
        {
            /* This element is available for use.
             * Store the task info here and drop out
             * of the loop.
             */
             g_apsThreadInfo[i] = pTaskID;
			 stiOSWin32TaskAddRef(pTaskID);

             break;
        } /* end if */
    } /* end for */

    if (i == g_nThreadCount)
	{
		stiTrace ("Thread Info array size exceeded.\n");
	}

	pthread_mutex_unlock (&g_ThreadInfoMutex);

} /* end ThreadInfoStore */


/********************************************************************************
*; stiOSWin32TaskIDSelf
*
* Description: Retrieve the task info structure for the current thread.
*
* Abstract:
*
* Returns:
*   The task info structure for the given thread.
*/
stiTaskID stiOSWin32TaskIDSelf (void)
{
    pthread_t pThread = pthread_self ();
    int i;
    stiTaskID taskId = stiOS_TASK_INVALID_ID;

    pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

    /* Locate the task info structure for this thread */
    for (i = 0; i < g_nThreadCount; i++)
    {
		if (NULL != g_apsThreadInfo[i]
			&& ((pThread.p == g_apsThreadInfo[i]->m_stPThread.p)
				&& (pThread.x == g_apsThreadInfo[i]->m_stPThread.x)))
        {
            taskId = g_apsThreadInfo[i];
            break;
        } /* end if */
    } /* end for */

	pthread_mutex_unlock (&g_ThreadInfoMutex);

	return (taskId);
} /* end stiOSWin32TaskIDSelf */



stiTaskID stiOSWin32TaskRegister (
	const std::string &taskName)
{
	pthread_t thread = pthread_self ();
	SstiTaskID *t = new SstiTaskID;

	strncpy(t->m_szTaskName, taskName.c_str(), sizeof(t->m_szTaskName) - 1);
	t->m_szTaskName[sizeof (t->m_szTaskName) - 1] = '\0';
	t->m_stPThread = thread;
	t->m_pTaskProc = NULL;
	t->m_pArg      = NULL;
	t->m_nRefCount = 0;

	ThreadInfoStore(t);
	return t;
}

void stiOSWin32TaskUnregister (
       stiTaskID taskId)
{
       stiOSWin32ThreadIDFree (taskId);
}

void stiOSWin32TaskSetCurrentThreadName (
       const std::string &threadName)
{
//This function is currently only defined for ntouchPC.
#if APPLICATION == APP_NTOUCH_PC
	SetThreadName(threadName.c_str());
#endif
}

/* end file stiOSWin32Task.cpp */
