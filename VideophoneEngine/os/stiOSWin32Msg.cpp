/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSMsgWin32
*
*	File Name:	stiOSMsgWin32.cpp
*
*	Abstract:
*		This module contains the OS Abstraction / layering functions between the RTOS
*		OS Abstraction functions and Win32 message queue functions.
*
*******************************************************************************/

/*
 * Includes
 */

#include <stdlib.h>
#include <string.h>
#include "stiError.h"
#include "stiOS.h"
#include "stiOSWin32Msg.h"
#include "stiTrace.h"
#include <sstream>

/*
 * Constants
 */
#define MSGQ_NUM_PRIO     2

/*
 * Typedefs
 */

typedef struct stiMSG_Q_MSG_s
{
	struct stiMSG_Q_MSG_s *qmsg_next;
	uint32_t       qmsg_length;
	char            qmsg_data[0];
} stiMSG_Q_MSG;

struct stiMSG_Q_ID_s 
{
	const char 	   *pszName;
	bool            bSendEnabled;
	uint32_t       q_num_msgs;
	uint32_t       q_hwm_msg_count;
	uint32_t       q_max_msgs;
	uint32_t       q_max_msg_length;
	stiMSG_Q_ID     q_magic_access;
	stiMSG_Q_ID     q_magic_delete;
	pthread_mutex_t q_mutex;
	pthread_cond_t  q_cond_in; //Signal that there is a message in the queue ready to be received.
	stiMSG_Q_MSG   *q_head[MSGQ_NUM_PRIO];
	stiMSG_Q_MSG   *q_tail[MSGQ_NUM_PRIO];
	stiMSG_Q_MSG   *q_free;
	pthread_cond_t  q_cond_free; //Signal that there is a message in the q_free, so sending waits or does not wait as appropriate.
	SOCKET         hRead;
	SOCKET         hWrite;
	sockaddr_in    DestAddr;
};

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Locals
 */
/*
 * Function Declarations
 */
extern stiMSG_Q_ID 
stiOSMsgQCreate (
	int32_t n32MaxMsgs,
	int32_t n32MaxMsgLength)
{
	return stiOSNamedMsgQCreate ("", n32MaxMsgs, n32MaxMsgLength);
}


extern stiMSG_Q_ID 
stiOSNamedMsgQCreate (
	const char *pszName,
	int32_t n32MaxMsgs,
	int32_t n32MaxMsgLength)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiMSG_Q_ID q;
	char fail = estiTRUE;
	stiMSG_Q_MSG *msg;
	int32_t msgLength = (n32MaxMsgLength + 3) & ~3;
	int32_t varLength = n32MaxMsgs * (sizeof(stiMSG_Q_MSG) + msgLength);

	if (n32MaxMsgs <= 0) {
		return (0);
	}
	//
	// Allocate memory for q (stiMSG_Q_ID) + n32MaxMsgs (message space will be accessed using &q[1])
	//
	q = (stiMSG_Q_ID) malloc (sizeof (*q) + varLength);

	pthread_win32_process_attach_np ();

	if (q != NULL)
	{
		pthread_mutexattr_t stAttribute;
		pthread_mutexattr_init (&stAttribute);
		pthread_mutexattr_settype (&stAttribute, PTHREAD_MUTEX_RECURSIVE_NP);

		/* Initialize the mutex as a recursive mutex  
		WHY?!?! I don't see how it needs to be recursive. Commented out. */   //...
		int nMutexInitResult = pthread_mutex_init (&q->q_mutex, NULL); //&stAttribute  //...

		pthread_mutexattr_destroy (&stAttribute);

		if (!nMutexInitResult &&
			!pthread_cond_init(&q->q_cond_in, NULL) &&
			!pthread_cond_init(&q->q_cond_free, NULL))
		{
			//if (q &&
			//  !pthread_mutex_init(&q->q_mutex, NULL) &&
			//	!pthread_cond_init(&q->q_cond, NULL)) {
			int prio;
			int32_t msgNo;

			q->pszName = pszName;

			q->bSendEnabled = true;
			q->q_hwm_msg_count = 0;
			q->q_max_msgs = n32MaxMsgs;
			q->q_max_msg_length = n32MaxMsgLength;
			q->q_magic_access = q;
			q->q_magic_delete = q;
			q->q_num_msgs = 0;
			q->q_free = NULL;
			for (prio = 0; prio < MSGQ_NUM_PRIO; prio++) {
				q->q_head[prio] = q->q_tail[prio] = NULL;
			}
			// Already allocated memory for q (stiMSG_Q_ID) + n32MaxMsgs (message space will be accessed using &q[1])
			// Get pointer to 1st message in our block of messages.
			//
			msg = (stiMSG_Q_MSG *)&q[1];
			for (msgNo = 0; msgNo < n32MaxMsgs; msgNo++) {
				//
				// Point message qmsg_next to q's stiMSG_Q_MSG q_free (initially = 0)
				// When done, each message's qmsg_next will point to the previous message in memory block.
				// The qmsg_next of the first message in memory block will point to 0 (no next message)
				//
				msg->qmsg_next = q->q_free;
				//
				// Update q's stiMSG_Q_MSG q_free to point to msg (when done, q_free will point to last message in memory block)
				//
				q->q_free = msg;
				//
				// Get pointer to next message in our block of messages (by finding memory at end of current message).
				//
				msg = (stiMSG_Q_MSG *)&msg->qmsg_data[msgLength];
			}
			pthread_cond_broadcast(&q->q_cond_free);

			WSADATA wsaData;
			//-----------------------------------------------
			// Initialize Winsock
			WSAStartup(MAKEWORD(2,2), &wsaData);

			//-----------------------------------------------
			// Create a sender and receiver socket to send/receive datagrams
			q->hRead = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			q->hWrite = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			//-------------------------
			// Set the socket I/O mode: In this case FIONBIO
			// enables or disables the blocking mode for the 
			// socket based on the numerical value of iMode.
			// If ulMode = 0, blocking is enabled; 
			// If ulMode != 0, non-blocking mode is enabled.
			u_long ulMode = 1;
			ioctlsocket(q->hRead, FIONBIO, &ulMode);

			//-----------------------------------------------
			// Bind the receive socket to any address and any port.
			sockaddr_in RecvAddr;
			RecvAddr.sin_family = AF_INET;
			RecvAddr.sin_port = htons(0);
			RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

			int nRet = bind(q->hRead, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
			if (nRet != -1)
			{
				// We have our socket bound.
				sockaddr_in name;
				int namelen = sizeof(name);
				getsockname (q->hRead, (struct sockaddr *)&name, &namelen);
				q->DestAddr.sin_family = AF_INET;
				q->DestAddr.sin_port = name.sin_port;
				q->DestAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

				fail = estiFALSE;
			}
		}
		if (fail) {
			DEALLOCATE(q);
		}
	}
	//else q is NULL

	return (q);
}


extern EstiResult 
stiOSMsgQDelete (stiMSG_Q_ID msgQId)
{
	EstiResult eStatus = estiOK;

	if (!msgQId || (msgQId->q_magic_delete != msgQId)) {
		return (estiERROR);
	}

	msgQId->q_magic_access = NULL;
	closesocket (msgQId->hRead); 
	closesocket (msgQId->hWrite);
	if (pthread_mutex_destroy(&msgQId->q_mutex)) {
		/* There is a thread holding the mutex */
		eStatus = estiERROR;
	} else if (pthread_cond_destroy(&msgQId->q_cond_in)) {
		/* There is a thread holding the condition */
		eStatus = estiERROR;
	} else if (pthread_cond_destroy(&msgQId->q_cond_free)) {
		/* There is a thread holding the condition */
		eStatus = estiERROR;
	} else {
		msgQId->q_magic_delete = NULL;
		DEALLOCATE(msgQId);
	}
	return (eStatus);
}

extern int32_t 
stiOSMsgQNumMsgs (stiMSG_Q_ID msgQId)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId)) {
		return (-1);
	}
	int32_t count;
	pthread_mutex_lock(&msgQId->q_mutex);
	count = msgQId->q_num_msgs;
	pthread_mutex_unlock(&msgQId->q_mutex);
	return (count);
}


EstiResult stiOSMsgQReceiveNoLock (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32MaxNBytes,
	int32_t n32Timeout)
{
	struct timespec wait_time;
	struct timeval now;
	int cond_errno;

	if (!msgQId->q_num_msgs)
	{
		if (!n32Timeout) { /* No message and no wait */
			return (estiTIME_OUT_EXPIRED);
		} else if (n32Timeout > 0) {
			/* Caller wants to wait a maximum amount of time. */
			gettimeofday(&now, NULL);

			now.tv_usec += n32Timeout * USEC_PER_MSEC;

			now.tv_sec += now.tv_usec / USEC_PER_SEC;
			now.tv_usec %= USEC_PER_SEC;

			//
			// Convert to seconds and nanoseconds.
			//
			wait_time.tv_sec = now.tv_sec;
			wait_time.tv_nsec = now.tv_usec * NSEC_PER_USEC;
		}
	}

	do {
		if (msgQId->q_num_msgs) {
			int prio;
			for (prio = 0; prio < MSGQ_NUM_PRIO; prio++) { 
				//
				// Set pointer to message at q_head (q_head points to 1st (or next) message to process).
				//
				stiMSG_Q_MSG *msg = msgQId->q_head[prio];
				if (msg) {
					if (msg->qmsg_length > un32MaxNBytes) {
						/* Passed buffer is too small */
						return (estiERROR);
					}
					//
					// Copy message we're using/de-queuing from active queue into buffer.
					//
					memcpy(pcBuffer, msg->qmsg_data, msg->qmsg_length);
					//
					// Move our current message's qmsg_next (next message to use) into q_head
					//
					msgQId->q_head[prio] = msg->qmsg_next;
					if (!msgQId->q_head[prio]) {
						//
						// There is NO next message - Set q_tail to 0 to show no next message.
						//
						msgQId->q_tail[prio] = 0;
					}
					//
					// Point qmsg_next (of message we are using/de-queuing) to q_free (last unused/free message in memory block)
					// (Link message we are using/de-queuing back into unused/free message block)
					//
					msg->qmsg_next = msgQId->q_free;
					//
					// Point q_free to current message we are de-queuing (last unused/free message in memory block).
					//
					msgQId->q_free = msg;
					msgQId->q_num_msgs--;
					//
					// The pthread_cond_broadcast() function shall unblock all threads currently blocked on the specified condition variable cond.
					//
					pthread_cond_broadcast(&msgQId->q_cond_free);
					//does not matter - Do this later, not now.

					return (estiOK);
				}
			}
		}
		//
		// Block until q_cond is signaled
		//
		if (n32Timeout > 0)
		{
			cond_errno =  pthread_cond_timedwait(&msgQId->q_cond_in,
				&msgQId->q_mutex,
				&wait_time);
		}
		else
		{
			cond_errno = pthread_cond_wait(&msgQId->q_cond_in, &msgQId->q_mutex);
		}

	} while ((cond_errno != ETIMEDOUT) && (cond_errno != EAGAIN));

	return estiTIME_OUT_EXPIRED;
}


extern EstiResult
stiOSMsgQReceive (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32MaxNBytes,
	int32_t n32Timeout)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId) || !pcBuffer) {
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex)) {  /* Lock failed */
		return (estiERROR);
	}

	EstiResult eResult = stiOSMsgQReceiveNoLock (msgQId, pcBuffer, un32MaxNBytes, n32Timeout);
	//
	// Read everything on the socket buffer.  If there are still messages to receive we will next write
	// a new byte into the socket buffer.  This new write into the socket seems to be necessary to make
	// the RV seli work correctly.

	/* 
	* Try to recover from any previous errors by eating everything
	* in the fifo if the queue depth is now zero.
	*/
	//Clear out the socket. That will kill the signal that got us here. 
	//if (msgQId->q_num_msgs == 0)
	char rdr_buffer[128];
	int bytes = 1;
	int rdr_bytesRecvd = 0;
	while (bytes > 0)
	{
		bytes = recvfrom (msgQId->hRead, rdr_buffer + rdr_bytesRecvd, sizeof(rdr_buffer), 0, NULL, 0);
		if (bytes > 0)
		{
			rdr_bytesRecvd += bytes;
			break; //Got what we needed
		}
		else if (bytes < 0)
		{ //we won the race. Where are the bytes?
			int rc;
			fd_set readFs, writeFs, exceptFs;
			struct timeval now;
				
			now.tv_usec = 10 * USEC_PER_MSEC;

			now.tv_sec = 0;
			FD_ZERO(&readFs); FD_ZERO(&writeFs); FD_ZERO(&exceptFs);
			FD_SET(msgQId->hRead, &readFs);
			rc = select(0, &readFs, &writeFs, &exceptFs, &now);
			if (rc > 0)
			{ //number of sockets
				bytes = 1; //loop around and try again
				continue;
			}
			else if (rc == 0)
			{ //time out
				//failure. Where are the bytes??
			}
		}
	}
	//
	// Write a byte into the socket buffer to triggger RV seli that there is a message to process.
	//
	if (msgQId->q_num_msgs > 0)
		//There's more messages, signal to make sure they get processed.
	{
		std::stringstream ss;
		ss << msgQId << "-ENDRECV";
		std::string message = ss.str ();
		int nBytesWritten = sendto(msgQId->hWrite, 
			message.c_str(), 
			message.size(), 
			0, 
			(SOCKADDR *) &(msgQId->DestAddr), 
			sizeof(sockaddr_in));
	}
	else { //no messages
		//TODO put in code to make sure that select() really does not think it's signaled. It should not be signaled at this point.
		int rc;
		fd_set readFs, writeFs, exceptFs;
		struct timeval now;
			
		now.tv_usec = 1 * USEC_PER_MSEC;

		now.tv_sec = 0;
		
		FD_ZERO(&readFs); FD_ZERO(&writeFs); FD_ZERO(&exceptFs);
		FD_SET(msgQId->hRead, &readFs);
		rc = select(0, &readFs, &writeFs, &exceptFs, &now);
		if (rc > 0)
		{ //number of sockets
			//THIS IS A PROBLEM
			char buffer[64];
			int bytes = 0;
			do
			{
				bytes = recvfrom(msgQId->hRead, buffer, sizeof(buffer), 0, NULL, 0);
			} 
			while (bytes > 0);
		}
		else if (rc == 0)
		{ //time out
			//this is what it is supposed to do!
		}
		else { //error
			int err = WSAGetLastError();
		}
	}

	int retMutex = pthread_mutex_unlock(&msgQId->q_mutex);
	return (eResult);
}


static void AddMessage (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
    uint32_t priority)
{
	//
	//
	// Initially, q_free will point to last message in memory block and
	// each message's qmsg_next will point to the previous message in memory block.
	// The qmsg_next of the first message in memory block will point to 0 (no next message)
	//
	// Get pointer to last free/unused message in memory block
	//
	stiMSG_Q_MSG *msg = msgQId->q_free;
	msgQId->q_num_msgs++;
	if (msgQId->q_num_msgs > msgQId->q_max_msgs)//msgQId->q_hwm_msg_count)
	{
		msgQId->q_hwm_msg_count = msgQId->q_num_msgs;

		stiDEBUG_TOOL (g_stiMessageQueueDebug,
			stiTrace ("Maximum number of messages in queue %s (%p): %d out of %d\n", msgQId->pszName,
			msgQId, msgQId->q_hwm_msg_count, msgQId->q_max_msgs);
		);
	}
	//
	// Update q_free to point to NEW NEXT last free/unused message in memory block
	//
	msgQId->q_free = msg->qmsg_next;
	//
	// Initially, q_free will point to last message in memory block and
	// each message's qmsg_next will point to the previous message in memory block.
	// The qmsg_next of the first message in memory block will point to 0 (no next message)
	//
	// Set qmsg_next (of the new message we are adding to the active queue
	// (and removing from the inactive/free queue)) to 0 (to show no next message in active queue.)
	//
	msg->qmsg_next = NULL;
	msg->qmsg_length = un32Bytes;
	memcpy(msg->qmsg_data, pcBuffer, un32Bytes);
	if (msgQId->q_tail[priority])
	{
		//
		// There IS a message in q_tail.
		// Set q_tail's NEXT message to point to the new message we are adding to the active queue
		//
		msgQId->q_tail[priority]->qmsg_next = msg;
	}
	else
	{
		//
		// There is NOT a message in q_tail.
		// Set q_head's message to point to the new message we are adding to the active queue (1st message to process)
		//
		msgQId->q_head[priority] = msg;
	}
	//
	// Set q_tail to point to the new message we are adding to the active queue
	//
	msgQId->q_tail[priority] = msg;

	/* Successful enqueue */
	//
	// The pthread_cond_broadcast() function shall unblock all threads currently blocked on the specified condition variable cond.
	//
	pthread_cond_broadcast(&msgQId->q_cond_in);
}


EstiResult stiOSMsgQSendNoLock (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout,
	uint32_t priority)
{
	struct timespec wait_time;
	struct timeval now;
	int cond_errno;
	EstiResult eResult = estiOK;

	if (!msgQId->bSendEnabled)
	{
		eResult = estiERROR;
	}
	else
	{
		if (!msgQId->q_free)
		{
			if (n32Timeout > 0)  /* User wants to wait a while */
			{
				gettimeofday(&now, NULL);

				now.tv_usec += n32Timeout * USEC_PER_MSEC;

				now.tv_sec += now.tv_usec / USEC_PER_SEC;
				now.tv_usec %= USEC_PER_SEC;

				//
				// Convert to seconds and nanoseconds.
				//
				wait_time.tv_sec = now.tv_sec;
				wait_time.tv_nsec = now.tv_usec * NSEC_PER_USEC;

				for (;;)
				{
					//
					// Block until q_cond is signaled
					//
					cond_errno = pthread_cond_timedwait(&msgQId->q_cond_free,
						&msgQId->q_mutex,
						&wait_time);

					if (cond_errno == 0)
					{
						if (msgQId->q_free)
						{
							AddMessage (msgQId, pcBuffer, un32Bytes, priority);

							break;
						}
					}
					else if (cond_errno == ETIMEDOUT)
					{
						stiTrace ("*** Message Queue %s Send failed due to timeout.  Max: %d\n", msgQId->pszName, msgQId->q_max_msg_length);
						eResult = estiERROR;
						break;
					}
				}
			}
			else if (n32Timeout < 0)
			{
				//
				// Block until q_cond is signaled
				//
				/* Wait forever */
				pthread_cond_wait(&msgQId->q_cond_free, &msgQId->q_mutex);

				AddMessage (msgQId, pcBuffer, un32Bytes, priority);
			}
			else
			{
				eResult = estiERROR;
			}
		}
		else
		{
			AddMessage (msgQId, pcBuffer, un32Bytes, priority);
		}
	}

	return eResult;
}


void stiOSWin32MsgQPurgeNoLock (
	stiMSG_Q_ID msgQId)
{
	int prio;

	for (prio = 0; prio < MSGQ_NUM_PRIO; prio++)
	{ 
		stiMSG_Q_MSG *msg;
		for (; (msg = msgQId->q_head[prio]); )
		{
			msgQId->q_head[prio] = msg->qmsg_next;
			if (!msgQId->q_head[prio])
			{
				msgQId->q_tail[prio] = 0;
			}
			msg->qmsg_next = msgQId->q_free;
			msgQId->q_free = msg;
			msgQId->q_num_msgs--;
		}
	}
	pthread_cond_broadcast(&msgQId->q_cond_free);
}


static EstiResult 
send_message (
			  stiMSG_Q_ID msgQId,
			  char * pcBuffer,
			  uint32_t un32Bytes,
			  int32_t n32Timeout,
			  uint32_t priority )
{
	if (!msgQId || (msgQId->q_magic_access != msgQId)) {
		return (estiERROR);
	}
	if (msgQId->q_max_msg_length < un32Bytes) {
		return (estiERROR);
	}

	if (priority >= MSGQ_NUM_PRIO) {
		/* Illegal priority */
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex)) {  /* Lock failed */
		return (estiERROR);
	}

	EstiResult eResult = stiOSMsgQSendNoLock(msgQId, pcBuffer, un32Bytes, n32Timeout, priority);

	if (eResult == estiOK)
	{
		if (msgQId->q_num_msgs == 1)
			//Only one signal at a time, so only send a byte if no previous signal is active
		{
			std::stringstream ss;
			ss << msgQId << "-REGSEND";
			std::string message = ss.str ();

			int nBytesWritten = sendto(msgQId->hWrite, 
				message.c_str(), 
				message.size(), 
				0, 
				(SOCKADDR *) &(msgQId->DestAddr), 
				sizeof(sockaddr_in));

			if(SOCKET_ERROR == nBytesWritten)
			{
				stiOSWin32MsgQPurgeNoLock(msgQId);
				eResult = estiERROR;
			}
		}
	}
	pthread_mutex_unlock(&msgQId->q_mutex);
	return (eResult);
}

extern EstiResult 
stiOSMsgQSend (
			   stiMSG_Q_ID msgQId,
			   char * pcBuffer,
			   uint32_t un32Bytes,
			   int32_t n32Timeout)
{
	return (send_message(msgQId, pcBuffer, un32Bytes, n32Timeout, 1));
}

extern EstiResult 
stiOSMsgQUrgentSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout)
{
	return (send_message(msgQId, pcBuffer, un32Bytes, n32Timeout, 0));
}


extern EstiResult
stiOSMsgQForceSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId)) {
		return (estiERROR);
	}
	if (msgQId->q_max_msg_length < un32Bytes) {
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex)) {  /* Lock failed */
		return (estiERROR);
	}

	stiOSWin32MsgQPurgeNoLock (msgQId);

	EstiResult eResult = stiOSMsgQSendNoLock(msgQId, pcBuffer, un32Bytes, -1, 1);

	if (eResult == estiOK)
	{
		if (msgQId->q_num_msgs == 1)
		{
			std::stringstream ss;
			ss << msgQId << "-FORSEND";
			std::string message = ss.str ();
			int nBytesWritten = sendto(msgQId->hWrite, 
				message.c_str(), 
				message.size(), 
				0, 
				(SOCKADDR *) &(msgQId->DestAddr), 
				sizeof(sockaddr_in));

			if(SOCKET_ERROR == nBytesWritten)
			{
				stiOSWin32MsgQPurgeNoLock(msgQId);
				eResult = estiERROR;
			}
		}
	}

	pthread_mutex_unlock(&msgQId->q_mutex);
	return (eResult);
}

/*
* This is not really a public function, but is needed by the linx
* pipe implementation.
*/
extern EstiResult
stiOSWin32MsgQPurge (stiMSG_Q_ID msgQId)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId)) {
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex)) {  /* Lock failed */
		return (estiERROR);
	}

	stiOSWin32MsgQPurgeNoLock (msgQId);
	//
	// The pthread_cond_broadcast() function shall unblock all threads currently blocked on the specified condition variable cond.
	//
	//pthread_cond_broadcast(&msgQId->q_cond_free);
	pthread_mutex_unlock(&msgQId->q_mutex);
	return (estiOK);
}


EstiResult stiOSMsgQLock (
	stiMSG_Q_ID msgQId)
{
	pthread_mutex_lock(&msgQId->q_mutex);

	return estiOK;
}


EstiResult stiOSMsgQUnlock (
	stiMSG_Q_ID msgQId)
{
	pthread_mutex_unlock(&msgQId->q_mutex);

	return estiOK;
}

////////////////////////////////////////////////////////////////////////////////
//; stiOsPipeDescriptor
//
// Description: Return a select'able descriptor corresponding to the pipe
//
// Abstract:
//
// Returns:
//	-1 on error, otherwise descriptor
//
int stiOsMsgQDescriptor (
	stiMSG_Q_ID msgQId)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId))
	{
		return (-1);
	}
	return ((int)msgQId->hRead);
}

stiHResult stiOSMsqQSendEnable (
	stiMSG_Q_ID msgQId,
	bool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (msgQId && (msgQId->q_magic_access == msgQId), stiRESULT_ERROR);

	stiOSMsgQLock (msgQId);

	msgQId->bSendEnabled = bEnable;

	stiOSMsgQUnlock (msgQId);

STI_BAIL:

	return (hResult);
}

/* end file stiOSMsgWin32.cpp */
