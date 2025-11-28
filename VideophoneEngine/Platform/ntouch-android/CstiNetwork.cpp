#include "CstiNetwork.h"
#import "stiTools.h"

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

std::string CstiNetwork::localIpAddressGet(EstiIpAddressType addressType,
											const std::string &destinationIpAddress) const {
    std::string localIpAddress;
    stiGetLocalIp(&localIpAddress, addressType);
    return localIpAddress;
}

void CstiNetwork::networkInfoSet (NetworkType type, const std::string & data)
{
	networkType = type;
	networkData = data;
}

NetworkType CstiNetwork::networkTypeGet () const
{
	return networkType;
}

std::string CstiNetwork::networkDataGet () const
{
	return networkData;
}
