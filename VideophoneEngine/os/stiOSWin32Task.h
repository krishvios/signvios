/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSTask
*
*	File Name:		stiOSWin32Task.h
*
*	Owner:			Scot L. Brooksby
*
*	Abstract:
*		Implements the OS task functions under Win32.
*
*******************************************************************************/
#ifndef STIOSWIN32TASK_H
#define STIOSWIN32TASK_H

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
#define stiOS_TASK_MIN_STACK_SIZE   (0x10000)
#define stiOS_TASK_DEFAULT_STACK_SIZE -1
#define stiOS_TASK_DEFAULT_PRIORITY -1

/*
 * Typedefs
 */

struct SstiMutex;

typedef struct SstiTaskID
{
	pthread_t m_stPThread;
	void * (*m_pTaskProc)(void *);
	void *m_pArg;
	char m_szTaskName[20];
	int m_nRefCount;
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

extern stiTaskID stiOSWin32TaskSpawn (
	const char* pszName,	/* task name */
	int nPriority,	/* task priority */
	int nStackSize,	/* desired stack size */
    stiPTHREAD_TASK_FUNC_PTR pEntry, /* pointer to the task entry function */
	void* pArg);		/* argument to the task entry function */

extern EstiResult stiOSWin32TaskDelete (
	stiTaskID pTaskID);

EstiResult stiOSWin32TaskDelay (
	int nTicks);

const char* stiOSWin32TaskName (
	stiTaskID pTaskID);

stiTaskID stiOSWin32TaskIDSelf ();

// Tasks that have been migrated to use CstiEventQueue
// instead of CstiOsTaskMQ, will need to manually register
// themselves with this function in order for stiERROR and
// others to identify the task they were called in.
// This will create an stiTaskID for the thread and store it
stiTaskID stiOSWin32TaskRegister (
	const std::string &taskName);

void stiOSWin32TaskUnregister (
       stiTaskID taskId);

void stiOSWin32TaskSetCurrentThreadName (
       const std::string &threadName);

void ThreadInfoStore (
	stiTaskID pTaskID);

#define stiOSTaskSpawn(name, pri, stacksize, entry, arg) \
		stiOSWin32TaskSpawn(name, pri, stacksize, entry, arg)

#define stiOSTaskDelete(tid) \
		stiOSWin32TaskDelete(tid)

#define stiOSTaskSuspend(tid)
//Win32 - Task suspend not defined

#define stiOSTaskResume(tid)
//Win32 - Task resume not defined

#define stiOSTaskDelay(ticks) \
		stiOSWin32TaskDelay(ticks)

#define stiOSTaskName(tid) \
		stiOSWin32TaskName(tid)

#define stiOSTaskIDSelf() stiOSWin32TaskIDSelf ()

#define stiOSTaskRegister(thread, name) \
        stiOSWin32TaskRegister(name)

#define stiOSTaskUnregister(task) \
        stiOSWin32TaskUnregister(task)

#define stiOSTaskSetCurrentThreadName(name) \
        stiOSWin32TaskSetCurrentThreadName(name)

#define stiOSTaskIDVerify(tid) estiERROR
//Win32 - stiOSTaskIDVerify not defined


void stiOSWin32TaskAddRef (
	SstiTaskID *pTaskID);

void stiOSWin32TaskRelease (
	SstiTaskID *pTaskID);


#endif /* STIOSWIN32TASK_H */
/* end file stiOSWin32Task.h */
