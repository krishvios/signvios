/*!
* \file ServiceOutageClient.h
* \brief See ServiceOutageClient.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2017 - 2018 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once
#include <functional>
#include <Poco/URI.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/HTTPSClientSession.h>
#include "CstiEventQueue.h"
#include "CstiTimer.h"
#include "CstiSignal.h"
#include "ServiceOutageMessage.h"


namespace vpe
{
	class ServiceOutageClient : CstiEventQueue
	{
	public:
		ServiceOutageClient ();

		~ServiceOutageClient () override = default;
		
		void initialize ();
		void forceCheckForOutage ();
		void shutdown ();
		void urlSet (std::string url);
		void timerIntervalSet (int milliseconds);
		void enabledSet (bool enable);
		bool enabledGet ();

		void publicIpAddressSet (std::string publicIpAddress);

		CstiSignal<std::shared_ptr<ServiceOutageMessage>> outageMessageReceived;

		static constexpr int DefaultTimerInterval = 300000;

	protected:


	private:
		void checkForOutage ();
		stiHResult getOutageResponse (const std::string &url, std::string &errMessage, int attempts=3);
		std::string urlGet ();
		int timerIntervalGet ();
		stiHResult messageParse (std::string responseText);

		static const std::string RootElementName;
		static const std::string TextElementName;
		static const std::string TypeElementName;
		static const std::string MessageType_Error;
		static const std::string MessageType_None;

		std::unique_ptr<Poco::Net::HTTPClientSession> clientSessionGet (const Poco::URI &uri);
		EventTimer m_timer;
		CstiSignalConnection m_timerSignalConnection;
		std::string m_messageText;
		std::string m_url = stiSERVICE_OUTAGE_SERVER_DEFAULT;
		int m_timerInterval = DefaultTimerInterval;
		std::string m_publicIpAddress;
		bool m_isEnabled = true;
	};
}
