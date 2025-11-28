/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSLinux
*
*  File Name:	stiOSLinux.c
*
*  Owner:		Scot L. Brooksby
*
*	Abstract:
*		This module contains the OS Abstraction / layering functions between the RTOS
*		OS Abstraction functions and the actual RTOS. For this module, this is
*		Linux version 5.4.
*
*******************************************************************************/

/*
 * Includes
 */

/* Cross-platform generic includes:*/
#include <cstdio>

/* Linux General include files:*/

/* Linux networking include files:*/

/* Component include files:*/
#include "stiOS.h"	/* public functions for External compatibility layer.*/
#include "stiOSLinux.h"	/* public functions for Linux compatibility layer.*/

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
/* Shows if we have been initialized:*/
static EstiBool stiOSLinuxInitialized = estiFALSE;
#ifndef stiUSE_MONOTONIC_CLOCK
	static struct timeval g_timeStart;
#else
	static timespec g_timeStart;
#endif

/*
 * Function Declarations
 */
static EstiResult stiOSLinuxInitDo ();

/*******************************************************************************
*; stiOSLinuxInit
*
* Description: (Global) From a higher level task to initialize anything that
* is required.
*
* Abstract:
*
* Returns:
*	EstiResult
*/
EstiResult stiOSLinuxInit ()
{
	EstiResult result = estiOK;

	if (! stiOSLinuxInitialized)
	{
		result = stiOSLinuxInitDo();
		if (result == estiOK) {
			stiOSLinuxInitialized = estiTRUE;
		}
	}

	return (result);
} /* end stiOSLinuxInit */

/*******************************************************************************
*; stiOSLinuxInitDo
*
* Description: Internal initialization prior to any user calls.
*
* Abstract:
*
* Returns:
*	uint32_t
*/
static EstiResult stiOSLinuxInitDo ()
{
	EstiResult eResult = estiERROR;

#ifndef stiUSE_MONOTONIC_CLOCK
	if (!gettimeofday (&g_timeStart, NULL))
	{
		eResult = estiOK;
	}

#else
	// Use ClockGetTime so that we have a monotonic clock
	clock_gettime (CLOCK_MONOTONIC, &g_timeStart);
	eResult = estiOK;
#endif

	return (eResult);
} /* end stiOSLinuxInitDo */

/*******************************************************************************
*; stiOSLinuxTickGet
*
* Description: (Global) OS Abstraction function to tickGet() in Linux.
*
* Abstract:
*
* Returns:
*	uint32_t
*/
uint32_t stiOSLinuxTickGet ()
{
	/*
	* This is static so we can return a reasonable value
	* if gettimeofday fails for some reason.
	*/
	static uint32_t un32Ticks;
	
#ifndef stiUSE_MONOTONIC_CLOCK
	// using gettimeofday ... which could change based on feedback from core for some systems.
	struct timeval timeNow;

	/* Get the current time */
	if (0 == gettimeofday (&timeNow, NULL))
	{
		/* Calculate the time since initialization */
		uint32_t un32Seconds = timeNow.tv_sec - g_timeStart.tv_sec;
		uint32_t un32Microseconds = 0;
		
		if (g_timeStart.tv_usec >= timeNow.tv_usec)
		{
			// A subtraction problem where we have to borrow one from the next larger value.
			un32Seconds -= 1;
			timeNow.tv_usec += USEC_PER_SEC;
		}
		un32Microseconds = timeNow.tv_usec - g_timeStart.tv_usec;

		/* Convert absolute time into ticks since initialization */
		un32Ticks = (un32Seconds * TICKS_PER_SEC) + (un32Microseconds / (USEC_PER_SEC / TICKS_PER_SEC));
	}
#else
	//
	// Get the time from the monotonic clock
	// since it isn't affected by the setting of the
	// wall clock (i.e. stime)
	//

	timespec TimeNow{};

	clock_gettime (CLOCK_MONOTONIC, &TimeNow);

	/* Calculate the time since initialization */
	uint32_t un32Seconds = TimeNow.tv_sec - g_timeStart.tv_sec;
	uint32_t un32NanoSeconds = 0;

	if (g_timeStart.tv_nsec >= TimeNow.tv_nsec)
	{
		// A subtraction problem where we have to borrow one from the next larger value.
		un32Seconds -= 1;
		TimeNow.tv_nsec += stiNSEC_PER_SEC;
	}
	un32NanoSeconds = TimeNow.tv_nsec - g_timeStart.tv_nsec;

	/* Convert absolute time into ticks since initialization */
	un32Ticks = (un32Seconds * TICKS_PER_SEC) + (un32NanoSeconds / (stiNSEC_PER_SEC / TICKS_PER_SEC));
#endif
	
	return (un32Ticks);
} /* end stiOSLinuxTickGet */

/*******************************************************************************
*; stiOSLinuxSysClkRateGet
*
* Description: (Global) OS Abstraction function to return scheduler ticks per second.
*
* Abstract:
*
* Returns:
*	int32_t
*/
int32_t stiOSLinuxSysClkRateGet ()
{
	// ACTION SMI ERC Willow - Need to return the real System Clock Rate 
	return (TICKS_PER_SEC);
} /* end stiOSLinuxSysClkRateGet */


int stiOSRead(int nFd, void *pcBuffer, size_t MaxBytes)
{
	int nResult = 0;

	do
	{
		//
		// If the read returns with an error (-1) due to being interrupted (errno == EINTR)
		// then attempt the read again.
		//
		nResult = read (nFd, pcBuffer, MaxBytes);

		if (nResult != -1 || errno != EINTR)
		{
			break;
		}
	}
	while (1);

	return nResult;
}

// This function is only needed for Windows
void stiOSMinimumTimerResolutionSet ()
{
}

// This function is only needed for Windows
void stiOSMinimumTimerResolutionReset ()
{
}

/* end file stiOSLinux.cpp */
