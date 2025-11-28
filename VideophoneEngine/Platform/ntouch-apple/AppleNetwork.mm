//
//  AppleNetwork.m
//  ntouch
//
//  Created by DBaker on 2/5/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

#include "AppleNetwork.h"
#import "stiTools.h"
#if APPLICATION == APP_NTOUCH_IOS
#import "AppDelegate.h"
#elif APPLICATION == APP_NTOUCH_MAC
#import "SCIAppDelegate.h"
#define appDelegate [SCIAppDelegate sharedDelegate]
#endif

namespace vpe
{
	AppleNetwork::AppleNetwork ()
	{
	}
	
	AppleNetwork::~AppleNetwork()
	{
	}
	
	stiHResult AppleNetwork::Startup ()
	{
		m_started = true;
		return stiRESULT_SUCCESS;
	}
	
	stiHResult AppleNetwork::CallbackRegister (
		PstiObjectCallback callback,
		size_t CallbackParam,
		size_t CallbackFromId)
	{
		m_callback = callback;
		m_CallbackParam = CallbackParam;
		return stiRESULT_SUCCESS;
	}
	
	void AppleNetwork::networkInfoSet (
		NetworkType type,
		const std::string &data)
	{
		m_type = type;
		m_data = data;
	}

	std::string AppleNetwork::localIpAddressGet(EstiIpAddressType addressType, const std::string& destinationIpAddress) const
	{
		std::string localIpAddress;
		stiGetLocalIp(&localIpAddress, addressType);
		return localIpAddress;
	}
	
	NetworkType AppleNetwork::networkTypeGet () const
	{
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_MAC
		[appDelegate networkDataToLog];
#endif
		return m_type;
	}
	
	std::string AppleNetwork::networkDataGet () const
	{
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_MAC
		[appDelegate networkDataToLog];
#endif
		return m_data;
	}
}
