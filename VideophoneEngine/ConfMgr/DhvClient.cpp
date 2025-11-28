/*!
 * \file DhvClient.cpp
 * \brief Client for the DHV web API
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 - 2019 Sorenson Communications, Inc. -- All rights reserved
 */


//
// Includes
//
#include "DhvClient.h"
#include "CstiCall.h"
#include "CstiSipCall.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/SSLManager.h"
#include "stiOS.h"
#include "stiSSLTools.h"
#include "stiTools.h"

using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;

namespace vpe
{
	/*!\brief Default Constructor
	 * Intializes Poco::Net::initializeSSL and sets up the certificates we will need to move this when it is used with more certificates.
	 * Starts the event loop
	 * \return none
	 */
	DhvClient::DhvClient () :
	CstiEventQueue("DhvClient")
	{
		StartEventLoop ();
	}

	/*!\brief Default Destructor
	 * uninitializes SSL
	 * Stops the event loop
	 * \return none
	 */
	DhvClient::~DhvClient()
	{
		StopEventLoop ();
		Poco::Net::uninitializeSSL ();
	}


	/*!\brief DhvClient setter for URL
	 *
	 * \return void
	 */
	void DhvClient::urlSet (const std::string &url)
	{
		PostEvent ([this, url] ()
		{
			m_url = url;
		});
	}


	/*!\brief DhvClient private getter for URL
	 *
	 * \return URL value
	 */
	std::string DhvClient::urlGet ()
	{
		return m_url;
	}


	/*!\brief DhvClient setter for API key
	 *
	 * \return void
	 */
	void DhvClient::apiKeySet (const std::string &key)
	{
		PostEvent ([this, key] ()
		{
			m_apiKey = key;
		});
	}


	/*!\brief DHVClient private getter for API key
	 *
	 * \return API key value
	 */
	std::string DhvClient::apiKeyGet ()
	{
		return m_apiKey;
	}

	std::unique_ptr<HTTPClientSession> DhvClient::clientSessionGet (const Poco::URI &uri)
	{
		if (uri.getScheme () == "https")
		{
			return sci::make_unique<HTTPSClientSession> (uri.getHost (), uri.getPort ());
		}

		return sci::make_unique<HTTPClientSession> (uri.getHost (), uri.getPort ());
	}


	/*!\brief DHVClient post event for Deaf-Hearing-Video capability check
	 *
	 * \return void
	 */
	void DhvClient::dhvCapableGet (CstiCallSP call, const std::string &phoneNumber)
	{
		PostEvent ([this, call, phoneNumber] ()
		{
			auto sipCall = std::static_pointer_cast<CstiSipCall> (call->ProtocolCallGet ());
			try
			{
				constexpr auto QUERY_PHONENUMBER = "phoneNumber";
				
				URI uri (urlGet () + CAPABLE_REQUEST);
				uri.addQueryParameter (QUERY_PHONENUMBER, phoneNumber);
				
				auto session = clientSessionGet (uri);

				HTTPRequest request (HTTPRequest::HTTP_HEAD, uri.getPathAndQuery (), HTTPMessage::HTTP_1_1);
				request.set (https::AWS_APIKEY_HEADER, apiKeyGet ());
			
				HTTPResponse response;
				session->sendRequest (request);
				std::string responseBody (std::istreambuf_iterator<char> (session->receiveResponse (response)), {});
				
				auto status = response.getStatus ();
				switch (status)
				{
					case HTTPResponse::HTTP_OK:
						sipCall->sendDhviCapableMsg (true);						
						if (!sipCall->isDhvCall ()) // Don't tell the UI it can create a DHV call if it is in one
						{
							call->dhviStateSet (IstiCall::DhviState::Capable);
						}
						break;
					case HTTPResponse::HTTP_NO_CONTENT:
						sipCall->sendDhviCapableMsg (false);
						call->dhviStateSet (IstiCall::DhviState::NotAvailable);
						break;
					default:
						sipCall->sendDhviCapableMsg (false);
						call->dhviStateSet (IstiCall::DhviState::NotAvailable);
						stiASSERTMSG (false, "DHV Status Code %d from %s responded with %s", status, urlGet ().c_str (), responseBody.c_str ());
						break;
				}
			}
			catch (Exception &ex)
			{
				stiASSERTMSG (false, "Failed to connect to %s due to exception: %s", urlGet ().c_str (), ex.displayText ().c_str ());
			}
		});
	}


/*!\brief DHVClient post event for Deaf-Hearing-Video start call
 *
 * \return void
 */
void DhvClient::dhvStartCall (
	CstiCallSP call,
	const std::string &toPhoneNumber,
	const std::string &fromPhoneNumber,
	const std::string &displayName,
	const std::string &dhvUri)
{
	PostEvent ([this, toPhoneNumber, fromPhoneNumber, displayName, dhvUri, call] ()
	{
		auto sipCall = std::static_pointer_cast<CstiSipCall> (call->ProtocolCallGet ());
		try
		{
			constexpr auto TO_PHONENUMBER_PARAM = "toPhoneNumber";
			constexpr auto FROM_PHONENUMBER_PARAM = "fromPhoneNumber";
			constexpr auto DISPLAY_NAME_PARAM = "displayName";
			constexpr auto ROOM_URI_PARAM = "roomURI";
			
			URI uri (urlGet () + INVITE_REQUEST);

			auto session = clientSessionGet (uri);

			HTTPRequest request (HTTPRequest::HTTP_POST, uri.getPathAndQuery (), HTTPMessage::HTTP_1_1);
			request.set (https::AWS_APIKEY_HEADER, apiKeyGet ());
			request.setContentType(https::JSON_CONTENT_TYPE);
			
			// start making the poco json object here
			Poco::JSON::Object jsonObject;
			std::stringstream jsonStream;
			jsonObject.set (TO_PHONENUMBER_PARAM, toPhoneNumber);
			jsonObject.set (FROM_PHONENUMBER_PARAM, fromPhoneNumber);
			jsonObject.set (DISPLAY_NAME_PARAM, displayName);
			jsonObject.set (ROOM_URI_PARAM, dhvUri);
			
			
			jsonObject.stringify (jsonStream);
			request.setContentLength (jsonStream.str ().size ());
		
			HTTPResponse response;
			std::ostream& requestStream = session->sendRequest (request);
			jsonObject.stringify(requestStream);
			
			std::string responseBody (std::istreambuf_iterator<char> (session->receiveResponse (response)), {});
			
			if (response.getStatus () != HTTPResponse::HTTP_OK)
			{
				auto reason =  HTTPResponse::getReasonForStatus (response.getStatus ());
				stiASSERTMSG (false, "dhvStartCall failed Reason=%s with message=%s", reason.c_str (), responseBody.c_str());
				call->dhviMcuDisconnect ();
				call->dhviStateSet (IstiCall::DhviState::Failed);
			}
		}
		catch (Exception &ex)
		{
			stiASSERTMSG (false, "Failed to connect to %s due to exception: %s", urlGet ().c_str (), ex.displayText ().c_str ());
		}
	});
}
}
