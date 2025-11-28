/*!
* \file IstiNetwork.h
* \brief Interface for the CstiNetwork class.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2022 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef CSTINETWORK_H
#define CSTINETWORK_H

#include "BaseNetwork.h"
#include "stiError.h"

class CstiNetwork : public BaseNetwork
{
public:
	CstiNetwork ();
	virtual ~CstiNetwork ();
	
	stiHResult Startup () {return stiRESULT_SUCCESS;};
	stiHResult SettingsGet (SstiNetworkSettings *) const {return stiRESULT_SUCCESS;};
	stiHResult SettingsSet (
		const SstiNetworkSettings *pstSettings,
		unsigned int *punRequestId) {return stiRESULT_SUCCESS;};
	void InCallStatusSet (bool bSet) {};
	void networkInfoSet (
		NetworkType type,
		const std::string &data) {};
	
	NetworkType networkTypeGet () const {return NetworkType::Wired;};
	std::string networkDataGet () const {return "";};
	std::string localIpAddressGet(EstiIpAddressType addressType, const std::string& destinationIpAddress) const;
};
	
#endif // #ifndef CSTINETWORK_H
