/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSTask
*
*	File Name:		stiOSLinuxTask.h
*
*	Owner:			Scot L. Brooksby
*
*	Abstract:
*		Implements the OS task functions under Linux.
*
*******************************************************************************/
#ifndef STIOSLINUXTASK_H
#define STIOSLINUXTASK_H

/*
 * Includes
 */

#include <pthread.h>
#include <string>
#include <list>

/*
 * Constants
 */

#define stiOS_TASK_INVALID_ID ((PSstiTaskID)(-1))

/* For enforcing a minimum stack size */
#if DEVICE == DEV_NTOUCH_VP4
#define stiOS_TASK_MIN_STACK_SIZE   (PTHREAD_STACK_MIN)
#else
#define stiOS_TASK_MIN_STACK_SIZE   (0x10000)
#endif
#define stiOS_TASK_DEFAULT_STACK_SIZE (-1)
#define stiOS_TASK_DEFAULT_PRIORITY (-1)

/*
 * Typedefs
 */

struct SstiMutex;

#ifdef stiMUTEX_DEBUGGING
typedef struct SstiTaskMutex
{
	SstiMutex *pMutex;
	char szName[64];
} SstiTaskMutex;
#endif

typedef struct SstiTaskID
{
	SstiTaskID ()
	{
		m_szTaskName[0] = '\0';
	}

	pthread_t m_stPThread{};
	void * (*m_pTaskProc)(void *){nullptr};
	void * m_pArg{nullptr};
	char   m_szTaskName[20]{};
#ifdef stiMUTEX_DEBUGGING
	std::list <SstiTaskMutex> m_MutexList;
#endif
//	stiMUTEX_ID m_LockMutex;
	int m_nRefCount{0};
	int m_nTID{0};
	int m_nTaskTime{0};
} SstiTaskID, *PSstiTaskID;

#define stiTaskID PSstiTaskID

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */
extern stiTaskID stiOSLinuxTaskSpawn (
	const char* pszName,	/* task name */
	int nPriority,	/* task priority */
	int nStackSize,	/* desired stack size */
	stiPTHREAD_TASK_FUNC_PTR pEntry, /* pointer to the task entry function */
	void* pArg);		/* argument to the task entry function */

extern EstiResult stiOSLinuxTaskDelete (
	stiTaskID pTaskID);

EstiResult stiOSLinuxTaskDelay (
	int nTicks);

const char* stiOSLinuxTaskName (
	stiTaskID pTaskID);

stiTaskID stiOSLinuxTaskIDSelf ();

void stiOSLinuxTaskTimeReport ();

void ThreadInfoStore (
	stiTaskID pTaskID);

#ifdef stiMUTEX_DEBUGGING
void stiOSTaskMutexAdd (
	stiTaskID pTaskID,
	SstiMutex *pstMutex);

void stiOSTaskMutexRemove (
	stiTaskID pTaskID,
	SstiMutex *pstMutex);

void stiOSTaskMutexPrint (
	stiTaskID pTaskID);
#endif
	
#define stiOSTaskSpawn(name, pri, stacksize, entry, arg) \
		stiOSLinuxTaskSpawn(name, pri, stacksize, entry, arg)

#define stiOSTaskDelete(tid) \
		stiOSLinuxTaskDelete(tid)

#define stiOSTaskSuspend(tid)
//linux - Task suspend not defined

#define stiOSTaskResume(tid)
//linux - Task resume not defined

#define stiOSTaskDelay(ticks) \
		stiOSLinuxTaskDelay(ticks)

#define stiOSTaskName(tid) \
		stiOSLinuxTaskName(tid)

#define stiOSTaskIDSelf() stiOSLinuxTaskIDSelf ()

#define stiOSTaskRegister(thread, name) \
        stiOSLinuxTaskRegister(thread, name)

#define stiOSTaskUnregister(task) \
        stiOSLinuxTaskUnregister(task)

#define stiOSTaskSetCurrentThreadName(name) \
        stiOSLinuxTaskSetCurrentThreadName(name)


#define stiOSTaskIDVerify(tid) estiERROR
//linux - stiOSTaskIDVerify not defined


void stiOSLinuxTaskAddRef (
	SstiTaskID *pTaskID);

void stiOSLinuxTaskRelease (
	SstiTaskID *pTaskID);

// Tasks that have been migrated to use CstiEventQueue
// instead of CstiOsTaskMQ, will need to manually register
// themselves with this function in order for stiERROR and
// others to identify the task they were called in.
// This will create an stiTaskID for the thread and store
// it in the g_apsThreadInfo array.
stiTaskID stiOSLinuxTaskRegister (
	pthread_t pthreadId,
	const std::string &taskName);

void stiOSLinuxTaskUnregister (
	stiTaskID taskId);

void stiOSLinuxTaskSetCurrentThreadName (
	const std::string &threadName);

int stiOSLinuxTaskThreadIdGet ();

#endif /* STIOSLINUXTASK_H */
/* end file stiOSLinuxTask.h */
