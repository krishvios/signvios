/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSLinux
*
*	File Name:		stiOSLinux.h
*
*	Owner:			Scot L. Brooksby
*
*	Abstract:
*		see stiOSLinux.c
*
*******************************************************************************/
#ifndef STIOSLINUX_H
#define STIOSLINUX_H

/*
 * Includes
 */
#include "stiDefs.h"		/* STI system-wide generic definitions.*/
#include <pthread.h>
#include <cerrno>
#include <cstdarg>
#include <sys/time.h>	/* for time*/


/*
 * Constants
 */
#define stiNO_WAIT 0
#define stiWAIT_FOREVER (-1)

/* Ticks per Second */
// ACTION SMI ERC Willow - Need to define the real System Clock Rate 
#define TICKS_PER_SEC 1000
#define stiOSSetEnv(A, B) setenv(A, B, 1)
/*
 * Typedefs
 */
typedef int (*stiFUNC_PTR)(size_t);
typedef void * (*stiPTHREAD_TASK_FUNC_PTR)(void *);

/*
 * Forward Declarations
 */

/*
 * Globals
 */
#define DEALLOCATE(pointer) \
	do { \
		if (pointer) {\
			free(pointer);\
			(pointer) = NULL;\
		}\
	} while (0)

/*
 * Function Declarations
 */
EstiResult stiOSLinuxInit ();

/* Get the current kernel tick value */
uint32_t stiOSLinuxTickGet ();

/* System clock functions:*/
/* (This is the only clock function used outside of BSP):*/
int32_t stiOSLinuxSysClkRateGet ();

int stiOSRead(int nFd, void *pcBuffer, size_t MaxBytes);

#endif /* STIOSLINUX_H */
/* end file stiOSLinux.h */
