/*!
 * \file DhvClient.h
 * \brief See DhvClient.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 - 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once
#include "stiSVX.h"
#include "CstiEventQueue.h"
#include <Poco/URI.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/HTTPSClientSession.h>

namespace vpe
{
	class DhvClient : CstiEventQueue
	{
	public:
		DhvClient ();
		~DhvClient () override;
		
		DhvClient (const DhvClient &other) = delete;
		DhvClient (DhvClient &&other) = delete;
		DhvClient &operator= (const DhvClient &other) = delete;
		DhvClient &operator= (DhvClient &&other) = delete;

		void urlSet (const std::string &url);
		void apiKeySet (const std::string &key);
		void dhvEnabledSet (bool value) { m_dhvEnabled = value; };
		bool dhvEnabledGet () { return m_dhvEnabled; };
		void dhvCapableGet (CstiCallSP call, const std::string &phoneNumber);
		void dhvStartCall (
			CstiCallSP call,
			const std::string &toPhoneNumber,
			const std::string &fromPhoneNumber,
			const std::string &displayName,
			const std::string &uri);

		static constexpr const char* INVITE_REQUEST = "/subscriptions/video-invite";
		static constexpr const char* CAPABLE_REQUEST = "/subscriptions/";
	private:
		std::string urlGet ();
		std::string apiKeyGet ();
		std::unique_ptr<Poco::Net::HTTPClientSession> clientSessionGet (const Poco::URI &uri);
		std::string m_url = stiDHV_API_URL_DEFAULT;
		std::string m_apiKey = stiDHV_API_KEY_DEFAULT;
		bool m_dhvEnabled {};
	};
}
