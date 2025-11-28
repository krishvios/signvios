/*!
 * \file EndpointMonitor.cpp
 * \brief Client for the DHV web API
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
 */


//
// Includes
//
#include "EndpointMonitor.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/WebSocket.h"
#include "stiOS.h"
#include <iostream>
#include <vector>
#include "CstiCall.h"
#ifndef stiDISABLE_PROPERTY_MANAGER
#include "PropertyManager.h"
#endif
#include <sstream>
#include "stiTools.h"

using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;

std::string StateStringBuild (
	EsmiCallState eState, // The state to be converted to text
	uint32_t un32SubState);   // The substate to be converted to text

namespace vpe
{
	static std::unique_ptr<HTTPClientSession> clientSessionCreate (const Poco::URI &uri)
	{
		if (uri.getScheme () == "https")
		{
			return sci::make_unique<HTTPSClientSession> (uri.getHost (), uri.getPort ());
		}

		return sci::make_unique<HTTPClientSession> (uri.getHost (), uri.getPort ());
	}

	/*!\brief Default Constructor
	 * Intializes Poco::Net::initializeSSL and sets up the certificates we will need to move this when it is used with more certificates.
	 * Starts the event loop
	 * \return none
	 */
	EndpointMonitor::EndpointMonitor ()
	:
		CstiEventQueue("EndpointMonitor"),
		m_pingTimer (30000, this)
	{
		m_signalConnections.push_back (m_pingTimer.timeoutSignal.Connect (
			[this]
			{
				if (webSocketActive () && enabledGet ())
				{
					try
					{
					    Poco::DynamicStruct event;

					    event["t"] = static_cast<int>(PacketType::PING);

					    std::string s = event.toString ();

						m_webSocket->sendFrame(s.c_str (), s.size (), WebSocket::FRAME_TEXT);
					}
					catch (Exception &ex)
					{
						stiASSERTMSG (false, "Could not ping: %s", ex.displayText ().c_str ());

						socketConnect ();
					}

					m_pingTimer.restart ();
				}
			}));

		StartEventLoop ();
	}

	EndpointMonitor* EndpointMonitor::instanceGet()
	{
		// Meyers singleton: a local static variable (cleaned up automatically at exit)
		static EndpointMonitor localEndpointMonitor;

		return &localEndpointMonitor;
	}

	/*!\brief Default Destructor
	 * uninitializes SSL
	 * Stops the event loop
	 * \return none
	 */
	EndpointMonitor::~EndpointMonitor()
	{
		StopEventLoop ();
	}


	/*!\brief EndpointMonitor setter for URL
	 *
	 * \return void
	 */
	void EndpointMonitor::urlSet (const std::string &url)
	{
		PostEvent (
			[this, url] ()
			{
				m_url = url;

				if (enabledGet ())
				{
					socketConnect ();
				}
			});
	}

	void EndpointMonitor::enabled (bool enabled)
	{
		PostEvent (
			[this, enabled] ()
			{
				m_enabled = enabled;

				if (enabledGet ())
				{
					socketConnect ();
				}
				else if (!m_enabled && m_webSocket)
				{
					m_webSocket->shutdown ();
					
					if (m_socketFileDescriptor >= 0)
					{
						FileDescriptorDetach (m_socketFileDescriptor);
						m_socketFileDescriptor = -1;
					}
				}
			});
	}

	bool EndpointMonitor::enabledGet () const
	{
		return m_enabled && !urlGet ().empty ();
	}

	bool EndpointMonitor::webSocketActive () const
	{
		return m_webSocket && !m_webSocket->shutdownGet ();
	}

	/*!\brief EndpointMonitor private getter for URL
	 *
	 * \return URL value
	 */
	std::string EndpointMonitor::urlGet () const
	{
		return m_url;
	}

	void EndpointMonitor::socketConnect ()
	{
		PostEvent (
			[this] ()
			{
				if (enabledGet ())
				{
					try
					{
						stiOSGetUniqueID (&m_uniqueID);
						m_uniqueID.erase(std::remove(m_uniqueID.begin(), m_uniqueID.end(), ':'), m_uniqueID.end());

						URI uri (urlGet ());

						auto session = clientSessionCreate (uri);

						HTTPRequest request (HTTPRequest::HTTP_GET, uri.getPathAndQuery (), HTTPMessage::HTTP_1_1);
						HTTPResponse response;

						m_webSocket = sci::make_unique<WebSocket>(*session, request, response);
						m_webSocket->setBlocking(false);

						if (m_socketFileDescriptor >= 0)
						{
							FileDescriptorDetach (m_socketFileDescriptor);
							m_socketFileDescriptor = -1;
						}

						m_socketFileDescriptor = m_webSocket->sockfd();

						FileDescriptorAttach (m_socketFileDescriptor,
							[this]
							{
								readPacket ();
							});

						Poco::DynamicStruct event;

						event["t"] = static_cast<int>(PacketType::JOIN);
						event["d"] = Poco::DynamicStruct();
						event["d"]["topic"] = "endpoint:" + m_uniqueID;

						std::string subscribe = event.toString ();

						m_webSocket->sendFrame(subscribe.c_str (), subscribe.size (), WebSocket::FRAME_TEXT);

					}
					catch (Exception &ex)
					{
						stiASSERTMSG (false, "Failed to connect to %s due to exception: %s", urlGet ().c_str (), ex.displayText ().c_str ());
					}
				}
			});
	}

	void EndpointMonitor::readPacket ()
	{
		// Read data from web socket
		std::vector<char> buffer(256);
		int flags = 0;
		int length = 0;

		try
		{
			length = m_webSocket->receiveFrame(buffer.data (), buffer.size (), flags);
		}
		catch (Exception &ex)
		{
			stiASSERTMSG (false, "Could not read packet: %s", ex.displayText ().c_str ());
		}

		if (length > 0)
		{
			buffer.resize(length);
			std::string s(buffer.begin(), buffer.end());

			try
			{
				Poco::JSON::Parser parser;
				auto result = parser.parse(s);

				Object::Ptr object = result.extract<Object::Ptr>();

				Poco::DynamicStruct request = *object;

				stiDEBUG_TOOL(g_endpointMonitorDebug,
						vpe::trace ("response: ", request.toString (), "\n");
				);

				auto packetType = static_cast<PacketType>(static_cast<int>(request["t"]));

				switch (packetType)
				{
					case PacketType::OPEN:

						m_pingInterval = request["d"]["clientInterval"];
						m_connectionId = request["d"]["connId"].toString ();

						m_pingTimer.timeoutSet(m_pingInterval);
						m_pingTimer.restart ();

						break;

					case PacketType::JOIN:
					case PacketType::LEAVE:
					case PacketType::LEAVE_ACK:
					case PacketType::LEAVE_ERROR:

						break;

					case PacketType::JOIN_ACK:

						stiDEBUG_TOOL(g_endpointMonitorDebug,
								vpe::trace ("Received join ack over websocket\n");
						);

						break;

					case PacketType::JOIN_ERROR:

						stiDEBUG_TOOL(g_endpointMonitorDebug,
								vpe::trace ("Received join error over websocket\n");
						);

						break;

					case PacketType::EVENT:
					{
						stiDEBUG_TOOL(g_endpointMonitorDebug,
								vpe::trace ("Received event over websocket\n");
						);

						auto event = request["d"]["event"].toString ();

						if (event == "getProperties")
						{
							propertiesSend ();
						}
						else if (event == "getSipStack")
						{
							querySipStackInfoSignal.Emit ();
						}
						else if (event == "getCallStorage")
						{
							queryCallStorageStateSignal.Emit ();
						}

						break;
					}

					case PacketType::PING:

						stiDEBUG_TOOL(g_endpointMonitorDebug,
								vpe::trace ("Received ping over websocket\n");
						);

						break;

					case PacketType::PONG:

						stiDEBUG_TOOL(g_endpointMonitorDebug,
								vpe::trace ("Received pong over websocket\n");
						);

						break;
				}
			}
			catch (Exception &ex)
			{
				stiASSERTMSG (false, "Could not parse JSON: %s", ex.displayText ().c_str ());
			}
		}
	}


	void EndpointMonitor::eventSend (const std::string &eventName, const Poco::Dynamic::Var &data)
	{
		if (enabledGet () && webSocketActive ())
		{
			try
			{
				Poco::DynamicStruct event;

				event["t"] = static_cast<int>(PacketType::EVENT);
				event["d"] = Poco::DynamicStruct();
				event["d"]["topic"] = "endpoint:" + m_uniqueID;
				event["d"]["event"] = eventName;
				event["d"]["data"] = data;

				auto eventString = event.toString ();

				stiDEBUG_TOOL(g_endpointMonitorDebug,
						vpe::trace ("Sending event: ", eventString, "\n");
				);

				m_webSocket->sendFrame(eventString.c_str (), eventString.size (), WebSocket::FRAME_TEXT);
			}
			catch (Exception &ex)
			{
				stiASSERTMSG (false, "Could not send event: %s", ex.displayText ().c_str ());

				socketConnect ();
			}
		}
	}

	void EndpointMonitor::callStorageCalls (const std::list<CstiCallSP> &calls)
	{
		PostEvent (
			[this, calls] ()
			{
				if (enabledGet ())
				{
					try
					{
						Poco::Dynamic::Array callArr;
						for (auto &call: calls)
						{
							Poco::DynamicStruct c;

							c["callIndex"] = call->CallIndexGet ();

							std::string callID;
							call->CallIdGet(&callID);
							c["callID"] = callID;

							c["state"] = StateStringBuild (call->StateGet (), call->SubstateGet ());

							callArr.push_back(c);
						}

						Poco::DynamicStruct event;

						event["calls"] = callArr;

						eventSend("callStorage", event);
					}
					catch (Exception &ex)
					{
						stiASSERTMSG (false, "Could not send event: %s", ex.displayText ().c_str ());

						socketConnect ();
					}
				}
			});
	}

	void EndpointMonitor::callAdded (const CstiCallSP &call)
	{
		PostEvent (
			[this, call] ()
			{
				if (enabledGet ())
				{
					Poco::DynamicStruct event;

					event["callIndex"] = call->CallIndexGet ();

					eventSend("callAdded", event);
				}
			});
	}

	void EndpointMonitor::callRemoved (const CstiCallSP &call)
	{
		PostEvent (
			[this, call] ()
			{
				if (enabledGet ())
				{
					Poco::DynamicStruct event;

					event["callIndex"] = call->CallIndexGet ();

					eventSend("callRemoved", event);
				}
			});
	}

	void EndpointMonitor::callCreated (int callIndex)
	{
		PostEvent (
			[this, callIndex] ()
			{
				if (enabledGet ())
				{
					Poco::DynamicStruct event;

					event["callIndex"] = callIndex;

					eventSend("callCreated", event);
				}
			});
	}


	void EndpointMonitor::callDestroyed (int callIndex)
	{
		PostEvent (
			[this, callIndex] ()
			{
				if (enabledGet ())
				{
					Poco::DynamicStruct event;

					event["callIndex"] = callIndex;

					eventSend("callDestroyed", event);
				}
			});
	}

	void EndpointMonitor::callStateChanged (const SstiStateChange &stateChange)
	{
		PostEvent (
			[this, stateChange] ()
			{
				if (enabledGet ())
				{
					Poco::DynamicStruct event;

					std::string callID;
					stateChange.call->CallIdGet(&callID);

					event["callIndex"] = stateChange.call->CallIndexGet ();
					event["callID"] = callID;
					event["state"] = StateStringBuild (stateChange.eNewState, stateChange.un32NewSubStates);

					eventSend("callStateChange", event);
				}
			});
	}

	void EndpointMonitor::callStatistics (const CstiCallSP &call)
	{
		PostEvent (
			[this, call]
			{
				if (enabledGet ())
				{
					SstiCallStatistics stats;
					call->StatisticsGet (&stats);

					std::string callID;
					call->CallIdGet (&callID);

					Poco::DynamicStruct event;

					event["callIndex"] = call->CallIndexGet ();
					event["callID"] = callID;

					auto playbackStats = Poco::DynamicStruct();
					playbackStats["videoDataRate"] = stats.Playback.nActualVideoDataRate;
					playbackStats["maxVideoDataRate"] = stats.Playback.nTargetVideoDataRate;
					playbackStats["audioDataRate"] = stats.Playback.nActualAudioDataRate;
					playbackStats["textDataRate"] = stats.Playback.nActualTextDataRate;
					playbackStats["videoHeight"] = stats.Playback.nVideoHeight;
					playbackStats["frameRate"] = stats.Playback.fActualFrameRate;
					playbackStats["keyframeReqs"] = stats.Playback.keyframeRequests;
					playbackStats["keyframes"] = stats.Playback.keyframes;
					playbackStats["packetsReceived"] = stats.Playback.packetsReceived;
					playbackStats["packetsLost"] = stats.Playback.packetsLost;
					playbackStats["actualPacketsLost"] = stats.Playback.actualPacketsLost;
					playbackStats["nackRequests"] = stats.Playback.nackRequestsSent;
					playbackStats["rtxPacketsReceived"] = stats.Playback.rtxPacketsReceived;
					event["playback"] = playbackStats;

					auto recordStats = Poco::DynamicStruct();
					recordStats["videoDataRate"] = stats.Record.nActualVideoDataRate;
					recordStats["maxVideoDataRate"] = stats.un32MaxSendRate / 1000;
					recordStats["audioDataRate"] = stats.Record.nActualAudioDataRate;
					recordStats["textDataRate"] = stats.Record.nActualTextDataRate;
					recordStats["videoHeight"] = stats.Record.nVideoHeight;
					recordStats["frameRate"] = stats.Record.fActualFrameRate;
					recordStats["keyframeReqs"] = stats.Record.keyframeRequests;
					recordStats["keyframes"] = stats.Record.keyframes;
					recordStats["nackRequests"] = stats.Record.nackRequestsReceived;
					recordStats["rtxPacketsSent"] = stats.Record.rtxPacketsSent;
					event["record"] = recordStats;

					eventSend("mediaStats", event);
				}
			});
	}

	void EndpointMonitor::sipStackStatistics (const SstiRvSipStats &sipStackStats)
	{
		PostEvent (
			[this, sipStackStats]
			{
				if (enabledGet ())
				{
					eventSend("sipStackStats", static_cast<Poco::JSON::Object>(sipStackStats));
				}
			});
	}

	void EndpointMonitor::sipMessage (
		const std::string &message,
		const std::string &direction,
		const std::string &remoteAddress,
		int remotePort,
		const std::string &localAddress,
		int localPort,
		const std::string &transport,
		int64_t time)
	{
		PostEvent (
			[this, message, direction, remoteAddress, remotePort, localAddress, localPort, transport, time]
			{
				if (enabledGet ())
				{
					Poco::DynamicStruct event;

					event["message"] = message;
					event["direction"] = direction;
					event["remoteAddr"] = remoteAddress;
					event["remotePort"] = remotePort;
					event["localAddr"] = localAddress;
					event["localPort"] = localPort;
					event["transport"] = transport;
					event["time"] = time;

					eventSend ("sipMessage", event);
				}
			});
	}

	void EndpointMonitor::propertiesSend ()
	{
		if (enabledGet ())
		{
#ifndef stiDISABLE_PROPERTY_MANAGER
			Poco::DynamicStruct event;

			WillowPM::PropertyManager::getInstance()->each(
				[this, &event](const std::string &name, const std::string &value)
				{
					if (name.find ("Password") == std::string::npos &&
						name.find ("ShareTextSavedStr") == std::string::npos &&
						name != "GreetingText" &&
						name != "LDAPLoginName" &&
						name != "DhvApiKey")
					{
						event[name] = value;
					}
				});
			
			eventSend("properties", event);
#endif
		}
	}
}
