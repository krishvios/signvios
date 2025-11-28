/*!
* \file CstiAudioPlayback.cpp
* \brief Manages incoming audio packets and passes them down to AudioOutput
*        for playback
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"

#include "CstiAudioPlayback.h"
#include "stiTaskInfo.h"
#include "stiPayloadTypes.h"
#include "rvrtpnatfw.h"
#include <memory>

//
// Constants
//

//
// Typedefs
//

//
// Globals
//

#ifdef stiDEBUGGING_TOOLS
EstiBool g_bReSyncJitterBuffer = estiFALSE;
#endif


/*!
 * \brief Contsructor
 * \param hSipMidMgr
 */
CstiAudioPlayback::CstiAudioPlayback (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t callIndex,
	uint32_t maxRate,
	bool rtpKeepAliveEnable,
	RvSipMidMgrHandle sipMidMgr,
	int remoteSipVersion)
:
	vpe::MediaPlayback<CstiAudioPlaybackRead, CstiAudioPacket, EstiAudioCodec, TAudioPayloadMap>(
		stiAP_TASK_NAME,
		session,
		callIndex,
		maxRate,
		rtpKeepAliveEnable,
		sipMidMgr,
		estiAUDIO_NONE,
		estiG711_ULAW_64K_AUDIO,
		remoteSipVersion
	)
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::CstiAudioPlayback);
}


/*!
 * \brief Destructor
 */
CstiAudioPlayback::~CstiAudioPlayback ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::~CstiAudioPlayback);

	DisconnectAudioOutputSignals ();

#ifdef stiENABLE_MULTIPLE_AUDIO_OUTPUT
	if (m_pAudioOutput)
	{
		//This is a smart pointer below so the regular instance delete can not be called.
		m_pAudioOutput = NULL;
	}
#endif

}

int CstiAudioPlayback::ClockRateGet() const
{
	return GetAudioClockRate();
}

stiHResult CstiAudioPlayback::AudioOutputDeviceSet(
	const std::shared_ptr<IstiAudioOutput> &audioOutput)
{
	PostEvent (
		std::bind(&CstiAudioPlayback::EventAudioOutputDeviceSet, this, audioOutput));

	return stiRESULT_SUCCESS;
}


void CstiAudioPlayback::EventAudioOutputDeviceSet (
	std::shared_ptr<IstiAudioOutput> audioOutput)
{
	if (m_pAudioOutput == audioOutput)
	{
		return;
	}

	if (m_pAudioOutput)
	{
		m_pAudioOutput->AudioPlaybackStop();
	}

	DisconnectAudioOutputSignals ();

	m_pAudioOutput = audioOutput;

	ConnectAudioOutputSignals ();

	if (m_pAudioOutput)
	{
		m_pAudioOutput->AudioPlaybackStart();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::Initialize
//
// Description: First stage of initialization of the audio playback task.
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiAudioPlayback::Initialize (
	const TAudioPayloadMap &audioPayloadMap)
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Open audio device driver
	//
	
#ifdef stiENABLE_MULTIPLE_AUDIO_OUTPUT
	// Get instance of the audio device controller
	if (!m_pAudioOutput)
	{
		m_pAudioOutput = std::shared_ptr<IstiAudioOutput>(IstiAudioOutput::InstanceCreate(m_callIndex), [](IstiAudioOutput*){});
	}
#else
	// Get instance of the audio device controller
	if (!m_pAudioOutput)
	{
		m_pAudioOutput = std::shared_ptr<IstiAudioOutput>(IstiAudioOutput::InstanceGet(), [](IstiAudioOutput*){});
	}
#endif

	stiTESTCOND (nullptr != m_pAudioOutput, stiRESULT_TASK_INIT_FAILED);

	m_displayPacket = nullptr;

	hResult = MediaPlaybackType::Initialize(audioPayloadMap);

STI_BAIL:
	return hResult;

}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::Uninitialize
//
// Description: Final cleanup of the audio playback task.
//
// Returns:
//	none
//
void CstiAudioPlayback::Uninitialize ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::Cleanup);

	//
	// Cleanup the class-specific variables
	//

	// Empty the packet queues
	m_oPacketQueue.clear ();
	m_oDisplayQueue.clear ();

	MediaPlaybackType::Uninitialize ();
}


/*!
 * \brief Changes the audio mode in the driver
 *
 *	This function changes the audio mode value into audio driver
 */
stiHResult CstiAudioPlayback::SetDeviceCodec (
		EstiAudioCodec eMode)
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::SetDeviceAudioCodec);

	stiDEBUG_TOOL ( g_stiAPDebug,
					stiTrace("AP:: SetDeviceAudioCodec mode = %d\n", eMode);
			);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiAudioCodec esmdAudioCodec = estiAUDIO_NONE;


	m_nCurrentFrameSize = 0;
	m_un32TimestampIncrement = 0;

	switch (eMode)
	{
	case estiAUDIO_G711_ALAW:
	case estiAUDIO_G711_ALAW_R:
		esmdAudioCodec = estiAUDIO_G711_ALAW;
		break;

	case estiAUDIO_G711_MULAW:
	case estiAUDIO_G711_MULAW_R:
		esmdAudioCodec = estiAUDIO_G711_MULAW;
		break;

	case estiAUDIO_G722:
		esmdAudioCodec = estiAUDIO_G722;
		break;

	default:
		stiASSERT (estiFALSE);
		//eResult = estiERROR;
		break;
	}

	// Tell audio output of a codec change.
	if (estiAUDIO_NONE != esmdAudioCodec
		&& ((m_currentCodec != esmdAudioCodec) ||
			m_channelResumed))
	{
		m_channelResumed  = false;
		
		stiHResult hResult = m_pAudioOutput->AudioPlaybackCodecSet (esmdAudioCodec);
		if (!stiIS_ERROR(hResult))
		{
			// save the current audio codec type
			m_currentCodec = esmdAudioCodec;
		}
		else
		{
			stiASSERT (estiFALSE);
			hResult = stiRESULT_ERROR;
		}
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::PacketQueueProcess
//
// Desc: Processes out-of-order packets previously received
//
// Abstract:
//	Looks for packets which were received before their time. If found, they are
//	processed. If the packet queue fills up, then loss will be determined from the
//	packets in this queue.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiAudioPlayback::PacketQueueProcess (
	const std::shared_ptr<CstiAudioPacket> &readPacket)
{

	stiLOG_ENTRY_NAME (CstiAudioPlayback::PacketQueueProcess);

	EstiResult eResult = estiOK;
	EstiBool bContinue = estiTRUE;


	while (bContinue)
	{
		// If the queue is empty, break out
		if (m_oPacketQueue.empty ())
		{
			break;
		}

		// Search the queue for the expected sequence number
		uint32_t unNextPacket;
		auto packet = m_oPacketQueue.SequenceNumberSearchEx (
						m_un16ExpectedPacket, &unNextPacket);

		stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
					   stiTrace ("AP:: Searching for SQ# %d...\n",
								 m_un16ExpectedPacket);
				);

		// Did we find the packet we were looking for?
		if (packet && unNextPacket == CstiPacketQueue<CstiAudioPacket>::un32INVALID_SEQ)
		{
			// Yes!
			stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
						   stiTrace ("AP:: Found SQ# %d!\n",
									 m_un16ExpectedPacket);
					);

			// Remove the packet from the packet queue
			m_oPacketQueue.Remove (packet);

			// Process the found packet
			PacketProcess (packet);

			// Increment the expected sequence number to look for the next
			// sequence number in the queue
			m_un16ExpectedPacket++;

			if(m_bSendToDriver)
			{
				// Yes! Write this Frame to the driver.
				AudioPacketToDisplayQueue ();
			}

		} // end if
		else
		{
			if (!readPacket)
			{
				// No more to do, break out and continue
				bContinue = estiFALSE;
			}
			else
			{
				// Is the queue full?
				if (m_oPacketQueue.CountGet () >= nOUTOFORDER_AP_BUFFERS)

				{
					//
					// Queue is full, we will lose packets.
					// Update the m_un16ExpectedPacket to the next one
					// we expected which could be the ones in the queue or
					// the one just read in
					//
					if (packet && unNextPacket != CstiPacketQueue<CstiAudioPacket>::un32INVALID_SEQ)
					{

						uint16_t un16PacketsLost;
						RvRtpParam * pstRTPParam = readPacket->RTPParamGet ();
						uint16_t un16SequenceNumber = pstRTPParam->sequenceNumber;

						if (unNextPacket > m_un16ExpectedPacket)
						{
							// Check if the next one we expected is the one just read in
							if(un16SequenceNumber >= m_un16ExpectedPacket && un16SequenceNumber < unNextPacket)
							{
								// Yes! we will break out and continue from there
								unNextPacket = un16SequenceNumber;
								bContinue = estiFALSE;
							}
						}
						else
						{
							// Check if the next one we expected is the one just read in
							if((un16SequenceNumber >= m_un16ExpectedPacket) || (un16SequenceNumber < unNextPacket))
							{
								// Yes! we will break out and continue from there
								unNextPacket = un16SequenceNumber;
								bContinue = estiFALSE;
							}
						}

						// Was the packet that was found less than the one we
						// were looking for?
						if (unNextPacket < m_un16ExpectedPacket)
						{
							// Yes! Change the loss calculation to account for the wrap
							un16PacketsLost = (uint16_t)((0xFFFF - m_un16ExpectedPacket) + unNextPacket);
						} // end if
						else
						{
							// Calculate how many packets were lost
							un16PacketsLost = (uint16_t)(unNextPacket - m_un16ExpectedPacket);
						}

						stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
									   stiTrace ("AP:: The Queue is Full!. We have a "
												 "lost packet - %d \nNextPacket - %d\n",
												 m_un16ExpectedPacket, unNextPacket);
								);

						// Keep track of how many packets have been lost.
						PacketLossCountAdd (un16PacketsLost);

						// Set our expected sequence number equal to this number,
						// and let it be handled the next time through the loop.

						m_un16ExpectedPacket = (uint16_t) unNextPacket;
					}
					else
					{
						// We should never get here
						stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
									   stiTrace ("AP:: we should NEVER get here, "
												 "something wrong !!!\n");
								);

						stiASSERT (estiFALSE);
						bContinue = estiFALSE;

						//
						// Move all queued packets back into the empty queue.
						//
						AudioQueueEmpty (&m_oPacketQueue);

						// Clear the current frame we are processing
						m_framePacket = nullptr;
						m_bSendToDriver = estiFALSE;

						// Reset first packet to true.
						m_bFirstPacket = estiTRUE;

					} // end else

				} // end if
				else
				{
					// No! The queue is not full, so we will break out and continue.

					bContinue = estiFALSE;
				}

			} // end if
		} // end else
	} // end while

	// Queue must never be full at this point!!!
	stiASSERT (m_oPacketQueue.CountGet () <= nOUTOFORDER_AP_BUFFERS);

	return (eResult);
} // end CstiAudioPlayback::PacketQueueProcess



////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::PutPacketIntoQueue
//
// Description: Put the packet into queue
//
// Abstract:
//	This packet is a out-of-order packet. Put the packet into queue
//  for later processing
//
// Returns:
//	estiOK if successful, estiERROR otherwise.
//
EstiResult CstiAudioPlayback::PutPacketIntoQueue (
	const std::shared_ptr<CstiAudioPacket> &packet)
{
	EstiResult eResult = estiOK;
	uint16_t un16SequenceNumber = (packet->RTPParamGet ())->sequenceNumber;

	if (un16SequenceNumber >= m_un16ExpectedPacket)
	{
		// Yes! Has this sequence number already been added to the queue?
		if (nullptr != m_oPacketQueue.SequenceNumberSearch (un16SequenceNumber))
		{
			stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
						   stiTrace("AP::  SQ Greater than expected "
									"Discarding SQ# %d. Already in Queue\n",
									un16SequenceNumber);
					);
		}
		else
		{
			stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
						   stiTrace ("AP:: SQ Greater than expected "
									 "Adding SQ# %d to Queue\n",
									 un16SequenceNumber);
					);

			// No! Place the current packet into the packet queue and we will
			// processs it later.
			m_oPacketQueue.Add (packet);
		}

		// Reset the throw away count.
		m_nThrowAwayCount = 0;
	}
	else // (un16SequenceNumber < expected)
	{
		// The sequence number was less than expected.
		// Is there a possibility that the sequence number has wrapped?
		if(un16SequenceNumber <= 0x00ff && m_un16ExpectedPacket >= (0xffff - 0x00ff))
		{
			// Yes! the sequence number has wrapped.
			// Has this sequence number already been added to the queue?
			if (nullptr != m_oPacketQueue.SequenceNumberSearch (un16SequenceNumber))
			{
				stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
							   stiTrace ("AP:: SQ less than expected "
										 "Discarding SQ# %d. Already in Queue\n",
										 un16SequenceNumber);
						);
			}
			else
			{
				stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
							   stiTrace ("AP:: SQ less than expected "
										 "Possible Wrap Adding SQ# %d to Queue\n",
										 un16SequenceNumber);
						);

				// Add this packet to the queue, and we will eventually
				// get around to processing it out of the queue.
				m_oPacketQueue.Add (packet);
			}


			// Reset the throw away count.
			m_nThrowAwayCount = 0;
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
						   stiTrace ("AP:: Discarding SQ# %d. "
									 "Less than expected\n",
									 un16SequenceNumber);
					);

			// Increment the number of consecutive packets we have thrown
			// away because the sequence number is less than expected.
			m_nThrowAwayCount++;

			// Is the throw away count greater than nMAX_THROW_COUNT?
			if (m_nThrowAwayCount > nMAX_THROW_COUNT)
			{
				// Yes! Empty Packet Queue
				AudioQueueEmpty (&m_oPacketQueue);

				// Reset first packet to true.
				m_bFirstPacket = estiTRUE;

				// Reset throw away count.
				m_nThrowAwayCount = 0;
			} // end if
		} // end else
	} // end else

	return eResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::ReadPacketProcess
//
// Description: Processes the packet getting from audio playback read task
//
// Abstract:
//	Handles packet ordering problems, and passes packets to be decoded.
//
// Returns:
//	estiOK if successful, estiERROR
//
EstiResult CstiAudioPlayback::ReadPacketProcess (
	const std::shared_ptr<CstiAudioPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::ReadPacketProcess);

	EstiResult 	eResult = estiOK;
	RvRtpParam * 	pstRTPParam = packet->RTPParamGet ();
	uint16_t 	un16SequenceNumber = pstRTPParam->sequenceNumber;

	// Is this the sequence number we were expecting?
	if(un16SequenceNumber == m_un16ExpectedPacket)
	{
		// Yes! Is this sequence number already in our queue?
		auto duplicatePacket = m_oPacketQueue.SequenceNumberSearch (un16SequenceNumber);

		if (duplicatePacket)
		{
			stiDEBUG_TOOL (g_stiAPPacketQueueDebug,
						   stiTrace ("AP:: Found Duplicate of "
									 "SQ# %d in Queue. Removing...\n",
									 un16SequenceNumber);
					);

			// Yes! Remove the duplicate from the packet queue.
			m_oPacketQueue.Remove (duplicatePacket);

		} // end if

		// Send the packet to be processed.
		PacketProcess (packet);

		// We are now expecting the next sequence number to be found.
		m_un16ExpectedPacket++;

		// Reset the throw away count.
		m_nThrowAwayCount = 0;

	} // end if
	else
	{
		PutPacketIntoQueue (packet);
	} // end else


	return eResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::PacketProcess
//
// Description: Deassembly packet into frames if necessary, then decodes the frames.
//
// Abstract:
//	Deassembly packet into audio frames, then sends the frames to the decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
stiHResult CstiAudioPlayback::PacketProcess (
	std::shared_ptr<CstiAudioPacket> packet)
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::PacketProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	RvRtpParam * pstRTPParam = packet->RTPParamGet ();

	SsmdAudioFrame * pstAudioFrame = packet->m_frame;

	m_framePacket = nullptr;
	m_bSendToDriver = estiFALSE;

	pstAudioFrame->bIsSIDFrame = estiFALSE;

	if (m_nCurrentRTPPayload != pstRTPParam->payload)
	{
		TAudioPayloadMap::const_iterator i;

		i = m_PayloadMap.find (pstRTPParam->payload);

		if (i != m_PayloadMap.end ())
		{
			EstiAudioCodec eNewCodecType = i->second.eCodec;

			// Change to it!
			m_nCurrentRTPPayload = pstRTPParam->payload;

			m_pAudioOutput->AudioPlaybackStop ();

			hResult = SetDeviceCodec (eNewCodecType);

			m_pAudioOutput->AudioPlaybackStart ();
			// TODO: how to handle this error
		} // end if
		else
		{
			// Tried to switch to an unknown codec.
			stiASSERT (estiFALSE);

			// It's not a valid video packet, ignore this one
			packet = nullptr;
		} // end else
	} // end if

	if (packet)
	{
		m_bSendToDriver = estiFALSE;

		switch (m_currentCodec)
		{
		case estiAUDIO_G722:
			m_bSendToDriver = estiTRUE;
			break;

		case estiAUDIO_G711_ALAW:
		case estiAUDIO_G711_MULAW:

			// Make sure the data length is correct
			if (smdAUDIO_G711_MAX_SIZE >= pstAudioFrame->unFrameSizeInBytes &&
				smdAUDIO_G711_MIN_SIZE <= pstAudioFrame->unFrameSizeInBytes)
			{
				m_bSendToDriver = estiTRUE;
			}
			else
			{
				stiASSERT (false);
			}
			break;

		default:
			stiASSERT (estiFALSE);
			break;

		} // end switch


		if (estiTRUE == m_bSendToDriver)
		{
			pstAudioFrame->esmdAudioMode = m_currentCodec;
			m_framePacket = packet;
		}
		else
		{
			// This is not a valid frame to send to decoder
			m_framePacket = nullptr;

		}
	}
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::ProcessAvailableData
//
// Desc: Looks for and processes a packet which will be ready to send to decoder.
//
// Abstract:
//  Processes out-of-order packets previously received, or read in a new one
//  to find the expected packet which is ready to send to decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiAudioPlayback::ProcessAvailableData ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::ProcessAvailableData);
	EstiResult eResult = estiOK;

	RvRtpParam * pstRTPParam;
	uint16_t un16SequenceNumber;

	if (estiTRUE == m_bChannelOpen)
	{
		if (m_playbackRead.NumPacketAvailable() > 0)
		{
			// Get a packet from the read task
			auto packet = MediaPacketFullGet ();

			// Did we get a packet?
			if (!packet)
			{
				// No! This is a problem.
				stiASSERT (estiFALSE);
				eResult = estiERROR;
			} // end if
			else if (!m_nMuted)
			{
				// Yes! process this packet
				pstRTPParam = packet->RTPParamGet ();
				un16SequenceNumber = pstRTPParam->sequenceNumber;


				stiDEBUG_TOOL (g_stiAPDebug,
							   stiTrace("AP:: Getting SQ# %d\n",
										un16SequenceNumber);
						);

				// Keep track of this sequence number
				PacketsReceivedCountAdd (pstRTPParam->sequenceNumber);

				// Keep track of how many (payload) bytes were received
				// ACTION SLB - 2/18/02 Does payload bytes make more sense than
				// total RTP Bytes?
				// TYY - 2/26/04 should we count byte based on what we processed (here) or
				// based on what we received (CstiAudioPlaybackRead::PacketRead()).
				// They are different in the case of system saturation.
				ByteCountAdd (pstRTPParam->len - pstRTPParam->sByte);

				if (m_bFirstPacket)
				{
					// Set the expected sequence number
					m_un16ExpectedPacket = un16SequenceNumber;

					// Set the expected frame size and time increment for one audio frame
					SsmdAudioFrame * pstAudioFrame = packet->m_frame;
					{
						m_nCurrentFrameSize = pstAudioFrame->unFrameSizeInBytes;
					}

					if (RV_RTP_PAYLOAD_PCMA == pstRTPParam->payload ||
						RV_RTP_PAYLOAD_PCMU ==pstRTPParam->payload)
					{
						// For sample rate is equal to 8000Hz, the following two value are the same
						m_un32TimestampIncrement = m_nCurrentFrameSize;
					}

					m_bFirstPacket = estiFALSE;

					stiDEBUG_TOOL (g_stiAPDebug,
								   stiTrace("AP:: First packet SQ# %d\n\n", m_un16ExpectedPacket);
							);

				}

				// Process any out-of-order packets that might be in the packet queue
				PacketQueueProcess (packet);

				// Process the packet
				ReadPacketProcess (packet);
			}

		}
		else
		{
			// Process any out-of-order packets that might be in the packet queue
			PacketQueueProcess (nullptr);
		}
	}
#if 0
	else
	{
		stiDEBUG_TOOL (g_stiAPDebug,
					   //stiTrace ("CstiAudioPlayback:: Connection Close!!!\n");
					   );
	}
#endif

	if (estiTRUE == m_bSendToDriver)
	{
		AudioPacketToDisplayQueue ();
	}

	return eResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::AudioPacketToDisplayQueue
//
// Description: Sends a packet containing one or more complete frames to the
//  sync packet queue
//
// Abstract:
//	The sync packet queue contains audio frames which are ready to send to driver
//  for playback
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiAudioPlayback::AudioPacketToDisplayQueue ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::AudioPacketToDisplayQueue);

	EstiResult eResult = estiOK;

	// Get the pointer to the audio packet structure
	SsmdAudioFrame * pstAudioFrame = m_framePacket->m_frame;
	{
		//
		// TODO: Do we have to take care of padding to a 16 bit boundary
		//

		// How many frames in this packet
		pstAudioFrame->un8NumFrames = (uint8_t) 1;

		// Time duration for each frame
		pstAudioFrame->unFrameDuration = m_un32TimestampIncrement;

		// Add this packet to sync queue

		m_oDisplayQueue.Add (m_framePacket);
		stiDEBUG_TOOL (g_stiAPDebug,
					   stiTrace ("AP:: SQ# %d is Sending to Sync Queue (%d packets in queue)\n\n",
								 (m_framePacket->RTPParamGet ())->sequenceNumber,
								 m_oDisplayQueue.CountGet());
				);

		m_framePacket = nullptr;
		m_bSendToDriver = estiFALSE;
	}

	return (eResult);
} // end CstiAudioPlayback::AudioPacketToDisplayQueue

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlayback::SyncPlayback
//
// Desc: Looks for and processes a audio packet which will be ready
//  to send to decoder from sync packet queue
//
// Abstract:
//	The function will send audio frames whose time become current to audio
//  driver for playback
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiAudioPlayback::SyncPlayback ()
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::SyncPlayback);

	EstiResult eResult = estiOK;

	//if(estiTRUE == m_bChannelOpen)
	{

#ifdef stiDEBUGGING_TOOLS
		static EstiBool bTest = estiFALSE;

		if (m_displayPacket)
		{
			g_bReSyncJitterBuffer = estiTRUE;
		}
#endif

		if (!m_displayPacket && !m_oDisplayQueue.empty ())
		{
			auto packet = m_oDisplayQueue.OldestGet ();
			
			if (packet)
			{
				// Now remove that packet from the queue
				m_oDisplayQueue.Remove (packet);
				m_displayPacket = packet;
			} // end if
		} // end if

		if (m_displayPacket)
		{
			SsmdAudioFrame * pstAudioFrame = m_displayPacket->m_frame;
			RvRtpParam * pstRTPParam = m_displayPacket->RTPParamGet ();

			pstAudioFrame->un32RtpTimestamp = pstRTPParam->timestamp;

			// set timestamp of the current audio frame to sync manager
			m_pSyncManager->SetCurrAudioTimestamp(pstRTPParam->timestamp, pstAudioFrame->unFrameDuration);

			if (0 != pstAudioFrame->unFrameSizeInBytes)
			{
				stiHResult hResult = m_pAudioOutput->AudioPlaybackPacketPut(pstAudioFrame);

				if (stiIS_ERROR (hResult))
				{
					// We had an error sending data to driver
					stiASSERT (estiFALSE);
					eResult = estiERROR;
				}
				else
				{
					stiDEBUG_TOOL (g_stiAPDebug,
								   static int nCount = 0;
							if (pstAudioFrame->un8NumFrames > 1 || g_bReSyncJitterBuffer)
					{
						nCount = 300;
						g_bReSyncJitterBuffer = estiFALSE;
					}

					if (++nCount > 300)
					{
						stiTrace ("AP:: SQ# %d (with %d packets) NTP = %lu is Sending to driver\n",
								  pstRTPParam->sequenceNumber,
								  pstAudioFrame->un8NumFrames,
								  //pstRTPParam->timestamp,
								  m_pSyncManager->GetCurrAudioNTPTimestamp()
								  );
					} // end if (++nCount > 300)

					if (nCount == 302)
					{
						nCount = 0;
						bTest = estiTRUE;
					}
					);
				} // end else
			} // end if (0 != pstAudioFrame->unFrameSizeInBytes)
			else
			{
				stiDEBUG_TOOL (g_stiAPDebug,
							   stiTrace ("AP:: SQ# %d timestamp = %lu NTP = %lu is silence frame\n\n",
										 pstRTPParam->sequenceNumber,
										 pstRTPParam->timestamp,
										 m_pSyncManager->GetCurrAudioNTPTimestamp());
						);
			} // end else

			pstAudioFrame->un8NumFrames --;

			if (pstAudioFrame->un8NumFrames == 0)
			{

				// we are done with this packet
				m_displayPacket = nullptr;

				stiDEBUG_TOOL (g_stiAPDebug,
							   if (estiTRUE == bTest)
				{
								   stiTrace("AP:: empty = %d full = %d display = %d packet queue = %d\n",
								   m_playbackRead.NumEmptyBufferAvailable(),
								   m_playbackRead.NumPacketAvailable(),
								   m_oDisplayQueue.CountGet(), m_oPacketQueue.CountGet());
								   stiTrace ("\n");
								   bTest = estiFALSE;
							   }
							   );
			}
			else if (pstAudioFrame->un8NumFrames > 0)
			{
				// update to the buffer position of the next audio frame
				pstAudioFrame->pun8Buffer += pstAudioFrame->unFrameSizeInBytes;
				pstRTPParam->timestamp += pstAudioFrame->unFrameDuration;
				pstRTPParam->sequenceNumber ++;
			}

		} // end if (NULL != m_poDisplayPacket)
		else
		{
			stiDEBUG_TOOL (g_stiAPDebug,
						   stiTrace("AP:: !!! No packet ready to send to drvier !!!\n");
					);
		}
	} // end if

	return eResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiAudioPlaybackRead::ReSyncJitterBuffer
//
// Description: Indicate to hold some packets in the jitter buffer
//
// Abstract:
//
// Returns:
//	stiOK on Success, otherwise estiERROR
//
EstiResult CstiAudioPlayback::ReSyncJitterBuffer ()
{

	stiLOG_ENTRY_NAME (CstiAudioPlayback::ReSyncJitterBuffer);

	// Get the oldest packet in the full queue
	EstiResult eResult = m_playbackRead.ReSyncJitterBuffer ();

	return (eResult);
} // end CstiAudioPlaybackRead::ReSyncJitterBuffer


/*!\brief Stops the processing of data received from the remote endpoint
 *
 */
stiHResult CstiAudioPlayback::DataChannelHold ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiAudioPlayback::DataChannelHold);

	stiDEBUG_TOOL (g_stiAPDebug,
				   stiTrace ("CstiAudioPlayback::DataChannelHold\n");
			);

	DisconnectSignals ();
	m_outputDeviceReady = false;

	// Make sure the keep alive timer stops running
	m_keepAliveTimer.stop ();

	MediaPlaybackType::DataChannelHold();

	m_bSendToDriver = estiFALSE;
	m_framePacket = nullptr;

	m_un32TimestampIncrement = 0;

	AudioQueueEmpty (&m_oDisplayQueue);
	m_displayPacket = nullptr;

	hResult = m_pAudioOutput->AudioPlaybackStop ();
	stiASSERT(!stiIS_ERROR(hResult));

	return hResult;
} // end CstiAudioPlayback::DataChannelHold



/*!\brief Resumes the processing of data received from the remote endpoint
 *
 */
stiHResult CstiAudioPlayback::DataChannelResume ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Set our first packet indicator and reset the rtp structure
	m_bFirstPacket = estiTRUE;
	
	// Indicate that the channel resumed to cause AudioPlaybackCodecSet to be called.
	m_channelResumed = true;

	//
	// Move all queued packets back into the empty queue.
	//
	AudioQueueEmpty (&m_oPacketQueue);

	if(m_pSyncManager != nullptr)
	{
		m_pSyncManager->Initialize();
	}

	AudioQueueEmpty (&m_oDisplayQueue);

	if (m_displayPacket)
	{
		m_displayPacket = nullptr;
	}

	hResult = m_playbackRead.RtpHaltedSet(false);
	stiTESTRESULT ();

	ConnectSignals ();

	// Note: AudioOutput will start on receipt of the first audio packet after resuming

	m_bChannelOpen = estiTRUE;

STI_BAIL:

	return hResult;
}


/*!
 * \brief Helper method to DataChannelInitialize for connecting to signals
 */
void CstiAudioPlayback::ConnectSignals ()
{
	// Forward DTMF digits to audio output
	if (!m_playbackReadDtmfReceivedSignalConnection)
	{
		m_playbackReadDtmfReceivedSignalConnection = m_playbackRead.dtmfReceivedSignal.Connect (
					[this](EstiDTMFDigit digit){
				m_pAudioOutput->DtmfReceived (digit);
	});
	}

	// Handle incoming data packets
	if (!m_playbackReadDataAvailableSignalConnection)
	{
		m_playbackReadDataAvailableSignalConnection = m_playbackRead.dataAvailableSignal.Connect (
					[this]{
			PostEvent (
						std::bind(&CstiAudioPlayback::EventDataAvailable, this));
		});
	}

	ConnectAudioOutputSignals ();
}


/*!
 * \brief Connect to platform signals
 */
void CstiAudioPlayback::ConnectAudioOutputSignals ()
{
	// Listen for ready state changes from audio output,
	// so we know when we can safely feed it more packets to play
	if (!m_audioOutputReadyStateChangedSignalConnection)
	{
		m_audioOutputReadyStateChangedSignalConnection = m_pAudioOutput->readyStateChangedSignalGet().Connect (
			[this](bool ready){
				if (m_outputDeviceReady != ready)
				{
					m_outputDeviceReady = ready;
					if (ready)
					{
						PostEvent ([this]{
							// Clear out all existing packets so we don't play audio
							// that is late due to waiting for the pipeline to be ready
							AudioQueueEmpty (&m_oPacketQueue);
							AudioQueueEmpty (&m_oDisplayQueue);

							EventDataAvailable();
						});
					}
				}
			});
	}
}


/*!
 * \brief Helper method to DataChannelClose for disconnecting from signals
 */
void CstiAudioPlayback::DisconnectSignals ()
{
	m_playbackReadDtmfReceivedSignalConnection.Disconnect ();

	m_playbackReadDataAvailableSignalConnection.Disconnect ();

	DisconnectAudioOutputSignals ();
}


/*!
 * \brief Disconnect signals from platform
 */
void CstiAudioPlayback::DisconnectAudioOutputSignals ()
{
	m_audioOutputReadyStateChangedSignalConnection.Disconnect ();
}


/*! brief Inform the data task if the remote has set video privacy or not
 *
 * \retval stiRESULT_SUCCESS or stiRESULT_ERROR
 */
stiHResult CstiAudioPlayback::Muted (
		eMutedReason eReason) // Is privacy set to ON or OFF
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::SetRemoteAudioMute);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	switch (eReason)
	{
	case estiMUTED_REASON_PRIVACY:

		if ((m_nMuted & estiMUTED_REASON_PRIVACY) == 0)
		{
			// Reset to get the new sync information
			m_pSyncManager->AudioSyncReset ();

			m_nMuted |= estiMUTED_REASON_PRIVACY;
			m_bFirstPacket = estiTRUE;

			// Empty the packet queues
			AudioQueueEmpty (&m_oPacketQueue);
			AudioQueueEmpty (&m_oDisplayQueue);
		}

		break;

	case estiMUTED_REASON_HELD:

		if ((m_nMuted & estiMUTED_REASON_HELD) == 0)
		{
			// Reset to get the new sync information
			m_pSyncManager->AudioSyncReset ();

			m_playbackRead.RtpHaltedSet (true);

			m_nMuted |= estiMUTED_REASON_HELD;
			m_bFirstPacket = estiTRUE;

			// Empty the packet queues
			AudioQueueEmpty (&m_oPacketQueue);
			AudioQueueEmpty (&m_oDisplayQueue);
		}

		break;

	case estiMUTED_REASON_HOLD:

		if ((m_nMuted & estiMUTED_REASON_HOLD) == 0)
		{
			m_un32InitialRtcpTime = m_playbackRead.InitialRTCPTimeGet ();

			DataChannelHold ();

			m_nMuted |= estiMUTED_REASON_HOLD;
			m_bFirstPacket = estiTRUE;

			// Empty the packet queues
			AudioQueueEmpty (&m_oPacketQueue);
			AudioQueueEmpty (&m_oDisplayQueue);
		}

		break;
			
		case estiMUTED_REASON_DHV:
			break;
	}

	Unlock ();

	return (hResult);
}


/*! brief Inform the data task if the remote has set video privacy or not
 *
 * \retval stiRESULT_SUCCESS or stiRESULT_ERROR
 */
stiHResult CstiAudioPlayback::Unmuted (
		eMutedReason eReason) // Is privacy set to ON or OFF
{
	stiLOG_ENTRY_NAME (CstiAudioPlayback::SetRemoteAudioMute);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	switch (eReason)
	{
	case estiMUTED_REASON_PRIVACY:

		if ((m_nMuted & estiMUTED_REASON_PRIVACY) != 0)
		{
			m_nMuted &= ~estiMUTED_REASON_PRIVACY;

			m_pSyncManager->AudioSyncReset ();
		}

		break;

	case estiMUTED_REASON_HELD:

		if ((m_nMuted & estiMUTED_REASON_HELD) != 0)
		{
			m_nMuted &= ~estiMUTED_REASON_HELD;

			m_pSyncManager->AudioSyncReset ();

			m_playbackRead.RtpHaltedSet (false);
		}

		break;

	case estiMUTED_REASON_HOLD:

		if ((m_nMuted & estiMUTED_REASON_HOLD) != 0)
		{
			m_nMuted &= ~estiMUTED_REASON_HOLD;

			DataChannelResume ();
		}

		break;
			
		case estiMUTED_REASON_DHV:
			break;
	}

	Unlock ();

	return (hResult);
}


///\brief Return packets from the passed in queue to the read queue for re-use.
///
///\retval stiRESULT_SUCCESS or stiRESULT_ERROR (and a result)
///
stiHResult CstiAudioPlayback::AudioQueueEmpty (CstiPacketQueue<CstiAudioPacket> *poQueue)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (poQueue, stiRESULT_ERROR);
	
	poQueue->empty ();

STI_BAIL:

	return hResult;
}


/*!
 * \brief Event handler for when playback read notifies us that data is available
 */
void CstiAudioPlayback::EventDataAvailable ()
{
	ProcessAvailableData ();

	if (m_outputDeviceReady && m_bChannelOpen)
	{
		SyncPlayback ();
	}

	if (m_outputDeviceReady && (m_playbackRead.NumPacketAvailable() > 0 || m_oDisplayQueue.CountGet() > 0))
	{
		PostEvent (
			std::bind(&CstiAudioPlayback::EventDataAvailable, this));
	}
}

/*!
* \brief Load some initial settings
*/
stiHResult CstiAudioPlayback::DataInfoStructLoad ()
{
	auto hResult = stiRESULT_SUCCESS;
	bool useSRA = false;

	switch (NegotiatedCodecGet())
	{
		case estiG711_ALAW_64K_AUDIO:
			if (useSRA)
			{
				MaxChannelRateSet (un32AUDIO_RATE * 2);
			}
			else
			{
				MaxChannelRateSet (un32AUDIO_RATE);
			}
			break;

		case estiG711_ULAW_64K_AUDIO:
			if (useSRA)
			{
				 MaxChannelRateSet (un32AUDIO_RATE * 2);
			}
			else
			{
				MaxChannelRateSet (un32AUDIO_RATE);
			}
			break;

		case estiG722_64K_AUDIO:
			MaxChannelRateSet (un32AUDIO_RATE);
			break;

		default:
			stiTESTCOND (estiFALSE, stiRESULT_ERROR);
			break;
	}

STI_BAIL:
	return hResult;
}

