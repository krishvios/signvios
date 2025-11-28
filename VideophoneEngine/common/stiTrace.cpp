/*!
 * \file stiTrace.cpp
 * \brief Contains the definition of stiTrace, which is a function that can be
 * used to print information to stdio.
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2000-2011 by Sorenson Communications, Inc. All rights reserved.
 */

//
// Includes
//
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include "stiTrace.h"
#ifndef WIN32
	#include <sys/time.h>
#endif //WIN32
#include <mutex>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
static size_t* g_poRemoteLoggerTrace = nullptr;
static pRemoteTraceSend g_pmRemoteTraceSend = nullptr;

//
// Locals
//

//
// Function Declarations
//

#if APPLICATION == APP_NTOUCH_IOS ||  APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_DHV_IOS
void nsprintf(const char * msg);
#endif

// Set the printing from stiDEBUG_TOOL outside of that macro to allow us to use preprocessor
// commands based on APPLICATION. nPC reports errors if this is attempted inside the macro.
void stiPrintf(const char * fmt, va_list args)
{
#if APPLICATION == APP_NTOUCH_IOS  || APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_DHV_IOS
	static char msg[MAX_TRACE_MSG];
	static char msg2[MAX_TRACE_MSG];
	vsnprintf(msg2,MAX_TRACE_MSG, fmt, args);
	snprintf(msg,MAX_TRACE_MSG , "%s", msg2);
	nsprintf (msg);
#elif APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_MEDIASERVER
	static char msg[MAX_TRACE_MSG];
	std::string message = "";
	auto length = vsnprintf (msg, MAX_TRACE_MSG, fmt, args);
	if (length >= MAX_TRACE_MSG || length < 0)
	{
		length += 1;
		char *buffer = new char[length];
		vsnprintf (buffer, length, fmt, args);
		message = buffer;
		delete[] buffer;
	}
	else
	{
		message = msg;
	}
	DebugTools::instanceGet ()->DebugOutput (message);
#else
	// Print the formatted string to stdio for now and cause it to be written immediately.
	vprintf (fmt, args);
	fflush (stdout);
#endif
}


/*! \brief Log data to some output device
 *
 * This method is written for being able to log data to some output location.  
 * During debugging sessions, it is desired to be able to log information 
 * about the current state, what functions are being called, what events are 
 * occurring and about data itself.  This method allows all of them to call the 
 * same thing and let this function take care of where to send the data.
 * 
 * Note:  When using the udp trace facility, the character string being sent
 * has a limit of 1024 bytes.  Anything over this will cause a memory stomp!!!
 * \return None
 */
void stiTrace (
	const char * fmt,	 //!< Format specifier (like printf).
	...)				//!< Additional parameters (see ANSI function printf for more details).
{
	stiDEBUG_TOOL 
	(
		g_stiTraceDebug, 

		if (g_stiTraceDebug > 1)
		{
			timeval now {};
			gettimeofday (&now, nullptr);
			printf ("%d.%06d: ", (int)now.tv_sec, (int)now.tv_usec);
		}
		
		va_list args;	// A list of arguments

		// A macro to initialize the arguments
		va_start (args, fmt);
	
/*#ifdef stiDEBUG_OUTPUT //...
		static char msg[1024];
		static char msg2[1024];
		vsprintf(msg2, fmt, args);
		sprintf(msg, "VpE     %s", msg2);
		DebugTools::instanceGet()->DebugOutput(msg);
#else*/
	 stiPrintf(fmt, args);
//#endif*/
		
		// A macro which ends the use of variable arguments
		va_end (args);
		fflush(stdout);
	)

	if(g_poRemoteLoggerTrace != nullptr && g_pmRemoteTraceSend != nullptr)
	{
		stiDEBUG_TOOL
		(
			g_stiTraceDebug,

			va_list args;			// A list of arguments

			const int nMAX_BUF_LEN = 1024;

			char szBuf[nMAX_BUF_LEN];

			// A macro to initialize the arguments
			va_start (args, fmt);

			// Print the formatted string to a buffer then call the remote logger function
			(void) vsnprintf (szBuf, nMAX_BUF_LEN, fmt, args);

			//Send buffer to remote logging service
			(*g_pmRemoteTraceSend)(g_poRemoteLoggerTrace, szBuf);

			// Check for a memory stomp
			if (nMAX_BUF_LEN <= (int)strlen (szBuf))
			{
				// We've experienced a memory stomp.  Send a message to the remote logger host
				// system to alert of the error.
				(*g_pmRemoteTraceSend)(g_poRemoteLoggerTrace, "\n!!! A memory stomp has just occurred in stiTrace!!!\n\n");
			}  // end if

			// A macro which ends the use of variable arguments
			va_end (args);
		)
	}//end if RemoteLogger

} // end stiTrace


namespace vpe
{

void traceOne (std::ostream &output)
{
}

static std::mutex g_traceLock;

void traceLock ()
{
	g_traceLock.lock ();
}

void traceUnlock ()
{
	g_traceLock.unlock ();
}

}

/*
*
*
*
*/
void stiTraceRemoteLoggingSet (size_t* poRemoteLogger, pRemoteTraceSend pmRemoteTraceSend)
{
	g_poRemoteLoggerTrace = poRemoteLogger;
	g_pmRemoteTraceSend = pmRemoteTraceSend;
}

// end file stiTrace.cpp
