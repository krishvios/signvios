#pragma once
#include "XMLManager.h"
#include "XMLList.h"
#include "CstiUserAccountInfo.h"
#include "CstiSignal.h"
#include "UserAccountListItem.h"
#include "CstiEventQueue.h"
#include "ServiceResponse.h"
#include "Address.h"
#include "IUserAccountManager.h"

class CstiCoreService;
class CstiCoreResponse;
class CstiStateNotifyResponse;
struct ClientAuthenticateResult;

class UserAccountManager : public WillowPM::XMLManager, public IUserAccountManager
{
public:
	UserAccountManager (CstiCoreService* coreService);

	CstiUserAccountInfo userAccountInfoGet ();
	vpe::Address emergencyAddressGet ();
	EstiEmergencyAddressProvisionStatus addressStatusGet ();

	bool cacheAvailable ();
	bool coreResponseHandle (CstiCoreResponse* response);
	void stateNotifyEventHandle (CstiStateNotifyResponse* response);

private:
	const char* m_fileName = "User.xml";

	WillowPM::XMLListItemSharedPtr NodeToItemConvert (IXML_Node* pNode) override;
	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& authenticateResponse);
	void clientLoggedOutHandle ();
	void userAccountInfoResponseHandle (const ServiceResponseSP<CstiUserAccountInfo>& userAccountInfoResponse);
	void emergencyAddressResponseHandle (const ServiceResponseSP<vpe::Address>& addressResponse);
	void emergencyAddressStatusResponseHandle (const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>& addressStatusResponse);
	void emergencyAddressRequestSend ();

	void userPinSet (const std::string& pin) override;
	void userPinSet (const std::string& pin, CoreResponseCallback coreResponseCallback) override;

	void profileImageIdSet (const std::string& imageID) override;
	void profileImageIdSet (const std::string& imageID, CoreResponseCallback coreResponseCallback) override;

	void signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary) override;
	void signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary, CoreResponseCallback coreResponseCallback) override;

	void userRegistrationInfoSet (const std::string& birthDate, const std::string& lastFourSSN, bool hasIdentification) override;
	void userRegistrationInfoSet (const std::string& birthDate, const std::string& lastFourSSN, bool hasIdentification, CoreResponseCallback coreResponseCallback) override;

	std::shared_ptr<UserAccountListItem> m_userAccountListItem;
	CstiCoreService* m_coreService = nullptr;
	CstiUserAccountInfo m_userAccountInfo{};
	vpe::Address m_emergencyAddress{};
	EstiEmergencyAddressProvisionStatus m_addressStatus{};

	bool m_loadedFromFile{};
	CstiEventQueue& m_eventQueue;
	CstiSignalConnection::Collection m_signalConnections;
	bool m_authenticated = false;
	bool m_pendingAddressRequest = false;
};
