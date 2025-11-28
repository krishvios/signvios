/*!
 * \file CstiSdp.cpp
 * \brief Wraps a pointer to the RvSdpMsg data structure so that it gets freed
 * when an object of this type goes out of scope.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#include "CstiSdp.h"
#include "stiSipConstants.h"
#include "stiTools.h"
#include <memory>


/*!\brief Initializes the object from a SIP message containing SDP or
 *
 *
 */
stiHResult CstiSdp::initialize (
	RvSipMsgHandle hMsg)
{
	auto hResult = stiRESULT_SUCCESS;
	RvSdpMsgType sdpMsg;

	if (hMsg == nullptr)
	{
		sdpMsg = RvSdpMsgType(rvSdpMsgConstruct (nullptr), rvSdpDeleter);
		stiTESTCOND (sdpMsg != nullptr, stiRESULT_ERROR);
	}
	else
	{
		int nStringLen = 0;

		// How big is the SDP message?
		nStringLen = RvSipMsgGetStringLength (hMsg, RVSIP_MSG_BODY);

		if (nStringLen > 0)
		{
			auto rvStatus = RV_OK;
			auto bufLen = 0U;

			// Allocate a buffer sufficient for the message
			auto sdpMsgBuffer = sci::make_unique<char[]>(nStringLen + 1);
			stiTESTCOND (sdpMsgBuffer != nullptr, stiRESULT_ERROR);

			// Retrieve the content type header
			rvStatus = RvSipMsgGetContentTypeHeader (hMsg,
					sdpMsgBuffer.get (), nStringLen, &bufLen);
			stiTESTRVSTATUS ();

			// Was it an SDP header?
			if (0 == strncmp (sdpMsgBuffer.get (), SDP_MIME, nStringLen))
			{
				// It is an SDP message.  Retrieve the message body
				rvStatus = RvSipMsgGetBody (hMsg, sdpMsgBuffer.get (), nStringLen, &bufLen);
				stiTESTRVSTATUS ();

				//
				// Get the string length (again?)
				///\todo: check to see if we need to do this
				//
				nStringLen = strlen (sdpMsgBuffer.get ());

				RvSdpParseStatus eSdpParseStatus;

				sdpMsg = RvSdpMsgType(rvSdpMsgConstructParse (nullptr, (RvChar*)sdpMsgBuffer.get (), &nStringLen, &eSdpParseStatus), rvSdpDeleter);
				stiTESTCOND (RV_SDPPARSER_STOP_ERROR != eSdpParseStatus, stiRESULT_ERROR);
				stiTESTCOND (sdpMsg != nullptr, stiRESULT_ERROR);
			}
		}
	}

	m_sdpMsg = std::move(sdpMsg);

STI_BAIL:

	return hResult;

}


/*!\brief Print the SDP
 *
 *
 */
void CstiSdp::Print () const
{
	std::string sdpMsg;

	auto hResult = encodeToString(sdpMsg);

	if (!stiIS_ERROR (hResult))
	{
		stiTrace ("%s\n", sdpMsg.c_str ());
		stiTrace ("SDP Length: %d\n", sdpMsg.length ());
	}
}


/*!\brief Get the number of media descriptors
 *
 *
 */
int CstiSdp::numberOfMediaDescriptorsGet () const
{
	return rvSdpMsgGetNumOfMediaDescr (m_sdpMsg.get ());
}


/*!\brief Get the media descriptor at the specified index
 *
 *
 */
const RvSdpMediaDescr *CstiSdp::mediaDescriptorGet (int i) const
{
	return rvSdpMsgGetMediaDescr (m_sdpMsg.get (), i);
}


/*!\brief Get the media descriptor at the specified index
 *
 *
 */
RvSdpMediaDescr *CstiSdp::mediaDescriptorGet (int i)
{
	return rvSdpMsgGetMediaDescr (m_sdpMsg.get (), i);
}


/*!\brief Add a media descriptor
 *
 *
 */
RvSdpMediaDescr *CstiSdp::mediaDescriptorAdd (
	RvSdpMediaType mediaType,
	RvInt32 rtpPort,
	RvSdpProtocol mediaProtocol)
{
	return rvSdpMsgAddMediaDescr (m_sdpMsg.get (), mediaType,
			rtpPort, mediaProtocol);
}


/*!\brief Get the origin
 *
 *
 */
const RvSdpOrigin *CstiSdp::originGet () const
{
	return rvSdpMsgGetOrigin (m_sdpMsg.get ());
}


/*!\brief Get the connection
 *
 *
 */
const RvSdpConnection *CstiSdp::connectionGet () const
{
	return rvSdpMsgGetConnection (m_sdpMsg.get ());
}


/*!\brief Get the connection
 *
 *
 */
RvSdpConnection *CstiSdp::connectionGet ()
{
	return rvSdpMsgGetConnection (m_sdpMsg.get ());
}


/*!\brief Get the bandwidth
 *
 *
 */
const RvSdpBandwidth *CstiSdp::bandwidthGet () const
{
	return rvSdpMsgGetBandwidth (m_sdpMsg.get ());
}


/*!\brief Get the number of attributes
 *
 *
 */
size_t CstiSdp::numberOfAttributesGet () const
{
	return rvSdpMsgGetNumOfAttr2 (m_sdpMsg.get ());
}


/*!\brief Get the attribute at the specified index
 *
 *
 */
const RvSdpAttribute *CstiSdp::attributeGet (size_t i) const
{
	return rvSdpMsgGetAttribute2 (m_sdpMsg.get (), i);
}


/*!\brief Set the media descriptor's port at the specified index
 *
 *
 */
void CstiSdp::mediaDescriptorPortSet (int i, int port)
{
	auto pMediaDescriptor = rvSdpMsgGetMediaDescr (m_sdpMsg.get (), i);

	rvSdpMediaDescrSetPort (pMediaDescriptor, 0);
}


/*!\brief Encode the SDP to a buffer
 *
 *
 */
stiHResult CstiSdp::encodeToString (
	std::string &sdpBuffer) const
{
	auto hResult = stiRESULT_SUCCESS;
	RvSdpStatus encodeStatus;

	sdpBuffer.resize (MAX_SDP_MESSAGE_SIZE);

	rvSdpMsgEncodeToBuf (m_sdpMsg.get (), const_cast<RvChar *>(sdpBuffer.c_str ()), static_cast<int>(sdpBuffer.size ()), &encodeStatus);
	stiTESTCOND (RV_SDPSTATUS_OK == encodeStatus, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


/*!\brief Get the pointer to the SDP structure
 *
 *
 */
RvSdpMsg *CstiSdp::sdpMessageGet () const
{
	return m_sdpMsg.get ();
}


/*!\brief Set the origin
 *
 *
 */
stiHResult CstiSdp::originSet (
	const std::string &username,
	const std::string &sessionId,
	const std::string &version,
	RvSdpNetType netType,
	RvSdpAddrType addrType,
	const std::string &address)
{
	auto hResult = stiRESULT_SUCCESS;
	auto rvStatus = RV_OK;

	rvStatus = rvSdpMsgSetOrigin (m_sdpMsg.get (), username.c_str (), sessionId.c_str (),
			version.c_str (), netType, addrType, address.c_str ());
	stiTESTRVSTATUS ();

STI_BAIL:

	return hResult;
}


/*!\brief Set the session name
 *
 *
 */
stiHResult CstiSdp::sessionNameSet (
	const std::string &sessionName)
{
	auto hResult = stiRESULT_SUCCESS;
	auto rvStatus = RV_OK;

	rvStatus = rvSdpMsgSetSessionName (m_sdpMsg.get (), sessionName.c_str ());
	stiTESTRVSTATUS ();

STI_BAIL:

	return hResult;
}


/*!\brief Set the connection
 *
 *
 */
stiHResult CstiSdp::connectionSet (
	RvSdpNetType netType,
	RvSdpAddrType addrType,
	const std::string &address)
{
	auto hResult = stiRESULT_SUCCESS;
	auto rvStatus = RV_OK;

	rvStatus = rvSdpMsgSetConnection (m_sdpMsg.get (), netType, addrType, address.c_str ());
	stiTESTRVSTATUS ();

STI_BAIL:

	return hResult;
}


/*!\brief Set the session time
 *
 *
 */
void CstiSdp::sessionTimeAdd (
	RvSdpTimeVal start,
	RvSdpTimeVal stop)
{
	rvSdpMsgAddSessionTime (m_sdpMsg.get (), start, stop);
}


/*!\brief Set the bandwidth
 *
 *
 */
void CstiSdp::bandwidthAdd (
	const std::string &bwType,
    int bandwidth)
{
	rvSdpMsgAddBandwidth (m_sdpMsg.get (), bwType.c_str (), bandwidth);
}


/*!\brief Set the attribute
 *
 *
 */
void CstiSdp::attributeAdd (
	const std::string &name,
	const std::string &value)
{
	rvSdpMsgAddAttr (m_sdpMsg.get (), name.c_str (), value.c_str ());
}


/*!\brief Search sdp for a specific attribute.
 *
 *
 */
bool CstiSdp::mediaTypeContainsAttribute (
	const std::string &mediaTypeWanted,
	const std::string &attributeName)
{
	bool attributeFound = false;
	
	for (unsigned int i = 0; i < rvSdpMsgGetNumOfMediaDescr(sdpMessageGet()); i++)
	{
		const RvSdpMediaDescr *mediaDescriptor = mediaDescriptorGet (i);
		std::string mediaType = rvSdpMediaDescrGetMediaTypeStr (mediaDescriptor);
		
		if (mediaType == mediaTypeWanted)
		{
			for (unsigned int j = 0; j < rvSdpMediaDescrGetNumOfAttr2 (const_cast<RvSdpMediaDescr *>(mediaDescriptor)); j++)
			{
				auto mediaAttribute = rvSdpMediaDescrGetAttribute2 (const_cast<RvSdpMediaDescr *>(mediaDescriptor), j);
				if (mediaAttribute)
				{
					std::string mediaAttributeName = rvSdpAttributeGetName (mediaAttribute);
					if (mediaAttributeName.compare(attributeName) == 0)
					{
						attributeFound = true;
						break;
					}
				}
			}
			break;
		}
	}
	
	return attributeFound;
}
