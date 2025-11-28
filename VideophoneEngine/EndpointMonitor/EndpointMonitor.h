/*!
 * \file EndpointMonitor.h
 * \brief See EndpointMonitor.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once
#include "stiSVX.h"
#include "CstiEventQueue.h"
#include <Poco/URI.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/WebSocket.h>
#include "Poco/JSON/Parser.h"
#include "stiSipStatistics.h"

struct SstiStateChange;
struct SstiCallStatistics;

namespace vpe
{
	class WebSocket : public Poco::Net::WebSocket
	{
	public:
		WebSocket (Poco::Net::HTTPClientSession& session, Poco::Net::HTTPRequest& request, Poco::Net::HTTPResponse& response)
		:
			Poco::Net::WebSocket (session, request, response)
		{
		}

		~WebSocket () override = default;

		WebSocket (const WebSocket &other) = delete;
		WebSocket (WebSocket &&other) = delete;
		WebSocket &operator= (const WebSocket &other) = delete;
		WebSocket &operator= (WebSocket &&other) = delete;

		int sockfd ()
		{
			return Poco::Net::WebSocket::sockfd();
		}

		void shutdown ()
		{
			Poco::Net::WebSocket::shutdown ();
			m_shutdown = true;
		}

		bool shutdownGet () {return m_shutdown;};

	private:
		bool m_shutdown {false};
	};

	class EndpointMonitor : CstiEventQueue
	{
	public:
		EndpointMonitor ();
		~EndpointMonitor () override;

		EndpointMonitor (const EndpointMonitor &other) = delete;
		EndpointMonitor (EndpointMonitor &&other) = delete;
		EndpointMonitor &operator= (const EndpointMonitor &other) = delete;
		EndpointMonitor &operator= (EndpointMonitor &&other) = delete;

		static EndpointMonitor* instanceGet();

		void urlSet (const std::string &url);
		void enabled (bool enabled);

		void callCreated (int callIndex);
		void callDestroyed (int callIndex);
		void callStateChanged (const SstiStateChange &stateChange);
		void callStatistics (const CstiCallSP &call);
		void sipStackStatistics (const SstiRvSipStats &sipStackStats);
		void callStorageCalls (const std::list<CstiCallSP> &calls);
		void callAdded(const CstiCallSP &call);
		void callRemoved(const CstiCallSP &call);

		CstiSignal<> querySipStackInfoSignal;
		CstiSignal<> queryCallStorageStateSignal;

		void sipMessage (
			const std::string &message,
			const std::string &direction,
			const std::string &remoteAddress,
			int remotePort,
			const std::string &localAddress,
			int localPort,
			const std::string &transport,
			int64_t time);

	private:

		enum class PacketType
		{
			OPEN = 0,
			JOIN = 1,
			LEAVE = 2,
			JOIN_ACK = 3,
			JOIN_ERROR = 4,
			LEAVE_ACK = 5,
			LEAVE_ERROR = 6,
			EVENT = 7,
			PING = 8,
			PONG = 9
		};

		bool enabledGet () const;

		bool webSocketActive () const;

		void socketConnect ();

		void readPacket ();

		void eventSend (const std::string &eventName, const Poco::Dynamic::Var &data);

		void propertiesSend ();

		CstiSignalConnection::Collection m_signalConnections;

		bool m_enabled {false};

		vpe::EventTimer m_pingTimer;
		int m_pingInterval {30000};
		std::string m_connectionId;

		std::string m_uniqueID;

		std::string urlGet () const;
		
		std::string m_url;
		std::unique_ptr<vpe::WebSocket> m_webSocket;
		int m_socketFileDescriptor {-1};
	};
}
