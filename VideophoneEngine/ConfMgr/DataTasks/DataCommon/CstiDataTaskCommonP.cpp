////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Module Name: CstiDataTaskCommon
//
//  File Name: CstiDataTaskCommon.h
//
//  Owner: Ting-Yu Yang
//
//	Abstract:
//		Data tasks common definitions
//
////////////////////////////////////////////////////////////////////////////////

#include "CstiDataTaskCommonP.h"
#include "payload.h"

// Instantiation of static member variables
int CstiDataTaskCommonP::m_nVideoClockRate = 0;
int CstiDataTaskCommonP::m_nAudioClockRate = 0;
int CstiDataTaskCommonP::m_nTextClockRate = 0;

/*!
 * \brief Constructor used by VP,VR,AP,VR,TP,TR
 */
CstiDataTaskCommonP::CstiDataTaskCommonP (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t callIndex,
	uint32_t maxRate,
	ECodec codec)
:
	m_session (session),
	m_callIndex (callIndex),
	m_maxRate (maxRate)
{
	NegotiatedCodecSet (codec);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiDataTaskCommon::Initialize
//
// Description: Base data class initialization function.
//
// Abstract:
//	This function provides shared initialization for the data tasks.
//
// Returns:
//	estiOK if successful, estiERROR otherwise.
//
stiHResult CstiDataTaskCommonP::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiDataTaskCommonP::Initialize);

	// Also calculate the audio, text, and video rates (so we only have to do the
	// divisions one time)
	// These clock rate is shifted left by 10 in order to give us more
	// precision after the divide. The final calculated values will be
	// shifted left after the multiplication.

	m_nVideoClockRate = nVIDEO_CLOCK_RATE / stiOSSysClkRateGet ();
	m_nAudioClockRate = nAUDIO_CLOCK_RATE / stiOSSysClkRateGet ();
	m_nTextClockRate = nTEXT_CLOCK_RATE / stiOSSysClkRateGet ();

	return stiRESULT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//; CstiDataTaskCommonP::~CstiDataTaskCommonP
//
//  Description:
//
//  Abstract:
//		
//
//  Returns: 
//
CstiDataTaskCommonP::~CstiDataTaskCommonP ()
{
	stiLOG_ENTRY_NAME (CstiDataTaskCommonP::~CstiDataTaskCommonP);
} // end CstiDataTaskCommonP::~CstiDataTaskCommonP


void CstiDataTaskCommonP::StatsLock ()
{
	// Take the mutex semaphore
	m_StatsLockMutex.lock();
}


void CstiDataTaskCommonP::StatsUnlock ()
{
	// Give the mutex semaphore
	m_StatsLockMutex.unlock();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiDataTaskCommonP::ByteCountAdd
//
// Description: Adds to the byte counter
//
// Abstract:
//
// Returns:
//	None
//
void CstiDataTaskCommonP::ByteCountAdd (uint32_t un32AmountToAdd)
{
	stiLOG_ENTRY_NAME (CstiDataTaskCommonP::ByteCountAdd);

	StatsLock ();

	// Yes! Set the byte count.
	m_un32ByteCount += un32AmountToAdd;

	StatsUnlock ();
	
} // end CstiDataTaskCommonP::ByteCountAdd


////////////////////////////////////////////////////////////////////////////////
//; CstiDataTaskCommonP::ByteCountCollect
//
// Description: Collects the number of bytes received since the last collection
//
// Abstract:
//
// Returns:
//	uint32_t - The byte count since last collection
//
uint32_t CstiDataTaskCommonP::ByteCountCollect ()
{
	stiLOG_ENTRY_NAME (CstiDataTaskCommonP::ByteCountCollect);

	uint32_t un32ByteCount = 0;

	StatsLock ();
	
	// Yes! Collect the byte count, and reset it.
	un32ByteCount = m_un32ByteCount;
	m_un32ByteCount = 0;

	StatsUnlock ();

	return (un32ByteCount);
	
} // end CstiDataTaskCommonP::ByteCountCollect


////////////////////////////////////////////////////////////////////////////////
//; CstiDataTaskCommonP::ByteCountReset
//
// Description: Resets the byte counter
//
// Abstract:
//
// Returns:
//	None
//
void CstiDataTaskCommonP::ByteCountReset ()
{
	stiLOG_ENTRY_NAME (CstiDataTaskCommonP::ByteCountReset);

	StatsLock ();
	
	// Yes! Reset the byte count.
	m_un32ByteCount = 0;

	StatsUnlock ();
} // end CstiDataTaskCommonP::ByteCountReset


void CstiDataTaskCommonP::PacketCountAdd ()
{
	StatsLock ();
	m_packetsSent++;
	StatsUnlock ();
}


uint32_t CstiDataTaskCommonP::PacketCountCollect ()
{
	StatsLock ();
	auto packetsSent = m_packetsSent;
	m_packetsSent = 0;
	StatsUnlock ();
	
	return packetsSent;
}


void CstiDataTaskCommonP::PacketCountReset ()
{
	StatsLock ();
	m_packetsSent = 0;
	StatsUnlock ();
}


/*!
* \brief Sets the channel on hold by registering a throw-away callback for unwanted media
*/
stiHResult CstiDataTaskCommonP::Hold (
	EHoldLocation location)
{
	auto hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (!m_abCurrentHoldState[location], stiRESULT_ERROR);

	hResult = m_session->hold ();
	stiTESTRESULT ();

	m_abCurrentHoldState[location] = true;

STI_BAIL:
	return hResult;
}


/*!
* \brief Resumes from a hold state
*/
stiHResult CstiDataTaskCommonP::Resume (
	EHoldLocation location)
{
	stiLOG_ENTRY_NAME (CstiRtpChannel::Resume);
	auto hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (m_abCurrentHoldState[location], stiRESULT_ERROR);

	hResult = m_session->resume ();
	stiTESTRESULT ();

	m_abCurrentHoldState[location] = false;

STI_BAIL:
	return hResult;
}


/*!
* \brief Return the negotiated codec in use with this channel
* \param none
* \return The negotiated codec by this channel
*/
ECodec CstiDataTaskCommonP::NegotiatedCodecGet () const
{
	return m_negotiatedCodec;
}


/*!
 * \brief Set the negotiated codec to use
 */
void CstiDataTaskCommonP::NegotiatedCodecSet (
	ECodec codec)
{
	m_negotiatedCodec = codec;

	// Set the payload type
	switch (codec)
	{
		case estiH263_1998_VIDEO: // The actual payload bytes shouldn't be different
		case estiH263_VIDEO:
			m_payloadTypeNbr = RV_RTP_PAYLOAD_H263;
			break;

		case estiG711_ALAW_64K_AUDIO:
			m_payloadTypeNbr = RV_RTP_PAYLOAD_PCMA;
			break;

		case estiG711_ULAW_64K_AUDIO:
			m_payloadTypeNbr = RV_RTP_PAYLOAD_PCMU;
			break;

		case estiG722_64K_AUDIO:
			m_payloadTypeNbr = RV_RTP_PAYLOAD_G722;
			break;
		case estiH265_VIDEO:
		case estiH264_VIDEO:
		case estiT140_TEXT:
		default:
			// Do nothing, at this point.  For outbound dynamic payloads, they are to be set in the "Open" method
			// for that type.  For inbound dynamic payloads, they are to be set as the information is known.
			break;
	}
}


/*!
* \brief Store the current flow control rate
*/
void CstiDataTaskCommonP::FlowControlRateSet (
	int rate)
{
	m_flowControlRate = rate;
}


/*!
 * \brief Gets the current flow control rate
 */
int CstiDataTaskCommonP::FlowControlRateGet () const
{
	return m_flowControlRate;
}


/*!
* \brief What was the negotiated maximum channel rate?
* \note This is the minimum of the negotiated call rate, the UI MaxSend and MaxRecv speeds
* \note and the negotiated channel rate.
*/
int CstiDataTaskCommonP::MaxChannelRateGet () const
{
	return m_maxRate;
}


/*!
* \brief Store the negotiated maximum channel rate
*/
void CstiDataTaskCommonP::MaxChannelRateSet (
	int rate)
{
	m_maxRate = rate;
}


/*!
* \brief Sets the privacy mode
*/
void CstiDataTaskCommonP::PrivacyModeSet (
	EstiSwitch mode)
{
	m_privacyMode = mode;
}


/*!
* \brief Returns the current privacy mode
*/
EstiSwitch CstiDataTaskCommonP::PrivacyModeGet () const
{
	return m_privacyMode;
}


/*!
* \brief Returns the current payload type ID
*/
int8_t CstiDataTaskCommonP::PayloadTypeGet () const
{
	return m_payloadTypeNbr;
}


/*!
* \brief Sets the payload type id
*/
void CstiDataTaskCommonP::PayloadTypeSet (
	int8_t payloadTypeNbr)
{
	m_payloadTypeNbr = payloadTypeNbr;
}


/*!
 * \brief Gets local addresses for RTP and RTCP
 */
stiHResult CstiDataTaskCommonP::LocalAddressesGet (
	std::string *rtpAddress,
	int *rtpPort,
	std::string *rtcpAddress,
	int *rtcpPort)
{
	return m_session->LocalAddressGet (
		rtpAddress, rtpPort, rtcpAddress, rtcpPort);
}


/*!
 * \brief Returns true if TMMBR was negotiated
 */
bool CstiDataTaskCommonP::tmmbrNegotiatedGet () const
{
	return m_session && m_session->rtcpFeedbackTmmbrEnabledGet();
}



#if 0
////////////////////////////////////////////////////////////////////////////////
//; rtpRecvBufferUsageGet
//
// Description: Retrieve the sockets receive buffer usage
//
// Abstract:
//	This function, given an RTP session handle, returns the actual sockets 
//	receive buffer usage at the given moment
//
// Returns:
//	Current buffer usage (in bytes) of the socket passed in
//
uint32_t rtpRecvBufferUsageGet (
	IN stiHRTP_SESSION hRTP) // The rtp session handle
{
	stiLOG_ENTRY_NAME (rtpRecvBufferUsageGet);

	struct socket * pstSocket = 
		(struct socket *)iosFdValue (rtpSocketFDGet (hRTP));
			
	// Return the actual number of bytes queued
	return (pstSocket->so_rcv.sb_cc); 
} // end rtpRecvBufferUsageGet


////////////////////////////////////////////////////////////////////////////////
//; rtpSendBufferUsageGet
//
// Description: Retrieve the sockets send buffer usage
//
// Abstract:
//	This function, given an RTP session handle, returns the actual sockets 
//	send buffer usage at the given moment
//
// Returns:
//	Current buffer usage (in bytes) of the socket passed in
//
uint32_t rtpSendBufferUsageGet (
	IN stiHRTP_SESSION hRTP) // The rtp session handle
{
	struct socket * pstSocket = 
		(struct socket *)iosFdValue (rtpSocketFDGet (hRTP));
			
	// Return the actual number of bytes queued
	return (pstSocket->so_snd.sb_cc); 
} // end rtpSendBufferUsageGet
#endif


