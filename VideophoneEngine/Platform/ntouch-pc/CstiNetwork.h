#pragma once
#include "BaseNetwork.h"
class CstiNetwork :	public BaseNetwork
{
public:
	// Inherited via IstiNetwork
	stiHResult Startup () override;
	stiHResult SettingsGet (SstiNetworkSettings *) const override;
	stiHResult SettingsSet (const SstiNetworkSettings * pstSettings, unsigned int * punRequestId) override;
	void InCallStatusSet (bool bSet) override;
	void networkInfoSet (NetworkType type, const std::string & data) override;
	NetworkType networkTypeGet () const override;
	std::string networkDataGet () const override;
	std::string localIpAddressGet(EstiIpAddressType ipAddressType, const std::string& remoteIpAddress) const override;
};

