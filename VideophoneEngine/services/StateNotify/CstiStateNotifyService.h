//!
// \filename CstiStateNotifyService.h
// \brief Contains the class definition for the State Notify Service.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
#ifndef CSTISTATENOTIFYSERVICE_H
#define CSTISTATENOTIFYSERVICE_H

//
// Includes
//
#include "stiSVX.h"
#include "stiEventMap.h"
#include "ixml.h"
#include "CstiStateNotifyResponse.h"
#include "CstiHTTPTask.h"
#include "stiTools.h"
#include "CstiVPService.h"
#include <random>

//
// Constants
//
const int nDEFAULT_HEARTBEAT_DELAY = 300;


//
// Typedefs
//

//
// Forward Declarations
//
class CstiEvent;
class CstiStateNotifyRequest;
class CstiCoreService;


//
// Globals
//

//!
// \brief Class for the State Notify Service.
//
class CstiStateNotifyService : public CstiVPService
{
public:

	CstiStateNotifyService (
		CstiCoreService *pCoreService,
		CstiHTTPTask *pHTTPTask,
		PstiObjectCallback pAppCallback,
		size_t CallbackParam);
	
	stiHResult Initialize (
		const std::string *serviceContacts,
		const std::string &uniqueID) override;

	CstiStateNotifyService (const CstiStateNotifyService &other) = delete;
	CstiStateNotifyService (CstiStateNotifyService &&other) = delete;
	CstiStateNotifyService &operator= (const CstiStateNotifyService &other) = delete;
	CstiStateNotifyService &operator= (CstiStateNotifyService &&other) = delete;

	~CstiStateNotifyService () override;

	//
	// Task Functions
	//
	stiHResult Startup (
		bool bPause) override;
	
	void HeartbeatRequestAsync (
		uint32_t delaySeconds);
		
	void HeartbeatDelaySet (
		int nHeartbeatDelay);
	
	void PublicIPClear ();

	void clientTokenSet(const std::string& clientToken) override;

private:

	void RequestRemoved(
		unsigned int unRequestId) override;
		
	void SSLFailedOver() override;

	EstiBool m_bConnected;			// Keep track of having been connected
	unsigned int m_unHeartbeatRequestId;	// Request id of current heartbeat request

	vpe::EventTimer m_heartbeatTimer;	// Timer for Heartbeat Messages
	int m_nHeartbeatDelay;
	
	std::string m_LastIP;			// Last detected IP address

	CstiCoreService *m_pCoreService;
	
	stiHResult LoggedOutErrorProcess (
		CstiVPServiceRequest *pRequest,
		EstiBool * pbLoggedOutNotified);
	
	CstiStateNotifyResponse::EStateNotifyError NodeErrorGet (
		IXML_Node *pNode,
		EstiBool *pbDatabaseAvailable);
	
	//
	// task notify handle methods
	//
	stiHResult ShutdownHandle ();

	EstiResult ConnectHandle ();
		
	void HeartbeatRequestHandle (uint32_t delaySeconds);

	//
	// Utility Methods
	//
	stiHResult HeartbeatSend ();

	stiHResult ResponseNodeProcess (
		IXML_Node *pNode,
		CstiVPServiceRequest *pRequest,
		unsigned int unRequestId,
		int nPriority,
		EstiBool *pbDatabaseAvailable,
		EstiBool *pbLoggedOutNotified,
		int *pnSystemicError,
		EstiRequestRepeat *peRepeatRequest) override;

	stiHResult ProcessRemainingResponse (
		CstiVPServiceRequest *pRequest,
		int nResponse,
		int nSystemicError) override;
		
	stiHResult StateSet (
		EState eState) override;

	stiHResult Restart () override;
	
	#ifdef stiDEBUGGING_TOOLS
	EstiBool IsDebugTraceSet (const char **pszService) const override;
	#endif
	
	const char *ServiceIDGet () const override;

	// NOTE: this isn't seeded with any random data, so it will
	// generate the same "random" numbers over and over
	std::minstd_rand m_numberGenerator;
};

#endif // CSTISTATENOTIFYSERVICE_H

// end file CstiStateNotifyService.h
