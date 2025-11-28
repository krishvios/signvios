/*!
* \file CstiTextPlaybackRead.cpp
* \brief Receives text packets from the Rtp session / socket
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"

#include "CstiTextPlaybackRead.h"
#include "CstiDataTaskCommonP.h"
#include "stiTaskInfo.h"
#include "stiPayloadTypes.h"
#include "RtpPayloadAddon.h"


//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

EstiBool g_bFirstTextNTPTimestamp;


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::CstiTextPlaybackRead
//
// Description: Constructor
//
// Abstract:
//
// Returns:
//	None
//
CstiTextPlaybackRead::CstiTextPlaybackRead (const std::shared_ptr<vpe::RtpSession> &session)
:
	PlaybackReadType(stiTP_READ_TASK_NAME, session)
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::CstiTextPlaybackRead);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::~CstiTextPlaybackRead
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	None
//
CstiTextPlaybackRead::~CstiTextPlaybackRead ()
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::~CstiTextPlaybackRead);

	Shutdown ();

	// Empty the packet queue
	m_oFullQueue.clear ();
	
	// Destroy the packet pool
	m_emptyPacketPool.destroy ();

} // end CstiTextPlaybackRead::~CstiTextPlaybackRead


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::Initialize
//
// Description: Initializes the task
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure
//
stiHResult CstiTextPlaybackRead::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::Initialize);

	stiDEBUG_TOOL ( g_stiTPReadDebug,
		stiTrace("<CstiTextPlaybackRead::Initialize>\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Create the empty packet pool with the buffered packets
	m_emptyPacketPool.create (nTEXT_PACKETS_ALLOCATED);

	// Initialize the empty pools's text buffers
	for (int i = 0; i < nTEXT_PACKETS_ALLOCATED; i++)
	{
		// Allocate the buffers
		if (m_textReadBuffer[i].empty())
		{
			m_textReadBuffer[i].resize(nMAX_TEXT_PLAYBACK_BUFFER_BYTES + 1);
		}

		m_astTextStruct[i].pun8Buffer = &m_textReadBuffer[i][0];
		m_astTextStruct[i].unBufferMaxSize = nMAX_TEXT_PLAYBACK_BUFFER_BYTES;

		// Add all structures to the packets in the pool.
		m_emptyPacketPool.bufferAdd (&m_textReadBuffer[i][0], 0, &m_astTextStruct[i]);

	} // end for

//STI_BAIL:

	return (hResult);

} // end CstiTextPlaybackRead::Initialize


/*!
 * \brief Read and process a packet from the RTP Port
 */
void CstiTextPlaybackRead::EventRtpSocketDataAvailable ()
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::EventRtpSocketDataAvailable);

	EstiPacketResult ePacketResult = ePACKET_OK;

	// Get an empty packet from the queue
	auto packet = m_emptyPacketPool.packetGet ();

	// Were we able to get a packet?
	if (!packet)
	{
		// No! This is a major problem!
		ePacketResult = ePACKET_ERROR;
		stiASSERT (estiFALSE);
	}
	else
	{
		// Read a packet from the RTP layer and place it into this packet.
		ePacketResult = PacketRead (packet);

#if 0
	static uint16_t un16RandNumber = 5000;
	if (((poPacket->RTPParamGet ())->sequenceNumber) % un16RandNumber == 0)
	{
	  srand(stiOSTickGet ());
	  un16RandNumber = (uint16_t) rand();
	  un16RandNumber = un16RandNumber % 5000;

	  stiTrace("TPR:: dropping SQ# %d\n", (packet->RTPParamGet ())->sequenceNumber);
	  ePacketResult = ePACKET_CONTINUE;
	}
#endif

		// Was the packet read successful
		if (ePACKET_OK == ePacketResult)
		{
			// Yes, We got a packet, so process it now.
			ReadPacketProcess (packet);
		}
	}

}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::PacketRead
//
// Description: Read a packet from the RTP layer and place it into this packet.
//
// Abstract:
//
// Returns:
//	ePACKET_OK - read successful
//	ePACKET_ERROR - read failed
//	ePACKET_CONTINUE - no data available to read
//
EstiPacketResult CstiTextPlaybackRead::PacketRead (
	const std::shared_ptr<CstiTextPacket> &packet)	// Packet received from socket
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::PacketRead);

	EstiPacketResult eReadResult = ePACKET_CONTINUE;
	int nBytesRead;
	RvRtpParam * pstRTPParam = packet->RTPParamGet ();


	// Do we have a valid session handle?
	if (m_rtpSession->rtpGet() && !m_rtpSession->rtpTransportGet())
	{
		// Yes! Read the data from the socket. NOTE - the Radvision
		// documentation is incorrect. RvRtpRead does NOT return the number of
		// bytes read, it returns the result of rtpUnpack.  Was RvRtpRead
		// successful?
		// NOTE:  Even if m_bRtpHalted is true, we still want to read the packet
		//        off the socket so we can discard it.
		auto readResult =
			m_rtpSession->RtpRead (
				packet->BufferGet (),
				nMAX_TEXT_PLAYBACK_BUFFER_BYTES,
				pstRTPParam);

		if (readResult >= 0 && !m_bRtpHalted && pstRTPParam->len)
		{
			extendedSequenceNumberDetermine (packet);
			
			//
			// Indicate that the payload is at the beginning of the buffer.
			//
			packet->RtpHeaderSet (packet->BufferGet ());
			packet->PayloadSet (packet->RtpHeaderGet () + pstRTPParam->sByte);
			
			// Yes! Find out how many bytes we actually read
			nBytesRead = pstRTPParam->len;

			// Did we read any data?
			if (nBytesRead > 0)
			{

				// Yes! Do we have a handle to the RTCP Session?
				if (nullptr != m_rtpSession->rtcpGet())
				{
					// Yes! Notify the rtcp session that the packet was received
					// (use 90kHz clock frequency).
					RvRtcpRTPPacketRecv (
						m_rtpSession->rtcpGet(),
						pstRTPParam->sSrc,
						(stiOSTickGet() - m_un32InitialRTCPTime) * m_mediaClockRate,
						pstRTPParam->timestamp,
						pstRTPParam->sequenceNumber);

				} // end if

				// We were successful with the read, set the return code
				// to reflect this.
				eReadResult = ePACKET_OK;
			} // end if
			else
			{
				// No! We failed to read any valid data.
				eReadResult = ePACKET_CONTINUE;
			} // end else
		}
		else
		{
			// No! We failed to read any valid data.
			eReadResult = ePACKET_CONTINUE;
		} // end else
	} // end if

	return (eReadResult);
} // end CstiTextPlaybackRead::PacketRead


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::DataUnPack
//
// Description: Calls the RadVision rtpXXXUnpack function
//
// Abstract:
//	This function determines the type of text and calls the appropriate rtp
//	unpack function.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextPlaybackRead::DataUnPack (
	const std::shared_ptr<CstiTextPacket> &packet,	// Buffer to be unpacked
	bool *pbIsTextPacket)  // Flag whether this is text data or not
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::DataUnPack);

	stiHResult hResult = stiRESULT_SUCCESS;
	uint8_t* paun8TextBuffer = packet->PayloadGet ();
	SsmdTextFrame * pstTextFrame = packet->m_frame;
	RvRtpParam* pstRTPParam = packet->RTPParamGet ();
	*pbIsTextPacket = false;

#if 0
static uint32_t un32SQExpecting = 0;

if (0 == un32SQExpecting)
{
	un32SQExpecting = pstRTPParam->sequenceNumber;
}
else if (un32SQExpecting != pstRTPParam->sequenceNumber)
{
	stiTrace("TPR:: ******* lost packet SQ# %d, next SQ# %d Empty = %d FULL = %d *******\n",
	un32SQExpecting,
	pstRTPParam->sequenceNumber,
	m_emptyPacketPool.count (),
	m_oFullQueue.CountGet ());

	un32SQExpecting =  pstRTPParam->sequenceNumber;
}

un32SQExpecting ++;
#endif

	//
	// Given the payload type, lookup the text codec
	// that it maps to.
	//
	EstiTextCodec eTextCodec = estiTEXT_NONE;
	TTextPayloadMap::const_iterator i;

	i = m_PayloadMap.find (pstRTPParam->payload);

	if (i != m_PayloadMap.end ())
	{
		eTextCodec = i->second.eCodec;
		// If need to support different packetization schemes for text,
		// the scheme for this payload map is in i->second.ePacketizationScheme
	}

	// Unpack the data and perform any processing on the data stream header.
	switch (eTextCodec)
	{
		case estiTEXT_T140:
		case estiTEXT_T140_RED:
			*pbIsTextPacket = true;
			stiDEBUG_TOOL (g_stiTPReadDebug,
				stiTrace ("<CstiTextPlaybackRead::DataUnPack> Text SQ# %d Empty = %d Full = %d size = %d\n",
				pstRTPParam->sequenceNumber, m_emptyPacketPool.count (),
				m_oFullQueue.CountGet (), pstRTPParam->len - pstRTPParam->sByte);
			); // stiDEBUG_TOOL

			if (0 > rtpT140Unpack (paun8TextBuffer, nMAX_TEXT_PLAYBACK_BUFFER_BYTES, pstRTPParam, nullptr))
			{
				stiTHROW (stiRESULT_ERROR);
			} // end if
			break;
			
		default:
			if (pstRTPParam->len - pstRTPParam->sByte >= 1) // if there are no bytes, this is just a keepalive packet
			{
				stiDEBUG_TOOL (g_stiTPReadDebug,
					stiTrace ("<CstiTextPlaybackRead::DataUnPack> Received data for an unregistered Payload Type[%d]\n", pstRTPParam->payload);
				);
				stiTHROW (stiRESULT_ERROR);
			}
			else
			{
				stiDEBUG_TOOL (g_stiTPReadDebug,
					stiTrace ("<CstiTextPlaybackRead::DataUnPack> RTP keepalive (text) received - SQ#: %u\n", pstRTPParam->sequenceNumber);
				);
			}

			break;
	} // end switch

	// Yes! Calculate the offset into the actual Text data and store into
	// the text frame structure for the driver.
	pstTextFrame->pun8Buffer = packet->PayloadGet ();

	// Save the number of bytes of Text we received into the structure.
	pstTextFrame->unFrameSizeInBytes = pstRTPParam->len - pstRTPParam->sByte;

STI_BAIL:

	return (hResult);

} // end CstiTextPlaybackRead::DataUnPack


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::ReadPacketProcess
//
// Description: Process the packet that was read from the port
//
// Abstract:
//
// Returns:
//	stiHResult - success or failure
//
stiHResult CstiTextPlaybackRead::ReadPacketProcess (
	const std::shared_ptr<CstiTextPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::ReadPacketProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	bool bIsTextPacket = false;

	// Unpack the data into it's raw form
	hResult = DataUnPack (packet, &bIsTextPacket);
	stiTESTRESULT ();

	if (bIsTextPacket)
	{
#if 0
		//
		// Check the codec type to decide the jitter variables
		//
		if (m_bSetJitter)
		{
			// Has the codec changed?
			switch ((packet->RTPParamGet ())->payload)
			{
				case RV_RTP_PAYLOAD_PCMA:
				case RV_RTP_PAYLOAD_PCMU:
					m_bSetJitter = estiFALSE;
					m_nPacketsToHold = NumTextPacketAvailable ();
					m_nPacketsToHold = (m_nPacketsToHold < nT140_TEXT_PACKETS_BUFFERED) ?
						(nT140_TEXT_PACKETS_BUFFERED - m_nPacketsToHold) : nT140_TEXT_PACKETS_BUFFERED;
					m_nPacketsHeld = 0;
					break;
				default:
					stiTHROW (stiRESULT_ERROR);
					break;
			}
		}
#endif

		// Is this the only empty packet that we have?
		if ((estiFALSE == m_packetDiscarding && m_emptyPacketPool.count () == 0) ||
		(estiTRUE == m_packetDiscarding && m_emptyPacketPool.count () < 3))
		{
			m_packetDiscarding = estiTRUE;

			// Yes! If this is the only empty packet, then it means the system is
			// saturated.
			// Since we are saturated, throw this packet away

			stiDEBUG_TOOL (g_stiTPReadDebug,
				if (0 == m_nDiscardedTextPackets)
				{
					stiTrace ("<CstiTextPlaybackRead::ReadPacketProcess> Discarding text packets SQ# %d...\n",
						(packet->RTPParamGet ())->sequenceNumber);
				} // end if
				m_nDiscardedTextPackets++;
			); // stiDEBUG_TOOLS

		} // end if
		else
		{
			m_packetDiscarding = estiFALSE;

			// No! There are other empty packets, so it is OK to add this one to
			// the full queue
			m_oFullQueue.Add (packet);


			// Do we need to hold some packets for the beginning of a call?

			if (m_nPacketsToHold > 0)
			{

				// Yes! Just hold this packet until we have enough
				m_nPacketsHeld++;
				m_nPacketsToHold--;

				stiDEBUG_TOOL (g_stiTPReadDebug,
					stiTrace ("<CstiTextPlaybackRead::ReadPacketProcess> Now holding %d text packets SQ# %d...\n", m_nPacketsHeld,
					(packet->RTPParamGet ())->sequenceNumber);
				); // stiDEBUG_TOOLS
			} // end if
			else
			{
				// No! Do we have any packets that we have been holding?
				if (m_nPacketsHeld > 0)
				{
					stiDEBUG_TOOL (g_stiTPReadDebug,
						stiTrace ("<CstiTextPlaybackRead::ReadPacketProcess> Releasing %d text packets...\n", m_nPacketsHeld);
					); // stiDEBUG_TOOLS

					// Yes! Let them go to the playback thread all at once.

					while (m_nPacketsHeld > 0)
					{
						dataAvailableSignal.Emit ();
						m_nPacketsHeld--;
					} // end while

					// We also have the packet that we just received.
					dataAvailableSignal.Emit ();
				} // end if
				else
				{
					// No! Let the read task know that this packet is available
					dataAvailableSignal.Emit ();

					/*
					stiDEBUG_TOOL (g_stiTPReadDebug,
					static int nCnt = 0;
					if (++nCnt == 1)
					{
					stiTrace ("TPR:: Buffer Empty Left: %d\n", m_oEmptyQueue.CountGet ());
					nCnt = 0;
					}
					); // stiDEBUG_TOOLS
					*/
				} // end else
			} // end else


			stiDEBUG_TOOL (g_stiTPReadDebug,
				if (m_nDiscardedTextPackets > 0)
				{
					stiTrace (
						"<CstiTextPlaybackRead::ReadPacketProcess> Discarded Text packets from %d to %d, empty = %d full = %d\n\n",
						(packet->RTPParamGet ())->sequenceNumber - m_nDiscardedTextPackets,
						(packet->RTPParamGet ())->sequenceNumber - 1,
						m_emptyPacketPool.count (),
						m_oFullQueue.CountGet ());
					m_nDiscardedTextPackets = 0;
				} // end if
			); // stiDEBUG_TOOLS

		} // end else

	}
	else
	{
		// If there are no bytes, this is a keepalive packet
		if (packet->isKeepAlive())
		{
			m_oFullQueue.Add (packet);
			dataAvailableSignal.Emit ();
		}
	}

STI_BAIL:

	return (hResult);
} // end CstiTextPlaybackRead::ReadPacketProcess



//:-----------------------------------------------------------------------------
//:
//:	Public Methods
//:
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; VREnumerator
//
// Description: Enumerator function for RTCP information
//
// Abstract:
//
// Returns:
//	BOOL - FALSE if enumeration should continue, otherwise TRUE
//
RvBool CstiTextPlaybackRead::TextEnumerator (
	RvRtcpSession hRTCP,	// Handle to RadVision RTCP Session
	RvUint32 ssrc,		// Synchronization Source ID
	void *pContext)
{
	RvBool bResult = RV_FALSE;

	RvRtcpINFO stRTCPInfo;
	auto pThis = (CstiTextPlaybackRead *)pContext;

	// Get the RTCP information from the stack.

	RvRtcpGetSourceInfo (hRTCP, ssrc, &stRTCPInfo);
#if 0
	stiDEBUG_TOOL (g_stiRTCPDebug,
		stiTrace ("TP-hRTCP=0x%X, SSRC=0x%X, CNAME=%s\n\n", hRTCP, ssrc, stRTCPInfo.cname);
	); // stiDEBUG_TOOL
#endif
	if(stRTCPInfo.sr.valid && stRTCPInfo.sr.packets > 0)
	{

		pThis->m_nNumPackets = static_cast<int>(stRTCPInfo.sr.packets) - pThis->m_nNumPackets;

		if (pThis->m_nNumPackets > 0)
		{

			stiDEBUG_TOOL (g_stiRTCPDebug,
				stiTrace ("\nText SR: mNTPtimestamp = %lu, lNTPtimestamp = %lu, timestamp = %lu, packets = %u\n\n",
				stRTCPInfo.sr.mNTPtimestamp,	// fraction part
				stRTCPInfo.sr.lNTPtimestamp,	// second part
				stRTCPInfo.sr.timestamp,
				stRTCPInfo.sr.packets);
			); // stiDEBUG_TOOL


			//
			// TYY May.27, 2004 - NOTE: NTP format (H.323 v4.0.0.48)
			//
			// stRTCPInfo.sr.lNTPtimestamp contains the second part, the most significant 32 bits, of the NTP timestamp
			// stRTCPInfo.sr.mNTPtimestamp contains the fraction part, the least significant 32 bits, of the NTP timestamp
			//
			// TYY Jan.7, 2005 - NOTE: NTP format  (H.323 v4.2.2.3)
			//
			// stRTCPInfo.sr.mNTPtimestamp contains the second part, the most significant 32 bits, of the NTP timestamp
			// stRTCPInfo.sr.lNTPtimestamp contains the fraction part, the least significant 32 bits, of the NTP timestamp
			//
			// We need to deal with the inconsistencies

		}
		pThis->m_nNumPackets = stRTCPInfo.sr.packets;
	}

	return (bResult);
} // end enumerator


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlaybackRead::TextRTCPReportProcess
//
// Description: Event handler for incoming RTCP data
//
// Abstract:
//	This method is called by the RADVision stack upon receipt of incoming data
//	for the session identified by the parameter hRCTP.
//
// Returns:
//	none
//
void CstiTextPlaybackRead::TextRTCPReportProcess (
	RvRtcpSession hRTCP,	// The RTCP session for which the data has arrived
	void* pContext,
	RvUint32 ssrc)
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::TextRTCPReportProcess);

	// Collect the RTCP data from the stack (using VREnumerator function)
	RvRtcpEnumParticipants (
		hRTCP,
		TextEnumerator,
		pContext);
} // end stiRTCPReportProcess


/*!
 * \brief Post a message that the channel is closing.
 *
 * This method posts a message to the Text Playback task that the
 * channel is closing.
 *
 * \return stiRESULT_SUCCESS when successful, otherwise an error
 */
stiHResult CstiTextPlaybackRead::DataChannelClose ()
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::DataChannelClose);
	stiDEBUG_TOOL ( g_stiTPReadDebug,
		printf("CstiTextPlaybackRead::DataChannelClose\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!m_dataChannelClosed)
	{
		MediaPlaybackRead::DataChannelClose();

		g_bFirstTextNTPTimestamp = estiTRUE;
	}

	return hResult;
}


/*!
 * \brief Event handler used to process packets received from the TURN server
 */
void CstiTextPlaybackRead::EventRtpPacketReceived (const std::shared_ptr<CstiTextPacket> &packet)
{
	if (!m_bRtpHalted)
	{
		// Do we have a handle to the RTCP Session?
		if (nullptr != m_rtpSession->rtcpGet())
		{
			// Yes! Notify the rtcp session that the packet was received
			// (use 90kHz clock frequency).
			RvRtpParam *pRtpParam = packet->RTPParamGet ();

			RvRtcpRTPPacketRecv (
				m_rtpSession->rtcpGet(),
				pRtpParam->sSrc,
				(stiOSTickGet() - m_un32InitialRTCPTime) * m_mediaClockRate,
				pRtpParam->timestamp,
				pRtpParam->sequenceNumber);
		} // end if

		ReadPacketProcess (packet);
	}
}

void CstiTextPlaybackRead::RTPOnReadEvent(RvRtpSession hRTP)
{
	CstiTextPlaybackRead *pThis = this;
	
	// NOTE: Avoid accessing class data members in the list (except for the packet queue)
	// to avoid the need for locking the task's mutex.  This method is called by Radvision through
	// another thread.
	
	//
	// Lock the queue, grab the oldest available, remove it from the queue and then unlock the queue.
	//
	auto packet = pThis->m_emptyPacketPool.packetGet ();

	if (packet)
	{
		// Yes! Read the data from the socket. NOTE - the Radvision
		// documentation is incorrect. RvRtpRead does NOT return the number of
		// bytes read, it returns the result of rtpUnpack.  Was RvRtpRead
		// successful?
		if (0 <= pThis->m_rtpSession->RtpRead (
			packet->BufferGet (),
			nMAX_TEXT_PLAYBACK_BUFFER_BYTES,
			packet->RTPParamGet ()) &&
			packet->RTPParamGet()->len)
		{
			extendedSequenceNumberDetermine (packet);
			
			//
			// Indicate that the payload is at the beginning of the buffer.
			//
			packet->RtpHeaderSet (packet->BufferGet ());

			packet->PayloadSet (packet->RtpHeaderGet () + (packet->RTPParamGet ())->sByte);
			
			pThis->PostEvent (
				std::bind(&CstiTextPlaybackRead::EventRtpPacketReceived, pThis, packet));
		}
	}
}

/*!
 * \brief specializations of initialization for text
 *
 * \return stiRESULT_SUCCESS when successful, otherwise stiRESULT_ERROR
 */
stiHResult CstiTextPlaybackRead::DataChannelInitialize (
	uint32_t un32InitialRtcpTime,
	const TTextPayloadMap *pPayloadMap)
{
	stiLOG_ENTRY_NAME (CstiTextPlaybackRead::DataChannelInitialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_dataChannelClosed)
	{
		hResult = MediaPlaybackRead::DataChannelInitialize(un32InitialRtcpTime, pPayloadMap);

		// Register the RTCP report handler
		// TODO: could put this in the base class, but video doesn't do this here
		if (nullptr != m_rtpSession->rtcpGet())
		{
			RvRtcpSetRTCPRecvEventHandler(m_rtpSession->rtcpGet(), TextRTCPReportProcess, this);
		}

		g_bFirstTextNTPTimestamp = estiTRUE;
	}

	return hResult;
}

