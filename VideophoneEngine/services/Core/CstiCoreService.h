///
/// \file CstiCoreService.h
/// \brief Declaration of the CstiCoreService class.
///
/// This task class coordinates sending core requests and processes the received core
///  responses and sends the responses back to the application.  See also the
///  CstiCoreRequest and CstiCoreResponse classes.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///

#ifndef CSTICORESERVICE_H
#define CSTICORESERVICE_H

//
// Includes
//
#include "CstiEvent.h"
#include "stiSVX.h"
#include "stiEventMap.h"
#include "CstiCallList.h"
#include "CstiPhoneNumberList.h"
#include "CstiCoreRequest.h"
#include "CstiHTTPTask.h"
#include "CstiCoreResponse.h"
#include "ServiceResponse.h"
#include "CstiVPService.h"
#include "CstiSignal.h"
#include "OfflineCore.h"
#include "ClientAuthenticateResult.h"
#include "Address.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
class CstiStateNotifyService;
class CstiMessageService;
class CstiConferenceService;
class CstiRemoteLoggerService;
typedef struct _IXML_Node IXML_Node;
typedef struct _IXML_Document IXML_Document;
typedef struct _IXML_Element IXML_Element;
typedef struct _IXML_NodeList IXML_NodeList;

//
// Globals
//

//
// Class Declaration
//
class CstiCoreService : public CstiVPService
{
public:

	CstiCoreService () = delete;

	CstiCoreService (
		CstiHTTPTask *pHTTPTask,
		PstiObjectCallback pAppCallback,
		size_t CallbackParam);

	CstiCoreService (const CstiCoreService &other) = delete;
	CstiCoreService (CstiCoreService &&other) = delete;
	CstiCoreService &operator= (const CstiCoreService &other) = delete;
	CstiCoreService &operator= (CstiCoreService &&other) = delete;
	
	virtual stiHResult Initialize (
		const std::string *serviceContacts,
		const std::string &uniqueID,
		ProductType productType);

	~CstiCoreService () override;

	stiHResult StateNotifyServiceSet (
		CstiStateNotifyService *pStateNotifyService);
	
	stiHResult MessageServiceSet (
		CstiMessageService *pMessageService);
	
	stiHResult ConferenceServiceSet (
		CstiConferenceService *pConferenceService);

	stiHResult RemoteLoggerServiceSet (
		CstiRemoteLoggerService *remoteLoggerService);

	//
	// Core Methods
	//
	 stiHResult Startup (
		bool bPause) override;

	stiHResult Login (
		const char *pszPhoneNumber,
		const char *pszPin,
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId,
		const char *pszLoginAs = nullptr);

	stiHResult Logout (
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId);

	void LoggedOutNotifyAsync (const std::string& token);

	void ReauthenticateAsync ();

	stiHResult RequestSend (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId,
		int nPriority = CstiCoreRequest::estiCRP_NORMAL);

	stiHResult RequestSendEx (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId,
		int nPriority = CstiCoreRequest::estiCRP_NORMAL);

	stiHResult RequestSendAsync (
		CstiCoreRequest *poCoreRequest,
		PstiObjectCallback Callback,
		size_t CallbackParam,
		int nPriority = CstiCoreRequest::estiCRP_NORMAL);

	void UserPinSet (
		const char *pszPin);

	int APIMajorVersionGet ()
	{
		return m_nAPIMajorVersion;
	}

	int APIMinorVersionGet ()
	{
		return m_nAPIMinorVersion;
	}

	stiHResult logInUsingCache (
		const std::string& phoneNumber,
		const std::string& pin,
		const std::string& token);

	OfflineCore offlineCore;
	
	bool isOfflineGet () override;

	stiHResult Callback (
		int32_t message,
		CstiVPServiceResponse* response) override;

	CstiSignal<const ServiceResponseSP<ClientAuthenticateResult>&> clientAuthenticateSignal{};
	CstiSignal<const ServiceResponseSP<CstiUserAccountInfo>&> userAccountInfoReceivedSignal{};
	CstiSignal<const ServiceResponseSP<vpe::Address>&> emergencyAddressReceivedSignal{};
	CstiSignal<const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>&> emergencyAddressStatusReceivedSignal{};

private:

	struct SstiAsyncRequest
	{
		CstiCoreRequest* pCoreRequest;
		PstiObjectCallback Callback;
		size_t CallbackParam;
		int nPriority;
	};

	//
	//	Task functions
	//
	stiHResult QueueServiceContactsGet ();

	stiHResult Reauthenticate ();

	void RequestRemoved(
		unsigned int unRequestId) override;
		
	stiHResult RequestRemovedOnError(
		CstiVPServiceRequest *request) override;

	void SSLFailedOver() override;

	std::string m_userPhone;
	std::string m_userPin;
	
	EstiBool m_bAuthenticationInProcess;				// Flag indicating an authentication is in process
	unsigned int m_reauthenticateUserLoginCheckRequestId = 0;

	EstiBool m_bFirstAuthentication;			// Indicates if this is the first eAUTHENTICATED state change

	vpe::EventTimer m_refreshTimer;				// Timer for Clock and Public IP refresh

	unsigned int m_unConnectionId;				// Request id of the most recent Connection Request.

	ProductType m_productType{ProductType::Unknown};
	
	int m_nAPIMajorVersion;
	int m_nAPIMinorVersion;
	
	CstiMessageService *m_pMessageService;
	CstiStateNotifyService *m_pStateNotifyService;
	CstiConferenceService *m_pConferenceService;
	CstiRemoteLoggerService *m_remoteLoggerService;

	CstiSignalConnection::Collection m_signalConnections;

	stiHResult StateSet (EState eState) override;

	void LoggedOutErrorProcess (
		CstiVPServiceRequest *pRequest,
		EstiBool * pbLoggedOutNotified);
	
	CstiCoreResponse::ECoreError NodeErrorGet (
		IXML_Node *pNode,
		EstiBool *pbDatabaseAvailable);
	
	//
	// task handle methods
	//
	stiHResult ShutdownHandle ();
	
	//
	// Other message handlers
	//
	stiHResult RequestSendHandle (SstiAsyncRequest* pAsyncRequest);
		
	stiHResult RefreshHandle ();

	void LoggedOutNotifyHandle (const std::string& token);

	//
	// Utility Methods
	//
	stiHResult PhoneAccountCreate (
		int nRequestPriority);

	void retryOffline (CstiVPServiceRequest *request, const std::function<void ()>& callback) override;
protected:	
	stiHResult CallListProcess (
		CstiCoreResponse * poResponse,
		IXML_Node * pNode);

private:		
#ifdef stiFAVORITES
	stiHResult FavoriteListProcess (
		CstiCoreResponse * poResponse,
		IXML_Node * pNode);
#endif

	stiHResult PhoneNumberListProcess (
		CstiCoreResponse * poResponse,
		IXML_Node * pNode);
		
	stiHResult DirectoryResolveProcess (
		CstiCoreResponse * poResponse,
		IXML_Node * pNode);
		
	void ServiceContactsProcess (
		IXML_Node *pNode,
		const char *pszServiceName,
		std::string ServiceContacts[]);
	
	stiHResult UserAccountInfoProcess (
		CstiUserAccountInfo * userAccountInfo,
		IXML_NodeList * pChildNodes);
		
	stiHResult SettingsGetProcess (
		CstiCoreResponse * poResponse,
		IXML_NodeList * pChildNodes,
		std::list <std::string> *pRequestedSettingsList);

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
	
	stiHResult RelayLanguageListGetProcess (
		CstiCoreResponse * poResponse,
		IXML_NodeList * pChildNodes);

	stiHResult RingGroupInfoGetProcess (
		CstiCoreResponse * poResponse,
		IXML_NodeList * pChildNodes);

	stiHResult RegistrationInfoGetProcess (
		CstiCoreResponse * poResponse,
		IXML_NodeList * pChildNodes);

	stiHResult AgreementStatusGetProcess (
		CstiCoreResponse * poResponse,
		IXML_NodeList * pChildNodes);

	stiHResult ScreenSaverListProcess (
		CstiCoreResponse *poResponse,
		IXML_Node *pScreenSaverListNode);

	stiHResult ProcessClientAuthenticateWhichLogin (
		std::vector<CstiCoreResponse::SRingGroupInfo>& ringGroupInfoList,
		IXML_NodeList * pChildNodes);

	stiHResult Restart () override;

	#ifdef stiDEBUGGING_TOOLS
	EstiBool IsDebugTraceSet (const char **ppszService) const override;
	#endif
	
	const char *ServiceIDGet () const override;
	
	//
	// Access Methods
	//
	void offlineCoreResponseSend (CstiCoreResponse *response);
	void offlineCoreCredentialsUpdate (const std::string& userName, const std::string& password);
	void offlineCoreClientAuthenticate (const ServiceResponseSP<ClientAuthenticateResult>& response);

	template<typename T>
	ServiceResponseSP<T> successfulResponseCreate (unsigned int requestId)
	{
		auto response = std::make_shared <vpe::ServiceResponse<T>> ();
		response->origin = vpe::ResponseOrigin::VPServices;
		response->requestId = requestId;
		response->responseSuccessful = true;
		response->coreErrorNumber = CstiCoreResponse::eNO_ERROR;
		return response;
	}

	template<typename T>
	ServiceResponseSP<T> failureResponseCreate (unsigned int requestId, CstiCoreResponse::ECoreError coreErrorNumber, std::string& coreErrorMessage)
	{
		auto response = std::make_shared <vpe::ServiceResponse<T>> ();
		response->origin = vpe::ResponseOrigin::VPServices;
		response->responseSuccessful = false;
		response->requestId = requestId;
		response->coreErrorNumber = coreErrorNumber;
		response->coreErrorMessage = coreErrorMessage;
		return response;
	}


};

#endif // CSTICORESERVICE_H

// end file CstiCoreService.h
