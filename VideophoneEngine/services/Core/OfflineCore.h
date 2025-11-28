///
/// \file OfflineCore.h
/// \brief Declaration of the OfflineCore class.
///
/// This class returns Core responses based on cached data the endpoint already
///    has or default values.  This is used by Core service when the endpoint
///    cannot connect to Core.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2018-2018 by Sorenson Communications, Inc. All rights reserved.
///

#pragma once
#include <cstring>
#include <functional>
#include <boost/property_tree/ptree.hpp>

#include "CstiCoreRequest.h"
#include "CstiCoreResponse.h"
#include "CstiSignal.h"
#include "CstiEventQueue.h"
#include "stiOSMutex.h"
#include "SstiFavoriteList.h"
#include "Address.h"
#include "ServiceResponse.h"

struct SstiRegistrationInfo;
struct ClientAuthenticateResult;

class OfflineCore : public CstiEventQueue
{
public:
	OfflineCore ();
	~OfflineCore () = default;

	void login (const std::string &phoneNumber, const std::string &pin, unsigned int *punRequestId);
	void handleRequest (CstiCoreRequest *request, const std::function<void ()>& callback);
	void enable ();
	void disable ();
	
	bool enabledGet ();

	CstiSignal<CstiCoreResponse*> sendCoreResponse;
	CstiSignal<const ServiceResponseSP<ClientAuthenticateResult>&> clientAuthenticateSignal;
	CstiSignal<const ServiceResponseSP<CstiUserAccountInfo>&> userAccountInfoReceivedSignal;
	CstiSignal<const ServiceResponseSP<vpe::Address>&> emergencyAddressReceivedSignal;
	CstiSignal<const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>&> emergencyAddressStatusReceivedSignal;
	CstiSignal<const std::string&, const std::string&> credentialsUpdate;
	std::function<int()> nextRequestIdGet;
	std::function<bool (const std::string&, const std::string&)> authenticate;
	std::function<void(SstiRegistrationInfo&)> registrationInfoGet;
	std::function<void(SstiUserPhoneNumbers *phoneNumbers)> userPhoneNumbersGet;
	std::function<void(CstiContactList*)> contactListGet;
	std::function<void (SstiFavoriteList*)> favoriteListGet;
	std::function<void(CstiCallList*,const CstiCallList::EType)> callListGet;
	std::function<void(CstiCallList*)> blockListGet;
	std::function<std::string(const std::string&)> propertyStringGet;
	std::function<int(const std::string&)> propertyIntegerGet;
	std::function<void (const std::string&)> remoteLog;
	std::function<void (const std::string&)> remoteLoggerPhoneNumberSet;
	std::function<bool (CstiUserAccountInfo&)> userAccountInfoGet;
	std::function<bool (vpe::Address&)> emergencyAddressGet;
	std::function<bool (EstiEmergencyAddressProvisionStatus&)> emergencyAddressStatusGet;
	std::function<void (std::vector<CstiCoreResponse::SRingGroupInfo>&)> myPhoneGroupListGet;

private:
	struct UserIdentity
	{
		std::string phoneNumber;
		std::string pin;
	};
	void requestParse (CstiCoreRequest *request, const std::string &responseString);
	void nodeProcess (CstiCoreRequest *request, boost::property_tree::ptree::value_type &node);
	std::string callListTypeParse (boost::property_tree::ptree::value_type &node);
	UserIdentity userIdentityParse (boost::property_tree::ptree::value_type &node);
	CstiCoreResponse::EResponse responseTypeDetermine (const std::string& nodeName);
	bool m_enabledState = false;
};

