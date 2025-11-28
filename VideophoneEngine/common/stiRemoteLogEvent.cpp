/*!
 * \file stiRemoteLogEvent.cpp
 * \brief Methods for sending events to the remote logging service
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include "stiRemoteLogEvent.h"
#include <cstdio>
#include <cstdarg>

//
// Constants
//

//
// Typedefs
//

//
// Globals
//
static size_t* g_poRemoteLoggerEvent = nullptr;
static pRemoteLogEventSend g_pmRemoteLogEventSend = nullptr;

//
// Locals
//

//
// Forward Declarations
//


/*!\brief Send the event to the remote logging service
 *
 */
void stiRemoteLogEventSend (
	const char *pFormat,	//!< Format specifier (like printf).
	...)					//!< Additional parameters (see ANSI function printf for more details).
{
	if (g_poRemoteLoggerEvent != nullptr && g_pmRemoteLogEventSend != nullptr)
	{
		va_list args;			// A list of arguments

		const int nMAX_BUF_LEN = 1024;
		char szBuffer[nMAX_BUF_LEN];

		// A macro to initialize the arguments
		va_start (args, pFormat);

		// Print the formatted string to a buffer then call the remote logger function
		int nLength = vsnprintf (szBuffer, sizeof(szBuffer), pFormat, args);

		//
		// Check to see if the output was truncated to the buffer size.
		// If it was then allocate a new buffer that is the length needed (returned in nLength)
		// and reformat the string.
		//
		if (nLength >= (signed)sizeof(szBuffer))
		{
			auto pszBuffer = new char[nLength + 1];

			if (pszBuffer)
			{
				(void)vsnprintf (pszBuffer, nLength + 1, pFormat, args);

				delete [] pszBuffer;
			}
		}

		// A macro which ends the use of variable arguments
		va_end (args);

		(*g_pmRemoteLogEventSend)(g_poRemoteLoggerEvent, szBuffer);
	}
}


void stiRemoteLogEventSend(
	std::string *pMessage)
{
	if (g_poRemoteLoggerEvent != nullptr && g_pmRemoteLogEventSend != nullptr)
	{
		(*g_pmRemoteLogEventSend)(g_poRemoteLoggerEvent, pMessage->c_str());
	}
}

/*!\brief Register the remote logging method and callback parameter
*
*/
void stiRemoteLogEventRemoteLoggingSet (
	size_t* poRemoteLogger,
	pRemoteLogEventSend pmRemoteLogEventSend)
{
	g_poRemoteLoggerEvent = poRemoteLogger;
	g_pmRemoteLogEventSend = pmRemoteLogEventSend;
}
