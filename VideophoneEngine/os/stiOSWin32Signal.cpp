////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	stiWin32Signal.c
//
//  Owner:		
//
//	Abstract:
//		Win32 O/S abstraction for signal access.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiOS.h"
#include "stiOSWin32Mutex.h"
#include "stiOSWin32Signal.h"

#include <mutex>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

//
// Constants
//

//
// Typedefs
//
typedef struct stiSIGNAL_ID_s
{
//    int     pipe_fd[2];
	SOCKET         hRead;
	SOCKET         hWrite;
	sockaddr_in    DestAddr;
    struct stiSIGNAL_ID_s *pipe_magic_access;
    pthread_mutex_t pipe_mutex;
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
static std::mutex *signalLock = nullptr;

/*
 * allocate and initialize a pipe data structure.
 */
static stiSIGNAL_ID *allocate_signal ()
{
    stiSIGNAL_ID *signal;

    signal = (stiSIGNAL_ID *) malloc (sizeof (*signal));

    if (signal)
	{
		signal->hRead = (SOCKET)INVALID_HANDLE_VALUE;
		signal->hWrite = (SOCKET)INVALID_HANDLE_VALUE;
		//signal->pipe_fd[0] = (int)INVALID_HANDLE_VALUE;
		//signal->pipe_fd[1] = (int)INVALID_HANDLE_VALUE;

		if (pthread_mutex_init(&signal->pipe_mutex, NULL))
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
	if (signal->hRead != (SOCKET)INVALID_HANDLE_VALUE)
	{
		closesocket (signal->hRead); 
		signal->hRead = (SOCKET)INVALID_HANDLE_VALUE;
	}
	if (signal->hWrite != (SOCKET)INVALID_HANDLE_VALUE)
	{
		closesocket (signal->hWrite);
		signal->hWrite = (SOCKET)INVALID_HANDLE_VALUE;
    }

	//if (signal->pipe_fd[0] != (int)INVALID_HANDLE_VALUE)
	//{
	//	CloseHandle((HANDLE)signal->pipe_fd[0]);
	//	signal->pipe_fd[0] = (int)INVALID_HANDLE_VALUE;
	//}
	//if (signal->pipe_fd[1] != (int)INVALID_HANDLE_VALUE)
	//{
	//	CloseHandle((HANDLE)signal->pipe_fd[1]);
	//	signal->pipe_fd[1] = (int)INVALID_HANDLE_VALUE;
 //   }

    signal->pipe_magic_access = 0;
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

	if (signalLock == nullptr)
	{
		signalLock = new std::mutex ();
	}

	std::lock_guard<std::mutex> lock(*signalLock);

	pSignal = allocate_signal();
  
	if (pSignal)
	{
		WSADATA wsaData;
		//-----------------------------------------------
		// Initialize Winsock
		WSAStartup(MAKEWORD(2,2), &wsaData);

		//-----------------------------------------------
		// Create a sender and receiver socket to send/receive datagrams
		pSignal->hRead = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		pSignal->hWrite = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		//-------------------------
		// Set the socket I/O mode: In this case FIONBIO
		// enables or disables the blocking mode for the 
		// socket based on the numerical value of iMode.
		// If ulMode = 0, blocking is enabled; 
		// If ulMode != 0, non-blocking mode is enabled.
		u_long ulMode = 1;
		ioctlsocket(pSignal->hRead, FIONBIO, &ulMode);

		//-----------------------------------------------
		// Bind the receive socket to any address and any port.
		sockaddr_in RecvAddr;
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(0);
		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		int nRet = bind(pSignal->hRead, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
		if (nRet != -1)
		{
			// We have our socket bound.
			sockaddr_in name;
			int namelen = sizeof(name);
			getsockname (pSignal->hRead, (struct sockaddr *)&name, &namelen);
			pSignal->DestAddr.sin_family = AF_INET;
			pSignal->DestAddr.sin_port = name.sin_port;
			pSignal->DestAddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

			eResult = estiOK;
			*tag = pSignal;
		}

		//// Create the pipe
		//BOOL bSuccess = CreatePipe((PHANDLE)&pSignal->pipe_fd[0], (PHANDLE)&pSignal->pipe_fd[1], NULL, 0);

		//if (bSuccess)
		//{
		//	int i;
		//	
		//	for (i = 0; i < 2; i++)
		//	{
		//		// Set pipe handles as non-blocking
		//		DWORD dwState = 0;
		//		GetNamedPipeHandleState((HANDLE)pSignal->pipe_fd[i], &dwState, NULL, NULL, NULL, NULL, 0);

		//		dwState = dwState | PIPE_NOWAIT;
		//		SetNamedPipeHandleState((HANDLE)pSignal->pipe_fd[i], &dwState, NULL, NULL); 
		//	}
		//	
		//	eResult = estiOK;
		//	*tag = pSignal;
		//}
    }

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

	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&fd->pipe_mutex))
	{
		return (estiERROR);
	}
	
	char buffer[1];
	buffer[0]=0; // This value is irrelevant. We just need to write a byte to the socket.
	int nBytesWritten = sendto(fd->hWrite, 
		buffer, 
		sizeof(buffer), 
		0, // no flags
		(SOCKADDR *) &(fd->DestAddr), 
		sizeof(sockaddr_in));
	if (nBytesWritten != sizeof(buffer))
	{	
		eResult = estiERROR;
	}

	//DWORD dwBytesWritten;
	//if (TRUE != WriteFile((HANDLE)fd->pipe_fd[1], &data, 1, &dwBytesWritten, NULL)) 
	//{	
	//	eResult = estiERROR;
	//}

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
	//unsigned char data;
	
	if (!fd || (fd->pipe_magic_access != fd))
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&fd->pipe_mutex))
	{
		return (estiERROR);
	}
	
	char rdr_buffer[1];
	int bytes = 0;
	bytes = recvfrom (fd->hRead, rdr_buffer, sizeof(rdr_buffer), 0, NULL, 0);
	if (bytes != 1)
	{
		eResult = estiERROR;
	}

	//DWORD dwBytesRead = 0;
	//ReadFile((HANDLE)fd->pipe_fd[0], &data, 1, &dwBytesRead, NULL);

	//if (dwBytesRead <= 0)
	//{
	//	eResult = estiERROR;
	//}
		
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
	*pfd = NULL;
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
	return (fd->hRead);
}
