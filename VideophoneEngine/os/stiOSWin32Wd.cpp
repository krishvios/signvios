/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSWin32Wd
*
*	File Name:	stiOSWin32Wd.cpp
*
*	Abstract:
*		This module contains the OS Abstraction / layering functions between the RTOS
*		OS Abstraction functions and Windows message queue functions.
*
*******************************************************************************/

/*
 * Includes
 */
#include <stdlib.h>
#include <string.h>

#include "stiOS.h"
#include "stiOSWin32.h"
#include "stiOSWin32Wd.h"
#include "stiTrace.h"

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

	stiFUNC_PTR w_TimerCallback;
	stiFUNC_PTR w_CancelCallback;
	stiTIMER_CALLBACK w_TimerCallback2;
	stiTIMER_CALLBACK w_CancelCallback2;
	int w_parameter1;
	int w_parameter2;
    struct timeval w_expires;
    char        w_active;		// Indicates that it currently resides in the linked list

    stiWDOG_ID  w_magic;
};

stiWDOG_ID_s::stiWDOG_ID_s ()
{
	w_active = estiFALSE;
	w_magic = this;
	w_next = NULL;
	w_prev = NULL;
}


static pthread_once_t wdog_init_once = PTHREAD_ONCE_INIT;
static struct stiWDOG_ID_s *g_pRoot;
static bool g_bListItemChanged = false;
static stiWDOG_ID g_NextItem = NULL;
static stiWDOG_ID g_FiringTimer = NULL;
static stiTaskID g_WDogTask;
static pthread_t wdog_thread;
static char wdog_init_failed;
static pthread_mutex_t wdog_mutex;
static pthread_cond_t wdog_cond;
static pthread_cond_t wdog_shutdown_cond;
static pthread_cond_t wdog_fire_cond;
static bool wdog_exit = false;


static inline int wd_lock (void)
{
    return (!pthread_mutex_lock(&wdog_mutex));
}

static inline void wd_unlock (void)
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

	w->w_next = NULL;
	w->w_prev = NULL;
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


static stiWDOG_ID find_nearest_expiration (void)
{
	stiWDOG_ID nearest;
	stiWDOG_ID w;
	struct timeval now;

	gettimeofday(&now, NULL);
	nearest = NULL;
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
			g_FiringTimer = NULL;
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
				nearest = NULL;
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
	g_NextItem = NULL;
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
                struct timespec ts;
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


static void wdog_init (void)
{
	g_pRoot = new stiWDOG_ID_s;
    g_pRoot->w_next = g_pRoot;
    g_pRoot->w_prev = g_pRoot;

    wdog_init_failed = estiFALSE;
}


stiHResult stiOSWdInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	
    pthread_once(&wdog_init_once, wdog_init);

    stiTESTCOND_NOLOG (!wdog_init_failed, stiRESULT_ERROR);

	pthread_mutexattr_t stAttribute;
	pthread_mutexattr_init (&stAttribute);
	pthread_mutexattr_settype (&stAttribute, PTHREAD_MUTEX_RECURSIVE_NP);

	pthread_mutex_init(&wdog_mutex, &stAttribute);
	
	nResult = pthread_cond_init(&wdog_cond, NULL);
	stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	
	nResult = pthread_cond_init(&wdog_shutdown_cond, NULL);
	stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	
	nResult = pthread_cond_init (&wdog_fire_cond, NULL);
	stiTESTCOND_NOLOG (nResult == 0, stiRESULT_ERROR);
	
	g_WDogTask = stiOSWin32TaskSpawn ("wdog", stiOS_TASK_DEFAULT_PRIORITY,
							stiOS_TASK_DEFAULT_STACK_SIZE, wdog_start, 0);
	stiTESTCOND_NOLOG (g_WDogTask, stiRESULT_ERROR);

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
	}
}


extern stiWDOG_ID stiOSWdCreate (void)
{
    stiWDOG_ID w;

    pthread_once(&wdog_init_once, wdog_init);

    if (wdog_init_failed)
	{
        return (NULL);
    }

    w = new stiWDOG_ID_s;

    return (w);
}


extern stiHResult stiOSWdDelete(stiWDOG_ID w)
{
	stiHResult eResult = stiRESULT_SUCCESS;

    if (wd_lock())
	{
		if (w && (w->w_magic == w))
		{
			if (w->w_active)
			{
				wdog_remove (w);
			}
			
			//
			// We need to block here if we are not in the same thread as the watchdog timers
			// and the timer that is being fired, if any, is the timer we want to delete.
			//
			//if (pthread_self () != wdog_thread)
			//{
			//	while (g_FiringTimer == w)
			//	{
			//		pthread_cond_wait (&wdog_fire_cond, &wdog_mutex);
			//	}
			//}
			
			w->w_magic = NULL;
			delete w;
		}
		wd_unlock ();
	}
	return (eResult);
}


extern stiHResult stiOSWdCancel(stiWDOG_ID w)
{
	stiHResult status = stiRESULT_ERROR;

    if (wd_lock())
	{
		if (w && (w->w_magic == w))
		{
			status = stiRESULT_SUCCESS;

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
		}
        wd_unlock();
    }
    return (status);
}

extern stiHResult stiOSWdStart(
	stiWDOG_ID w,
	int nDelay,				// in milliseconds
	stiFUNC_PTR pTimerCallback,
	intptr_t Parameter)
{
	return stiOSWdStart(w, nDelay, pTimerCallback, NULL, Parameter);
}

extern stiHResult stiOSWdStart (
	stiWDOG_ID w,
	int nDelay,				// in milliseconds
	stiFUNC_PTR pTimerCallback,
	stiFUNC_PTR pCancelCallback,
	intptr_t Parameter)
{
	stiHResult status = stiRESULT_ERROR;
    struct timeval expires;

    if (wd_lock())
	{
		if (w && (w->w_magic == w))
		{
			gettimeofday(&expires, NULL);

			expires.tv_usec += (nDelay % MSEC_PER_SEC) * USEC_PER_MSEC;
			expires.tv_sec += expires.tv_usec / USEC_PER_SEC + nDelay / MSEC_PER_SEC;
			expires.tv_usec %= USEC_PER_SEC;
			
			w->w_TimerCallback = pTimerCallback;
			w->w_CancelCallback = pCancelCallback;
			w->w_TimerCallback2 = NULL;
			w->w_CancelCallback2 = NULL;
			w->w_parameter1 = Parameter;
			w->w_expires = expires;
			status = stiRESULT_SUCCESS;
			if (!w->w_active)
			{
				wdog_add (w);
			}
			else
			{
				g_bListItemChanged = true;
			}
			pthread_cond_broadcast(&wdog_cond);
		}
        wd_unlock();
    }
    return (status);
}

stiHResult stiOSWdStart2(
	stiWDOG_ID w,
	int nDelay, 				// in milliseconds
	stiTIMER_CALLBACK pTimerCallback,
	intptr_t Parameter1,
	intptr_t Parameter2)
{
	return stiOSWdStart2(w, nDelay, pTimerCallback, NULL, Parameter1, Parameter2);
}

extern stiHResult stiOSWdStart2(
	stiWDOG_ID w,
	int nDelay, 				// in milliseconds
	stiTIMER_CALLBACK pTimerCallback,
	stiTIMER_CALLBACK pCancelCallback,
	intptr_t Parameter1,
	intptr_t Parameter2)
{
	stiHResult status = stiRESULT_ERROR;
    struct timeval expires;

    if (wd_lock())
	{
		if (w && (w->w_magic == w))
		{
			gettimeofday(&expires, NULL);
			
			expires.tv_usec += (nDelay % MSEC_PER_SEC) * USEC_PER_MSEC;
			expires.tv_sec += expires.tv_usec / USEC_PER_SEC + nDelay / MSEC_PER_SEC;
			expires.tv_usec %= USEC_PER_SEC;

			w->w_TimerCallback = NULL;
			w->w_CancelCallback = NULL;
			w->w_TimerCallback2 = pTimerCallback;
			w->w_CancelCallback2 = pCancelCallback;
			w->w_parameter1 = Parameter1;
			w->w_parameter2 = Parameter2;
			w->w_expires = expires;
			status = stiRESULT_SUCCESS;
			if (!w->w_active)
			{
				wdog_add (w);
			}
			else
			{
				g_bListItemChanged = true;
			}
			pthread_cond_broadcast(&wdog_cond);
		}
        wd_unlock();
    }
    return (status);
}

/* end file stiOSWin32Wd.cpp */


