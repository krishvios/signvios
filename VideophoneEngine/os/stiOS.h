/*!
 * \file stiOS.h
 *
 * \brief See stiOS.c
 * 
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIOS_H
#define STIOS_H

#if defined stiLINUX
	#define stricmp strcasecmp
#elif defined WIN32
#ifndef _WIN32_WINNT
	/* _WIN32_WINNT specifies that 
	Certain functions that depend on a particular version of Windows are declared 
	using conditional code. This enables you to use the compiler to detect whether 
	your application uses functions that are not supported on its target version(s) 
	of Windows. To compile an application that uses these functions, you must define 
	the appropriate macros. Otherwise, you will receive the C2065 error message.
	see: http://msdn.microsoft.com/en-us/library/aa383745(v=vs.85).aspx 
	*/
#if APPLICATION == APP_NTOUCH_PC
	#define _WIN32_WINNT NTDDI_WIN2K	// 0x05000000  Windows 2000
#elif APPLICATION == APP_HOLDSERVER || APPLICATION == APP_MEDIASERVER
	#define _WIN32_WINNT NTDDI_WS03		// 0x05020000  Windows 2003 Server
#else
	//#define _WIN32_WINNT NTDDI_WINXP	// 0x05010000  Windows XP
	//#define _WIN32_WINNT NTDDI_VISTA	// 0x06000000  Windows Vista
	#define _WIN32_WINNT NTDDI_WIN7	// 0x06010000  Windows 7
#endif

#endif
	#include <winsock2.h>
	#include <windows.h>

#if _MSC_VER < 1900 
	// Standard functions and types that are slightly different
	#define caddr_t _in_bcount(len) const char FAR *
	#define snprintf _snprintf
	#define strcmpi _strcmpi
#endif

#if APPLICATION != APP_NTOUCH_PC && \
	APPLICATION != APP_HOLDSERVER && \
	APPLICATION != APP_MEDIASERVER
	#define strncpy(dst,src,len) strncpy_s(dst,len,src,_TRUNCATE)
#endif

	// Manage warnings:
	//#define _CRT_SECURE_NO_WARNINGS	// Disable all warnings (bad idea!)
	#pragma warning(disable:4200)		// Declaring an array of zero elements
#endif // WIN32


/*
 * Includes
 */
#include "stiDefs.h"		/* STI system-wide generic definitions.*/
#include "stiOSMsg.h"
#include "stiOSMutex.h"
#include "stiOSNet.h"
#include "stiOSTask.h"
#include "stiOSWd.h"
#if defined (stiLINUX)
	#include "stiOSLinux.h"

#elif defined (WIN32)
	#include "stiOSWin32.h"

#else
	/* Unknown platform */
	#error *** Error *** Unknown Operating System
#endif
#include <string>

#ifdef stiLINUX
#include "stiOSSignal.h"
#endif

/*
 * Constants
 */
#ifndef USEC_PER_SEC // Microseconds per second
#define USEC_PER_SEC 1000000
#endif

#ifndef NSEC_PER_USEC // Nanoseconds per microsecond
#define NSEC_PER_USEC 1000
#endif

#ifndef USEC_PER_MSEC // Microseconds per millisecond
#define USEC_PER_MSEC 1000
#endif

#ifndef MSEC_PER_SEC // Milliseconds per second
#define MSEC_PER_SEC 1000
#endif

#ifndef NSEC_PER_MSEC // Nanoseconds per Milliseconds
#define NSEC_PER_MSEC 1000000
#endif

#ifndef stiNSEC_PER_SEC
#define stiNSEC_PER_SEC 1000000000
#endif

#define nMAX_UNIQUE_ID_LENGTH	18

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4 || APPLICATION == APP_REGISTRAR_LOAD_TEST_TOOL
#define stiMAX_FILE_PATH_LENGTH	256
#endif

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


/* Component Initialization:*/
EstiResult stiOSInit ();

/* Get the current kernel tick value:*/
uint32_t stiOSTickGet ();

/* System clock functions:*/
/* (This is the only clock function used outside of BSP):*/
int32_t stiOSSysClkRateGet ();

void stiOSDynamicDataFolderGet (std::string *folder);

void stiOSDynamicDataFilePathGet (const char *fileName, char *path);
void stiOSDynamicDataFolderSet (const std::string &dynamicDataFolder);
void stiOSStaticDataFolderGet (std::string *folder);
void stiOSStaticDataFolderSet (const std::string &staticDataFolder);
void stiOSMinimumTimerResolutionSet ();
void stiOSMinimumTimerResolutionReset ();

#endif /* STIOS_H */
/* end file stiOS.h */
