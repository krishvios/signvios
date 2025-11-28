/*******************************************************************************
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*	Module Name:	stiOSWin32Msg
*
*	File Name:		stiOSWin32Msg.h
*
*	Abstract:
*		see stiOSWin32Msg.cpp
*
*******************************************************************************/
#ifndef STIOSWIN32MSG_H
#define STIOSWIN32MSG_H

/*
 * Includes
 */
#include <sys/types.h>

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

struct stiMSG_Q_ID_s;    /* Private implementation */
typedef struct stiMSG_Q_ID_s *stiMSG_Q_ID;

stiMSG_Q_ID stiOSMsgQCreate (
	int32_t n32MaxMsgs,
	int32_t n32MaxMsgLength);

stiMSG_Q_ID stiOSNamedMsgQCreate (
	const char *pszName,
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
	int32_t n32Timeout);			// In milliseconds

EstiResult stiOSMsgQSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout);			// In milliseconds
 
EstiResult stiOSMsgQUrgentSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes,
	int32_t n32Timeout);			// In milliseconds
 
EstiResult stiOSMsgQForceSend (
	stiMSG_Q_ID msgQId,
	char * pcBuffer,
	uint32_t un32Bytes);

/* Private definitions for use by Windows pipes implementation */
EstiResult stiOSWin32MsgQPurge (stiMSG_Q_ID msgQid);

int stiOsMsgQDescriptor (
	stiMSG_Q_ID msgQId);

stiHResult stiOSMsqQSendEnable (
	stiMSG_Q_ID msgQId,
	bool bEnable);

#endif /* STIOSWIN32MSG_H */
/* end file stiOSWin32Msg.h */
