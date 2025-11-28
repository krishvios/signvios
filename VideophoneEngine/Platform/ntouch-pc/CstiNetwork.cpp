#include "CstiNetwork.h"
#include <iphlpapi.h>


std::string addressToString(LPSOCKADDR address);

stiHResult CstiNetwork::Startup ()
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiNetwork::SettingsGet (SstiNetworkSettings *) const
{
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

stiHResult CstiNetwork::SettingsSet (const SstiNetworkSettings * pstSettings, unsigned int * punRequestId)
{
	return stiRESULT_SUCCESS;
}

void CstiNetwork::InCallStatusSet (bool bSet)
{
}

void CstiNetwork::networkInfoSet (NetworkType type, const std::string & data)
{
}

NetworkType CstiNetwork::networkTypeGet () const
{
	auto hResult = stiRESULT_SUCCESS;
	NetworkType networkType = NetworkType::None;
	std::unique_ptr<byte[]> addressBuffer (new byte[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;

	GetAdapterAddresses (estiTYPE_IPV4, addressBuffer, addressList);
	stiTESTCOND (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();

	switch (preferredAdapterAddress->IfType)
	{
		case IF_TYPE_ETHERNET_CSMACD:
			networkType = NetworkType::Wired;
			break;

		case IF_TYPE_IEEE80211:
			networkType = NetworkType::WiFi;
			break;

		case IF_TYPE_WWANPP:
		case IF_TYPE_WWANPP2:
			networkType = NetworkType::Cellular;
			break;
	}

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	return networkType;
}

std::string CstiNetwork::networkDataGet () const
{
	std::string networkData;

	NetworkType networkType = NetworkType::None;
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_ptr<byte[]> addressBuffer (new byte[ADAPTER_ADDRESSES_BUFFER_SIZE]);
	PIP_ADAPTER_ADDRESSES addressList = nullptr;
	PIP_ADAPTER_ADDRESSES preferredAdapterAddress = nullptr;

	GetAdapterAddresses (estiTYPE_IPV4, addressBuffer, addressList);
	stiTESTCOND (addressList, stiRESULT_ERROR);

	hResult = GetPreferredAdapterAddress (addressList, preferredAdapterAddress, "");
	stiTESTRESULT ();
	
	constexpr uint64_t Mb = 1000000;
	auto tx_Mb = preferredAdapterAddress->TransmitLinkSpeed / Mb;
	auto rx_Mb = preferredAdapterAddress->ReceiveLinkSpeed / Mb;
	networkData = std::string ("NetworkType: ") + std::to_string (preferredAdapterAddress->IfType) +
		", TxMb: " + std::to_string (tx_Mb) +
		", RxMb: " + std::to_string (rx_Mb);

STI_BAIL:
	addressList = nullptr;
	preferredAdapterAddress = nullptr;

	return networkData;
}

std::string CstiNetwork::localIpAddressGet(EstiIpAddressType ipAddressType, const std::string& remoteIpAddress) const
{
	auto hResult = stiRESULT_SUCCESS;
	std::string localIpAddress;

	PIP_ADAPTER_ADDRESSES addressList{};
	PIP_ADAPTER_ADDRESSES preferredAddress{};
	std::unique_ptr<byte[]> buffer;

	GetAdapterAddresses(ipAddressType, buffer, addressList);

	GetPreferredAdapterAddress(addressList, preferredAddress, remoteIpAddress);

	if (preferredAddress != nullptr)
	{
		localIpAddress = addressToString(preferredAddress->FirstUnicastAddress->Address.lpSockaddr);
	}

	return localIpAddress;
}

std::string addressToString(LPSOCKADDR address)
{
	auto ip_address = address->sa_data;
	if (address->sa_family == AF_INET)
	{
		ip_address += 2;
	}
	char buffer[INET6_ADDRSTRLEN];
	inet_ntop(address->sa_family, ip_address, buffer, sizeof(buffer));
	return buffer;
}
