////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiAudioPlaybackRead
//
//  File Name:	CstiAudioPlaybackRead.cpp
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:
//		Audio playback Read class definition
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"

#include "IstiAudioOutput.h"
#include "CstiAudioPlaybackRead.h"
#include "CstiDataTaskCommonP.h"
#include "stiTaskInfo.h"
#include "stiPayloadTypes.h"
#include "CstiSyncManager.h"

#include <boost/optional.hpp>


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

EstiBool g_bFirstAudioNTPTimestamp;


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::CstiAudioPlaybackRead
//
// Description: Constructor
//
// Abstract:
//
// Returns:
//	None
//
CstiAudioPlaybackRead::CstiAudioPlaybackRead (const std::shared_ptr<vpe::RtpSession> &session)
:
	PlaybackReadType(stiAP_READ_TASK_NAME, session)
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::CstiAudioPlaybackRead);
} // end CstiAudioPlaybackRead::CstiAudioPlaybackRead

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::~CstiAudioPlaybackRead
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	None
//
CstiAudioPlaybackRead::~CstiAudioPlaybackRead ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::~CstiAudioPlaybackRead);

	Shutdown ();

	// Make sure we didn't accidentally lose any buffers in the transistions between queues
	auto packetCount =
		m_emptyPacketPool.count () +
		m_oFullQueue.CountGet () +
		m_oRecvdQueue.CountGet ();

	stiASSERTMSG (packetCount == nAUDIO_PACKETS_ALLOCATED,
		"Not all buffers accounted for (%d+%d+%d != %d) - memory leak\n",
		m_emptyPacketPool.count (),
		m_oFullQueue.CountGet (),
		m_oRecvdQueue.CountGet (),
		nAUDIO_PACKETS_ALLOCATED);

	// Empty the queues
	m_oRecvdQueue.clear ();
	m_oFullQueue.clear ();
	
	// Destroy the packet pool
	m_emptyPacketPool.destroy ();

} // end CstiAudioPlaybackRead::~CstiAudioPlaybackRead


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::Initialize
//
// Description: Initializes the task
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on Success, otherwise estiERROR
//
stiHResult CstiAudioPlaybackRead::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::Initialize);

	stiDEBUG_TOOL ( g_stiAPReadDebug,
		stiTrace("CstiAudioPlaybackRead::Initialize\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_pSyncManager = CstiSyncManager::GetInstance();
	stiTESTCOND (nullptr != m_pSyncManager, stiRESULT_TASK_INIT_FAILED);

	// Create the empty packet pool with the buffered packets
	m_emptyPacketPool.create (nAUDIO_PACKETS_ALLOCATED);

	// Initialize the empty pool's audio buffers
	for (int i = 0; i < nAUDIO_PACKETS_ALLOCATED; i++)
	{
		// Allocate the buffers
		if (m_audioReadBuffer[i].empty())
		{
			m_audioReadBuffer[i].resize(nMAX_AUDIO_PLAYBACK_BUFFER_BYTES_SRA + 1);
		}

		m_astAudioStruct[i].pun8Buffer = &m_audioReadBuffer[i][0];
		m_astAudioStruct[i].unBufferMaxSize = nMAX_AUDIO_PLAYBACK_BUFFER_BYTES;

		// Add all structures to the packets in the pool.
		m_emptyPacketPool.bufferAdd (&m_audioReadBuffer[i][0], 0, &m_astAudioStruct[i]);

	} // end for

STI_BAIL:

	return (hResult);

} // end CstiAudioPlaybackRead::Initialize


/*!
 * \brief Read and process a packet from the RTP port
 */
void CstiAudioPlaybackRead::EventRtpSocketDataAvailable ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::EventRtpSocketDataAvailable);

	// Get an empty packet from the queue
	auto packet = m_emptyPacketPool.packetGet ();

	// Were we able to get a packet?
	if (!packet)
	{
		stiASSERT (estiFALSE);
	}
	else
	{
		// Read a packet from the RTP layer and place it into this packet.
		auto ePacketResult = PacketRead (packet);

#if 0
	static uint16_t un16RandNumber = 5000;
	if (((packet->RTPParamGet ())->sequenceNumber) % un16RandNumber == 0)
	{
	  srand(stiOSTickGet ());
	  un16RandNumber = (uint16_t) rand();
	  un16RandNumber = un16RandNumber % 5000;

	  stiTrace("APR:: dropping SQ# %d\n", (poPacket->RTPParamGet ())->sequenceNumber);
	  ePacketResult = ePACKET_CONTINUE;
	}
#endif

		// Was the packet read successful
		if (!m_bRtpHalted && ePACKET_OK == ePacketResult)
		{
			// Yes, We got a packet, so process it now.
			ReadPacketProcess (packet);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::PacketRead
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
EstiPacketResult CstiAudioPlaybackRead::PacketRead (
	const std::shared_ptr<CstiAudioPacket> &packet)	// Packet received from socket
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::PacketRead);

	EstiPacketResult eReadResult = ePACKET_CONTINUE;
	int nBytesRead;
	RvRtpParam * pstRTPParam = packet->RTPParamGet ();


	// Do we have a valid session handle?
	if (!m_bRtpHalted && m_rtpSession->rtpGet() && nullptr == m_rtpSession->rtpTransportGet())
	{
		if (m_pSyncManager->GetRemoteAudioMute ())
		{
			stiDEBUG_TOOL ( g_stiAPReadDebug,
				stiTrace ("APR:: task - ***** Remote Audio UnMute *****\n\n");
			);

			m_pSyncManager->SetRemoteAudioMute (estiFALSE);

			ReSyncJitterBuffer();
		}

		// Yes! Read the data from the socket. NOTE - the Radvision
		// documentation is incorrect. RvRtpRead does NOT return the number of
		// bytes read, it returns the result of rtpUnpack.  Was RvRtpRead
		// successful?
		if (0 <= m_rtpSession->RtpRead (
			packet->BufferGet (),
			nMAX_AUDIO_PLAYBACK_BUFFER_BYTES,
			pstRTPParam) &&
			pstRTPParam->len)
		{
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
				if (!m_bRtpHalted && nullptr != m_rtpSession->rtcpGet())
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
} // end CstiAudioPlaybackRead::PacketRead


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::ProcessDTMF
//
// Description: Process a single DTMF packet.  Skip any redundant packets.
//
// Returns:
//	EstiResult
//
EstiResult CstiAudioPlaybackRead::ProcessDTMF (
	const std::shared_ptr<CstiAudioPacket> &packet)
{
	uint8_t* buf = packet->RtpHeaderGet ();
	RvRtpParam* pParam = packet->RTPParamGet ();

	RvRtpDtmfEventParams dtmf;
	RvRtpDtmfEventUnpack (buf, pParam->len, pParam, &dtmf);

	// Check the timestamp to see if this is a redundant packet of a DTMF event already received
	//
	if (m_unDtmfTimestamp != pParam->timestamp)
	{
		// New DTMF character received...

		// Save the new timestamp,
		m_unDtmfTimestamp = pParam->timestamp;

		// Send the dTMF code up to the application
		dtmfReceivedSignal.Emit (static_cast<EstiDTMFDigit>(dtmf.event));
	}

	return estiOK;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::DataUnPack
//
// Description: Calls the RadVision rtpXXXUnpack function
//
// Abstract:
//	This function determines the type of audio and calls the appropriate rtp
//	unpack function.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiAudioPlaybackRead::DataUnPack (
	const std::shared_ptr<CstiAudioPacket> &packet,	// Buffer to be unpacked
	bool *pbIsAudioPacket)  // Flag whether this is audio data or not
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::DataUnPack);

	uint8_t* paun8AudioBuffer = packet->PayloadGet ();
	SsmdAudioFrame * pstAudioFrame = packet->m_frame;
	RvRtpParam* pstRTPParam = packet->RTPParamGet ();
	EstiResult eResult = estiOK;
	*pbIsAudioPacket = false;

#if 0
static uint32_t un32SQExpecting = 0;

if (0 == un32SQExpecting)
{
	un32SQExpecting = pstRTPParam->sequenceNumber;
}
else if (un32SQExpecting != pstRTPParam->sequenceNumber)
{
	stiTrace("APR:: ******* lost packet SQ# %d, next SQ# %d Empty = %d FULL = %d *******\n",
	un32SQExpecting,
	pstRTPParam->sequenceNumber,
	m_emptyPacketPool.count (),
	m_oFullQueue.CountGet ());

	un32SQExpecting =  pstRTPParam->sequenceNumber;
}

un32SQExpecting ++;
#endif

	//
	// Given the payload type, lookup the audio codec
	// that it maps to.
	//
	EstiAudioCodec eAudioCodec = estiAUDIO_NONE;
	TAudioPayloadMap::const_iterator i;

	i = m_PayloadMap.find (pstRTPParam->payload);

	if (i != m_PayloadMap.end ())
	{
		eAudioCodec = i->second.eCodec;
		// If need to support different packetization schemes for audio,
		// the scheme for this payload map is in i->second.ePacketizationScheme
	}

	// Unpack the data and perform any processing on the data stream header.
	switch (eAudioCodec)
	{
		case estiAUDIO_G711_ALAW:
			*pbIsAudioPacket = true;
			stiDEBUG_TOOL (g_stiAPReadDebug,
				stiTrace ("APR:: PCMA SQ# %d Empty = %d Full = %d size = %d\n",
				pstRTPParam->sequenceNumber, m_emptyPacketPool.count (),
				m_oFullQueue.CountGet (), pstRTPParam->len - pstRTPParam->sByte);
			); // stiDEBUG_TOOL

			if (0 > RvRtpPCMAUnpack (paun8AudioBuffer,
				nMAX_AUDIO_PLAYBACK_BUFFER_BYTES,
				pstRTPParam,
				nullptr))
			{
				// Error returned from Unpack function
				stiASSERT (estiFALSE);

				eResult = estiERROR;
			} // end if
			break;

		case estiAUDIO_G711_MULAW:
			*pbIsAudioPacket = true;
			stiDEBUG_TOOL (g_stiAPReadDebug,
				stiTrace ("APR:: PCMU SQ# %d Empty = %d Full = %d size = %d\n",
				pstRTPParam->sequenceNumber, m_emptyPacketPool.count (),
				m_oFullQueue.CountGet (), pstRTPParam->len - pstRTPParam->sByte);
			); // stiDEBUG_TOOL

			if (0 > RvRtpPCMUUnpack (paun8AudioBuffer,
				nMAX_AUDIO_PLAYBACK_BUFFER_BYTES,
				pstRTPParam,
				nullptr))
			{
				// Error returned from Unpack function
				stiASSERT (estiFALSE);
				eResult = estiERROR;
			} // end if
			break;
			
		case estiAUDIO_G722:
			*pbIsAudioPacket = true;
			stiDEBUG_TOOL (g_stiAPReadDebug,
				stiTrace ("APR:: G722 SQ# %d Empty = %d Full = %d size = %d\n",
				pstRTPParam->sequenceNumber, m_emptyPacketPool.count (),
				m_oFullQueue.CountGet (), pstRTPParam->len - pstRTPParam->sByte);
			); // stiDEBUG_TOOL

			if (0 > RvRtpG722Unpack (paun8AudioBuffer,
				nMAX_AUDIO_PLAYBACK_BUFFER_BYTES,
				pstRTPParam,
				nullptr))
			{
				// Error returned from Unpack function
				stiASSERT (estiFALSE);

				eResult = estiERROR;
			} // end if
			break;

		case estiAUDIO_DTMF:
			if (estiOK != ProcessDTMF (packet))
			{
				// Error returned from Unpack function
				stiASSERT (estiFALSE);
				eResult = estiERROR;
			}
			break;

		default:
			eResult = estiERROR;			
			if (pstRTPParam->len - pstRTPParam->sByte >= 1) // if there are no bytes, this is just a keepalive packet
			{
				stiTrace ("Payload type=%d\n", pstRTPParam->payload);
				stiASSERT (estiFALSE);
			}
			else
			{
				// stiTrace ("RTP keepalive (audio) received.\n");
			}

			break;
	} // end switch

	if (estiOK == eResult)
	{
		
		// Yes! Calculate the offset into the actual Audio data and store into
		// the audio frame structure for the driver.
		pstAudioFrame->pun8Buffer = packet->PayloadGet ();

		// Save the number of bytes of Audio we received into the structure.
		pstAudioFrame->unFrameSizeInBytes = pstRTPParam->len - pstRTPParam->sByte;
	} // end if

	return (eResult);

} // end CstiAudioPlaybackRead::DataUnPack


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::ReadPacketProcess
//
// Description: Process the packet that was read from the port
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on Success, othwise estiERROR
//
EstiResult CstiAudioPlaybackRead::ReadPacketProcess (
	const std::shared_ptr<CstiAudioPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::ReadPacketProcess);

	EstiResult eResult = estiOK;
	bool bIsAudioPacket = false;

	// Unpack the data into it's raw form
	eResult = DataUnPack (packet, &bIsAudioPacket);

	if (bIsAudioPacket)
	{
		//
		// Check the codec type to decide the jitter variables
		//
		if (m_bSetJitter && estiOK == eResult)
		{
			// Has the codec changed?
			switch ((packet->RTPParamGet ())->payload)
			{
				case RV_RTP_PAYLOAD_PCMA:
				case RV_RTP_PAYLOAD_PCMU:
				case RV_RTP_PAYLOAD_G722:
					m_bSetJitter = estiFALSE;
					m_nPacketsToHold = NumPacketAvailable ();
					m_nPacketsToHold = (m_nPacketsToHold < nAUDIO_PACKETS_BUFFERED) ?
						(nAUDIO_PACKETS_BUFFERED - m_nPacketsToHold) : nAUDIO_PACKETS_BUFFERED;
					m_nPacketsHeld = 0;
					break;

				default:
					stiASSERT (estiFALSE);
					eResult = estiERROR;
					break;
			}
		}

		if (estiOK == eResult)
		{
			// Is this the only empty packet that we have?
			if ((estiFALSE == m_packetDiscarding && m_emptyPacketPool.count () == 0) ||
			(estiTRUE == m_packetDiscarding && m_emptyPacketPool.count () < 3))
			{
				m_packetDiscarding = estiTRUE;

				// Yes! If this is the only empty packet, then it means the system is
				// saturated.
				// Since we are saturated, throw this packet away

				stiDEBUG_TOOL (g_stiAPReadDebug,
					if (0 == m_nDiscardedAudioPackets)
					{
						stiTrace ("APR:: Discarding audio packets SQ# %d...\n",
							(packet->RTPParamGet ())->sequenceNumber);
					} // end if
					m_nDiscardedAudioPackets++;
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

					stiDEBUG_TOOL (g_stiAPReadDebug,
						stiTrace ("APR:: Now holding %d audio packets SQ# %d...\n", m_nPacketsHeld,
						(packet->RTPParamGet ())->sequenceNumber);
					); // stiDEBUG_TOOLS
				} // end if
				else
				{
					// No! Do we have any packets that we have been holding?
					if (m_nPacketsHeld > 0)
					{
						stiDEBUG_TOOL (g_stiAPReadDebug,
							stiTrace ("APR:: Releasing %d audio packets...\n", m_nPacketsHeld);
						); // stiDEBUG_TOOLS

						// Yes! Let them go to the playback thread all at once.
						for (int i = 0; i < m_nPacketsHeld; i++)
						{
							dataAvailableSignal.Emit ();
						}
						m_nPacketsHeld = 0;
					} // end if
					else if (m_oFullQueue.CountGet() == 1)
					{
						// Let the read task know that there is a packet available
						dataAvailableSignal.Emit ();
					} // end else
				} // end else


				stiDEBUG_TOOL (g_stiAPReadDebug,
					if (m_nDiscardedAudioPackets > 0)
					{
						stiTrace (
							"APR:: Discarded Audio packets from %d to %d, empty = %d full = %d\n\n",
							(packet->RTPParamGet ())->sequenceNumber - m_nDiscardedAudioPackets,
							(packet->RTPParamGet ())->sequenceNumber - 1,
							m_emptyPacketPool.count (),
							m_oFullQueue.CountGet ());
						m_nDiscardedAudioPackets = 0;
					} // end if
				); // stiDEBUG_TOOLS

			} // end else

		}
	}

	return (eResult);
} // end CstiAudioPlaybackRead::ReadPacketProcess

//:-----------------------------------------------------------------------------
//:
//:	Public Methods
//:
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::ReSyncJitterBuffer
//
// Description: Indicates to hold some packets in the jitter buffer
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on Success, otherwise estiERROR
//
EstiResult CstiAudioPlaybackRead::ReSyncJitterBuffer ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::ReSyncJitterBuffer);

	EstiResult eResult = estiOK;
	m_bSetJitter = estiTRUE;

	return (eResult);
} // end CstiAudioPlaybackRead::AudioPacketEmptyPut


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
RvBool CstiAudioPlaybackRead::AudioEnumerator (
	RvRtcpSession hRTCP,	// Handle to RadVision RTCP Session
	RvUint32 ssrc,		// Synchronization Source ID
	void *pContext)
{
	RvBool bResult = RV_FALSE;

	RvRtcpINFO stRTCPInfo;
#ifndef stiNO_LIP_SYNC
	EstiBool bNTPSwapped = estiFALSE;
#endif
	auto pThis = (CstiAudioPlaybackRead *)pContext;

	// Get the RTCP information from the stack.

	RvRtcpGetSourceInfo (hRTCP, ssrc, &stRTCPInfo);
#if 0
	stiDEBUG_TOOL (g_stiRTCPDebug,
		stiTrace ("AP-hRTCP=0x%X, SSRC=0x%X, CNAME=%s\n\n", hRTCP, ssrc, stRTCPInfo.cname);
	); // stiDEBUG_TOOL
#endif
	if(stRTCPInfo.sr.valid && stRTCPInfo.sr.packets > 0)
	{

		pThis->m_nNumPackets = static_cast<int>(stRTCPInfo.sr.packets)- pThis->m_nNumPackets;

		if (pThis->m_nNumPackets > 0)
		{

			stiDEBUG_TOOL (g_stiRTCPDebug,
				stiTrace ("\nAudio SR: mNTPtimestamp = %lu, lNTPtimestamp = %lu, timestamp = %lu, packets = %u\n\n",
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
#ifndef stiNO_LIP_SYNC
#if 1
			if (g_bFirstAudioNTPTimestamp)
			{
				pThis->m_un32MostNTPTimestamp = stRTCPInfo.sr.mNTPtimestamp;
				pThis->m_un32LeastNTPTimestamp = stRTCPInfo.sr.lNTPtimestamp;
				g_bFirstAudioNTPTimestamp = estiFALSE;
				pThis->m_nNTPSolved = 0;
			}
			else if (pThis->m_nNTPSolved < 5)
			{
#if 0
				stiDEBUG_TOOL (g_stiRTCPDebug,
					stiTrace ("Audio SR: New mNTPtimestamp = %lu, lNTPtimestamp = %lu\n",
						stRTCPInfo.sr.mNTPtimestamp,
						stRTCPInfo.sr.lNTPtimestamp
					);

					stiTrace ("Audio SR: Old mNTPtimestamp = %lu, lNTPtimestamp = %lu\n",
						pThis->m_un32MostNTPTimestamp,	// fraction part
						pThis->m_un32LeastNTPTimestamp	// second part
					);
				);
#endif

				uint32_t un32NTP = 0;
				uint32_t un32NTPSwap = 0;

				// Assune the NTP is in the correct order
				if (stRTCPInfo.sr.mNTPtimestamp > pThis->m_un32MostNTPTimestamp + 1)
				{
					if (stRTCPInfo.sr.lNTPtimestamp >= pThis->m_un32LeastNTPTimestamp)
					{
						un32NTP =  (((stRTCPInfo.sr.mNTPtimestamp - pThis->m_un32MostNTPTimestamp) & 0x0000ffff) << 16)
						+ (((stRTCPInfo.sr.lNTPtimestamp - pThis->m_un32LeastNTPTimestamp) & 0xffff0000) >> 16);
					}
					else
					{
						auto  un32Diff = (uint32_t) (pThis->m_un32LeastNTPTimestamp - stRTCPInfo.sr.lNTPtimestamp);
						un32Diff = (uint32_t) 0xffffffff - un32Diff;
						un32NTP =  (((stRTCPInfo.sr.mNTPtimestamp - pThis->m_un32MostNTPTimestamp - 1) & 0x0000ffff) << 16)
						+ ((un32Diff & 0xffff0000) >> 16);
					}
				}

				// Assmue the NTP is in the swapped order
				if (stRTCPInfo.sr.lNTPtimestamp > pThis->m_un32LeastNTPTimestamp + 1)
				{
					if (stRTCPInfo.sr.mNTPtimestamp >= pThis->m_un32MostNTPTimestamp)

					{
						un32NTPSwap =  (((stRTCPInfo.sr.lNTPtimestamp - pThis->m_un32LeastNTPTimestamp) & 0x0000ffff) << 16)
						+ (((stRTCPInfo.sr.mNTPtimestamp - pThis->m_un32MostNTPTimestamp) & 0xffff0000) >> 16);
					}
					else
					{
						auto  un32Diff = (uint32_t) (pThis->m_un32MostNTPTimestamp - stRTCPInfo.sr.mNTPtimestamp);
						un32Diff = (uint32_t) 0xffffffff - un32Diff;
						un32NTPSwap =  (((stRTCPInfo.sr.lNTPtimestamp - pThis->m_un32LeastNTPTimestamp - 1) & 0x0000ffff) << 16)
						+ ((un32Diff & 0xffff0000) >> 16);
					}
				}

				stiDEBUG_TOOL (g_stiRTCPDebug,
					stiTrace("Audio SR: un32NTP = %lu un32NTPSwap = %lu\n",

					un32NTP, un32NTPSwap);
				);

				if (un32NTP == 0 || un32NTPSwap == 0)
				{
					if (un32NTP != 0)
					{
						bNTPSwapped = estiFALSE;
						pThis->m_nNTPSolved ++;
					}
					else if(un32NTPSwap != 0)
					{
						bNTPSwapped = estiTRUE;
						pThis->m_nNTPSolved ++;
					}
					else
					{
						pThis->m_nNTPSolved = 0;
					}
				}
				else
				{
					uint32_t un325Sec = (5 << 16);
					uint32_t un32NTPDiff = (un32NTP > un325Sec) ? (un32NTP - un325Sec) : (un325Sec - un32NTP);
					uint32_t un32NTPSwapDiff = (un32NTPSwap > un325Sec) ? (un32NTPSwap - un325Sec) : (un325Sec - un32NTPSwap);
#if 0
				stiDEBUG_TOOL (g_stiRTCPDebug,
					stiTrace("Audio SR: un32NTPDiff = %lu un32NTPSwapDiff = %lu\n",
					un32NTPDiff, un32NTPSwapDiff);
				);
#endif
		  if (un32NTPDiff >= un32NTPSwapDiff)
					{
						bNTPSwapped = estiTRUE;
						pThis->m_nNTPSolved ++;
					}
					else
					{
						bNTPSwapped = estiFALSE;
						pThis->m_nNTPSolved ++;
					}
				}

#if 1
				if (bNTPSwapped)
				{
					stiDEBUG_TOOL (g_stiRTCPDebug,
						stiTrace("Audio SR: the incomoing NTPTimestamp format is swapped\n");
					);
				}
				else
				{
					stiDEBUG_TOOL (g_stiRTCPDebug,
						stiTrace("Audio SR: the incomoing NTPTimestamp format is NOT swapped\n");
					);
				}
#endif

				pThis->m_un32MostNTPTimestamp = stRTCPInfo.sr.mNTPtimestamp;
				pThis->m_un32LeastNTPTimestamp = stRTCPInfo.sr.lNTPtimestamp;

				if (pThis->m_nNTPSolved > 0)
				{
					if (1 == pThis->m_nNTPSolved)
					{
						pThis->m_bIsNTPSwapped = bNTPSwapped;
					}
					else if (pThis->m_bIsNTPSwapped != bNTPSwapped)
					{
						pThis->m_nNTPSolved = 0;
						stiDEBUG_TOOL (g_stiRTCPDebug,
							stiTrace("Audio SR: NTPTimestamp format CHANGED !!!\n");
						);
// 						stiASSERT(estiFALSE);
					}
				}
			} // end else if (pThis->m_nNTPSolved < 5)


			if (pThis->m_nNTPSolved >= 5)
			{
				if (pThis->m_bIsNTPSwapped)
				{
					stiDEBUG_TOOL (g_stiRTCPDebug,

						stiTrace("Audio SR: NTPTimestamp swapped\n");
					);
					if (pThis)
					{
						pThis->m_pSyncManager->SetAudioSyncInfo(
							stRTCPInfo.sr.lNTPtimestamp,
							stRTCPInfo.sr.mNTPtimestamp,
							stRTCPInfo.sr.timestamp);
					}
				}
				else
				{
					stiDEBUG_TOOL (g_stiRTCPDebug,
						stiTrace("Audio SR: NTPTimestamp NOT swapped **************\n");
					);
					if (pThis)
					{
						pThis->m_pSyncManager->SetAudioSyncInfo(
							stRTCPInfo.sr.mNTPtimestamp,
							stRTCPInfo.sr.lNTPtimestamp,
							stRTCPInfo.sr.timestamp);
					}
				}
			}
#else
			if (pThis)
			{
				pThis->m_pSyncManager->SetAudioSyncInfo(
					stRTCPInfo.sr.lNTPtimestamp,
					stRTCPInfo.sr.mNTPtimestamp,
					stRTCPInfo.sr.timestamp);
			}
#endif
#endif // #ifndef stiNO_LIP_SYNC
		}
		pThis->m_nNumPackets = stRTCPInfo.sr.packets;
	}

	return (bResult);
} // end enumerator


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::AudioRTCPReportProcess
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
void CstiAudioPlaybackRead::AudioRTCPReportProcess (
	RvRtcpSession hRTCP,	// The RTCP session for which the data has arrived
	void* pContext,
	RvUint32 ssrc)
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::AudioRTCPReportProcess);

	// Collect the RTCP data from the stack (using VREnumerator function)
	RvRtcpEnumParticipants (
		hRTCP,
		AudioEnumerator,
		pContext);
} // end stiRTCPReportProcess


/*!
 * \brief Post a message that the channel is closing.
 *
 * This method posts a message to the Audio Playback task that the
 * channel is closing.
 *
 * \return estiOK when successful, otherwise estiERROR
 */
stiHResult CstiAudioPlaybackRead::DataChannelClose ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::DataChannelClose);
	stiDEBUG_TOOL (g_stiAPReadDebug,
		stiTrace ("CstiAudioPlaybackRead::DataChannelClose\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!m_dataChannelClosed)
	{
		MediaPlaybackRead::DataChannelClose();

		m_pSyncManager->SetRemoteAudioMute (estiTRUE);

		g_bFirstAudioNTPTimestamp = estiTRUE;

		// Clear the packet queues returning all packets to the empty pool.
		m_oRecvdQueue.clear ();
		m_oFullQueue.clear ();
	}

	return hResult;
}


/*!
 * \brief Process packets in the received queue
 */
void CstiAudioPlaybackRead::EventReceivedPacketsProcess()
{
	while (m_oRecvdQueue.CountGet () > 0)
	{
		auto packet = m_oRecvdQueue.OldestGet();

		if (!packet)
		{
			stiASSERTMSG (false, "CstiAudioPlaybackReady::EventReceivedPacketsProcess - packet was null\n");
			break;
		}

		m_oRecvdQueue.Remove (packet);

		if (!m_bRtpHalted)
		{
			if (nullptr != m_rtpSession->rtcpGet())
			{
				RvRtpParam *pRtpParam = packet->RTPParamGet ();

				// Yes! Notify the rtcp session that the packet was received
				// (use 90kHz clock frequency).
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
}


void CstiAudioPlaybackRead::RTPOnReadEvent(RvRtpSession hRTP)
{
	CstiAudioPlaybackRead *pThis = this;
	
	// NOTE: Avoid accessing class data members in the list (except for the packet queue)
	// to avoid the need for locking the task's mutex.  This method is called by Radvision through
	// another thread.
	
	EstiBool	bDone	= estiFALSE;
	uint32_t 	un32Cnt	= 0;
	while(!bDone)
	{
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
					nMAX_AUDIO_PLAYBACK_BUFFER_BYTES,
					packet->RTPParamGet ()) &&
				packet->RTPParamGet()->len)
			{
				//
				// Indicate that the payload is at the beginning of the buffer.
				//
				packet->RtpHeaderSet (packet->BufferGet ());

				packet->PayloadSet (packet->RtpHeaderGet () + packet->RTPParamGet ()->sByte);

				//
				// Cause the task thread to wake up if we went from empty
				// to non-empty.
				//
				if (1 == pThis->m_oRecvdQueue.AddAndCountGet (packet))	// This is the trigger for the data task to start again.
				{
					pThis->PostEvent (
						std::bind(&CstiAudioPlaybackRead::EventReceivedPacketsProcess, pThis));
				}

				un32Cnt++;

				// RvRtpGetAvailableBytes is not supported for the tunnel socket
				auto availableBytes = boost::make_optional (false, int());
				if (!m_rtpSession->tunneledGet ())
				{
					availableBytes = RvRtpGetAvailableBytes (m_rtpSession->rtpGet ());
				}

				if (un32Cnt > 10 || (availableBytes && *availableBytes <= 0))
				{
					bDone = estiTRUE;
				}
			}
			else
			{
				//
				// An error occured so just drop the packet.
				//
				bDone = estiTRUE;
			}
		}
		else
		{
			bDone = estiTRUE;
		}
	}
}

/*!
 * \brief specializations of initialization for audio
 *
 * \return stiRESULT_SUCCESS when successful, otherwise stiRESULT_ERROR
 */
stiHResult CstiAudioPlaybackRead::DataChannelInitialize (
	uint32_t un32InitialRtcpTime,
	const TAudioPayloadMap *pPayloadMap)
{
	stiLOG_ENTRY_NAME (CstiAudioPlaybackRead::DataChannelInitialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if (m_dataChannelClosed)
	{
		hResult = MediaPlaybackRead::DataChannelInitialize(un32InitialRtcpTime, pPayloadMap);

		// Register the RTCP report handler
		// TODO: could put this in the base class
		// but video handles this differently
		if (nullptr != m_rtpSession->rtcpGet())
		{
			RvRtcpSetRTCPRecvEventHandler(m_rtpSession->rtcpGet(), AudioRTCPReportProcess, this);
		}

		g_bFirstAudioNTPTimestamp = estiTRUE;

		ReSyncJitterBuffer();

		m_pSyncManager->SetRemoteAudioMute (estiTRUE);
	}

	Unlock ();

	return hResult;
}
