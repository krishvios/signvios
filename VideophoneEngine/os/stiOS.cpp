/*!
 * \file stiOS.c
 *
 * This module contains the OS Abstraction / layering functions between the
 * application and the underlying RTOS. Beneath this layer is the actual
 * interface calls to the actual RTOS being used. Currently the lower
 * levels support Windows and Linux.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

/*
 * Includes
 */
#include "stiOS.h"	/* public functions for compatibility layer.*/

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
static bool g_rtosPrimaryInit = false;

/* Unused semaphore IDs:*/
stiMUTEX_ID stiOS_tempMutexId;


/* Unused Msg queue:*/
stiMSG_Q_ID stiOS_tempQueue;

static std::string g_dynamicDataFolder;
static std::string g_staticDataFolder;

/*
 * Function Declarations
 */

/*! \brief Initializes the OS Abstraction Layer
 *
 * \return EstiResult
 */
EstiResult stiOSInit ()
{
	if (!g_rtosPrimaryInit)
	{
		uint32_t ticks;  stiUNUSED_ARG (ticks);
		g_rtosPrimaryInit = true;

#if defined (stiLINUX)

		return (stiOSLinuxInit ());
#elif defined (WIN32)

		return (stiOSWin32Init ());
#endif


		// This should force the linker to pull in the correct functions:
		stiOS_tempMutexId = stiOSMutexCreate ();

		// Ditto:
		stiOS_tempQueue = stiOSMsgQCreate (10, 64);

		// Ditto:
		ticks = stiOSTickGet ();

		// Ditto:
		stiOSTaskDelay (2);	// so that it is guaranteed one tick
	}
	return (estiOK);	// do not run init more than once.
} /* end stiOSInit */

/*! \brief Returns the current RTOS kernel time tick.
 *
 * \return The number of system ticks since the system was started.
 */
uint32_t stiOSTickGet ()
{
#if defined (stiLINUX)

	return (stiOSLinuxTickGet ());

#elif defined (WIN32)

	return (stiOSWin32TickGet ());
#endif
} /* end stiOSTickGet */


/*! \brief Returns the number of clock ticks per second.
 * \return The number of clock ticks per second on the current CPU.
 */
int32_t stiOSSysClkRateGet ()
{
#if defined (stiLINUX)

	return (stiOSLinuxSysClkRateGet ());

#elif defined (WIN32)

	return (stiOSWin32SysClkRateGet ());
#endif
} /* end stiOSSysClkRateGet */



/*! \brief Returns the root location for dynamic data to be read from/written to
 * \return none.
 */
void stiOSDynamicDataFolderGet (std::string *folder)
{
	*folder = g_dynamicDataFolder;
}


/*!\brief Prefixes the passed in filename with the dynamic data folder path
 *
 * Note: This function may be called by signal handlers and therefore must only
 * use functions approved for use within signal handlers by POSIX. See 'man 7 signal'
 * for details.
 *
 * \param fileName The name to append to the end of the dynamic data folder path
 * \param path A pointer to a buffer that will contain the dynamic data folder path and file name
 */
void stiOSDynamicDataFilePathGet (
	const char *fileName,
	char *path)
{

	for (auto letter : g_dynamicDataFolder)
	{
		*path++ = letter;
	}

	for (const char *current = fileName; *current; ++current)
	{
		*path++ = *current;
	}

	*path = '\0';
}


/*! \brief Stores the root location for dynamic data to be read from/written to.
 * \return none.
 */
void stiOSDynamicDataFolderSet (const std::string &dynamicDataFolder)
{
	g_dynamicDataFolder = dynamicDataFolder;

	// Add a directory separator to the end if not already present.
	if (g_dynamicDataFolder.empty () || g_dynamicDataFolder.back () != '/')
	{
		g_dynamicDataFolder += '/';
	}
}


/*! \brief Returns the root location for static data to be read from/written to
 * \return none.
 */
void stiOSStaticDataFolderGet (std::string *staticDataFolder)
{
	*staticDataFolder = g_staticDataFolder;
}


/*! \brief Stores the root location for static data to be read from/written to.
 * \return none.
 */
void stiOSStaticDataFolderSet (const std::string &staticDataFolder)
{
	g_staticDataFolder = staticDataFolder;

	// Add a directory separator to the end if not already present.
	if (g_staticDataFolder.back () != '/')
	{
		g_staticDataFolder += '/';
	}
}

/* end file stiOS.c */
