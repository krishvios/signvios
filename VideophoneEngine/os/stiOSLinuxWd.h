/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSLinuxWd
*
*	File Name:		stiOSLinuxWd.h
*
*	Owner:			Eugene R. Christensen
*
*	Abstract:
*		This module provides the linkage to the Linux OS implementation
*		of WatchDog Timers.
*
*******************************************************************************/
#ifndef STIOSLINUXWD_H
#define STIOSLINUXWD_H

/*
 * Includes
 */
/*SLB #include <wdLib.h>*/

/*
 * Constants
 */

/*
 * Typedefs
 */
struct stiWDOG_ID_s;  /* private implementation */
typedef struct stiWDOG_ID_s *stiWDOG_ID;

#define stiINVALID_WDOG_TIMER	NULL

typedef void (*stiTIMER_CALLBACK)(stiWDOG_ID, intptr_t, intptr_t);

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */
stiHResult stiOSWdInitialize ();

void stiOSWdUninitialize ();

stiHResult stiOSWdCancel (
	stiWDOG_ID wdId);

stiWDOG_ID stiOSWdCreate ();

stiHResult stiOSWdDelete (
	stiWDOG_ID wdId);

stiHResult stiOSWdStart (
	stiWDOG_ID wdId,
	int nDelay,					// Time to wait in milliseconds
	stiFUNC_PTR pTimerCallback,
	intptr_t Parameter);

stiHResult stiOSWdStart (
	stiWDOG_ID wdId,
	int nDelay,					// Time to wait in milliseconds
	stiFUNC_PTR pTimerCallback,
	stiFUNC_PTR pCancelCallback,
	intptr_t Parameter);

stiHResult stiOSWdStart2 (
	stiWDOG_ID wdId,
	int nDelay,					// Time to wait in milliseconds
	stiTIMER_CALLBACK pRoutine,
	intptr_t Parameter1,
	intptr_t Parameter2);


stiHResult stiOSWdStart2 (
	stiWDOG_ID wdId,
	int nDelay,					// Time to wait in milliseconds
	stiTIMER_CALLBACK pTimerCallback,
	stiTIMER_CALLBACK pCancelCallback,
	intptr_t Parameter1,
	intptr_t Parameter2);

#endif /* STIOSLINUXWD_H */
/* end file stiOSLinuxWd.h */
