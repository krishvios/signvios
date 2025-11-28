#pragma once
#include "CstiSignal.h"
#include "IstiNetwork.h"

class BaseNetwork : virtual public IstiNetwork
{
public:
	BaseNetwork () = default;
	~BaseNetwork () override = default;

	BaseNetwork (const BaseNetwork &) = delete;
	BaseNetwork (BaseNetwork &&) = delete;
	BaseNetwork &operator= (const BaseNetwork &) = delete;
	BaseNetwork &operator= (BaseNetwork &&) = delete;

	ISignal<IstiNetwork::EstiState>& networkStateChangedSignalGet () override;
	ISignal<bool>& wirelessScanCompleteSignalGet () override;
	ISignal<>& networkSettingsChangedSignalGet () override;
	ISignal<>& networkSettingsErrorSignalGet () override;
	ISignal<>& networkSettingsSetSignalGet () override;
	ISignal<>& wiredNetworkDisconnectedSignalGet () override;
	ISignal<>& wirelessNetworkDisconnectedSignalGet () override;
	ISignal<>& enterpriseNetworkSignalGet () override;
	ISignal<std::string>& networkNeedsCredentialsSignalGet () override;
	ISignal<>& wirelessUpdatedSignalGet () override;
	ISignal<>& networkUnsupportedSignalGet () override;
	ISignal<>& wiredNetworkConnectedSignalGet () override;
	ISignal<>& connmanResetSignalGet () override;
	ISignal<>& wirelessPassphraseChangedSignalGet () override;

	CstiSignal<EstiState> networkStateChangedSignal;
	CstiSignal<bool> wirelessScanCompleteSignal;
	CstiSignal<> networkSettingsChangedSignal;
	CstiSignal<> networkSettingsErrorSignal;
	CstiSignal<> networkSettingsSetSignal;
	CstiSignal<> wiredNetworkDisconnectedSignal;
	CstiSignal<> wirelessNetworkDisconnectedSignal;
	CstiSignal<> enterpriseNetworkSignal;
	CstiSignal<std::string> networkNeedsCredentialsSignal;
	CstiSignal<> wirelessUpdatedSignal;
	CstiSignal<> networkUnsupportedSignal;
	CstiSignal<> wiredNetworkConnectedSignal;
	CstiSignal<> connmanResetSignal;
	CstiSignal<> wirelessPassphraseChangedSignal;
};

