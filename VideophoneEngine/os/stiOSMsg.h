/*!
 * \file stiOSMsg.h
 *
 * \brief See stiOSMsg.c
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef STIOSMSG_H
#define STIOSMSG_H

/*
 * Includes
 */

/* Include the appropriate OS specific header files */
#if defined (stiLINUX)
	#include "stiOSLinux.h"
	#include "stiOSLinuxMsg.h"

#elif defined (WIN32)
	#include "stiOSWin32.h"
	#include "stiOSWin32Msg.h"

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
stiMSG_Q_ID stiOSMsgQCreate (
	int32_t n32MaxMsgs,
	int32_t n32MaxMsgLength);

EstiResult stiOSMsgQDelete (
	stiMSG_Q_ID msgQId);

int32_t stiOSMsgQNumMsgs (
	stiMSG_Q_ID msgQId);

EstiResult stiOSMsgQReceive (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32MaxNBytes,
	int32_t n32Timeout);

EstiResult stiOSMsgQSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout);
 
EstiResult stiOSMsgQUrgentSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout);
 

/*! \brief Create and initialize a message queue. 
 *
 * \return The ID of the message queue just created.
 */
stiMSG_Q_ID stiOSMsgQCreate (
	int32_t n32MaxMsgs,		/*!< Max messages that can be queued. */
	int32_t n32MaxMsgLength)	/*!< Max bytes in a message. */
{

}


/*! \brief Delete a message queue. 
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
*/
EstiResult stiOSMsgQDelete (
	stiMSG_Q_ID msgQId)			/*!< Message queue to delete. */
{

}


/*! \brief Get the number of messages queued to a message queue.
 *
 * \return The number of messages queued
or (-1) on ERROR.
 */
int32_t stiOSMsgQNumMsgs (
	stiMSG_Q_ID msgQId)		/*!< The message queue to examine */
{

}


/*! \brief Receive a message from a message queue.
 *
 * \return The number of bytes copied to buffer
or (-1) on ERROR.
*/
EstiResult stiOSMsgQReceive (
	stiMSG_Q_ID msgQId,		/*!< The message queue from which to receive. */
	char * pcBuffer,		/*!< The buffer to receive message. */
	uint32_t un32MaxNBytes,/*!< The length of buffer. */
	int32_t n32Timeout)	/*!< How many ticks to wait. */
{

}


/*! \brief Send a message to a message queue.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
*/
EstiResult stiOSMsgQSend (
	stiMSG_Q_ID msgQId,		/*!< The message queue on which to send. */
	char * pcBuffer,		/*!< The message to send. */
	uint32_t un32Bytes,	/*!< The length of message. */
	int32_t n32Timeout)	/*!< How many ticks to wait. */
{

}

/*! \brief Send an urgent message to a message queue.
 *
 * Sends a message to a message queue at urgent priority. 
 * NOTE: If urgent messages are not supported on the base RTOS, the message is 
 * sent at normal priority.
 *
 * \retval estiOK Upon success.
 * \retval estiERROR Upon failure.
*/
EstiResult stiOSMsgQUrgentSend (
	stiMSG_Q_ID msgQId,		/*!< The message queue on which to send. */
	char * pcBuffer,		/*!< The message to send. */
	uint32_t un32Bytes,	/*!< The length of message. */
	int32_t n32Timeout)	/*!< How many ticks to wait. */
{

}

#endif /* 0 */

#endif /* STIOSMSG_H */
/* end file stiOSMsg.h */
