#pragma once

#include <memory>
#include <VideophoneEngine/cci/IstiVideophoneEngine.h>
#include <VideophoneEngine/ConfMgr/IstiCall.h>
#include <VideophoneEngine/common/stiDefs.h>
#include <VideophoneEngine/BlockListMgr/IstiBlockListManager.h>
#include <VideophoneEngine/contacts/IstiContactManager.h>
#include <VideophoneEngine/services/ServiceOutage/ServiceOutageMessage.h>
#include <VideophoneEngine/services/Core/ClientAuthenticateResult.h>
#include <VideophoneEngine/services/ServiceResponse.h>
#include "IstiMessageViewer.h"

class VideophoneEngineCallback {
public:
	virtual void OnSharedContactReceivedMessage(const SstiSharedContact &contact) const;
	virtual void OnMessage(std::shared_ptr<vpe::ServiceOutageMessage> message) const;
	virtual void OnContactErrorMessage(IstiContactManager::Response response, stiHResult result) const;
	virtual void OnFavoritesErrorMessage(IstiContactManager::Response response, stiHResult result) const;
	virtual void OnBlockListErrorMessage(IstiBlockListManager::Response response, stiHResult result) const;
	virtual void OnClientAuthenticateMessage(std::shared_ptr<vpe::ServiceResponse<ClientAuthenticateResult>> message) const;
	virtual void OnUserAccountInfoMessage(std::shared_ptr<vpe::ServiceResponse<CstiUserAccountInfo>> message) const;
	virtual void OnEmergencyAddressReceivedMessage(std::shared_ptr<vpe::ServiceResponse<vpe::Address>> message) const;
	virtual void OnEmergencyAddressProvisionStatusMessage(std::shared_ptr<vpe::ServiceResponse<EstiEmergencyAddressProvisionStatus>> message) const;
	virtual void OnDHVChangedMessage(IstiCall::DhviState state) const;
	virtual void OnDeviceLocationTypeChanged(DeviceLocationType locationType) const;
	virtual void OnFilePlayStateChanged(IstiMessageViewer::EState state) const;
};
