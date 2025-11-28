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
#ifndef WIN32
	#include <sys/time.h>
	#include <sys/errno.h>
#endif
#include <cstring>

#include "stiOS.h"
#ifndef WIN32
	#include "stiOSLinux.h"
#endif
#include "stiOSLinuxMsg.h"
#include "stiTrace.h"
#include <fcntl.h>

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
	bool			bSendEnabled;
	uint32_t       q_num_msgs;
	uint32_t		q_hwm_msg_count;
	uint32_t       q_max_msgs;
	uint32_t       q_max_msg_length;
	stiMSG_Q_ID     q_magic_access;
	stiMSG_Q_ID     q_magic_delete;
	pthread_mutex_t q_mutex;
	pthread_cond_t  q_cond;
	stiMSG_Q_MSG   *q_head[MSGQ_NUM_PRIO];
	stiMSG_Q_MSG   *q_tail[MSGQ_NUM_PRIO];
	stiMSG_Q_MSG   *q_free;
	int             pipe_fd[2];
	int             last_msg;
};

/*
 * Forward Declarations
 */
#include "CstiEvent.h"
#include "CstiUniversalEvent.h"
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
	stiMSG_Q_ID q;
	char fail = estiTRUE;
	stiMSG_Q_MSG *msg;
	int32_t msgLength = (n32MaxMsgLength + 3) & ~3;
	int32_t varLength = n32MaxMsgs * (sizeof(stiMSG_Q_MSG) + msgLength);

	if (n32MaxMsgs <= 0)
	{
		return (nullptr);
	}

	q = (stiMSG_Q_ID) malloc (sizeof (*q) + varLength);

	if (q &&
		!pthread_mutex_init(&q->q_mutex, nullptr) &&
		!pthread_cond_init(&q->q_cond, nullptr))
	{
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
		q->q_free = nullptr;
		q->last_msg = 0;
		for (prio = 0; prio < MSGQ_NUM_PRIO; prio++)
		{
			q->q_head[prio] = q->q_tail[prio] = nullptr;
		}
		msg = (stiMSG_Q_MSG *)&q[1];
		for (msgNo = 0; msgNo < n32MaxMsgs; msgNo++)
		{
			msg->qmsg_next = q->q_free;
			q->q_free = msg;
			msg = (stiMSG_Q_MSG *)&msg->qmsg_data[msgLength];
		}
		
		//
		// Create pipe for signaling.
		//
		int eStatus = pipe (q->pipe_fd);

		if (0 == eStatus)
		{
			for (int i = 0; i < 2; i++)
			{
				int nStatus = fcntl (q->pipe_fd[i], F_GETFL) | O_NONBLOCK;
				fcntl (q->pipe_fd[i], F_SETFL, nStatus);
			}
		}
		fail = estiFALSE;
	}
	if (fail)
	{
		DEALLOCATE(q);
	}

	return (q);
}


extern EstiResult 
stiOSMsgQDelete (stiMSG_Q_ID msgQId)
{
	EstiResult eStatus = estiOK;

	if (!msgQId || (msgQId->q_magic_delete != msgQId))
	{
		return (estiERROR);
	}

	msgQId->q_magic_access = nullptr;
	
	if (msgQId->pipe_fd[0] >= 0)
	{
		close(msgQId->pipe_fd[0]);
		msgQId->pipe_fd[0] = -1;
	}
	if (msgQId->pipe_fd[1] >= 0)
	{
		close(msgQId->pipe_fd[1]);
		msgQId->pipe_fd[1] = -1;
	}

	if (pthread_mutex_destroy(&msgQId->q_mutex))
	{
		/* There is a thread holding the mutex */
		eStatus = estiERROR;
	}
	else if (pthread_cond_destroy(&msgQId->q_cond))
	{
		/* There is a thread holding the condition */
		eStatus = estiERROR;
	}
	else
	{
		msgQId->q_magic_delete = nullptr;
		DEALLOCATE(msgQId);
	}
	return (eStatus);
}

extern int32_t 
stiOSMsgQNumMsgs (stiMSG_Q_ID msgQId)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId))
	{
		return (-1);
	}
	return (msgQId->q_num_msgs);
}


EstiResult stiOSMsgQReceiveNoLock (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32MaxNBytes,
	int32_t n32Timeout)
{
	struct timespec wait_time{};
	struct timeval now{};
	int cond_errno;
	
	if (!msgQId->q_num_msgs)
	{
		if (!n32Timeout)
		{ /* No message and no wait */
			return (estiTIME_OUT_EXPIRED);
		}
		else if (n32Timeout > 0)
		{
			/* Caller wants to wait a maximum amount of time. */
			gettimeofday(&now, nullptr);

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

	do
	{
		if (msgQId->q_num_msgs)
		{
			int prio;
			for (prio = 0; prio < MSGQ_NUM_PRIO; prio++)
			{
				stiMSG_Q_MSG *msg = msgQId->q_head[prio];
				if (msg)
				{
					if (msg->qmsg_length > un32MaxNBytes)
					{
						/* Passed buffer is too small */
						return (estiERROR);
					}
					memcpy(pcBuffer, msg->qmsg_data, msg->qmsg_length);
					auto pEvent = (CstiEvent *)pcBuffer;
					if (pEvent->EventGet() == 4)
					{
						msgQId->last_msg = ((CstiUniversalEvent *)pEvent)->ParamGet(0);
					}
					else
					{
						msgQId->last_msg = pEvent->EventGet();
					}
					msgQId->q_head[prio] = msg->qmsg_next;
					if (!msgQId->q_head[prio])
					{
						msgQId->q_tail[prio] = nullptr;
					}
					msg->qmsg_next = msgQId->q_free;
					msgQId->q_free = msg;
					msgQId->q_num_msgs--;
					pthread_cond_broadcast(&msgQId->q_cond);

					return (estiOK);
				}
			}
		}

		if (n32Timeout > 0)
		{
			cond_errno =  pthread_cond_timedwait(&msgQId->q_cond,
												 &msgQId->q_mutex,
												 &wait_time);
		 }
		 else
		 {
			 cond_errno = pthread_cond_wait(&msgQId->q_cond, &msgQId->q_mutex);
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
	if (!msgQId || (msgQId->q_magic_access != msgQId) || !pcBuffer)
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex))
	{  /* Lock failed */
		return (estiERROR);
	}

	EstiResult eResult = stiOSMsgQReceiveNoLock (msgQId, pcBuffer, un32MaxNBytes, n32Timeout);
	
	/*
	 * Try to recover from any previous errors by eating everything
	 * in the fifo if the queue depth is now zero.
	 */
	if (stiOSMsgQNumMsgs(msgQId) == 0)
	{
		for (;;)
		{
			char buffer[64];
			ssize_t bytes = stiOSRead(msgQId->pipe_fd[0], buffer, sizeof(buffer));
			if (bytes <= 0)
			{
				break;
			}
		}
	}

	pthread_mutex_unlock(&msgQId->q_mutex);
	return (eResult);
}


static void AddMessage (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	uint32_t priority)
{
	stiMSG_Q_MSG *msg = msgQId->q_free;
	
	msgQId->q_num_msgs++;
	if (msgQId->q_num_msgs > msgQId->q_hwm_msg_count)
	{
		msgQId->q_hwm_msg_count = msgQId->q_num_msgs;
		
		stiDEBUG_TOOL (g_stiMessageQueueDebug,
			stiTrace ("High-water mark of messages in queue %s (%p): %d out of %d\n", msgQId->pszName,
						msgQId, msgQId->q_hwm_msg_count, msgQId->q_max_msgs);
		);
	}
	msgQId->q_free = msg->qmsg_next;
	msg->qmsg_next = nullptr;
	msg->qmsg_length = un32Bytes;
	memcpy(msg->qmsg_data, pcBuffer, un32Bytes);
	if (msgQId->q_tail[priority])
	{
		msgQId->q_tail[priority]->qmsg_next = msg;
	}
	else
	{
		msgQId->q_head[priority] = msg;
	}
	msgQId->q_tail[priority] = msg;

	/* Successful enqueue */
	pthread_cond_broadcast(&msgQId->q_cond);
}

extern bool g_bReturnedFromConvertFrame;
extern bool g_bFinishedDecodingInEncode;
extern bool g_bFinishedDecodingInDecode;

EstiResult stiOSMsgQSendNoLock (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout,
	uint32_t priority)
{
	struct timespec wait_time{};
	struct timeval now{};
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
				gettimeofday(&now, nullptr);

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
					cond_errno = pthread_cond_timedwait(&msgQId->q_cond,
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
						eResult = estiERROR;
						break;
					}
				}
			}
			else if (n32Timeout < 0)
			{
				for (;;)
				{
					/* Wait forever */
					pthread_cond_wait(&msgQId->q_cond, &msgQId->q_mutex);
					
					if (msgQId->q_free)
					{
						AddMessage (msgQId, pcBuffer, un32Bytes, priority);
						break;
					}
				}
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


void stiOSLinuxMsgQPurgeNoLock (
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
				msgQId->q_tail[prio] = nullptr;
			}
			msg->qmsg_next = msgQId->q_free;
			msgQId->q_free = msg;
			msgQId->q_num_msgs--;
			msgQId->last_msg = 0;
		}
	}
}


static EstiResult 
send_message (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout,
	uint32_t priority )
{
	if (!msgQId || (msgQId->q_magic_access != msgQId))
	{
		return (estiERROR);
	}
	if (msgQId->q_max_msg_length < un32Bytes)
	{
		return (estiERROR);
	}

	if (priority >= MSGQ_NUM_PRIO)
	{
		/* Illegal priority */
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex))
	{  /* Lock failed */
		return (estiERROR);
	}

	EstiResult eResult = stiOSMsgQSendNoLock(msgQId, pcBuffer, un32Bytes, n32Timeout, priority);

	if (eResult == estiOK)
	{
	   if (stiOSMsgQNumMsgs(msgQId) == 1)
	   {
		   if (1 != write(msgQId->pipe_fd[1], "0", 1))
		   {
			   stiOSLinuxMsgQPurgeNoLock(msgQId);
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
	if (!msgQId || (msgQId->q_magic_access != msgQId))
	{
		return (estiERROR);
	}
	if (msgQId->q_max_msg_length < un32Bytes)
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex))
	{  /* Lock failed */
		return (estiERROR);
	}
  
	stiOSLinuxMsgQPurgeNoLock (msgQId);
	
	EstiResult eResult = stiOSMsgQSendNoLock(msgQId, pcBuffer, un32Bytes, -1, 1);

	if (eResult == estiOK)
	{
	   if (stiOSMsgQNumMsgs(msgQId) == 1)
	   {
		   if (1 != write(msgQId->pipe_fd[1], "0", 1))
		   {
			   stiOSLinuxMsgQPurgeNoLock(msgQId);
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
stiOSLinuxMsgQPurge (stiMSG_Q_ID msgQId)
{
	if (!msgQId || (msgQId->q_magic_access != msgQId))
	{
		return (estiERROR);
	}

	if (pthread_mutex_lock(&msgQId->q_mutex))
	{  /* Lock failed */
		return (estiERROR);
	}

	stiOSLinuxMsgQPurgeNoLock (msgQId);

	pthread_cond_broadcast(&msgQId->q_cond);
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
	return (msgQId->pipe_fd[0]);
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
	

/* end file stiOSMsgLinux.c */
