/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*  Module Name:	stiOSWin32
*
*  File Name:	stiOSWin32.c
*
*	Abstract:
*		This module contains the OS Abstraction / layering functions between the RTOS
*		OS Abstraction functions and the actual RTOS. For this module, this is
*		WIN32.
*
*******************************************************************************/

/*
 * Includes
 */
 /* Component include files:*/
#include "stiOS.h"	/* public functions for External compatibility layer.*/
#include "stiOSWin32.h"	/* public functions for WIN32 compatibility layer.*/

/* Cross-platform generic includes:*/
#include <stdio.h>
#include <time.h>
#include <timeapi.h>



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
static EstiBool stiOSWin32Initialized = estiFALSE;
static LARGE_INTEGER g_appTimerStart = { 0 };
static LARGE_INTEGER g_timerFrequency = { 0 };
static int g_minTimerResolution{ 0 };

/*******************************************************************************
*; stiOSWin32Init
*
* Description: (Global) From a higher level task to initialize anything that
* is required.
*
* Abstract:
*
* Returns:
*	EstiResult
*/
EstiResult stiOSWin32Init (void)
{
	EstiResult result = estiOK;

	if (!stiOSWin32Initialized)
	{
		// Retrieves the frequency of the performance counter. 
		// The frequency of the performance counter is fixed at system boot and is consistent across all processors. 
		// Therefore, the frequency need only be queried upon application initialization, and the result can be cached. (MSDN)
		QueryPerformanceFrequency(&g_timerFrequency);
		// We are grabbing the current performance counter time to use as an application startup base time.
		QueryPerformanceCounter(&g_appTimerStart);
		stiOSWin32Initialized = estiTRUE;
	}

	return (result);
} /* end stiOSWin32Init */

/*******************************************************************************
*; stiOSWin32TickGet
*
* Description: (Global) OS Abstraction function to tickGet() in WIN32.
*
* Abstract: Returns the application start time in milliseconds.
*
* Returns:
*	uint32_t
*/
uint32_t stiOSWin32TickGet (void)
{
	LARGE_INTEGER currentTime = { 0 };
	//According to MSDN QueryPerformanceCounter and QueryPerformanceFrequency
	//On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
	QueryPerformanceCounter(&currentTime);
	//We are subtracting out the application's initial start time.
	currentTime.QuadPart -= g_appTimerStart.QuadPart;
	//We are converting to milliseconds
	currentTime.QuadPart = (currentTime.QuadPart * USEC_PER_MSEC) / g_timerFrequency.QuadPart;
	//LARGE_INTEGER is a union structure with QuadPart being 64 bit and HighPart and LowPart being 32 bit.
	//We are returning just the LowPart
	return currentTime.LowPart;
} /* end stiOSWin32TickGet */

/*******************************************************************************
*; stiOSWin32SysClkRateGet
*
* Description: (Global) OS Abstraction function to return scheduler ticks per second.
*
* Abstract:
*
* Returns:
*	int32_t
*/
int32_t stiOSWin32SysClkRateGet (void)
{
#if 0
	// ACTION SMI ERC Willow - Need to return the real System Clock Rate 
	return (TICKS_PER_SEC);
#else
	return CLOCKS_PER_SEC;
#endif
} /* end stiOSWin32SysClkRateGet */

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	// Define a structure to receive the current Windows filetime
	FILETIME ft;

	// Initialize the present time to 0 and the timezone to UTC
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		// The GetSystemTimeAsFileTime returns the number of 100 nanosecond 
		// intervals since Jan 1, 1601 in a structure. Copy the high bits to 
		// the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		// Convert to microseconds by dividing by 10
		tmpres /= 10;

		// The Unix epoch starts on Jan 1 1970.  Need to subtract the difference 
		// in seconds from Jan 1 1601.
		tmpres -= DELTA_EPOCH_IN_MICROSECS;

		// Finally change microseconds to seconds and place in the seconds value. 
		// The modulus picks up the microseconds.
		tv->tv_sec = (long)(tmpres / USEC_PER_SEC);
		tv->tv_usec = (long)(tmpres % USEC_PER_SEC);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}

int usleep (unsigned long ulMicroSeconds)
{
	Sleep(ulMicroSeconds/1000);
	return 0;
}

int winsetenv(
	const char *pVar, 
	const char *pVal)
{
 	char *string = new char[ (strlen(pVar) + strlen(pVal) + 2) ];
	strcpy(string, pVar);
	strcat(string,"=");
	strcat(string, pVal);
	int ret = _putenv(string);
	delete[] string;
	return ret;
}

int win_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
	int count = -1;

	if (size != 0)
	{
		count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
	}
	if (count == -1)
	{
		count = _vscprintf(format, ap);
	}

	return count;
}

void stiOSMinimumTimerResolutionSet ()
{
	if (g_minTimerResolution < 1)
	{
		TIMECAPS timeCaps{};
		auto result = timeGetDevCaps (&timeCaps, sizeof (TIMECAPS));
		if (S_OK == result)
		{
			g_minTimerResolution = timeCaps.wPeriodMin;
		}
		else
		{
			stiASSERTMSG (false, "Failed to get minimum timer resolution: %u", result);
		}
	}

	if (g_minTimerResolution > 0)
	{
		timeBeginPeriod (g_minTimerResolution);
	}
}

void stiOSMinimumTimerResolutionReset ()
{
	if (g_minTimerResolution > 0)
	{
		timeEndPeriod (g_minTimerResolution);
	}
}

/* end file stiOSWin32.c */
