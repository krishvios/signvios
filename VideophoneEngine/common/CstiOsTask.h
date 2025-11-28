////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiOsTask
//
//  File Name:  CstiOsTask.h
//
//  Owner:      Todd Christensen
//
//  Abstract:  This base class is for starting a task within a class
//      
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CstiOsTask_h
#define CstiOsTask_h

////////////////////////////////////////////////////////////////////////////////
// Includes
#include "stiOS.h"
#include "CstiTask.h"
#include "stiError.h"
#include "stiTrace.h"

#include <condition_variable>
#include <mutex>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

class CstiOsTask : public CstiTask
{
public:
	
	CstiOsTask (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam,
		size_t CallbackFromId,
		uint32_t un32MsgMaxCount,
		uint32_t un32MsgMaxLen,
		const char* szName,
		int nPriority,
		int32_t n32StackSize);
	
	~CstiOsTask () override;
	
	void ErrorLogPtrSet (SstiErrorLog *pstErrorLog);

	stiHResult Shutdown () override;
	
	virtual stiHResult WaitForShutdown ();
	
	virtual stiHResult MsgRecv (
		char* szBuffer, 
		int32_t n32BuffSize,
		int32_t n32RecvWait = stiWAIT_FOREVER) = 0;
	
	stiHResult MsgSend (
		IN CstiEvent* pMsg,
		IN int32_t n32Param,
		IN int32_t n32SendWait = stiWAIT_FOREVER) override = 0;
	
	stiHResult TimerStart (
		stiWDOG_ID Timer,
		uint32_t un32WaitTime,
		intptr_t Parameter);
		
	static void * TaskWrapper (
		void * param);
	
	EstiBool IsStarted () const;

	stiHResult Lock() const;

	void Unlock() const;

protected:
	
	// Message Queue variables
	uint32_t m_un32MsgMaxCount;	// The maximum number of messages in the 
								// queue at a given time
	uint32_t m_un32MsgMaxLen;	// The maximum message size to be passed to
								// the queue
	// Task variables
	const char* m_pszName;		// The name of the task being created
	int m_nPriority;			// The priority of the task
	int32_t m_n32StackSize;	// The stack size of the task
	SstiErrorLog *m_pstErrorLog;	// Structure to store errors in
	stiTaskID m_tidTaskId;   	// The task id assigned at task creation by th OS
	
	// A message is sent to call this method
	stiHResult stiEVENTCALL ShutdownHandle () override;
	
	mutable std::recursive_mutex m_LockMutex{};
	
private:
	
	std::condition_variable_any m_ShutdownCond{};
	bool m_bShuttingDown;
	
	// The function you want to spawned must be defined like this
	virtual int Task () = 0;
		
	stiHResult Spawn () override;
	
	static void TimerCallback (
		stiWDOG_ID Timer,
		intptr_t Parameter1, // pointer to class object that set the timer
		intptr_t Parameter2);
};

#endif // CstiOsTask_h
// end file CstiOsTask.h


