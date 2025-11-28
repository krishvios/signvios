////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiOsTask
//
//	File Name:	CstiOsTask.cpp
//
//  Owner:      Todd Christensen
//
//  Abstract:  This base class is for starting a task within a class
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiOsTask.h"
#include "stiTrace.h"
#include "CstiEvent.h"
#include "CstiExtendedEvent.h"
#include "IstiPlatform.h"

//
// Constants
//
#define MAX_TASK_SHUTDOWN_WAIT 0

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
// CstiOsTask methods
//

////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::CstiOsTask ()
//
// Description:	CstiOsTask's constructor
//
// Abstract:
//
// Returns: None
//
CstiOsTask::CstiOsTask (
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam,
	size_t CallbackFromId,
	uint32_t un32MsgMaxCount,
	uint32_t un32MsgMaxLen,
	const char *pszName,
	int nPriority,
	int32_t n32StackSize)
	:
	CstiTask (
		pfnAppCallback,
		CallbackParam,
		CallbackFromId),
	m_un32MsgMaxCount (un32MsgMaxCount),
	m_un32MsgMaxLen (un32MsgMaxLen),
	m_pszName (pszName),
	m_nPriority (nPriority),
	m_n32StackSize (n32StackSize),
	m_pstErrorLog (nullptr),
	m_tidTaskId      (nullptr),
	m_bShuttingDown (false)

{
	stiLOG_ENTRY_NAME (CstiOsTask::CstiOsTask);
} // end CstiOsTask::CstiOsTask ()


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::CstiOsTask ()
//
// Description:	CstiOsTask's destructor
//
// Abstract:
//
// Returns: None
//
CstiOsTask::~CstiOsTask ()
{
	stiDEBUG_TOOL (g_stiTaskDebug,
		stiTrace ("Destroying Task Object %s\n", m_pszName);
	);
	
	m_ShutdownCond.notify_all();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::Shutdown ()
//
// Description:	
//
// Abstract:
//
// Returns: 
//
stiHResult CstiOsTask::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiOsTask::Shutdown);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();
	
	stiDEBUG_TOOL (g_stiTaskDebug,
		stiTrace ("Shutdown request received: %s\n", m_pszName);
	);
	
	if (!m_bShuttingDown && IsStarted ())
	{
		m_bShuttingDown = true;
	
		hResult = CstiTask::Shutdown ();
	}
	else
	{
		if (m_bShuttingDown)
		{
			stiDEBUG_TOOL (g_stiTaskDebug,
				stiTrace ("Already shutting down: %s\n", m_pszName);
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiTaskDebug,
				stiTrace ("Already shutdown: %s\n", m_pszName);
			);
		}
	}
	
	Unlock ();
	
	return (hResult);
	
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::WaitForShutdown ()
//
// Description:	
//
// Abstract:
//
// Returns: 
//
stiHResult CstiOsTask::WaitForShutdown ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::unique_lock<std::recursive_mutex> lock (m_LockMutex);
	
	if (m_tidTaskId != nullptr)
	{
		stiDEBUG_TOOL (g_stiTaskDebug,
			stiTrace ("Waiting for shutdown: %s\n", m_pszName);
		);
		
		m_ShutdownCond.wait(lock);
	}
	else
	{
		stiDEBUG_TOOL (g_stiTaskDebug,
			stiTrace ("Already shutdown: %s\n", m_pszName);
		);
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::Spawn()
//
// Description:	Spawns the task
//
// Abstract:
//
// Returns: (stiHResult)
//		estiOK, or on error estiERROR
//
stiHResult CstiOsTask::Spawn ()
{
	stiLOG_ENTRY_NAME (CstiOsTask::Spawn);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	stiTESTCOND (nullptr == m_tidTaskId, stiRESULT_TASK_ALREADY_STARTED);
	
	stiDEBUG_TOOL (g_stiTaskDebug,
		stiTrace ("Spawning %s\n", m_pszName);
	);
	
	
	m_tidTaskId = stiOSTaskSpawn (m_pszName,
									m_nPriority,
									m_n32StackSize,
									TaskWrapper,
									this);
		
	if (stiOS_TASK_INVALID_ID == m_tidTaskId)
	{
		m_tidTaskId = nullptr;
	}
	
	stiTESTCOND (nullptr != m_tidTaskId, stiRESULT_TASK_SPAWN_FAILED);
	
STI_BAIL:
	
	Unlock ();
	
	return hResult;
	
} // end CstiOsTask::Spawn()

////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::ShutdownHandle ()
//
// Description:	Called before the task exits
//
// Abstract:
//
// Returns: None
//
stiHResult CstiOsTask::ShutdownHandle ()
{
	stiLOG_ENTRY_NAME (CstiOsTask::ShutdownHandle);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();
	
	//
	// Call the base class ShutdownHandle()
	//
	hResult = CstiTask::ShutdownHandle ();
	
	Unlock ();
	
	return hResult;
	
} // end CstiOsTask::ShutdownHandle ()


void CstiOsTask::TimerCallback (
	stiWDOG_ID Timer,
	intptr_t Parameter1, // pointer to class object that set the timer
	intptr_t Parameter2) 
{
	auto pThis = (CstiTask *)Parameter1;
	
	CstiExtendedEvent oEvent (estiEVENT_TIMER, (size_t)Timer, Parameter2);
	
	pThis->MsgSend (&oEvent, sizeof (oEvent));
	
} // end CstiOsTask::TimerCallback


stiHResult CstiOsTask::TimerStart (
	stiWDOG_ID Timer,
	uint32_t un32WaitTime,
	intptr_t Parameter)
{
	stiHResult hResult = stiOSWdStart2 (Timer,
									 un32WaitTime,
									 TimerCallback, // Associate the handler function
									 (intptr_t)this, Parameter);
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::ErrorLogPtrSet ()
//
// Description:  Stores the ErrorLog structure in the class
//
// Abstract:
//
void CstiOsTask::ErrorLogPtrSet (SstiErrorLog* pstErrorLog)
{
	stiLOG_ENTRY_NAME (CstiOsTask::ErrorLogPtrSet);

	m_pstErrorLog = pstErrorLog;
} // end CstiOsTask::ErrorLogPtrSet 


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::TaskWrapper()
//
// Description:	Wraps the task object
//
// Abstract:
//
// Returns: (int)
//
void * CstiOsTask::TaskWrapper (
	void * param)
{
	int nRtn = -1;
	
	auto poTask = static_cast<CstiOsTask *>(param);
	
	try
	{

	stiLOG_ENTRY_NAME (CstiOsTask::TaskWrapper);

	stiDEBUG_TOOL (g_stiTaskDebug,
		stiTrace ("Spawned %s (%x)\n", poTask->m_pszName, stiOSTaskIDSelf ());
	);

	poTask->ErrorLogPtrSet (stiErrorLogInit ());
	
	nRtn = poTask->Task ();
	
	poTask->Callback (estiMSG_CB_SERVICE_SHUTDOWN, 0);
	stiErrorLogClose ();
	poTask->ErrorLogPtrSet (nullptr);
	
	poTask->Lock ();
	
	//
	// This will mark this task as shutdown
	//
	poTask->m_tidTaskId = nullptr;

	poTask->m_bShuttingDown = false;
	
	stiDEBUG_TOOL (g_stiTaskDebug,
		stiTrace ("task %s (%x) shutdown. Return value = %d\n", poTask->m_pszName, stiOSTaskIDSelf (), nRtn);
	);
		
	poTask->m_ShutdownCond.notify_one ();
	
	poTask->Unlock ();

	}
	catch (const std::exception &e)
	{
		stiTrace ("CstiOsTask::TaskWrapper: ***UNHANDLED EXCEPTION*** (Thread: %s) - %s\n", poTask->m_pszName, e.what());
		IstiPlatform::InstanceGet()->RestartSystem(IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION);
	}
	catch (...)
	{
		stiTrace ("CstiOsTask::TaskWrapper: ***UNHANDLED EXCEPTION***: unknown\n");
		IstiPlatform::InstanceGet()->RestartSystem(IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION);
	}


	return (nullptr);
	
} // end CstiOsTask::TaskWrapper()


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::IsStarted()
//
// Description:	Determines whether the task is currently running or not
//
// Abstract:
//
// Returns: (EstiBool) estiTRUE id running estiFALSE if not
//
EstiBool CstiOsTask::IsStarted () const
{
	stiLOG_ENTRY_NAME (CstiOsTask::IsStarted);
	
	EstiBool bRunning = estiFALSE;
	
	Lock ();
	
	if (m_tidTaskId)
	{
		bRunning = estiTRUE;
	}
	
	Unlock ();
	
	return bRunning;
	
} // end CstiOsTask::IsStarted()



////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::Lock
//
// Description: Locks a mutex to lock the class
//
// Abstract:
//  Locks a mutex to lock access to certain data members of the class
//
// Returns:
//  None
//
stiHResult CstiOsTask::Lock() const
{
	m_LockMutex.lock ();
	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiOsTask::Unlock
//
// Description: Unlocks a mutex to lock the class
//
// Abstract:
//  Unlocks a mutex to unlock access to certain data members of the class
//
// Returns:
//  None
//
void CstiOsTask::Unlock() const
{
	m_LockMutex.unlock ();
}


		

// end file CstiOsTask.cpp
