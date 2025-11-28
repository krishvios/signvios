/*!
 * \file stiError.c
 * \brief This file implements error logging.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

/*
 * Includes
 */
#ifdef WIN32
#include <io.h>
#endif //WIN32
#include "stiError.h"
#include "stiMem.h"
#include "stiTrace.h"
#include "stiOS.h" /* OS Abstraction */
#include <cstdio>
#include <cstring>
#include <list>
#include <fcntl.h>
#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
#include <sys/stat.h>
#endif
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
#include <csignal>
#include <fcntl.h>
#include <execinfo.h>
#endif
#include "IstiPlatform.h"
#include <sstream>

// Support build systems that define the version number via compiler arg
#if defined(HAVE_VPE_BUILDINFO)
#include "vpebuildinfo.h"
#endif
#include <mutex>

/*
 * Constants
 */
#define	stiINITIAL_ERROR_STRUCT_COUNT	10
#define stiADDITIONAL_ERROR_STRUCT		5

/*
 * Typedefs
 */

/*
 * Forward Declarations
 */

int ErrorLogFind ();

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
void stiErrorSignalHandler (
	int sig,
	siginfo_t *si,
	void *ctx);

void stiUser1SignalHandler (
	int sig,
	siginfo_t *si,
	void *ctx);
#endif

/*
 * Globals
 */

/*
 * Locals
 */
static std::string g_VersionNumber;

static uint32_t g_un32stiErrorCounter = 1;	/* Counter used to keep track of the number
					 * of errors logged.
					 */

static std::recursive_mutex g_InitMutex{};	/* Global mutex to use to force mutual exclusion
					* of the initialization method.
									*/

SstiErrorLog *g_pstErrorLog = nullptr;

static std::list<SstiErrorLog *> g_ErrorLogList;


static size_t* g_poRemoteLogger = nullptr;
static pRemoteAssertSend g_pmRemoteErrorLogSend = nullptr;
static bool g_bRemoteLoggingInProgress = false;

/*
 * Function Definitions
 */

int ErrorLogFind (SstiErrorLog **ppstErrorLog)
{
	int nIndex = -1;
	stiTaskID	nCurrentTaskID = stiOSTaskIDSelf ();

	*ppstErrorLog = g_pstErrorLog;

	std::lock_guard<std::recursive_mutex> lock(g_InitMutex);
	/* Find the the correct structure pointer for this task. */
	std::list<SstiErrorLog *>::iterator iter;
	int i = 0;
	for (iter = g_ErrorLogList.begin(); iter != g_ErrorLogList.end(); iter++)
	{
		if ((*iter) != NULL && nCurrentTaskID == (*iter)->tidTaskId)
		{
			nIndex = i;
			*ppstErrorLog = *iter;
			break;
		} /* end if */
		i++;
	} /* end for */
	
	return (nIndex);
} /* end ErrorLogFind */


/*! \brief Initialize the global SStiErrorLog pointer
 *
 * To use the logging ability of the conference engine, it is neccessary to
 * call this function to set the global error log structure pointer.  This
 * pointer must change for each task and thus it is requisite to invoke this
 * function as not to overwrite the current error.
 * NOTE:
 * The CstiOsTask class method Task invokes this function.  Therefore
 * this function need only be explicitly called by those tasks that are not
 * derived from CstiOsTask.  But, those tasks that are derived from the
 * CstiOsTask need to explicitly call the base class task method inside
 * their own task functions.
 *
 * \retval estiOK The storing of the pointer is successful.
 * \retval estiERROR An error occurred in taskVarAdd.
 */
SstiErrorLog * stiErrorLogInit ()
{
	SstiErrorLog *pstErrorLog = nullptr;

	/* Lock the mutex to disallow anyone else access while in here */
	std::lock_guard<std::recursive_mutex> lock(g_InitMutex);

	// Allocate an error log structure for this task
	pstErrorLog = new SstiErrorLog;

	if (nullptr != pstErrorLog)
	{
		/* Store the address of the error struct for the current task and
		 * initialize the memory
		 */
		memset (pstErrorLog, 0, sizeof (SstiErrorLog));

		/* Add the task ID and name to this error structure */
		if (stiINVALID_WDOG_TIMER != (pstErrorLog->tidTaskId = stiOSTaskIDSelf ()))
		{
			if (nullptr != stiOSTaskName (pstErrorLog->tidTaskId))
			{
				strncpy (pstErrorLog->szTaskName,
					stiOSTaskName (pstErrorLog->tidTaskId),
					nMAX_TASK_NAME);
				pstErrorLog->szTaskName[nMAX_TASK_NAME - 1] = '\0';
			} /* end if */
		} /* end if */

		g_ErrorLogList.push_back(pstErrorLog);
	} /* end if */

	return (pstErrorLog);
} /* end stiErrorLogInit () */


/*! \brief Clean up an error log structure.
 *
 * This method is responsible for freeing the log structure associated with the
 * current task.
 *
 * \return None
 */
void stiErrorLogClose ()
{
	/* Lock the mutex to disallow anyone else access while in here */
	std::lock_guard<std::recursive_mutex> lock(g_InitMutex);

	SstiErrorLog *pstErrorLog = nullptr;
	ErrorLogFind (&pstErrorLog);

	/* Was the errlog structure associated with this task found? */
	if (pstErrorLog)
	{
		/* Free this structure from memory */
		g_ErrorLogList.remove(pstErrorLog);
		delete pstErrorLog;
	} /* end if */

} /* end stiErrorLogClose */


/*! \brief Initialize the Error Log System
 *
 * This method initializes the error logging system for the entire system.
 * It creates a necessary semaphore, creates an array of pointers to
 * structures and other necessary duties to have the system ready to receive
 * errors.
 *
 * \param pszVersionNumber The version number of the application which may be reported in error logs.
 * 
 * \retval estiOK Success
 * \retval EstiERROR Failure
 */
EstiResult stiErrorLogSystemInit (const std::string &version)
{
	EstiResult eResult = estiOK;  /* Default to success */

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	struct sigaction SigAction{};
#if DEVICE == DEV_NTOUCH_VP2
	void *trace[32];

	//
	// Request an empty back trace just to initialize the module.
	// Not doing so can cause malloc to be called in the signal handler
	//
	backtrace (trace, 0);
#endif
	// Signal handler for error conditions
	memset (&SigAction, 0, sizeof (SigAction));

	SigAction.sa_flags = SA_RESTART | SA_SIGINFO;
	SigAction.sa_sigaction = stiErrorSignalHandler;

	sigaction(SIGILL, &SigAction, nullptr);
	sigaction(SIGABRT, &SigAction, nullptr);
	sigaction(SIGBUS, &SigAction, nullptr);
	sigaction(SIGFPE, &SigAction, nullptr);
	sigaction(SIGSEGV, &SigAction, nullptr);

	//
	// Set a signal trap for SIGUSR1
	//
	SigAction.sa_flags = SA_RESTART | SA_SIGINFO;
	SigAction.sa_sigaction = stiUser1SignalHandler;

	sigaction(SIGUSR1, &SigAction, nullptr);
#endif

	g_VersionNumber = version;
	
	//
	// Allocate an error log for unregistered threads.
	//
	g_pstErrorLog = new SstiErrorLog;

	if (nullptr == g_pstErrorLog)
	{
		eResult = estiERROR;
	}
	else
	{
		memset (g_pstErrorLog, 0, sizeof (*g_pstErrorLog));

		g_pstErrorLog->un32Counter = 0;
		strcpy (g_pstErrorLog->szTaskName, "Unregistered");
	}
	
	return (eResult);

} /* end stiErrorLogSystemInit */


/*! \brief Shutdown the Error Logging system.
 *
 * Clean up of the error logging system, return the memory allocated for the
 * system, delete the semaphore, etc.
 *
 * \return None
 */
void stiErrorLogSystemShutdown ()
{
	/* Lock the mutex so that no one else can use it */
	std::lock_guard<std::recursive_mutex> lock(g_InitMutex);

	g_poRemoteLogger = nullptr;
	g_pmRemoteErrorLogSend = nullptr;

	/* Free the memory for the structures in the array */
	std::list<SstiErrorLog *>::iterator iter;
	for (iter = g_ErrorLogList.begin(); iter != g_ErrorLogList.end(); iter++)
	{
		delete *iter;
	} /* end for */
	g_ErrorLogList.clear();
	
	g_VersionNumber.clear();
	
	if (g_pstErrorLog)
	{
		delete g_pstErrorLog;
		g_pstErrorLog = nullptr;
	}
	
} /* end stiErrorLogSystemShutdown */


/*! \brief Log an error.
 *
 * This method is responsible for logging an error for the current task into
 * its' appropriate error structure.  Use stiLOG_ERROR or stiLOG_WARNING instead
 * of calling this directly.  It will handle the 2nd, 3rd and 4th parameters for
 * you.
 *
 * \return None
 */
void stiLogErrorMsg (
	uint32_t un32Error,					/*!< Error ID */
	const char* szFile,					/*!< File in which the error occurred */
	int nLineNbr,						/*!< Line number in the file */
	EstiErrorCriticality eCriticality,	/*!< Warning or Error */
	const char *pszFormat,
	...)
{
	const char *pszPathLocation = nullptr;
	const int nMAX_BUF_LEN = 1024;
	char *pszTmpBuffer = nullptr;
	char *pszMessage = nullptr;

	/* Lock the mutex to disallow anyone else access while in here */
	std::lock_guard<std::recursive_mutex> lock(g_InitMutex);

	SstiErrorLog *pstErrorLog = nullptr;
	ErrorLogFind (&pstErrorLog);

	/* Was the errlog structure associated with this task found? */
	if (pstErrorLog)
	{
		//
		// Update the counter.
		//
		pstErrorLog->un32Counter = g_un32stiErrorCounter++;
		
		/* Store the criticality */
		pstErrorLog->eCriticality = eCriticality;

		/* Get the current tick count, current task id and current task name. */
		pstErrorLog->un32TickCount = stiOSTickGet ();

		 /* Is there any path information? */
		if (nullptr != (pszPathLocation = strrchr (szFile, '/')))
		{
			/* Yes.  Strip all "path" information from the file.  All we are
			 * interested in is the actual file name.
			 */
			strncpy (pstErrorLog->szFile, pszPathLocation + 1, nMAX_FILE_NAME - 1);
		} /* end if */
		else
		{
			/* No.  Simply copy the filename */
			strncpy (pstErrorLog->szFile, szFile, nMAX_FILE_NAME - 1);
		} /* end else */

		/* Terminate the filename in case it was longer than nMAX_FILE_NAME */
		pstErrorLog->szFile[nMAX_FILE_NAME - 1] = '\0';

		/* Now copy the line number, STI error and System error numbers */
		pstErrorLog->nLineNbr = nLineNbr;
		pstErrorLog->un32StiErrorNbr = un32Error;
		pstErrorLog->un32SysErrorNbr = errno;

		char szMessage[nMAX_BUF_LEN];

		//
		// Format the optional message.
		//
		if (pszFormat)
		{
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
			else
			{
				pszMessage = szMessage;
			}

			// A macro which ends the use of variable arguments
			va_end (args);
		}

#ifdef stiDEBUGGING_TOOLS
		if(g_stiLogging)
		{
			/* Format and print the error to stdio */
			switch (pstErrorLog->eCriticality)
			{
				case estiLOG_WARNING:

					stiTrace ("\n\n\t*** Warning ***\n");

					break;

				case estiLOG_ERROR:

					stiTrace ("\n\n\t*** Error ***\n");

					break;

				case estiLOG_HRESULT:

					stiTrace ("\n\n\t*** HResult ***\n");

					break;

				default:

					stiTrace ("\n\n\t*** Error with unknown criticality ***\n");

					break;
			}
			stiTrace ("\tLog Number: %06ld\t\tTick Count: %06ld\n",
						pstErrorLog->un32Counter,
						pstErrorLog->un32TickCount);
			stiTrace ("\tTaskId: 0x%08lx\t\tTaskName: %s\n",
						(uint32_t)(uintptr_t) pstErrorLog->tidTaskId,
						pstErrorLog->szTaskName);
			stiTrace ("\tFile: %s\tLine: %d\n", pstErrorLog->szFile,
						pstErrorLog->nLineNbr);
			stiTrace ("\tSTI Error number: 0x%08lx",
						pstErrorLog->un32StiErrorNbr);
			stiTrace ("\tSYS Error number: 0x%08lx\n",
						pstErrorLog->un32SysErrorNbr);

			if (pszMessage)
			{
				stiTrace ("\tMessage: %s\n", pszMessage);
			}

			stiTrace ("\n");
		}
#endif /* stiDEBUGGING_TOOLS */

		if(g_poRemoteLogger != nullptr && g_pmRemoteErrorLogSend != nullptr
		 && !g_bRemoteLoggingInProgress)
		{
			//
			// The following variable should prevent the same thread from
			// recursively calling the remote logging code.
			//
			g_bRemoteLoggingInProgress = true;

			std::stringstream ErrorString;
			std::stringstream fileAndLine;

			ErrorString << "Criticality=";

			switch (pstErrorLog->eCriticality)
			{
				case estiLOG_WARNING:

					ErrorString << "Warning\n";

					break;

				case estiLOG_ERROR:

					ErrorString << "Error\n";

					break;

				case estiLOG_HRESULT:

					ErrorString << "HResult\n";

					break;

				default:

					ErrorString << "Unknown\n";

					break;
			}

			ErrorString << "LogNumber=" << pstErrorLog->un32Counter << "\n";
			ErrorString << "TickCount=" << pstErrorLog->un32TickCount << "\n";
			ErrorString << "TaskID=" << pstErrorLog->tidTaskId << "\n";
			ErrorString << "TaskName=" << pstErrorLog->szTaskName << "\n";
			ErrorString << "File=" << pstErrorLog->szFile << "\n";
			ErrorString << "Line=" << pstErrorLog->nLineNbr << "\n";
			ErrorString << "STIErrorNumber=" << pstErrorLog->un32StiErrorNbr << "\n";
			ErrorString << "SYSErrorNumber=" << pstErrorLog->un32SysErrorNbr << "\n";
			
			fileAndLine << pstErrorLog->szFile << pstErrorLog->nLineNbr;

			if (pszMessage)
			{
				ErrorString << "Message=\"" << pszMessage << "\"\n";
			}

			//Send buffer to remote logging service
			(*g_pmRemoteErrorLogSend)(g_poRemoteLogger, ErrorString.str (), fileAndLine.str());

			g_bRemoteLoggingInProgress = false;
		}

		if (pszTmpBuffer)
		{
			delete [] pszTmpBuffer;
			pszTmpBuffer = nullptr;
		}

	} /* end if */

} /* end LogError */

/*
 * \brief Writes a message to the open file descriptor (fd)
*/
int writeFd(int fd, char *szBuffer)
{
#ifndef WIN32
	int len = write(fd, szBuffer, strlen(szBuffer));
#else
	int len = _write(fd, szBuffer, strlen(szBuffer));
#endif //WIN32
	return len;
}


void stiLogError (
	uint32_t un32Error,					/*!< Error ID */
	const char* szFile,					/*!< File in which the error occurred */
	int nLineNbr,						/*!< Line number in the file */
	EstiErrorCriticality eCriticality)	/*!< Warning or Error */
{
	stiLogErrorMsg (un32Error, szFile, nLineNbr, eCriticality, nullptr);
}


/*!
 * \brief Writes a message to the error log file.
 * 
 * \param pszMessage A null terminated character string containing the messsage to write.
 */
void ErrorLogFileWrite(
	const char *pszMessage)
{
	char szBuffer[512];

	std::lock_guard<std::recursive_mutex> lock(g_InitMutex);

#ifndef WIN32
	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream ErrorLogFile;

	ErrorLogFile << DynamicDataFolder << "crash/error.log";

	int fd = open(ErrorLogFile.str ().c_str (), O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
#else
	int fd = _open("/crash/error.log", O_CREAT | O_APPEND | O_WRONLY);
#endif //WIN32

	if (fd != -1)
	{
		int len = 0; stiUNUSED_ARG(len);
		time_t now = time(nullptr);
		struct tm local{};
		localtime_r(&now, &local);
		strftime(szBuffer, sizeof (szBuffer), "\n%a %b %e, %Y %l:%M:%S %p\n", &local);
		len = writeFd(fd, szBuffer);

		snprintf (szBuffer, sizeof (szBuffer) - 1, "Version: %s\n", g_VersionNumber.c_str ());
		szBuffer[sizeof (szBuffer) - 1] = 0;
		len = writeFd (fd, szBuffer);

		snprintf(szBuffer, sizeof(szBuffer) - 1, "%s\n", pszMessage);
		szBuffer[sizeof(szBuffer) - 1] = 0;
		len = writeFd(fd, szBuffer);

#ifndef WIN32
		close(fd);
#else
		_close(fd);
#endif //WIN32
	}

}


#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
void stiUser1SignalHandler (
	int sig,
	siginfo_t *si,
	void *ctx)
{
	stiBackTrace ();
}
#endif


#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
#define stiMAX_FILE_PATH_LENGTH	256

/*!\brief Converts an integer to a null terminated string.
 *
 * Note: This function may be called by signal handlers and therefore must only
 * use functions approved for use within signal handlers by POSIX. See 'man 7 signal'
 * for details.
 *
 * \param un32Integer The integer to convert to a string
 * \param pszString A pointer to the buffer to contain the string
 */
int IntegerToString (
	uint32_t un32Integer,
	char *pszString)
{
	int nLength = 0;

	if (un32Integer < 10)
	{
		*pszString++ = static_cast<char>('0' + un32Integer);

		nLength = 1;
	}
	else
	{
		uint32_t un32Divisor = 1000000000;

		while (un32Divisor > un32Integer)
		{
			un32Divisor /= 10;
		}

		do
		{
			unsigned int unDigit = un32Integer / un32Divisor;

			*pszString++ = static_cast<char>('0' + unDigit);
			++nLength;

			un32Integer -= unDigit * un32Divisor;
			un32Divisor /= 10;
		}
		while (un32Divisor > 0);
	}

	*pszString = '\0';

	return nLength;
}


/*!\brief Prints a label and the provided register value in hexadecimal format
 *
 * Note: This function is a signal handler and therefore must only
 * use functions approved for use within signal handlers by POSIX.
 * See 'man 7 signal' for details.
 *
 */
void RegisterValuePrint (
	int nFd,
	const char *pszLabel,
	int nLabelLength,
	uint32_t un32RegValue)
{
	const int nNumHexadecimalDigits = 8;
	int nShift = 28;

	ssize_t result = 0;

	//
	// Print the label and hexadecimal prefix.
	//
	result = write (nFd, pszLabel, nLabelLength);
	result = write (nFd, "0x", 2);

	//
	// We are going to iterate over the number of hexadecimal digits
	// in a 32-bit number, working from left to right to print each one.
	//
	for (int i = 0; i < nNumHexadecimalDigits; i++)
	{
		//
		// Shift off the lower digits (we only want the high order digit)
		//
		uint8_t un8Digit = un32RegValue >> nShift;

		//
		// If the digit is less than 10 then print then
		// use an offset from the character '0' to print it. Otherwise,
		// use an offset from the character 'a' to print it.
		//
		if (un8Digit < 10)
		{
			un8Digit = '0' + un8Digit;
		}
		else
		{
			un8Digit = 'a' + (un8Digit - 10);
		}

		result = write (nFd, &un8Digit, 1);

		//
		// Now that we printed the most significant digit
		// mask it off and update the number of bits
		// we need to shift by to get to the new most significant digit.
		//
		un32RegValue = un32RegValue & (~0U >> (32 - nShift));

		nShift -= 4;
	}

	stiUNUSED_ARG(result);
}


#define stiVERSION_LABEL "Version: "
#define stiREG_0_LABEL     "    r0: "
#define stiREG_1_LABEL     "    r1: "
#define stiREG_2_LABEL     "    r2: "
#define stiREG_3_LABEL     "    r3: "
#define stiREG_4_LABEL     "    r4: "
#define stiREG_5_LABEL     "    r5: "
#define stiREG_6_LABEL     "    r6: "
#define stiREG_7_LABEL     "    r7: "
#define stiREG_8_LABEL     "    r8: "
#define stiREG_9_LABEL     "    r9: "
#define stiREG_10_LABEL    "   r10: "
#define stiREG_11_LABEL    "   r11: "
#define stiREG_12_LABEL    "   r12: "
#define stiREG_13_LABEL    "   r13: "
#define stiREG_14_LABEL    "   r14: "
#define stiREG_15_LABEL    "   r15: "
#define stiREG_FP_LABEL    "    fp: "
#define stiREG_IP_LABEL    "    ip: "
#define stiREG_SP_LABEL    "    sp: "
#define stiREG_LR_LABEL    "    lr: "
#define stiREG_PC_LABEL    "    pc: "
#define stiREG_CPSR_LABEL  "  cpsr: "
#define stiREG_TRAP_LABEL  "  trap: "
#define stiREG_ERROR_LABEL " error: "
#define stiREG_MASK_LABEL  "  mask: "
#define stiSIGNAL_LABEL    "Error Signal Handler: "
#define stiREG_RDI_LABEL    "   rdi: "
#define stiREG_RSI_LABEL    "   rsi: "
#define stiREG_RBP_LABEL    "   rbp: "
#define stiREG_RAX_LABEL    "   rax: "
#define stiREG_RBX_LABEL    "   rbx: "
#define stiREG_RCX_LABEL    "   rcx: "
#define stiREG_RDX_LABEL    "   rdx: "
#define stiREG_RSP_LABEL    "   rsp: "
#define stiREG_RIP_LABEL    "   rip: "
#define stiREG_EFL_LABEL    " eflgs: "
#define stiREG_CSGSFS_LABEL "  cgfs: "
#define stiREG_ERR_LABEL    "   err: "
#define stiREG_OLDMASK_LABEL " omask: "
#define stiREG_CR2_LABEL    "   cr2: "

/*!
 * \brief signal safe string length
 *
 * Documentation doesn't say strlen can be called (not until posix 2016)
 */

int signal_strlen(const char *str)
{
	const char *current = str;

	while(*current != '\0')
	{
		++current;
	}

	return static_cast<int>(current - str);
}

/*!\brief Writes the registers and back trace to the provided file descriptor
 *
 * Note: This function is a signal handler and therefore must only
 * use functions approved for use within signal handlers by POSIX.
 * See 'man 7 signal' for details.
 *
 */
static void CrashDumpWrite (
	int fd,
	int nSig,
	void *ctx)
{
	ssize_t result = 0;

	result = write (fd, stiVERSION_LABEL, signal_strlen(stiVERSION_LABEL));
#if defined(HAVE_VPE_BUILDINFO)
	result = write (fd, vpe::buildinfo::version(), signal_strlen(vpe::buildinfo::version()));
#else
	result = write (fd, smiSW_VERSION_NUMBER, signal_strlen(smiSW_VERSION_NUMBER));
#endif
	result = write (fd, "\n", 1);

#if DEVICE == DEV_NTOUCH_VP2
	//
	// Write the register values to the log file
	//
	auto pContext = static_cast<ucontext_t *>(ctx);

	RegisterValuePrint (fd, stiREG_0_LABEL, signal_strlen (stiREG_0_LABEL), pContext->uc_mcontext.arm_r0);
	RegisterValuePrint (fd, stiREG_1_LABEL, signal_strlen (stiREG_1_LABEL), pContext->uc_mcontext.arm_r1);
	RegisterValuePrint (fd, stiREG_2_LABEL, signal_strlen (stiREG_2_LABEL), pContext->uc_mcontext.arm_r2);
	RegisterValuePrint (fd, stiREG_3_LABEL, signal_strlen (stiREG_3_LABEL), pContext->uc_mcontext.arm_r3);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_4_LABEL, signal_strlen (stiREG_4_LABEL), pContext->uc_mcontext.arm_r4);
	RegisterValuePrint (fd, stiREG_5_LABEL, signal_strlen (stiREG_5_LABEL), pContext->uc_mcontext.arm_r5);
	RegisterValuePrint (fd, stiREG_6_LABEL, signal_strlen (stiREG_6_LABEL), pContext->uc_mcontext.arm_r6);
	RegisterValuePrint (fd, stiREG_7_LABEL, signal_strlen (stiREG_7_LABEL), pContext->uc_mcontext.arm_r7);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_8_LABEL,  signal_strlen (stiREG_8_LABEL),  pContext->uc_mcontext.arm_r8);
	RegisterValuePrint (fd, stiREG_9_LABEL,  signal_strlen (stiREG_9_LABEL),  pContext->uc_mcontext.arm_r9);
	RegisterValuePrint (fd, stiREG_10_LABEL, signal_strlen (stiREG_10_LABEL), pContext->uc_mcontext.arm_r10);
	RegisterValuePrint (fd, stiREG_FP_LABEL, signal_strlen (stiREG_FP_LABEL), pContext->uc_mcontext.arm_fp);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_IP_LABEL, signal_strlen (stiREG_IP_LABEL), pContext->uc_mcontext.arm_ip);
	RegisterValuePrint (fd, stiREG_SP_LABEL, signal_strlen (stiREG_SP_LABEL), pContext->uc_mcontext.arm_sp);
	RegisterValuePrint (fd, stiREG_LR_LABEL, signal_strlen (stiREG_LR_LABEL), pContext->uc_mcontext.arm_lr);
	RegisterValuePrint (fd, stiREG_PC_LABEL, signal_strlen (stiREG_PC_LABEL), pContext->uc_mcontext.arm_pc);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_CPSR_LABEL,  signal_strlen (stiREG_CPSR_LABEL),  pContext->uc_mcontext.arm_cpsr);
	RegisterValuePrint (fd, stiREG_TRAP_LABEL,  signal_strlen (stiREG_TRAP_LABEL),  pContext->uc_mcontext.trap_no);
	RegisterValuePrint (fd, stiREG_ERROR_LABEL, signal_strlen (stiREG_ERROR_LABEL), pContext->uc_mcontext.error_code);
	RegisterValuePrint (fd, stiREG_MASK_LABEL,  signal_strlen (stiREG_MASK_LABEL),  pContext->uc_mcontext.oldmask);
	result = write (fd, "\n", 1);

	result = write (fd, "\n", 1);

#elif DEVICE == DEV_NTOUCH_VP3

	auto pContext = static_cast<ucontext_t *>(ctx);

	RegisterValuePrint (fd, stiREG_8_LABEL, signal_strlen (stiREG_8_LABEL), pContext->uc_mcontext.gregs[REG_R8]);
	RegisterValuePrint (fd, stiREG_9_LABEL, signal_strlen (stiREG_9_LABEL), pContext->uc_mcontext.gregs[REG_R9]);
	RegisterValuePrint (fd, stiREG_10_LABEL, signal_strlen (stiREG_10_LABEL), pContext->uc_mcontext.gregs[REG_R10]);
	RegisterValuePrint (fd, stiREG_11_LABEL, signal_strlen (stiREG_11_LABEL), pContext->uc_mcontext.gregs[REG_R11]);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_12_LABEL, signal_strlen (stiREG_12_LABEL), pContext->uc_mcontext.gregs[REG_R12]);
	RegisterValuePrint (fd, stiREG_13_LABEL, signal_strlen (stiREG_13_LABEL), pContext->uc_mcontext.gregs[REG_R13]);
	RegisterValuePrint (fd, stiREG_14_LABEL, signal_strlen (stiREG_14_LABEL), pContext->uc_mcontext.gregs[REG_R14]);
	RegisterValuePrint (fd, stiREG_15_LABEL, signal_strlen (stiREG_15_LABEL), pContext->uc_mcontext.gregs[REG_R15]);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_RDI_LABEL, signal_strlen (stiREG_RDI_LABEL), pContext->uc_mcontext.gregs[REG_RDI]);
	RegisterValuePrint (fd, stiREG_RSI_LABEL, signal_strlen (stiREG_RSI_LABEL), pContext->uc_mcontext.gregs[REG_RSI]);
	RegisterValuePrint (fd, stiREG_RBP_LABEL, signal_strlen (stiREG_RBP_LABEL), pContext->uc_mcontext.gregs[REG_RBP]);
	RegisterValuePrint (fd, stiREG_RAX_LABEL, signal_strlen (stiREG_RAX_LABEL), pContext->uc_mcontext.gregs[REG_RAX]);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_RBX_LABEL, signal_strlen (stiREG_RBX_LABEL), pContext->uc_mcontext.gregs[REG_RBX]);
	RegisterValuePrint (fd, stiREG_RCX_LABEL, signal_strlen (stiREG_RCX_LABEL), pContext->uc_mcontext.gregs[REG_RCX]);
	RegisterValuePrint (fd, stiREG_RDX_LABEL, signal_strlen (stiREG_RDX_LABEL), pContext->uc_mcontext.gregs[REG_RDX]);
	RegisterValuePrint (fd, stiREG_RSP_LABEL, signal_strlen (stiREG_RSP_LABEL), pContext->uc_mcontext.gregs[REG_RSP]);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_RIP_LABEL, signal_strlen (stiREG_RIP_LABEL), pContext->uc_mcontext.gregs[REG_RIP]);
	RegisterValuePrint (fd, stiREG_EFL_LABEL, signal_strlen (stiREG_EFL_LABEL), pContext->uc_mcontext.gregs[REG_EFL]);
	RegisterValuePrint (fd, stiREG_CSGSFS_LABEL, signal_strlen (stiREG_CSGSFS_LABEL), pContext->uc_mcontext.gregs[REG_CSGSFS]);
	RegisterValuePrint (fd, stiREG_ERR_LABEL, signal_strlen (stiREG_ERR_LABEL), pContext->uc_mcontext.gregs[REG_ERR]);
	result = write (fd, "\n", 1);

	RegisterValuePrint (fd, stiREG_TRAP_LABEL, signal_strlen (stiREG_TRAP_LABEL), pContext->uc_mcontext.gregs[REG_TRAPNO]);
	RegisterValuePrint (fd, stiREG_OLDMASK_LABEL, signal_strlen (stiREG_OLDMASK_LABEL), pContext->uc_mcontext.gregs[REG_OLDMASK]);
	RegisterValuePrint (fd, stiREG_CR2_LABEL, signal_strlen (stiREG_CR2_LABEL), pContext->uc_mcontext.gregs[REG_CR2]);
	result = write (fd, "\n", 1);

	result = write (fd, "\n", 1);

#elif DEVICE == DEV_NTOUCH_VP4
	//TODO: vp4 registrer prints
	
#endif

	//
	// Write the signal that was caught.
	//
	result = write (fd, stiSIGNAL_LABEL, signal_strlen(stiSIGNAL_LABEL));

	char szBuffer[16];

	int length = IntegerToString (nSig, szBuffer);
	szBuffer[length] = '\n';
	szBuffer[length + 1] = '\0';

	result = write (fd, szBuffer, signal_strlen (szBuffer));

	stiUNUSED_ARG(result);

	//
	// Write the backtrace
	//
	stiBackTraceToFile(fd);
}


/*!\brief Handles signal such as signal 11 (segmentation fault)
 *
 * Note: This function is a signal handler and therefore must only
 * use functions approved for use within signal handlers by POSIX.
 * See 'man 7 signal' for details.
 *
 */
void stiErrorSignalHandler (
	int sig,
	siginfo_t *si,
	void *ctx)
{
	int nLen = 0; stiUNUSED_ARG(nLen);
	const int nNumOpenFileAttempts = 10;

	char BacktraceFile[stiMAX_FILE_PATH_LENGTH + 1];

	stiOSDynamicDataFilePathGet ("crash/backtrace.", BacktraceFile);

	//
	// Append a string of digits based on the current time to
	// help make the file unique.
	//
	auto  un32Time = (uint32_t)time(nullptr);

	//
	// Determine the length of the current file name.
	//
	for (nLen = 0; BacktraceFile[nLen]; ++nLen)
	{
	}

	//
	// Multiple threads can call this at the same time. Therefore, it is possible
	// to fail to open the file because it was opened by a different thread.
	// Try multiple times, changing the file name each time, until the
	// maximum number of allowed attempts is reached.
	//
	for (int i = 0; i < nNumOpenFileAttempts; i++)
	{
		un32Time += i;

		IntegerToString (un32Time, &BacktraceFile[nLen]);

		// Print the backtrace to a file...
		int fd = open(BacktraceFile, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU);

		if (fd != -1)
		{
			CrashDumpWrite (fd, sig, ctx);

			close(fd);

			//
			// We have successfully created and written to the crash log.
			// We can exit the loop.
			//
			break;
		}
	}

	// AND send it to the console too.
	CrashDumpWrite (STDOUT_FILENO, sig, ctx);

	//
	// Write the restart reason to the restart reason file.
	//
	char RestartReasonFile[stiMAX_FILE_PATH_LENGTH + 1];

	stiOSDynamicDataFilePathGet ("crash/RestartReason", RestartReasonFile);

	// Print the restart reason to a file...
	int fd = open(RestartReasonFile, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);

	if (fd != -1)
	{
		char szBuffer[16];

		int nLength = IntegerToString (IstiPlatform::estiRESTART_REASON_CRASH, szBuffer);
		szBuffer[nLength] = '\n';
		szBuffer[nLength + 1] = '\0';
		nLen = write (fd, szBuffer, strlen (szBuffer));

		close (fd);
	}

	//
	// Set the signal handler back to the default and raise it again.  This will allow the system
	// to terminate or kill the application as it would normally do.
	//
	struct sigaction SigAction{};

	memset (&SigAction, 0, sizeof (SigAction));

	SigAction.sa_handler = SIG_DFL;

	sigaction(sig, &SigAction, nullptr);

	raise (sig);
}
#endif


void stiErrorLogRemoteLoggingSet(size_t *poRemoteLogger, pRemoteAssertSend pmRemoteErrorLogSend)
{
	g_pmRemoteErrorLogSend = pmRemoteErrorLogSend;
	g_poRemoteLogger = poRemoteLogger;
}

/* end file stiError.cpp */
