//
//  AppleNetwork.h
//  ntouch
//
//  Created by DBaker on 2/5/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

#pragma once

#include "BaseNetwork.h"

namespace vpe
{
	class AppleNetwork : public BaseNetwork
	{
	public:
		AppleNetwork ();
		~AppleNetwork ();
		stiHResult Startup () override;
		EstiState StateGet () const {return estiUNKNOWN;}
		stiHResult SettingsGet (SstiNetworkSettings *) const override {return stiRESULT_SUCCESS;}
		stiHResult SettingsSet (
			const SstiNetworkSettings *pstSettings,
			unsigned int *punRequestId) override {return stiRESULT_SUCCESS;}
		void InCallStatusSet (bool bSet) override {}
		stiHResult CallbackRegister (
			PstiObjectCallback callback,
			size_t CallbackParam,
			size_t CallbackFromId);
		stiHResult CallbackUnregister (
			PstiObjectCallback callback,
			size_t CallbackParam) {return stiRESULT_SUCCESS;}
		void networkInfoSet (
			NetworkType type,
			const std::string &data) override;
		std::string localIpAddressGet(EstiIpAddressType addressType, const std::string& destinationIpAddress) const override;
		NetworkType networkTypeGet () const override;
		std::string networkDataGet () const override;
		
	private:
		PstiObjectCallback m_callback;
		size_t m_CallbackParam;
		NetworkType m_type = NetworkType::None;
		std::string m_data;
		bool m_started = false;
	};
	
	using AppleNetworkSP = std::shared_ptr<AppleNetwork>;
}
