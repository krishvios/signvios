/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSMsgLinux
*
*	File Name:	stiOSMsgLinux.c
*
*	Owner:		Scot L. Brooksby
*
*	Abstract:
*		This module contains the OS Abstraction / layering functions between the RTOS
*		OS Abstraction functions and Linux message queue functions.
*
*******************************************************************************/

/*
 * Includes
 */
#include <cstdlib>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/stat.h>
#if APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
#include <sys/msg.h>
#endif
#include <cstring>

#include "stiOS.h"
#include "stiOSLinux.h"
#include "stiOSLinuxWd.h"
#include "stiTrace.h"

#if defined (__APPLE__) || APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE 
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
 * Locals
 */
struct stiWDOG_ID_s
{
	stiWDOG_ID_s ();
	
	stiWDOG_ID w_next;
	stiWDOG_ID w_prev;

	stiFUNC_PTR w_TimerCallback{};
	stiFUNC_PTR w_CancelCallback{};
	stiTIMER_CALLBACK w_TimerCallback2{};
	stiTIMER_CALLBACK w_CancelCallback2{};
	intptr_t w_parameter1{};
	intptr_t w_parameter2{};
	struct timeval w_expires{};
	char        w_active;		// Indicates that it currently resides in the linked list

	stiWDOG_ID  w_magic;
};

stiWDOG_ID_s::stiWDOG_ID_s ()
{
	w_active = estiFALSE;
	w_magic = this;
	w_next = nullptr;
	w_prev = nullptr;
}


static pthread_once_t wdog_init_once = PTHREAD_ONCE_INIT;
static struct stiWDOG_ID_s *g_pRoot;
static bool g_bListItemChanged = false;
static stiWDOG_ID g_NextItem = nullptr;
static stiWDOG_ID g_FiringTimer = nullptr;
static stiTaskID g_WDogTask;
static pthread_t wdog_thread;
static bool bWdogInitialized = false;
static pthread_mutex_t wdog_mutex;
static pthread_cond_t wdog_cond;
static pthread_cond_t wdog_shutdown_cond;
static pthread_cond_t wdog_fire_cond;
static bool wdog_exit = false;


static inline int wd_lock ()
{
	return (!pthread_mutex_lock(&wdog_mutex));
}

static inline void wd_unlock ()
{
	pthread_mutex_unlock(&wdog_mutex);
}

static void wdog_remove (
	stiWDOG_ID w)
{
	if (w == g_NextItem)
	{
		//
		// The next item item is the one being 
		// removed.  Change the next item to be the next one
		// after the one being removed.
		//
		g_NextItem = w->w_next;
	}
	
	w->w_active = estiFALSE;

	w->w_prev->w_next = w->w_next;
	w->w_next->w_prev = w->w_prev;

	w->w_next = nullptr;
	w->w_prev = nullptr;
}


static void wdog_add (
	stiWDOG_ID w)
{
	w->w_active = estiTRUE;

	w->w_next = g_pRoot;
	w->w_prev = g_pRoot->w_prev;
	g_pRoot->w_prev->w_next = w;
	g_pRoot->w_prev = w;
	if (g_NextItem == g_pRoot)
	{
		//
		// The next item is the root which means
		// we reached the end of the list.  Change
		// the next item to the one just added.  Subsequent
		// additions to the list will happen after this one.
		//
		g_NextItem = w;
	}
}


static stiWDOG_ID find_nearest_expiration ()
{
	stiWDOG_ID nearest;
	stiWDOG_ID w;
	struct timeval now{};

	gettimeofday(&now, nullptr);
	nearest = nullptr;
	for (w = g_pRoot->w_next; w != g_pRoot; w = g_NextItem)
	{
		g_NextItem = w->w_next;

		/* First see if the timer has expired */
		if (timercmp(&now, &w->w_expires, >=))
		{
			// The timer has expired.
			// Remove it from the list and 
			// call its callback function.
			//
			wdog_remove (w);

			g_bListItemChanged = false;
			
			//
			// Save a pointer to the firing timer so that
			// others (in this file) may know which timer is being fired.
			//
			g_FiringTimer = w;
			
			wd_unlock ();
			
			if (w->w_TimerCallback2)
			{
				w->w_TimerCallback2(w, w->w_parameter1, w->w_parameter2);
			}
			else
			{
				w->w_TimerCallback(w->w_parameter1);
			}
			
			wd_lock ();
			
			//
			// We are done firing the timer.  Clear the pointer to the
			// firing timer and let everyone know that the timer is no longer
			// firing.
			//
			g_FiringTimer = nullptr;
			pthread_cond_broadcast (&wdog_fire_cond);
			
			//
			// If a list item has been changed then...
			//
			if (g_bListItemChanged)
			{
				//
				// An item's time has changed.  We need to traverse the list
				// from the beginning again.
				//
				g_NextItem = g_pRoot->w_next;
				nearest = nullptr;
			}
		}
		else if (!nearest)
		{
			nearest = w;
		}
		else if (timercmp(&nearest->w_expires, &w->w_expires, >))
		{
			nearest = w;
		}
	}
	g_NextItem = nullptr;
	return (nearest);
}

/*
 * Function Declarations
 */
static void * wdog_start (void * arg)
{
	/*
	 * Just a simple implementation that presumes there aren't very
	 * many callbacks.  If there are a lot of callbacks, then we
	 * should use a different data structure to maintain the active
	 * timers.
	 */

	wdog_thread = pthread_self ();
	wdog_exit = false;
	
	for (;;)
	{
		if (wd_lock())
		{
			stiWDOG_ID wd_wait = find_nearest_expiration();
			if (wd_wait)
			{
				struct timespec ts{};
				ts.tv_sec = wd_wait->w_expires.tv_sec;
				ts.tv_nsec = wd_wait->w_expires.tv_usec * NSEC_PER_USEC;
				pthread_cond_timedwait(&wdog_cond, &wdog_mutex, &ts);
				//wd_wait->w_TimerCallback(w->w_parameter);
			}
			else
			{
				pthread_cond_wait(&wdog_cond, &wdog_mutex);
			}

			if (wdog_exit)
			{
				wd_unlock();
				
				break;
			}
			
			wd_unlock();
		}
	}

	pthread_cond_broadcast(&wdog_shutdown_cond);
	
	return (nullptr);
}


static void wdog_init ()
{
	g_pRoot = new stiWDOG_ID_s;
	g_pRoot->w_next = g_pRoot;
	g_pRoot->w_prev = g_pRoot;
}


stiHResult stiOSWdInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	
	pthread_once(&wdog_init_once, wdog_init);

	stiTESTCOND_NOLOG (!bWdogInitialized, stiRESULT_ERROR);

	pthread_mutexattr_t stAttribute;
	pthread_mutexattr_init (&stAttribute);
	pthread_mutexattr_settype (&stAttribute, PTHREAD_MUTEX_RECURSIVE_NP);

	pthread_mutex_init(&wdog_mutex, &stAttribute);
	
	nResult = pthread_cond_init(&wdog_cond, nullptr);
	stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	
	nResult = pthread_cond_init(&wdog_shutdown_cond, nullptr);
	stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	
	nResult = pthread_cond_init (&wdog_fire_cond, nullptr);
	stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	
	g_WDogTask = stiOSLinuxTaskSpawn ("wdog", stiOS_TASK_DEFAULT_PRIORITY,
							stiOS_TASK_DEFAULT_STACK_SIZE, wdog_start, nullptr);
	stiTESTCOND_NOLOG (g_WDogTask, stiRESULT_ERROR);

	bWdogInitialized = true;

STI_BAIL:

	pthread_mutexattr_destroy (&stAttribute);
	
	return hResult;
}


void stiOSWdUninitialize ()
{
	if (wd_lock())
	{
		wdog_exit = true;
		
		pthread_cond_broadcast(&wdog_cond);

		pthread_cond_wait(&wdog_shutdown_cond, &wdog_mutex);
		
		pthread_cond_destroy (&wdog_cond);
		
		pthread_cond_destroy (&wdog_shutdown_cond);
		
		pthread_cond_destroy (&wdog_fire_cond);
		
		wd_unlock ();
		
		pthread_mutex_destroy (&wdog_mutex);

		bWdogInitialized = false;
	}
}


extern stiWDOG_ID stiOSWdCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiWDOG_ID w = nullptr;

	stiTESTCOND_NOLOG(bWdogInitialized, stiRESULT_ERROR);

	w = new stiWDOG_ID_s;

STI_BAIL:
	if (hResult) {}

	return (w);
}


extern stiHResult stiOSWdDelete (stiWDOG_ID w)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (wd_lock())
	{
		if (w && (w->w_magic == w))
		{
			stiOSWdCancel (w);
			
			//
			// We need to block here if we are not in the same thread as the watchdog timers
			// and the timer that is being fired, if any, is the timer we want to delete.
			//
			if (pthread_self () != wdog_thread)
			{
				while (g_FiringTimer == w)
				{
					pthread_cond_wait (&wdog_fire_cond, &wdog_mutex);
				}
			}
			
			w->w_magic = nullptr;
			delete w;
		}
		wd_unlock ();
	}

	return (hResult);
}


extern stiHResult stiOSWdCancel (stiWDOG_ID w)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bLocked = false;

	int nResult = wd_lock();
	stiTESTCOND_NOLOG (nResult != 0, stiRESULT_ERROR);

	bLocked = true;

	stiTESTCOND_NOLOG (w && (w->w_magic == w), stiRESULT_ERROR);

	//
	// If this is an active timer then remove it from the list.
	// Note that a currently firing timer is no longer in the
	// list of active timers.
	//
	if (w->w_active)
	{
		//
		// If there is a cancel callback then call it.
		//
		if (w->w_CancelCallback2)
		{
			w->w_CancelCallback2(w, w->w_parameter1, w->w_parameter2);
		}
		else if (w->w_CancelCallback)
		{
			w->w_CancelCallback(w->w_parameter1);
		}

		wdog_remove (w);
	}

STI_BAIL:

	if (bLocked)
	{
		wd_unlock();
	}

	return (hResult);
}


stiHResult stiOSWdStart (
	stiWDOG_ID w,
	int nDelay,				// in milliseconds
	stiFUNC_PTR pTimerCallback,
	intptr_t Parameter)
{
	return stiOSWdStart (w, nDelay, pTimerCallback, nullptr, Parameter);
}


stiHResult stiOSWdStart (
	stiWDOG_ID w,
	int nDelay,				// in milliseconds
	stiFUNC_PTR pTimerCallback,
	stiFUNC_PTR pCancelCallback,
	intptr_t Parameter)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct timeval expires{};
	bool bLocked = false;

	int nResult = wd_lock();
	stiTESTCOND_NOLOG (nResult != 0, stiRESULT_ERROR);

	bLocked = true;

	stiTESTCOND_NOLOG (w && (w->w_magic == w), stiRESULT_ERROR);

	gettimeofday(&expires, nullptr);

	expires.tv_usec += (nDelay % MSEC_PER_SEC) * USEC_PER_MSEC;
	expires.tv_sec += expires.tv_usec / USEC_PER_SEC + nDelay / MSEC_PER_SEC;
	expires.tv_usec %= USEC_PER_SEC;

	w->w_TimerCallback = pTimerCallback;
	w->w_CancelCallback = pCancelCallback;
	w->w_TimerCallback2 = nullptr;
	w->w_CancelCallback2 = nullptr;
	w->w_parameter1 = Parameter;
	w->w_expires = expires;

	if (!w->w_active)
	{
		wdog_add (w);
	}
	else
	{
		g_bListItemChanged = true;
	}

	pthread_cond_broadcast(&wdog_cond);

STI_BAIL:

	if (bLocked)
	{
		wd_unlock();
	}

	return (hResult);
}


stiHResult stiOSWdStart2 (
	stiWDOG_ID w,
	int nDelay, 				// in milliseconds
	stiTIMER_CALLBACK pTimerCallback,
	intptr_t Parameter1,
	intptr_t Parameter2)
{
	return stiOSWdStart2 (w, nDelay, pTimerCallback, nullptr, Parameter1, Parameter2);
}


stiHResult stiOSWdStart2 (
	stiWDOG_ID w,
	int nDelay, 				// in milliseconds
	stiTIMER_CALLBACK pTimerCallback,
	stiTIMER_CALLBACK pCancelCallback,
	intptr_t Parameter1,
	intptr_t Parameter2)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct timeval expires{};
	bool bLocked = false;

	int nResult = wd_lock();
	stiTESTCOND_NOLOG (nResult != 0, stiRESULT_ERROR);

	bLocked = true;

	stiTESTCOND_NOLOG (w && (w->w_magic == w), stiRESULT_ERROR);

	gettimeofday(&expires, nullptr);

	expires.tv_usec += (nDelay % MSEC_PER_SEC) * USEC_PER_MSEC;
	expires.tv_sec += expires.tv_usec / USEC_PER_SEC + nDelay / MSEC_PER_SEC;
	expires.tv_usec %= USEC_PER_SEC;

	w->w_TimerCallback = nullptr;
	w->w_CancelCallback = nullptr;
	w->w_TimerCallback2 = pTimerCallback;
	w->w_CancelCallback2 = pCancelCallback;
	w->w_parameter1 = Parameter1;
	w->w_parameter2 = Parameter2;
	w->w_expires = expires;

	if (!w->w_active)
	{
		wdog_add (w);
	}
	else
	{
		g_bListItemChanged = true;
	}

	pthread_cond_broadcast(&wdog_cond);

STI_BAIL:

	if (bLocked)
	{
		wd_unlock();
	}

	return (hResult);
}

/* end file stiOSLinuxWd.c */

