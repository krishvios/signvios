#include "BaseNetwork.h"

ISignal<IstiNetwork::EstiState>& BaseNetwork::networkStateChangedSignalGet ()
{
	return networkStateChangedSignal;
}

ISignal<bool>& BaseNetwork::wirelessScanCompleteSignalGet ()
{
	return wirelessScanCompleteSignal;
}

ISignal<>& BaseNetwork::networkSettingsChangedSignalGet ()
{
	return networkSettingsChangedSignal;
}

ISignal<>& BaseNetwork::networkSettingsErrorSignalGet ()
{
	return networkSettingsErrorSignal;
}

ISignal<>& BaseNetwork::networkSettingsSetSignalGet ()
{
	return networkSettingsSetSignal;
}

ISignal<>& BaseNetwork::wiredNetworkDisconnectedSignalGet ()
{
	return wiredNetworkDisconnectedSignal;
}

ISignal<>& BaseNetwork::wirelessNetworkDisconnectedSignalGet ()
{
	return wirelessNetworkDisconnectedSignal;
}

ISignal<>& BaseNetwork::enterpriseNetworkSignalGet ()
{
	return enterpriseNetworkSignal;
}

ISignal<std::string>& BaseNetwork::networkNeedsCredentialsSignalGet ()
{
	return networkNeedsCredentialsSignal;
}

ISignal<>& BaseNetwork::wirelessUpdatedSignalGet ()
{
	return wirelessUpdatedSignal;
}

ISignal<>& BaseNetwork::networkUnsupportedSignalGet ()
{
	return networkUnsupportedSignal;
}

ISignal<>& BaseNetwork::wiredNetworkConnectedSignalGet ()
{
	return wiredNetworkConnectedSignal;
}

ISignal<>& BaseNetwork::connmanResetSignalGet ()
{
	return connmanResetSignal;
}

ISignal<>& BaseNetwork::wirelessPassphraseChangedSignalGet ()
{
	return wirelessPassphraseChangedSignal;
}
