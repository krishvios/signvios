/*!
* \file CstiAudioRecord.cpp
* \brief The Audio Record task manages the flow of audio data from
*        the the audio input driver on through the sip stack.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

//
// Includes
//
#include "rtp.h"
#include "rtcp.h"
#include "payload.h"
#include "rvrtpnatfw.h"

#include "CstiAudioRecord.h"
#include "stiTaskInfo.h"
#ifndef stiDISABLE_PROPERTY_MANAGER
	#include "PropertyManager.h"
	#include "cmPropertyNameDef.h"
#endif

//
// Constants
//
#define DTMF_EVENT_DURATION 80
#define DTMF_TIMER_INTERVAL 15
#define DTMF_END_PACKET_TIMER_INTERVAL 5
#define DTMF_SPACING_DURATION 50
#define DTMF_VOLUME 10


/*!
 * \brief Constructor
 */
CstiAudioRecord::CstiAudioRecord (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t maxRate,
	ECodec codec,
	RvSipMidMgrHandle sipMidMgr)
:
	vpe::MediaRecord<EstiAudioCodec, SstiAudioRecordSettings>( // TODO: can't use MediaRecordType??
		stiAR_TASK_NAME,
		session,
		0,
		maxRate,
		codec,
		estiAUDIO_NONE,
		0,
		0
		),
    m_dtmfTimer (0, this),
    m_delaySendTimer (0, this),
	m_audioInputPacketReady (false)
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::CstiAudioRecord);

	m_currentAudioBuffer.resize(nMAX_AUDIO_RECORD_BUFFER_BYTES + 1);

	if (sipMidMgr)
	{
		// Just before the event loop starts, notify Radvision we will we using
		// the event loop's thread to communicate with its SIP APIs
		m_signalConnections.push_back (startedSignal.Connect (
				[this, sipMidMgr]{ RvSipMidAttachThread (sipMidMgr, m_taskName.c_str()); }));

		// Just after the event loop stops, notify Radvision we are done using
		// the event loop's thread to communicate with its SIP APIs
		m_signalConnections.push_back (shutdownSignal.Connect (
				[sipMidMgr]{ RvSipMidDetachThread (sipMidMgr); }));
	}

	m_signalConnections.push_back (m_dtmfTimer.timeoutSignal.Connect (
			std::bind(&CstiAudioRecord::EventDtmfTimerTimeout, this)));

	m_signalConnections.push_back (m_delaySendTimer.timeoutSignal.Connect (
			std::bind(&CstiAudioRecord::EventDelaySendTimerTimeout, this)));
}

///////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::~CstiAudioRecord
//
//  Description: Class destructor
//
//  Abstract:
//		This is the destructor for the Audio Record class
//
//  Returns:
//
CstiAudioRecord::~CstiAudioRecord ()
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::~CstiAudioRecord);

	Shutdown ();
}


stiHResult CstiAudioRecord::AudioInputDeviceSet(
	const std::shared_ptr<IstiAudioInput> &audioInput)
{
	PostEvent (
	    std::bind(&CstiAudioRecord::EventAudioInputDeviceSet, this, audioInput));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Sets the backend audio input instance we get data from
 * \param Pointer to audioInput instance
 */
void CstiAudioRecord::EventAudioInputDeviceSet (
	std::shared_ptr<IstiAudioInput> audioInput)
{
	// Do nothing if we already have this audio input instance
	if (m_pAudioInput == audioInput)
	{
		return;
	}

	// Halt the current data flow
	if (m_bChannelOpen && m_pAudioInput)
	{
		DisconnectSignals ();
		m_pAudioInput->AudioRecordStop ();
	}

	// Select the new device
	m_pAudioInput = audioInput;
	stiASSERT (m_pAudioInput);

	// Start the new data flowing
	if (m_bChannelOpen)
	{
		ConnectSignals ();
		m_pAudioInput->AudioRecordStart ();
	}

	// Update privacy state to the privacy state of the new input device
	{
		EstiBool bPrivacy;
		m_pAudioInput->PrivacyGet (&bPrivacy);
		if (bPrivacy)
		{
			Mute (estiMUTE_REASON_PRIVACY);
		}
		else
		{
			Unmute (estiMUTE_REASON_PRIVACY);
		}
	}
}



////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::Initialize
//
// Description: First stage of initialization of the audio record task.
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiAudioRecord::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Open audio device driver
	//
	if (!m_pAudioInput)
	{
		m_pAudioInput = std::shared_ptr<IstiAudioInput>(IstiAudioInput::InstanceGet(), [](IstiAudioInput*){});
	}

	//
	// Call the base class intitialization function
	//
	hResult = MediaRecordType::Initialize ();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);

}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::Startup
//
// Description: Start the audio record task.
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
void CstiAudioRecord::Startup ()
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::Startup);

	// Now initialize the "current" audio packet structure
	m_oCurrentAudioPacket.BufferSet (&m_currentAudioBuffer[0]);

	m_oCurrentAudioPacket.m_frame = (SsmdAudioFrame*) &m_stCurrentAudioFrame;

	MediaRecordType::Startup();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::DataChannelClose
//
// Description: Post a message that the channel is closing.
//
// Abstract:

//	This method posts a message to the Audio Record task that the channel is
//	closing.
//
// Returns:
//	estiOK when successfull, otherwise estiERROR
//
void CstiAudioRecord::DataChannelClose ()
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::DataChannelClose);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if(m_bChannelOpen)
	{
		DisconnectSignals ();

		// NOTE: must not call this before checking if channel is
		// open since this base function marks channel as closed
		MediaRecordType::DataChannelClose();

		// Tell the audio driver to stop audio playback
		stiHResult hResult = m_pAudioInput->AudioRecordStop ();
		stiASSERT(!stiIS_ERROR(hResult));
	}

	m_dtmfTimer.stop ();
	m_delaySendTimer.stop ();

	m_DtmfEvents.clear ();
	
	m_nRemainderSize = 0;
	if (m_pRemainderBuffer)
	{
		delete[] m_pRemainderBuffer;
		m_pRemainderBuffer = nullptr;
	}
} // end CstiAudioRecord::DataChannelClose


/*!
 * \brief Initializes the data channel
 */
stiHResult CstiAudioRecord::DataChannelInitialize ()
{
	stiHResult hResult = MediaRecordType::DataChannelInitialize();

	//
	// Initialize audio record driver
	//
	SetDeviceAudioCodec(m_Settings.codec);

	ConnectSignals ();

	EstiBool bPrivacy;

	m_pAudioInput->PrivacyGet (&bPrivacy);

	if (bPrivacy)
	{
		Mute (estiMUTE_REASON_PRIVACY);
	}

	// Tell the audio driver that the audio record is ready to start
	hResult = m_pAudioInput->AudioRecordStart();

	return hResult;
}


/*!
 * \brief Helper method to DataChannelInitialize to connect to audio input signals
 */
void CstiAudioRecord::ConnectSignals ()
{
	stiASSERTMSG (m_pAudioInput, "CstiAudioRecord::ConnectSignals - audioInput instance was null\n");

	if (!m_audioInputPacketReadyStateSignalConnection)
	{
		m_audioInputPacketReadyStateSignalConnection = m_pAudioInput->packetReadyStateSignalGet ().Connect (
			[this](bool ready){
				if (m_audioInputPacketReady != ready)
				{
					m_audioInputPacketReady = ready;
					if (ready)
					{
						PostEvent (
							std::bind(&CstiAudioRecord::EventPacketsProcess, this));
					}
				}
			});
	}

	if (!m_audioPrivacySignalConnection)
	{
		m_audioPrivacySignalConnection = m_pAudioInput->audioPrivacySignalGet ().Connect (
			[this](bool enabled){
				if (enabled)
				{
					Mute (estiMUTE_REASON_PRIVACY);
				}
				else
				{
					Unmute (estiMUTE_REASON_PRIVACY);
				}
			});
	}
}

/*!
 * \brief Helper method to DataChannelClose to disconnect from audio input signals
 */
void CstiAudioRecord::DisconnectSignals ()
{
	stiASSERTMSG (m_pAudioInput, "CstiAudioRecord::DisconnectSignals - audioInput instance was null\n");

	m_audioInputPacketReadyStateSignalConnection.Disconnect ();

	m_audioPrivacySignalConnection.Disconnect ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::DtmfToneSend
//
// Description: A dtmf event has come in.
//
// Returns:
//	hResult
//
void CstiAudioRecord::DtmfToneSend (EstiDTMFDigit eDTMFDigit)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_Settings.dtmfEventPayloadNbr > -1)
	{
		PostEvent (
			std::bind(&CstiAudioRecord::EventDtmfToneSend, this, eDTMFDigit));
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::DtmfSupportedGet
//
// Description: Does the remote endpoint support DTMF
//
// Returns:
//	EstiBool: True if DTMF is supported
//
EstiBool CstiAudioRecord::DtmfSupportedGet () const
{
	EstiBool bResult = estiFALSE;
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_Settings.dtmfEventPayloadNbr > -1)
	{
		bResult = estiTRUE;
	}
	
	return bResult;
}


/*!
 * \brief Handler for when a A DTMF event has come in
 * \param digit
 */
void CstiAudioRecord::EventDtmfToneSend (EstiDTMFDigit digit)
{
	DtmfEvent event;
	event.current = 0;
	event.dtmfDigit = digit;
	event.duration = DTMF_EVENT_DURATION;
	event.lastPacketCount = 3;

	m_DtmfEvents.push_back (event);
	
	if (!m_dtmfInProgress)
	{
		m_dtmfInProgress = true;
		m_dtmfTimer.timeoutSet (DTMF_TIMER_INTERVAL);
		m_dtmfTimer.restart ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::DtmfToneSendInternal
//
// Description: This is essentially the method used in the RadVision sample code
//	To send RFC 2833 DTMF packets.
//
// Arguments:
//	newEvent - the DTMF event to send.
//	duration - the DTMF duration in milliseconds.
//	bFirstPacket- indicate if this is the first dtmf rtp packet.
//	bEndPackets - indicate if this is the last dtmf rtp packet
//
// Returns:
//	EstiTRUE on Success
//
EstiBool CstiAudioRecord::DtmfToneSendInternal (
	EstiDTMFDigit newEvent, 
	uint32_t duration, 
	EstiBool bFirstPacket,
	EstiBool bEndPackets)
{
	RvRtpParam rtpParam;
	uint8_t buffer[30];
	RvRtpDtmfEventParams dtmf;
	RvStatus rvStatus{RV_OK};

	if (bFirstPacket == estiTRUE)
	{
		// this is the first rtp packet for this dtmf event.
		// initialize the rtp packet -
		// timestamp and sSrc parameters of this dtmf event will remain the same for all
		// rtp packets.
		// sequenceNumber - will be updated for every rtp packet, except the last
		// 3 retransmissions
		m_dtmfTimeStamp = RvSipMidTimeInMilliGet ();
		m_dtmfSSRC = RvRtpGetSSRC (m_session->rtpGet());
	}

	RvRtpParamConstruct(&rtpParam);
	rtpParam.timestamp = m_dtmfTimeStamp;
	rtpParam.sSrc = m_dtmfSSRC;

	// debugging
	memset (buffer, 0, sizeof (buffer));

	// set the appropriate parameters for DTMF header
	dtmf.event = (RvRtpDtmfEvent)newEvent;
	dtmf.end = bEndPackets;
	dtmf.volume = DTMF_VOLUME;
	dtmf.duration = duration << 3; // multiplying by 8 - to get to the timestamp units - 0.125ms

	// set the appropriate parameters for RTP header
	rtpParam.payload = (RvUint8)m_Settings.dtmfEventPayloadNbr; // the dynamic payload type as was agreed in the sdp negotiation
	rtpParam.marker = bFirstPacket; // the first packet of a dtmf event should be marked (marker = 1). other packets will not be marked

	// NOTE what about sip header additions?

	//There are additional parameters that we do not use that I'm setting here.
	rtpParam.nCSRC = 1; //Number of sources we only have 1
	
	rtpParam.sByte = rtpParam.len = RvRtpDtmfEventsGetHeaderLength (1, &rtpParam);

	// build the DTMF-event format header, for RTP packet
	RvRtpDtmfEventPack (buffer, 0, &rtpParam, &dtmf);

	// build the RTP header, and send the RTP packet
	rvStatus = RvRtpWrite (m_session->rtpGet (), buffer, rtpParam.len, &rtpParam);
	if (rvStatus < 0)
	{
		stiTrace ("CstiAudioRecord::DtmfToneSendInternal RvRtpWrite Failed.\n");
		return estiFALSE;
	}

	return estiTRUE;
}


//:-----------------------------------------------------------------------------
//:
//:	Message handlers
//:
//:-----------------------------------------------------------------------------


/*!
 * \brief Event handler for when the dtmf timer fires
 */
void CstiAudioRecord::EventDtmfTimerTimeout ()
{
	m_dtmfInProgress = false;

	// Do nothing if there are no DTMF events
	if (m_DtmfEvents.empty())
	{
		return;
	}

	DtmfEvent &event = m_DtmfEvents.front ();
	if (event.current < event.duration || event.lastPacketCount > 0)
	{
		if (event.current < event.duration)
		{
			event.current += DTMF_TIMER_INTERVAL;
		}
		else
		{
			--event.lastPacketCount;
		}

		DtmfToneSendInternal (
			event.dtmfDigit,
			event.current,	// This is used to calculate "Duration" in the DTMF packet
							// which should never be 0, unless it is a state event.
			(event.current == DTMF_TIMER_INTERVAL) ? estiTRUE : estiFALSE,
			(event.current >= event.duration) ? estiTRUE : estiFALSE);

		if (event.current < event.duration || event.lastPacketCount > 0)
		{
			// start the timer again
			auto timeoutMs =
				(event.current < event.duration) ?
					DTMF_TIMER_INTERVAL :
					DTMF_END_PACKET_TIMER_INTERVAL;

			m_dtmfInProgress = true;
			m_dtmfTimer.timeoutSet (timeoutMs);
			m_dtmfTimer.restart ();
		}
		else
		{
			// we finished sending so remove it from the queue
			m_DtmfEvents.erase (m_DtmfEvents.begin ());

			// still want the timer to go off if there are more events remaining to process
			if (!m_DtmfEvents.empty ())
			{
				m_dtmfInProgress = true;
				m_dtmfTimer.timeoutSet (DTMF_SPACING_DURATION);
				m_dtmfTimer.restart ();
			}
		}
	}
}


/*!
 * \brief Event handler for when the dtmf timer fires
 */
void CstiAudioRecord::EventDelaySendTimerTimeout ()
{
	PacketSend ();

	// Process remaining packets
	EventPacketsProcess ();
}


/*!
 * \brief Event handler to commence audio data flow
 */
void CstiAudioRecord::EventDataStart ()
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::EventDataStart);
	stiDEBUG_TOOL ( g_stiARDebug,
		stiTrace("CstiAudioRecord::EventDataStart\n");
	);

	// Is the audio mute off? It must be for us to turn on the audio.
	if (0 == m_nMuted)
	{
		// Indicate that we have started sending data
		m_bSendingMedia = estiTRUE;

		// Initially, there are old data in buffers which need to be flushed out
		m_nFlushPackets = nUNMUTE_FLUSH_AR_BUFFERS;

	} // end if
}


/*! \brief Do we need to delay sending the next packet to deliver at the proper rate?  If
 *	so, set the timer herein.
 *
 *  \retval true if a delay is needed, false otherwise.
 */
bool CstiAudioRecord::DelayTimerNeeded (uint32_t rtpTimeStamp)
{
	bool bTimerSet = false;
	
	// If we do not have an rtpTimeStamp being supplied from the platform
	// we do not want to set a delay. Fixes bug #25200.
	if (rtpTimeStamp)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		stiUNUSED_ARG (hResult); // Prevent compiler warning
		
		// How long should the delay be before sending this anything from the packet being processed?
		timeval TimeNow{};
		double dTimeNow;
		double dDelta;
		int nRet = gettimeofday (&TimeNow, nullptr);
		stiTESTCOND (nRet == 0, stiRESULT_ERROR);
		
		dTimeNow = TimeNow.tv_sec + (TimeNow.tv_usec / 1000000.0);
		dDelta = m_dNextSendTime - dTimeNow;
		
		// Our delta is now set based on the last time sent and a given time between packets.
		// I don't believe we should add time for the missing packets since that time will have already
		// been added as a result of the loss.
		
		// Now, has it been long enough since the last packet that we can send another?
		if (dDelta > 0.0)
		{
			bTimerSet = true;
			
			auto  nDelayMS = static_cast<int>(dDelta * 1000);
			
			// Start a timer to expire when it is time to send this packet.
			m_delaySendTimer.timeoutSet (nDelayMS);
			m_delaySendTimer.restart ();
		}
	}

STI_BAIL:

	return bTimerSet;
}


/*!
 * \brief Processes packets from driver, up to as many as are available
 *        If the send delay timer is active, it will not process anything
 */
void CstiAudioRecord::EventPacketsProcess ()
{
	// Do nothing if there are no packets available, or the delay send timer
	// is active.  This means that we need to slow down to achieve a certain rate.
	// Fixed bug #25893, changed from "while" to "if" statement and added code to
	// post an event to read more data if the delay timer is not set.
	if (m_audioInputPacketReady && !m_delaySendTimer.isActive())
	{
		PacketProcessFromDriver ();
		
		// If audio packets are ready and the delay timer is not active post
		// an event to read additional packets.
		if (m_audioInputPacketReady && !m_delaySendTimer.isActive())
		{
			PostEvent (std::bind(&CstiAudioRecord::EventPacketsProcess, this));
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::PacketProcessFromDriver
//
// Description: Read and process a packet from the audio driver
//
// Abstract:
//
// Returns:
//	estiOK if successful, otherwise estiERROR.
//
EstiResult CstiAudioRecord::PacketProcessFromDriver ()
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::PacketProcessFromDriver);
	EstiResult eResult = estiOK;

	if (m_bChannelOpen)
	{
		EstiPacketResult ePacketResult = ePACKET_OK;

		// Read audio frame from driver
		ePacketResult = ReadPacket(&m_oCurrentAudioPacket);

		// Check for error before continuing...
		if (ePACKET_OK == ePacketResult)
		{
			uint16_t nPacketByteCount = 0;
			
			SsmdAudioFrame *pstAudioFrame = m_oCurrentAudioPacket.m_frame;
			nPacketByteCount = pstAudioFrame->unFrameSizeInBytes;
			m_nCurrentOffset = 0;

// Only Do Delay and audio silence for endpoints that are supporting timestamps for audio packets from the driver.
#ifndef NO_AUDIO_TIMESTAMP_SUPPORT
			// If there is a timestamp delivered, we need to take some extra steps to:
			// - See if we need to account for lost packets
			// - Properly space the packets being sent
			if (pstAudioFrame->un32RtpTimestamp)
			{
				stiASSERT (m_un32LastTimestampRecvdFromAudioInput < pstAudioFrame->un32RtpTimestamp);

				// Have we experienced any packet loss?  Note that timestamp is incremented
				// by the byte count.  So if we are sending 20ms audio samples, the timestamp
				// will increment by 160 (20 x 8).
				if (0 != m_un32LastTimestampRecvdFromAudioInput &&

					(m_un32LastTimestampRecvdFromAudioInput + nPacketByteCount) <
					pstAudioFrame->un32RtpTimestamp)
				{
					// Looks like we missed some packets.

					// How much skipped data is there?
					uint32_t un32TimeDelta = pstAudioFrame->un32RtpTimestamp -
							m_un32LastTimestampRecvdFromAudioInput -
							m_nTimestampIncrement;

					// If we have a remainder sitting around; drop it
					m_nRemainderSize = 0;

					// Update the timestamp to account for the missed packet(s)

					// Increment the send time based on the number of samples/frame
					// The delta should be an even increment of m_nTimestampIncrement
					// If it isn't, prepend silence
					int nPrependBytes = un32TimeDelta % m_nTimestampIncrement;
					if (nPrependBytes && m_pRemainderBuffer)
					{
						memset (&m_pRemainderBuffer[m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ()],
								0xff, nPrependBytes * sizeof (uint8_t));
						m_nRemainderSize += nPrependBytes;
					}

					// Since we are prepending silence frames, reduce the time delta by the number
					// of prepended bytes
					un32TimeDelta -= nPrependBytes;

					// How long of a delay should there be since the last send to accommodate the loss?
					// This is measured as increments of the timestamp increment
					m_un32RTPInitTime += (un32TimeDelta);
				}

				// Store the received timestamp as the most recent one received
				m_un32LastTimestampRecvdFromAudioInput = pstAudioFrame->un32RtpTimestamp;
			}

			// If we are still to send the packet now, do it.
			if (!DelayTimerNeeded (pstAudioFrame->un32RtpTimestamp))
#endif
			{
				PacketSend ();
			}
		}
	}

	return eResult;
}


/*! \brief Loop through the data in the current audio packet buffer sending it in the
 *		   proper packet sizes.
 *
 *  \retval an stiHResult; stiRESULT_SUCCESS if successful
 */
stiHResult CstiAudioRecord::PacketSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;


	uint16_t nPacketByteCount = 0;
	uint8_t *pszDataBuffer = m_oCurrentAudioPacket.BufferGet ();

	SsmdAudioFrame *pstAudioFrame = m_oCurrentAudioPacket.m_frame;
	nPacketByteCount = pstAudioFrame->unFrameSizeInBytes - m_nCurrentOffset;
	RvRtpParam *pRTPParam = m_oCurrentAudioPacket.RTPParamGet();

	if (m_nRemainderSize > 0 && m_nRemainderSize + nPacketByteCount >= m_nTimestampIncrement)
	{
		// finish up what's in the remainder buffer and process it
		uint16_t nBytesNeededToCompletePacket = m_nTimestampIncrement - m_nRemainderSize;

		stiTESTCONDMSG (m_pRemainderBuffer, stiRESULT_ERROR, "ERR: No remainder buffer.  Data lost!");

		memcpy (&m_pRemainderBuffer[m_RTPHeaderOffset.RtpHeaderOffsetLengthGet () +
				m_nRemainderSize],
				&pszDataBuffer[m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ()],
				nBytesNeededToCompletePacket);

		EstiResult ePacketSendResult = PacketSend (pRTPParam, m_pRemainderBuffer);
		stiTESTCOND (estiOK == ePacketSendResult, stiRESULT_ERROR);

		nPacketByteCount -= nBytesNeededToCompletePacket;
		m_nCurrentOffset += nBytesNeededToCompletePacket;
		m_nRemainderSize = 0;
	}

	// If we are still to send the packet now, do it.
	if (nPacketByteCount >= m_nTimestampIncrement && !DelayTimerNeeded (pstAudioFrame->un32RtpTimestamp))
	{
		EstiResult ePacketSendResult = PacketSend (pRTPParam, &pszDataBuffer[m_nCurrentOffset]);
		stiTESTCOND (estiOK == ePacketSendResult, stiRESULT_ERROR);

		nPacketByteCount -= m_nTimestampIncrement;
		m_nCurrentOffset += m_nTimestampIncrement;
	}

	// If we have less than a valid packet size of data left in the audio buffer,
	// move it to the remainder buffer so that we can get the next audio buffer.
	if (nPacketByteCount > 0 && nPacketByteCount < m_nTimestampIncrement)
	{
		// Append whatever's left to the remainder buffer
		stiTESTCONDMSG (m_pRemainderBuffer, stiRESULT_ERROR, "ERR: No remainder buffer.  Data lost!");

		memcpy (&m_pRemainderBuffer[m_RTPHeaderOffset.RtpHeaderOffsetLengthGet () + m_nRemainderSize],
				&pszDataBuffer[m_RTPHeaderOffset.RtpHeaderOffsetLengthGet () + m_nCurrentOffset],
				nPacketByteCount);
		m_nRemainderSize += nPacketByteCount;
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::PacketSend
//
// Description: Send a single packet's worth of data
//
// Abstract:
//
// Returns:
//	estiOK if successful, otherwise estiERROR.
//
EstiResult CstiAudioRecord::PacketSend (RvRtpParam *pRTPParam, const uint8_t *pszDataBuffer)
{
	EstiResult eResult = estiOK;
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32NumBytesToSend = 0;
	uint32_t un32PayloadByte = 0;
	int nRTPRet = 0;
	timeval CurrentTime{};

	// initialize the rtp param structure
	RvRtpParamConstruct (pRTPParam);
	
	// What is the current time
	int nRet = gettimeofday (&CurrentTime, nullptr);
	stiTESTCOND (nRet == 0, stiRESULT_ERROR);

	if (m_bSendingMedia && 0 == m_nFlushPackets && m_nMuted == 0)
	{
		// Offset of the first payload byte in the rtp packet buffer
		pRTPParam->sByte = m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();

		// Set timestamp parameter
		pRTPParam->timestamp = m_un32RTPInitTime;

		// Set marker flag. - This parameter is used to indicate the first
		// frame following silence.
		pRTPParam->marker = 0;

#if 0
		//
		// TYY: Following rtp parameters are not applicable for RvRtpWrite()
		//

		// Sync Source flag - This param is obtained in RvRtpWrite() from the rtpSession.
		pRTPParam->sSrc = 0;

		// Sequence number - the stack generates the sequence number
		pRTPParam->sequenceNumber = 0;

		// The number of payload bytes in the rtp packet -
		// The parameter of len of RvRtpWrite() and not this parameter,
		// determines the length of the RTP packet
		pRTPParam->len = 0;

		// Payload type - This parameter is set by the rtpXXXPack function.
		pRTPParam->payload = 0;
#endif

		{

			// Calculate the total bytes to send for a RTP packet
			un32PayloadByte = m_nTimestampIncrement;
			un32NumBytesToSend = m_RTPHeaderOffset.RtpHeaderOffsetLengthGet () + un32PayloadByte;
		}

		if (un32NumBytesToSend > 0)
		{
			switch (m_currentCodec)
			{
				case estiAUDIO_G711_ALAW:

					// Call the rtp function to place the RTP header at the
					// start of the data.
					nRTPRet = RvRtpPCMAPack (
						(void *) pszDataBuffer,
						un32NumBytesToSend,
						pRTPParam,
						(void *) nullptr);

					if (nRTPRet < 0)
					{
						stiASSERT (estiFALSE);
						eResult = estiERROR;
					} // end if

					break;

				case estiAUDIO_G711_MULAW:

					// Call the rtp function to place the RTP header at the
					// start of the data.
					nRTPRet = RvRtpPCMUPack (
						(void *) pszDataBuffer,
						un32NumBytesToSend,
						pRTPParam,
						(void *) nullptr);

					if (nRTPRet < 0)
					{
						stiASSERT (estiFALSE);
						eResult = estiERROR;
					} // end if

					break;

				case estiAUDIO_G722:

					// Call the rtp function to place the RTP header at the
					// start of the data.
					nRTPRet = RvRtpG722Pack (
						(void *) pszDataBuffer,
						un32NumBytesToSend,
						pRTPParam,
						(void *) nullptr);

					if (nRTPRet < 0)
					{
						stiASSERT (estiFALSE);
						eResult = estiERROR;
					} // end if

					break;

				default:
					stiASSERT (estiFALSE);
					eResult = estiERROR;
					break;
			} // end of switch

			// Keep track of how many (payload) bytes will be sent
			ByteCountAdd (un32PayloadByte);
			PacketCountAdd ();

			// If we have an RTP session, send the packet
			if (nullptr != m_session->rtpGet())
			{

				// Send the data to the stack
				nRTPRet = RvRtpWrite (m_session->rtpGet(),
					(void *) pszDataBuffer,
					un32NumBytesToSend,
					pRTPParam);

				if (nRTPRet < 0)
				{
					stiDEBUG_TOOL (g_stiARDebug,
						stiTrace ("CstiAudioRecord: RvRtpWrite error!\n");

					); // stiDEBUG_TOOL

					stiASSERT (estiFALSE);
					eResult = estiERROR;

				} // end if
				else
				{

					stiDEBUG_TOOL (g_stiARDebug,
						static int nCnt = 0;
						if(++nCnt == 1)
						{
							stiTrace ("CstiAudioRecord:: Write audio SQ# %d timestamp = %d size = %d successful!!\n\n",
								pRTPParam->sequenceNumber, pRTPParam->timestamp, un32PayloadByte);
							nCnt = 0;
						}
					); // stiDEBUG_TOOL
				} // end else

			} // end if
			else
			{
				stiDEBUG_TOOL (g_stiARDebug,
					stiTrace ("CstiAudioRecord: No hRTPSession handle\n");
				); // stiDEBUG_TOOL

				//stiASSERT (estiFALSE);
				//eResult = estiERROR;
			} // end else
		} // end if if(un32NumBytesToSend > 0)
	} // end if (m_bSendingData)

	// Updating the number of packets need to be flushed
	if(m_nFlushPackets > 0)
	{
		m_nFlushPackets --;
	}
	else
	{
		// Increment the BaseSendTime based on the number of samples/frame
		m_un32RTPInitTime += m_nTimestampIncrement;
	}

	// Calculate the next time we can send another packet
	int nSampleSize;
	nSampleSize = G711_SIZE_TO_MS (m_nTimestampIncrement);
	m_dNextSendTime = CurrentTime.tv_sec + (CurrentTime.tv_usec / 1000000.0) + ((double)nSampleSize / 1000.0);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		eResult = estiERROR;
	}

	return eResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::ReadPacket
//
// Description: Read a packet from the audio driver, and prepare the RTP
//
// Abstract:
//
// Returns:
//	ePACKET_OK - read successful
//	ePACKET_ERROR - read failed
//	ePACKET_CONTINUE - no data available to read
//
EstiPacketResult CstiAudioRecord::ReadPacket(CstiAudioPacket *poPacket)

{
	stiLOG_ENTRY_NAME (CstiAudioRecord::PacketRead);

	EstiPacketResult ePacketResult = ePACKET_OK;
	SsmdAudioFrame * pstAudioFrame = poPacket->m_frame;

	pstAudioFrame->un32RtpTimestamp = 0;  // Initialize to 0; it will be set by the driver if needed.

	// Offset into the buffer to the start of the actual data
	pstAudioFrame->pun8Buffer = poPacket->BufferGet () + m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();

	// NOTE: The buffer is allocated to the size of nMAX_AUDIO_RECORD_BUFFER_BYTES,
	// but the other space are left for rtp header and SRA data
	pstAudioFrame->unBufferMaxSize = nMAX_AUDIO_RECORD_BYTES;


	pstAudioFrame->unFrameSizeInBytes = 0;

	// Retrieve an audio packet from the audio driver.
	stiHResult hResult = m_pAudioInput->AudioRecordPacketGet(pstAudioFrame);

	// Did we get a frame?
	if (!stiIS_ERROR (hResult))
	{
		if(pstAudioFrame->unFrameSizeInBytes <= 0)
		{
			// No! We failed to read any valid data.
			ePacketResult = ePACKET_CONTINUE;
		}
	}
	else
	{
		// No! We failed to read any valid data.
		ePacketResult = ePACKET_CONTINUE;
	}

	return (ePacketResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioRecord::SetDeviceAudioCodec
//
// Description: Changes the audio mode in the driver
//
// Abstract:
//
//	This function set the audio codec type, and maximum packet size

//  into audio driver
//
// Returns:
//
EstiResult CstiAudioRecord::SetDeviceAudioCodec (
	IN EstiAudioCodec eMode)
{
	stiLOG_ENTRY_NAME (CstiAudioRecord::InitializeAudioDriver);
	stiDEBUG_TOOL ( g_stiARDebug,
		stiTrace("CstiAudioRecord::SetDeviceAudioCodec eMode = %d\n", eMode);
	);

	EstiResult eResult = estiOK;
	EstiAudioCodec esmdAudioCodec = estiAUDIO_NONE;
	unsigned short usSamplesPerPacket = 0;
	RvRtpParam rtpParam;

	RvRtpParamConstruct (&rtpParam);

	switch (eMode)
	{
		case estiAUDIO_G711_ALAW:
		case estiAUDIO_G711_ALAW_R:
			esmdAudioCodec = estiAUDIO_G711_ALAW;
			m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpPCMAGetHeaderLengthEx (&rtpParam));
			break;

		case estiAUDIO_G711_MULAW:
		case estiAUDIO_G711_MULAW_R:
			esmdAudioCodec = estiAUDIO_G711_MULAW;
			m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpPCMUGetHeaderLengthEx (&rtpParam));
			break;

		case estiAUDIO_G722:
			esmdAudioCodec = estiAUDIO_G722;
			m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpG722GetHeaderLengthEx (&rtpParam));
			break;

		default:

			//esmdAudioCodec = estiAUDIO_G711_ALAW;
			//m_RTPHeaderOffset.RTPHeaderLengthSet (RvRtpPCMAGetHeaderLength ());

			stiASSERT (estiFALSE);
			//eResult = estiERROR;
			break;
	}

	// Decide the maximum packet size according to codec type
	switch (eMode)
	{
		case estiAUDIO_G711_ALAW:
		case estiAUDIO_G711_ALAW_R:
		case estiAUDIO_G711_MULAW:
		case estiAUDIO_G711_MULAW_R:
		case estiAUDIO_G722:
		{
			if (m_Settings.nMaxAudioFrames < nMIN_RECORD_AUDIO_FRAME_MS)
			{
				m_Settings.nMaxAudioFrames = nMIN_RECORD_AUDIO_FRAME_MS;
			}
			else if (m_Settings.nMaxAudioFrames > nMAX_RECORD_AUDIO_FRAME_MS)
			{
				m_Settings.nMaxAudioFrames = nMAX_RECORD_AUDIO_FRAME_MS;
			}
			
			unsigned short nFrameSize = G711_MS_TO_SIZE (m_Settings.nMaxAudioFrames);
			if (estiAUDIO_G722 == eMode)
			{
				nFrameSize = G722_MS_TO_SIZE (m_Settings.nMaxAudioFrames);
			}
			
			m_nTimestampIncrement = nFrameSize;
			usSamplesPerPacket = nFrameSize;
			
			// Set up a buffer to hold partial packets
			m_nRemainderSize = 0;
			if (m_pRemainderBuffer)
			{
				delete[] m_pRemainderBuffer;
				m_pRemainderBuffer = nullptr;
			}
			m_pRemainderBuffer = new uint8_t[m_RTPHeaderOffset.RtpHeaderOffsetLengthGet () + nFrameSize];
			
			break;
		}

		default:
			stiASSERT (estiFALSE);
			//eResult = estiERROR;
			break;
	} // end switch

	if (estiAUDIO_NONE != esmdAudioCodec)
	{
		// Set audio codec
		stiHResult hResult = m_pAudioInput->AudioRecordCodecSet (esmdAudioCodec);
		if(!stiIS_ERROR (hResult))
		{
			// Set settings to audio record driver
			IstiAudioInput::SstiAudioRecordSettings stAudioSettings;
			stAudioSettings.un16SamplesPerPacket = usSamplesPerPacket;
			stAudioSettings.encodingBitrate = MaxChannelRateGet ();
			hResult = m_pAudioInput->AudioRecordSettingsSet(&stAudioSettings);
		}

		if(!stiIS_ERROR (hResult))
		{
			// Save the current audio codec type
			m_currentCodec = esmdAudioCodec;
		}
		else
		{
			stiASSERT (estiFALSE);
			eResult = estiERROR;
		}
	}

	return (eResult);
} // end CstiAudioRecord::SetDeviceAudioCodec


bool CstiAudioRecord::PrivacyGet() const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	EstiBool bPrivacy;

	m_pAudioInput->PrivacyGet (&bPrivacy);

	return bPrivacy == estiTRUE;
}


/*!
* \brief Load the DataInfo Structure
*/
stiHResult CstiAudioRecord::DataInfoStructLoad (
	const SstiSdpStream &sdpStream,
	const SstiMediaPayload &offer, int8_t /*secondaryPayloadId*/)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto negotiatedCodec = offer.eCodec;
	NegotiatedCodecSet (negotiatedCodec);

	m_Settings = {};  // reset the settings

	m_Settings.payloadTypeNbr = offer.n8PayloadTypeNbr;
	PayloadTypeSet (offer.n8PayloadTypeNbr);
	m_Settings.nMaxAudioFrames = sdpStream.nPtime;

	// Add extra features
	for (auto payloadId : offer.FeatureMediaOffersIds)
	{
		auto mediaPayload = sdpStream.mediaPayloadGet (payloadId);

		if (mediaPayload)
		{
			switch (mediaPayload->eCodec)
			{
			case estiTELEPHONE_EVENT:
				m_Settings.dtmfEventPayloadNbr = mediaPayload->n8PayloadTypeNbr;
				break;

			default:
				break;
			}
		}
	}

	switch (negotiatedCodec)
	{
		case estiG711_ALAW_56K_AUDIO:
		case estiG711_ALAW_64K_AUDIO:
			m_Settings.codec = estiAUDIO_G711_ALAW;
			MaxChannelRateSet (un32AUDIO_RATE);
			break;
		case estiG711_ULAW_56K_AUDIO:
		case estiG711_ULAW_64K_AUDIO:
			m_Settings.codec = estiAUDIO_G711_MULAW;
			MaxChannelRateSet (un32AUDIO_RATE);
			break;
		case estiG722_48K_AUDIO:
		case estiG722_56K_AUDIO:
		case estiG722_64K_AUDIO:
			m_Settings.codec = estiAUDIO_G722;
			MaxChannelRateSet (un32AUDIO_RATE);
			break;

		default:
			stiTHROW (stiRESULT_ERROR);
			break;
	}

STI_BAIL:

	return hResult;
}


EstiAudioCodec CstiAudioRecord::CodecGet ()
{
	return m_Settings.codec;
}
