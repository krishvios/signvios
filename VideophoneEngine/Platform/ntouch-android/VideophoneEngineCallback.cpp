#include "VideophoneEngineCallback.h"

void VideophoneEngineCallback::OnSharedContactReceivedMessage(const SstiSharedContact &contact) const {}
void VideophoneEngineCallback::OnBlockListErrorMessage(IstiBlockListManager::Response response, stiHResult result) const{}
void VideophoneEngineCallback::OnContactErrorMessage(IstiContactManager::Response response, stiHResult result) const{}
void VideophoneEngineCallback::OnFavoritesErrorMessage(IstiContactManager::Response response, stiHResult result) const{}
void VideophoneEngineCallback::OnMessage(std::shared_ptr<vpe::ServiceOutageMessage> message) const {}
void VideophoneEngineCallback::OnClientAuthenticateMessage(std::shared_ptr<vpe::ServiceResponse<ClientAuthenticateResult>> message) const {}
void VideophoneEngineCallback::OnUserAccountInfoMessage(std::shared_ptr<vpe::ServiceResponse<CstiUserAccountInfo>> message) const {}
void VideophoneEngineCallback::OnEmergencyAddressReceivedMessage(std::shared_ptr<vpe::ServiceResponse<vpe::Address>> message) const {}
void VideophoneEngineCallback::OnEmergencyAddressProvisionStatusMessage(std::shared_ptr<vpe::ServiceResponse<EstiEmergencyAddressProvisionStatus>> message) const {}
void VideophoneEngineCallback::OnDHVChangedMessage(IstiCall::DhviState state) const {}
void VideophoneEngineCallback::OnDeviceLocationTypeChanged(DeviceLocationType locationType) const {}
void VideophoneEngineCallback::OnFilePlayStateChanged(IstiMessageViewer::EState state) const {}
