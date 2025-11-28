/*!
 * \file stiError.h
 * \brief See stiError.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIERROR_H
#define STIERROR_H

/*
 * Includes
 */
#include "stiDefs.h"

#ifdef stiOS
#include "stiOS.h"	// OS Abstraction Layer.
#include "stiOSTask.h"	// OS Abstraction Layer.
#endif /* stiOS */


/*
 * Constants
 */
#define nMAX_FILE_NAME 128	/* Max filename length stored in the error log */
#define nMAX_TASK_NAME 20   /* Max taskname length stored in the error log */

/*
 * Typedefs
 */
/*! \brief This enumeration is used to log how critical the error is that has
 * occurred.
 */
typedef enum EstiErrorCriticality
{
	estiLOG_ERROR,
	 //!< This is error situation (e.g. couldn't allocate memory).
	estiLOG_WARNING,    /*!< This is simply a warning message (e.g. Password is incorrect
						 * and therefore the user can't be logged into the system.
						 */
	estiLOG_HRESULT
} EstiErrorCriticality;

/*! \brief This structure is used to store all the gathered information about
 * an error that has occurred.
 */
typedef struct SstiErrorLog
{
	EstiErrorCriticality eCriticality;	/*!< Error or warning. */
	uint32_t un32Counter;				/*!< System counter value. */
	uint32_t un32TickCount;			/*!< Current system tick Count. */
	stiTaskID tidTaskId;				/*!< Task ID that logged the error. */
	char szTaskName[nMAX_TASK_NAME];	/*!< Task Name that logged the error. */
	char szFile[nMAX_FILE_NAME];		/*!< File where in the error was logged. */
	int nLineNbr;						/*!< Line number within the file. */
	uint32_t un32StiErrorNbr;			/*!< STI Error number. */
	uint32_t un32SysErrorNbr;			/*!< Value of errno at time of error. */
} SstiErrorLog;

typedef void (*pRemoteErrorLogSend)(size_t *, const char *);

void stiErrorLogRemoteLoggingSet(size_t *poRemoteLogger, pRemoteAssertSend pmRemoteErrorLogSend);

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */

SstiErrorLog * stiErrorLogInit ();

void stiErrorLogClose ();

EstiResult stiErrorLogSystemInit (
	const std::string &versionNumber);

void stiErrorLogSystemShutdown ();

void stiLogError (
	uint32_t un32Error,
	const char *pszFile,
	int nLineNbr,
	EstiErrorCriticality eCriticality);

void stiLogErrorMsg (
	uint32_t un32Error,
	const char *pszFile,
	int nLineNbr,
	EstiErrorCriticality eCriticality,
	const char *pszFormat,
	...);

void ErrorLogFileWrite(
	const char *pszMessage);


/*! \brief NOTE:  The following macro is expected to have as its' parameter an
 * unsigned integer.  More correctly, it should have as its incoming
 * parameter a value that is defined as an error message in an
 * accompanying file(s).
 */
#define stiLOG_ERROR(A)	stiLogError(A, __FILE__, __LINE__, estiLOG_ERROR);
#define stiLOG_WARNING(A) stiLogError(A, __FILE__, __LINE__, estiLOG_WARNING);

#endif /* STIERROR_H */
/* end file stiError.h */
