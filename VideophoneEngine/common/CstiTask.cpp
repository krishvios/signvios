////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiTask
//
//	File Name:	CstiTask.cpp
//
//  Owner:      Todd Christensen
//
//  Abstract:  This base class is for starting a task within a class
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiTask.h"
#include "CstiEvent.h"
#include "CstiExtendedEvent.h"
#include "CstiUniversalEvent.h"

//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Locals
//

//
// Forward Declarations
//

////////////////////////////////////////////////////////////////////////////////
// CstiTask methods
//

////////////////////////////////////////////////////////////////////////////////
//; CstiTask::CstiTask ()
//
// Desc: CstiTask's constructor
//
// Returns: None
//
CstiTask::CstiTask (
	PstiObjectCallback pfnObjectCallback,
	size_t CallbackParam,
	size_t CallbackFromId)
	:
	m_pfnObjectCallback (pfnObjectCallback),
	m_ObjectCallbackParam (CallbackParam),
	m_CallbackFromId (CallbackFromId)
{
	stiLOG_ENTRY_NAME (CstiTask::CstiTask)

} // end CstiTask::CstiTask ()


////////////////////////////////////////////////////////////////////////////////
//; CstiTask::Startup ()
//
// Desc: Initializes the task by creating a message queue
//
// Returns: (stiHResult)
//		estiOK, or on error estiERROR
//
stiHResult CstiTask::Startup ()
{
	stiLOG_ENTRY_NAME (CstiTask::Startup)

	return Spawn ();

} // end CstiTask::Startup ()


////////////////////////////////////////////////////////////////////////////////
//; CstiTask::Shutdown
//
// Description: Tells the task to shutdown
//
// Abstract:
//	Post a message to the task to shutdown.
//
//
stiHResult CstiTask::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiTask::Shutdown);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Send the message
	//
	CstiEvent oEvent (estiEVENT_SHUTDOWN);

	hResult = MsgSend (&oEvent, sizeof (oEvent));
	if (stiIS_ERROR (hResult))
	{
		hResult = ForceMsgSend (&oEvent, sizeof (oEvent));
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;

} // end CstiTask::Shutdown


////////////////////////////////////////////////////////////////////////////////
//; CstiTask::ShutdownHandle ()
//
// Desc: Called before the task exits
//
// Returns: None
//
stiHResult CstiTask::ShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CstiTask::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;

	return (hResult);

} // end CstiTask::ShutdownHandle ()

PstiObjectCallback CstiTask::DefaultCallbackGet ()
{
	return DefaultCallbackFunction;
}


// A default callback for tasks that have not implemented the callback
stiHResult CstiTask::DefaultCallbackFunction(
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t CallbackFromId)
{
	auto pThis = (CstiTask *)CallbackParam;

	if (pThis)
	{
		CstiUniversalEvent oEvent (estiEVENT_CALLBACK, n32Message, MessageParam, CallbackFromId);

		pThis->MsgSend (&oEvent, sizeof (oEvent));
	}

	return stiRESULT_SUCCESS;
};

// end file CstiTask.cpp

