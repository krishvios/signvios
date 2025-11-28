/*!
 * \file stiTrace.h
 * \brief See stiTrace.cpp.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

//
// Includes
//
#include "stiOS.h"   /* For varargs support */
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
#include <android/log.h>
#endif
#include "stiDebugTools.h"
#include <iostream>
#include <iomanip>

//
// Constants
//

//
// Typedefs
//
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_MEDIASERVER || APPLICATION == APP_DHV_IOS
	#define MAX_TRACE_MSG 10000
#endif

#if (APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID) && !defined(SORENSON_ANDROID_LOG)
	#define SORENSON_ANDROID_LOG

	// Android doesn't use print statements.  In order
	// setup some helper logging statements.
	#define TAG "VideophoneEngine"

	#define aLOGL(level,msg,...) __android_log_print(level, TAG,"(%s:%d): " msg, __FILE__, __LINE__, ## __VA_ARGS__);

	#define aLOGD(msg,...) aLOGL(ANDROID_LOG_DEBUG,msg, ## __VA_ARGS__);
	#define aLOGI(msg,...) aLOGL(ANDROID_LOG_INFO,msg, ## __VA_ARGS__);
	#define aLOGW(msg,...) aLOGL(ANDROID_LOG_WARN,msg, ## __VA_ARGS__);
	#define aLOGE(msg,...) aLOGL(ANDROID_LOG_ERROR,msg, ## __VA_ARGS__);
	#define aLOGS(msg,...) aLOGL(ANDROID_LOG_DEBUG,#msg, ## __VA_ARGS__);

	// Also override print in order to take advantage
	// of videophone trace abilities.
	#ifdef vprintf
	#undef vprintf
	#endif
	#define vprintf(msg, args) __android_log_vprint(ANDROID_LOG_DEBUG, TAG, msg, args);
#endif

#define stiLOG_ENTRY_NAME(A) /* ERC - Collapse to nothing for now. */
//#define stiLOG_ENTRY_NAME(name) stiTrace ("ENTER %s, Thread = 0x%08x, Task = %s\n", #name, stiOSTaskIDSelf (), stiOSTaskName (stiOSTaskIDSelf ()) ? stiOSTaskName (stiOSTaskIDSelf ()) : "Unknown");
//#define stiLOG_ENTRY_NAME(name, ...) aLOGS(name, ## __VA_ARGS__);

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//


typedef void (*pRemoteTraceSend)(size_t *, const char *);

void stiTraceRemoteLoggingSet(size_t* poRemoteLogger, pRemoteTraceSend pmRemoteTraceSend);

void stiTrace (IN const char * fmt, ...);


namespace vpe
{
void traceOne (std::ostream &output);

template <class Arg, class ...Args>
void traceOne (std::ostream &output, const Arg &arg, const Args &...args)
{
	output << std::boolalpha << arg;
	traceOne (output, args...);
}

template <class ...Args>
void trace (std::ostream &output, const Args &...args)
{
	traceOne(output, args...);
}

void traceLock ();
void traceUnlock ();

template <class ...Args>
void trace (const Args &...args)
{
	traceLock ();
	trace (std::cout, args...);
	traceUnlock ();
}

}

// end file stiTrace.h
