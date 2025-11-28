/*!
 * \file stiOSNet.h
 *
 * \brief See stiOSNet.c
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIOSNET_H
#define STIOSNET_H

typedef enum EstiIpAddressType
{
	estiTYPE_IPV4,
	estiTYPE_IPV6
} EstiIpAddressType;

/*
 * Includes
 */
/* Include the appropriate OS specific header files */
#if defined (stiLINUX)
	#include "stiOSLinux.h"
	#include "stiOSLinuxNet.h"

#elif defined (WIN32)
	#include "stiOSWin32.h"
	#include "stiOSWin32Net.h"

#else
	/* Unknown platform */
	#error *** Error *** Unknown Operating System
#endif

/*
 * Constants
 */
/* Default receive buffer size:*/
#ifndef RTOS_TCP_RCV_SIZE_DFLT
#define RTOS_TCP_RCV_SIZE_DFLT	8192
#endif

/* Default transmit buffer size:*/
#ifndef RTOS_TCP_RCV_SIZE_DFLT
#define RTOS_TCP_RCV_SIZE_DFLT	8192
#endif

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
EstiResult stiOSConnect (
	int nFd,
	struct sockaddr * pstAddr,
	int	nLen);

EstiResult stiOSConnectWithTimeout (
	int nFd,
	struct sockaddr * pstAddr,
	int	nLen
	struct timeval * pstTimeout);

stiHResult stiOSGetHostByName (
	const char *pszHostName,
	struct hostent *pstHostEnt,
	char *pszHostBuff,
	int nBufLen,
	struct hostent **ppstHostTable);

EstiResult stiOSGetMACAddress (
	IN char acMACAddress[]);

int stiOSRead (
	int nFd,
	char * pcBuffer,
	size_t MaxBytes);

void stiOSResolvParamsGet (
	stiRESOLV_PARAMS * pResolvParams);

EstiResult stiOSResolvParamsSet (
	stiRESOLV_PARAMS * pResolvParams);

int stiOSSelect (
	int nWidth,
	fd_set * pstReadFds,
	fd_set * pstWriteFds,
	fd_set * pstExceptFds,
	struct timeval * pstTimeOut);

EstiResult stiOSSetsockopt (
	int nSock,
	int nLevel,
	int nOptName,
	void * pOptVal,
	int nOptLen);

int stiOSSocket (
	int nDomain,
	int nType,
	int nProtocol);

int stiOSWrite (
	int nFd,
	char * pcBuffer,
	size_t nBytes);


/*! \brief Initiate a connection to a socket.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
 */
EstiResult stiOSConnect (
	int nFd,						/*!< The socket descriptor. */
	struct sockaddr * pstAddr,		/*!< The addr of the socket to connect. */
	int	nLen)						/*!< The length of addr in bytes. */
{

} /* end stiOSConnect */


/*! \brief Initiate a connection to a socket with a timeout.
 *
 * This routine is basically the same as stiOSConnect, except that the user can
 * specify how long to keep trying for a connection.
 *
 * If pstTimeout is NULL, this routine acts exactly like stiOSConnect. If 
 * pstTimeout is not NULL, it tries to establish a new connection for the 
 * duration of the time specified. After that time, this routine reports a 
 * time-out error if the connection is not established.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
 */
EstiResult stiOSConnectWithTimeout (
	int nFd,						/*!< The socket descriptor. */
	struct sockaddr * pstAddr,		/*!< The addr of the socket to connect. */
	int	nLen,						/*!< The length of addr in bytes. */
	struct timeval * pstTimeout)	/*!< The max time to wait. */
{

} /* end stiOSConnectWithTimeout */


/*! \brief Query the DNS server for the IP address of a host.
 *
 * \return A pointer to the passed in hostent structure.
*/
struct hostent * stiOSGetHostByName (
	const char * pcHostName,	/*!< A ptr to the name of the host. */
	hostent * pstHostEnt, 		/*!< A structure to be filled in with the data. */
	char * pcHostBuf,			/*!< A ptr to the buffer used by hostent struct. */
	int nBufLen)				/*!< The length of the buffer pcHostBuf. */
{

} /* end stiOSGetHostByName */


/*! \brief Get the MAC address of the hardware.
 *
 * This function will retrieve the MAC address for the hardware and place it
 * in the passed in character array.  The character array MUST be at least
 * 6 characters in length being passed in or there will be a memory stomp!
 * The resultant array IS NOT a string and therefore is not NULL-terminated.
 *
 * \retval estiOK Upon uccess.
 * \retval estiERROR Upon error.
 */
EstiResult stiOSGetMACAddress (
	IN char acMACAddress[])	/*!< A buffer to place the MAC address in upon return. */
{

} /* end stiOSGetMACAddress */


/*! \brief Read from a file or device.
 *
 * \return The number of bytes read
or (-1) on error	
 */
int stiOSRead (
	int nFd,			/*!< The file descriptor to read from. */
	char * pcBuffer,	/*!< A pointer to the buffer to receive the data. */
	size_t MaxBytes)  	/*!< The max number of bytes to read into buffer. */
{

} /* end stiOSRead */


/*! \brief Get the parameters which control the resolver library.
 *
 * \return None.
 */
void stiOSResolvParamsGet (
	stiRESOLV_PARAMS * pResolvParams)	/*!< A ptr to the resolv params struct. */
{

} /* end stiOSResolvParamsGet */


/*! \brief Set the parameters which control the resolver library.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
 */
EstiResult stiOSResolvParamsSet (
	stiRESOLV_PARAMS * pResolvParams)  	/*!< A ptr to the resolv params struct. */
{

} /* end stiOSResolvParamsSet */


/*! \brief Pend on a set of file descriptors.
 *
 * \return The number of file descriptors with activity, (0) if timed out and 
 * (-1) on error.
 */
int stiOSSelect (
	int nWidth,						/*!< The number of bits to examine from 0. */
	fd_set * pstReadFds,			/*!< Read fds. */
	fd_set * pstWriteFds,			/*!< Write fds. */
	fd_set * pstExceptFds,			/*!< Exception fds. */
	struct timeval * pstTimeOut)	/*!< The maximum time to wait. */
{

} /* end stiOSSelect */


/*! \brief Set the options of a specified socket.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
 */
EstiResult stiOSSetsockopt (
	int nSock,      /*!< The socket to operate on. */
	int nLevel,
	/*!< The level. */
	int nOptName,
	/*!< The option to set. */
	void * pOptVal,
	/*!< The option value. */
	int nOptLen)
	/*!< The length of the new values (in bytes). */
{

} /* end stiOSSetsockopt */


/*! \brief Open a socket.
 *
 * \return A socket descriptor
or (-1) on error.
*/
int stiOSSocket (
	int nDomain,		/*!< The address family. */
	int nType,			/*!< SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW. */
	int nProtocol)		/*!< The socket protocol. */
{

} /* end stiOSSocket */

/*! \brief Write to a file or device.
 *
 * \return The number of bytes written
or (-1) on error.
 */
int stiOSWrite (
	int nFd,
		   /*!< The file descriptor to write to. */
	char * pcBuffer,
	/*!< The buffer containing the data to write. */
	size_t nBytes)
	 /*!< The number of bytes to be written. */
{

} /* end stiOSWrite */

#endif /* 0 */

#endif /* STIOSNET_H */
/* end file stiOSNet.h */
