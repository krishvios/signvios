/*!
 * \file stiOSWd.h
 *
 * Implements Watchdog timer functionality for the OS abstraction layer.
 * Watchdog timers are used for recurrent and one-shot timers. When the 
 * timer fires, a specified function is called. Note that this function is
 * called within the context of an interrupt service routine, so the ISR
 * limitations apply (like no printfs).
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef STIOSWD_H
#define STIOSWD_H

/*
 * Includes
 */
/* Include the appropriate OS specific header files */
#if defined (stiLINUX)
	#include "stiOSLinuxWd.h"
	#include "stiOSLinux.h"

#elif defined (WIN32)
	#include "stiOSWin32Wd.h"
	#include "stiOSWin32.h"

#else
	/* Unknown platform */
	#error *** Error *** Unknown Operating System
#endif

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
 * Function Declarations
 */
#if 0  /* This section is here for documentation purposes. Function prototypes 
		* for automated documentation are listed first, followed by "fake" 
		* function definitions for documentation purposes. Actual prototypes 
		* and implementations are done in the OS-specific files.
		*/

EstiResult stiOSWdCancel (
	stiWDOG_ID wdId);

stiWDOG_ID stiOSWdCreate ();

EstiResult stiOSWdDelete (
	stiWDOG_ID wdId);

EstiResult stiOSWdStart (
	stiWDOG_ID wdId,
	int nDelay,
	stiFUNC_PTR pRoutine,
	void *pParameter);


/*! \brief Cancels a running watchdog timer.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
 */
EstiResult stiOSWdCancel (
	stiWDOG_ID wdId)	/*!< The ID of watchdog timer to cancel. */
{
} /* end stiOSWdCancel */

/*! \brief Create a watchdog timer.
 *
 * \return The ID of the newly created watchdog timer or NULL upon a malloc error.
 */
stiWDOG_ID stiOSWdCreate ()
{
} /* end stiOSWdCreate */

/*! \brief Delete a watchdog timer.
 *
 * Frees the memory allocated by stiOSWdCreate.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
*/
EstiResult stiOSWdDelete (
	stiWDOG_ID wdId) /*!< The ID of the watchdog timer to delete. */
{
} /* end stiOSWdDelete */


/*! \brief Start a watchdog timer.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
 */
EstiResult stiOSWdStart (
	stiWDOG_ID wdId,	/*!< The ID of watchdog timer to start. */
	int nDelay,			/*!< Time to wait (in system ticks) before calling the
						 * callback function.
						 */
	stiFUNC_PTR pRoutine, 	/*!< The callback function to call when timer expires. */
	void *pParameter)		/*!< A parameter to be passed to callback function. */
{
} /* end stiOSWdStart */


#endif /* Documentation section */
#endif /* STIOSWD_H */
/* end file stiOSWd.h */
