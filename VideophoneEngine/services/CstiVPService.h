/*!
 * \file CstiVPService.h
 * \brief Base class for all enterpise services classes
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIVPSERVICE_H
#define CSTIVPSERVICE_H

#include "stiSVX.h"
#include "CstiHTTPTask.h"
#include "CstiVPServiceResponse.h"
#include "CstiEventQueue.h"
#include "CstiEventTimer.h"
#include <string>
#include <list>

class CstiVPServiceRequest;
struct SstiRemovedRequest;
typedef struct _IXML_Node IXML_Node;
typedef struct _IXML_Document IXML_Document;
typedef struct _IXML_Element IXML_Element;
typedef struct _IXML_NodeList IXML_NodeList;

struct NodeTypeMapping
{
	const char *pszNodeName;
	int nNodeType;
};


class CstiRequestQueueItem
{
public:

	int m_nPriority;
	CstiVPServiceRequest *m_pRequest;
};


class CstiVPService : public CstiEventQueue
{
public:

	CstiVPService () = delete;
	CstiVPService(
		CstiHTTPTask *pHTTPTask,
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam,
		size_t CallbackFromId,
		uint32_t un32MsgMaxLen,
		const char *pszResponseType,
		const char *pszName,
		unsigned int unMaxTrysOnServer,
		unsigned int unBaseRetryTimeOut,
		unsigned int unMaxRetryTimeOut,
		int nMaxServers);

	~CstiVPService () override = default;

	CstiVPService (const CstiVPService &other) = delete;
	CstiVPService (CstiVPService &&other) = delete;
	CstiVPService &operator= (const CstiVPService &other) = delete;
	CstiVPService &operator= (CstiVPService &&other) = delete;

	virtual stiHResult Startup (
		bool bPause);

	virtual stiHResult Shutdown ();

	void ServiceContactsSet (
		const std::string *pServiceContacts);

	stiHResult RequestCancel (
		unsigned int unRequestId);

	virtual void RequestCancelAsync (
		unsigned int unRequestId);

	void PauseSet (
		bool bPause);

	bool PauseGet () const;

	void EmptyQueue ();

	virtual void clientTokenSet (
		const std::string &clientToken);

	void SSLFlagSet (
		ESSLFlag eSSLFlag);

	ESSLFlag SSLFlagGet () const;

	virtual stiHResult Callback (
		int32_t message,
		CstiVPServiceResponse *response);

	//
	// Public Enumerations
	//
	enum EState
	{
		eUNITIALIZED,
		eDISCONNECTED,
		eCONNECTING,
		eCONNECTED,
		eAUTHENTICATED,
		eDNSERROR,
	}; // end EState

	EState StateGet();

	virtual bool isOfflineGet ();

	virtual void retryOffline (CstiVPServiceRequest* request, const std::function<void ()>& callback)
	{
		stiUNUSED_ARG(request);
	}

	enum EStateEvent
	{
		eSE_INITIALIZED,
		eSE_REQUEST_SENT,
		eSE_HTTP_RESPONSE,
		eSE_AUTHENTICATED,
		eSE_AUTHENTICATION_FAILED,
		eSE_LOGOUT,
		eSE_HTTP_ERROR,
		eSE_DNS_ERROR,
		eSE_RESTART
	};

protected:

	enum EstiRequestRepeat
	{
		estiRR_NO = 0,
		estiRR_IMMEDIATE = 1,
		estiRR_DELAYED = 2
	};

	virtual stiHResult Initialize(
		const std::string *serviceContacts,
		const std::string &uniqueID);

	stiHResult ShutdownHandle ();

	void Cleanup();

	std::string clientTokenGet () const;

	#ifdef stiDEBUGGING_TOOLS
	virtual EstiBool IsDebugTraceSet (const char **ppszService) const = 0;
	#endif
	
	virtual const char *ServiceIDGet () const = 0;

	CstiVPServiceRequest *RequestGet(
		unsigned int unRequestId,
		int *pnPriority);

	EstiBool RequestRemove (
		unsigned int unRequestId);

	virtual void RequestRemoved (
		unsigned int unRequestId);

	virtual stiHResult RequestRemovedOnError(
		CstiVPServiceRequest *request);

	stiHResult CommunicationErrorMarkedRequestRemove ();

	stiHResult RequestCancelHandle (unsigned int unRequestId);

	stiHResult HTTPResultCallbackHandle (uint32_t un32RequestNum, EstiHTTPState eState, SHTTPResult* pstResult);

	virtual stiHResult RequestQueue(
		CstiVPServiceRequest *poRequest,
		int nPriority);

	unsigned int NextRequestIdGet ();

	stiHResult StateEvent(
		EStateEvent eEvent);

	virtual stiHResult StateSet(
		EState eState);

	stiHResult HTTPResponseParse (
		unsigned int unRequestId,
		const char *pszResponse);

	stiHResult HTTPError (bool bSslError);

	stiHResult RequestError ();

	int NodeTypeDetermine (
		const char *pszNodeName,
		const NodeTypeMapping *pNodeTypeMapping,
		int nNumNodeTypeMappings);

	stiHResult ResponseParse (
		IXML_Document *pDocument,
		CstiVPServiceRequest *pRequest,
		unsigned int unRequestId,
		int nPriority,
		EstiRequestRepeat *peRepeatRequest,
		int *pnSystemicError);

	virtual stiHResult ResponseNodeProcess (
		IXML_Node *pNode,
		CstiVPServiceRequest *pRequest,
		unsigned int unRequestId,
		int nPriority,
		EstiBool *pbDatabaseAvailable,
		EstiBool *pbLoggedOutNotified,
		int *pnSystemicError,
		EstiRequestRepeat *peRepeatRequest) = 0;

	static stiHResult ProcessRemainingResponse (
		CstiVPServiceRequest *pRequest,
		int nResponse,
		int nSystemicError,
		size_t CallbackParam);

	virtual stiHResult ProcessRemainingResponse (
		CstiVPServiceRequest *pRequest,
		int nResponse,
		int nSystemicError) = 0;

	virtual stiHResult Restart();

	std::string m_uniqueID;

	void SSLFailOver ();

	virtual void SSLFailedOver () = 0;

	inline unsigned int ErrorCountGet ()
	{
		return m_unErrorCount;
	}

	void MaxRetryTimeoutSet (
		unsigned int unMaxRetryTimeOut);

	// Added to accomodate Core bug when deleting multiple messages.
	CstiVPServiceRequest *NextRequestGet();
	void InsertAsFirstRequest(CstiRequestQueueItem *pItem)
	{
		m_Requests.push_front(*pItem);
	}

	stiHResult requestSendHandle ();

	EstiBool m_bAuthenticated;					// To keep track of having been authenticated or not
	CstiSignalConnection::Collection m_signalConnections;

private:

	static stiHResult HTTPResultCallback (
		uint32_t un32RequestNum,
		EstiHTTPState eState,
		SHTTPResult *pstResult,
		void *pParam1);

	void pendingResponseClear ();

	CstiHTTPTask *m_pHTTPTask;

	int m_nCurrentPrimaryServer;				// Server that is currently the primary server

	uint32_t m_un32MaxMessageLength;
	std::vector<char> m_messageBuffer;

	std::string m_URLID;						// Unique ID with only alphanumeric characters
	std::string m_clientToken;					// Last Valid Client Token

	EState m_eState;							// Current state
	unsigned int m_unPauseCount;				// No requests are sent while the value is non-zero.

	std::list<CstiRequestQueueItem> m_Requests;
	unsigned int m_unResponsePending;			// Which response in pending?
	bool m_bCancelPending;

	unsigned int m_unErrorCount;				// Number of comm errors encountered
	unsigned int m_unRetryCount;
	unsigned int m_unErroredRequest;			// If an error occured then which request errored.

	vpe::EventTimer m_retryTimer;				// Timer for retry attempts
	const unsigned int m_unMaxTrysOnServer;
	const unsigned int m_unBaseRetryTimeOut;
	unsigned int m_unMaxRetryTimeOut;

	unsigned int m_unRequestId;					// Next request number

	ESSLFlag m_eSSLFlag;

	EstiBool m_bPassError;

	const char *m_pszResponseType;

	std::string m_ServiceContacts[2];

	std::mutex m_pendingRequestMutex;
	std::condition_variable m_WaitForPendingCond;
	bool m_bShutdownInitiated;

	PstiObjectCallback m_pfnObjectCallback;
	size_t m_ObjectCallbackParam;
	size_t m_CallbackFromId;
};


// xml tools
const char * AttributeValueGet (
	IXML_Node * pNode,
	const char * szAttributeName);

std::string attributeStringGet (
	IXML_Node* node,
	const std::string& attributeName
);

const char * ElementValueGet (
	IXML_Node * pNode);

std::string elementStringValueGet (
	IXML_Node* node);

const char * SubElementValueGet (
	IXML_Node * pNode,
	const char * szItem);

std::string subElementStringGet (
	IXML_Node* node,
	const std::string& item);

IXML_Node * SubElementGet (
	IXML_Node * pNode,
	const char * szItem);

IXML_Node * NextElementGet (
	IXML_Node * pNode);

EstiBool ResultSuccess (
	IXML_Node * pNode);

#endif // CSTIVPSERVICE_H
