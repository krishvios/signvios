/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSMacOSX
*
*	File Name:		stiOSMacOSX.h
*
*	Owner:			Eric J. Herrmann
*
*	Abstract:
*		see stiOSMacOSX.c
*
*******************************************************************************/
#ifndef STIOSMACOSX_H
#define STIOSMACOSX_H

/*
 * Includes
 */
#include "stiDefs.h"		/* STI system-wide generic definitions.*/
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>

/*
 * Constants
 */
#define stiNO_WAIT 0
#define stiWAIT_FOREVER -1

/* uSeconds per Second */
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 1000

/* Ticks per Second */
// ACTION SMI ERC Willow - Need to define the real System Clock Rate 
#define TICKS_PER_SEC 1000

/*
 * Typedefs
 */
typedef int (*stiFUNC_PTR)(size_t);
typedef void (*stiVOID_FUNC_PTR)();

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
EstiResult stiOSMacOSXInit (void);

/* Get the current kernel tick value */
uint32_t stiOSMacOSXTickGet (void);

/* System clock functions:*/
/* (This is the only clock function used outside of BSP):*/
int32_t stiOSMacOSXSysClkRateGet (void);

#endif /* STIOSMACOSX_H */
/* end file stiOSMacOSX.h */
