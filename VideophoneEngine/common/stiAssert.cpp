/*!
 * \file stiAssert.cpp
 * \brief STI custom assert function
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//
#ifndef UNDER_CE
	#include <cstdio>
#endif // !UNDER_CE

#if defined (stiHOST) && defined (stiWIN32)
	#include <windows.h>
	#include <winuser.h>
	#include "stiTChar.h"
#endif // defined (stiHOST) && defined (stiWIN32)

#ifdef stiOS
	#include "stiOS.h"
#endif

#include "stiAssert.h"
#include "stiDefs.h"
#include "stiTrace.h"
#include <sstream>

//
// Constants
//

//
// Typedefs
//

//
// Globals
//
static size_t* g_poRemoteLoggerAssert = nullptr;
static pRemoteAssertSend g_pmRemoteAssertSend = nullptr;

//
// Locals
//

//
// Forward Declarations
//

/*! \brief This assert method is called from the stiASSERT macro.
 *
 * This function is called from the stiASSERT macro after an assertion fails.
 *
 * \return 1, it can't return void because stiASSERT macro will complain about 
 * right hand of "||" having a void type.
 */
int stiAssert (
	const void *pExp,       //!< Expression that was tested.
	const void *pFileName,  //!< Name of file where assertion failed.
	int nLineNumber)        //!< Line number where assertion failed.
{
	stiLOG_ENTRY_NAME (stiAssert);

#if defined (stiWIN32)
#if 0
	TCHAR szMsg[128];

	// Format the message
	wsprintf (szMsg, _TEXT("Assertion failed: %s, file %s, line %d\n\n") 
		_TEXT("Press OK to ignore."), pExp, pFileName, nLineNumber);

	// Display the assertion message
	if (IDCANCEL == MessageBox (NULL, szMsg, _TEXT("Assertion failed"), 
		MB_OKCANCEL | MB_ICONERROR))
	{
		// Exit with an exit code of 3, which is the same as abort()
		DebugBreak ();
	} // end if (IDCANCEL == MessageBox ())
	// else just keep running
#endif
	return 1;
#endif

#ifdef stiOS
	stiTaskID tidThisTask;
	const char * pTaskName;
	char szMsg[300];
	char szErr[] = "ERROR - Invalid Task ID";

	tidThisTask = stiOSTaskIDSelf ();
	pTaskName = stiOSTaskName (tidThisTask);
	if (nullptr == pTaskName)
	{
		pTaskName = szErr;
	} // end if

	// Put the assertion string together. The \x07's are 'bell' commands for beeps
	snprintf (szMsg, sizeof(szMsg),
			 "\n\n"
			 "\x07\t**** ASSERTION FAILED ****\n"
			 "\x07\t\tAssertion: %s\n"
			 "\x07\t\tFile: %s\n"
			 "\x07\t\tLine: %d\n\n\n",
			 (char*)pExp,
			 (char*)pFileName,
			 nLineNumber);

	// Print the string to the console
	printf ("%s", szMsg);

	if(g_poRemoteLoggerAssert != nullptr && g_pmRemoteAssertSend != nullptr)
	{
		szMsg[0] = '\0';//Clear the Assert buffer
		std::stringstream fileAndLine;

		// Put the assertion string together. The \x07's are 'bell' commands for beeps
		snprintf (szMsg, sizeof(szMsg),
				 "\nAssertion = \'%s\'\n"
				 "File = %s\n"
				 "Line = %d\n",
				 (char*)pExp,
				 (char*)pFileName,
				 nLineNumber);
		
		fileAndLine << static_cast<const char*>(pFileName) << nLineNumber;

		//Send buffer to remote logging service
		(*g_pmRemoteAssertSend)(g_poRemoteLoggerAssert, szMsg, fileAndLine.str ());
	}
	return 1;
#endif  // stiOS

} // end stiAssert


/*! \brief This assert method is called from the stiASSERTMSG macro.
 *
 * This function is called from the stiASSERTMSG macro after an assertion fails.
 *
 * \return 1, it can't return void because stiASSERT macro will complain about
 * right hand of "||" having a void type.
 */
int stiAssertMsg (
	const void *pExp,       //!< Expression that was tested.
	const void *pFileName,  //!< Name of file where assertion failed.
	int nLineNumber,        //!< Line number where assertion failed.
	const char *pszFormat,
	...)
{
	stiLOG_ENTRY_NAME (stiAssertMsg);

#if defined (stiWIN32)
	return 1;
#endif

#ifdef stiOS
	stiTaskID tidThisTask;
	const char * pTaskName;
	std::stringstream message;
	std::stringstream fileAndLine;
	char szErr[] = "ERROR - Invalid Task ID";
	const int nMAX_BUF_LEN = 1024;
	char szMessage[nMAX_BUF_LEN];
	char *pszTmpBuffer = nullptr;
	char *pszMessage = szMessage;

	tidThisTask = stiOSTaskIDSelf ();
	pTaskName = stiOSTaskName (tidThisTask);
	if (nullptr == pTaskName)
	{
		pTaskName = szErr;
	} // end if

	//
	// Format the message
	//
	va_list args;	// A list of arguments

	// A macro to initialize the arguments
	va_start (args, pszFormat);

	// Print the formatted string to a buffer then call the remote logger function
	int nLength = vsnprintf (szMessage, sizeof(szMessage), pszFormat, args);

	//
	// Check to see if the output was truncated to the buffer size.
	// If it was then allocate a new buffer that is the length needed (returned in nLength)
	// and reformat the string.
	//
	if (nLength >= (signed)sizeof(szMessage))
	{
		//
		// The buffer is not long enough for the message so allocate
		// a new buffer that is long enough and format the message into it.
		//
		pszTmpBuffer = new char[nLength + 1];

		if (pszTmpBuffer)
		{
			(void)vsnprintf (pszTmpBuffer, nLength + 1, pszFormat, args);
			pszMessage = pszTmpBuffer;
		}
	}

	// A macro which ends the use of variable arguments
	va_end (args);
	
	// Put the assertion string together.
	message <<
	"\nAssertion = \'" << (char*)pExp << "\'\n"
	"File = " << (char*)pFileName <<"\n"
	"Line = " << nLineNumber << "\n"
	"Message = \"" << pszMessage << "\"\n";
	
	fileAndLine << static_cast<const char*>(pFileName) << nLineNumber;

	// Print the string to the console
	printf ("%s", message.str().c_str());

	if(g_poRemoteLoggerAssert != nullptr && g_pmRemoteAssertSend != nullptr)
	{
		//Send buffer to remote logging service
		(*g_pmRemoteAssertSend)(g_poRemoteLoggerAssert, message.str(), fileAndLine.str ());
	}

	if (pszTmpBuffer)
	{
		delete [] pszTmpBuffer;
		pszTmpBuffer = nullptr;
	}

	return 1;
#endif  // stiOS

} // end stiAssertMsg


/*
*
*
*
*/
void stiAssertRemoteLoggingSet (size_t* poRemoteLogger, pRemoteAssertSend pmRemoteAssertSend)
{
	g_poRemoteLoggerAssert = poRemoteLogger;
	g_pmRemoteAssertSend = pmRemoteAssertSend;
}
// end stiAssert.c
