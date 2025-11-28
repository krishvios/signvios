////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	stiOSSignal.c
//
//  Owner:		
//
//	Abstract:
//		O/S abstraction for signal access.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiOS.h"
#include "stiOSLinuxMutex.h"

#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//
// Constants
//

//
// Typedefs
//
typedef struct stiSIGNAL_ID_s
{
	int pipe_fd[2];
	struct stiSIGNAL_ID_s *pipe_magic_access;
	pthread_mutex_t pipe_mutex;
	int nDataCounter;
} stiSIGNAL_ID;

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//
static pthread_mutex_t pending_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * allocate and initialize a pipe data structure.
 */
static stiSIGNAL_ID *allocate_signal ()
{
	stiSIGNAL_ID *signal;

	signal = (stiSIGNAL_ID *) malloc (sizeof (*signal));

	if (signal)
	{
		signal->pipe_fd[0] = -1;
		signal->pipe_fd[1] = -1;
		signal->nDataCounter = 0;

		if (pthread_mutex_init(&signal->pipe_mutex, nullptr))
		{
			DEALLOCATE(signal);
		}
		else
		{
			signal->pipe_magic_access = signal;
		}
	}
	
	return (signal);
}

/*
 * destroy a pipe 
 */
static EstiResult signal_destroy (stiSIGNAL_ID *signal)
{
	if (signal->pipe_fd[0] >= 0)
	{
		close(signal->pipe_fd[0]);
		signal->pipe_fd[0] = -1;
	}
	if (signal->pipe_fd[1] >= 0)
	{
		close(signal->pipe_fd[1]);
		signal->pipe_fd[1] = -1;
	}

	signal->pipe_magic_access = nullptr;
	if (!pthread_mutex_destroy(&signal->pipe_mutex))
	{
		DEALLOCATE(signal);
		return (estiOK);
	}
	else
	{
		/* memory leak here because the mutex is still held */
		return (estiERROR);
	}
}

//
// Class Definitions
//

////////////////////////////////////////////////////////////////////////////////
//; stiOsSignalCreate
//
// Description: Create a signal
//
// Abstract:
//
// Returns: (EstiResult)
//		estiOK, or on error estiERROR
//
EstiResult stiOSSignalCreate (
	STI_OS_SIGNAL_TYPE *tag)
{
	stiSIGNAL_ID *pSignal;
	EstiResult eResult = estiERROR;

	pthread_mutex_lock(&pending_mutex);

	pSignal = allocate_signal();

	if (pSignal)
	{
//		int eStatus = pipe2 (pipe->pipe_fd, O_NONBLOCK);
		int eStatus = pipe (pSignal->pipe_fd);

		if (0 == eStatus)
		{
			int i;
			
			for (i = 0; i < 2; i++)
			{
				int nStatus = fcntl (pSignal->pipe_fd[i], F_GETFL) | O_NONBLOCK;
				fcntl (pSignal->pipe_fd[i], F_SETFL, nStatus);
			}
			
			eResult = estiOK;
			
			*tag = pSignal;
		}
	}

	pthread_mutex_unlock(&pending_mutex);

	return (eResult);
}

////////////////////////////////////////////////////////////////////////////////
//; stiOSSignalSet
//
// Description: write to a pipe to signal something is ready
//
// Abstract:
//
// Returns: (EstiResult)
//		estiOK, or on error estiERROR
//
EstiResult stiOSSignalSet (
	IN STI_OS_SIGNAL_TYPE fd)	// pipe descriptor
{
	EstiResult eResult = estiOK;
	static unsigned char data = 0;

	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&fd->pipe_mutex))
	{
		return (estiERROR);
	}
	
	//
	// If the data counter is zero then go ahead and
	// put a byte of data in the pipe.  If the
	// counter is greater than zero then don't bother putting
	// a byte in the pipe as one byte is enough to get a 
	// select statement to return.
	//
	if (fd->nDataCounter == 0)
	{
		//!\todo: do we need this? Was this for debugging? 
		data ++;
		if (data == 0xff)
		{	
			data = 0;
		}

		if (1 != write(fd->pipe_fd[1], &data, 1))
		{	
			eResult = estiERROR;
		}
	}
	
	fd->nDataCounter++;

	pthread_mutex_unlock(&fd->pipe_mutex);
	return (eResult); 
} // end stiOSSignalSet

EstiResult stiOSSignalSet2 (
	STI_OS_SIGNAL_TYPE fd,
	unsigned int unCount)	// pipe descriptor
{
	EstiResult eResult = estiOK;

	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&fd->pipe_mutex))
	{
		return (estiERROR);
	}

	if (unCount == 0)
	{
		//
		// If the data counter is greater than zero then go ahead and
		// read the byte of data from the pipe.
		//
		if (fd->nDataCounter > 0)
		{
			unsigned char data = 0;

			ssize_t bytes = stiOSRead(fd->pipe_fd[0], &data, 1);
			//	printf("stiOsSignalclear data = %d\n", data);
			if (bytes <= 0)
			{
				eResult = estiERROR;
			}
		}
	}
	else
	{
		//
		// If the data counter is zero then go ahead and
		// put a byte of data in the pipe.  If the
		// counter is greater than zero then don't bother putting
		// a byte in the pipe as one byte is enough to get a
		// select statement to return.
		//
		if (fd->nDataCounter == 0)
		{
			unsigned char data = 0;

			ssize_t bytes = stiOSWrite(fd->pipe_fd[1], &data, sizeof(data));
			if (bytes <= 0)
			{
				eResult = estiERROR;
			}
		}
	}

	fd->nDataCounter = unCount;

	pthread_mutex_unlock(&fd->pipe_mutex);
	return (eResult);
} // end stiOSSignalSet

////////////////////////////////////////////////////////////////////////////////
//; stiOSSignalClear
//
// Description: Read from a pipe to clear signal
//
// Abstract:
//
// Returns:
//	eOK (Success), 
//	eTIME_OUT_EXPIRED (Timeout occurred)
//	eERROR (Failure) 
//
EstiResult stiOSSignalClear (
	IN STI_OS_SIGNAL_TYPE fd)	// pipe descriptor
{
	EstiResult eResult = estiOK;
	unsigned char data;
	
	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&fd->pipe_mutex))
	{
		return (estiERROR);
	}
	
	//
	// If the data counter is greater than zero then decrement it.  If
	// it reaches zero then read the one byte from the pipe.
	//
	if (fd->nDataCounter > 0)
	{
		fd->nDataCounter--;
		
		if (fd->nDataCounter == 0)
		{
			ssize_t bytes = stiOSRead(fd->pipe_fd[0], &data, 1);
			//	printf("stiOsSignalclear data = %d\n", data);
			if (bytes <= 0)
			{
				eResult = estiERROR;
			}
		}
	}

	pthread_mutex_unlock(&fd->pipe_mutex);

	return (eResult);
} // end stiOSSignalClear

////////////////////////////////////////////////////////////////////////////////
//; stiOSSignalClose
//
// Description: Close the signal
//
// Abstract:
//
// Returns: estiOK, estiERROR
//
EstiResult stiOSSignalClose (IN STI_OS_SIGNAL_TYPE *pfd)
{
	STI_OS_SIGNAL_TYPE fd;
	
	if (!pfd)
	{
		return (estiERROR);
	}
	fd = *pfd;
	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (estiERROR);
	}
	*pfd = nullptr;
	return (signal_destroy(fd));
} // end stiOSSignalClose


////////////////////////////////////////////////////////////////////////////////
//; stiOSSignalDescriptor
//
// Description: Return a select'able descriptor corresponding to the signal pipe
//
// Abstract:
//
// Returns:
//	-1 on error, otherwise descriptor
//
extern int stiOSSignalDescriptor (IN STI_OS_SIGNAL_TYPE fd)
{
	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (-1);
	}
	return (fd->pipe_fd[0]);
}
