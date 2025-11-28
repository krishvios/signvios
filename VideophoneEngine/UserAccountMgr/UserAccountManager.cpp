#include "UserAccountManager.h"
#include "CstiString.h"
#include "UserAccountListItem.h"
#include "CstiCoreService.h"
#include "CstiStateNotifyResponse.h"
#include "ClientAuthenticateResult.h"
#include <sstream>


UserAccountManager::UserAccountManager (CstiCoreService* coreService) :
	m_coreService(coreService),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet())
{
	std::string dynamicDataFolder;
	stiOSDynamicDataFolderGet (&dynamicDataFolder);
	std::stringstream filePath;
	filePath << dynamicDataFolder << m_fileName;
	SetFilename (filePath.str ());

	m_userAccountListItem = std::make_shared<UserAccountListItem> (m_userAccountInfo, m_emergencyAddress, m_addressStatus);

	m_signalConnections.push_back (m_coreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& authenticateResponse)
	{
		m_eventQueue.PostEvent (std::bind (&UserAccountManager::clientAuthenticateHandle, this, authenticateResponse));
	}));

	m_signalConnections.push_back (m_coreService->userAccountInfoReceivedSignal.Connect ([this](const ServiceResponseSP<CstiUserAccountInfo>& userAccountResponse)
	{
		m_eventQueue.PostEvent (std::bind (&UserAccountManager::userAccountInfoResponseHandle, this, userAccountResponse));
	}));

	m_signalConnections.push_back (m_coreService->emergencyAddressReceivedSignal.Connect ([this](const ServiceResponseSP<vpe::Address>& addressResponse)
	{
		m_eventQueue.PostEvent (std::bind (&UserAccountManager::emergencyAddressResponseHandle, this, addressResponse));
	}));

	m_signalConnections.push_back (m_coreService->emergencyAddressStatusReceivedSignal.Connect ([this](const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>& addressStatusResponse)
	{
		m_eventQueue.PostEvent (std::bind (&UserAccountManager::emergencyAddressStatusResponseHandle, this, addressStatusResponse));
	}));

	auto hResult = init ();
	m_loadedFromFile = !stiIS_ERROR (hResult);

	// This implementation of the XML manager does not store individual items.  Instead it uses one
	// object and writes out all of its properties as nodes.  When the nodes are read in, it creates
	// a list of empty items that need to be cleared and the userAccountListItem added instead.
	m_pdoc->ListFree ();
	m_pdoc->ItemAdd (m_userAccountListItem);
}

CstiUserAccountInfo UserAccountManager::userAccountInfoGet ()
{
	return m_eventQueue.executeAndWait<CstiUserAccountInfo> ([this]() { return m_userAccountInfo; });
}

vpe::Address UserAccountManager::emergencyAddressGet ()
{
	return m_eventQueue.executeAndWait<vpe::Address> ([this]() { return m_emergencyAddress; });
}

EstiEmergencyAddressProvisionStatus UserAccountManager::addressStatusGet ()
{
	return m_eventQueue.executeAndWait<EstiEmergencyAddressProvisionStatus> ([this]() { return m_addressStatus; });
}

bool UserAccountManager::cacheAvailable ()
{
	return m_loadedFromFile && m_userAccountListItem->loadedRequiredValues();
}


void UserAccountManager::clientLoggedOutHandle ()
{
	m_authenticated = false;
	m_userAccountInfo = {};
	m_addressStatus = {};
	m_emergencyAddress = {};
	m_pdoc->ListFree ();
	startSaveDataTimer ();
}

bool UserAccountManager::coreResponseHandle (CstiCoreResponse* response)
{
	auto responseValue = response->ResponseGet ();
	m_eventQueue.PostEvent ([this, responseValue]() {
		switch (responseValue)
		{
			case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
			{
				clientLoggedOutHandle ();
				break;
			}
			default:
			{
				break;
			}
		}
	});

	return false;
}

void UserAccountManager::stateNotifyEventHandle (CstiStateNotifyResponse* response)
{
	auto responseValue = response->ResponseGet ();
	m_eventQueue.PostEvent ([this, responseValue]() {
		switch (responseValue)
		{
			case CstiStateNotifyResponse::eLOGGED_OUT:
			case CstiStateNotifyResponse::ePASSWORD_CHANGED:
			{
				clientLoggedOutHandle ();
				break;
			}
			case CstiStateNotifyResponse::eEMERGENCY_ADDRESS_UPDATE_VERIFY:
			case CstiStateNotifyResponse::eEMERGENCY_PROVISION_STATUS_CHANGE:
			{
				if (m_authenticated)
				{
					emergencyAddressRequestSend ();
				}
				else
				{
					m_pendingAddressRequest = true;
				}
				break;
			}
			default:
			{
				break;
			}
		}
	});
}

void UserAccountManager::emergencyAddressRequestSend ()
{
	m_pendingAddressRequest = false;
	auto request = new CstiCoreRequest ();
	request->emergencyAddressProvisionStatusGet ();
	request->emergencyAddressGet ();
	m_coreService->RequestSend (request, nullptr);
}


WillowPM::XMLListItemSharedPtr UserAccountManager::NodeToItemConvert (IXML_Node* node)
{
	m_userAccountListItem->valueSet (node);
	return nullptr;
}

void UserAccountManager::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& authenticateResponse)
{
	m_authenticated = authenticateResponse->responseSuccessful && authenticateResponse->coreErrorNumber == 0;
	if (m_authenticated && m_pendingAddressRequest)
	{
		emergencyAddressRequestSend ();
	}
}

void UserAccountManager::userAccountInfoResponseHandle (const ServiceResponseSP<CstiUserAccountInfo>& userAccountInfoResponse)
{
	if (userAccountInfoResponse->responseSuccessful && userAccountInfoResponse->origin == vpe::ResponseOrigin::VPServices)
	{
		m_eventQueue.PostEvent ([this, userAccountInfoResponse] () {
			m_pdoc->ListFree ();
			m_pdoc->ItemAdd (m_userAccountListItem);
			m_userAccountInfo = userAccountInfoResponse->content;
			startSaveDataTimer ();
		});
	}
}

void UserAccountManager::emergencyAddressResponseHandle (const ServiceResponseSP<vpe::Address>& addressResponse)
{
	if (addressResponse->responseSuccessful && addressResponse->origin == vpe::ResponseOrigin::VPServices)
	{
		m_userAccountListItem->emergencyAddressSet (addressResponse);
		startSaveDataTimer ();
	}
}

void UserAccountManager::emergencyAddressStatusResponseHandle (const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>& addressStatusResponse)
{
	if (addressStatusResponse->responseSuccessful && addressStatusResponse->origin == vpe::ResponseOrigin::VPServices)
	{
		m_userAccountListItem->addressStatusSet (addressStatusResponse->content);
		startSaveDataTimer ();
	}
}

void UserAccountManager::userPinSet (const std::string& pin)
{
	userPinSet (pin, nullptr);
}

void UserAccountManager::userPinSet (const std::string& pin, CoreResponseCallback responseCallback)
{
	m_eventQueue.PostEvent ([this, pin, responseCallback]() {
		auto request = new CstiCoreRequest ();
		request->userPinSet (pin);
		if (responseCallback)
		{
			request->callbackAdd<CstiCoreResponse> (responseCallback, responseCallback);
		}
		m_coreService->RequestSend (request, nullptr);
	});
}

void UserAccountManager::profileImageIdSet (const std::string& imageId)
{
	profileImageIdSet (imageId, nullptr);
}

void UserAccountManager::profileImageIdSet (const std::string& imageId, CoreResponseCallback responseCallback)
{
	m_eventQueue.PostEvent ([this, imageId, responseCallback]() {
		auto request = new CstiCoreRequest ();
		request->profileImageIdSet (imageId);
		if (responseCallback)
		{
			request->callbackAdd<CstiCoreResponse> (responseCallback, responseCallback);
		}
		m_coreService->RequestSend (request, nullptr);

		// Update the profile image ID in the cache
		m_userAccountInfo.m_ImageId = imageId;
		startSaveDataTimer ();
		});
}

void UserAccountManager::signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary)
{
	signMailEmailPreferencesSet (enableSignMailNotifications, emailPrimary, emailSecondary, nullptr);
}

void UserAccountManager::signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary, CoreResponseCallback responseCallback)
{
	m_eventQueue.PostEvent ([this, enableSignMailNotifications, emailPrimary, emailSecondary, responseCallback]() {
		auto request = new CstiCoreRequest ();
		request->signMailEmailPreferencesSet (enableSignMailNotifications, emailPrimary, emailSecondary);
		if (responseCallback)
		{
			request->callbackAdd<CstiCoreResponse> (responseCallback, responseCallback);
		}
		m_coreService->RequestSend (request, nullptr);

		// Update SignMail email prefs in the cache
		m_userAccountInfo.m_SignMailEnabled = enableSignMailNotifications ? "True" : "False";
		m_userAccountInfo.m_EmailMain = emailPrimary;
		m_userAccountInfo.m_EmailPager = emailSecondary;
		startSaveDataTimer ();
	});
}

void UserAccountManager::userRegistrationInfoSet (const std::string& birthDate, const std::string& lastFourSSN, bool hasIdentification)
{
	userRegistrationInfoSet (birthDate, lastFourSSN, hasIdentification, nullptr);
}

void UserAccountManager::userRegistrationInfoSet (const std::string& birthDate, const std::string& lastFourSSN, bool hasIdentification, CoreResponseCallback responseCallback)
{
	m_eventQueue.PostEvent ([this, birthDate, lastFourSSN, hasIdentification, responseCallback] {
		auto request = new CstiCoreRequest ();
		request->userRegistrationInfoSet (birthDate, lastFourSSN, hasIdentification);
		if (responseCallback)
		{
			request->callbackAdd<CstiCoreResponse> (responseCallback, responseCallback);
		}
		m_coreService->RequestSend (request, nullptr);
	});
}

