/*!
 * \file stiOSSignal.h
 *
 * \brief The OS abstraction for signal access.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef stiOSSIGNAL_H
#define stiOSSIGNAL_H

/*
 * Includes
 */

/* Include the appropriate OS specific header files */
#if defined (stiLINUX)
	#include "stiOSLinux.h"
	#include "stiOSLinuxSignal.h"
#elif defined (WIN32)
	#include "stiOSWin32.h"
	#include "stiOSWin32Signal.h"
#else
	/* Unknown platform */
	#error *** Error *** Unknown Operating System
#endif /* #if defined (stiLINUX) */

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

EstiResult stiOSSignalCreate (
	STI_OS_SIGNAL_TYPE *pPipe);

EstiResult stiOSSignalClose (
	IN STI_OS_SIGNAL_TYPE *p_fd);

EstiResult stiOSSignalSet (
	IN STI_OS_SIGNAL_TYPE fd);

EstiResult stiOSSignalClear (
	IN STI_OS_SIGNAL_TYPE fd);

int stiOSSignalDescriptor (
	IN STI_OS_SIGNAL_TYPE fd);


/*! \brief Create a signal.
 *
 * \retval estiOK Upon success. 
 * \retval estiERROR Upon error.
 */
EstiResult stiOSSignalCreate (
	STI_OS_SIGNAL_TYPE *pPipe) /*!< The pipe descriptor. */
{
	stiLOG_ENTRY_NAME (stiOSSignalCreate);
}

/*! \brief Set signal to indicate something is ready
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon error.
 */
extern EstiResult 
stiOSSignalSet (
	IN STI_OS_SIGNAL_TYPE fd); //!< The reference to the pipe.

/*! \brief Clear signal to reset the ready state.
 *
 * \retval eOK Success.
 * \retval eERROR Failure.
 */
extern EstiResult 
stiOSSignalClear (
	IN STI_OS_SIGNAL_TYPE fd);   //!< The pipe file descriptor.

/*! \brief Close the signal.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Otherwise.
 */
extern EstiResult 
stiOSSignalClose (
	IN STI_OS_SIGNAL_TYPE *p_fd); //!< The pipe file descriptor.

/*! \brief Return a select'able descriptor corresponding to the pipe (ready to read).
 *
 * \return A select'able descriptor or (-1) on error.
 */
extern int
stiOSSignalDescriptor (
	IN STI_OS_SIGNAL_TYPE fd); //!< The pipe file descriptor.

#endif // #ifdef stiLINUX

#endif /* stiOSSIGNAL_H */
/* end file stiOSSignal.h */
