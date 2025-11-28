#include "./Message.h"
#include "CstiCall.h"
#include "CstiSipCall.h"
#include "CstiSipCallLeg.h"
#include "stiSipTools.h"
#include "RvSipOtherHeader.h"
#include "RvSipRegClient.h"
#include "RvSipAllowHeader.h"

namespace vpe
{
namespace sip
{
	Message::Message(RvSipMsgHandle hMessage)
	:
		m_message(hMessage)
	{
	}

	Message::Message(CstiSipCallLegSP callLeg, bool outbound)
	:
		m_callLeg(callLeg)
	{
		if (outbound)
		{
			RvSipCallLegGetOutboundMsg (callLeg->m_hCallLeg, &m_message);
		}
		else
		{
			RvSipCallLegGetReceivedMsg (callLeg->m_hCallLeg, &m_message);
		}
	}

	Message::Message(CstiSipCallLegSP callLeg, RvSipMsgHandle hMessage)
	:
		m_callLeg(callLeg),
		m_message(hMessage)
	{
	}

	Message::Message(RvSipTranscHandle hTransaction, CstiSipCallLegSP callLeg, bool outbound)
	:
		m_callLeg(callLeg)
	{
		if (outbound)
		{
			RvSipTransactionGetOutboundMsg (hTransaction, &m_message);
		}
		else
		{
			RvSipTransactionGetReceivedMsg(hTransaction, &m_message);
		}
	}

	Message::Message(RvSipRegClientHandle hRegClient, bool outbound)
	{
		if (outbound)
		{
			RvSipRegClientGetOutboundMsg (hRegClient, &m_message);
		}
		else
		{
			RvSipRegClientGetReceivedMsg (hRegClient, &m_message);
		}
	}

	void Message::clear ()
	{
		m_callLeg = nullptr;
		m_message = nullptr;
	}

	stiHResult Message::bodySet (
		const std::string &contentType,
		const std::string &body)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvStatus rvStatus = RV_OK;

		rvStatus = RvSipMsgSetContentTypeHeader (m_message, const_cast<RvChar *>(contentType.c_str()));
		stiTESTRVSTATUS ();

		rvStatus = RvSipMsgSetBody (m_message, const_cast<RvChar*>(body.c_str()));
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}

	stiHResult Message::bodySet (
		const CstiSdp &Sdp)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvStatus rvStatus = RV_OK;
		std::string sdpText;

		hResult = Sdp.encodeToString (sdpText);
		stiTESTRESULT();

		rvStatus = RvSipMsgSetContentTypeHeader (m_message, const_cast<RvChar *>(SDP_MIME));
		stiTESTRVSTATUS ();

		rvStatus = RvSipMsgSetBody (m_message, const_cast<RvChar *>(sdpText.c_str ()));
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}

	stiHResult Message::bodyClear ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvStatus rvStatus = RV_OK;

		rvStatus = RvSipMsgSetBody (m_message, nullptr);
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}

	stiHResult Message::headerAdd(
		const std::string &headerName,
		const std::string &headerValue)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvStatus rvStatus = RV_OK;
		RvSipOtherHeaderHandle hOtherHeader;

		rvStatus = RvSipOtherHeaderConstructInMsg (m_message, RV_FALSE, &hOtherHeader);
		stiTESTRVSTATUS ();

		rvStatus = RvSipOtherHeaderSetName (hOtherHeader, const_cast<RvChar *>(headerName.c_str()));
		stiTESTRVSTATUS ();

		if (!headerValue.empty())
		{
			rvStatus = RvSipOtherHeaderSetValue (hOtherHeader, const_cast<RvChar *>(headerValue.c_str()));
			stiTESTRVSTATUS ();
		}

	STI_BAIL:

		return hResult;
	}

	stiHResult Message::inviteRequirementsAdd()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		auto sipCall = m_callLeg->m_sipCallWeak.lock ();
		stiTESTCOND(sipCall != nullptr, stiRESULT_ERROR);

		hResult = sipCall->SipManagerGet ()->InviteRequirementsAdd (
				m_message,
				sipCall->IsBridgedCall(),
				sipCall->CstiCallGet ()->geolocationRequestedGet ());
		stiTESTRESULT ();

		additionalHeadersAdd();

	STI_BAIL:

		return hResult;
	}

	stiHResult Message::geolocationAdd()
	{
		stiHResult hResult {stiRESULT_SUCCESS};

		auto sipCall = m_callLeg->m_sipCallWeak.lock ();
		stiTESTCOND(sipCall != nullptr, stiRESULT_ERROR);

		hResult = sipCall->SipManagerGet ()->geolocationAdd(m_message);
		stiTESTRESULT();

	STI_BAIL:

		return hResult;
	}

	/*!\brief Add any additional headers in call object to the outgoing message.
	 * 
	 */
	stiHResult Message::additionalHeadersAdd()
	{
		stiHResult hResult{ stiRESULT_SUCCESS };

		auto sipCall = m_callLeg->m_sipCallWeak.lock();
		stiTESTCONDMSG(sipCall != nullptr, stiRESULT_ERROR, "Message::additionalHeadersAdd failed to obtain sipCall");

		for (auto& header : sipCall->CstiCallGet()->additionalHeadersGet())
		{
			hResult = headerAdd(header.name, header.body);
			stiTESTRESULT();
		}

	STI_BAIL:

		return hResult;
	}


	/**
	 * Enables VRCL to set the AlwaysAllow header to this call
	 */
	stiHResult Message::allowIncomingCallHeaderAdd()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		RvStatus rvStatus = RV_OK;
		RvSipOtherHeaderHandle hOtherHeader = nullptr;

		rvStatus = RvSipOtherHeaderConstructInMsg (m_message, RV_FALSE, &hOtherHeader);
		stiTESTRVSTATUS ();

		rvStatus = RvSipOtherHeaderSetName (hOtherHeader, (RvChar*)g_szSVRSAllowIncomingCallPrivateHeader);
		rvStatus = RvSipOtherHeaderSetValue (hOtherHeader, (RvChar*)g_szSVRSAllowIncomingCallPrivateHeader);
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}

	/**
	 * \brief Adds a private header to allow for SIP call to be converted to H323
	 *
	 * \param hOutboundMsg handle of the message object
	 */
	stiHResult Message::convertSipToH323HeaderAdd ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		RvStatus rvStatus = RV_OK;
		RvSipOtherHeaderHandle hOtherHeader = nullptr;

		rvStatus = RvSipOtherHeaderConstructInMsg (m_message, RV_FALSE, &hOtherHeader);
		stiTESTRVSTATUS ();

		rvStatus = RvSipOtherHeaderSetName (hOtherHeader, (RvChar*)g_szSVRSConvertSIPToH323PrivateHeader);
		rvStatus = RvSipOtherHeaderSetValue (hOtherHeader, (RvChar*)g_szSVRSConvertSIPToH323PrivateHeader);
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}

	stiHResult Message::sinfoHeaderAdd(const std::string &systemInfo)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		RvStatus rvStatus = RV_OK;
		RvSipOtherHeaderHandle hHeader = nullptr;

		rvStatus = RvSipOtherHeaderConstructInMsg (m_message, RV_FALSE, &hHeader);
		stiTESTRVSTATUS ();

		rvStatus = RvSipOtherHeaderSetName (hHeader, (RvChar*)g_szSVRSSInfoPrivateHeader);
		stiTESTRVSTATUS ();

		if (m_callLeg->m_nRemoteSIPVersion >= 3)
		{
			std::string EncryptedSInfo;

			EncryptString (systemInfo, &EncryptedSInfo);

			rvStatus = RvSipOtherHeaderSetValue (hHeader, (RvChar*)EncryptedSInfo.c_str ());
			stiTESTRVSTATUS ();
		}
		else
		{
			rvStatus = RvSipOtherHeaderSetValue (hHeader, (RvChar*)systemInfo.c_str ());
			stiTESTRVSTATUS ();
		}

	STI_BAIL:

		return hResult;
	}

	template<typename T>
	std::string Message::rvMessageStringGet(T handle, messageStringFunc<T> func) const
	{
		RvUint length {50};
		std::vector<RvChar> buffer;

		buffer.resize(length);
		auto rvStatus = func (handle, buffer.data(), buffer.size(), &length);

		if (RV_ERROR_INSUFFICIENT_BUFFER == rvStatus)
		{
			buffer.resize(length);
			rvStatus = func (handle, buffer.data(), buffer.size(), &length);
		}

		if (rvStatus == RV_OK)
		{
			std::string contentType{buffer.begin(), std::find(buffer.begin(), buffer.end(), '\0')};

			return contentType;
		}

		return {};
	}

	template<typename T>
	Message::Method Message::translateMethodType(
		RvSipMethodType rvMethodType,
		T handle,
		messageStringFunc<T> func) const
	{
		switch (rvMethodType)
		{
			case RVSIP_METHOD_UNDEFINED:
				return Method::Undefined;
			case RVSIP_METHOD_INVITE:
				return Method::Invite;
			case RVSIP_METHOD_ACK:
				return Method::Ack;
			case RVSIP_METHOD_BYE:
				return Method::Bye;
			case RVSIP_METHOD_REGISTER:
				return Method::Register;
			case RVSIP_METHOD_REFER:
				return Method::Refer;
			case RVSIP_METHOD_NOTIFY:
				return Method::Notify;
			case RVSIP_METHOD_PRACK:
				return Method::Prack;
			case RVSIP_METHOD_CANCEL:
				return Method::Cancel;
			case RVSIP_METHOD_SUBSCRIBE:
				return Method::Subscribe;
			case RVSIP_METHOD_OTHER:
			{
				auto method = rvMessageStringGet(handle, func);

				if (method == "MESSAGE")
				{
					return Method::Message;
				}

				if (method == "UPDATE")
				{
					return Method::Update;
				}

				break;
			}
		}

		return Method::Undefined;
	}

	Message::Method Message::methodGet () const
	{
		auto rvMethodType = RvSipMsgGetRequestMethod(m_message);

		return translateMethodType(rvMethodType, m_message, RvSipMsgGetStrRequestMethod);
	}

	int Message::statusCodeGet () const
	{
		return RvSipMsgGetStatusCode (m_message);
	}

	/*! \brief get the transport from the via header from the incoming message
	 */
	EstiTransport Message::viaTransportGet () const
	{
		EstiTransport transport = estiTransportUnknown;

		// NOTE: this duplicates the code getting the message handle,
		// but it's to provide a nice api to the call leg

		// Get the Via at the top of the list
		auto  viaHeader = viaHeaderGet (m_message);

		switch (viaHeader.transport)
		{
			case RVSIP_TRANSPORT_TCP:
				transport = estiTransportTCP;
				break;
			case RVSIP_TRANSPORT_TLS:
				transport = estiTransportTLS;
				break;
			case RVSIP_TRANSPORT_UDP:
				transport = estiTransportUDP;
				break;
			case RVSIP_TRANSPORT_WEBSOCKET:
			case RVSIP_TRANSPORT_WSS:
			case RVSIP_TRANSPORT_UNDEFINED:
			case RVSIP_TRANSPORT_SCTP:
			case RVSIP_TRANSPORT_OTHER:
				stiASSERTMSG(false, "Unable to determine transport (%d) from header", viaHeader.transport);
				break;
		}

		return transport;
	}


	RvSipPartyHeaderHandle Message::toHeaderGet()
	{
		return RvSipMsgGetToHeader (m_message);
	}

	void Message::forEachHeaderType (
		RvSipHeaderType headerType,
		std::function<void(void *, RvSipHeaderListElemHandle)> callback) const
	{
		RvSipHeaderListElemHandle hListElem {nullptr};

		auto hHeader = RvSipMsgGetHeaderByType (
			m_message, headerType, RVSIP_FIRST_HEADER, &hListElem);

		while (hHeader)
		{
			callback(hHeader, hListElem);

			hHeader = RvSipMsgGetHeaderByType (
				m_message, headerType, RVSIP_NEXT_HEADER, &hListElem);
		}
	}

	std::vector<ContactHeader> Message::contactHeadersGet() const
	{
		std::vector<ContactHeader> contacts;

		forEachHeaderType(
			RVSIP_HEADERTYPE_CONTACT,
			[&contacts](void *header, RvSipHeaderListElemHandle hListElem)
			{
				contacts.emplace_back(static_cast<RvSipContactHeaderHandle>(header));
			}
		);

		return contacts;
	}

	void Message::contactHeadersDelete ()
	{
		forEachHeaderType(
			RVSIP_HEADERTYPE_CONTACT,
			[this](void * header, RvSipHeaderListElemHandle hListElem)
			{
				if (RV_OK != RvSipMsgRemoveHeaderAt (m_message, hListElem))
				{
					stiASSERTMSG (estiFALSE, "Unable to delete contact header.");
				}
			}
		);
	}

	std::string Message::contentTypeHeaderGet() const
	{
		return rvMessageStringGet(m_message, RvSipMsgGetContentTypeHeader);
	}

	#define MAX_UA_NAME 50 * 2
	std::tuple<std::string, std::string, int, bool> Message::userAgentGet () const
	{
		stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
		std::string remoteProductName;
		std::string remoteProductVersion;
		int remoteSIPVersion {-1};
		bool userAgentHeader = false;

		// We will always update the the remote product name to resolve issues with transfers using Bridge bug #25307
		if (m_message != nullptr)
		{
			RvSipHeaderListElemHandle hListIterator = nullptr;
			RvSipOtherHeaderHandle hUaHeader = RvSipMsgGetHeaderByName (
				m_message, (RvChar*)"User-Agent", RVSIP_FIRST_HEADER, &hListIterator);

			if (nullptr != hUaHeader)
			{
				RvChar szUaName[MAX_UA_NAME];
				int nLength = sizeof (szUaName);
				userAgentHeader = true;
				if (RV_OK == RvSipOtherHeaderGetValue (hUaHeader, szUaName, nLength, (RvUint*)&nLength))
				{
					if (0 == strncmp (szUaName, g_szSVRS_DEVICE, sizeof (g_szSVRS_DEVICE) - 1))
					{
						// Find where SEPARATOR occurs
						int nPos;
						for (nPos = 0; szUaName[nPos]; ++nPos)
						{
							if (0 == strncmp (&szUaName[nPos], SIP_VERSION_SEPARATOR, sizeof (SIP_VERSION_SEPARATOR) - 1))
							{
								break;
							}
						}

						if (!szUaName[nPos])
						{
							// No version
							remoteProductName = szUaName;
						}
						else
						{
							int i = 0;

							// Has version
							szUaName[nPos] = '\0';
							char *pszVersion = &szUaName[nPos + sizeof (SIP_VERSION_SEPARATOR) - 1];
							for (; *pszVersion == ' '; ++pszVersion) // Strip leading whitespace
							{
							}

							//
							// Find start of whitespace and terminate version string
							//
							for (i = 0; pszVersion[i]; ++i) // Strip trailing whitespace and/or parentheses
							{
								if (pszVersion[i] == ' ')
								{
									pszVersion[i] = '\0';
									++i;
									break;
								}
							}

							//
							// Now check to see if there is an "SSV" (Sorenson SIP Version) version string
							//
							char *pszSIPVersion = &pszVersion[i];

							//
							// Trim off leading whitespace
							//
							for (; *pszSIPVersion == ' '; ++pszSIPVersion) // Strip leading whitespace
							{
							}

							if (*pszSIPVersion)
							{

								char *pSeparator = strchr (pszSIPVersion, '/');

								if (pSeparator)
								{
									//
									// Separator found.  Replace with a NULL terminator
									//
									*pSeparator = '\0';

									if (0 == strcmp (pszSIPVersion, SORENSON_SIP_VERSION_STRING))
									{
										pszSIPVersion = pSeparator + 1;

										//
										// Trim off leading whitespace
										//
										for (; *pszSIPVersion == ' '; ++pszSIPVersion) // Strip leading whitespace
										{
										}

										//
										// Find start of whitespace and terminate version string
										//
										for (int i = 0; pszSIPVersion[i]; ++i) // Strip trailing whitespace and/or parentheses
										{
											if (pszSIPVersion[i] == ' ')
											{
												pszSIPVersion[i] = '\0';
												++i;
												break;
											}
										}

										remoteSIPVersion = atoi (pszSIPVersion);
									}
								}
							}

							remoteProductName = szUaName;
							remoteProductVersion = pszVersion;
						}
					}
					else
					{
						// Non-Sorenson phone
						remoteProductName = szUaName;
					}
				}
			}
		}

		return std::tuple<std::string, std::string, int, bool>{remoteProductName, remoteProductVersion, remoteSIPVersion, userAgentHeader};
	} // end CstiSipCallLeg::RemoteUANameSet
	#undef MAX_UA_NAME


	std::tuple<std::string, bool> Message::headerValueGet(
		const std::string &headerName) const
	{
		stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
		std::string headerValue;
		bool present {false};
		RvSipHeaderListElemHandle listIterator = nullptr;
		RvSipOtherHeaderHandle headerHandle = nullptr;

		stiTESTCOND (!headerName.empty(), stiRESULT_ERROR);

		//Check to see if the header exist
		headerHandle = RvSipMsgGetHeaderByName(
			m_message, const_cast<RvChar*>(headerName.c_str()), RVSIP_FIRST_HEADER, &listIterator);

		if (headerHandle)
		{
			//
			// Header available in message.
			// Retrieve the header and return it
			//
			present = true;

			RvUint length = RvSipOtherHeaderGetStringLength(headerHandle, RVSIP_OTHER_HEADER_VALUE);

			std::string sipHeader; //local variable to hold sip header value
			sipHeader.resize (length);

			RvStatus rvStatus = RvSipOtherHeaderGetValue(headerHandle, &sipHeader[0], length, (RvUint*)&length);
			stiTESTCONDMSG (rvStatus == RV_OK, stiRESULT_ERROR, "Failed to obtain value for Header %s rvStatus %d", headerName.c_str(), rvStatus);

			// RvSipOtherHeaderGetValue returns a null terminated string. Remove the null terminating character.
			sipHeader.resize (length - 1);

			headerValue = sipHeader;
		}

	STI_BAIL:

		return std::tuple<std::string, bool>{headerValue, present};
	}

	std::tuple<std::vector<Message::Method>, bool> Message::allowHeaderGet() const
	{
		RvSipHeaderListElemHandle hListElem;
		RvSipHeaderType HeaderType;
		std::vector<Method> allowHeader;
		bool present {false};

		auto hVoidHeader = RvSipMsgGetHeader (m_message, RVSIP_FIRST_HEADER, &hListElem, &HeaderType);

		while (nullptr != hVoidHeader)
		{
			if (RVSIP_HEADERTYPE_ALLOW == HeaderType)
			{
				present = true;

				auto hAllowHeader = static_cast<RvSipAllowHeaderHandle>(hVoidHeader);

				allowHeader.push_back(translateMethodType(
					RvSipAllowHeaderGetMethodType(hAllowHeader),
					hAllowHeader, RvSipAllowHeaderGetStrMethodType));
			}

			hVoidHeader = RvSipMsgGetHeader (m_message, RVSIP_NEXT_HEADER, &hListElem, &HeaderType);
		}

		return std::tuple<std::vector<Message::Method>, bool>{allowHeader, present};
	}
}
}
