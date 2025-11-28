/*!
 * \file ServiceOutageClient.cpp
 * \brief Client that checks for service outage messages
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 - 2018 Sorenson Communications, Inc. -- All rights reserved
 */


 //
 // Includes
 //
#include "ServiceOutageClient.h"
#include "ixml.h"
#include "cmPropertyNameDef.h"
#include "PropertyManager.h"
#include "stiRemoteLogEvent.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Format.h"
#include "stiTools.h"

using namespace Poco;
using namespace Poco::Net;

namespace vpe
{
	//
	// Constants
	//
	const std::string ServiceOutageClient::RootElementName = "Message";
	const std::string ServiceOutageClient::TextElementName = "MessageText";
	const std::string ServiceOutageClient::TypeElementName = "MessageType";
	const std::string ServiceOutageClient::MessageType_Error = "Error";
	const std::string ServiceOutageClient::MessageType_None = "None";


	/*!\brief Default Constructor
	*
	*  \return none.
	*/
	ServiceOutageClient::ServiceOutageClient () :
		CstiEventQueue("ServiceOutageClient"),
		m_timer(DefaultTimerInterval, this)
	{
	}


	/*!
	* \brief Shuts down the ServiceOutageClient and stops its HTTP request timer
	* \return void
	*/
	void ServiceOutageClient::shutdown ()
	{
		outageMessageReceived.DisconnectAll ();
		m_timer.stop ();
		StopEventLoop ();
	}


	/*!
	* \brief Starts the event loop and timer
	* \return void
	*/
	void ServiceOutageClient::initialize ()
	{
		StartEventLoop ();
		m_timer.start ();

		m_timerSignalConnection = m_timer.timeoutSignal.Connect ([this]()
		{
			checkForOutage ();
			m_timer.restart ();
		});
	}


	/*!
	* \brief Transistions to ServiceOutageClient thread and calls checkForOutage
	* \return void
	*/
	void ServiceOutageClient::forceCheckForOutage ()
	{
		PostEvent (std::bind (&ServiceOutageClient::checkForOutage, this));
	}
	

	/*!
	* \brief Performs an HTTP GET and then parses the message.  After that checks if the timer interval has changed.
	* \return void
	*/
	void ServiceOutageClient::checkForOutage ()
	{
		std::string errMessage;
		if (getOutageResponse(urlGet(), errMessage) != stiRESULT_SUCCESS)
		{
			auto message = std::make_shared<ServiceOutageMessage> ();
			outageMessageReceived.Emit (message);
			stiRemoteLogEventSend (&errMessage);
		}

		auto timerInterval = timerIntervalGet ();
		if (m_timer.timeoutGet () != timerInterval)
		{
			m_timer.timeoutSet (timerInterval);
		}
	}


	/*!
	* \brief Performs an HTTP/HTTPS GET and then parses the message. If the response indicates an HTTP 301 redirect,
	* it will resolve and follow the forwarding URL - up to 3 times.
	* \return success or failure
	*/
	stiHResult ServiceOutageClient::getOutageResponse (const std::string &url, std::string &errMessage, int attempts)
	{
		try {
			URI uri (url);
			auto session = clientSessionGet (uri);

			HTTPRequest request (HTTPRequest::HTTP_GET, uri.getPathAndQuery (), HTTPMessage::HTTP_1_1);
			session->sendRequest (request);

			HTTPResponse response;
			std::string responseBody (std::istreambuf_iterator<char> (session->receiveResponse (response)), {});

			auto status = response.getStatus ();
			if (status == HTTPResponse::HTTP_OK)
			{
				messageParse (responseBody);
				return stiRESULT_SUCCESS;
			}
			else if (status == HTTPResponse::HTTP_MOVED_PERMANENTLY && response.has("Location") && attempts > 0)
			{
				return getOutageResponse(response.get("Location"), errMessage, attempts - 1);
			}
			else
			{
				errMessage = format("EventType=ServiceOutageError ErrorType=RequestFailed StatusCode=%d URL=%s PublicIP=%s", 
					(int)status,
					urlGet (),
					m_publicIpAddress);
				return stiRESULT_ERROR;
			}
		}
		catch (Exception &ex)
		{
			errMessage = format("EventType=ServiceOutageError ErrorType=RequestFailed URL=%s PublicIP=%s", 
				urlGet (),
				m_publicIpAddress);
			return stiRESULT_ERROR;
		}
	}

	/*!
	* \brief Parses the received XML and calls back with the received message
	* \return success or failure
	*/
	stiHResult ServiceOutageClient::messageParse (std::string responseText)
	{
		Docptr document = nullptr;
		int rc = IXML_SUCCESS;
		stiHResult hResult = stiRESULT_SUCCESS;
		IXML_NodeList *nodeList = nullptr;
		IXML_Node *tempNode = nullptr;
		IXML_NodeList *childNodes = nullptr;
		auto message = std::make_shared<ServiceOutageMessage> ();
		int x = 0;

		rc = ixmlParseBufferEx (responseText.c_str (), &document);

		if (IXML_SUCCESS == rc)
		{
			nodeList = ixmlDocument_getElementsByTagName (document, (DOMString)RootElementName.c_str());
			stiTESTCOND (nodeList, stiRESULT_ERROR);

			// While there are still nodes...
			while ((tempNode = ixmlNodeList_item (nodeList, x++)))
			{
				// Process any child nodes
				childNodes = ixmlNode_getChildNodes (tempNode);
				if (childNodes != nullptr && childNodes->nodeItem)
				{
					int y = 0;
					IXML_Node * childNode = nullptr;

					// Loop through the children, and process them.
					while ((childNode = ixmlNodeList_item (childNodes, y++)))
					{
						auto childVal = ixmlNode_getFirstChild (childNode);
						if (childVal)
						{
							auto nodeValue = ixmlNode_getNodeValue (childVal);
							if (nodeValue)
							{
								// Process this node
								if (TextElementName == childNode->nodeName)
								{
									message->MessageText = nodeValue;
								}
								else if (TypeElementName == childNode->nodeName)
								{
									if (nodeValue == MessageType_Error)
									{
										message->MessageType = ServiceOutageMessageType::Error;
									}
									else if (nodeValue == MessageType_None)
									{
										message->MessageType = ServiceOutageMessageType::None;
									}
									else
									{
										stiRemoteLogEventSend ("EventType=ServiceOutageError ErrorType=UnexpectedMessageType MessageType=%s", nodeValue);
										message->MessageType = ServiceOutageMessageType::None;
									}
								}
								else
								{
									stiRemoteLogEventSend ("EventType=ServiceOutageError ErrorType=UnexpectedXmlNode NodeName=%s NodeValue=%s", childNode->nodeName, nodeValue);
								}
							}
							else
							{
								stiRemoteLogEventSend ("EventType=ServiceOutageError ErrorType=NullXmlValue NodeName=%s", childNode->nodeName);
							}
						}
						else
						{
							// MessageText is not required, but message type is
							if (TypeElementName == childNode->nodeName)
							{
								stiRemoteLogEventSend ("EventType=ServiceOutageError ErrorType=MissingTextNode NodeName=%s", childNode->nodeName);
							}
						}
					} // end while

					  // Free the child nodes
					ixmlNodeList_free (childNodes);
					childNodes = nullptr;

				}
				else
				{
					stiRemoteLogEventSend ("EventType=ServiceOutageError ErrorType=NoChildNodes");
				}
			} // end while
		}
		else
		{
			stiRemoteLogEventSend ("EventType=ServiceOutageError ErrorType=XmlParsingFailure");
		}

		if (enabledGet ())
		{
			outageMessageReceived.Emit (message);
		}
	STI_BAIL:

		if (childNodes)
		{
			// Free the child nodes
			ixmlNodeList_free (childNodes);
			childNodes = nullptr;
		}

		if (nodeList)
		{
			// Free the node list
			ixmlNodeList_free (nodeList);
			nodeList = nullptr;
		}

		if (document)
		{
			// Free the document.
			ixmlDocument_free (document);
			document = nullptr;
		}

		return hResult;
	}


	/*!
	* \brief Gets the URL for the outage service
	* \return void
	*/
	std::string ServiceOutageClient::urlGet ()
	{
		return m_url;
	}


	/*!
	* \brief Sets the URL for the outage service
	* \return void
	*/
	void ServiceOutageClient::urlSet (std::string url)
	{
		PostEvent ([this, url]()
		{
			m_url = url;
		});
	}


	std::unique_ptr<HTTPClientSession> ServiceOutageClient::clientSessionGet (const Poco::URI &uri)
	{
		if (uri.getScheme () == "https")
		{
			return sci::make_unique<HTTPSClientSession> (uri.getHost (), uri.getPort ());
		}

		return sci::make_unique<HTTPClientSession> (uri.getHost (), uri.getPort ());
	}


	/*!
	* \brief Gets the time interval in milliseconds
	* \return void
	*/
	int ServiceOutageClient::timerIntervalGet ()
	{
		return m_timerInterval;
	}


	/*!
	* \brief Sets the time interval in milliseconds
	* \return void
	*/
	void ServiceOutageClient::timerIntervalSet (int milliseconds)
	{
		PostEvent ([this, milliseconds]()
		{
			m_timerInterval = milliseconds;
		});
	}

	void ServiceOutageClient::publicIpAddressSet (std::string publicIpAddress)
	{
		PostEvent([this, publicIpAddress]()
		{
			m_publicIpAddress = publicIpAddress;
		});
	}


	void ServiceOutageClient::enabledSet (bool enable)
	{
		PostEvent ([this, enable]()
		{
			if (m_isEnabled != enable)
			{
				m_isEnabled = enable;
				if (m_isEnabled)
				{
					m_timer.start ();
				}
				else
				{
					m_timer.stop ();
					auto message = std::make_shared<ServiceOutageMessage> ();
					outageMessageReceived.Emit (message);
				}
			}
		});
	}


	bool ServiceOutageClient::enabledGet ()
	{
		return m_isEnabled;
	}
}
