/*!
* \file CstiVideoPlayback.cpp
* \brief Assembles received media packets into frames and passes
*        them down to VideoOutput in the platform layer for displaying
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2000-2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"
#include "RtpPayloadAddon.h"
#include "CstiVideoPlayback.h"
#include "stiTaskInfo.h"
#include "stiTools.h"
#include "IstiVideoOutput.h"
#include "rvrtpnatfw.h"
#include "rvtimer.h"
#include "rvmutex.h"
#include "rtcpfbplugin.h"


#ifndef stiNO_H264_MISSING_PACKET_DETECTION
//#define stiH264_MISSING_PACKET_KEYFRAME_REQUEST
#endif

//
// Constants
//
const uint8_t un8FU_INDICATOR_SIZE = 1;
const uint8_t un8FU_HEADER_SIZE = 1;

const uint8_t un8STAPA_TYPE_SIZE = 1;
const uint8_t un8STAPA_NALULEN_SIZE = 2;

const uint8_t un8RTP_H265_PAYLOAD_HEADER_SIZE = 2;
const uint8_t un8H265FU_HEADER_SIZE = 1;

uint8_t const un8StartMask[9] =
	{0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};

uint8_t const un8EndMask[9] =
	{0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};

static const int nKEYFRAME_RE_REQUEST_INTERVAL = 3000; // 3 seconds

// How often we should check if we need to make an adjustment to max receive rate,
// and send a TMMBR request.  Changing this value will not effect how quickly we
// raise the max rate, only how fast we can react to packet loss.
constexpr int TMMBR_INTERVAL_MS = 5000;

constexpr int JITTER_TIMER_MS = 5;
constexpr int JITTER_TIMER_AFTER_SEND = 15;
constexpr int NACK_INTERVAL_MS = 20;


//
// Typedefs
//


//
// Forward Declarations
//

//
// Globals
//
#ifdef stiDEBUGGING_TOOLS
const char szVPFileOut[] = "wdataVP.264";
FILE * pVPFileOut;
EstiBool g_bMissingPacket = estiFALSE;
#endif

//
// Locals
//
#ifdef ercDebugPacketQueue
#endif //#ifdef ercDebugPacketQueue

namespace
{

/*!
 * \brief Sometimes a changing payload will require a restart (ie: h263/h264), but sometimes
 *        it doesn't (ie: h264/h264 but packetization mode changes)
 */
bool payloadChangeRequiresRestart (
	const VideoPayloadAttributes &newAttributes,
	const TVideoPayloadMap &payloadMap,
	int8_t currentPayload)
{
	auto iter = payloadMap.find(currentPayload);

	// If current payload is not set, the video is already shut down
	if(payloadMap.end() == iter)
	{
		return false;
	}

	// If codecs don't change, no need to restart
	// TODO: any other conditions to check?
	return (iter->second.eCodec != newAttributes.eCodec);
}

}

/*!
 * \brief Constructor
 */
CstiVideoPlayback::CstiVideoPlayback (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t callIndex,
	uint32_t maxRate,
	EstiAutoSpeedMode autoSpeedMode,
	bool rtpKeepAliveEnable,
	RvSipMidMgrHandle sipMidMgr,
	int remoteSipVersion)
:
	vpe::MediaPlayback<CstiVideoPlaybackRead, CstiVideoPacket, EstiVideoCodec, TVideoPayloadMap>(
		stiVP_TASK_NAME,
		session,
		callIndex,
		maxRate,
		rtpKeepAliveEnable,
		sipMidMgr,
		estiVIDEO_NONE,
		estiH264_VIDEO,
		remoteSipVersion
		),
    m_keyFrameTimer (nKEYFRAME_RE_REQUEST_INTERVAL, this),
	m_flowControl (maxRate),
    m_flowControlTimer (TMMBR_INTERVAL_MS, this),
	m_autoSpeedMode (autoSpeedMode),
	m_nackTimer (NACK_INTERVAL_MS, this),
    m_jitterTimer (JITTER_TIMER_MS, this)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::CstiVideoPlayback);

#ifdef ercDebugPacketQueue
	poPacketQueue = &m_oPacketQueue;
#endif

	// Connect the timers to some handlers
	m_signalConnections.push_back (m_keyFrameTimer.timeoutSignal.Connect (
			std::bind(&CstiVideoPlayback::EventKeyframeTimer, this)));

	m_signalConnections.push_back (m_flowControlTimer.timeoutSignal.Connect (
			std::bind(&CstiVideoPlayback::EventTmmbrFlowControlTimer, this)));

	m_signalConnections.push_back (m_nackTimer.timeoutSignal.Connect (
			std::bind (&CstiVideoPlayback::EventNackTimer, this)));

	m_signalConnections.push_back (m_jitterTimer.timeoutSignal.Connect (
			std::bind (&CstiVideoPlayback::EventJitterCalculate, this)));
}


int CstiVideoPlayback::ClockRateGet() const
{
	return GetVideoClockRate();
}

/*!
 * \brief Initializes and starts the VideoPlayback data task
 */
stiHResult CstiVideoPlayback::Initialize(
	const TVideoPayloadMap &payloadMap)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::Initialize);

	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
				   stiTrace ("CstiVideoPlayback::Initialize\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = MediaPlaybackType::Initialize (payloadMap);
	stiTESTRESULT ();

#ifdef stiENABLE_MULTIPLE_VIDEO_OUTPUT
	m_pVideoOutput = IstiVideoOutput::instanceCreate(m_callIndex);
#else
	m_pVideoOutput = IstiVideoOutput::InstanceGet();
#endif

	// Place empty video frame objects into the frame queue.
	for (int i = 0; i < nMAX_FRAME_BUFFERS; i++)
	{
		// TODO:  Use RAII for video frames instead of new
		m_oFrameQueue.Put (new CstiVideoFrame);
	}

	m_signalConnections.push_back (m_playbackRead.dataAvailableSignal.Connect (
		[this]{
			PostEvent (std::bind(&CstiVideoPlayback::EventDataAvailable, this));
		}));

STI_BAIL:
	return hResult;
}

void CstiVideoPlayback::Uninitialize()
{

#ifdef stiENABLE_MULTIPLE_VIDEO_OUTPUT
	if (m_pVideoOutput)
	{
		m_pVideoOutput->instanceRelease ();
		m_pVideoOutput = NULL;
	}
#endif
	//
	// Now loop through all of the frame queue frames
	// and destroy each of the frame objects.
	//
	while (m_oFrameQueue.CountGet () > 0)
	{
		CstiVideoFrame *pFrame = m_oFrameQueue.Get ();
		delete pFrame;
	}
	
	VideoPacketQueueEmpty ();

	MediaPlaybackType::Uninitialize();
}


/*!
 * \brief Shuts down the playback and playbackread event loops
 *        and performs any cleanup
 * \return stiHResult
 */
stiHResult CstiVideoPlayback::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::Shutdown);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Start the shutdown process for playback read.
	m_playbackRead.Shutdown ();

	m_keyFrameTimer.stop ();

	hResult = MediaPlaybackType::Shutdown();

	return hResult;
}


/*!
* \brief Accept the offered channel.  Cause it to be setup.
*/
stiHResult CstiVideoPlayback::Answer (
	bool startKeepAlives)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::Answer);
	auto hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	auto negotiatedCodec = NegotiatedCodecGet ();

	stiTESTCOND (m_session->rtpGet(), stiRESULT_ERROR);

	// Set the receive buffer size (to avoid buffer overflow)
	RvRtpSetReceiveBufferSize(m_session->rtpGet(), stiSOCK_RECV_BUFF_SIZE);

	// TODO:  This block is seems really fishy to me...
	// For H.263 and H.264 video, the following settings apply
	if (estiH263_VIDEO == negotiatedCodec || estiH263_1998_VIDEO == negotiatedCodec ||
		estiH264_VIDEO == negotiatedCodec || estiH265_VIDEO == negotiatedCodec)
	{
		// TODO: Determine MaxBitRate based on SDP
		int maxBitRate = 1536 * 1000;
		if (maxBitRate < MaxChannelRateGet ())
		{
			MaxChannelRateSet (maxBitRate);
		}
	}

	hResult = MediaPlaybackType::Answer(startKeepAlives);

STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::KeyFrameReceived
//
// Description: Handles a key frame received notification.
//
// Abstract:
//	This function is called in response to a key frame being received by the
//	hardware. If a key frame timer is on, it will be shut off and the key frame
//	status will be changed to indicate we are no longer waiting for one.
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiVideoPlayback::KeyframeReceived (uint32_t un32Timestamp)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::KeyFrameReceived);

	EstiResult eResult = estiOK;

	stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
		stiTrace ("<VP::KeyFrameReceived> Timestamp %u.\n", un32Timestamp);
	);

	// Clear the key frame request status
	m_bKeyFrameRequested = estiFALSE;

	m_keyFrameTimer.stop ();

	// If the timestamp of this keyframe is before the needed one, request another
	if (un32Timestamp && un32Timestamp < m_un32KeyframeNeededForTimestamp)
	{
		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
				stiTrace ("<VP::KeyframeReceived> Received keyframe BUT we have need for another after timestamp %u\n", m_un32KeyframeNeededForTimestamp);
		);
		KeyframeRequest (m_un32KeyframeNeededForTimestamp);
	}

	StatsLock ();
	
	// Keep track of the number of keyframes received
	m_Stats.totalKeyframesReceived++;
	m_Stats.keyframesReceived++;

	// If we were waiting on this, store the timing stats
	if (m_Stats.un32KeyframeRequestedBeginTick)
	{
		uint32_t un32DiffTicks = stiOSTickGet () - m_Stats.un32KeyframeRequestedBeginTick;

		// Update the min/max values as is appropriate.
		if (m_Stats.un32KeyframeMinWait > un32DiffTicks || m_Stats.un32KeyframeMinWait == 0)
		{
			m_Stats.un32KeyframeMinWait = un32DiffTicks;
		}

		if (m_Stats.un32KeyframeMaxWait < un32DiffTicks)
		{
			m_Stats.un32KeyframeMaxWait = un32DiffTicks;
		}

		// Add to the total ticks spent waiting on keyframes
		m_Stats.un32KeyframeTotalTicksWaited += un32DiffTicks;

		// Reset the begin tick to show we aren't waiting on one.
		m_Stats.un32KeyframeRequestedBeginTick = 0;
	}

	StatsUnlock ();
	
	return (eResult);
} // end CstiVideoPlayback::KeyFrameReceived


 /*!
  * \brief Handler for when a request is received to change playback size from video output
  */
void CstiVideoPlayback::EventVideoPlaybackSizeChanged (
	uint32_t width,
	uint32_t height)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::EventVideoPlaybackSizeChanged);

	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
		stiTrace ("VP:: EventVideoPlaybackSizeChanged\n");
	);

	m_unLastKnownWidth = width;
	m_unLastKnownHeight = height;
}

/*!
 * \brief Changes the audio mode in the driver
 *
 *	This function changes the audio mode value into audio driver
 */
stiHResult CstiVideoPlayback::SetDeviceCodec (
	EstiVideoCodec eMode)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::SetDeviceCodec);

	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
		stiTrace ("VP:: SetDeviceCodec %d\n", eMode);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiVideoCodec esmdVideoCodec = estiVIDEO_NONE;

	// Set the DSP driver codec type
	switch(eMode)
	{
		case estiVIDEO_H263:
			esmdVideoCodec = estiVIDEO_H263;
			break;
		case estiVIDEO_H264:
			esmdVideoCodec = estiVIDEO_H264;
			break;
		case estiVIDEO_H265:
			esmdVideoCodec = estiVIDEO_H265;
			break;
		default:
			stiASSERT (estiFALSE);
			//eResult = estiERROR;
			esmdVideoCodec = estiVIDEO_H264;
			break;
	}

	// If the mode different from the current setting,
	// set the new mode into driver
	if (estiVIDEO_NONE != esmdVideoCodec
		&& ((m_currentCodec != esmdVideoCodec) ||
			m_channelResumed))
	{
		m_channelResumed = false;
		
		stiHResult hResult = m_pVideoOutput->VideoPlaybackCodecSet (esmdVideoCodec);

		if(!stiIS_ERROR (hResult))
		{
			m_currentCodec = esmdVideoCodec;
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
//; VideoFrameInitialize
//
// Description: Initializes the poFrame frame object and its buffer to an empty state
//
// Abstract:
//
// Returns:
//
void CstiVideoPlayback::VideoFrameInitialize (
	CstiVideoFrame *poFrame,
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	IstiVideoPlaybackFrame *pFrameBuffer = poFrame->m_pPlaybackFrame;
	
	pFrameBuffer->DataSizeSet (0);
	
	poFrame->unZeroBitsAtStart = 0;
	poFrame->unZeroBitsAtEnd = 0;

	poFrame->m_un32TimeStamp = packet->RTPParamGet ()->timestamp;
	
	poFrame->m_bKeyframe = false;

	//
	// Make sure that we clear out any half-finished fragmented NAL unit
	// that we have been building.
	//
	m_bStartedFNAL = false;
}

/*!
 * \brief When a video packet arrives with a different payload than the previous packet,
 *        we may need to do some teardown/initialization work to prepare the decoder, as
 *        well as check to see what rtcp feedback mechanisms are supported for that payload
 *        and set the appropriate feedback callbacks
 */
bool CstiVideoPlayback::payloadChange (
	const TVideoPayloadMap &payloadMap,
	int8_t payload)
{
	EstiVideoCodec newCodec = estiVIDEO_H264;

#ifndef sciRTP_FROM_FILE
	auto itr = payloadMap.find (payload);
	if (itr == payloadMap.end ())
	{
		stiASSERTMSG (false, "Unknown payload id: %d", payload);
		return false;
	}

	newCodec = itr->second.eCodec;

	// Which rtcp feedback mechanisms, if any, are supported under the new payload?
	m_rtcpFeedbackTmmbrEnabled = false;
	m_rtcpFeedbackFirEnabled = false;
	m_rtcpFeedbackPliEnabled = false;
	m_rtcpFeedbackNackRtxEnabled = false;

	for (const auto &fbType : itr->second.rtcpFeedbackTypes)
	{
		switch (fbType)
		{
			case RtcpFeedbackType::TMMBR:
				m_rtcpFeedbackTmmbrEnabled = m_session->rtcpFeedbackTmmbrEnabledGet ();
				break;
			case RtcpFeedbackType::FIR:
				m_rtcpFeedbackFirEnabled = m_session->rtcpFeedbackFirEnabledGet ();
				break;
			case RtcpFeedbackType::PLI:
				m_rtcpFeedbackPliEnabled = m_session->rtcpFeedbackPliEnabledGet ();
				break;
			case RtcpFeedbackType::NACKRTX:
				m_rtcpFeedbackNackRtxEnabled = m_session->rtcpFeedbackNackRtxEnabledGet ();
				break;
			case RtcpFeedbackType::UNKNOWN:  // do nothing
				break;
		};
	}
#endif  // sciRTP_FROM_FILE

	bool restartRequired = payloadChangeRequiresRestart(itr->second, payloadMap, m_nCurrentRTPPayload);

	m_nCurrentRTPPayload = payload;

	if (m_bVideoPlaybackStarted && restartRequired)
	{
		m_pVideoOutput->VideoPlaybackStop ();
		m_bVideoPlaybackStarted = false;
	}

	// NOTE: ok to set this with the same codec type as currently set (ignored)
	auto result = SetDeviceCodec (newCodec);

	if (!m_bVideoPlaybackStarted)
	{
		m_pVideoOutput->VideoPlaybackStart ();
		m_bVideoPlaybackStarted = true;
		m_bKeyframeSentToPlatform = false;
	}

	flowControlReset ();

	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
		stiTrace ("CstiVideoPlayback::payloadChanged Selected codec: %d, Payload: %d\n", newCodec, payload);
	);

	return result == stiRESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::PacketProcess
//
// Description: Assembles packets into frames, then decodes the frames.
//
// Abstract:
//	Assembles packets into frames, then sends the frames to the decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiVideoPlayback::PacketProcess (
	std::shared_ptr<CstiVideoPacket> packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::PacketProcess);

	EstiResult eResult = estiOK;

	// Get the RTPParam pointer to our new packet
	RvRtpParam * pstRTPParam = packet->RTPParamGet ();

	// Check to see if the codec is different than what we thought was negotiated.
	if (m_nCurrentRTPPayload != pstRTPParam->payload)
	{
		stiDEBUG_TOOL (2 & g_stiVideoPlaybackPacketDebug,
			stiTrace ("CstiVideoPlayback::PacketProcess Payload (%d) is not what we were expecting (%d).  Changing payload...\n", pstRTPParam->payload, m_nCurrentRTPPayload);
		);

		// Change the payload type and perform any re-initialization that needs to take place
		if (!payloadChange (m_PayloadMap, pstRTPParam->payload))
		{
			stiDEBUG_TOOL (2 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("CstiVideoPlayback::PacketProcess PacketProcess - SQ# %d is NOT a valid video type. "
				"Ignoring it\n",
				packet->m_un32ExtSequenceNumber);
			);

			// It's not a valid video packet, ignore this one by marking it NULL
			packet = nullptr;
		}
	}


	if (packet)
	{
		stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
					   stiTrace ("CstiVideoPlayback::PacketProcess\n"
								"\ttimestamp = %u\n"
								"\tmarker = %d\n"
								//"\tpayload = %d\n"
								//"\tsSrc = 0x%x\n"
								"\tsequenceNumber = %d\n"
								//"\tsByte = %d\n"
								"\tlen = %d\n",
								(packet->RTPParamGet ())->timestamp,
								(packet->RTPParamGet ())->marker,
								//stRTPParam.payload,
								//stRTPParam.sSrc,
								packet->m_un32ExtSequenceNumber,
								//stRTPParam.sByte,
								(packet->RTPParamGet ())->len);
							  );

		switch (m_currentCodec)
		{
			case estiVIDEO_H263:
			{
				stiHResult hResult = H263PacketProcess (packet);

				if (stiIS_ERROR (hResult))
				{
					eResult = estiERROR;
				}

				break;
			}

			case estiVIDEO_H264:
			{
				bool bFrameComplete = estiFALSE;
				
				// Are their timestamps the same (meaning that they belong to the
				// same frame)?
				if (m_poFrame->m_un32TimeStamp != pstRTPParam->timestamp)
				{
					stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
						stiTrace ("VP:: SQ# %d does NOT belong to the frame with time stamp %d"
							" --- Missing packet\n",
							pstRTPParam->sequenceNumber,
							m_poFrame->m_un32TimeStamp);
					);

					VideoFrameInitialize (m_poFrame, packet);
				} // end else

				eResult = H264PacketProcess (packet);

				if (pstRTPParam->marker)
				{
					// Yes! We have a complete frame.
					bFrameComplete = estiTRUE;
				} // end if

				// Do we still have a frame we are working on?
				if (nullptr != m_poFrame)
				{
					// Yes! Is the current frame complete?
					if (bFrameComplete)
					{
						m_bFrameComplete = estiTRUE;
						
#if 0
						static uint32_t un32FrameCount = 0;
						static FILE *pFile = NULL;
						
						if (un32FrameCount == 0)
						{
							pFile = fopen ("/Frame.264", "wb");
						}
							
						if (pFile)
						{
							IstiVideoPlaybackFrame *pFrameBuffer = m_poFrame->m_pPlaybackFrame;
							
							fwrite (pFrameBuffer->BufferGet (), pFrameBuffer->DataSizeGet (),
									sizeof (uint8_t), pFile);
						}
							
						if (un32FrameCount > 500)
						{
							if (pFile)
							{
								fclose (pFile);
								pFile = NULL;
							}
						}
						
						++un32FrameCount;
#endif
					}
				} // end if
				
				break;
			}
			case estiVIDEO_H265:
			{
				bool bFrameComplete = estiFALSE;
				
				// Are their timestamps the same (meaning that they belong to the
				// same frame)?
				if (m_poFrame->m_un32TimeStamp != pstRTPParam->timestamp)
				{
					stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
								   stiTrace ("VP:: SQ# %d does NOT belong to the frame with time stamp %d"
											 " --- Missing packet\n",
											 pstRTPParam->sequenceNumber,
											 m_poFrame->m_un32TimeStamp);
								   );
					
					VideoFrameInitialize (m_poFrame, packet);
				} // end else

				eResult = H265PacketProcess (packet);

				if (pstRTPParam->marker)
				{
					// Yes! We have a complete frame.
					bFrameComplete = estiTRUE;
				} // end if
				
				// Do we still have a frame we are working on?
				if (nullptr != m_poFrame)
				{
					// Yes! Is the current frame complete?
					if (bFrameComplete)
					{
						m_bFrameComplete = estiTRUE;
						
#if 0
						static uint32_t un32FrameCount = 0;
						static FILE *pFile = NULL;
						
						if (un32FrameCount == 0)
						{
							pFile = fopen ("/Frame.265", "wb");
						}
						
						if (pFile)
						{
							IstiVideoPlaybackFrame *pFrameBuffer = m_poFrame->m_pPlaybackFrame;
							
							fwrite (pFrameBuffer->BufferGet (), pFrameBuffer->DataSizeGet (),
									sizeof (uint8_t), pFile);
						}
						
						if (un32FrameCount > 500)
						{
							if (pFile)
							{
								fclose (pFile);
								pFile = NULL;
							}
						}
						
						++un32FrameCount;
#endif
					}
				} // end if
				
				break;
			}
			default:
				stiASSERT (estiFALSE);
				eResult = estiERROR;
				break;
		} // end of switch
	} // end if

	return (eResult);
} // end CstiVideoPlayback::PacketProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::H263PacketProcess
//
// Description: Assembles packets into H263 frames, then decodes the frames.
//
// Abstract:
//	Assembles packets into frames, then sends the frames to the decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
stiHResult CstiVideoPlayback::H263PacketProcess (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::H263PacketProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiBool bFrameComplete = estiFALSE;

	// Get the RTPParam pointer to our new packet
	RvRtpParam * pstRTPParam = packet->RTPParamGet ();

	// Is this the first packet in a frame?
	if (m_poFrame->m_pPlaybackFrame->DataSizeGet() == 0 && PacketCheckPictureStartCode (packet))
	{
		vpe::VideoFrame * pstVideoPacket = packet->m_frame;

		IstiVideoPlaybackFrame *pFrameBuffer = m_poFrame->m_pPlaybackFrame;

		// Check to see if this is a keyframe?  Check bit 1 of the 5th byte.  If 0, it is a keyframe.
		if ((pstVideoPacket->buffer[4] & 0x02) == 0)
		{
			m_poFrame->m_bKeyframe = true;
		}

		m_poFrame->unZeroBitsAtStart = packet->unZeroBitsAtStart;
		m_poFrame->unZeroBitsAtEnd = packet->unZeroBitsAtEnd;

		if (pFrameBuffer->BufferSizeGet () < packet->unPacketSizeInBytes)
		{
			hResult = pFrameBuffer->BufferSizeSet(packet->unPacketSizeInBytes);
			stiTESTRESULTMSG_FORCELOG ("H263PacketProcess: Failed to increase buffer size to %u bytes.", packet->unPacketSizeInBytes);
		}

		memcpy (pFrameBuffer->BufferGet (), pstVideoPacket->buffer,
				packet->unPacketSizeInBytes);
		pFrameBuffer->DataSizeSet (packet->unPacketSizeInBytes);

#ifdef stiDEBUGGING_TOOLS
		m_nPacketsPerFrame = 1;
#endif

		// Is this packet a complete frame?
		if (pstRTPParam->marker)
		{
			// Yes! We have a complete frame.
			bFrameComplete = estiTRUE;
		} // end if
	} // end if
	else
	{
		uint32_t un32CurrentTimestamp = m_poFrame->m_un32TimeStamp;

		// Are their timestamps the same (meaning that they belong to the
		// same frame)?
		if (m_poFrame->m_un32TimeStamp == pstRTPParam->timestamp)
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("CstiVideoPlayback::H263PacketProcess SQ# %d belongs to our frame. "
				"Merging with %d...\n",
				packet->m_un32ExtSequenceNumber,
				m_poFrame->m_un32TimeStamp);
			);


			// Yes! Merge the new packet into the one we already have.
			EstiPacketResult eMergeResult = PacketMerge (m_poFrame, packet);

			// Was the merge successful?
			if (eMergeResult == ePACKET_OK)
			{
#ifdef stiDEBUGGING_TOOLS
				m_nPacketsPerFrame ++;
#endif

				// Yes! Is the new packet the end of the frame?
				if (pstRTPParam->marker)
				{
					// Yes! We have a complete frame.
					bFrameComplete = estiTRUE;
				} // end if

				stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
					stiTrace ("CstiVideoPlayback::H263PacketProcess Done with SQ# %d. "
					"Marking as empty\n",
					packet->m_un32ExtSequenceNumber);
				);
			} // end if
			else
			{
				stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
					stiTrace ("CstiVideoPlayback::H263PacketProcess Error Merging packets!\n");
				);
#ifndef stiNO_H263_MISSING_PACKET_DETECTION
				// Indicate missing packet
				m_bMissingPacket = estiTRUE;
#endif

				// We also need to throw out the frame that we have been
				// building
				FrameClear ();

				// We need a keyframe, since the frame we have currently
				// built has been dropped
				KeyframeRequest (un32CurrentTimestamp);
			} // end else
		} // end if
		else
		{
			stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("CstiVideoPlayback::H263PacketProcess SQ# %d does NOT belong to frame\n",
				packet->m_un32ExtSequenceNumber);
			);

			// No! We missed some packets in our frame, so throw away the
			// current frame.
			FrameClear ();

			// We need a keyframe, since the frame we are currenting
			// built has been dropped
			KeyframeRequest (un32CurrentTimestamp);

#ifndef stiNO_H263_MISSING_PACKET_DETECTION
			// Indicate missing packet
			m_bMissingPacket= estiTRUE;
#endif
			// Now try to process the new packet again (since it might be the
			// start of a new frame)
		} // end else
	} // end else


	// Do we still have a frame we are working on?
	if (nullptr != m_poFrame)
	{
		// Yes! Is the current frame complete?
		if (bFrameComplete)
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("CstiVideoPlayback::H263PacketProcess Frame with time stamp %d is complete. "
				"Sending to driver...\n",
				m_poFrame->m_un32TimeStamp);
			);

			m_bFrameComplete = estiTRUE;

#ifndef stiNO_H263_MISSING_PACKET_DETECTION
			if (estiTRUE == m_bMissingPacket)
			{
				// We need a keyframe, since there are missing packets
				//m_bKeyFrameNeeded = estiTRUE;

				m_bMissingPacket = estiFALSE;

				stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
					stiTrace("CstiVideoPlayback::H263PacketProcess H263 end of missing packets ...\n\n");
				);
			}
#endif

		} // end if
		else
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("CstiVideoPlayback::H263PacketProcess Frame with time stamp %d not complete...\n",
					m_poFrame->m_un32TimeStamp);
			);
		} // end else
	} // end if

STI_BAIL:
	return hResult;
}


stiHResult CstiVideoPlayback::NALUnitHeaderAdd (
	CstiVideoFrame *poFrame,
	uint32_t un32Size)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	IstiVideoPlaybackFrame *pFrameBuffer = poFrame->m_pPlaybackFrame;
	uint32_t un32DesiredSize = pFrameBuffer->DataSizeGet() +
		 sizeof(uint32_t) + // the size or byte stream header
		 un32Size; // the payload
	
	if (pFrameBuffer->BufferSizeGet() < un32DesiredSize)
	{
		hResult = pFrameBuffer->BufferSizeSet(un32DesiredSize);

		if (stiIS_ERROR (hResult))
		{
			stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
						"NALUnitHeaderAdd: Failed to increase buffer size to %u bytes.", un32DesiredSize);
		}
	}

	{
		uint8_t *pDst = pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet ();

		// Assign the correct buffer pointer and size back to the packet
		if (m_pVideoOutput->H264FrameFormatGet () == eH264FrameFormatByteStream)
		{
			pDst[0] = 0;
			pDst[1] = 0;
			pDst[2] = 0;
			pDst[3] = 1;
		}
		else
		{
			pDst[3] = (un32Size & 0xFF);
			pDst[2] = ((un32Size >> 8) & 0xFF);
			pDst[1] = ((un32Size >> 16) & 0xFF);
			pDst[0] = ((un32Size >> 24) & 0xFF);
		}

		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + sizeof (uint32_t));
	}

STI_BAIL:

	return hResult;
}


#if (APPLICATION != APP_HOLDSERVER)
////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::H264PacketProcess
//
// Description: Assembles packets into H264 frames, then decodes the frames.
//
// Abstract:
//	Assembles packets into frames, then sends the frames to the decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiVideoPlayback::H264PacketProcess (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::H264PacketProcess);

	EstiResult eResult = estiOK;

	// Get the vpe::VideoFrame pointer to our new packet
	vpe::VideoFrame *pstVideoPacket = packet->m_frame;
	uint8_t * pun8Buffer = pstVideoPacket->buffer;

	// Find out the H.264 packet type
	EstiH264PacketType eH264PacketType = estiH264PACKET_NONE;
	int nPacketType = 0;

	if (packet->unPacketSizeInBytes > 0)
	{
		if (estiVIDEO_H264 == m_currentCodec)
		{
			nPacketType = (pun8Buffer[0] & 0x1F);
			switch(nPacketType)
			{
				case estiH264PACKET_NAL_CODED_SLICE_OF_NON_IDR:
				case estiH264PACKET_NAL_CODED_SLICE_PARTITION_A:
				case estiH264PACKET_NAL_CODED_SLICE_PARTITION_B:
				case estiH264PACKET_NAL_CODED_SLICE_PARTITION_C:
				case estiH264PACKET_NAL_CODED_SLICE_OF_IDR:
				case estiH264PACKET_NAL_SEI:
				case estiH264PACKET_NAL_SPS:
				case estiH264PACKET_NAL_PPS:
				case estiH264PACKET_NAL_AUD:
				case estiH264PACKET_NAL_END_OF_SEQUENCE:
				case estiH264PACKET_NAL_END_OF_STREAM:
				case estiH264PACKET_NAL_FILLER_DATA:
				case estiH264PACKET_STAPA:
				case estiH264PACKET_FUA:

					eH264PacketType = (EstiH264PacketType)nPacketType;

					break;

				case estiH264PACKET_NONE:
				case estiH264PACKET_STAPB:
				case estiH264PACKET_MTAP16:
				case estiH264PACKET_MTAP24:
				case estiH264PACKET_FUB:
					stiTrace ("VP:: nPacketType = %s, Size = %u\n",
							estiH264PACKET_NONE == nPacketType ? "estiH264PACKET_NONE" :
							estiH264PACKET_STAPB == nPacketType ? "estiH264PACKET_STAPB" :
							estiH264PACKET_MTAP16 == nPacketType ? "estiH264PACKET_MTAP16" :
							estiH264PACKET_MTAP24 == nPacketType ? "estiH264PACKET_MTAP24" :
							estiH264PACKET_FUB == nPacketType ? "estiH264PACKET_FUB" : "???", (unsigned)packet->unPacketSizeInBytes);

					// non-support packet types
					stiASSERT (estiFALSE);

					break;

				//
				// Unsupported packet types
				//
				case estiH264PACKET_NAL_SPS_EXTENSION:
				case estiH264PACKET_NAL_PREFIX_NALU:
				case estiH264PACKET_NAL_SUBSET_SPS:
				case estiH264PACKET_NAL_RESERVED16:
				case estiH264PACKET_NAL_RESERVED17:
				case estiH264PACKET_NAL_RESERVED18:
				case estiH264PACKET_NAL_CODED_SLICE_AUX:
				case estiH264PACKET_NAL_CODED_SLICE_EXT:
				case estiH264PACKET_NAL_CODED_SLICE_EXT_DEPTH_VIEW:
				case estiH264PACKET_NAL_RESERVED22:
				case estiH264PACKET_NAL_RESERVED23:
					
					break;
			} // end switch

		}
	}

#ifndef WIN32 //TODO: Fix stiTrace for Windows
	stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
		stiTrace ("VP:: SQ# %u nPacketType = %d\n",
				packet->m_un32ExtSequenceNumber, nPacketType);
	);
#endif

	// Process this packet only if it's a supported H.264 packet type
	if(estiH264PACKET_NONE != eH264PacketType)
	{
		EstiPacketResult eMergeResult = ePACKET_OK;

		IstiVideoPlaybackFrame *pFrameBuffer = m_poFrame->m_pPlaybackFrame;
		
		if (estiH264PACKET_STAPA == eH264PacketType)
		{
			//
			// If we started assembling a fragmented NAL unit then
			// throw it away.
			//
			if (m_bStartedFNAL)
			{
				pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
				
				m_bStartedFNAL = false;
			}
			
			m_un32NALUnitStart = pFrameBuffer->DataSizeGet ();
			
			eMergeResult = H264STAPAOrH265APToVideoFrame (m_poFrame, packet, un8STAPA_TYPE_SIZE);
		}
		else if (estiH264PACKET_FUA == eH264PacketType)
		{
			eMergeResult = H264FUAToVideoFrame (m_poFrame, packet);
		}
		else
		{
			//
			// If we started assembling a fragmented NAL unit then
			// throw it away.
			//
			if (m_bStartedFNAL)
			{
				pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
				
				m_bStartedFNAL = false;
			}
			
			m_un32NALUnitStart = pFrameBuffer->DataSizeGet ();
			
			stiHResult hResult = NALUnitHeaderAdd (m_poFrame, packet->unPacketSizeInBytes);

			if (!stiIS_ERROR (hResult))
			{
				// Merge the new packet into the one we already have.
				eMergeResult = PacketMerge (m_poFrame, packet);
			}
			else
			{
				eMergeResult = ePACKET_ERROR;
			}

			// Check to see if this is a keyframe.
			if (eH264PacketType == estiH264PACKET_NAL_CODED_SLICE_OF_IDR)
			{
				m_poFrame->m_bKeyframe = true;
			}
		}

		// Was the merge successful?
		if (eMergeResult == ePACKET_OK)
		{
#ifdef stiDEBUGGING_TOOLS
			++m_nPacketsPerFrame;
#endif
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("VP:: Success Merging with SQ# %d.\n",
				packet->m_un32ExtSequenceNumber);
			);
		} // end if
		else
		{
			stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("VP:: Error Merging with SQ# %d. "
					" --- Missing packet\n",
					packet->m_un32ExtSequenceNumber);
			);

			if (m_bStartedFNAL)
			{
				pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
				
				m_bStartedFNAL = false;
			}
			
#ifndef stiNO_H264_MISSING_PACKET_DETECTION
			// Indicate missing packet
			m_bMissingPacket = estiTRUE;
#endif
		} // end else
	} //end of if(estiH264PACKET_NONE != eH264PacketType)

	return eResult;
} // end CstiVideoPlayback::H264PacketProcess
#else
////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::H264PacketProcess

//
// Description: Assembles packets into H264 frames, then decodes the frames.
//
// Abstract:
//	Assembles packets into frames, then sends the frames to the decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiVideoPlayback::H264PacketProcess (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::H264PacketProcess);
	return estiOK;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::H265PacketProcess
//
// Description: Assembles packets into H265 frames, then decodes the frames.
//
// Abstract:
//	Assembles packets into frames, then sends the frames to the decoder.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiVideoPlayback::H265PacketProcess (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::H265PacketProcess);
	
	EstiResult eResult = estiOK;
	
	int tid /*, lid*/;
	
	// Get the vpe::VideoFrame pointer to our new packet
	vpe::VideoFrame *pstVideoPacket = packet->m_frame;
	uint8_t * pun8Buffer = pstVideoPacket->buffer;
	
	// Find out the H.265 packet type
	EstiH265PacketType eH265PacketType = estiH265PACKET_NAL_UNIT_INVALID;
	int nPacketType = 0;
	
	if (packet->unPacketSizeInBytes > 0)
	{
		if (estiVIDEO_H265 == m_currentCodec)
		{
			// The H265 payload header
			//
			//   0                   1
			//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
			//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//  |F|   Type    |  LayerId  | TID |
			//  +-------------+-----------------+
			//

			nPacketType = ((pun8Buffer[0] >> 1) & 0x3F);
			//lid = ((pun8Buffer[0] << 5) & 0x20) | ((pun8Buffer[1] >> 3) & 0x1f);
			tid = pun8Buffer[1] & 0x07;
			eH265PacketType = (EstiH265PacketType)nPacketType;
		 
			/* sanity check for correct temporal ID */
			if (!tid) {
				stiASSERT (estiFALSE);
			}

			/* sanity check for correct NAL unit type */
			if (eH265PacketType > 50) {
				stiTrace ("VP:: nPacketType = %d, Size = %u\n", nPacketType, (unsigned)packet->unPacketSizeInBytes);
				// non-support packet types
				stiASSERT (estiFALSE);
			}
			
//			switch(nPacketType)
//			{
//				case estiH264PACKET_NAL_CODED_SLICE_OF_NON_IDR:
//				case estiH264PACKET_NAL_CODED_SLICE_PARTITION_A:
//				case estiH264PACKET_NAL_CODED_SLICE_PARTITION_B:
//				case estiH264PACKET_NAL_CODED_SLICE_PARTITION_C:
//				case estiH264PACKET_NAL_CODED_SLICE_OF_IDR:
//				case estiH264PACKET_NAL_SEI:
//				case estiH264PACKET_NAL_SPS:
//				case estiH264PACKET_NAL_PPS:
//				case estiH264PACKET_NAL_AUD:
//				case estiH264PACKET_NAL_END_OF_SEQUENCE:
//				case estiH264PACKET_NAL_END_OF_STREAM:
//				case estiH264PACKET_NAL_FILLER_DATA:
//				case estiH264PACKET_STAPA:
//				case estiH264PACKET_FUA:
//					
//					eH265PacketType = (EstiH265PacketType)nPacketType;
//					
//					break;
//					
//				case estiH264PACKET_NONE:
//				case estiH264PACKET_STAPB:
//				case estiH264PACKET_MTAP16:
//				case estiH264PACKET_MTAP24:
//				case estiH264PACKET_FUB:
//					stiTrace ("VP:: nPacketType = %s, Size = %u\n",
//							  estiH264PACKET_NONE == nPacketType ? "estiH264PACKET_NONE" :
//							  estiH264PACKET_STAPB == nPacketType ? "estiH264PACKET_STAPB" :
//							  estiH264PACKET_MTAP16 == nPacketType ? "estiH264PACKET_MTAP16" :
//							  estiH264PACKET_MTAP24 == nPacketType ? "estiH264PACKET_MTAP24" :
//							  estiH264PACKET_FUB == nPacketType ? "estiH264PACKET_FUB" : "???", (unsigned)packet->unPacketSizeInBytes);
//
//					// non-support packet types
//					stiASSERT (estiFALSE);
//
//					break;
//					
//					//
//					// Unsupported packet types
//					//
//				case estiH264PACKET_NAL_SPS_EXTENSION:
//				case estiH264PACKET_NAL_PREFIX_NALU:
//				case estiH264PACKET_NAL_SUBSET_SPS:
//				case estiH264PACKET_NAL_RESERVED16:
//				case estiH264PACKET_NAL_RESERVED17:
//				case estiH264PACKET_NAL_RESERVED18:
//				case estiH264PACKET_NAL_CODED_SLICE_AUX:
//				case estiH264PACKET_NAL_CODED_SLICE_EXT:
//				case estiH264PACKET_NAL_CODED_SLICE_EXT_DEPTH_VIEW:
//				case estiH264PACKET_NAL_RESERVED22:
//				case estiH264PACKET_NAL_RESERVED23:
//					
//					break;
//			} // end switch
			
		}
	}
	
#ifndef WIN32 //TODO: Fix stiTrace for Windows
	stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				   stiTrace ("VP:: SQ# %u nPacketType = %d\n",
							 packet->m_un32ExtSequenceNumber, nPacketType);
				   );
#endif
	
	// Process this packet only if it's a supported H.265 packet type
	if(estiH265PACKET_NAL_UNIT_INVALID != eH265PacketType)
	{
		EstiPacketResult eMergeResult = ePACKET_OK;
		
		IstiVideoPlaybackFrame *pFrameBuffer = m_poFrame->m_pPlaybackFrame;
		
		if (estiH265PACKET_AP == eH265PacketType)
		{
			//
			// If we started assembling a fragmented NAL unit then
			// throw it away.
			//
			if (m_bStartedFNAL)
			{
				pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
				
				m_bStartedFNAL = false;
			}
			
			m_un32NALUnitStart = pFrameBuffer->DataSizeGet ();
			
			eMergeResult = H264STAPAOrH265APToVideoFrame (m_poFrame, packet, un8RTP_H265_PAYLOAD_HEADER_SIZE);
		}
		else if (estiH265PACKET_FU == eH265PacketType)
		{
			eMergeResult = H265FUToVideoFrame (m_poFrame, packet);
		}
		else
		{
			//
			// If we started assembling a fragmented NAL unit then
			// throw it away.
			//
			if (m_bStartedFNAL)
			{
				pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
				
				m_bStartedFNAL = false;
			}
			
			m_un32NALUnitStart = pFrameBuffer->DataSizeGet ();
			
			stiHResult hResult = NALUnitHeaderAdd (m_poFrame, packet->unPacketSizeInBytes);
			
			if (!stiIS_ERROR (hResult))
			{
				// Merge the new packet into the one we already have.
				eMergeResult = PacketMerge (m_poFrame, packet);
			}
			else
			{
				eMergeResult = ePACKET_ERROR;
			}
			
			// Check to see if this is a keyframe.
			if (eH265PacketType == estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_LP ||
				eH265PacketType == estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_RADL ||
				eH265PacketType == estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_N_LP ||
				eH265PacketType == estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
				eH265PacketType == estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_N_LP ||
				eH265PacketType == estiH265PACKET_NAL_UNIT_CODED_SLICE_CRA)
			{
				m_poFrame->m_bKeyframe = true;
			}
		}
		
		// Was the merge successful?
		if (eMergeResult == ePACKET_OK)
		{
#ifdef stiDEBUGGING_TOOLS
			++m_nPacketsPerFrame;
#endif
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
						   stiTrace ("VP:: Success Merging with SQ# %d.\n",
									 packet->m_un32ExtSequenceNumber);
						   );
		} // end if
		else
		{
			stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
						   stiTrace ("VP:: Error Merging with SQ# %d. "
									 " --- Missing packet\n",
									 packet->m_un32ExtSequenceNumber);
						   );
			
			if (m_bStartedFNAL)
			{
				pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
				
				m_bStartedFNAL = false;
			}
			
#ifndef stiNO_H264_MISSING_PACKET_DETECTION
			// Indicate missing packet
			m_bMissingPacket = estiTRUE;
#endif
		} // end else
	} //end of if(estiH264PACKET_NONE != eH264PacketType)
	
	return eResult;
} // end CstiVideoPlayback::H264PacketProcess

#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::FNALPacketClear
//
// Description: Clears the fragmented unit packet stores incomplete frames
//
// Abstract:
//	Thows out any incomplete fragmented unit and recovers the packet object.
//
// Returns:
//	None.
//
void CstiVideoPlayback::FNALPacketClear ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::FNALPacketClear);

	stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
		stiTrace ("CstiVideoPlayback::FNALPacketClear\n");
	);

	m_poFNALPacket = NULL;
} // end CstiVideoPlayback::FNALPacketClear
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::FrameClear
//
// Description: Clears the any incomplete frame
//
// Abstract:
//	Thows out any incomplete frames and recovers the packet object.
//
// Returns:
//	None.
//
void CstiVideoPlayback::FrameClear ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::FrameClear);

	// Do we have a packet pointer for a frame that we have been building?
	if (nullptr != m_poFrame)
	{
		stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
			stiTrace ("CstiVideoPlayback::FrameClear %d is dropping ...\n",
				m_poFrame->m_un32TimeStamp);
			m_nPacketsPerFrame = 0;
		);

		// Put the packet back into the Frame queue
		VideoFramePut (m_poFrame);
		m_poFrame = nullptr;

		//
		// Make sure that we clear out any half-finished fragmented NAL unit
		// that we have been building.
		//
		m_bStartedFNAL = false;

		// Indicate that the frame is not complete.
		m_bFrameComplete = estiFALSE;
	} // end if

} // end CstiVideoPlayback::FrameClear


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::SyncQueueClear
//
// Description: Clears the sync queue
//
// Abstract:
//	Returns any frame objects to the video frame queue.
//
// Returns:
//	None.
//
void CstiVideoPlayback::SyncQueueClear ()
{
	//
	// Return the pending playback frame and
	// all frames in the sync queue back to the
	// frame queue.
	//
	if (m_poPendingPlaybackFrame)
	{
		VideoFramePut(m_poPendingPlaybackFrame);
		m_poPendingPlaybackFrame = nullptr;
	}
	
	while (m_oSyncQueue.CountGet () > 0)
	{
		CstiVideoFrame *pFrame = m_oSyncQueue.Get ();
		
		VideoFramePut (pFrame);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::PacketMerge
//

// Desc: Merge two packets together
//
// Abstract:
//	Merges a source packet into a destination packet for building a frame
//
// Returns:
//	ePACKET_OK - merge successful
//	ePACKET_ERROR - error logged
//	ePACKET_CONTINUE - error with merge
//
EstiPacketResult CstiVideoPlayback::PacketMerge (
	CstiVideoFrame *poFrame,	// Destination frame
	const std::shared_ptr<CstiVideoPacket> &srcPacket)	// Source Packet
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::PacketMerge);

	EstiPacketResult	eMergeResult = ePACKET_OK;
	stiHResult hResult = stiRESULT_SUCCESS;

	stiASSERT (nullptr != poFrame);
	stiASSERT (nullptr != srcPacket);

	// Were we passed valid pointers?
	if (nullptr != poFrame && nullptr != srcPacket)
	{
		// Yes! Gather the information about the packets
		//RvRtpParam *			pstRTPParamSrc = poSrcPacket->RTPParamGet ();
		vpe::VideoFrame *pstSrcVideoPacket = srcPacket->m_frame;
		IstiVideoPlaybackFrame *pFrameBuffer = poFrame->m_pPlaybackFrame;
		uint8_t *pun8SrcBuffer = pstSrcVideoPacket->buffer;
		uint8_t *pun8DestBuffer = pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet ();
		uint8_t *pun8SourcePayload = pun8SrcBuffer;
		int nBlended = 0; // Modifies length count when merging

		// If the end bit is greater than 0, mask the last byte of the
		// destination packet with the first byte of the source packet, based on
		// the settings of eBit and sBit.
		if (poFrame->unZeroBitsAtEnd > 0)
		{
			// Are the Destination start bits and Source end bits compliments
			// of each other?

			if (8 != (srcPacket->unZeroBitsAtStart +
				poFrame->unZeroBitsAtEnd))
			{
				stiDEBUG_TOOL (1/*g_stiVideoPlaybackPacketDebug*/,
							   stiTrace ("CstiVideoPlayback::PacketMerge - End and Start bits don't match!\n"
					"\tEnd = %d, Start = %d\n",
					poFrame->unZeroBitsAtEnd,
					srcPacket->unZeroBitsAtStart);
				);

				// No! The start and stop bits do not match. Bail out since
				// these cannot be merged.
				eMergeResult = ePACKET_CONTINUE;
			} // end if
			else
			{
				// Yes! The start and end bits match, so get the last byte of
				// the destination buffer
				auto  pLastByte = (int8_t *) &pun8DestBuffer[-1];

				// Is the last byte of the destination packet different than the
				// first byte of the source buffer?
				if (*pLastByte != *pun8SrcBuffer) // first byte of payload data
				{
					// Yes! Mask the destination bits
					uint8_t un8DestBits =
						*pLastByte & un8EndMask[poFrame->unZeroBitsAtEnd];

					// Mask the source bits

					uint8_t un8SourceBits =
						(*pun8SrcBuffer & un8StartMask[srcPacket->unZeroBitsAtStart]);

					// Now combine the masked bits into the last byte of the
					// destination buffer

					*pLastByte = un8DestBits | un8SourceBits;
				} // end if

				// Increment the payload offset to ignore the first byte in
				// the stream.
				pun8SourcePayload++;

				// Remember that there will be one less byte in the final size
				// because we merged two bytes into one.
				nBlended = 1;
			} // end else
		} // end if

		if (ePACKET_OK == eMergeResult)
		{
			// Is there enough room in the frame buffer for the read buffer data?
			uint32_t un32DesiredSize = srcPacket->unPacketSizeInBytes - nBlended + pFrameBuffer->DataSizeGet();
			if (un32DesiredSize > pFrameBuffer->BufferSizeGet())
			{
				hResult = pFrameBuffer->BufferSizeSet(un32DesiredSize);

				if (stiIS_ERROR (hResult))
				{
					stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
								"PacketMerge: Failed to increase buffer size to %u bytes.", un32DesiredSize);
				}

				// the buffer location may be different if memory was realloced
				pun8DestBuffer = pFrameBuffer->BufferGet() + pFrameBuffer->DataSizeGet();
			}
			
			// Yes! Copy the read buffer data to the frame buffer
			memcpy (pun8DestBuffer,// destination
					pun8SourcePayload,	 							// source
					srcPacket->unPacketSizeInBytes - nBlended);	// len

			// Add the length of data, just copied, to the frame data length
			pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet ()
										+ srcPacket->unPacketSizeInBytes - nBlended);
				
			// Save the endbit value for the last byte of the buffer
			poFrame->unZeroBitsAtEnd = srcPacket->unZeroBitsAtEnd;
		} // end if
	} // end if
	else
	{
		stiDEBUG_TOOL (1/*g_stiVideoPlaybackPacketDebug*/,
			stiTrace ("CstiVideoPlayback:: PacketMerge - Bad pointers used for merge!\n");
		);

		// Bad pointers. Return an error
		stiASSERT (estiFALSE);
		eMergeResult = ePACKET_ERROR;
	} // end else

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		eMergeResult = ePACKET_ERROR;
	}

	return (eMergeResult);
} // end CstiVideoPlayback::PacketMerge


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::PacketCheckPictureStartCode
//
// Description: Checks for a valid picture start code.
//
// Abstract:
//	This function checks the beginning of the frame for a valid picture start
//	code. This check should only be done on the start of a suspected valid
//	frame.
//
// Returns:
//	estiTRUE if the picture start code is valid, estiFALSE if not.
//
EstiBool CstiVideoPlayback::PacketCheckPictureStartCode (
	const std::shared_ptr<CstiVideoPacket> &packet) 	// Pointer to a packet
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::PacketCheckPictureStartCode);

	EstiBool bResult = estiFALSE;
	vpe::VideoFrame * pstVideoPacket = packet->m_frame;
	uint8_t * pun8Buff = pstVideoPacket->buffer;

	// Perform any stream specific processing.
	switch(m_currentCodec)
	{
		case estiVIDEO_H263:
			// Do we have a valid H.263 picture start code?
			if ( pun8Buff[0] == 0x00 &&
				 pun8Buff[1] == 0x00 &&
				 (pun8Buff[2] & 0xf0) >= 0x80 && // picture start code is
				 (pun8Buff[2] & 0xff) <= 0x83)	// 0x80 to 0x83

			{
				// Yes! Set the return to true.
				bResult = estiTRUE;
			} // end if
			break;
		default:
			stiASSERT (estiFALSE);
			bResult = estiFALSE;
			break;
	} // end switch

	return (bResult);
} // end CstiVideoPlayback::PacketCheckPictureStartCode


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::KeyframeRequest
//
// Description: Requests a key frame be sent from the remote endpoint.
//
// Abstract:
//	This function handles the tasks related to getting a key frame from the
//	remote endpoint. This includes calling the Conference Manager function to
//	send the H.245 message to the remote endpoint, as well as starting a timer
//	and setting local variables to keep track of the status of the key frame
//	request.
//
// un32Timestamp - If not 0, is stored and then looked at later to decide if we
//  need to request another keyframe or not.
//
// Returns:
//	estiOK if successful, estiERROR otherwise.
//
EstiResult CstiVideoPlayback::KeyframeRequest (uint32_t timestamp)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::KeyframeRequest);

	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

	if (timestamp && timestamp > m_un32KeyframeNeededForTimestamp)
	{
		// Keep track of this timestamp.  We don't want to reset the need for a timestamp
		// with a frame that has an earlier timestamp.
		m_un32KeyframeNeededForTimestamp = timestamp;
	}

	if (!m_bKeyFrameRequested)
	{
		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
			stiTrace ("<VP::KeyframeRequest> Requesting keyframe for timestamp. %u\n\n", m_un32KeyframeNeededForTimestamp);
		);

		KeyframeRequestNoLock ();
	}

	return estiOK;
}


/*!
 * \brief Handler for when keyFrameTimer fires, which requests a keyframe
 *        This appears to be a failsafe for when a requested keyframe
 *        doesn't come in - we re-request the keyframe
 */
void CstiVideoPlayback::EventKeyframeTimer ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::EventKeyframeTimer);

	stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
		stiTrace("<VideoPlayback::KeyframeRequestNoLock> Keyframe request timeout, request again ...\n\n");
	);

	KeyframeRequestNoLock ();
}


/*!
 * \brief Helper method for requesting a keyframe
 *        The event loop lock should be held before calling this
 */
void CstiVideoPlayback::KeyframeRequestNoLock ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::KeyframeRequestNoLock);

	if (!m_bChannelOpen || m_nMuted != 0)
	{
		return;
	}

	// If we support PLI via RTCP feedback, send the PLI message
	if (m_rtcpFeedbackPliEnabled && m_session->rtcpGet() != nullptr)
	{
		// Send request for keyframe via PLI RTCP feedback message
		RvRtcpFbAddPLI (m_session->rtcpGet(), m_un32sSrc);
		RvRtcpFbSend (m_session->rtcpGet(), RtcpFbModeImmediate);

		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
			stiTrace ("\tSending Keyframe request via PLI Feedback\n");
		);
	}
	else if (m_rtcpFeedbackFirEnabled && m_session->rtcpGet () != nullptr)
	{
		// Send request for keyframe via FIR RTCP feedback message
		// Note:  FIR is not meant to be used to recover from packet loss
		//        but we use it anyway if PLI is not supported and FIR is
		RvRtcpFbAddFIR (m_session->rtcpGet(), m_un32sSrc, m_nFirSeqNbr++);
		RvRtcpFbSend (m_session->rtcpGet(), RtcpFbModeImmediate);

		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
			stiTrace ("\tSending Keyframe request via FIR Feedback\n");
		);
	}
	else
	{
		// Request a new key frame via SIP XML method
		keyframeRequestSignal.Emit ();
	}

	// Keep track of the number of keyframes requested.
	StatsLock ();
	m_Stats.totalKeyframesRequested++;
	m_Stats.keyframeRequests++;
	m_Stats.un32KeyframeRequestedBeginTick = stiOSTickGet ();
	StatsUnlock ();

	m_bKeyFrameRequested = estiTRUE;
	m_keyFrameTimer.restart();
}


/*
 * \brief Checks whether or not we need to send a TMMBR request to adjust the
 *        sender's rate.
 */
void CstiVideoPlayback::EventTmmbrFlowControlTimer ()
{
	// Do nothing if TMMBR isn't enabled for the current payload
	if (!m_session->rtcpFeedbackTmmbrEnabledGet () || m_bDataChannelClosed)
	{
		return;
	}

	uint32_t avgPacketSize = 0;
	if (m_flowControlStats.packetsReceived > 0)
	{
		avgPacketSize =
			(m_flowControlStats.payloadSizeSum +
			 m_flowControlStats.overheadSizeSum) /
			m_flowControlStats.packetsReceived;
	}

	auto timeNow = std::chrono::steady_clock::now ();
	auto durationMs =
		std::chrono::duration_cast<std::chrono::milliseconds>
		    (timeNow - m_flowControlStats.startTime);

	auto currentMaxRate = m_flowControl.maxRateGet ();

	// Ask flow control to calculate a new max rate
	// which may or may not change from the current max
	
	auto newMaxRate =
		m_flowControl.maxRateCalculate (
			static_cast<uint32_t>(durationMs.count()),
			m_flowControlStats.packetsReceived,
			m_flowControlStats.packetsLost,
			avgPacketSize,
			m_flowControlStats.actualPacketsLost);

	// If the max rate has changed, send a TMMBR message!
	if (newMaxRate != currentMaxRate)
	{
		uint32_t avgPacketOverhead = 0;
		
		if (m_flowControlStats.packetsReceived)
		{
			avgPacketOverhead =
			m_flowControlStats.overheadSizeSum /
			m_flowControlStats.packetsReceived;
		}

		stiDEBUG_TOOL (g_stiVideoPlaybackFlowControlDebug,
			stiTrace ("EventTmmbrFlowControlTimer: sending TMMBR request maxrate=%u overhead=%u\n",
					newMaxRate, avgPacketOverhead);
		);

		if (m_session->rtcpGet () != nullptr)
		{

			RvRtcpFbAddTMMBR (
				m_session->rtcpGet (),
				m_un32sSrc,
				newMaxRate,
				avgPacketOverhead);

			auto rvStatus = RvRtcpFbSend (m_session->rtcpGet (), RtcpFbModeImmediate);
			stiASSERTMSG (rvStatus == RV_OK, "Sending TMMBR message encountered an issue");


			// This updates the stats value for target receive rate
			FlowControlRateSet (newMaxRate);
		}
	}

	// Reset the stats and restart timer
	flowControlReset ();
}


/*!
 * \brief Event handler to send NACK for missing packets.
 */
void CstiVideoPlayback::EventNackTimer ()
{
	auto requestsSent = missingPacketsNACK (m_un32LastReceivedSequence, m_un32sSrc);
	
	if (requestsSent)
	{
		StatsLock();
		
		m_Stats.nackRequestsSent += requestsSent;
		
		StatsUnlock();
	}
	
	stiDEBUG_TOOL (g_stiNackTimerInterval,
		m_nackTimer.timeoutSet (g_stiNackTimerInterval);
	);
	
	m_nackTimer.restart ();
}


/*!
 * \brief Event handler to check processed packets for a complete frame to send.
 */
void CstiVideoPlayback::EventCheckForCompleteFrame ()
{
	checkOldestPacketsSendCompleteFrame ();
}


/*!
 * \brief Event handler to send frame.
 */
void CstiVideoPlayback::EventSyncPlayback ()
{
	SyncPlayback ();
}


/*!
 * \brief Event handler to assembled packets in jitter buffer in a frame.
 */
void CstiVideoPlayback::EventJitterCalculate ()
{
	jitterCalculate ();
}

		 
/*!
 * \brief Event handler for when the read task notifies that we went from
 *        an empty queue to having packets in the queue.
 */
void CstiVideoPlayback::EventDataAvailable ()
{
	ProcessAvailableData ();

	// If there's still more data to process, schedule it to be processed
	if (m_playbackRead.NumPacketAvailable() > 0)
	{
		PostEvent (
			std::bind(&CstiVideoPlayback::EventDataAvailable, this));
	}
}


/*!
 * \brief Handler for when a keyframe request is received from video output
 */
 void CstiVideoPlayback::EventKeyframeRequest (uint32_t timestamp)
 {
    KeyframeRequest (timestamp);
 }


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::ProcessAvailableData
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
EstiResult CstiVideoPlayback::ProcessAvailableData ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::ProcessAvailableData);

	EstiResult eResult = estiOK;
	RvRtpParam * pstRTPParam;

	if(estiTRUE == m_bChannelOpen)
	{
		if(m_playbackRead.NumPacketAvailable())
		{

			// Yes, there are more packets in read task.
			// Get a packet from the read task
			auto packet = m_playbackRead.MediaPacketFullGet ((uint16_t)(m_un32ExpectedPacket & 0x0000ffffUL));

			// Did we get a packet?
			if (nullptr == packet)
			{
				// No! This is a problem.
				stiASSERT (estiFALSE);

				eResult = estiERROR;
			} // end if
			else if (m_nMuted)
			{
				// Throw away any packet received
				packet = nullptr;

				StatsLock();
				m_Stats.packetsDiscardedMuted++;
				StatsUnlock();
			}
			else
			{
				// Yes! process this packet
				pstRTPParam = packet->RTPParamGet ();
				
				// If the RTP param length is zero, do not process this packet
				if (pstRTPParam->len == 0)
				{
					packet = nullptr;
					eResult = estiERROR;

					StatsLock();
					m_Stats.packetsDiscardedEmpty++;
					StatsUnlock();
				}
				else
				{
					// Keep track of how many (payload) bytes were received
					// ACTION SLB - 2/18/02 Does payload bytes make more sense than
					// total RTP Bytes?
					// TYY - 2/26/04 should we count byte based on what we processed (here) or
					// based on what we received (CstiVideoPlaybackRead::PacketRead()).
					// They are different in the case of system saturation.
					ByteCountAdd(pstRTPParam->len - pstRTPParam->sByte);

					m_flowControlStats.payloadSizeSum += pstRTPParam->len - pstRTPParam->sByte;
					m_flowControlStats.overheadSizeSum += pstRTPParam->sByte;

					if (m_bFirstPacket)
					{
						m_bFirstPacket = estiFALSE;
						bool newFirstPacket = true;
						
						// If we still have packets in the queue and they have an older sequence number than this packet
						// use the oldest packet from the queue as the first packet. Otherwise use this new packet as the first one.
						if (!m_processedPackets.empty ())
						{
							auto nextFrameToSend = m_processedPackets.find (nextTimeToSend (m_processedPackets));
							
							if (nextFrameToSend != m_processedPackets.end ())
							{
								auto& oldestReceived = nextFrameToSend->second;
								
								if (!oldestReceived->processedPackets.empty())
								{
									auto oldestPacket = oldestReceived->processedPackets.front();
									
									if(oldestPacket && oldestPacket->m_un32ExtSequenceNumber < packet->m_un32ExtSequenceNumber)
									{
										FirstPacketSetup (oldestPacket);
										newFirstPacket = false;
									}
								}
							}
						}
						
						if (newFirstPacket)
						{
							FirstPacketSetup (packet);
						}
					}
					else
					{
						// Has the SSRC # changed?
						// We don't want to go back and forth with the SSRC #.  Allow it to change but if something else
						// shows up with a smaller sequence number and a different SSRC, don't let it swap back.
						if (pstRTPParam->sSrc != m_un32sSrc)
						{
							if (pstRTPParam->sSrc == m_un32sSrcPrevious)
							{
								// throw this packet away
								stiDEBUG_TOOL(2 & g_stiVideoPlaybackPacketDebug,
									stiTrace("<VP::EventProcessAvailableData> Throwing away sequence %d.\n\tReceived 0x%x\n\tSSRC:0x%x\n\tExpected SSRC:0x%x\n",
									packet->m_un32ExtSequenceNumber, m_un32ExpectedPacket, pstRTPParam->sSrc, m_un32sSrc);
								);

								packet = nullptr;

								StatsLock();
								m_Stats.packetsUsingPreviousSSRC++;
								StatsUnlock();
							}
							else
							{
								// Since the SSRC # has changed, set expected packet to be this packet and throw away everything
								// in the OOO queue.
								stiDEBUG_TOOL(2 & g_stiVideoPlaybackPacketDebug,
									stiTrace("<VP::EventProcessAvailableData> SSrc changing from 0x%x to 0x%x.\n\tExpected SQ# changing from %d to %d\n",
									m_un32sSrc, pstRTPParam->sSrc, m_un32ExpectedPacket, packet->m_un32ExtSequenceNumber);
								);
								
								VideoPacketQueueEmpty ();

								m_un32sSrcPrevious = m_un32sSrc;
								m_un32sSrc = pstRTPParam->sSrc;
								m_un32ExpectedPacket = packet->m_un32ExtSequenceNumber;

								/*
								 * m_bKeyframeSentToPlatform = false;
								 * Resetting this here is problematic since we could already have a
								 * keyframe prepared to be sent to the platform layer, waiting in the sync
								 * queue.  If so, and if it is a frame from the former SSRC, it will request
								 * a new keyframe and it will set the timestamp being waited on incorrectly
								 * to the timestamp of this former SSRC (which could be larger than the
								 * timestamps being used by the new SSRC).
								 */

								// Since we are now receiving from a new sync source, send the current frame as it is and
								// then start holding on to the new packets.
								m_un32LastReceivedSequence = ~0;
								m_missingSeqNumbers.clear ();
								m_bFirstPacket = estiTRUE;

								m_un32KeyframeNeededForTimestamp = 0;
							}
						}
					}

					if (packet)
					{
						bool existingTimestamp = false;
						
						// Update the missing packet list as needed.
						if ((m_lastProcessedPacket + 1) < packet->m_un32ExtSequenceNumber)
						{
							if (m_session->rtcpFeedbackNackRtxEnabledGet())
							{
								uint32_t lost = packet->m_un32ExtSequenceNumber - m_lastProcessedPacket - 1;
								auto time = stiOSTickGet ();

								for (uint32_t i = 1; i <= lost; ++i)
								{
									missingSeqNumStore (m_lastProcessedPacket + i, NackInfo (time));
								}

								auto requestsSent = missingPacketsNACK (m_un32LastReceivedSequence, m_un32sSrc);

								if (requestsSent)
								{
									StatsLock();

									m_Stats.nackRequestsSent += requestsSent;

									StatsUnlock();
								}
							}
							
							m_lastProcessedPacket = packet->m_un32ExtSequenceNumber;
						}
						else if (m_lastProcessedPacket > packet->m_un32ExtSequenceNumber)
						{
							auto iter = m_missingSeqNumbers.find(packet->m_un32ExtSequenceNumber);

							if (iter != m_missingSeqNumbers.end())
							{
								packet->retransmit = true;

								m_missingSeqNumbers.erase (packet->m_un32ExtSequenceNumber);
							}
							
							if (m_un32LastReceivedSequence > packet->m_un32ExtSequenceNumber)
							{
								packet = nullptr;
							}
						}
						else
						{
							m_lastProcessedPacket = packet->m_un32ExtSequenceNumber;
						}
						
						// Only continue if we still have a packet.
						if (packet)
						{
							for (auto& receivedPackets : m_processedPackets)
							{
								if (receivedPackets.first == packet->m_stRTPParam.timestamp)
								{
									existingTimestamp = true;
									break;
								}
							}
							
							if (!existingTimestamp)
							{
								const int minFramesBeforeForcing = 2;
								const int maxFramesBeforeForcing = 36;
								// If we have nothing in the jiter buffer or we are not doing NACK
								// force the oldest frame through when we have more than the minimum frames waiting to be processed.
								// Otherwise wait no more than the maximum amount (currently 36 frames a little over 1 second at 30 FPS).
								if (m_processedPackets.size() > minFramesBeforeForcing &&
									((!m_session->rtcpFeedbackNackRtxEnabledGet() || m_jitterBuffer.empty ()) ||
									 m_processedPackets.size() > maxFramesBeforeForcing))
								{
									oldestProcessedPacketsToFrame (false);
									
									// See if we have more frames to send.
									PostEvent (std::bind (&CstiVideoPlayback::EventCheckForCompleteFrame, this));
								}
								
								m_processedPackets[packet->m_stRTPParam.timestamp] = sci::make_unique<ReceivedPackets>();
							}
							
							auto &frame = m_processedPackets[packet->m_stRTPParam.timestamp];

							// Put the packet in order with other packets of the same timestamp.
							if (!frame->processedPackets.empty ())
							{
								auto insertPosition = frame->processedPackets.begin();
								
								while (insertPosition != frame->processedPackets.end())
								{
									// Discard duplicate packets received and count amount of duplicates received.
									if ((*insertPosition)->m_un32ExtSequenceNumber == packet->m_un32ExtSequenceNumber)
									{
										StatsLock  ();
										m_Stats.duplicatePacketsReceived++;
										StatsUnlock ();
										
										packet = nullptr;
										break;
									}

									if ((*insertPosition)->m_un32ExtSequenceNumber > packet->m_un32ExtSequenceNumber)
									{
										break;
									}

									insertPosition++;
								}
								
								if (packet)
								{
									frame->processedPackets.insert(insertPosition, packet);
								}
							}
							else
							{
								frame->processedPackets.push_back (packet);
							}
							
							if (packet && packet->m_stRTPParam.marker)
							{
								frame->markerSet = true;
							}
							
							// Check if we have a frame to send.
							if (packet)
							{
								auto nextFrameToSend = m_processedPackets.find (nextTimeToSend (m_processedPackets));
								
								if (nextFrameToSend != m_processedPackets.end ())
								{
									auto lastPacketGroup = nextFrameToSend->second->processedPackets.back();
									
									if ((packet->m_stRTPParam.timestamp == lastPacketGroup->m_stRTPParam.timestamp)
										|| (packet->m_un32ExtSequenceNumber == (lastPacketGroup->m_un32ExtSequenceNumber + 1)))
									{
										checkOldestPacketsSendCompleteFrame ();
									}
									else if (!m_playbackRead.NumEmptyBufferAvailable())
									{
										oldestProcessedPacketsToFrame (false);
									}
								}
							}
						}
					}
				} // end else
			} // end else
		} // end if
		else if (!m_processedPackets.empty () && !m_playbackRead.NumEmptyBufferAvailable())
		{
			// If we have no more packets available then we are saturated send what we can to free packets for read.
			stiASSERTMSG (estiFALSE, "Failed to process available data, nMAX_VP_BUFFERS = %d m_processedPackets.size = %d\n",nMAX_VP_BUFFERS, m_processedPackets.size ());
			// Send whatever we have so we can continue to process video.
			oldestProcessedPacketsToFrame (false);
			
			// See if we have more frames to send.
			PostEvent (std::bind (&CstiVideoPlayback::EventCheckForCompleteFrame, this));
		}
	} // end if
#if 0
	else
	{
		stiDEBUG_TOOL ( g_stiVideoPlaybackDebug,
			stiTrace("VP::EventProcessAvailableData - Connection Close!!!\n");
		);
	}
#endif

	return eResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback:: FirstPacketSetup
//
// Description: Sets values used to for processing data to ensure it is treated
// as the first packet.
//
// Abstract:
//  Setting these values will impact what packets are consider to be used
// in constructing a frame and what will be discarded.
//
// Returns:
// None.
//
void CstiVideoPlayback::FirstPacketSetup (const std::shared_ptr<CstiVideoPacket> &packet)
{
	m_un32ExpectedPacket = packet->m_un32ExtSequenceNumber;
	m_un32sSrc = packet->RTPParamGet ()->sSrc;
	m_un32sSrcPrevious = packet->RTPParamGet ()->sSrc;
	m_un32TicksWhenLastPlayed = stiOSTickGet ();
	m_lastAssembledFrameTS = packet->RTPParamGet ()->timestamp;
	m_bStartWithOldest = false;
	m_un32KeyframeNeededForTimestamp = 0;
	m_un32LastReceivedSequence = ~0;
	m_lastProcessedPacket = packet->m_un32ExtSequenceNumber;
	m_playbackRead.playbackSsrcSet (m_un32sSrc);
	m_playbackDelayTime.arrivalOfFirstPacket = m_un32TicksWhenLastPlayed;
	m_lastSeqNumInBuffer = 0;
	if (!m_nackTimer.isActive())
	{
		m_nackTimer.restart ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback:: VideoPacketToSyncQueue
//
// Description: Sends a packet containing a complete frame to the sync packet queue
//
// Abstract:
//  The sync packet queue contains video frames which are ready to send to driver
//  for playback
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiVideoPlayback::VideoPacketToSyncQueue (
	bool bFrameIsComplete,
	std::deque<std::shared_ptr<CstiVideoPacket>> *packets)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::VideoPacketToSyncQueue);

	EstiResult 	eResult = estiOK;

	uint32_t un32CurrentTimestamp =
			m_poFrame ? m_poFrame->m_un32TimeStamp : 0;

	// What is the last sequence number in the queue?  Store this off to help us know
	// if we have a complete frame.
	m_un32PreviousFrameLastSq = packets->back()->m_un32ExtSequenceNumber;

	// Assemble the frame
	auto packet = packets->front();
	int lostPackets {0};

	// Update the current timestamp since we wouldn't have had an m_poFrame above.
	un32CurrentTimestamp = packet->RTPParamGet()->timestamp;

	// If the m_un32ExtSequenceNumber is set as all 1's, it is in an uninitialized state.
	// Set it to be 1 less than the first packet's extended sequence number.
	if ((uint32_t)~0 == m_un32LastReceivedSequence)
	{
		m_un32LastReceivedSequence = packet->m_un32ExtSequenceNumber - 1;
	}

	m_poFrame = VideoFrameGet (); // Get a video frame from the video frame queue

	if (m_poFrame)
	{
		VideoFrameInitialize (m_poFrame, packet); // Initializes the m_poFrame frame object and its buffer to an empty state
	}

	PacketsReceivedCountAdd(packets->size());

	int retransmittedPackets {0};

	while (!packets->empty ())
	{
		// Get a packet and remove the packet from the packet queue
		packet = packets->front ();
		packets->pop_front ();

		if (packet->m_un32ExtSequenceNumber > m_un32ExpectedPacket)
		{
			m_un32ExpectedPacket = packet->m_un32ExtSequenceNumber;
		}

		if (packet->retransmit)
		{
			retransmittedPackets++;
		}

		// Process the found packet, but only if we still have a valid m_poFrame pointer.  It is possible
		// in H263PacketProcess, if the merge fails, to have the m_poFrame returned to VideoOutput and
		// set to NULL.  For H.263, we want to send all or nothing; no partial frames.
		// Don't drop out of the loop though ... we want to keep going so as to properly account for any
		// packet loss.
		if (m_poFrame)
		{
			PacketProcess (packet);
		}

		// Check for packet loss
		if (packet->m_un32ExtSequenceNumber != m_un32LastReceivedSequence + 1)
		{
			// Don't allow a difference that would be less than 0
			if (packet->m_un32ExtSequenceNumber <= m_un32LastReceivedSequence)
			{
				stiASSERT (estiFALSE);
			}
			else
			{
				lostPackets += (packet->m_un32ExtSequenceNumber - m_un32LastReceivedSequence - 1);
			}
		}

		m_un32LastReceivedSequence = packet->m_un32ExtSequenceNumber;
	}

	// If the frame is incomplete, request a keyframe
	if (!m_poFrame || !bFrameIsComplete || lostPackets)
	{
		KeyframeRequest (un32CurrentTimestamp);
	}

	StatsLock ();
	m_Stats.actualPacketLoss += lostPackets + retransmittedPackets;
	m_flowControlStats.actualPacketsLost += lostPackets + retransmittedPackets;
	StatsUnlock ();

	if (lostPackets)
	{
		PacketLossCountAdd (lostPackets);
	}

	if (m_poFrame)
	{

		// Add one to our frame counter
		StatsLock ();
		++m_Stats.un32FrameCount;
		StatsUnlock ();

		//
		// TODO: Do we have to take care of padding to a 16 bit boundary
		//

		//
		// If we are in the middle of assembling a fragmented NAL
		// unit then discard the fragments we have.
		//
		if (m_bStartedFNAL)
		{
			m_poFrame->m_pPlaybackFrame->DataSizeSet (m_un32NALUnitStart);
			m_bStartedFNAL = false;
		}

		//
		// If we still have a frame to send then go ahead and send it.
		//
		if (m_poFrame->m_pPlaybackFrame->DataSizeGet () > 0)
		{
			// Set the FrameIsComplete flag.
			m_poFrame->m_pPlaybackFrame->FrameIsCompleteSet (bFrameIsComplete);
			
			// Add this packet to sync queue
			m_oSyncQueue.Put (m_poFrame);
			
			PostEvent (std::bind (&CstiVideoPlayback::EventSyncPlayback, this));
			
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
						   stiTrace ("VP:: Time stamp %u is sending to sync queue (%d frames in queue)\n",
									m_poFrame->m_un32TimeStamp,
									m_oSyncQueue.CountGet());
						  );
		}
		else
		{
			//
			// Since there is nothing to send then return the frame to the empty queue.
			//
			VideoFramePut (m_poFrame);
			KeyframeRequest (un32CurrentTimestamp);
		}

		m_poFrame = nullptr;
	}

	m_bFrameComplete = estiFALSE;

	return (eResult);
} // end CstiVideoPlayback::VideoPacketToSyncQueue




////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::SyncPlayback
//
// Desc: Looks for and processes a video frame which will be ready
//  to send to decoder from sync frame queue
//
// Abstract:
//	The function will send video frames whose time become current to video
//  driver for playback
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiVideoPlayback::SyncPlayback ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::SyncPlayback);

	EstiResult eResult = estiOK;

#ifdef stiDEBUGGING_TOOLS
	static EstiBool bTest = estiFALSE;
#endif

	if (!m_poPendingPlaybackFrame)
	{
		// Get the oldest packet in the full queue
		m_poPendingPlaybackFrame = m_oSyncQueue.Get ();
	}

	if (m_poPendingPlaybackFrame != nullptr)
	{
		uint32_t un32CurrentTimestamp = m_poPendingPlaybackFrame->m_un32TimeStamp;

		bool bFullframeSentToPlatform = false;
		bool keyFrame = m_poPendingPlaybackFrame->m_bKeyframe;

		// Store the recorded stream to a file for debugging purposes.
		stiDEBUG_TOOL (g_stiVideoPlaybackCaptureStream2File,
			fwrite (m_poPendingPlaybackFrame->m_pPlaybackFrame->BufferGet (),
				sizeof (uint8_t),
				m_poPendingPlaybackFrame->m_pPlaybackFrame->DataSizeGet (),
				pVPFileOut);
			fflush (pVPFileOut);
		);  // end stiDEBUG_TOOL

		if (m_poPendingPlaybackFrame->m_pPlaybackFrame->FrameIsCompleteGet ())
		{
			bFullframeSentToPlatform = true;
		}

		// If it is a keyframe, set it in the frame being sent to the platform layer.
		m_poPendingPlaybackFrame->m_pPlaybackFrame->FrameIsKeyframeSet (m_poPendingPlaybackFrame->m_bKeyframe);

		// Pass the video frame to the video device
		stiHResult hResult = m_pVideoOutput->VideoPlaybackFramePut(m_poPendingPlaybackFrame->m_pPlaybackFrame);

		m_poPendingPlaybackFrame->m_pPlaybackFrame = nullptr;

		if(stiIS_ERROR (hResult))
		{
#ifdef sciRTP_FROM_FILE
			stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
				stiTrace ("VP:: SQ# %d.  Failure to send to driver\n",
					  poPacket->m_un32ExtSequenceNumber);
			);
#endif
			// We had an error sending data to driver
			eResult = estiERROR;

			KeyframeRequest (un32CurrentTimestamp);
		}
		else
		{
#ifdef sciRTP_FROM_FILE
			if (0 < (2 & g_stiUPnPDebug)) // if 2 is set, print the debug trace (it's pretty noisy though).
			{
				stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
					stiTrace ("VP:: SQ# %d (to %d) successfully sent to driver\n",
						poPacket->m_un32ExtSequenceNumber,
						poPacket->m_un32ExtSequenceNumber + poPacket->un8PacketsPerFrame - 1);
				);

			}
#endif

			// Add one to our frame counter
			FrameCountAdd (bFullframeSentToPlatform, keyFrame);

			// If the frame was not a keyframe and we have not yet sent a keyframe, request one from the
			// remote endpoint.
			if (!m_bKeyframeSentToPlatform)
			{
				if (m_poPendingPlaybackFrame->m_bKeyframe
					&& bFullframeSentToPlatform)
				{
					m_bKeyframeSentToPlatform = true;
				}
				else
				{
					// Request a keyframe
					KeyframeRequest (un32CurrentTimestamp);
				}
			}

			stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
				static int nCount = 0;
				if (g_bMissingPacket)
				{
					nCount = 600;
					g_bMissingPacket = estiFALSE;
				}

				if (++nCount > 600)
				{
					stiTrace ("VP:: VideoNTP = %lu is Sending to driver ...\n",
							m_poPendingPlaybackFrame->m_un32TimeStamp);
				} // end if (++nCount > 600)

				if (nCount == 602)
				{
					nCount = 0;
					bTest = estiTRUE;
				}
			);

			if (m_poPendingPlaybackFrame->m_bKeyframe
				&& bFullframeSentToPlatform)
			{
				// Since we successfully sent the frame, make sure to handle the receipt.
				KeyframeReceived (un32CurrentTimestamp);
			}
		}

		// we are done with this Frame, put it back to frame queue
		VideoFramePut (m_poPendingPlaybackFrame);
		m_poPendingPlaybackFrame = nullptr;

		stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
			if (estiTRUE == bTest)
			{
				stiTrace("VP:: empty = %d full = %d display = %d frames pending assembly = %d\n",
					m_playbackRead.NumEmptyBufferAvailable(),
					m_playbackRead.NumPacketAvailable(),
					m_oSyncQueue.CountGet(), m_processedPackets.size());
				stiTrace ("\n");
				bTest = estiFALSE;
			}
		);

	} // end else

	return eResult;
}


//:-----------------------------------------------------------------------------
//:
//: Data flow functions
//:
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::DataChannelClose
//
// Description: Post a message that the channel is closing.
//
// Abstract:
//	This method posts a message to the Video Playback task that the channel is
//	closing.
//
// Returns:
//	estiOK when successfull, otherwise estiERROR
//
stiHResult CstiVideoPlayback::DataChannelClose ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiVideoPlayback::DataChannelClose);

	std::lock_guard<std::recursive_mutex> lk(m_execMutex);

	if (!m_bDataChannelClosed)
	{
		DisconnectSignals ();

		//
		// Tell the video read task that we are closing
		//
		hResult = m_playbackRead.DataChannelClose ();

		// Yes, close the channel we are currently work on
		stiDEBUG_TOOL (g_stiVideoPlaybackCaptureStream2File,
			stiTrace ("VP:: Closing file\n");
			fclose (pVPFileOut);
		); // stiDEBUG_TOOL


		m_keyFrameTimer.stop ();

		m_flowControlTimer.stop ();

		//
		// Make sure that we clear out any half-finished frames
		// that we have been building.
		//
		FrameClear ();

		//
		// Return any frames in the sync queue back to the video frame queue.
		//
		SyncQueueClear ();

		//
		// Set a global indicating the channel is being shutdown.
		//
		m_bChannelOpen = estiFALSE;

		if (m_nMuted)
		{
			m_timeMutedByRemote += MutedDurationGet ();

			// Make sure to disable the far end hold media
			if (m_nMuted & (estiMUTED_REASON_HELD | estiMUTED_REASON_HOLD))
			{
				m_pVideoOutput->RemoteViewHoldSet (estiFALSE);
			}
		}

		// Tell the video driver to stop video playback
		if (m_bVideoPlaybackStarted)
		{
			hResult = m_pVideoOutput->VideoPlaybackStop ();
			if(stiIS_ERROR (hResult))
			{
				stiASSERT (estiFALSE);
			} // end if
			m_bVideoPlaybackStarted = false;
		}

		m_nCurrentRTPPayload = -1;
		m_bDataChannelClosed = estiTRUE;
	}

	return hResult;
}


stiHResult CstiVideoPlayback::DataChannelHold ()
{
	std::lock_guard<std::recursive_mutex> lk(m_execMutex);
	MediaPlaybackType::DataChannelHold();

	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoPlayback::DataChannelHold);

	DisconnectSignals ();

	// Make sure to disable the far end hold media
	if (m_nMuted & (estiMUTED_REASON_HELD | estiMUTED_REASON_HOLD))
	{
		m_pVideoOutput->RemoteViewHoldSet (estiFALSE);
	}

	// Tell the video driver to stop video playback
	if (m_bVideoPlaybackStarted)
	{
		hResult = m_pVideoOutput->VideoPlaybackStop ();
		stiASSERT (!stiIS_ERROR(hResult));

		m_bVideoPlaybackStarted = false;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::DataChannelInitialize
//
// Description: Sets up for the initialization of a data channel.
//
// Abstract:
//	This function is called at the time of negotiation of the data channel.
//	The conference manager will call this function with data that is needed to
//	prepare for the sending of data through the video playback channel. This is
//	the second level of startup initialization.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
stiHResult CstiVideoPlayback::DataChannelInitialize (
	bool bStartKeepAlives)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::DataChannelInitialize);
	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

	stiHResult hResult = MediaPlaybackType::DataChannelInitialize(bStartKeepAlives);

	m_rtcpFeedbackTmmbrEnabled = false;
	m_rtcpFeedbackFirEnabled = false;
	m_rtcpFeedbackPliEnabled = false;
	m_rtcpFeedbackNackRtxEnabled = false;

	return hResult;
}



/*!
 * \brief Connects to signals from video output (and potentially others)
 */
void CstiVideoPlayback::ConnectSignals ()
{
	if (!m_keyframeNeededSignalConnection)
	{
		// Handle keyframe requests from video output
		m_keyframeNeededSignalConnection = m_pVideoOutput->keyframeNeededSignalGet ().Connect (
			[this]{
				PostEvent (
					std::bind(&CstiVideoPlayback::EventKeyframeRequest, this, 0));
			});
	}

	if (!m_decodeSizeChangedSignalConnection)
	{
		// Handle decode size changed events
		m_decodeSizeChangedSignalConnection = m_pVideoOutput->decodeSizeChangedSignalGet ().Connect (
			[this](uint32_t width, uint32_t height){
				PostEvent (
					std::bind(&CstiVideoPlayback::EventVideoPlaybackSizeChanged, this, width, height));
			});
	}
}


/*!
 * \brief Disconnects from signals in video output (and potentially others)
 */
void CstiVideoPlayback::DisconnectSignals ()
{
	if (m_pVideoOutput)
	{
		m_keyframeNeededSignalConnection.Disconnect ();

		m_decodeSizeChangedSignalConnection.Disconnect ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::DataChannelInitialize
//
// Description: Sets up for the initialization of a data channel.
//
// Abstract:
//	This function is called at the time of negotiation of the data channel.
//	The conference manager will call this function with data that is needed to
//	prepare for the sending of data through the video playback channel. This is
//	the second level of startup initialization.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
// TODO: this was different enough from the base class to skip
// consolidation, although maybe with more effort it can be
// consolidated.
//
stiHResult CstiVideoPlayback::DataChannelInitialize ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::DataChannelInitialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_bKeyFrameRequested = estiFALSE;
	m_bFirstPacket = estiTRUE;
	m_nThrowAwayCount = 0;
#if !defined(stiNO_H263_MISSING_PACKET_DETECTION) || !defined(stiNO_H264_MISSING_PACKET_DETECTION)
	m_bMissingPacket = estiFALSE;
#endif
	// set the current video codec type to be NONE
	m_currentCodec = estiVIDEO_NONE;

	stiDEBUG_TOOL (g_stiVideoPlaybackCaptureStream2File,
		stiTrace ("VP:: Open file\n");
		pVPFileOut = fopen (szVPFileOut, "wb");
	);  // end stiDEBUG_TOOL

	//
	// Move all queued packets back into the empty queue.
	//
	VideoPacketQueueEmpty ();

	if(m_pSyncManager != nullptr)
	{
		m_pSyncManager->Initialize();
	}

	//
	// Return any pending playback frame or frames in the sync queue
	// back to the frame queue.
	//
	SyncQueueClear();

	//
	// Tell the Video Playback Read Task to start reading
	//

	hResult = m_playbackRead.DataChannelInitialize (m_un32InitialRtcpTime,
														  &m_PayloadMap);
	stiASSERT (!stiIS_ERROR(hResult));

	//
	// Set a global indicating the channel is now open.
	//
	if (!stiIS_ERROR(hResult))
	{
		m_bChannelOpen = estiTRUE;
	}

	ConnectSignals ();

	flowControlReset ();

	//
	// Turn off the hold and privacy screen (in case they are on for another call).
	//
	m_pVideoOutput->RemoteViewHoldSet ((m_nMuted & estiMUTED_REASON_HELD) ? estiTRUE : estiFALSE);
	m_pVideoOutput->RemoteViewPrivacySet ((m_nMuted & estiMUTED_REASON_PRIVACY) ? estiTRUE : estiFALSE);

	m_bDataChannelClosed = estiFALSE;

	// Send the message
	return hResult;
}




stiHResult CstiVideoPlayback::DataChannelResume ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::DataChannelResume);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

#if !defined(stiNO_H263_MISSING_PACKET_DETECTION) || !defined(stiNO_H264_MISSING_PACKET_DETECTION)
	m_bMissingPacket = estiFALSE;
#endif
	// Indicate that the channel is resuming to cause VideoPlaybackCodecSet to be triggered.
	m_channelResumed = true;

	stiDEBUG_TOOL (g_stiVideoPlaybackCaptureStream2File,
		stiTrace ("VP:: Open file\n");
		pVPFileOut = fopen (szVPFileOut, "wb");
	);  // end stiDEBUG_TOOL

	if(m_pSyncManager != nullptr)
	{
		m_pSyncManager->Initialize();
	}

	m_bChannelOpen = estiTRUE;

	ConnectSignals ();

	//
	// Turn off the hold and privacy screen (in case they are on for another call).
	//
	m_pVideoOutput->RemoteViewHoldSet ((m_nMuted & estiMUTED_REASON_HELD) ? estiTRUE : estiFALSE);
	m_pVideoOutput->RemoteViewPrivacySet ((m_nMuted & estiMUTED_REASON_PRIVACY) ? estiTRUE : estiFALSE);

	m_bDataChannelClosed = estiFALSE;

	// Send the message
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::StatsClear
//
// Description: Clears the stats
//
// Abstract:
//
// Returns:
//	None
//
void CstiVideoPlayback::StatsClear ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::StatsClear);

	StatsLock ();

	m_Stats = {};

	ByteCountReset ();
	m_playbackRead.StatsClear ();

	StatsUnlock ();
} // end CstiVideoPlayback::StatsClear


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::StatsCollect
//
// Description: Collects the stats since the last collection
//
// Abstract:
//
// Returns:
//	Return Value: EstiResult - estiOK or estiERROR
//
EstiResult CstiVideoPlayback::StatsCollect (
	SstiVPStats *pStats)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::StatsCollect);

	static float fTicksToMillisecondsMultiplier = 1000.0F / (float)stiOSSysClkRateGet ();

	StatsLock ();

	*pStats = m_Stats;

	pStats->un32ByteCount = ByteCountCollect ();

	vpe::MediaPlaybackReadStats VPRStats;

	m_playbackRead.StatsCollect (&VPRStats);

	pStats->packetsRead = VPRStats.packetsRead;
	pStats->keepAlivePackets = VPRStats.keepAlivePackets;
	pStats->packetQueueEmptyErrors = VPRStats.packetQueueEmptyErrors;
	pStats->un32UnknownPayloadTypeErrors = VPRStats.unknownPayloadTypeErrors;
	pStats->un32PayloadHeaderErrors = VPRStats.payloadHeaderErrors;
	pStats->packetsDiscardedReadMuted = VPRStats.packetsDiscardedMuted;

	pStats->timeMutedByRemote = m_timeMutedByRemote;
	pStats->rtxPacketsReceived = VPRStats.rtxPacketsReceived;

	if (m_nMuted)
	{
		pStats->timeMutedByRemote += MutedDurationGet ();
	}

	pStats->un32KeyframeMinWait = (uint32_t)(m_Stats.un32KeyframeMinWait * fTicksToMillisecondsMultiplier);
	pStats->un32KeyframeMaxWait = (uint32_t)(m_Stats.un32KeyframeMaxWait * fTicksToMillisecondsMultiplier);
	pStats->un32KeyframeAvgWait = (uint32_t)((float)m_Stats.un32KeyframeTotalTicksWaited / (float)m_Stats.totalKeyframesReceived * fTicksToMillisecondsMultiplier);
	pStats->fKeyframeTotalWait = (float)m_Stats.un32KeyframeTotalTicksWaited / (float)stiOSSysClkRateGet ();

	// Clear the members that are only counts since last collection
	m_Stats.packetsReceived = 0;
	m_Stats.packetsDiscardedEmpty = 0;
	m_Stats.packetsUsingPreviousSSRC = 0;
	m_Stats.packetsLost = 0;
	m_Stats.actualPacketLoss = 0;
	m_Stats.nackRequestsSent = 0;
	m_Stats.burstPacketsDropped = 0;
	m_Stats.un32FrameCount = 0;
	m_Stats.keyframeRequests = 0;
	m_Stats.keyframesReceived = 0;
	m_Stats.un32OutOfOrderPackets = 0;
	m_Stats.un32MaxOutOfOrderPackets = 0;
	m_Stats.un32DuplicatePackets = 0;
	m_Stats.un32VideoPlaybackFrameGetErrors = 0;
	m_Stats.un32ErrorConcealmentEvents = 0;
	m_Stats.un32IFramesDiscardedIncomplete = 0;
	m_Stats.un32PFramesDiscardedIncomplete = 0;
	m_Stats.un32GapsInFrameNumber = 0;
	m_Stats.bTunneled = m_session->tunneledGet();
	memset (m_Stats.aun32PacketPositions, 0, sizeof (m_Stats.aun32PacketPositions));
	m_Stats.highestPacketLossSeen = 0;
	m_Stats.countOfCallsToPacketLossCountAdd = 0;

	m_Stats.framesSentToPlatform = 0;
	m_Stats.partialKeyFramesSentToPlatform = 0;
	m_Stats.partialNonKeyFramesSentToPlatform = 0;
	m_Stats.wholeKeyFramesSentToPlatform = 0;
	m_Stats.wholeNonKeyFramesSentToPlatform = 0;
	m_Stats.playbackDelay = 0;

	StatsUnlock ();

	return (estiOK);
} // end CstiVideoPlayback::StatsCollect


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::FrameCountAdd
//
// Description: Adds to the Frame counter
//
// Abstract:
//
// Returns:
//	None
//
void CstiVideoPlayback::FrameCountAdd (
	bool frameIsComplete,
	bool frameIsKeyFrame)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::FrameCountAdd);

	StatsLock ();

	m_Stats.framesSentToPlatform++;
	
	if (!frameIsComplete)
	{
		if (frameIsKeyFrame)
		{
			m_Stats.partialKeyFramesSentToPlatform++;
		}
		else
		{
			m_Stats.partialNonKeyFramesSentToPlatform++;
		}
	}
	else
	{
		if (frameIsKeyFrame)
		{
			m_Stats.wholeKeyFramesSentToPlatform++;
		}
		else
		{
			m_Stats.wholeNonKeyFramesSentToPlatform++;
		}
	}

	StatsUnlock ();
} // end CstiVideoPlayback::FrameCountAdd

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::PacketLossCountAdd
//
// Description: Adds to the PacketLoss counter
//
// Abstract:
//
// Returns:
//	None
//
// NOTE: wasn't consolidated to base class because it's
// different than what audio/text do
//
void CstiVideoPlayback::PacketLossCountAdd (
	uint32_t un32AmountToAdd)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::PacketLossCountAdd);

	StatsLock ();

	m_Stats.packetsLost += un32AmountToAdd;
	m_flowControlStats.packetsLost += un32AmountToAdd;

	StatsUnlock ();
} // end CstiVideoPlayback::PacketLossCountAdd


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::PacketsReceivedCountAdd
//
// Description: Adds to the PacketsReceived counter
//
// Abstract:
//
// Returns:
//	None
//
void CstiVideoPlayback::PacketsReceivedCountAdd (int numPackets)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::PacketsReceivedCountAdd);

	StatsLock ();

	m_Stats.packetsReceived += numPackets;
	m_flowControlStats.packetsReceived += numPackets;

	StatsUnlock ();
} // end CstiVideoPlayback::PacketsReceivedCountAdd


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::SetVideoSyncInfo
//
// Description: set the synchronization information to sync manager object
//
// Abstract:
//
// Returns:
//	None
//
EstiResult CstiVideoPlayback::SetVideoSyncInfo (
	uint32_t unNTP32sec,
	uint32_t un32NTPfrac,
	uint32_t un32Timestamp)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::SetVideoSyncInfo);

	EstiResult eResult = estiOK;

	m_pSyncManager->SetVideoSyncInfo(unNTP32sec, un32NTPfrac, un32Timestamp);

	return (eResult);
}


/*!\brief Stops the receipt of video packets
 *
 * Note: some of the actions in this method are really for preparing to receive
 * video after hold or privacy is turned off.
 */
void CstiVideoPlayback::VideoStreamStop ()
{
	m_pSyncManager->SetRemoteVideoMute(estiTRUE);
	VideoPacketQueueEmpty ();
	FrameClear ();
	SyncQueueClear();
	m_flowControlTimer.stop ();
	m_playbackRead.RtpHaltedSet(true);
	m_keyFrameTimer.stop ();
	m_bFirstPacket = estiTRUE;
	m_mutedStartTime = std::chrono::steady_clock::now ();
}


/*!\brief Prepare the system to start receiving video
 *
 */
void CstiVideoPlayback::VideoStreamStart ()
{
	m_pSyncManager->SetRemoteVideoMute(estiFALSE);
	VideoPacketQueueEmpty ();
	SyncQueueClear();
	flowControlReset ();
	m_playbackRead.RtpHaltedSet(false);
	m_bKeyFrameRequested = estiFALSE;
	m_bFirstPacket = estiTRUE;
	m_bKeyframeSentToPlatform = false;
	m_nThrowAwayCount = 0;
	m_timeMutedByRemote += MutedDurationGet ();
}


/*! brief Inform the data task if the remote has set video privacy or not
 *
 * \retval stiRESULT_SUCCESS or stiRESULT_ERROR
 */
stiHResult CstiVideoPlayback::Muted (
	eMutedReason eReason) // Is privacy set to ON or OFF
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::Muted);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

	switch (eReason)
	{
		case estiMUTED_REASON_PRIVACY:

			if ((m_nMuted & estiMUTED_REASON_PRIVACY) == 0)
			{
				// Is this the first change to a muted state?
				if (m_nMuted == 0)
				{
					VideoStreamStop ();
				}

				m_nMuted |= estiMUTED_REASON_PRIVACY;
			}

			//
			// If we are holding the call then don't change the remote view
			// (this video playback data task doesn't have access to the video
			// output device).
			//
			if ((m_nMuted & estiMUTED_REASON_HOLD) == 0)
			{
				m_pVideoOutput->RemoteViewPrivacySet (estiTRUE);
			}

			break;

		case estiMUTED_REASON_HELD:

			if ((m_nMuted & estiMUTED_REASON_HELD) == 0)
			{
				// Is this the first change to a muted state?
				if (m_nMuted == 0)
				{
					VideoStreamStop ();
				}

				//
				// If we are holding the call then don't change the remote view
				// (this video playback data task doesn't have access to the video
				// output device).
				//
				if ((m_nMuted & estiMUTED_REASON_HOLD) == 0)
				{
					m_pVideoOutput->RemoteViewHoldSet (estiTRUE);
				}

				m_nMuted |= estiMUTED_REASON_HELD;
			}

			break;

		case estiMUTED_REASON_HOLD:
		case estiMUTED_REASON_DHV:

			if ((m_nMuted & eReason) == 0)
			{
				m_un32InitialRtcpTime = m_playbackRead.InitialRTCPTimeGet();

				if (m_nMuted == 0)
				{
					VideoStreamStop ();
				}

				DataChannelHold ();

				m_nMuted |= eReason;
			}

			break;
	}

	return (hResult);
}


/*! brief Inform the data task if the remote has set video privacy or not
 *
 * \retval stiRESULT_SUCCESS or stiRESULT_ERROR
 */
stiHResult CstiVideoPlayback::Unmuted (
	eMutedReason eReason) // Is privacy set to ON or OFF
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::Unmuted);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

	switch (eReason)
	{
		case estiMUTED_REASON_PRIVACY:

			if ((m_nMuted & estiMUTED_REASON_PRIVACY) != 0)
			{
				m_nMuted &= ~estiMUTED_REASON_PRIVACY;

				// Are there any remaining muted reasons?
				if (m_nMuted == 0)
				{
					VideoStreamStart ();
				}
				
				// Set the privacy mode of the remote view to video driver
				m_pVideoOutput->RemoteViewPrivacySet (estiFALSE);
			}

			break;

		case estiMUTED_REASON_HELD:

			if ((m_nMuted & estiMUTED_REASON_HELD) != 0)
			{
				m_nMuted &= ~estiMUTED_REASON_HELD;

				// If there are no remaining muted reasons...
				if (m_nMuted == 0)
				{
					VideoStreamStart ();
					m_pSyncManager->SetRemoteVideoMute(estiFALSE);

					//
					// Turn off the hold screen
					//
					m_pVideoOutput->RemoteViewHoldSet (estiFALSE);
				}
				// If the remaining muted reason is only for privacy, disable showing the "hold" screen.
				else if (m_nMuted == estiMUTED_REASON_PRIVACY)
				{
					m_pVideoOutput->RemoteViewHoldSet (estiFALSE);
				}
			}

			break;

		case estiMUTED_REASON_HOLD:
		case estiMUTED_REASON_DHV:

			if ((m_nMuted & eReason) != 0)
			{
				m_nMuted &= ~eReason;

				if ((m_nMuted & estiMUTED_REASON_HOLD) == 0 &&
					(m_nMuted & estiMUTED_REASON_DHV) == 0)
				{
					//
					// Initialize the channel. This will setup the video output device
					// with the correct privacy, hold screen or live video.
					//
					DataChannelResume ();
					
					if (m_nMuted == 0)
					{
						VideoStreamStart ();
					}
				}
			}

			break;
	}

	return (hResult);
}


/*!
* \brief Store the privacy mode for this channel
*/
void CstiVideoPlayback::PrivacyModeSet (
	EstiSwitch mode)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::PrivacyModeSet);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (mode != PrivacyModeGet())
	{
		CstiDataTaskCommonP::PrivacyModeSet (mode);

		bool muted = (mode == estiON);
		privacyModeChangedSignal.Emit (muted);

		if (muted)
		{
			Muted (estiMUTED_REASON_PRIVACY);
		}
		else
		{
			Unmuted (estiMUTED_REASON_PRIVACY);
		}
	}
}


///\brief Get the currently active codec being used
///
///\returns estiOK if successful otherwise an error.
///
EstiResult CstiVideoPlayback::CaptureSizeGet (
	uint32_t *pun32PixelsWide,	///< The width of the received media
	uint32_t *pun32PixelsHigh) ///< The height of the received media
	const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

	*pun32PixelsWide = m_unLastKnownWidth;
	*pun32PixelsHigh = m_unLastKnownHeight;

	return 	(!stiIS_ERROR (hResult) ? estiOK : estiERROR);
}


///\brief Get the currently active codec being used
///
///\retval The codec that is being used for the received media.
///
void CstiVideoPlayback::PayloadMapSet (
	const TVideoPayloadMap &pPayloadMap)
{
	std::lock_guard<std::recursive_mutex> lk (m_execMutex);

	MediaPlaybackType::PayloadMapSet(pPayloadMap);

	// We need to clear the rtcp feedback flags because support for them
	// may have been removed in this new payload map
	m_rtcpFeedbackFirEnabled = false;
	m_rtcpFeedbackPliEnabled = false;
	m_rtcpFeedbackTmmbrEnabled = false;
	m_rtcpFeedbackNackRtxEnabled = false;

	// Reset the current payload so it forces reinitialization of TMMBR
	m_nCurrentRTPPayload  = -1;
}


EstiPacketResult CstiVideoPlayback::H264FUAToVideoFrame (
	CstiVideoFrame * poFrame,	// Destination Packet
	const std::shared_ptr<CstiVideoPacket> &srcPacket)// Source Packet
{
	EstiPacketResult eMergeResult = ePACKET_OK;
	stiHResult hResult = stiRESULT_SUCCESS;
	
	EstiBool bStartFragment = estiFALSE;
	EstiBool bEndFragment = estiFALSE;

	IstiVideoPlaybackFrame *pFrameBuffer = poFrame->m_pPlaybackFrame;
	vpe::VideoFrame *pstSrcVideoPacket = srcPacket->m_frame;
	uint8_t *pun8SrcBuffer = pstSrcVideoPacket->buffer;
	
	const int32_t n32FUHeaderPos = un8FU_INDICATOR_SIZE;
	const int32_t n32FUPayloadStartPos = (un8FU_INDICATOR_SIZE + un8FU_HEADER_SIZE);
	const int32_t n32FUPayloadSize = srcPacket->unPacketSizeInBytes - n32FUPayloadStartPos;
	
	// The NAL unit header is conveyed in F and NRI fields of the FU indicator
	// and in the type field of the FU header
	// Bit 		|0|12 |34567|
	// Field	|F|NRI|Type | - FU indicator
	// Field	|S|E|R|Type | - FU Header
	
	// To FU header and check the start & end bit
	if (0 != (pun8SrcBuffer[n32FUHeaderPos] & 0x80))
	{
		bStartFragment = estiTRUE;
	}
	else if (0 != (pun8SrcBuffer[n32FUHeaderPos] & 0x40))
	{
		bEndFragment = estiTRUE;
	}

	// Yes! Is this packet a start fragment of a new fragmented NAL unit?
	if (bStartFragment)
	{
	
		// ensure space for fragment
		uint32_t un32DesiredSize = m_bStartedFNAL ? m_un32NALUnitStart : pFrameBuffer->DataSizeGet() + // start
																sizeof(uint32_t) + sizeof(uint8_t) + // header sizes
																n32FUPayloadSize; // payload
		if (pFrameBuffer->BufferSizeGet() < un32DesiredSize) 
		{
			hResult = pFrameBuffer->BufferSizeSet(un32DesiredSize);

			if (stiIS_ERROR (hResult))
			{
				stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
							"H264FUAToVideoFrame_1: Failed to increase buffer size to %u bytes.", un32DesiredSize);
			}
		}

		//
		// If we have started assembling a fragmented NAL Unit
		// then throw what we've done away because we
		// just got the start of another.
		//
		if (m_bStartedFNAL)
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("VP:: SQ# %d restarted fragmented NAL.\n",
					srcPacket->m_un32ExtSequenceNumber);
			);			
			pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
		}
		else
		{
			m_bStartedFNAL = true;
		}
		
		stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
			stiTrace ("VP:: SQ# %d is First packet of a fragmented NAL.\n",
				srcPacket->m_un32ExtSequenceNumber);
		);

		//
		// Remember where we have started the NAL unit and make room for the start code or
		// NAL unit size.
		//
		m_un32NALUnitStart = pFrameBuffer->DataSizeGet ();

		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + sizeof (uint32_t));
		
		//
		// Assemble the NAL Header and write it in place.
		//
		uint8_t un8NALHeader = (pun8SrcBuffer[0] & 0xE0) | (pun8SrcBuffer[1] & 0x1F);

		// Check to see if this is a keyframe.
		if ((pun8SrcBuffer[1] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_OF_IDR)
		{
			poFrame->m_bKeyframe = true;
		}

		// is this the only NAL header that needs to be checked?
		
		*(pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet ()) = un8NALHeader;
		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + sizeof (uint8_t));
		
		//
		// Copy the payload in place.
		//
		memcpy (pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet (),
				&pun8SrcBuffer[n32FUPayloadStartPos], n32FUPayloadSize);
		
		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + n32FUPayloadSize);
		
		m_un32FNALExpectedPacket = srcPacket->m_un32ExtSequenceNumber + 1;
	}
	else if (m_bStartedFNAL)
	{
		// Is this the packet we expect?
		if (srcPacket->m_un32ExtSequenceNumber == m_un32FNALExpectedPacket)
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("VP:: SQ# %d belongs to our fragmented NAL . "
					"Merging with time stamp %d...\n",
					srcPacket->m_un32ExtSequenceNumber,
					poFrame->m_un32TimeStamp);
			);

			if (pFrameBuffer->BufferSizeGet() < pFrameBuffer->DataSizeGet() + n32FUPayloadSize)
			{
				hResult = pFrameBuffer->BufferSizeSet(pFrameBuffer->DataSizeGet() + n32FUPayloadSize);

				if (stiIS_ERROR (hResult))
				{
					stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
								"H264FUAToVideoFrame_2: Failed to increase buffer size to %u bytes.",
								pFrameBuffer->DataSizeGet() + n32FUPayloadSize);
				}
			}

			memcpy (pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet (),
					&pun8SrcBuffer[n32FUPayloadStartPos], n32FUPayloadSize);
			
			pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + n32FUPayloadSize);
			
			// Is the new packet the end of the fragmented NAL unit?
			if (bEndFragment)
			{
				stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
					stiTrace ("VP:: SQ# %d is the end of a fragmented NAL. \n",
						srcPacket->m_un32ExtSequenceNumber);
				);

				uint8_t *pDst = pFrameBuffer->BufferGet () + m_un32NALUnitStart;

				if (m_pVideoOutput->H264FrameFormatGet () == eH264FrameFormatByteStream)
				{
					pDst[0] = 0;
					pDst[1] = 0;
					pDst[2] = 0;
					pDst[3] = 1;
				}
				else
				{
					uint32_t un32Size = pFrameBuffer->DataSizeGet () - m_un32NALUnitStart - sizeof (uint32_t);

					pDst[3] = (un32Size & 0xFF);
					pDst[2] = ((un32Size >> 8) & 0xFF);
					pDst[1] = ((un32Size >> 16) & 0xFF);
					pDst[0] = ((un32Size >> 24) & 0xFF);
				}

				//
				// We have finished assembling the fragmented NAL.
				//
				m_bStartedFNAL = false;
			}
			else
			{
				++m_un32FNALExpectedPacket;
			}
		}
		else
		{
			stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
				stiTrace ("VP:: SQ# %d does NOT belong to our fragmented NAL ."
					"--- Missing fragments\n",
					srcPacket->m_un32ExtSequenceNumber);
			);

			// No! this is NOT the fragment unit we expect.
			// We miss fragments for the fragmented NAL unit,
			// so throw away the current fragmented NAL unit.
			eMergeResult = ePACKET_ERROR;
		}
	} // end else

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		eMergeResult = ePACKET_ERROR;
	}

	return eMergeResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::H264STAPAOrH265APToVideoFrame
//
// Desc: Unpack H264 STAP-A or H265 AP packets to NAL units
//
// Abstract:
// 	Copy all NAL units in the STAP-A or AP packet into video frame buffer
// 	by adding 4 bytes size information that indicates the size of the
// 	NAL unit in bytes, followed by the NAL unit itself.
//
// Returns:
//	ePACKET_OK - merge successful
//	ePACKET_ERROR - error logged
//	ePACKET_CONTINUE - error with merge
//
EstiPacketResult CstiVideoPlayback::H264STAPAOrH265APToVideoFrame (
	CstiVideoFrame * poFrame,	// Destination Packet
	const std::shared_ptr<CstiVideoPacket> &srcPacket, // Source Packet
	const uint8_t headerSize)
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::H264STAPAOrH265APToVideoFrame);

	EstiPacketResult eMergeResult = ePACKET_OK;
	stiHResult hResult = stiRESULT_SUCCESS;

	// Were we passed valid pointers?
	if (nullptr != poFrame && nullptr != srcPacket)
	{
		IstiVideoPlaybackFrame *pFrameBuffer = poFrame->m_pPlaybackFrame;

		vpe::VideoFrame * pstSrcVideoPacket = srcPacket->m_frame;
		uint8_t *pun8Src = &pstSrcVideoPacket->buffer[headerSize];
		uint32_t un32SrcPos = 0;
		uint32_t un32SrcLength = srcPacket->unPacketSizeInBytes - headerSize;

		// Determine the total size which needs to be copied to the destination buffer
		uint32_t un32TotalLen = 0;
		while (un32SrcPos + 2 < un32SrcLength)
		{
			uint32_t un32NALUnitLen = (((uint32_t)pun8Src[un32SrcPos] << 8) + ((uint32_t)pun8Src[un32SrcPos + 1]));

			un32SrcPos += un8STAPA_NALULEN_SIZE;
			un32SrcPos += un32NALUnitLen;

			// Add 4 bytes size information for NAL unit
			un32TotalLen += sizeof(uint32_t);
			un32TotalLen += un32NALUnitLen;
		}
		
		uint32_t un32DesiredSize = pFrameBuffer->DataSizeGet() + un32TotalLen;

		if (pFrameBuffer->BufferSizeGet() < un32DesiredSize)
		{
			hResult = pFrameBuffer->BufferSizeSet(un32DesiredSize);

			if (stiIS_ERROR (hResult))
			{
				stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
							"H264STAPAOrH265APToVideoFrame: Failed to increase buffer size to %u bytes.", un32DesiredSize);
			}
		}

		uint32_t un32DstPos = 0;
		uint8_t *pun8Dst = pFrameBuffer->BufferGet() + pFrameBuffer->DataSizeGet();

		un32SrcPos = 0;

		while (un32SrcPos + 2 < un32SrcLength) // because there could be rtp padding ...
		{
			uint32_t un32NALUnitLen = (((uint32_t)pun8Src[un32SrcPos] << 8) + ((uint32_t)pun8Src[un32SrcPos + 1]));

			un32SrcPos += un8STAPA_NALULEN_SIZE;

			if (m_pVideoOutput->H264FrameFormatGet () == eH264FrameFormatByteStream)
			{
				pun8Dst[un32DstPos + 0] = 0;
				pun8Dst[un32DstPos + 1] = 0;
				pun8Dst[un32DstPos + 2] = 0;
				pun8Dst[un32DstPos + 3] = 1;
			}
			else
			{
				uint32_t n32NALUnitLenBigEndian = stiDWORD_BYTE_SWAP (un32NALUnitLen);

				// Add 4 bytes size information for NAL unit
				memcpy (&pun8Dst[un32DstPos], &n32NALUnitLenBigEndian, sizeof(uint32_t));
			}

			un32DstPos += sizeof (uint32_t);

			// Copy NAL unit itself
			memcpy (&pun8Dst[un32DstPos], &pun8Src[un32SrcPos], un32NALUnitLen);

			// Check to see if this is a keyframe.
			if ((pun8Src[un32SrcPos] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_OF_IDR)
			{
				poFrame->m_bKeyframe = true;
			}

			un32SrcPos += un32NALUnitLen;
			un32DstPos += un32NALUnitLen;
		}

		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + un32DstPos);
	} // end if
	else
	{
		stiDEBUG_TOOL (1/*g_stiVideoPlaybackPacketDebug*/,
			stiTrace ("CstiVideoPlayback::H264STAPAOrH265APToVideoFrame - Bad pointers used for H264STAPAOrH265APToVideoFrame!\n");
		);

		// Bad pointers. Return an error
		stiASSERT (estiFALSE);
		eMergeResult = ePACKET_ERROR;
	} // end else

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		eMergeResult = ePACKET_ERROR;
	}

	return (eMergeResult);
} // end CstiVideoPlayback::H264STAPAOrH265APToVideoFrame

EstiPacketResult CstiVideoPlayback::H265FUToVideoFrame (
	CstiVideoFrame * poFrame,	// Destination Packet
	const std::shared_ptr<CstiVideoPacket> &srcPacket)// Source Packet
{
	EstiPacketResult eMergeResult = ePACKET_OK;
	stiHResult hResult = stiRESULT_SUCCESS;
	
	EstiBool bStartFragment = estiFALSE;
	EstiBool bEndFragment = estiFALSE;
	
	IstiVideoPlaybackFrame *pFrameBuffer = poFrame->m_pPlaybackFrame;
	vpe::VideoFrame *pstSrcVideoPacket = srcPacket->m_frame;
	uint8_t *pun8SrcBuffer = pstSrcVideoPacket->buffer;
	
	// The FU header consists of an S bit, an E bit, and a 6-bit FuType
	// field, as shown in Figure 10.
	//
	// +---------------+
	// |0|1|2|3|4|5|6|7|
	// +-+-+-+-+-+-+-+-+
	// |S|E|  FuType   |
	// +---------------+
	//
	// Figure 10   The structure of FU header
	// The semantics of the FU header fields are as follows:
	// S: 1 bit
	// When set to one, the S bit indicates the start of a fragmented
	// NAL unit i.e. the first byte of the FU payload is also the
	// first byte of the payload of the fragmented NAL unit.  When
	// the FU payload is not the start of the fragmented NAL unit
	// payload, the S bit MUST be set to zero.
	//
	// E: 1 bit
	// When set to one, the E bit indicates the end of a fragmented
	// NAL unit, i.e. the last byte of the payload is also the last
	// byte of the fragmented NAL unit.  When the FU payload is not
	// the last fragment of a fragmented NAL unit, the E bit MUST be
	// set to zero.
	//
	// FuType: 6 bits
	// The field FuType MUST be equal to the field Type of the
	// fragmented NAL unit.

	int fu_type;
	const int32_t n32FUHeaderPos = un8RTP_H265_PAYLOAD_HEADER_SIZE;
	const int32_t n32FUPayloadStartPos = (un8RTP_H265_PAYLOAD_HEADER_SIZE + un8H265FU_HEADER_SIZE);
	const int32_t n32FUPayloadSize = srcPacket->unPacketSizeInBytes - n32FUPayloadStartPos;
	
	// To FU header and check the start & end bit
	if (0 != (pun8SrcBuffer[n32FUHeaderPos] & 0x80))
	{
		bStartFragment = estiTRUE;
	}
	else if (0 != (pun8SrcBuffer[n32FUHeaderPos] & 0x40))
	{
		bEndFragment = estiTRUE;
	}
	
	fu_type = pun8SrcBuffer[n32FUHeaderPos] & 0x3f;
	
	// Yes! Is this packet a start fragment of a new fragmented NAL unit?
	if (bStartFragment)
	{
		
		// ensure space for fragment
		uint32_t un32DesiredSize = m_bStartedFNAL ? m_un32NALUnitStart : pFrameBuffer->DataSizeGet() + // start
																sizeof(uint32_t) + sizeof(uint16_t) + // header sizes
																n32FUPayloadSize; // payload
		if (pFrameBuffer->BufferSizeGet() < un32DesiredSize)
		{
			stiHResult hResult = pFrameBuffer->BufferSizeSet(un32DesiredSize);

			if (stiIS_ERROR (hResult))
			{
				stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
							"H265FUToVideoFrame_1: Failed to increase buffer size to %u bytes.", un32DesiredSize);
			}
		}
		
		//
		// If we have started assembling a fragmented NAL Unit
		// then throw what we've done away because we
		// just got the start of another.
		//
		if (m_bStartedFNAL)
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
						   stiTrace ("VP:: SQ# %d restarted fragmented NAL.\n",
									 srcPacket->m_un32ExtSequenceNumber);
						   );
			pFrameBuffer->DataSizeSet (m_un32NALUnitStart);
		}
		else
		{
			m_bStartedFNAL = true;
		}
		
		stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
					   stiTrace ("VP:: SQ# %d is First packet of a fragmented NAL.\n",
								 srcPacket->m_un32ExtSequenceNumber);
					   );
		
		//
		// Remember where we have started the NAL unit and make room for the start code or
		// NAL unit size.
		//
		m_un32NALUnitStart = pFrameBuffer->DataSizeGet ();
		
		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + sizeof (uint32_t));
		
		//
		// Assemble the NAL Header and write it in place.
		//
		// +---------------+---------------+
		// |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |F|   Type    |  LayerId  | TID |
		// +-------------+-----------------+
		//
		// Figure 1 The structure of HEVC NAL unit header
		
		uint8_t pNALHeader[2];
		pNALHeader[0] = (pun8SrcBuffer[0] & 0x81) | (fu_type << 1);
		pNALHeader[1] = pun8SrcBuffer[1];

		// Check to see if this is a keyframe.
		if (fu_type == estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_LP ||
			fu_type == estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_RADL ||
			fu_type == estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_N_LP ||
			fu_type == estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
			fu_type == estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_N_LP ||
			fu_type == estiH265PACKET_NAL_UNIT_CODED_SLICE_CRA)
		{
			poFrame->m_bKeyframe = true;
		}
		
		// is this the only NAL header that needs to be checked?
		memcpy (pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet (),
				pNALHeader, sizeof(pNALHeader));
		
		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + sizeof (pNALHeader));
		
		//
		// Copy the payload in place.
		//
		memcpy (pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet (),
				&pun8SrcBuffer[n32FUPayloadStartPos], n32FUPayloadSize);
		
		pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + n32FUPayloadSize);
		
		m_un32FNALExpectedPacket = srcPacket->m_un32ExtSequenceNumber + 1;
	}
	else if (m_bStartedFNAL)
	{
		// Is this the packet we expect?
		if (srcPacket->m_un32ExtSequenceNumber == m_un32FNALExpectedPacket)
		{
			stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
						   stiTrace ("VP:: SQ# %d belongs to our fragmented NAL . "
									 "Merging with time stamp %d...\n",
									 srcPacket->m_un32ExtSequenceNumber,
									 poFrame->m_un32TimeStamp);
						   );
			
			if (pFrameBuffer->BufferSizeGet() < pFrameBuffer->DataSizeGet() + n32FUPayloadSize)
			{
				stiHResult hResult = pFrameBuffer->BufferSizeSet(pFrameBuffer->DataSizeGet() + n32FUPayloadSize);

				if (stiIS_ERROR (hResult))
				{
					stiTHROWMSG (stiRESULT_MEMORY_ALLOC_ERROR,
								"H265FUToVideoFrame_2: Failed to increase buffer size to %u bytes.",
								pFrameBuffer->DataSizeGet() + n32FUPayloadSize);
				}
			}
			
			memcpy (pFrameBuffer->BufferGet () + pFrameBuffer->DataSizeGet (),
					&pun8SrcBuffer[n32FUPayloadStartPos], n32FUPayloadSize);
			
			pFrameBuffer->DataSizeSet (pFrameBuffer->DataSizeGet () + n32FUPayloadSize);
			
			// Is the new packet the end of the fragmented NAL unit?
			if (bEndFragment)
			{
				stiDEBUG_TOOL (4 & g_stiVideoPlaybackPacketDebug,
							   stiTrace ("VP:: SQ# %d is the end of a fragmented NAL. \n",
										 srcPacket->m_un32ExtSequenceNumber);
							   );
				
				uint8_t *pDst = pFrameBuffer->BufferGet () + m_un32NALUnitStart;
				
				if (m_pVideoOutput->H264FrameFormatGet () == eH264FrameFormatByteStream)
				{
					pDst[0] = 0;
					pDst[1] = 0;
					pDst[2] = 0;
					pDst[3] = 1;
				}
				else
				{
					uint32_t un32Size = pFrameBuffer->DataSizeGet () - m_un32NALUnitStart - sizeof (uint32_t);
					
					pDst[3] = (un32Size & 0xFF);
					pDst[2] = ((un32Size >> 8) & 0xFF);
					pDst[1] = ((un32Size >> 16) & 0xFF);
					pDst[0] = ((un32Size >> 24) & 0xFF);
				}
				
				//
				// We have finished assembling the fragmented NAL.
				//
				m_bStartedFNAL = false;
			}
			else
			{
				++m_un32FNALExpectedPacket;
			}
		}
		else
		{
			stiDEBUG_TOOL (1 & g_stiVideoPlaybackPacketDebug,
						   stiTrace ("VP:: SQ# %d does NOT belong to our fragmented NAL ."
									 "--- Missing fragments\n",
									 srcPacket->m_un32ExtSequenceNumber);
						   );
			
			// No! this is NOT the fragment unit we expect.
			// We miss fragments for the fragmented NAL unit,
			// so throw away the current fragmented NAL unit.
			eMergeResult = ePACKET_ERROR;
		}
	} // end else
	
STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		eMergeResult = ePACKET_ERROR;
	}

	return eMergeResult;
}
	
////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::VideoFramePacketPut
//
// Description: Gives an video frame packet back to the video frame queue
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on Success, otherwise estiERROR
//
EstiResult CstiVideoPlayback::VideoFramePut (
	CstiVideoFrame *poFrame) // Empty packet being returned.
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::VideoFramePacketPut);

	EstiResult eResult = estiOK;

	if (nullptr != poFrame)
	{
		if (poFrame->m_pPlaybackFrame)
		{
			m_pVideoOutput->VideoPlaybackFrameDiscard (poFrame->m_pPlaybackFrame);
			poFrame->m_pPlaybackFrame = nullptr;
		}
		
		m_oFrameQueue.Put (poFrame);
	}

	return (eResult);
} // end CstiVideoPlayback::VideoFramePacketPut


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::VideoFrameGet
//
// Description: Get a video frame from the video frame queue
//
// Abstract:
//
// Returns:
//	CstiVideoFrame* - Pointer to a video frame
//
CstiVideoFrame * CstiVideoPlayback::VideoFrameGet ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlayback::VideoFrameGet);

	CstiVideoFrame *poFrame = nullptr;
	IstiVideoPlaybackFrame *pFrameBuffer = nullptr;
	
	m_pVideoOutput->VideoPlaybackFrameGet (&pFrameBuffer);
	
	if (pFrameBuffer)
	{
		poFrame = m_oFrameQueue.Get ();

		if (poFrame)
		{
			m_bLastFrameGetSuccessful = estiTRUE;
			
			poFrame->m_pPlaybackFrame = pFrameBuffer;
			
			pFrameBuffer->DataSizeSet (0);
		}
		else
		{
			if (m_bLastFrameGetSuccessful)
			{
				stiASSERT (estiFALSE);
			}
		
			stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
				stiTrace("CstiVideoPlayback::VideoFrameGet: m_oFrameQueue.Get () failed\n");
			);
			
			m_pVideoOutput->VideoPlaybackFrameDiscard (pFrameBuffer);
			pFrameBuffer = nullptr;
			
			m_bLastFrameGetSuccessful = estiFALSE;
		}
	}
	else
	{
		if (m_bLastFrameGetSuccessful)
		{
			stiASSERT (estiFALSE);
		}
		
		StatsLock ();
		m_Stats.un32VideoPlaybackFrameGetErrors++;
		StatsUnlock ();
		
		stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
			stiTrace("CstiVideoPlayback::VideoFrameGet: m_pVideoOutput->VideoPlaybackFrameGet failed\n");
		);
		
		m_bLastFrameGetSuccessful = estiFALSE;
	}
	
	return poFrame;
	
} // end CstiVideoPlayback::VideoFrameGet


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::VideoPacketQueueEmpty
//
// Description: Return packets from the packet queue to the empty queue.
//
// Abstract: Loop through the packet queue removing each packet and returning
//					each to the empty queue for re-use.
//
// Returns:
//	estiOK - Success, estiERROR otherwise.
//
EstiResult CstiVideoPlayback::VideoPacketQueueEmpty ()
{
	EstiResult eResult = estiOK;
	stiLOG_ENTRY_NAME (CstiVideoPlayback::VideoPacketQueueEmpty);
	
	m_processedPackets.clear ();
	
	m_jitterBuffer.clear ();
	
	return eResult;
}


/*!
 * \brief Resets the time period for collecting packet stats
 *        that are fed to flow control for controlling max
 *        receive rate via TMMBR
 */
void CstiVideoPlayback::flowControlReset ()
{
	m_flowControlStats = {};
	m_flowControlStats.startTime = std::chrono::steady_clock::now ();

	if (m_session->rtcpFeedbackTmmbrEnabledGet ())
	{
		m_flowControlTimer.restart ();
	}
}


/*!
 * \brief Sends oldest group of packets to be assembled in a frame to be played.
 */
void CstiVideoPlayback::assembleAndSendFrame()
{
	if (!m_jitterBuffer.empty() && m_bChannelOpen)
	{	
		auto frameToAssemble = m_jitterBuffer.find (nextTimeToSend (m_jitterBuffer));
		
		if (frameToAssemble != m_jitterBuffer.end() && !frameToAssemble->second->processedPackets.empty ())
		{
			auto timestamp = frameToAssemble->second->processedPackets.front()->RTPParamGet()->timestamp;
			VideoPacketToSyncQueue (frameToAssemble->second->frameComplete, &frameToAssemble->second->processedPackets);
			m_jitterBuffer.erase(timestamp);
			m_un32TicksWhenLastPlayed = stiOSTickGet ();
			m_lastAssembledFrameTS = timestamp;
		}
	}
}


/*!
 * \brief Dumps all packets in the jitter buffer, logs the loss and requests a keyframe.
 */
void CstiVideoPlayback::dumpJitterBuffer ()
{
	if (!m_jitterBuffer.empty ())
	{
		auto lastTimestamp = m_jitterBuffer.rbegin()->first;
		auto firstPacketDropped = m_jitterBuffer.begin()->second->processedPackets.front()->m_un32ExtSequenceNumber;
		auto lastPacketDropped = m_jitterBuffer.rbegin()->second->processedPackets.back()->m_un32ExtSequenceNumber;
		auto amountDropped = lastPacketDropped - firstPacketDropped + 1;
		
		stiASSERTMSG (amountDropped < 1000, "Possible Stats Error:\n"
					  "burstPacketsDropped = %zu\n"
					  "amountDropped = %zu\n"
					  "lastPacketDroppedExtSQN = %zu\n"
					  "firstPacketDroppedExtSQN = %zu\n"
					  "firstPacketSSRC = %zu\n"
					  "firstPacketTS = %zu\n"
					  "lastPacketSSRC = %zu\n"
					  "lastPacketTS = %zu",
					  m_Stats.burstPacketsDropped, amountDropped, lastPacketDropped, firstPacketDropped,
					  m_jitterBuffer.begin()->second->processedPackets.front()->RTPParamGet()->sSrc,
					  m_jitterBuffer.begin()->second->processedPackets.front()->RTPParamGet()->timestamp,
					  m_jitterBuffer.rbegin()->second->processedPackets.front()->RTPParamGet()->sSrc,
					  m_jitterBuffer.rbegin()->second->processedPackets.front()->RTPParamGet()->timestamp);
		
		StatsLock ();
		m_Stats.burstPacketsDropped += amountDropped;
		StatsUnlock ();
		
		m_jitterBuffer.clear();
		KeyframeRequest (lastTimestamp);
	}
}


/*!
 * \brief Stores groups of packets ready to be played and controls the rate to be assembled into frames based on jitterbuffer size.
 */
void CstiVideoPlayback::jitterCalculate ()
{
	stiDEBUG_TOOL (g_stiNackAvgNumFramesToDelay,
		m_framesToBuffer = g_stiNackAvgNumFramesToDelay;
	);
	
	const int minFramesInBuffer = m_framesToBuffer;
	const int bufferSize = m_jitterBuffer.size ();
	const int maxBufferSize = 40; // Don't allow playback delay beyond this.
	int frameDiff = 0;
	const auto largestDelay = 1000 / 15; // Don't allow a frameDiff greater than 15 FPS;
	
	if (bufferSize > maxBufferSize)
	{
		dumpJitterBuffer ();
	}
	else if (bufferSize)
	{
		auto timestamp = nextTimeToSend (m_jitterBuffer);

		frameDiff = (timestamp - m_lastAssembledFrameTS) / 90;

		if (bufferSize <= minFramesInBuffer)
		{
			// Ensure the frameDiff does not produce a playback slower than 15 FPS.
			frameDiff = std::min(frameDiff, largestDelay);
		}

		uint32_t timeToPlay {m_un32TicksWhenLastPlayed};
		if (bufferSize < minFramesInBuffer) // Slow playback to get at least minFramesInBuffer
		{
			timeToPlay += static_cast<uint32_t>(1.15 * frameDiff);
		}
		else if (bufferSize == minFramesInBuffer) // Playback as recorded for minFramesInBuffer
		{
			timeToPlay += frameDiff; 
		}
		else // Speed up playback because buffer is getting too large
		{
			timeToPlay += static_cast<uint32_t>(0.90 * frameDiff);
		}

		stiDEBUG_TOOL (g_stiVideoPlaybackDebug > 1,
			stiTrace ("Frames in jitter buffer: %d, Playback Time: %u, Current Time: %u\n",
				bufferSize, timeToPlay, stiOSTickGet ());
		);
		
		auto currentTime = stiOSTickGet ();
		if (currentTime >= timeToPlay || m_playbackRead.NumEmptyBufferAvailable () == 0)
		{
			m_Stats.playbackDelay += bufferSize;
			assembleAndSendFrame ();
		
			if (!m_jitterBuffer.empty ())
			{
				m_jitterTimer.timeoutSet (JITTER_TIMER_AFTER_SEND);
				m_jitterTimer.restart ();
			}
		}
		else
		{
			m_jitterTimer.timeoutSet (timeToPlay - currentTime);
			m_jitterTimer.restart ();
		}
	}
}


/*!
 * \brief Sends the oldest group of processed packets to be put in a frame regardless if frame is missing data.
 */
void CstiVideoPlayback::oldestProcessedPacketsToFrame (bool frameVerifiedComplete)
{
	
	if (!m_processedPackets.empty ())
	{
		//We are backed up lets force something through.
		// Check for loss and force the oldest timestamp through.
		auto timestamp = nextTimeToSend (m_processedPackets);
		auto processedPacketsIter = m_processedPackets.find(timestamp);
		
		if (processedPacketsIter != m_processedPackets.end() && ((m_lastSeqNumInBuffer < processedPacketsIter->second->processedPackets.back()->m_un32ExtSequenceNumber) || !m_lastSeqNumInBuffer))
		{
			bool frameIsComplete = frameVerifiedComplete;
			
			if (!frameVerifiedComplete)
			{
				frameIsComplete = packetsCompleteFrame (processedPacketsIter->second);
			}
			
			processedPacketsIter->second->frameComplete = frameIsComplete;
			m_lastSeqNumInBuffer = processedPacketsIter->second->processedPackets.back()->m_un32ExtSequenceNumber;
			
			if (m_jitterBuffer.empty ())
			{
				m_jitterTimer.timeoutSet (JITTER_TIMER_MS);
				m_jitterTimer.restart ();
			}			
			// If we get two different frames with the same timestamp this will cause a problem.
			else if (m_jitterBuffer.find (timestamp) != m_jitterBuffer.end())
			{
				stiASSERTMSG (false, "Timestamp %u already exists in jitter buffer %d packets lost",
							  timestamp, m_jitterBuffer.find(timestamp)->second->processedPackets.size());
			}

			m_jitterBuffer[timestamp] = std::move(processedPacketsIter->second);
			
			m_processedPackets.erase(timestamp);
		}
		else if (processedPacketsIter != m_processedPackets.end() && !processedPacketsIter->second->processedPackets.empty ())
		{
			m_processedPackets.erase(timestamp);
		}
	}
}


/*!
 * \brief Sends the oldest group of processed packets to be put in a frame only if the group is not missing packets.
 */
void CstiVideoPlayback::checkOldestPacketsSendCompleteFrame ()
{
	if (!m_processedPackets.empty ())
	{
		auto nextFrameToSend = m_processedPackets.find (nextTimeToSend (m_processedPackets));
		if (nextFrameToSend != m_processedPackets.end () && packetsCompleteFrame (nextFrameToSend->second))
		{
			oldestProcessedPacketsToFrame (true);
			
			// See if we have more frames to send.
			PostEvent (std::bind (&CstiVideoPlayback::EventCheckForCompleteFrame, this));
		}
	}
}


/*!
 * \brief Checks if a group of processed packets complete a frame
 */
bool CstiVideoPlayback::packetsCompleteFrame (const std::unique_ptr<ReceivedPackets> &packetsToCheck)
{
	bool completeFrame = false;
	
	if (packetsToCheck)
	{
		uint32_t first = (*packetsToCheck).processedPackets.front()->m_un32ExtSequenceNumber;
		uint32_t last = (*packetsToCheck).processedPackets.back()->m_un32ExtSequenceNumber;
		uint32_t startOfNextFrame = 0;
		uint32_t timeStamp = (*packetsToCheck).processedPackets.front()->m_stRTPParam.timestamp;
		bool gapInSqn = ((last - first + 1) != (*packetsToCheck).processedPackets.size ());
		auto nextPacketGroup = m_processedPackets.find (timeStamp);
		nextPacketGroup++;
		
		if (m_processedPackets.size() > 1 && nextPacketGroup == m_processedPackets.end())
		{
			nextPacketGroup = m_processedPackets.begin();
		}
		
		if (nextPacketGroup != m_processedPackets.end())
		{
			auto& nextprocessedPackets = nextPacketGroup->second;
			
			if (nextprocessedPackets && !nextprocessedPackets->processedPackets.empty ())
			{
				startOfNextFrame = nextprocessedPackets->processedPackets.front()->m_un32ExtSequenceNumber;
			}
		}
		
		// Check to see if we have a full frame.  This is the case if:
		// 1- we are not missing any packets
		// 2- the first SQ# is one greater than the last SQ# of the previous frame
		// 3- the first SQ# of the next frame is one greater than the last SQ# of this frame
		if (!gapInSqn &&
			(!m_lastSeqNumInBuffer || m_lastSeqNumInBuffer + 1 == first) &&
			((*packetsToCheck).markerSet || startOfNextFrame == last + 1))
		{
			completeFrame = true;
		}
	}
	else
	{
		stiASSERTMSG (estiFALSE, "Called packetsCompleteFrame with invalid packet group, current group size = %d", m_processedPackets.size ());
	}
	
	return completeFrame;
}


/*!
 * \brief Finds the next timestamp we need to send from the passed in frame container.
 *
 * Since timestamps can wrap we cannot always assume that the first (smallest) timestamp
 * is actually the next timestamp to send. This checks the sequence numbers from the beginning and end
 * if the sequence number of the beginning is higher than the end we need to compare sequence numbers to find
 * the next frame to send.
 */
uint32_t CstiVideoPlayback::nextTimeToSend (const std::map<uint32_t, std::unique_ptr<ReceivedPackets>> &frames)
{
	uint32_t timestamp = 0;
	
	if (!frames.empty ())
	{
		// Make sure timestamps haven't wrapped, if this happens the beginning will have a higher sequence number than the end.
		if (frames.begin()->second->processedPackets.back()->m_un32ExtSequenceNumber >
			frames.rbegin ()->second->processedPackets.back()->m_un32ExtSequenceNumber)
		{
			stiASSERTMSG(false, "\n\n We wrapped \nFirst TS = %u SEQ# = %d Payload = %d SSRC = %u \nLast TS = %u SEQ# = %d Payload = %d SSRC = %u\n\n",
				frames.begin()->second->processedPackets.back()->RTPParamGet()->timestamp,
				frames.begin()->second->processedPackets.back()->RTPParamGet()->sequenceNumber,
				frames.begin()->second->processedPackets.back()->RTPParamGet()->payload,
				frames.begin()->second->processedPackets.back()->RTPParamGet()->sSrc,
				frames.rbegin ()->second->processedPackets.back()->RTPParamGet()->timestamp,
				frames.rbegin ()->second->processedPackets.back()->RTPParamGet()->sequenceNumber,
				frames.rbegin ()->second->processedPackets.back()->RTPParamGet()->payload,
				frames.rbegin ()->second->processedPackets.back()->RTPParamGet()->sSrc);

			for (auto framesIter = frames.rbegin(); framesIter != frames.rend (); framesIter++)
			{
				timestamp = framesIter->first;
				auto nextReversedFrame = framesIter;
				nextReversedFrame++;
				
				if (nextReversedFrame != frames.rend ())
				{
					// If the next frame has a higher sequence number we are on the oldest frame and this timestamp needs to be sent.
					if (framesIter->second->processedPackets.back()->m_un32ExtSequenceNumber < nextReversedFrame->second->processedPackets.back()->m_un32ExtSequenceNumber)
					{
						break;
					}
				}
			}
		}
		else
		{
			timestamp = frames.begin()->first;
		}
	}
	else
	{
		stiASSERTMSG (false, "CstiVideoPlayback::nextTimeToSend should not be called with an empty map");
	}
	
	return timestamp;
}
