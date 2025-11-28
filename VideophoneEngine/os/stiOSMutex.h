/*!
 * \file stiOSMutex.h
 *
 * This module contains the OS Abstraction / layering functions between the
 * application and the underlying RTOS for the mutex sub-functions.
 * 
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef STIOSMUTEX_H
#define STIOSMUTEX_H

#include "stiAssert.h"

/*
 * Includes
 */
/* Include the appropriate OS specific header files */
#if defined (stiLINUX)
	#include "stiOSLinux.h"
	#include "stiOSLinuxMutex.h"

#elif defined (WIN32)
	#include "stiOSWin32.h"
	#include "stiOSWin32Mutex.h"

#else
	/* Unknown platform */
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



/**
 * CstiOSMutexLock
 *
 * This is a class to manage locking and unlocking mutexes withing
 * the scope of a function.  The idea is to lock the mutex
 * at the start of the function and then not have to worry about 
 * when the mutex is unlocked.  The mutex unlocks when the lock
 * object goes out of scope.
 *
 * void SomeClass::threadsafe_func() {
 *  CstiOSMutexLock threadSafe(m_Mutex);
 *  // thread safe code...
 * }
 **/
class CstiOSMutexLock
{
public:
	CstiOSMutexLock( stiMUTEX_ID mutex)
	{
		m_Mutex = mutex;
		stiHResult hResult = stiOSMutexLock(mutex);
		stiASSERT(!stiIS_ERROR(hResult));
	}

	~CstiOSMutexLock()
	{
		stiOSMutexUnlock(m_Mutex);
	}

private:

	stiMUTEX_ID m_Mutex;
};




#if 0  /* This section is here for documentation purposes. Function prototypes 
		* for automated documentation are listed first, followed by "fake" 
		* function definitions for documentation purposes. Actual prototypes 
		* and implementations are done in the OS-specific files.
		*/
stiMUTEX_ID stiOSMutexCreate (void);

EstiResult stiOSMutexDestroy (
	stiMUTEX_ID mutexID);

EstiResult stiOSMutexLock (
	stiMUTEX_ID mutexID);

EstiResult stiOSMutexUnlock (
	stiMUTEX_ID mutexID);

/*! \brief Creates a mutual exclusion lock.
 *
 * A Mutex is similar to a semaphore, but it only has two states, locked and 
 * unlocked.  This function creates and initializes the mutex to an unlocked 
 * state.
 *
 * \return The mutex ID or NULL when there is not enough memory for the allocation.
 */
stiMUTEX_ID stiOSMutexCreate (void)
{
	stiLOG_ENTRY_NAME (stiOSMutexCreate);
} /* end stiOSMutexCreate */


/*! \brief Destroys a mutual exclusion lock.
 *
 * This should not be called on a Mutex that is in a locked state.  The
 * behavior is undefined.
 *
 * \retval estiOK - Successfully destroyed the mutex.
 * \retval estiERROR - Undefined mutex ID
 */
EstiResult stiOSMutexDestroy (
	stiMUTEX_ID mutexID)
/*!< The mutex ID to destroy. */
{
	stiLOG_ENTRY_NAME (stiOSMutexDestroy);
} /* end stiOSMutexDestroy */


/*! \brief Changes Mutex from an unlocked to locked state.
 *
 * If the mutex is already locked the calling thread will block until it 
 * becomes unlocked.  If the mutex is in an unlocked state, the state is 
 * changed to locked and the calling thread becomes the owner of the mutex.
 *	
 * \retval estiOK Successfully locked the mutex.
 * \retval estiERROR Otherwise.
 */
EstiResult stiOSMutexLock (
	stiMUTEX_ID mutexID)
/*!< The mutex ID to be locked. */
{
	stiLOG_ENTRY_NAME (stiOSMutexLock);
} /* end stiOSMutexLock */


/*! \brief Changes Mutex from locked to an unlocked state.
 *
 * This function should only be called by the thread that locked the mutex.  It
 * should not be called on a mutex that is already unlocked.
 *	
 * \retval estiOK When the mutex is successfully unlocked. 
 * \retval estiERROR An invalid mutex ID
was supplied.
*/
EstiResult stiOSMutexUnlock (
	stiMUTEX_ID mutexID)
/*!< The mutex ID to be unlocked. */
{
	stiLOG_ENTRY_NAME (stiOSMutexUnlock);
} /* end stiOSMutexUnlock */

#endif /* ifdef 0 */

#endif // ifndef STIOSMUTEX_H
/* end file stiOSMutex.h */
