// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STIOSWIN32_H
#define STIOSWIN32_H

#include <process.h>
#include <stiDefs.h>

/*
 * Constants
 */
#define stiNO_WAIT 0
#define stiWAIT_FOREVER -1

#define TICKS_PER_SEC 1000

int winsetenv(const char *pVar, const char *pVal);
#define _setenv(A, B) winsetenv(A, B)

/*
 * Typedefs
 */
typedef int (*stiFUNC_PTR)(size_t);
typedef void * (*stiPTHREAD_TASK_FUNC_PTR)(void *);

/*
 * Globals
 */

#ifndef strncasecmp
#define strncasecmp strnicmp
#endif
#define getpid _getpid

#ifndef uint8_t
typedef unsigned char uint8_t;
#endif

#define DEALLOCATE(pointer) \
    do { \
        if (pointer) {\
            free(pointer);\
            (pointer) = NULL;\
        }\
    } while (0)


int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifndef timersub
#define timersub(a, b, result)							\
do {													\
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;	\
	if ((result)->tv_usec < 0)							\
	{													\
		-- (result)->tv_sec;							\
		(result)->tv_usec += 1000000;					\
	}													\
} while (0)
#endif 

int usleep (unsigned long ulMicroSeconds);

#ifndef strcasecmp
#define strcasecmp(a,b) _stricmp(a,b)
#endif

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
EstiResult stiOSWin32Init (void);

/*******************************************************************************
*; stiOSWin32TickGet
*
* Description: (Global) OS Abstraction function to tickGet() in WIN32.
*
* Abstract:
*
* Returns:
*	uint32_t
*/
uint32_t stiOSWin32TickGet (void);

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
int32_t stiOSWin32SysClkRateGet (void);

int win_vsnprintf(char* str, size_t size, const char* format, va_list ap);

#endif // STIOSWIN32_H
