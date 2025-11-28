/*!
 * \file stiOSTask.h
 *
 * This module contains the OS Abstraction / layering functions between the
 * application and the underlying RTOS for the Task Management sub-functions.
 * 
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef STIOSTASK_H
#define STIOSTASK_H

/*
 * Includes
 */

/* Include the appropriate OS specific header files */
#if defined (stiLINUX)
	#include "stiOSLinux.h"
	#include "stiOSLinuxTask.h"
#elif defined (WIN32)
	#include "stiOSWin32.h"
	#include "stiOSWin32Task.h"
#else
	/* Unknown platform */
	#error *** Error *** Unknown Operating System
#endif

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
 * Function Declarations
 */

#if 0  /* This section is here for documentation purposes. Function prototypes 
		* for automated documentation are listed first, followed by "fake" 
		* function definitions for documentation purposes. Actual prototypes 
		* and implementations are done in the OS-specific files.
		*/
stiTaskID stiOSTaskSpawn (
	char *	pszName,
	int		nPriority,
	int		nStackSize,
	void *	pEntry,
	void *	pArg);

EstiResult stiOSTaskDelete (
	stiTaskID tid);

void stiOSTaskSuspend (
	stiTaskID tid);

void stiOSTaskResume (
	stiTaskID tid);

void stiOSTaskDelay (
	int nTicks);

char *stiOSTaskName (
	stiTaskID tid);

stiTaskID stiOSTaskIDSelf (void);

EstiResult stiOSTaskIDVerify (
	stiTaskID tid);

/*! \brief Spawn a task.
 *
 * \return A valid stiTaskID, or stiOS_TASK_INVALID_ID on error.
 */
stiTaskID stiOSTaskSpawn (
	char *	pszName,	/*!< The task name. */
	int		nPriority,	/*!< The task priority. */
	int		nStackSize,	/*!< The desired stack size. */
	void *	pEntry,		/*!< A pointer to the task entry function. */
	void *	pArg)		/*!< An argument to the task entry function. */
{
}

/*! \brief Delete a task.
 *
 * The ability to delete oneself varies by OS. Use of this function should be
 * avoided where possible.
 *
 * \return None.
 */
EstiResult stiOSTaskDelete (
	stiTaskID tid)		/*!< The ID of the task to be deleted. */
{
}

/*! \brief Suspend a task.
 *
 * \return None.
 */
void stiOSTaskSuspend (
	stiTaskID tid)		/*!< The ID of the task to suspend. */
{
}

/*! \brief Resume a suspended task.
 *
 * \return None.
 */
void stiOSTaskResume (
	stiTaskID tid)		/*!< The ID of the task to resume. */
{
}

/*! \brief Causes the task to sleep for the given number of clock ticks.
 *
 * \return None.
 */
void stiOSTaskDelay (
	int nTicks)			/*!< The number of ticks to sleep. */
{
}

/*! \brief Get the task name.
 *
 * return A pointer to the task name.
 */
char *stiOSTaskName (
	stiTaskID tid)		/*!< The ID of the task to get the name of. */
{
}

/*! \brief Get the ID of the calling task.
 *
 * \return The task ID of the calling task.
 */
stiTaskID stiOSTaskIDSelf (void)
{
}

/*! \brief Verifies a task ID.
 *
 * Checks to see if any task has the given ID.
 *
 * \retval estiOK If a task is found with the given ID.
 * \retval estiERROR Otherwise.
 */
EstiResult stiOSTaskIDVerify (
	stiTaskID tid)		/*!< The task ID to verify. */
{
}

#endif /* if 0 */

#endif /* STIOSTASK_H */
/* end file stiOSTask.h */
