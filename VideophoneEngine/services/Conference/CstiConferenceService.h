/*!
 * \file CstiConferenceService.h
 * \brief Definition of the CstiConferenceService class.
 *
 * Processes ConferenceService requests and responses.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2014 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef CSTICONFERENCESERVICE_H
#define CSTICONFERENCESERVICE_H

//
// Includes
//
#include "CstiEvent.h"
#include "stiSVX.h"
#include "stiEventMap.h"
#include "CstiConferenceRequest.h"
#include "CstiConferenceResponse.h"
#include "CstiHTTPTask.h"
#include "CstiVPService.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
class CstiCoreService;
typedef struct _IXML_Node IXML_Node;
typedef struct _IXML_Document IXML_Document;

//
// Globals
//

//
// Class Declaration
//
class CstiConferenceService : public CstiVPService
{
public:
	
	CstiConferenceService (
		CstiCoreService *pCoreService,
		CstiHTTPTask *pHTTPTask,
		PstiObjectCallback pAppCallback,
		size_t CallbackParam);
	
	stiHResult Initialize (
		const std::string *serviceContacts,
		const std::string &uniqueID) override;

	CstiConferenceService (const CstiConferenceService &other) = delete;
	CstiConferenceService (CstiConferenceService &&other) = delete;
	CstiConferenceService &operator= (const CstiConferenceService &other) = delete;
	CstiConferenceService &operator= (CstiConferenceService &&Other) = delete;

	~CstiConferenceService () override;

	//
	// Task Functions
	//
	stiHResult RequestSendEx (
		CstiConferenceRequest *poMessageRequest,
		unsigned int *punRequestId);

private:

	stiHResult StateSet (
		EState eState) override;

	stiHResult RequestRemovedOnError(
		CstiVPServiceRequest *request) override;

	void SSLFailedOver() override;

	CstiConferenceResponse::EResponseError NodeErrorGet (
		IXML_Node *pNode,
		EstiBool *pbDatabaseAvailable);
	
	stiHResult LoggedOutErrorProcess (
		CstiVPServiceRequest *pRequest,
		EstiBool * pbLoggedOutNotified);
		
	stiHResult ConferenceRoomStatusProcess (
		CstiConferenceResponse *poResponse,
		IXML_Node *pNode);
		
	//
	// Utility Methods
	//
	stiHResult ResponseNodeProcess (
		IXML_Node * pNode,
		CstiVPServiceRequest *pRequest,
		unsigned int unRequestId,
		int nPriority,
		EstiBool * pbDatabaseAvailable,
		EstiBool * pbLoggedOutNotified,
		int *pnSystemicError,
		EstiRequestRepeat *peRepeatRequest) override;
	
	stiHResult ProcessRemainingResponse (
		CstiVPServiceRequest *pRequest,
		int nResponse,
		int nSystemicError) override;
		
//	void Cleanup ();

	stiHResult Restart () override; 

	EstiBool m_bConnected;				// Keep track of having been connected
	
	CstiCoreService *m_pCoreService;

	#ifdef stiDEBUGGING_TOOLS
	EstiBool IsDebugTraceSet (const char **ppszService) const override;
	#endif

	const char *ServiceIDGet () const override;
};

#endif // CSTICONFERENCESERVICE_H

// end file CstiConferenceService.h
