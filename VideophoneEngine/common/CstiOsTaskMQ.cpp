////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiOsTaskMQ
//
//  File Name:	CstiOsTaskMQ.cpp
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		This class implements a Message Queue on top of the base class
//		to provide communication between tasks and objects.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiOsTaskMQ.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//

//
// Class Definitions
//

////////////////////////////////////////////////////////////////////////////////
//; CstiOsTaskMQ::CstiOsTaskMQ ()
//
// Description:	CstiOsTaskMQ's constructor
//
// Abstract:
//
// Returns: None
//
CstiOsTaskMQ::CstiOsTaskMQ (
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam,
	size_t CallbackFromId,
	uint32_t un32MsgMaxCount,
	uint32_t un32MsgMaxLen,
	const char* szName,
	int nPriority,
	int32_t n32StackSize)
	:
	CstiOsTask (
		pfnAppCallback,
		CallbackParam,
		CallbackFromId,
		un32MsgMaxCount,
		un32MsgMaxLen,
		szName,
		nPriority, 
		n32StackSize),
	m_qidQId (nullptr),
	m_nCurrentEvent (-1)
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::CstiOsTaskMQ);
	
	m_qidQId = stiOSNamedMsgQCreate  (m_pszName, m_un32MsgMaxCount, m_un32MsgMaxLen);
} // end CstiOsTaskMQ::CstiOsTaskMQ ()


/*!\brief Destructor for the message queue task class
 * 
 */
CstiOsTaskMQ::~CstiOsTaskMQ()
{
	//
	// Delete the message queue
	//
	if (m_qidQId)
	{
		stiOSMsgQDelete (m_qidQId);
		m_qidQId = nullptr;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTaskMQ::Startup ()
//
// Description:	Initializes specific objects for this class
//
// Abstract:
//
// Returns: (stiHResult)
//		estiOK, or on error estiERROR
//
stiHResult CstiOsTaskMQ::Startup ()
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::Startup);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	//
	// Validate values required for the message queue
	//
	stiTESTCOND (m_un32MsgMaxCount > 0 && m_un32MsgMaxLen > 0, stiRESULT_ERROR);
	
	if (nullptr == m_qidQId)
	{
		//
		// Create the message queue
		//
		m_qidQId = stiOSNamedMsgQCreate  (m_pszName, m_un32MsgMaxCount, m_un32MsgMaxLen);
	}
	
	stiTESTCOND (m_qidQId != nullptr, stiRESULT_ERROR);

	//
	// Call the base class Init method
	//
	hResult = CstiOsTask::Startup ();
	
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
	
} // end CstiOsTaskMQ::Startup ()


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTaskMQ::MsgSend ()
//
// Description:	Send message to task's message queue
//
// Abstract:
//
// Returns: (stiHResult)
//		estiOK, or on error estiERROR
//
stiHResult CstiOsTaskMQ::MsgSend (
	IN CstiEvent* pMsg,			// the message to be sent
	IN int32_t n32MsgLength,	// the length of the message
	IN int32_t n32SendWait)// How many milliseconds to wait on the queue if it's busy
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::MsgSend);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	int nNewEvent = pMsg->EventGet ();
	
	stiTESTCOND (m_qidQId != nullptr, stiRESULT_ERROR);

	eResult = stiOSMsgQSend  (m_qidQId, (char*)pMsg, n32MsgLength, n32SendWait);
	stiTESTCONDMSG (estiOK == eResult, stiRESULT_ERROR, "TaskName: %s.  Event=%d, waiting on event %d", m_pszName, nNewEvent, m_nCurrentEvent);
	
STI_BAIL:
		
	return hResult; 
} // end CstiOsTaskMQ::MsgSend ()


stiHResult CstiOsTaskMQ::ForceMsgSend (
	void* pMsg, 
	int32_t n32MsgLength)
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::ForceMsgSend);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	
	stiTESTCOND (m_qidQId != nullptr, stiRESULT_ERROR);

	eResult = stiOSMsgQForceSend  (m_qidQId, (char*)pMsg, n32MsgLength);
	stiTESTCOND (estiOK == eResult, stiRESULT_ERROR);
	
STI_BAIL:
		
	return hResult; 
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTaskMQ::MsgRecv
//
// Description: Receive a message from the message queue
//
// Abstract:
//
stiHResult CstiOsTaskMQ::MsgRecv (
	IN char* szBuffer, 			// The buffer to read into
	IN int32_t n32BuffSize,	// The length of the buffer
	IN int32_t nRecvWait)		// Number of ticks to wait for incoming message
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::MsgRecv);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	EstiResult eResult = estiOK;
	
	stiTESTCOND (m_qidQId != nullptr, stiRESULT_ERROR);

	eResult = stiOSMsgQReceive  (m_qidQId, szBuffer, n32BuffSize, nRecvWait);
	
	hResult = (stiHResult)eResult;
	
STI_BAIL:
	
	return (hResult);
	
} // end CstiOsTaskMQ::MsgRecv

////////////////////////////////////////////////////////////////////////////////
//; CstiOsTaskMQ::ShutdownHandle ()
//
// Description: Called to shut down the anything specific to this class
//
// Abstract:
//	In this case, we created a pipe that needs to be shutdown.  We will
//	also call the base class Shutdown method to further shutdown anything
//	it has created.
//
// Returns: None
//
stiHResult CstiOsTaskMQ::ShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	//
	// Delete the message queue
	//
	if (m_qidQId)
	{
		stiOSMsgQDelete (m_qidQId);
		m_qidQId = nullptr;
	}
	
	//
	// Call base class ShutdownHandle
	//
	hResult = CstiOsTask::ShutdownHandle ();
	stiTESTRESULT ();
	
STI_BAIL:
	
	return (hResult);
	
} // end CstiOsTaskMQ::ShutdownHandle ()


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTaskMQ::FileDescriptorGet
//
// Description: Get the file descriptor
//
// Abstract:
//
// Returns:
//	The file descriptor for the message queue used by the class
//
int CstiOsTaskMQ::FileDescriptorGet ()
{
	stiLOG_ENTRY_NAME (CstiOsTaskMQ::FileDescriptorGet);

	return (stiOsMsgQDescriptor(m_qidQId));
	
} // end CstiOsTaskMQ::FileDescriptorGet



void CstiOsTaskMQ::QueueEnabledSet (
	bool bEnabled)
{
	Lock ();
	
	stiOSMsqQSendEnable (m_qidQId, bEnabled);
	
	Unlock ();
}


bool CstiOsTaskMQ::QueueIsEmpty ()
{
	Lock ();
	
	bool bIsEmpty = (stiOSMsgQNumMsgs (m_qidQId) == 0);
	
	Unlock ();
	
	return (bIsEmpty);
}

// end file CstiOsTaskMQ.cpp

