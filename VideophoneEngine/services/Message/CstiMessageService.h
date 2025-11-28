/*!
 * \file CstiMessgeService.h
 * \brief Definition of the CstiMessageService class.
 *
 * Processes MessageService requests and responses.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef CSTIMESSAGESERVICE_H
#define CSTIMESSAGESERVICE_H

//
// Includes
//
#include "CstiEvent.h"
#include "stiSVX.h"
#include "stiEventMap.h"
#include "CstiMessageRequest.h"
#include "CstiMessageResponse.h"
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
class CstiMessageService : public CstiVPService
{
public:
	
	CstiMessageService (
		CstiCoreService *pCoreService,
		CstiHTTPTask *pHTTPTask,
		PstiObjectCallback pAppCallback,
		size_t CallbackParam);
	
	stiHResult Initialize (
		const std::string *serviceContacts,
		const std::string &uniqueID) override;

	CstiMessageService (const CstiMessageService &other) = delete;
	CstiMessageService (CstiMessageService &&other) = delete;
	CstiMessageService &operator= (const CstiMessageService &other) = delete;
	CstiMessageService &operator= (CstiMessageService &&other) = delete;

	~CstiMessageService () override;

	//
	// Task Functions
	//
	EstiResult RequestSend (
		CstiMessageRequest *poMessageRequest,
		unsigned int *punRequestId = nullptr);

	stiHResult RequestSendEx (
		CstiMessageRequest *poMessageRequest,
		unsigned int *punRequestId);

	CstiCoreService* coreServiceGet ();

private:

	stiHResult StateSet (
		EState eState) override;

	stiHResult RequestRemovedOnError(
		CstiVPServiceRequest *request) override;

	void SSLFailedOver() override;

	CstiMessageResponse::EResponseError NodeErrorGet (
		IXML_Node *pNode,
		EstiBool *pbDatabaseAvailable);
	
	stiHResult LoggedOutErrorProcess (
		CstiVPServiceRequest *pRequest,
		EstiBool * pbLoggedOutNotified);
	
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
		
	stiHResult MessageListProcess (
		CstiMessageResponse *poResponse,
		IXML_Node * pNode);
	
	stiHResult MessageDownloadUrlListProcess (
		CstiMessageResponse *poResponse,
		IXML_Node * pNode);
	
	stiHResult GreetingInfoGetProcess (
		CstiMessageResponse * poResponse,
		IXML_NodeList * pChildNodes,
		int nMaxSeconds);

	stiHResult errorResponseSend (
		CstiVPServiceRequest *request,
		CstiMessageResponse::EResponse response,
		CstiMessageResponse::EResponseError errResponse,
		const char *xmlError);

//	void Cleanup ();

	stiHResult Restart () override; 

	EstiBool m_bConnected;				// Keep track of having been connected
	
	CstiCoreService *m_pCoreService;

	#ifdef stiDEBUGGING_TOOLS
	EstiBool IsDebugTraceSet (const char **ppszService) const override;
	#endif
	
	const char *ServiceIDGet () const override;
};

#endif // CSTIMESSAGESERVICE_H

// end file CstiMessageService.h
