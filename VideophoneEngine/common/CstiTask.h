////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiTask
//
//  File Name:  CstiTask.h
//
//  Owner:      Todd Christensen
//
//  Abstract:  This base class is for starting a task within a class
//
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTITASK_H
#define CSTITASK_H

//
// Includes
//
#include "stiDefs.h"
#include "stiSVX.h"
#include "stiOS.h"
#include "stiAssert.h"
#include "stiTrace.h"
#include "stiError.h"
#include "CstiEvent.h"

#ifdef WIN32
#define stiEVENTCALL __cdecl
#else
#define stiEVENTCALL
#endif

//
// Forward Declarations
//

//
// Typedefs
//

// Function pointer description for callbacks from tasks to app (or CCI, etc.)
//
class CstiTask
{
public:
	CstiTask (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam,
		size_t CallbackFromId);

	virtual ~CstiTask () = default;

	virtual stiHResult Startup ();

	virtual stiHResult Shutdown ();

	virtual stiHResult MsgSend (
		CstiEvent* pMsg,
		int32_t n32Param,
		int32_t n32SendWait = 5000) = 0;

	virtual stiHResult ForceMsgSend (
		void* pMsg, 
		int32_t n32Param) = 0;

	stiHResult Callback (
		int32_t n32Message,
		size_t MessageParam)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		
		if (m_pfnObjectCallback)
		{
			hResult = m_pfnObjectCallback (n32Message, MessageParam,
									m_ObjectCallbackParam, m_CallbackFromId);
		}
		
		return hResult;
	}
		
	PstiObjectCallback DefaultCallbackGet ();
		
protected:
	
	// a message is sent to call this method
	virtual stiHResult stiEVENTCALL ShutdownHandle ();

private:
	
	// A default callback for tasks that have not implemented the callback
	static stiHResult DefaultCallbackFunction(
		int32_t n32Message, 
		size_t MessageParam,
		size_t CallbackParam,
		size_t CallbackFromId);

	// App callback function data members (allow access by derived classes)
	PstiObjectCallback m_pfnObjectCallback;
	size_t m_ObjectCallbackParam;
	size_t m_CallbackFromId;

	// use this to Spawn the task
	virtual stiHResult Spawn () = 0;
};


#endif // CSTITASK_H
// end file CstiTask.h


