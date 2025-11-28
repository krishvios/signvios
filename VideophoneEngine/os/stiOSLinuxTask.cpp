/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSTask
*
*  File Name:	stiOSLinuxTask.c
*
*  Owner:		Scot L. Brooksby
*
*	Abstract:
*		Implements the OS task functions under Linux.
*
*******************************************************************************/

/*
 * Includes
 */

#include <cstdlib>
#include <cstring>		/* for strncpy */
#include <sys/syscall.h>

#ifndef WIN32
	#include <sys/time.h>	/* for gettimeofday */
	#include <unistd.h>		/* for usleep */
#endif

#include "stiOS.h"
#include "stiOSLinuxMutex.h"
#include "stiOSLinuxTask.h"
#include "stiTrace.h"
#include "stiSysUtilizationInfo.h"

#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
#include <map>
#define SYS_gettid __NR_gettid
#endif

/*
 * Constants
 */
	#define g_nThreadCount 2500
//#define DEBUG_THREAD_COUNT

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
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
static std::map<pthread_t, EstiBool> g_bmIsCancelled;
#endif

#define MAX_LIB_THREADS 4
static int g_numLibThreads = 0;
static SstiTaskID g_libThreads[MAX_LIB_THREADS];

extern "C"
{
	void AddLibThreadToCPUUsage(pthread_t thread, char *name)
	{
		char taskName[sizeof(g_libThreads[0].m_szTaskName)];
		strncpy(taskName, name, sizeof (g_libThreads[0].m_szTaskName) - 1);
		taskName[sizeof(g_libThreads[0].m_szTaskName) - 1] = 0;

		bool found = false;
		int  i     = -1;
		for (int j=0; (j<g_numLibThreads) && (!found); j++)
		{
			if (strcmp(taskName, name) == 0)
			{
				found = true;
				i     = j;
			}
		}

		if ((i < 0) && (g_numLibThreads < MAX_LIB_THREADS))
		{
			i = g_numLibThreads++;
		}

		if (i >= 0)
		{
			g_libThreads[i].m_stPThread = thread;
			g_libThreads[i].m_pTaskProc = nullptr;
			g_libThreads[i].m_pArg		= nullptr;
			strcpy(g_libThreads[i].m_szTaskName, taskName);
			g_libThreads[i].m_nRefCount	= 0;
			g_libThreads[i].m_nTID		= stiOSLinuxTaskThreadIdGet ();
			g_libThreads[i].m_nTaskTime	= 0;

			ThreadInfoStore (&g_libThreads[i]);
		}
	}
}


/*
 * Locals
 */

/*
 * Function Declarations
 */
void * stiOSLinuxThreadWrapper (void * pArg);
void stiOSLinuxThreadIDFree (void * pArg);


static void TaskInit ()
{
	stiOSRecursiveMutexCreate (&g_ThreadInfoMutex);
	pthread_cond_init (&g_ThreadStartCondition, nullptr);
}


void stiOSLinuxTaskAddRef (
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


void stiOSLinuxTaskRelease (
	SstiTaskID *pTaskID)
{
#if 1
	if (pTaskID == nullptr)
	{
		stiTrace ("pTaskID == NULL\n");
	}
	else
	{
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
	}
#endif
}


/*******************************************************************************
*; stiOSLinuxTaskSpawn
*
* Description: Spawns a task.
*
* Abstract:
*
* Returns:
*	A valid stiTaskID, or stiOS_TASK_INVALID_ID on error
*/
stiTaskID stiOSLinuxTaskSpawn (
	const char* pszName,	/* task name */
	int nPriority,	/* task priority */
	int nStackSize,	/* desired stack size */
	stiPTHREAD_TASK_FUNC_PTR pEntry, /* pointer to the task entry function */
	void* pArg)		/* argument to the task entry function */
{
	int nReturn = 0;

	auto pThreadID = (SstiTaskID *)stiOS_TASK_INVALID_ID;

	/* Allocate a new structure for this thread */
//	pThreadID = (PSstiTaskID) malloc (sizeof (*pThreadID));
	pThreadID = new SstiTaskID;

	/* Did we allocate the memory? */
	if (pThreadID != nullptr)
	{
//		pThreadID->m_nRefCount = 0;
		
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
				if (nStackSize < stiOS_TASK_MIN_STACK_SIZE)
				{
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
						stiOSLinuxThreadWrapper,	/* Start Routine */
						pThreadID);					/* Arguments */
			}
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
			if (!nReturn)
			{
				g_bmIsCancelled[pThreadID->m_stPThread] = estiFALSE;
			}
#endif

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
            if (!nReturn)
            {
                char szName[16];
                strncpy(szName, pThreadID->m_szTaskName, 16);
                szName[15] = 0;
                nReturn = pthread_setname_np(pThreadID->m_stPThread, szName);
            }
#endif

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

			stiDEBUG_TOOL (g_stiTaskDebug,
				if (nRet != 0)
				{
					stiTrace ("<stiOSLinuxTaskSpawn> pthread_setschedparam returned %d for %s\n", nRet, pszName);
				}
			);

			nRet = pthread_getschedparam (pThreadID->m_stPThread, &nMethod, &stSched);

			stiDEBUG_TOOL (g_stiTaskDebug,
				stiTrace ("<stiOSLinuxTaskSpawn> Task %s, method = %s, priority = %d\n",
					pszName,
					SCHED_RR == nMethod ? "RoundRobin" :
					SCHED_FIFO == nMethod ? "FIFO" :
					SCHED_OTHER == nMethod ? "Other" :
					"???",
					stSched.sched_priority);
			);
#endif
		} /* end else */
	} /* end if */

	return (pThreadID);
} /* end stiOSLinuxTaskSpawn */

/*******************************************************************************
*; stiOSLinuxThreadWrapper
*
* Description: Wraps the thread start function so resources are freed
*
* Abstract:
*
* Returns:
*	void * - the return value from the thread procedure
*/
void * stiOSLinuxThreadWrapper (
	void * pArg)
{
	auto  pTaskID = (PSstiTaskID)pArg;
	void * pReturn = nullptr;

	/* Push our freeing function onto the cleanup stack */
	pthread_cleanup_push (stiOSLinuxThreadIDFree, pArg);

	//
	// Wait to make sure we are in the thread array before proceeding.
	//
	pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

	// We do not want to contiue if stiOSLinuxTaskIDSelf () == stiOS_TASK_INVALID_ID as this will cause problems.
	while (stiOSLinuxTaskIDSelf () == stiOS_TASK_INVALID_ID)
	{
		pthread_cond_wait (&g_ThreadStartCondition, &g_ThreadInfoMutex);
	}

	pthread_mutex_unlock (&g_ThreadInfoMutex);

	/* Save off the tid */
	pTaskID->m_nTID = stiOSLinuxTaskThreadIdGet ();
	pTaskID->m_nTaskTime = 0;

#if APPLICATION == APP_NTOUCH_MAC
	pthread_setname_np(pTaskID->m_szTaskName);
#endif

	/* Call the task function */
	pReturn = pTaskID->m_pTaskProc (pTaskID->m_pArg);

	/* Pop our freeing function from the cleanup stack, executing it */
	pthread_cleanup_pop (1);

	return (pReturn);
} /* end stiOSLinuxThreadWrapper */

/*******************************************************************************
*; stiOSLinuxThreadIDFree
*
* Description: Frees resources allocated by stiOSLinuxTaskSpawn
*
* Abstract:
*
* Returns:
*	None
*/
void stiOSLinuxThreadIDFree (
	void * pArg)
{
	auto  pTaskId = (stiTaskID)pArg;

	/* Look for and remove the task info structure from the global array */
	int i;

	pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

#if 0
	stiTrace ("Release thread information for %s (refcount = %d)\n", pTaskId->m_szTaskName, pTaskId->m_nRefCount);
#endif

#ifdef DEBUG_THREAD_COUNT
	int numThreads = 0;
	for (i = 0; i < g_nThreadCount; i++)
	{
		numThreads += (g_apsThreadInfo[i] != NULL) ? 1 : 0;
	}
	stiTrace("Num threads before remove (-1): %d", numThreads);
#endif

	/* Locate the task info structure for this thread */
	for (i = 0; i < g_nThreadCount; i++)
	{
		if (pTaskId == g_apsThreadInfo[i])
		{
			g_apsThreadInfo[i] = nullptr;

			stiOSLinuxTaskRelease (pTaskId);

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

} /* end stiOSLinuxThreadIDFree */

/*******************************************************************************
*; stiOSLinuxTaskDelete
*
* Description: Forcibly stops execution of a task.
*
* Abstract:
*
* Returns:
*	EstiResult - estiOK on success, otherwise estiERROR
*/
EstiResult stiOSLinuxTaskDelete (
	stiTaskID pTaskID)
{
	EstiResult eResult = estiERROR;

	/* Is the task ID valid? */
	if ((pTaskID != nullptr) && (pTaskID != stiOS_TASK_INVALID_ID))
	{
		/* Yes! Cancel this task. */
#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
		if (0 == pthread_cancel (pTaskID->m_stPThread))
		{
			eResult = estiOK;
		} /* end if */
#else
		/* Android does not have pthread_cancel(...) support.  A work
		 around using kill is used instead. */
		g_bmIsCancelled[pTaskID->m_stPThread] = estiTRUE;
		eResult = estiOK;
#endif
	} /* end if */

	return (eResult);
} /* end stiOSLinuxTaskDelete */


/*******************************************************************************
*; stiOSLinuxTaskDelay
*
* Description: Delays a task by the specified number of ticks
*
* Abstract:
*	One tick is implemented as 1 millisecond.
*
* Returns:
*	EstiResult - estiOK on success, otherwise estiERROR
*/
EstiResult stiOSLinuxTaskDelay (
	int nTicks)
{
	unsigned long ulMicroSeconds;
	struct timeval stNow{};
	struct timeval stEnd{};
	struct timeval stTest{};

	/* Convert the delay from ticks (milliseconds) to microseconds */
	ulMicroSeconds = static_cast<unsigned long>(nTicks) * TICKS_PER_SEC;

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
		gettimeofday (&stNow, (struct timezone *)nullptr);
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
#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
			pthread_testcancel ();
#else
			/* Android does not have pthread_cancel(...) support.  A work
			 around using pthread_kill is used instead. */
			pthread_t pThread = pthread_self();
			if (g_bmIsCancelled[pThread] == estiTRUE)
			{
				if (pthread_kill(pThread, SIGUSR1) == 0)
				{
					/* Wait for system to kill us off. */
					while (true)
					{
					}
				}
			}
#endif

			/* Check the time */
			gettimeofday (&stNow, (struct timezone *)nullptr);

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
} /* end stiOSLinuxTaskDelay */

extern "C"
{
const char *stiOSSelfTaskName()
{
	return stiOSLinuxTaskName(stiOSLinuxTaskIDSelf());
}
}

/*******************************************************************************
*; stiOSLinuxTaskName
*
* Description: Returns the name of the specified task
*
* Abstract:
*
* Returns:
*	char * - Name of specified task, otherwise NULL
*/
const char * stiOSLinuxTaskName (
	stiTaskID pTaskID)
{
	const char * szName = nullptr;

	/* Is the task ID valid? */
	if ((pTaskID != nullptr) && (pTaskID != stiOS_TASK_INVALID_ID))
	{
		/* Yes! Return the name of this task. */
		szName = pTaskID->m_szTaskName;
	} /* end if */

	return (szName);
} /* end stiOSLinuxTaskName */


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
			g_apsThreadInfo[i] = nullptr;
		} /* end for */

		g_bIsInitialized = estiTRUE;
	} /* end if */

#ifdef DEBUG_THREAD_COUNT
	int numThreads = 0;
	for (i = 0; i < g_nThreadCount; i++)
	{
		numThreads += (g_apsThreadInfo[i] != NULL) ? 1 : 0;
	}
	stiTrace("Num threads before adding (+1): %d", numThreads);
#endif

	/* Locate an array location that isn't in use */
	for (i = 0; i < g_nThreadCount; i++)
	{
		if (nullptr == g_apsThreadInfo[i])
		{
			/* This element is available for use.
			 * Store the task info here and drop out
			 * of the loop.
			 */
			 g_apsThreadInfo[i] = pTaskID;
			 stiOSLinuxTaskAddRef(pTaskID);

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
*; stiOSLinuxTaskIDSelf
*
* Description: Retrieve the task info structure for the current thread.
*
* Abstract:
*
* Returns:
*   The task info structure for the given thread.
*/
stiTaskID stiOSLinuxTaskIDSelf ()
{
	pthread_t pThread = pthread_self ();
	int i;
	stiTaskID taskId = stiOS_TASK_INVALID_ID;

	pthread_once(&task_init_once, TaskInit);
	pthread_mutex_lock (&g_ThreadInfoMutex);

	/* Locate the task info structure for this thread */
	for (i = 0; i < g_nThreadCount; i++)
	{
#ifdef WIN32
		if (NULL != g_apsThreadInfo[i]
			&& ((pThread.p == g_apsThreadInfo[i]->m_stPThread.p)
				&& (pThread.x == g_apsThreadInfo[i]->m_stPThread.x)))
#else
		if (nullptr != g_apsThreadInfo[i] && pthread_equal(pThread, g_apsThreadInfo[i]->m_stPThread))
#endif
		{
			taskId = g_apsThreadInfo[i];
			break;
		} /* end if */
	} /* end for */

	pthread_mutex_unlock (&g_ThreadInfoMutex);

	return (taskId);
} /* end stiOSLinuxTaskIDSelf */


// CstiEventQueue-based tasks need to register themselves in order
// for stiERROR and such to work correctly
// This method needs to be called from the thread that is being register
// due to a limitation in the pthread_setname_np interface for MacOS
stiTaskID stiOSLinuxTaskRegister (
       pthread_t thread,
       const std::string &taskName)
{
       auto t = new SstiTaskID;

       strncpy(t->m_szTaskName, taskName.c_str(), sizeof(t->m_szTaskName) - 1);
	   t->m_szTaskName[sizeof (t->m_szTaskName) - 1] = '\0';
       t->m_stPThread = thread;
       t->m_pTaskProc = nullptr;
       t->m_pArg      = nullptr;
       t->m_nRefCount = 0;
       t->m_nTID      = stiOSLinuxTaskThreadIdGet ();
       t->m_nTaskTime = 0;

       ThreadInfoStore(t);
       return t;
}


void stiOSLinuxTaskUnregister (
       stiTaskID task)
{
       stiOSLinuxThreadIDFree(task);
}


void stiOSLinuxTaskSetCurrentThreadName (
	const std::string &threadName)
{
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
	pthread_setname_np(threadName.c_str());
#else
	pthread_t thread = pthread_self();
	pthread_setname_np(thread, threadName.c_str());
#endif
}


int stiOSLinuxTaskThreadIdGet ()
{
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
	uint64_t tid;
	pthread_threadid_np(nullptr, &tid);
	return tid;
#else
	return static_cast<int>(syscall (SYS_gettid));
#endif
}


void stiOSLinuxTaskTimeReport ()
{
	// We first need to make sure the CPU info is updated to ensure our
	// thread percentages are accurate.
	SstiSysUtilizationInfo CPUInfo;
	stiSystemInfoGet(&CPUInfo);

	const char **names = CPUInfo.cpustate_names;
	int* states = CPUInfo.cpustates;
	const char* thisname;

	stiTrace("Overall CPU usage:\n");
	stiTrace("--------------------------------\n");

	while ((thisname = *names++) != nullptr)
	{
		if (*thisname != '\0')
		{
			// retrieve the value and remember it
			int value = *states++;

			// if percentage is >= 1000, print it as 100%
			stiTrace((value >= 1000 ? "%s: %4.0f%%  " : "%s: %4.1f%%  "), thisname, ((float)value)/10.0);
		}
	}

	stiTrace("\n\n");
	stiTrace("Thread CPU usage:\n");
	stiTrace("--------------------------------\n");

	int nThreadCount = 0;

	for (auto threadInfo: g_apsThreadInfo)
	{
		if (threadInfo)
		{
			SstiThreadUtilizationInfo info;
			info.nTaskId = threadInfo->m_nTID;
			info.nOldTime = threadInfo->m_nTaskTime;

			if (info.nTaskId != 0)
			{
				// Read thread usage
				stiSystemThreadInfoGet(&info);

				stiTrace("%2d. %20s %8d %4d%%\n", ++nThreadCount, threadInfo->m_szTaskName,
					info.nTime - info.nOldTime, info.nPercentage);
				threadInfo->m_nTaskTime = info.nTime;
			}
		}
	}

	stiTrace("\n");
}

#ifdef stiMUTEX_DEBUGGING
void stiOSTaskMutexAdd (
	stiTaskID pTaskID,
	SstiMutex *pstMutex)
{
//    pthread_once(&task_init_once, TaskInit);
//	pthread_mutex_lock (&pTaskID->m_LockMutex->mutex);
#if 0
	std::list<SstiTaskMutex>::iterator i;

	for (i = pTaskID->m_MutexList.begin (); i != pTaskID->m_MutexList.end (); i++)
	{
		if (i->pMutex == pstMutex)
		{
			i->nCount++;

			break;
		}
	}

	if (i == pTaskID->m_MutexList.end ())
	{
#endif
		SstiTaskMutex TaskMutex;

		TaskMutex.pMutex = pstMutex;
//		TaskMutex.nCount = 1;

		if (pstMutex->pszName)
		{
			strncpy (TaskMutex.szName, pstMutex->pszName, sizeof (TaskMutex.szName) - 1);
			TaskMutex.szName[sizeof (TaskMutex.szName) - 1] = '\0';
		}
		else
		{
			TaskMutex.szName[0] = '\0';
		}

		//
		// Most recent mutex locks go to the front of the list.
		//
		pTaskID->m_MutexList.push_front (TaskMutex);
//	}

//	pthread_mutex_unlock (&pTaskID->m_LockMutex->mutex);
}


void stiOSTaskMutexRemove (
	stiTaskID pTaskID,
	SstiMutex *pstMutex)
{
	std::list<SstiTaskMutex>::iterator i;

	i = pTaskID->m_MutexList.begin ();

	if (i != pTaskID->m_MutexList.end ())
	{
		if (i->pMutex == pstMutex)
		{
			pTaskID->m_MutexList.erase (i);
		}
		else
		{
			for (; i != pTaskID->m_MutexList.end (); i++)
			{
				if (i->pMutex == pstMutex)
				{
					stiTrace ("Mutex unlocked out of order: ");

					if (!pstMutex->pszName || pstMutex->pszName[0] == '\0')
					{
						stiTrace ("%p\n", pstMutex);
					}
					else
					{
						stiTrace ("%s (%p)\n", pstMutex->pszName, pstMutex);
					}

					stiOSTaskMutexPrint (pTaskID);

					pTaskID->m_MutexList.erase (i);

					break;
				}
			}

			if (i == pTaskID->m_MutexList.end ())
			{
				stiTrace ("Mutex not found in list of %d mutexes: ", pTaskID->m_MutexList.size ());

				if (!pstMutex->pszName || pstMutex->pszName[0] == '\0')
				{
					stiTrace ("%p\n", pstMutex);
				}
				else
				{
					stiTrace ("%s (%p)\n", pstMutex->pszName, pstMutex);
				}

				stiOSTaskMutexPrint (pTaskID);
			}
		}
	}
	
//	pthread_mutex_unlock (&pTaskID->m_LockMutex->mutex);
}


void stiOSTaskMutexPrint (
	stiTaskID pTaskID)
{
	std::list<SstiTaskMutex>::iterator i;

//    pthread_once(&task_init_once, TaskInit);
//	pthread_mutex_lock (&pTaskID->m_LockMutex->mutex);

	stiTrace ("%s (%p) Mutexes: ", pTaskID->m_szTaskName, pTaskID->m_stPThread);

	if (!pTaskID->m_MutexList.empty ())
	{
		for (i = pTaskID->m_MutexList.begin (); i != pTaskID->m_MutexList.end (); i++)
		{
			if (i != pTaskID->m_MutexList.begin ())
			{
				stiTrace (", ");
			}

			if (i->szName[0] == '\0')
			{
				stiTrace ("%p", i->pMutex);
			}
			else
			{
				stiTrace ("%s (%p)", i->szName, i->pMutex);
			}
		}
	}

	stiTrace ("\n");

//	pthread_mutex_unlock (&pTaskID->m_LockMutex->mutex);
}
#endif

/* end file stiOSLinuxTask.c */
