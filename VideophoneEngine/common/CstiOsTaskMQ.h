////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiOsTaskMQ
//
//  File Name:	CstiOsTaskMQ.h
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		This class implements a Message Queue on top of the base class
//		to provide communication between tasks and objects.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CstiOsTaskMQ_h
#define CstiOsTaskMQ_h

//
// Includes
//
#include "CstiOsTask.h"
#include "stiOS.h"
#include "stiAssert.h"
#include "stiTrace.h"


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
// Class Declaration
//
class CstiOsTaskMQ : public CstiOsTask
{

public:
	
	CstiOsTaskMQ (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam,
		size_t CallbackFromId,
		uint32_t un32MsgMaxCount,
		uint32_t un32MsgMaxLen,
		const char* szName,
		int nPriority,
		int32_t n32StackSize);

	~CstiOsTaskMQ () override;
	
	stiHResult Startup () override;
	
	stiHResult MsgRecv (
		char *pszBuffer, 
		int32_t n32BuffSize,
		int32_t n32RecvWait = stiWAIT_FOREVER) override;
	
	stiHResult MsgSend (
		CstiEvent* pMsg,
		int32_t n32Param,
		int32_t n32SendWait = 5000) override;
	
	stiHResult ForceMsgSend (
		void* pMsg, 
		int32_t n32Param) override;
		
	int FileDescriptorGet ();
	
	void QueueEnabledSet (
		bool bEnabled);

	bool QueueIsEmpty ();
	
protected:
	
	stiHResult stiEVENTCALL ShutdownHandle () override;
	stiMSG_Q_ID m_qidQId;      // The queue id assigned at queue creation
	int m_nCurrentEvent;	// The event that is currently being executed; -1 otherwise.

private:

	//
	// This is the function you want to be spawned as a separate task
	//
	int Task () override = 0;
};


#endif // CstiOsTaskMQ_h
// end file CstiOsTaskMQ.h
