/*!
 * \file CstiVideoRecord.cpp
 * \brief Manages flow of video data from the VideoInput platform driver on
 *        through the SIP stack
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2000 – 2024 Sorenson Communications, Inc. -- All rights reserved
 */

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"
#include "rvrtpnatfw.h"
#include "rvtimer.h"
#include "rvmutex.h"
#include "rtcpfbplugin.h"

#include <numeric>
#include <memory>
#include <cstdio>
#include <ctime>
#include <cmath>
#ifndef WIN32
#include <sys/time.h>
#endif
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "RtpPayloadAddon.h"
#include "CstiVideoRecord.h"
#include "stiTaskInfo.h"
#include "stiTools.h"
#include "IstiVideoInput.h"
#include "videoDefs.h"

#if APPLICATION == APP_NTOUCH_PC
#include "CstiNativeVideoLink.h"
#elif APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_DHV_IOS
#include "AppleCalculateCaptureSize.h"
#endif

#ifdef WIN32
	#define round(A) floor (A + .5)
#endif

#include "Packetization.h"

//
// Constants
//
const int nVIDEO_PRIVACY_HOLD_BURST_COUNT = 10;

// Interval at which the sender flow control will check to see
// if it needs to increase its send rate.  The check is cheap
// and there's no harm in doing so frequently since increases
// will not occur if enough time has not elapsed
const int RECORD_FLOW_CONTROL_INTERVAL_MS = 1000;

// Interval at which autospeed flow control is run when TMMBR is disabled
const int AUTOSPEED_INTERVAL_MS = 5000;
const int AUTOSPEED_INTERVAL_WITH_REMB_MS = 2000;


struct SstiFrameSizeCaps
{
	int nRows;
	int nColumns;
	int nMaxFrameBytes;
};


std::map<std::string, SstiFrameSizeCaps> g_frameSizeCaps =
{
	{"CIF", {unstiCIF_ROWS, unstiCIF_COLS, nMAX_CIF_BYTES_PER_CODED_PICTURE}},
	{"QCIF", {unstiQCIF_ROWS, unstiQCIF_COLS, nMAX_QCIF_BYTES_PER_CODED_PICTURE}},
	{"SQCIF", {unstiSQCIF_ROWS, unstiSQCIF_COLS, nMAX_SQCIF_BYTES_PER_CODED_PICTURE}},
};


//
// Typedefs
//

//
// Globals
//
#ifdef stiDEBUGGING_TOOLS

const char* szVRFileIn = "/home/sent_bitstream.test";
FILE * pVRFileIn;

#endif


///////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::CstiVideoRecord
//
//  Description:
//
//  Abstract:
//
//
//  Returns:
//
CstiVideoRecord::CstiVideoRecord (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t callIndex,
	uint32_t initialMaxRate,
	ECodec codec,
	EstiPacketizationScheme packetizationScheme,
	RvSipMidMgrHandle sipMidMgr,
	const RTCPFlowcontrol::Settings &flowControlSettings,
	const std::string &callId,
	EstiInterfaceMode localInterfaceMode,
	uint32_t reservedBitRate)
:
	vpe::MediaRecord<EstiVideoCodec, SstiVideoRecordSettings>(
		stiVR_TASK_NAME,
		session,
		callIndex,
		initialMaxRate,
		codec,
		estiVIDEO_NONE,
		nMAX_VP_BUFFERS,
		stiMAX_VIDEO_RECORD_PACKET_BUFFER
		),
	m_nHoldPrivacyPacketBurst (nVIDEO_PRIVACY_HOLD_BURST_COUNT),
	m_holdPrivacyTimer (0, this),
	m_session (session),
	m_flowControlTimer (RECORD_FLOW_CONTROL_INTERVAL_MS, this),
	m_flowControl (flowControlSettings.publicIPv4, initialMaxRate),
	m_autoSpeedTimer (AUTOSPEED_INTERVAL_MS, this),
	m_autoSpeedFlowControl (512000, flowControlSettings, callId),
	m_localInterfaceMode (localInterfaceMode)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::CstiVideoRecord);

	PacketizationSchemeSet (packetizationScheme);

	m_signalConnections.push_back (m_holdPrivacyTimer.timeoutSignal.Connect (
			std::bind(&CstiVideoRecord::EventHoldPrivacyTimer, this)));

	m_signalConnections.push_back (m_flowControlTimer.timeoutSignal.Connect (
			std::bind(&CstiVideoRecord::EventFlowControlTimer, this)));

	m_signalConnections.push_back (m_autoSpeedTimer.timeoutSignal.Connect (
			std::bind(&CstiVideoRecord::EventAutoSpeedTimer, this)));

	// Notify Radvision that we'll be using our event loop thread to access
	// their SIP APIs.  This must be called from within the event loop thread
	if (sipMidMgr)
	{
		m_signalConnections.push_back (startedSignal.Connect (
				[this, sipMidMgr]{ RvSipMidAttachThread (sipMidMgr, m_taskName.c_str()); }));

		// When the event loop is shutdown, tell Radvision we are done using that thread
		// to access its SIP APIs
		m_signalConnections.push_back (shutdownSignal.Connect (
				[sipMidMgr]{ RvSipMidDetachThread (sipMidMgr); }));
	}
	
	m_flowControl.reservedBitRateSet (reservedBitRate);

	stiOSMinimumTimerResolutionSet ();
}

///////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::~CstiVideoRecord
//
//  Description:
//
//  Abstract:
//
//
//  Returns:
//
CstiVideoRecord::~CstiVideoRecord ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::~CstiVideoRecord);

	stiOSMinimumTimerResolutionReset ();
	
	Shutdown ();

#ifdef stiENABLE_MULTIPLE_VIDEO_INPUT
	// Release the instance of the VideoInput object
	if (m_pVideoInput)
	{
		m_pVideoInput->instanceRelease ();
		m_pVideoInput = NULL;
	}
#endif //stiENABLE_MULTIPLE_VIDEO_INPUT

	if (m_pHoldPrivacyPacket)
	{
		delete m_pHoldPrivacyPacket->pData;
		delete m_pHoldPrivacyPacket;
		m_pHoldPrivacyPacket = nullptr;
	}

} // end destructor


/*!
 * \brief Initializes and starts the video record task
 */
stiHResult CstiVideoRecord::Initialize(
	bool sorensonPrivacy,
	bool mcuConnection)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::Initialize);

	stiDEBUG_TOOL (g_stiVideoRecordDebug,
		stiTrace ("CstiVideoRecord::Initialize\n");
	); // stiDEBUG_TOOL

	stiHResult hResult = stiRESULT_SUCCESS;

	m_bSorensonPrivacy = sorensonPrivacy;
	m_bMCUConnection = mcuConnection;

	FlowControlRateSet (MaxChannelRateGet());

	//
	// Open video device driver
	//

	// Get instance of the video input device
#ifdef stiENABLE_MULTIPLE_VIDEO_INPUT
	m_pVideoInput = IstiVideoInput::instanceCreate(m_callIndex);
#else
	m_pVideoInput = IstiVideoInput::InstanceGet();
#endif //stiENABLE_MULTIPLE_VIDEO_INPUT
	stiTESTCOND (nullptr != m_pVideoInput, stiRESULT_TASK_INIT_FAILED);
	m_packetPool.create (stiMAX_VR_BUFFERS, stiMAX_RTP_PACKET_OVERHEAD + stiMAX_VIDEO_RECORD_PACKET);

	// Call the base class intitialization function
	hResult = CstiDataTaskCommonP::Initialize ();
	stiTESTRESULT ();

	Startup ();

STI_BAIL:

	return (hResult);

} // end CstiVideoRecord::Initialize


/*!
* \brief Close the data tasks
*/
void CstiVideoRecord::Close ()
{
	DataChannelClose ();
}



/*!
 * \brief Event handler that gets called when the holdPrivacyTimer fires
 * \return stiHResult
 */
void CstiVideoRecord::EventHoldPrivacyTimer ()
{
	if (m_bChannelOpen && m_nMuted != 0)
	{
		HoldPrivacyPacketProcess ();
		TimerStart ();
	}
}


/*!
 * \brief Handler for when the flow control timer fires.
 *        This is used to increase our current send rate
 *        when RecordFlowControl tells us we should.
 */
void CstiVideoRecord::EventFlowControlTimer ()
{
	// Do nothing if the data channel was closed
	if (!m_bChannelOpen)
	{
		return;
	}

	auto currentSendRate = m_flowControl.sendRateGet ();
	auto newSendRate = m_flowControl.sendRateCalculate ();

	// If the rate changed, notify for the rate to be changed
	if (newSendRate != currentSendRate)
	{
		CurrentBitRateSet (newSendRate);
	}

	m_flowControlTimer.restart ();
}


/*!
 * \brief Handler for when the autospeed timer fires.
 *        This is used to control our current send rate
 *        when tmmbr is not enabled.
 */
void CstiVideoRecord::EventAutoSpeedTimer ()
{
	// Perform auto speed, only if TMMBR is not in use
	if (!m_session->rtcpFeedbackTmmbrEnabledGet())
	{
		// Lock the receiver report list mutex
		std::lock_guard<std::mutex> lock (m_reportMutex);

		if (!m_receiverReports.empty ())
		{
			uint32_t averageRTPPacketSize = AveragePacketSizeSentGet ();
			uint32_t currentRate = CurrentBitRateGet ();

			SstiRTCPInfo rtcpInfo = {};
			rtcpInfo.n32CumulativePacketsLost = m_receiverReports.front ().n32CumulativePacketsLost;
			rtcpInfo.un32SequenceNumber = m_receiverReports.front ().un32SequenceNumber;
			rtcpInfo.un32LastSR = m_receiverReports.front ().un32LastSR;
			rtcpInfo.un32DelaySinceLastSR = m_receiverReports.front ().un32DelaySinceLastSR;

			// Average the fraction lost fields (un32PacktsLost is a poorly named fraction...)
			uint32_t sum = std::accumulate (m_receiverReports.begin(), m_receiverReports.end(), 0,
			                                [](uint32_t sum, const SstiRTCPInfo &report){ return sum + report.un32PacketsLost; });
			rtcpInfo.un32PacketsLost = sum / m_receiverReports.size();

			uint32_t newRate = m_autoSpeedFlowControl.FlowControlRecommend (
						&rtcpInfo,
						averageRTPPacketSize,
						currentRate,
						FlowControlRateGet(),
						m_remb);

			stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
				stiTrace ("EventAutoSpeedTimer - AutoSpeed Recommends Rate [newRate: %u  currentRate: %u]\n", newRate, currentRate);
			);

			if (newRate != currentRate)
			{
				CurrentBitRateSet (newRate);
			}

			m_receiverReports.clear ();
		}
	}

	// Fail-safe to ensure this list doesn't grow out of control
	constexpr size_t maxReports = 100;

	if (m_receiverReports.size() > maxReports)
	{
		stiASSERTMSG (false, "Receiver reports are not being consumed.  This indicates a bug.\n");
		m_receiverReports.clear();
	}

	if (m_remb)
	{
		m_autoSpeedTimer.timeoutSet (AUTOSPEED_INTERVAL_WITH_REMB_MS);
		m_remb = 0;
	}
	else
	{
		m_autoSpeedTimer.timeoutSet (AUTOSPEED_INTERVAL_MS);
	}

	m_autoSpeedTimer.restart ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::TimerStart
//
// Description: Starts a timer for a key frame request.
//
// Abstract:
//	This functions performs all the duties required in order to start a timer
//	so we can monitor the key frame request.
//
// Returns:
//	None.
//
void CstiVideoRecord::TimerStart ()
{
	stiLOG_ENTRY_NAME(CstiVideoPlayback::TimerStart);

	if (m_nHoldPrivacyPacketBurst <= 0)
	{
		m_holdPrivacyTimer.timeoutSet (1500); // Send a frame every 1.5 seconds
	}
	else
	{
		m_nHoldPrivacyPacketBurst--;
		m_holdPrivacyTimer.timeoutSet (350); // Send a frame every 350 ms
	}
	m_holdPrivacyTimer.restart ();
}


/*!
 * \brief Event handler for when VideoInput notifies us there is a packet
 */
void CstiVideoRecord::EventPacketAvailable ()
{
	stiDEBUG_TOOL (g_stiVideoTracking,
		static bool bStartTimer = true;
		static timeval start;
		static uint32_t un32Frames = 0;

		if (bStartTimer)
		{
			gettimeofday (&start, nullptr);
			un32Frames = 0;
			bStartTimer = false;
		}

		++un32Frames;

		if (un32Frames >= 1000)
		{
			timeval result{};
			timeval end{};
			gettimeofday (&end, nullptr);

			timersub (&end, &start, &result);

			float fFrameRate = un32Frames / (result.tv_sec + (result.tv_usec / 1000000.0F));

			stiTrace ("VideoRecord: Average fps: %0.2f\n", fFrameRate);

			gettimeofday (&start, nullptr);
			un32Frames = 0;
		}
	);

	PacketFromDriverProcess ();
}


/*!
 * \brief Starts the event loop and preps Radvision SIP
 */
void CstiVideoRecord::Startup ()
{
	StartEventLoop ();
}


/*!
 * \brief Shuts down the data task cleanly
 */
void CstiVideoRecord::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::Shutdown);

	stiDEBUG_TOOL (g_stiVideoRecordDebug,
		stiTrace ("CstiVideoRecord::Shutdown\n");
	);

	// Reset the flag that the record task keys off of.
	m_bCaptureVideo = estiFALSE;

	DataChannelClose ();

	StopEventLoop ();
}


stiHResult CstiVideoRecord::Setup ()
{
	//
	// Set video codec settings into driver
	//
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoRecordDebug,
		stiTrace ("CstiVideoRecord::Setup: Set Picture size to %ux%u, frame rate = %u, target bitrate = %u, profile = 0x%x, constraints = 0x%x\n",
				m_stVideoSettings.unColumns,
				m_stVideoSettings.unRows,
				m_stVideoSettings.unTargetFrameRate,
				m_stVideoSettings.unTargetBitRate,
				m_stVideoSettings.eProfile,
				m_stVideoSettings.unConstraints);
	); // stiDEBUG_TOOL

	// Set encode settings
	hResult = m_pVideoInput->VideoRecordSettingsSet (&m_stVideoSettings);
	stiTESTRESULT ();

	// Request a key frame
	hResult = m_pVideoInput->KeyFrameRequest ();
	stiTESTRESULT ();

STI_BAIL:

	m_nPacketLossExceededCount = 0;

	return hResult;
}


///\Brief Retrieves the privacy state of the data task.
///\Brief Since privacy is a device setting, get it from the VideoInput driver.
///
bool CstiVideoRecord::PrivacyGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	EstiBool bPrivacy;

	m_pVideoInput->PrivacyGet (&bPrivacy);
	return bPrivacy == estiTRUE;
}


/*!
 * \brief Event handler to start recording video data
 */
void CstiVideoRecord::EventDataStart ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::EventDataStart);

	if (!m_bVideoRecordStart)
	{
		stiHResult hResult = m_pVideoInput->VideoRecordStart();
		if (!stiIS_ERROR(hResult))
		{
			m_bVideoRecordStart = estiTRUE;
		}
	}

	if (m_bVideoRecordStart)
	{
		// Is the video mute off (privacy and local)?
		if (0 == (m_nMuted & estiMUTE_REASON_PRIVACY) &&
			0 == (m_nMuted & estiMUTE_REASON_HOLD) &&
			0 == (m_nMuted & estiMUTE_REASON_DHV))
		{
			stiDEBUG_TOOL (g_stiVideoRecordDebug,
				stiTrace ("\nCstiVideoRecord::DataStartHandle\n");
			)	; // stiDEBUG_TOOL

			// Request a Key Frame
			RequestKeyFrame ();

			// Indicate that we have started sending data
			m_bCaptureVideo = estiTRUE;
#if 0
			// Indicate the number of packets need to be flushed
			m_nFlushFrames = nUNMUTE_FLUSH_VR_FRAMES;
			m_bCheckKeyFrame = estiFALSE;
#else
			// Indicate the number of packets need to be flushed
			m_nFlushFrames = 0;
			m_bCheckKeyFrame = estiTRUE;
#endif

		} // end if
	} // end else

	EstiBool bPrivacy;

	m_pVideoInput->PrivacyGet (&bPrivacy);

	if (bPrivacy)
	{
		Mute (estiMUTE_REASON_PRIVACY);
	}
}


/*!
 * \brief Event handler for when the video record settings have been changed,
 *        and need to be passed down to the hardware
 * \param settings
 */
void CstiVideoRecord::videoSettingsSet (
	const IstiVideoInput::SstiVideoRecordSettings &settings)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::videoSettingsSet);

	stiDEBUG_TOOL (g_stiVideoRecordDebug,
		stiTrace ("CstiVideoRecord::videoSettingsSet\n");
	); // stiDEBUG_TOOL

	// Have the settings changed?
	if (m_stVideoSettings != settings)
	{
		// Update the stored structure to reflect the new video settings
		stiDEBUG_TOOL (g_stiVideoRecordDebug,
			stiTrace ("videoSettingsSet: Capture size from %d x %d to %d x %d, Frame Rate from %d to %d \n",
			m_stVideoSettings.unColumns, m_stVideoSettings.unRows,
			settings.unColumns, settings.unRows,
			m_stVideoSettings.unTargetFrameRate, settings.unTargetFrameRate);
		); // stiDEBUG_TOOL

		m_stVideoSettings = settings;

		// Only inform video input of the settings if the "channel" is open.
		if (m_bChannelOpen)
		{
			auto hResult = m_pVideoInput->VideoRecordSettingsSet (&m_stVideoSettings);
			
			std::unique_lock<std::mutex> lk {m_rtpSender->mutexGet()};
			m_rtpSender->pacingBitRateSet(static_cast<size_t>(settings.unTargetBitRate * stiPACING_RATE_FACTOR));
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return;
}


//:-----------------------------------------------------------------------------
//:
//: Supporting Functions
//:
//:-----------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////

stiHResult CstiVideoRecord::PacketConfigure (std::shared_ptr<vpe::RtpPacket> packet, bool isKeyframe, bool isLastInFrame, uint32_t timestamp)
{
	RvRtpParam *param = packet->rtpParamGet ();
	RvRtpParamConstruct (param);
	param->payload = m_Settings.payloadTypeNbr;
	param->marker = isLastInFrame ? 1 : 0;
	param->timestamp = timestamp;
	param->sByte = packet->headerSizeGet ();
	packet->isKeyframeSet (isKeyframe);
	
	return stiRESULT_SUCCESS;
}


 stiHResult CstiVideoRecord::PacketsProcess (SstiRecordVideoFrame *frame, uint32_t rtpTimestamp, bool *partialSend)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	vpe::Packetization packetization { rtpTimestamp, frame, PacketizationSchemeGet () };
	
	size_t totalRTPBytes = 0;
	auto currentTime = std::chrono::steady_clock::now ();
	
	{
		std::lock_guard<std::mutex> lk{ m_rtpSender->mutexGet () };
		if (!m_sendQueue->isEmptyGet () && currentTime - m_sendQueue->oldestTimestampGet () > std::chrono::milliseconds{ 500 })
		{
			// We're getting really far behind on sending packets, so clear the queue.
			m_sendQueue->clear ();
		}
	}
	
	while (!packetization.isEmptyGet ())
	{
		uint8_t *data;
		uint32_t length;
		
		hResult = packetization.nextRtpPayloadGet (&data, &length);
		stiTESTRESULT();
		
		{
			auto packet = m_packetPool.pop();
			stiTESTCONDMSG(packet != nullptr, stiRESULT_ERROR, "Warning: No more RTP packets left in pool, discarding packet.");

			{
				std::lock_guard<std::mutex> lk{ m_rtpSender->mutexGet() };

				packet->payloadSizeSet(length);
				packet->headerHeadroomSet(stiMAX_RTP_PACKET_OVERHEAD); // Extra headroom before the header allows us to insert the NACK header when reusing the packet
				packet->headerSizeSet(m_RTPHeaderOffset.RtpHeaderOffsetLengthGet());
				std::copy_n(data, length, packet->payloadGet());
				PacketConfigure(packet, frame->keyFrame, packetization.isEmptyGet(), rtpTimestamp);
				m_sendQueue->push(packet, currentTime);
				totalRTPBytes += packet->headerSizeGet() + packet->payloadSizeGet();
			}
		}
	}
	
	m_sendQueue->mediaFrameCreated.Emit (totalRTPBytes);
	m_rtpSender->packetPacingUpdate ();
	
STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::RequestKeyFrame
//
// Description: Request a key frame from driver
//
// Abstract:
//	This function request a key frame from hardware
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiVideoRecord::RequestKeyFrame ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::RequestKeyFrame);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	stiHResult hResult = stiRESULT_SUCCESS;
	const uint32_t un32DELAY_BETWEEN_KEYFRAMES = 300;
	uint32_t un32CurTime = stiOSTickGet ();

	// Don't pass the request on unless it has been at least the minimum delay
	// time since the last request.
	if (!m_bKeyFrameRequested || m_un32PrevTime <= un32CurTime - un32DELAY_BETWEEN_KEYFRAMES)
	{
		stiDEBUG_TOOL (g_stiVideoRecordDebug || g_stiVideoRecordKeyframeDebug,
				stiTrace ("VR:: Requesting a KeyFrame from DSP...\n");
		); // stiDEBUG_TOOL

		m_bKeyFrameRequested = estiTRUE;
		if (!m_bSorensonPrivacy && m_nMuted != 0)
		{
			m_holdPrivacyTimer.stop ();
			PostEvent (
				std::bind(&CstiVideoRecord::EventHoldPrivacyTimer, this));
		}
		else
		{
			hResult = m_pVideoInput->KeyFrameRequest ();
		}
		m_un32PrevTime = un32CurTime;

		StatsLock ();
		m_Stats.totalKeyframesRequested++;
		m_Stats.keyframesRequested++;
		m_Stats.un32KeyframeRequestedBeginTick = stiOSTickGet ();
		StatsUnlock ();
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::HoldPrivacyPacketProcess
//
// Description: Read a packet from the Hold/Privacy Video Files
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiVideoRecord::HoldPrivacyPacketProcess()
{
	stiLOG_ENTRY_NAME(CstiVideoRecord::HoldPrivacyPacketProcess);
	stiHResult hResult = stiRESULT_ERROR;
	if (m_bChannelOpen)
	{
		SstiRecordVideoFrame videoFrame;
		uint32_t un32RTPTimeStamp;

		// Offset into the buffer to the start of the actual data
		videoFrame.buffer = m_videoBuffer.data () + m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();
		videoFrame.unBufferMaxSize = m_videoBuffer.size () - m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();
		
		if ((m_nMuted & estiMUTE_REASON_HOLD) != 0)
		{
			hResult = HoldPrivacyVideoPacketGet (&videoFrame, EstiStatusFrameHold);
			stiTESTRESULT();
		}
		else if ((m_nMuted & estiMUTE_REASON_PRIVACY) != 0)
		{
			hResult = HoldPrivacyVideoPacketGet (&videoFrame, EstiStatusFramePrivacy);
			stiTESTRESULT();
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}

		// The m_nVideoClockRate constant is used to assure constant
		// video regardless of the speed of the system.
		un32RTPTimeStamp = (stiOSTickGet() - m_un32RTPInitTime) * GetVideoClockRate();
		hResult = PacketsProcess (&videoFrame, un32RTPTimeStamp, nullptr);
	}

STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::PacketFromDriverProcess
//
// Description: Read and process a packet from the video driver
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiVideoRecord::PacketFromDriverProcess ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::PacketFromDriverProcess);
	stiHResult hResult = stiRESULT_ERROR;
	static int nCheckKeyFrame = 0;

	if (m_bCaptureVideo && m_bChannelOpen)
	{
		SstiRecordVideoFrame videoFrame;
		uint32_t un32RTPTimeStamp;

		// Offset into the buffer to the start of the actual data
		videoFrame.buffer = m_videoBuffer.data() + m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();
		videoFrame.unBufferMaxSize = m_videoBuffer.size() - m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();

		//
		// Get actual Video Record Frame from PLATFORM Video Input/Capture (copy into our local pstCurrentVideoFrame)
		//
		hResult = m_pVideoInput->VideoRecordFrameGet(&videoFrame);
		stiTESTRESULT ();

		//
		// Process and send the video packet
		//
		if (videoFrame.frameSize > 0 && m_nMuted == 0)
		{
			// When the m_nFlushFrames become 0, the next video frame
			// should be a new frame & key frame. Otherwise keep flushing video frames
			// till getting a key frame
			if(m_bCheckKeyFrame)
			{
				m_bCheckKeyFrame = estiFALSE;

				if(estiFALSE == videoFrame.keyFrame)
				{
					m_nFlushFrames++;
					nCheckKeyFrame++;
				}
				else
				{
					nCheckKeyFrame = 0;
				}
			}

			if (m_bKeyFrameRequested && videoFrame.keyFrame)
			{
				m_bKeyFrameRequested = estiFALSE;

				stiDEBUG_TOOL (g_stiVideoRecordDebug,
					stiTrace ("CstiVideoRecord::PacketFromDriverProcess Got a keyframe\n");
				);
			}

			if (0 == m_nFlushFrames)
			{
				stiDEBUG_TOOL (0,
					stiTrace ("CstiVideoRecord::PacketFromDriverProcess pstCurrentVideoFrame->frameSize = %d\n", (int) videoFrame.frameSize);
				);

				// Determine the timestamp for each packet. Time stamp is
				// based on a 90 kHz clock. Only updates on a new frame.

				// The m_nVideoClockRate constant is used to assure constant
				// video regardless of the speed of the system.
				// TODO: We need to change from stiOSTickGet to something that will never allow for two frames processed back to back
				// ending up with the same timestamp as it can today.
				
				un32RTPTimeStamp = (stiOSTickGet() - m_un32RTPInitTime) * GetVideoClockRate();
				
				// If we have the same timestamp that we just used bump it by one to ensure the frames are seen as the separate frames they are.
				if (un32RTPTimeStamp <= m_previousTimeStamp)
				{
					un32RTPTimeStamp = m_previousTimeStamp + 1;
				}
				
				m_previousTimeStamp = un32RTPTimeStamp;
				
				bool partialFrameSent = false;

				hResult = PacketsProcess (&videoFrame, un32RTPTimeStamp, &partialFrameSent);
				
				// Check hResult of PacketsProcess methods above. If success and partial frame, update stat.
				if (!stiIS_ERROR (hResult) && partialFrameSent)
				{
					StatsLock ();
					m_Stats.countOfPartialFramesSent++;
					StatsUnlock ();
				}
			} // end if(m_bCaptureVideo && 0 == m_nFlushFrames)
			else
			{
				stiDEBUG_TOOL ( g_stiVideoRecordDebug,
					stiTrace ("CstiVideoRecord::PacketFromDriverProcess: m_bCaptureVideo = %d, m_nFlushFrames = %d\n", m_bCaptureVideo, m_nFlushFrames);
				);
			}
		}

		// update the number of Frames needed to be flushed
		if (m_nFlushFrames > 0)
		{
			m_nFlushFrames --;

			if(m_nFlushFrames == 0)
			{
				m_bCheckKeyFrame = estiTRUE;

				if (5 == nCheckKeyFrame)
				{
					nCheckKeyFrame = 0;
					RequestKeyFrame ();
				}
			}
		} // end if
	} // end if(m_bChannelOpen)

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::FrameCountAdd
//
// Description: Adds to the Frame counter
//
// Abstract:
//
// Returns:
//	None
//
void CstiVideoRecord::FrameCountAdd (uint32_t un32AmountToAdd)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::FrameCountAdd);

	StatsLock ();

	// Yes! Set the Frame count.
	m_Stats.un32FrameCount += un32AmountToAdd;

	StatsUnlock ();
} // end CstiVideoRecord::FrameCountAdd


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::StatsCollect
//
// Description: Collects the stats since the last collection
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiVideoRecord::StatsCollect (
	SstiVRStats *pStats)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::FrameCountCollect);

	static float fTicksToMillisecondsMultiplier = 1000.0F / (float)stiOSSysClkRateGet ();

	StatsLock ();

	*pStats = m_Stats;

	pStats->un32ByteCount = ByteCountCollect ();
	pStats->un32KeyframeMinWait = (uint32_t)(m_Stats.un32KeyframeMinWait * fTicksToMillisecondsMultiplier);
	pStats->un32KeyframeMaxWait = (uint32_t)(m_Stats.un32KeyframeMaxWait * fTicksToMillisecondsMultiplier);
	pStats->un32KeyframeAvgWait = (uint32_t)((float)m_Stats.un32Keyframe_TotalTicksWaited / (float)m_Stats.totalKeyframesSent * fTicksToMillisecondsMultiplier);

	// Clear the members that are only counts since last collection
	m_Stats.un32FrameCount = 0;
	m_Stats.keyframesRequested = 0;
	m_Stats.keyframesSent = 0;
	m_Stats.rtxPacketsSent = 0;
	m_Stats.nackRequestsReceived = 0;
	m_Stats.rtxKbpsSent = 0;
	m_Stats.rtcpCount = 0;
	m_Stats.rtcpTotalJitter = 0;
	m_Stats.rtcpTotalRTT = 0;

	StatsUnlock ();

	return stiRESULT_SUCCESS;
} // end CstiVideoRecord::StatsCollect


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::StatsClear
//
// Description: Resets the stats
//
// Abstract:
//
// Returns:
//	None
//
void CstiVideoRecord::StatsClear ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::StatsClear);

	StatsLock ();

	m_Stats = {};

	MediaRecordType::StatsClear();

	StatsUnlock ();

} // end CstiVideoRecord::StatsClear


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::DataChannelClose
//
// Description: Post a message that the channel is closing.
//
// Abstract:
//	ACTION - add an abstract
//
// Returns:
//	None.
//
void CstiVideoRecord::DataChannelClose ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::DataChannelClose);

	stiDEBUG_TOOL ( g_stiVideoRecordDebug,
		stiTrace ("CstiVideoRecord::DataChannelClose\n");
	);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_bChannelOpen)
	{
		m_packetSendSignalConnection.Disconnect();
		m_packetDropSignalConnection.Disconnect();
		m_rtxSender = nullptr;
 		m_rtpSender = nullptr;
		m_sendQueue = nullptr;
		
		// Clear any RTCP feedback handlers (if any)
		m_session->rtcpFeedbackFirEventHandlerSet (nullptr);
		m_session->rtcpFeedbackPlilEventHandlerSet (nullptr);
		m_session->rtcpFeedbackAfbEventHandlerSet (nullptr);
		m_session->rtcpFeedbackTmmbrEventHandlerSet (nullptr);

		m_flowControlTimer.stop ();
		m_autoSpeedTimer.stop ();

		if (m_session->rtcpFeedbackTmmbrEnabledGet ())
		{
			m_autoSpeedFlowControl.LastRateSet (m_Settings.nCurrentBitRate);
		}

		DisconnectSignals ();

		// Indicate that we have stopped sending data
		m_bCaptureVideo = estiFALSE;

		// Tell the video driver to do raw self view (not in a call)
		if (m_bVideoRecordStart)
		{
			stiHResult hResult = m_pVideoInput->VideoRecordStop ();
			stiASSERT (!stiIS_ERROR(hResult));

			m_bVideoRecordStart = estiFALSE;
		}

		m_bChannelOpen = estiFALSE;
	}

	m_bFirstVideoNTPTimestamp = estiTRUE;

	stiDEBUG_TOOL (g_stiVideoRecordCaptureStream2File,
		stiTrace ("VR:: Closing file\n");
		fclose (pVRFileIn);
	); // stiDEBUG_TOOL
} // end CstiVideoRecord::DataChannelClose


void CstiVideoRecord::ConnectSignals ()
{
	if (!m_videoPrivacySetSignalConnection)
	{
		m_videoPrivacySetSignalConnection =
			m_pVideoInput->videoPrivacySetSignalGet ().Connect (
				[this](bool enable){
					PostEvent(std::bind(&CstiVideoRecord::EventVideoPrivacySet, this, enable));
				});
	}

	if (!m_packetAvailableSignalConnection)
	{
		m_packetAvailableSignalConnection =
			m_pVideoInput->packetAvailableSignalGet ().Connect (
				[this]{
					PostEvent(std::bind(&CstiVideoRecord::EventPacketAvailable, this));
				});
	}

#if APPLICATION == APP_HOLDSERVER
	if (!m_holdserverVideoCompleteSignalConnection)
	{
		m_holdserverVideoCompleteSignalConnection =
			m_pVideoInput->holdserverVideoCompleteSignalGet ().Connect (
				[this]{
					PostEvent(std::bind(&CstiVideoRecord::EventHoldserverVideoComplete, this));
				});
	}
#endif

	if (!m_captureCapabilitiesChangedSignalConnection)
	{
		m_captureCapabilitiesChangedSignalConnection =
		m_pVideoInput->captureCapabilitiesChangedSignalGet ().Connect (
			[this]{
				PostEvent (std::bind (&CstiVideoRecord::eventRecalculateCaptureSize, this));
			});
	}
}


void CstiVideoRecord::DisconnectSignals ()
{
	m_videoPrivacySetSignalConnection.Disconnect ();

	m_packetAvailableSignalConnection.Disconnect ();
#if APPLICATION == APP_HOLDSERVER
	m_holdserverVideoCompleteSignalConnection.Disconnect ();
#endif

	m_captureCapabilitiesChangedSignalConnection.Disconnect ();
}


/*!
 * \brief Initializes the data channel
 */
stiHResult CstiVideoRecord::DataChannelInitialize ()
{
	stiHResult hResult = MediaRecordType::DataChannelInitialize();
	
	m_rtpSender = sci::make_unique<vpe::RtpSender> (m_session);
	m_sendQueue = m_rtpSender->rtpSendQueueGet ();
	{
		std::unique_lock<std::mutex> lk {m_rtpSender->mutexGet()};
		m_rtpSender->pacingBitRateSet (static_cast<size_t>(m_Settings.nCurrentBitRate * stiPACING_RATE_FACTOR));
	}
	
	// Collect stats whenever we send packets
	m_packetSendSignalConnection = m_rtpSender->packetSendSignal.Connect ([this](uint32_t ssrc, std::shared_ptr<vpe::RtpPacket> packet, RvStatus status)
	{
		if (ssrc == RvRtpGetSSRC (m_session->rtpGet ()))
		{
			StatsLock ();
			
			if (RV_OK != status)
			{
				m_Stats.packetSendErrors++;
			}
			
			ByteCountAdd(packet->payloadSizeGet ());
			
			m_Stats.packetsSent++;
						
			// If the frame sent was a keyframe, and it was the last packet of the frame, increment the
			// counter that is keeping track of sent keyframes.
			if (packet->isKeyframeGet () && packet->rtpParamGet ()->marker)
			{
				m_Stats.keyframesSent++;

				// If we were waiting on this, store the timing stats
				if (m_Stats.un32KeyframeRequestedBeginTick)
				{
					uint32_t un32DiffTicks = stiOSTickGet () - m_Stats.un32KeyframeRequestedBeginTick;

					// Update the min/max values as is appropriate.
					if (m_Stats.un32KeyframeMinWait > un32DiffTicks || m_Stats.un32KeyframeMinWait == 0)
					{
						m_Stats.un32KeyframeMinWait = un32DiffTicks;
					}

					if (m_Stats.un32KeyframeMaxWait  < un32DiffTicks)
					{
						m_Stats.un32KeyframeMaxWait = un32DiffTicks;
					}

					// Add to the total ticks spent waiting on keyframes
					m_Stats.un32Keyframe_TotalTicksWaited += un32DiffTicks;

					// Reset the begin tick to show we aren't waiting on one.
					m_Stats.un32KeyframeRequestedBeginTick = 0;
				}
			}
			
			if (packet->rtpParamGet ()->marker)
			{
				FrameCountAdd (1);
			}
			
			StatsUnlock ();
		}
	});
	
	m_packetDropSignalConnection = m_sendQueue->packetsDroppedSignal.Connect ([this](size_t)
	{
		// Be sure if we decide to empty the send queue, that we request a keyframe so the other endpoint can establish video faster.
		RequestKeyFrame();
	});

#if APPLICATION != APP_HOLDSERVER
		RvRtcpSetRTCPRecvEventHandler (m_session->rtcpGet(), VideoRTCPReportProcess, this);
#endif

	m_bFirstVideoNTPTimestamp = estiTRUE;

	hResult = Setup ();

	if (!stiIS_ERROR(hResult))
	{
		// TODO: DataChannelInitialize is by default set to try
		// in base function.  Is that going to be an issue?
		m_bChannelOpen = estiTRUE;

		ConnectSignals ();
	}

	// Set RTCP Feedback event handlers
	if (m_session->rtcpGet())
	{
		if (m_session->rtcpFeedbackFirEnabledGet())
		{
			m_session->rtcpFeedbackFirEventHandlerSet (&CstiVideoRecord::RtcpFbFirCallback);
		}

		if (m_session->rtcpFeedbackPliEnabledGet())
		{
			m_session->rtcpFeedbackPlilEventHandlerSet (&CstiVideoRecord::RtcpFbPliCallback);
		}

		if (m_session->rtcpFeedbackAfbEnabledGet())
		{
			m_session->rtcpFeedbackAfbEventHandlerSet (&CstiVideoRecord::RtcpFbAfbCallback);
		}

		if (m_session->rtcpFeedbackTmmbrEnabledGet())
		{
			m_session->rtcpFeedbackTmmbrEventHandlerSet (&CstiVideoRecord::RtcpFbTmmbrCallback);
			m_flowControlTimer.restart ();
		}
		else
		{
			m_autoSpeedTimer.restart ();
		}
		
		if (m_session->rtcpFeedbackNackRtxEnabledGet())
		{
			m_session->rtcpFeedbackNackEventHandlerSet (&CstiVideoRecord::RtcpFbNACKCallback);
			if (!m_rtxSender && m_rtpSender)
			{
				m_rtxSender = sci::make_unique<vpe::RtxSender>(m_session, *m_rtpSender, RvRtpGetSSRC(m_session->rtpGet()), m_Settings.rtxPayloadType, m_bufferSize, m_remoteSipVersion);
			}
		}

		// Set the minimum interval in seconds between regular RTCP packets
		RvRtcpFbSetMinimalInterval (m_session->rtcpGet(), stiMINIMUM_RTCP_INTERVAL);
	}

	stiDEBUG_TOOL (g_stiVideoRecordCaptureStream2File,
		stiTrace ("CstiVideoRecord::DataChannelInitialize:: Open file\n");
		pVRFileIn = fopen (szVRFileIn, "wb");
	);  // end stiDEBUG_TOOL

	return hResult;
}

/*! \brief Sets the feedback events for feedbacks that are signaled after a call is setup.
 *
 * 	Currently for NACK and TMMBR we are only signaling support once the call is started and
 *	we know the remote endpoint is a Sorenson device. Because of this we need to set the
 * 	event handlers for these feedback methods later in a call.
 */
void CstiVideoRecord::RtcpFeedbackEventsSet ()
{
	if (m_session->rtcpFeedbackTmmbrEnabledGet())
	{
		m_session->rtcpFeedbackTmmbrEventHandlerSet (&CstiVideoRecord::RtcpFbTmmbrCallback);
		m_flowControlTimer.restart ();
		m_autoSpeedTimer.stop();
	}
	else
	{
		m_flowControlTimer.stop ();
		m_autoSpeedTimer.restart ();
	}
	
	if (m_session->rtcpFeedbackNackRtxEnabledGet())
	{
		m_session->rtcpFeedbackNackEventHandlerSet (&CstiVideoRecord::RtcpFbNACKCallback);
		if (!m_rtxSender && m_rtpSender)
		{
			m_rtxSender = sci::make_unique<vpe::RtxSender>(m_session, *m_rtpSender, RvRtpGetSSRC(m_session->rtpGet()), m_Settings.rtxPayloadType, m_bufferSize, m_remoteSipVersion);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::Mute
//
// Description: Inform the data task to hold
//
// Abstract:
//	Inform the data task to resume a specific call which has been put on hold
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiVideoRecord::Mute (
	eMuteReason eReason)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::Mute);
	stiHResult hResult = stiRESULT_SUCCESS;

// 	stiTrace ("Muting VR for reason %d\n", eReason);

	Lock ();

	switch (eReason)
	{
		case estiMUTE_REASON_PRIVACY:

			if ((m_nMuted & estiMUTE_REASON_PRIVACY) == 0)
			{
				if (m_bSorensonPrivacy)
				{
					// Set the flag to false. This is checked in Task() and if false, no video
					// is retrieved from the driver.
					m_bCaptureVideo = estiFALSE;
	
					if (m_nMuted == 0)
					{
						StatsClear ();
					}
	
					privacyModeSetAndNotify (estiON);
	
					m_nMuted |= estiMUTE_REASON_PRIVACY;
				}
				else
				{
					if (m_bVideoRecordStart)
					{
						m_pVideoInput->VideoRecordStop ();
						m_bVideoRecordStart = estiFALSE;
						m_bCaptureVideo = estiFALSE;

						m_flowControlTimer.stop ();
						m_autoSpeedTimer.stop ();
					}

					if (m_nMuted == 0)
					{
						StatsClear ();
					}
	
					m_nMuted |= estiMUTE_REASON_PRIVACY;
					m_nHoldPrivacyPacketBurst = nVIDEO_PRIVACY_HOLD_BURST_COUNT;
					EventHoldPrivacyTimer ();
				}
			}

			break;

		case estiMUTE_REASON_HELD:

			if ((m_nMuted & estiMUTE_REASON_HELD) == 0)
			{
				if (m_nMuted == 0)
				{
					StatsClear ();
				}

				m_nMuted |= estiMUTE_REASON_HELD;
			}

			break;

		case estiMUTE_REASON_HOLD:
		case estiMUTE_REASON_DHV:

			if ((m_nMuted & eReason) == 0)
			{
				if (m_bSorensonPrivacy)
				{
					if (m_nMuted == 0)
					{
						StatsClear ();
					}
	
					DataChannelClose ();
	
					m_nMuted |= eReason;
				}
				else
				{
					m_pVideoInput->VideoRecordStop ();
					m_bVideoRecordStart = estiFALSE;
					m_bCaptureVideo = estiFALSE;

					m_flowControlTimer.stop ();
					m_autoSpeedTimer.stop ();
					DisconnectSignals ();

					if (m_nMuted == 0)
					{
						StatsClear ();
					}
	
					m_nMuted |= eReason;
					m_nHoldPrivacyPacketBurst = nVIDEO_PRIVACY_HOLD_BURST_COUNT;
					EventHoldPrivacyTimer ();
				}
			}

			break;
	}

	Unlock ();

	return hResult;
} // end CstiVideoRecord::Mute


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::Unmute
//
// Description: Inform the data task to resume a specific call
//
// Abstract:
//	Inform the data task to resume a specific call which has been put on hold
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiVideoRecord::Unmute (
	eMuteReason eReason)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::Unmute);
	stiHResult hResult = stiRESULT_SUCCESS;

// 	stiTrace ("Unmuting VR for reason %d\n", eReason);

	Lock ();

	switch (eReason)
	{
		case estiMUTE_REASON_PRIVACY:

			if ((m_nMuted & estiMUTE_REASON_PRIVACY) != 0)
			{
				m_nMuted &= ~estiMUTE_REASON_PRIVACY;
				if (m_bSorensonPrivacy)
				{
					m_bCaptureVideo = estiTRUE;
					privacyModeSetAndNotify (estiOFF);
				}

				if (m_nMuted == 0)
				{
					m_holdPrivacyTimer.stop ();

					//Flush the Frames
					m_nFlushFrames = 0;
					m_bCheckKeyFrame = estiTRUE;
					m_un32PrevTime = 0;

					//If video capture isn't running we need to start it.
					if (!m_bVideoRecordStart)
					{
						// Set encode settings
						m_pVideoInput->VideoRecordSettingsSet (&m_stVideoSettings);
						m_pVideoInput->VideoRecordStart ();
						m_bVideoRecordStart = estiTRUE;
						m_bCaptureVideo = estiTRUE;

						if (m_session->rtcpFeedbackTmmbrEnabledGet())
						{
							m_flowControlTimer.restart ();
						}
						else
						{
							m_autoSpeedTimer.restart ();
						}
					}
				}

			}

			break;

		case estiMUTE_REASON_HELD:

			if ((m_nMuted & estiMUTE_REASON_HELD) != 0)
			{
				m_nMuted &= ~estiMUTE_REASON_HELD;

				if (m_nMuted == 0)
				{
					m_nFlushFrames = 0;
					m_bCheckKeyFrame = estiTRUE;
					m_un32PrevTime = 0;
				}
			}

			break;

		case estiMUTE_REASON_HOLD:
		case estiMUTE_REASON_DHV:

			if ((m_nMuted & eReason) != 0)
			{
				m_nMuted &= ~eReason;
				
				//  Update the local privacy setting from VideoInput.  This is because while we've been holding,
				// we haven't been registered with the callback to videoInput so if privacy changed, this object has not
				// been updated.
				EstiBool bPrivacy;
				m_pVideoInput->PrivacyGet (&bPrivacy);
				
				// If we are supposed to be set for video privacy make sure we are set locally for it.
				if (bPrivacy)
				{
					m_nMuted |= estiMUTE_REASON_PRIVACY;
				}
				// If we aren't supposed to be set for video privacy make sure we are set locally for it.
				else
				{
					m_nMuted &= ~estiMUTE_REASON_PRIVACY;
				}
				
				if ((m_nMuted & estiMUTE_REASON_DHV) == 0 &&
					(m_nMuted & estiMUTE_REASON_HOLD) == 0)
				{
					if (m_bSorensonPrivacy)
					{
						privacyModeSetAndNotify ((EstiSwitch)bPrivacy);
						DataChannelInitialize ();
						DataStart ();
					}
					else
					{
						ConnectSignals ();
						if (m_nMuted == 0)
						{
							if (!m_bVideoRecordStart)
							{
								m_holdPrivacyTimer.stop ();
								//Flush the Frames
								m_nFlushFrames = 0;
								m_bCheckKeyFrame = estiTRUE;
								m_un32PrevTime = 0;
								// Set encode settings
								m_pVideoInput->VideoRecordSettingsSet (&m_stVideoSettings);
								m_pVideoInput->VideoRecordStart ();
								m_bVideoRecordStart = estiTRUE;
								m_bCaptureVideo = estiTRUE;
								
								if (m_session->rtcpFeedbackTmmbrEnabledGet())
								{
									m_flowControlTimer.restart ();
								}
								else
								{
									m_autoSpeedTimer.restart ();
								}
							}
						}
					}
				}
			}

			break;
	}

	Unlock ();

	return hResult;
} // end CstiVideoRecord::Unmute


void CstiVideoRecord::SorensonPrivacySet (bool bSorensonPrivacy)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	m_bSorensonPrivacy = bSorensonPrivacy;
}

bool CstiVideoRecord::SorensonPrivacyGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_bSorensonPrivacy;
}


/*!
 * \brief Event handler for when VideoInput notifies us of privacy mode changes
 * \param enable - true (privacy on), false (privacy off)
 */
void CstiVideoRecord::EventVideoPrivacySet (bool enable)
{
	if (enable)
	{
		Mute (estiMUTE_REASON_PRIVACY);
	}
	else
	{
		Unmute (estiMUTE_REASON_PRIVACY);
	}
}


#ifdef stiHOLDSERVER
/*!
* \brief Called from HoldServer to disconnect the call with appropriate unserviceable HS video
* \param state - unserviceable video
*/
stiHResult CstiVideoRecord::HSEndService(EstiMediaServiceSupportStates state)
{
	PostEvent (
		std::bind(&CstiVideoRecord::EventHSEndService, this, state));

	return stiRESULT_SUCCESS;
}

/*!
* \brief Event handler for when HoldServer notifies us to end the call
* \param state - unserviceable video
*/
void CstiVideoRecord::EventHSEndService (
	EstiMediaServiceSupportStates state)
{
	if (m_pVideoInput)
	{
		m_pVideoInput->HSEndService(state);
	}
}

/*!
* \brief Event handler for when VideoInput notifies us of HoldServer video completed
*/
void CstiVideoRecord::EventHoldserverVideoComplete ()
{
	m_bCaptureVideo = estiFALSE;
	holdserverVideoFileCompletedSignal.Emit ();
}

void CstiVideoRecord::HSServiceStartStateSet(EstiMediaServiceSupportStates eMediaServiceSupportState)
{
	if (m_pVideoInput)
	{
		m_pVideoInput->ServiceStartStateSet(eMediaServiceSupportState);
	}
}

#endif

/*! \brief Function that returns the Hold or Privacy Video Packet
*
* \retval stiRESULT_SUCCESS if successful, an error if not.
*/
stiHResult CstiVideoRecord::HoldPrivacyVideoPacketGet (SstiRecordVideoFrame *pVideoPacket, EstiStatusFramePacketType ePacketType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiAspectRatio eAspectRatio;
	std::string psFilename;
	EstiVideoCodec eCodec = m_stVideoSettings.codec;
	stiTESTCOND (pVideoPacket != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND (pVideoPacket->buffer != nullptr, stiRESULT_BUFFER_TOO_SMALL);

	eAspectRatio = AspectRatioCalculate(m_stVideoSettings.unColumns, m_stVideoSettings.unRows);

	if (!m_pHoldPrivacyPacket || 
		m_pHoldPrivacyPacket->eCodec != eCodec || 
		m_pHoldPrivacyPacket->eAspectRatio != eAspectRatio ||
		m_pHoldPrivacyPacket->ePacketType != ePacketType)
	{
		if (m_pHoldPrivacyPacket)
		{
			delete m_pHoldPrivacyPacket->pData;
			delete m_pHoldPrivacyPacket;
			m_pHoldPrivacyPacket = nullptr;
		}

		switch (ePacketType)
		{
			case EstiStatusFrameHold:
				hResult = HoldVideoFilenameGet(eAspectRatio, eCodec, &psFilename);
					break;
			case EstiStatusFramePrivacy:
				hResult = PrivacyVideoFilenameGet(eAspectRatio, eCodec, &psFilename);
				break;
			default:
				stiTHROW (stiRESULT_ERROR);
				break;
		}
		stiTESTRESULT ();
		m_pHoldPrivacyPacket = new SstiVideoPacket ();
		hResult = VideoDataLoadFromFile (&psFilename, m_pHoldPrivacyPacket);
		stiTESTRESULT ();
		m_pHoldPrivacyPacket->eCodec = eCodec;
		m_pHoldPrivacyPacket->ePacketType = ePacketType;
	}

	stiTESTCOND (pVideoPacket->unBufferMaxSize >= m_pHoldPrivacyPacket->n32Length, stiRESULT_BUFFER_TOO_SMALL);
	memcpy (pVideoPacket->buffer, m_pHoldPrivacyPacket->pData, m_pHoldPrivacyPacket->n32Length);
	pVideoPacket->eFormat = m_pHoldPrivacyPacket->eEndianFormat;
	pVideoPacket->frameSize = m_pHoldPrivacyPacket->n32Length;
	pVideoPacket->keyFrame = m_pHoldPrivacyPacket->bIsKeyFrame;

STI_BAIL:
	return hResult;
}

EstiAspectRatio CstiVideoRecord::AspectRatioCalculate (
	unsigned int width,
	unsigned int height)
{
	//We are forcing the MCU to calculate a 177 (16 x 9 aspect ratio)
	double dRatio = m_bMCUConnection ? 1.7777 : static_cast<double>(width) / height;

	//The below comparisons aren't exact matches because we are giving a buffer for resolutions
	//that are close to the aspect ratio
	if (dRatio <= 1.30) //11x9 = 1.222222
	{
		return EstiAspectRatio_11x9;
	}

	if (dRatio <= 1.40) // 4x3 = 1.333333
	{
		return EstiAspectRatio_4x3;
	}

	if (dRatio <= 1.50) //22x15 = 1.466666
	{
		return EstiAspectRatio_22x15;
	}

	//16x9 = 1.777777
	return EstiAspectRatio_16x9;
}

stiHResult CstiVideoRecord::VideoDataLoadFromFile (std::string* psFilename, SstiVideoPacket* pOutVideoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int32_t n32Length = 0;
	int8_t n8KeyFrame = 0;
	int32_t n32FormatBuffer = 0;
	size_t itemsRead = 0;
	FILE* pPacketFile = fopen (psFilename->c_str(), "rb");
	if (pPacketFile)
	{
		itemsRead = fread (&n8KeyFrame, sizeof(int8_t), 1, pPacketFile);
		stiTESTCOND (1 == itemsRead, stiRESULT_ERROR);

		itemsRead = fread (&n32FormatBuffer, sizeof(int32_t), 1, pPacketFile);
		stiTESTCOND (1 == itemsRead, stiRESULT_ERROR);

		itemsRead = fread (&n32Length, sizeof(int32_t), 1, pPacketFile);
		stiTESTCOND (1 == itemsRead, stiRESULT_ERROR);

		pOutVideoPacket->bIsKeyFrame = (n8KeyFrame == 0xF) ? estiTRUE : estiFALSE;
		pOutVideoPacket->eEndianFormat = static_cast<EstiVideoFrameFormat>(n32FormatBuffer);
		pOutVideoPacket->n32Length = n32Length;
		pOutVideoPacket->pData = new uint8_t[n32Length];

		itemsRead = fread(pOutVideoPacket->pData, sizeof(uint8_t), n32Length, pPacketFile);
		stiTESTCOND ((uint32_t)n32Length == itemsRead, stiRESULT_ERROR);

	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	if(pPacketFile)
	{
		fclose (pPacketFile);
		pPacketFile = nullptr;
	}

	return hResult;
}

stiHResult CstiVideoRecord::HoldVideoFilenameGet (EstiAspectRatio eRatio, EstiVideoCodec eCodec, std::string* psFilename)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string StaticFolder;
	stiOSStaticDataFolderGet (&StaticFolder);
	psFilename->assign (StaticFolder);
	
	if (eCodec == estiVIDEO_H264)
	{
		switch (eRatio)
		{
		case EstiAspectRatio_4x3:

			psFilename->append ("video/Hold320x240.264");
			break;
		case EstiAspectRatio_22x15:
			psFilename->append ("video/Hold352x240.264");
			break;
		case EstiAspectRatio_11x9:
			psFilename->append ("video/Hold352x288.264");
			break;
		case EstiAspectRatio_16x9:
			psFilename->append ("video/Hold384x216.264");
			break;
		default:
			stiTHROW (stiRESULT_ERROR);
			break;
		}
	}
	else if (eCodec == estiVIDEO_H263)
	{
		psFilename->append ("video/Hold352x288.263");
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}
STI_BAIL:
	return hResult;
}

stiHResult CstiVideoRecord::PrivacyVideoFilenameGet (EstiAspectRatio eRatio, EstiVideoCodec eCodec, std::string* eFilename)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string StaticFolder;
	stiOSStaticDataFolderGet (&StaticFolder);
	eFilename->assign (StaticFolder);

	if (eCodec == estiVIDEO_H264)
	{
		switch (eRatio)
		{
		case EstiAspectRatio_4x3:
			eFilename->append ("video/Privacy320x240.264");
			break;
		case EstiAspectRatio_22x15:
			eFilename->append ("video/Privacy352x240.264");
			break;
		case EstiAspectRatio_11x9:
			eFilename->append ("video/Privacy352x288.264");
			break;
		case EstiAspectRatio_16x9:
			eFilename->append ("video/Privacy384x216.264");
			break;
		default:
			stiTHROW (stiRESULT_ERROR);
			break;
		}
	}
	else if (eCodec == estiVIDEO_H263)
	{
		eFilename->append ("video/Privacy352x288.263");
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}
STI_BAIL:
	return hResult;
}


/*! \brief Callback function when we receive RTCP feedback that a keyframe is needed.
*
* \retval RV_OK
*/
RvStatus CstiVideoRecord::RtcpFbFirCallback (
		RvRtcpSession rtcpSession,
		RvUint32 ssrcAddressee,
		RvUint32 /*ssrcRequested*/)
{
	CstiVideoRecord *pThis = nullptr;
	RvStatus rvStatus = RvRtcpSessionGetContext (rtcpSession, (void**)&pThis);
	stiASSERTMSG(pThis, "Unable to process FIR message\n");

	if (pThis)
	{
		pThis->PostEvent (
			std::bind(&CstiVideoRecord::EventKeyframeRequested, pThis, ssrcAddressee));
	}
	
	return rvStatus;
}


/*!
 * \brief Callback for when a PLI RTCP Feedback message is received
 */
RvStatus CstiVideoRecord::RtcpFbPliCallback (
		RvRtcpSession rtcpSession,
		RvUint32 ssrcAddressee)
{
	CstiVideoRecord *pThis = nullptr;
	RvStatus rvStatus = RvRtcpSessionGetContext (rtcpSession, (void**)&pThis);
	stiASSERTMSG(pThis, "Unable to process PLI message\n");
	//stiTrace("\tPLI message received\n");

	if (pThis)
	{
		pThis->PostEvent (
			std::bind(&CstiVideoRecord::EventKeyframeRequested, pThis, ssrcAddressee));
	}

	return rvStatus;
}


/*!
 * \brief Callback for when a AFB RTCP Feedback message is received
 */
RvStatus CstiVideoRecord::RtcpFbAfbCallback (
		RvRtcpSession rtcpSession,
		RvUint32 ssrcAddressee,
		RvUint32* fci,
		RvUint32 fciLength)
{
	CstiVideoRecord *pThis = nullptr;
	RvStatus rvStatus = RvRtcpSessionGetContext (rtcpSession, (void**)&pThis);
	stiASSERTMSG(pThis, "Unable to process AFB message\n");

	if (pThis)
	{
		// Get unique ID
		std::string uniqueId (reinterpret_cast<const char*>(fci), 4);
		std::reverse (uniqueId.begin (), uniqueId.end ());

		if (uniqueId == "REMB")
		{
			++fci;
			uint32_t mantissa = *fci & 0x3ffff;
			uint32_t exponent = (*fci >> 18) & 0x3f;
			uint32_t numSources = *fci >> 24;
			uint32_t rate = mantissa << exponent;

			stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug > 1 || g_stiRTCPDebug,
				stiTrace ("RtcpFbAfbCallback: REMB received, maxrate %u\n", rate);
				stiTrace ("\tmantissa: %u, exponent: %u, numSources: %u\n", mantissa, exponent, numSources);
			);

			// Check to ensure this is for this session
			uint32_t i = 0;
			pThis->Lock ();
			for (; i < numSources; ++i)
			{
				++fci;
				if (*fci == RvRtpGetSSRC (pThis->m_session->rtpGet ()))
				{
					pThis->m_remb = rate;
					break;
				}
			}
			pThis->Unlock ();

			if (i >= numSources)
			{
				stiASSERTMSG (false, "REMB did not apply to any RTP sessions\n");
			}
		}
		else
		{
			stiASSERTMSG(false, "Unhandled AFB message, Id: %s\n", uniqueId.c_str ());
		}
	}

	return rvStatus;
}


/*!
 * \brief Callback for when a TMMBR RTCP Feedback message is received
 *        This occurs when the remote party wants to adjust the rate at which we are sending data
 */
RvStatus CstiVideoRecord::RtcpFbTmmbrCallback (
		RvRtcpSession rtcpSession,
		RvUint32 ssrcAddressee,
		RvUint32 maxBitRate,
		RvUint32 measuredOverhead)
{
	CstiVideoRecord *pThis = nullptr;
	RvStatus rvStatus = RvRtcpSessionGetContext (rtcpSession, (void**)&pThis);
	stiASSERTMSG(pThis, "Unable to process TMMBR message\n");

	// Ignore TMMBR requests if it's not available to us in this call
	if (pThis && pThis->m_session->rtcpFeedbackTmmbrEnabledGet())
	{
		pThis->PostEvent (
			std::bind(&CstiVideoRecord::EventTmmbrRequest, pThis, maxBitRate));

		// Send a TMMBN to notify that we received the TMMBR request
		RvRtcpFbAddTMMBN (
			rtcpSession,
			ssrcAddressee,
			maxBitRate,
			measuredOverhead);

		rvStatus = RvRtcpFbSend (rtcpSession, RtcpFbModeImmediate);
		stiASSERTMSG (rvStatus == RV_OK, "Sending TMMBN message encountered an issue");
	}

	return rvStatus;
}


/*!
 * \brief Callback for when a NACK Generic RTCP Feedback message is received
 */
RvStatus CstiVideoRecord::RtcpFbNACKCallback (
	RvRtcpSession rtcpSession,
	RvUint32 ssrcAddressee,
	RvUint16 packetID,
	RvUint16 bitMask)
{
	CstiVideoRecord *pThis = nullptr;
	RvStatus rvStatus = RvRtcpSessionGetContext (rtcpSession, (void**)&pThis);
	stiASSERTMSG(pThis, "Unable to process NACK Generic message\n");
	
	if (pThis)
	{
		uint16_t packetsIgnored = 0;
		uint16_t packetsSent = 0;
		uint16_t packetsNotFound = 0;
		uint64_t bytesSent = 0;
		
		pThis->Lock();
		
		if (pThis->m_rtxSender)
		{
			pThis->m_rtxSender->processNack (ssrcAddressee, packetID, bitMask, &packetsIgnored, &packetsSent, &packetsNotFound, &bytesSent);
		}
		
		pThis->StatsLock ();
		
		pThis->m_Stats.rtxPacketsIgnored += packetsIgnored;
		pThis->m_Stats.rtxPacketsSent += packetsSent;
		pThis->m_Stats.rtxPacketsNotFound += packetsNotFound;
		pThis->m_Stats.rtxKbpsSent += bytesSent;
		pThis->m_Stats.nackRequestsReceived++;
		pThis->ByteCountAdd (static_cast<uint32_t>(bytesSent));
		
		pThis->StatsUnlock ();
		
		pThis->Unlock();
	}
	
	return rvStatus;
}


/*! \brief Event handler for the request to send a keyframe
*
* \retval an stiHResult
*/
void CstiVideoRecord::EventKeyframeRequested (
	uint32_t ssrcAddressee)
{
	stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug,
			stiTrace ("<CstiVideoRecord::EventKeyframeRequested> Received a FIR request from SSRC 0x%08x\n", ssrcAddressee);
	);

	RequestKeyFrame ();
}


/*!
 * \brief Event handler for when we receive a TMMBR request
 */
void CstiVideoRecord::EventTmmbrRequest (
	uint32_t maxRate)
{
	auto maxChannelRate = static_cast<uint32_t>(MaxChannelRateGet ());

	// Ignore TMMBR requests if it's not enabled
	if (!m_session->rtcpFeedbackTmmbrEnabledGet())
	{
		return;
	}

	// Ensure we don't try setting a rate above our max speed. Fixes bug#27367.
	if (maxRate >= maxChannelRate)
	{
		maxRate = maxChannelRate;
	}

	stiDEBUG_TOOL(g_stiVideoRecordFlowControlDebug,
		stiTrace ("EventTmmbrRequest: TMMBR request received, maxrate %u\n", maxRate);
	);

	auto currentSendRate = m_flowControl.sendRateGet ();

	// Set the new max rate in senderFlowControl
	m_flowControl.maxRateSet (maxRate);

	// If the new maxrate is slower than our current send rate,
	// then notify the video encoder immediately of the change
	// rather than wait for the flow control timer to deal with it
	if (maxRate < currentSendRate)
	{
		CurrentBitRateSet (maxRate);
	}
}


/*!
 * \brief Notifies CstiVideoRecord of the amount of bandwidth used by Audio.
 */
void CstiVideoRecord::ReserveBitRate (const uint32_t &reservedBitRate)
{
	if (m_session->rtcpFeedbackTmmbrEnabledGet ())
	{
		PostEvent (
				   std::bind(&CstiVideoRecord::EventReserveBitRate, this, reservedBitRate));
	}
}


/*!
 * \brief Event handler to update flow control on the amount of data to reserver for other medias.
 */
void CstiVideoRecord::EventReserveBitRate (uint32_t reservedBitRate)
{
	m_flowControl.reservedBitRateSet (reservedBitRate);
}


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
RvBool CstiVideoRecord::VideoEnumerator (
	RvRtcpSession hRTCP,	// Handle to RadVision RTCP Session
	RvUint32 ssrc,		// Synchronization Source ID
	void *pContext)
{
	RvBool bResult = RV_FALSE;
	RvRtcpINFO stRTCPInfo;
#ifndef stiNO_LIP_SYNC
	EstiBool bNTPSwapped = estiFALSE;
#endif
	auto pThis = (CstiVideoRecord *)pContext;

	// Get the RTCP information from the stack.
	RvRtcpGetSourceInfo (hRTCP, ssrc, &stRTCPInfo);

#if 0
	stiDEBUG_TOOL (g_stiRTCPDebug,
		stiTrace ("VP-hRTCP=0x%X, SSRC=0x%X, CNAME=%s\n\n", hRTCP, ssrc, stRTCPInfo.cname);
	); // stiDEBUG_TOOL
#endif

	// Is there any valid receiver information?
	if (stRTCPInfo.rrFrom.valid)
	{

		// Yes! Check to make sure that the sequence number contains valid info
		if (stRTCPInfo.rrFrom.sequenceNumber != 0)
		{
#if 0
			stiDEBUG_TOOL (g_stiRTCPDebug,
				stiTrace ("Video RR: fraction lost = %u, cumulative lost = %d,\n\tseq# = %u, jitter = %u, lsr = %u, dlsr = %u\n\n",
				stRTCPInfo.rrFrom.fractionLost,
				stRTCPInfo.rrFrom.cumulativeLost,
				stRTCPInfo.rrFrom.sequenceNumber,
				stRTCPInfo.rrFrom.jitter,
				stRTCPInfo.rrFrom.lSR,
				stRTCPInfo.rrFrom.dlSR);
			); // stiDEBUG_TOOL
#endif

			// There is! Is the data new?
			if (stRTCPInfo.rrFrom.sequenceNumber != pThis->m_un32PrevSequenceRR)
			{
				// Yes! Save the FractionLost value in the global
//				un32FractionLost = stRTCPInfo.rrFrom.fractionLost;

				/*
				 * The round trip time is calculated in time units based on network time protocol time.
				 * To calculate round trip time, you convert current time to ntp time (which starts at  Jan 1, 1970.
				 * You then subtract that from the timestamp returned in the RTCP data.
				 * The rtcp data returns only 16 bits of the ntp times normal 32 bits values for seconds  And fractions
				 * of a second.  The dlsr field is the number of seconds (and parts of a second)  That it took the remote
				 * endpoint to process the rtcp packet.
				 *
				 * Total round trip time is calculated as
				 * current_time ntp - rtcp_time - dlsr;
				 */
				timeval ts{};
				gettimeofday(&ts,nullptr);
				ts.tv_sec += 2208988800UL; // to jan 1, 1900 instead of 1970
				double ntp_time = static_cast<double>(ts.tv_sec & 0xFFFF) /* 16 bits only */ + static_cast<double>(ts.tv_usec) / 1e6;
				double rtp_time = (stRTCPInfo.rrFrom.lSR >> 16) + (double)((stRTCPInfo.rrFrom.lSR & 0xFFFF) << 16) / ((uint64_t)1<<32);
				double dlsr = float(stRTCPInfo.rrFrom.dlSR) / (1<<16);
				auto  rtt = static_cast<uint32_t>((ntp_time-rtp_time-dlsr) * 1e6); // convert micro seconds
				stiDEBUG_TOOL (g_stiRTCPDebug,
					stiTrace ("RTCP: fraction lost = %u, round trip time %u\n",
						stRTCPInfo.rrFrom.fractionLost, rtt ););

				SstiRTCPInfo rtcpInfo = {};
				rtcpInfo.un32PacketsLost = stRTCPInfo.rrFrom.fractionLost;
				rtcpInfo.n32CumulativePacketsLost = stRTCPInfo.rrFrom.cumulativeLost;
				rtcpInfo.un32SequenceNumber = stRTCPInfo.rrFrom.sequenceNumber;
				rtcpInfo.un32Jitter = stRTCPInfo.rrFrom.jitter;
				rtcpInfo.un32LastSR = stRTCPInfo.rrFrom.lSR;
				rtcpInfo.un32DelaySinceLastSR = stRTCPInfo.rrFrom.dlSR;
				rtcpInfo.un32RTTms = rtt;
				
				pThis->StatsLock ();
				pThis->m_Stats.rtcpTotalRTT += (rtt / 1000);
				pThis->m_Stats.rtcpTotalJitter += rtcpInfo.un32Jitter;
				pThis->m_Stats.rtcpCount++;
				pThis->StatsUnlock ();

				if (pThis->m_autoSpeedFlowControl.autoSpeedModeGet () != estiAUTO_SPEED_MODE_LEGACY)
				{
					// When TMMBR is not enabled, we save off the receiver reports to a list.
					// The list will then be processed whenever the autospeed timer fires.
					// This makes it so that autospeed doesn't behave differently if we receive
					// RTCP reports more frequently than every 5 seconds, which can now occur
					// with the addition of PLI/FIR RTCP feedback.
					if (!pThis->m_session->rtcpFeedbackTmmbrEnabledGet())
					{
						std::lock_guard<std::mutex> lock (pThis->m_reportMutex);
						pThis->m_receiverReports.push_front (rtcpInfo);
					}
				}

				// Save the new sequence number
				pThis->m_un32PrevSequenceRR = stRTCPInfo.rrFrom.sequenceNumber;
			} // end if
		} // end if
	} // end if

	if (stRTCPInfo.sr.valid &&  stRTCPInfo.sr.packets > 0)
	{
		pThis->m_nNumPackets = static_cast<int>(stRTCPInfo.sr.packets) - pThis->m_nNumPackets;

		if (pThis->m_nNumPackets > 0)
		{
			stiDEBUG_TOOL (g_stiRTCPDebug,
				stiTrace ("\nVideo SR: mNTPtimestamp = %lu, lNTPtimestamp = %lu, timestamp = %lu, packets = %u\n\n",
				stRTCPInfo.sr.mNTPtimestamp,
				stRTCPInfo.sr.lNTPtimestamp,
				stRTCPInfo.sr.timestamp,
				stRTCPInfo.sr.packets);
			); // stiDEBUG_TOOL

			//
			// TYY May.27, 2004 - NOTE: NTP format
			//
			// stRTCPInfo.sr.lNTPtimestamp contains the second part of the NTP timestamp
			// stRTCPInfo.sr.mNTPtimestamp contains the fraction part of the NTP timestamp
			//
			// TYY Jan.7, 2005 - NOTE: NTP format  (H.323 v4.2.2.3)
			//
			// stRTCPInfo.sr.mNTPtimestamp contains the second part, the most significant 32 bits, of the NTP timestamp
			// stRTCPInfo.sr.lNTPtimestamp contains the fraction part, the least significant 32 bits, of the NTP timestamp
			//
			// We need to deal with the inconsistencies
#ifndef stiNO_LIP_SYNC
			if (pThis->m_bFirstVideoNTPTimestamp)
			{
				pThis->m_un32MostNTPTimestamp = stRTCPInfo.sr.mNTPtimestamp;
				pThis->m_un32LeastNTPTimestamp = stRTCPInfo.sr.lNTPtimestamp;
				pThis->m_bFirstVideoNTPTimestamp = estiFALSE;
				pThis->m_nNTPSolved = 0;
			}
			else if (pThis->m_nNTPSolved < 5)
			{
#if 0
				stiDEBUG_TOOL (g_stiRTCPDebug,
					stiTrace ("Video SR: New mNTPtimestamp = %lu, lNTPtimestamp = %lu\n",
						stRTCPInfo.sr.mNTPtimestamp,
						stRTCPInfo.sr.lNTPtimestamp
					);

					stiTrace ("Video SR: Old mNTPtimestamp = %lu, lNTPtimestamp = %lu\n",
						pThis->m_un32MostNTPTimestamp,	// fraction part
						pThis->m_un32LeastNTPTimestamp	// second part
					);
				);
#endif

				uint32_t un32NTP = 0;
				uint32_t un32NTPSwap = 0;

				// Assume the NTP is in the correct order
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

				// Assume the NTP is in the swapped order
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
					stiTrace("Video SR: un32NTP = %lu un32NTPSwap = %lu\n",
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
					stiTrace("Video SR: un32NTPDiff = %lu un32NTPSwapDiff = %lu\n",
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
						stiTrace("Video SR: the incoming NTPTimestamp format is swapped\n");
					);
				}
				else
				{
					stiDEBUG_TOOL (g_stiRTCPDebug,
						stiTrace("Video SR: the incoming NTPTimestamp format is NOT swapped\n");
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
							stiTrace("Video SR: NTPTimestamp format CHANGED !!!\n");
//	 						stiASSERT(estiFALSE);
						);
					}
				}
			}

			if (pThis->m_nNTPSolved >= 5)
			{
				if (pThis->m_bIsNTPSwapped)
				{

					stiDEBUG_TOOL (g_stiRTCPDebug,
						stiTrace("Video SR: NTPTimestamp swapped\n");
					);
				}
				else
				{

					stiDEBUG_TOOL (g_stiRTCPDebug,

						stiTrace("Video SR: NTPTimestamp NOT swapped **************\n");
					);
				}
			}
#endif //#ifndef stiNO_LIP_SYNC
		}

		pThis->m_nNumPackets = stRTCPInfo.sr.packets;
	}

	return (bResult);
} // end enumerator


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackProcess::VideoRTCPReportProcess
//
// Description: Event handler for incoming RTCP data
//
// Abstract:
//	This method is called by the RADVision stack upon receipt of incoming data
//	for the session identified by the parameter hRTCP.
//
// Returns:
//	none
//
void CstiVideoRecord::VideoRTCPReportProcess (
	RvRtcpSession hRTCP,	// The RTCP session for which the data has arrived
	void* pContext,
	RvUint32 ssrc)
{

	// Collect the RTCP data from the stack (using VREnumerator function)
	RvRtcpEnumParticipants (
		hRTCP,
		VideoEnumerator,
		pContext);

} // end VideoRTCPReportProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::ByteCountAdd
//
// Description: Keeps track of received data between RTCP reports
//
// Abstract:
//
// Returns:
//
//	None
//
void CstiVideoRecord::ByteCountAdd (uint32_t un32AmountToAdd)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::ByteCountAdd);

	StatsLock ();

	m_un32RTPPacketSizeSum += un32AmountToAdd;
	m_un32RTPPacketCount++;

	CstiDataTaskCommonP::ByteCountAdd (un32AmountToAdd);

	StatsUnlock ();

} // end CstiVideoRecord::ByteCountAdd


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoRecord::AveragePacketSizeSentGet
//
// Description: Returns the average packet size sent since the last time called.
//
// Abstract:
//
// Returns:
//	None
//
uint32_t CstiVideoRecord::AveragePacketSizeSentGet ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::RateDataGet);

	uint32_t un32Average = 0;

	StatsLock ();

	if (m_un32RTPPacketCount > 0)
	{
		un32Average = m_un32RTPPacketSizeSum / m_un32RTPPacketCount;

	}

	m_un32RTPPacketSizeSum = 0;
	m_un32RTPPacketCount = 0;

	StatsUnlock ();

	return un32Average;

} // end CstiVideoRecord::AveragePacketSizeSentGet


/*!
* \brief Set the current flow control bit rate
*/
void CstiVideoRecord::FlowControlRateSet (int rate)
{
	CstiDataTaskCommonP::FlowControlRateSet (rate);

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	if (m_autoSpeedFlowControl.autoSpeedModeGet () == estiAUTO_SPEED_MODE_LEGACY)
	{
		CurrentBitRateSet (rate);
	}
#endif
}


/*!
* \brief Inform the data task of a hold
*/
stiHResult CstiVideoRecord::Hold (
	EHoldLocation location)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	auto hResult = MediaRecordType::Hold(location);

	switch (location)
	{
	case estiHOLD_LOCAL:
	case estiHOLD_REMOTE:
	case estiHOLD_DHV:
		m_autoSpeedFlowControl.VideoRecordMutedNotify ();
		break;

	default:
		stiTHROW (stiRESULT_ERROR);
		break;
	}

STI_BAIL:
	return hResult;
}


/*!
* \brief Inform the data task of a hold resume
*/
stiHResult CstiVideoRecord::Resume (
	EHoldLocation location)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	auto hResult = MediaRecordType::Resume(location);

	switch (location)
	{
	case estiHOLD_LOCAL:
	case estiHOLD_DHV:
		if (estiOFF == PrivacyModeGet())
		{
			m_bUnmuting = true;
		}
		break;

	case estiHOLD_REMOTE:
		break;

	default:
		stiTESTCOND (false, stiRESULT_ERROR);
		break;
	}

STI_BAIL:
	return hResult;
}


/*!
 * \brief Sets privacy mode and notifies the application layer
 */
void CstiVideoRecord::privacyModeSetAndNotify (
    EstiSwitch enabled)
{
	PrivacyModeSet (enabled);
	if (!enabled)
	{
		m_bUnmuting = true;
	}
	else
	{
		m_autoSpeedFlowControl.VideoRecordMutedNotify ();
	}

	// Notify the application layer.
	privacyModeChangedSignal.Emit ((bool)enabled);
}


/*!
* \brief Get the packetization Scheme in use
*/
EstiPacketizationScheme CstiVideoRecord::PacketizationSchemeGet () const
{
	return m_stVideoSettings.ePacketization;
}


/*!
* \brief Set the packetization Scheme in use
*/
void CstiVideoRecord::PacketizationSchemeSet (
    EstiPacketizationScheme scheme)
{
	// If the scheme being set is unknown and we know the video type,
	// update it to use the default type for that codec
	if (scheme == estiPktUnknown)
	{
		switch (m_negotiatedCodec)
		{
		case estiH263_VIDEO: m_stVideoSettings.ePacketization = estiH263_RFC_2190; break;
		case estiH264_VIDEO: m_stVideoSettings.ePacketization = estiH264_SINGLE_NAL; break;
		case estiH265_VIDEO: m_stVideoSettings.ePacketization = estiH265_NON_INTERLEAVED; break;
		default:             m_stVideoSettings.ePacketization = scheme; break;
		}
	}
	else
	{
		m_stVideoSettings.ePacketization = scheme;
	}
}


/*!
* \brief Get the capture size in use
*/
void CstiVideoRecord::CaptureSizeGet (
	uint32_t *pixelWidth,
	uint32_t *pixelHeight) const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	*pixelWidth = m_stVideoSettings.unColumns;
	*pixelHeight = m_stVideoSettings.unRows;
}


/*!
 * \brief Recalculate the capture size
 */
void CstiVideoRecord::eventRecalculateCaptureSize ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::RecalculateCaptureSize);

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto settings = m_stVideoSettings;

	// If the codec is H.264, update the capture size
	if (estiVIDEO_H264 == settings.codec || estiVIDEO_H265 == settings.codec)
	{
		CalculateCaptureSize (
			m_unMaxFS,
			m_unMaxMBps,
			m_Settings.nCurrentBitRate,
			settings.codec,
			&settings.unColumns,
			&settings.unRows,
			&settings.unTargetFrameRate);
	}

	RemoteDisplaySizeGet (&settings.remoteDisplayWidth, &settings.remoteDisplayHeight);

	videoSettingsSet(settings);
}


/*!
* \brief Get the currently used bit rate.
* \return The bit rate currently being targeted.
*/
int CstiVideoRecord::CurrentBitRateGet () const
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::CurrentBitRateGet);
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_Settings.nCurrentBitRate;
}


/*!
* \brief Set the current bit rate
*/
void CstiVideoRecord::CurrentBitRateSet (
	int bitRate)
{
	PostEvent (std::bind (&CstiVideoRecord::eventCurrentBitRateSet, this, bitRate));
}


/*!
* \brief Set the current bit rate
*/
void CstiVideoRecord::eventCurrentBitRateSet (
	int bitRate)
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::eventCurrentBitRateSet);
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	auto settings = m_stVideoSettings;

	// If the codec is H.264, check to see if we need to change the capture size
	if (estiVIDEO_H264 == settings.codec || estiVIDEO_H265 == settings.codec)
	{
		CalculateCaptureSize (
			m_unMaxFS,
			m_unMaxMBps,
			bitRate,
			settings.codec,
			&settings.unColumns,
			&settings.unRows,
			&settings.unTargetFrameRate);
	}

	stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
		stiTrace ("CurrentBitRateSet - Setting BitRate [newRate: %d   oldRate: %d]\n", bitRate, m_stVideoSettings.unTargetBitRate);
	);

	m_Settings.nCurrentBitRate = bitRate;
	settings.unTargetBitRate = bitRate;

	videoSettingsSet(settings);
}


/*!
* \brief Get the current frame rate being targeted.
*/
int CstiVideoRecord::CurrentFrameRateGet () const
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::CurrentFrameRateGet);
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return static_cast<int>(m_stVideoSettings.unTargetFrameRate);
}


/*!
 * \brief If we were in process of unmuting, resume media flow
 */
stiHResult CstiVideoRecord::LocalPrivacyComplete ()
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::LocalPrivacyComplete);
	auto hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_bUnmuting)
	{
		m_bUnmuting = false;
		hResult = RequestKeyFrame();
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}


/*!
* \brief Load the DataInfo Structure
*/
stiHResult CstiVideoRecord::DataInfoStructLoad (
	const SstiSdpStream &sdpStream,
	const SstiMediaPayload &offer, int8_t /* secondaryPayloadId */ )
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::DataInfoStructLoad);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Reset all settings
	m_Settings = {};

	m_Settings.payloadTypeNbr = offer.n8PayloadTypeNbr;
	m_Settings.rtxPayloadType = offer.rtxPayloadType;

	m_stVideoSettings.eProfile = offer.eVideoProfile;

	auto negotiatedCodec = offer.eCodec;

	NegotiatedCodecSet (negotiatedCodec);
	PacketizationSchemeSet (offer.ePacketizationScheme);
	PayloadTypeSet (offer.n8PayloadTypeNbr);
	RvRtpParam rtpParam;

	RvRtpParamConstruct (&rtpParam);

	if (offer.bandwidth > 0)
	{
		m_Settings.nCurrentBitRate = offer.bandwidth * 1000;
	}
	else
	{
		m_Settings.nCurrentBitRate = 256000;
	}

	// If AutoSpeed and Not interpreter mode and Not tech support mode, limit initial speed if negotiated speed exceeds
	// last known good speed on this network (not less than m_conferenceParams.un32AutoSpeedMinStartSpeed)
	if (m_autoSpeedFlowControl.autoSpeedModeGet () != estiAUTO_SPEED_MODE_LEGACY &&
		m_localInterfaceMode != estiINTERPRETER_MODE &&
		m_localInterfaceMode != estiTECH_SUPPORT_MODE)
	{
		m_Settings.nCurrentBitRate = std::min ((uint32_t)m_Settings.nCurrentBitRate, m_autoSpeedFlowControl.initialDataRateGet());
	}
	else
	{
		// For TMMBR, we want interpreter and support phones to ignore the rate history file
		// and always start sending at the maximum offered bandwidth rate
		m_flowControl.initialSendRateSet (m_Settings.nCurrentBitRate);
	}

	m_stVideoSettings.unTargetBitRate = m_Settings.nCurrentBitRate;

	if (estiH263_VIDEO == negotiatedCodec)
	{
		if (estiH263_RFC_2190 == PacketizationSchemeGet())
		{
			m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpH263aGetHeaderLengthEx (&rtpParam));

			stiDEBUG_TOOL (g_stiVideoRecordDebug,
				stiTrace ("CstiVideoRecord::Setup case estiVIDEO_H263 estiH263_RFC_2190 == PacketizationScheme RTPHeaderOffset = %d\n", m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ());
			); // stiDEBUG_TOOL
		}
		else if (estiH263_RFC_2429 == PacketizationSchemeGet())
		{
			m_RTPHeaderOffset.RtpHeaderLengthSet (rtpH2631998GetHeaderLength ());

			stiDEBUG_TOOL (g_stiVideoRecordDebug,
				stiTrace ("CstiVideoRecord::Setup case estiVIDEO_H263 estiH263_RFC_2429 == PacketizationScheme RTPHeaderOffset = %d\n", m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ());
			); // stiDEBUG_TOOL
		}
		else
		{
			m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpH263GetHeaderLengthEx (&rtpParam));

			stiDEBUG_TOOL (g_stiVideoRecordDebug,
				stiTrace ("CstiVideoRecord::Setup case estiVIDEO_H263 RTPHeaderOffset = %d\n", m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ());
			); // stiDEBUG_TOOL
		}

		// Initially, assume CIF is supported in case no attributes are received
		m_stVideoSettings.unTargetFrameRate = 30;
		m_stVideoSettings.unRows = g_frameSizeCaps["CIF"].nRows;
		m_stVideoSettings.unColumns = g_frameSizeCaps["CIF"].nColumns;

		// Find resolution (if none is found, will use the default set above)
		for (auto &caps: g_frameSizeCaps)
		{
			auto it = offer.mAttributes.find (caps.first);
			if (it != offer.mAttributes.end ())
			{
				m_stVideoSettings.unTargetFrameRate = int (round (29.97 / atof (it->second.c_str ())));
				m_stVideoSettings.unRows = caps.second.nRows;
				m_stVideoSettings.unColumns = caps.second.nColumns;
				break;
			}
		}

		m_stVideoSettings.codec = estiVIDEO_H263;
	}
	else if (estiH264_VIDEO == negotiatedCodec)
	{
		m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpGetHeaderLength ());

		m_stVideoSettings.codec = estiVIDEO_H264;

		// For H.264, we want to support dynamic capture sizes
		auto it = offer.mAttributes.find ("profile-level-id");
		if (it != offer.mAttributes.end ())
		{
			int nMaxMBps = 0;
			int nMaxFS = 0;

			unsigned int unProfile = 0;
			unsigned int unProfileConstraints = 0;
			unsigned int unProfileLevel = 0;
			sscanf (it->second.c_str (), "%02x%02x%02x", &unProfile, &unProfileConstraints, &unProfileLevel);

			m_stVideoSettings.eProfile = (EstiVideoProfile)unProfile;
			m_stVideoSettings.unConstraints = unProfileConstraints;
			m_stVideoSettings.unLevel = unProfileLevel;

			// Initialize some values
			switch (unProfileLevel)
			{
				default:
				case estiH264_LEVEL_1:
					nMaxMBps = nMAX_MBPS_1;
					nMaxFS = nMAX_FS_1;
					break;

				case estiH264_LEVEL_1_1:
					nMaxMBps = nMAX_MBPS_1_1;
					nMaxFS = nMAX_FS_1_1;
					break;

				case estiH264_LEVEL_1_2:
					nMaxMBps = nMAX_MBPS_1_2;
					nMaxFS = nMAX_FS_1_2;
					break;

				case estiH264_LEVEL_1_3:
					nMaxMBps = nMAX_MBPS_1_3;
					nMaxFS = nMAX_FS_1_3;
					break;

				case estiH264_LEVEL_2:
					nMaxMBps = nMAX_MBPS_2;
					nMaxFS = nMAX_FS_2;
					break;

				case estiH264_LEVEL_2_1:
					nMaxMBps = nMAX_MBPS_2_1;
					nMaxFS = nMAX_FS_2_1;
					break;

				case estiH264_LEVEL_2_2:
					nMaxMBps = nMAX_MBPS_2_2;
					nMaxFS = nMAX_FS_2_2;
					break;

				case estiH264_LEVEL_3:
					nMaxMBps = nMAX_MBPS_3;
					nMaxFS = nMAX_FS_3;
					break;
				case estiH264_LEVEL_3_1:
					nMaxMBps = nMAX_MBPS_3_1;
					nMaxFS = nMAX_FS_3_1;
					break;

				case estiH264_LEVEL_4:
					nMaxMBps = nMAX_MBPS_4;
					nMaxFS = nMAX_FS_4;
					break;

				case estiH264_LEVEL_4_1:
					nMaxMBps = nMAX_MBPS_4_1;
					nMaxFS = nMAX_FS_4_1;
					break;
				case estiH264_LEVEL_4_2:
					nMaxMBps = nMAX_MBPS_4_1;
					nMaxFS = nMAX_FS_4_1;
					break;
				case estiH264_LEVEL_5:
					nMaxMBps = nMAX_MBPS_5;
					nMaxFS = nMAX_FS_5;
					break;
				case estiH264_LEVEL_5_1:
					nMaxMBps = nMAX_MBPS_5_1;
					nMaxFS = nMAX_FS_5_1;
					break;

			} // end switch

			m_unMaxFS = (unsigned int)nMaxFS;
			m_unMaxMBps = (unsigned int)nMaxMBps;

			// Do we have upgrades?
			// Check for Max Macroblock per second upgrade
			it = offer.mAttributes.find ("max-mbps");
			if (it != offer.mAttributes.end())
			{
				unsigned int unMaxMBps = 0;
				sscanf ( it->second.c_str(), "%d", &unMaxMBps );
				m_unMaxMBps = std::max<unsigned int>( m_unMaxMBps, unMaxMBps );
			}

			// Check for Max Framesize upgrade
			it = offer.mAttributes.find("max-fs");
			if (it != offer.mAttributes.end())
			{
				unsigned int unMaxFS = 0;
				sscanf ( it->second.c_str(), "%d", &unMaxFS );
				m_unMaxFS = std::max<unsigned int>(unMaxFS, m_unMaxFS);
			}

			CalculateCaptureSize (
				m_unMaxFS,
				m_unMaxMBps,
				m_Settings.nCurrentBitRate,
				m_stVideoSettings.codec,
				&m_stVideoSettings.unColumns,
				&m_stVideoSettings.unRows,
				&m_stVideoSettings.unTargetFrameRate);
		}
	}
	else if (estiH265_VIDEO == negotiatedCodec)
	{
		m_RTPHeaderOffset.RtpHeaderLengthSet (RvRtpGetHeaderLength ());

		auto AttributeIt = offer.mAttributes.find("profile-id");
		if (AttributeIt != offer.mAttributes.end())
		{
			unsigned int unProfile = 0;
			sscanf(AttributeIt->second.c_str(), "%d", &unProfile);
			m_stVideoSettings.eProfile = (EstiVideoProfile)unProfile;
		}
		EstiH265Level eH265Level = estiH265_LEVEL_3_1;
		AttributeIt = offer.mAttributes.find("level-id");
		if (AttributeIt != offer.mAttributes.end())
		{
			unsigned int unLevel = 0;
			sscanf(AttributeIt->second.c_str(), "%d", &unLevel);
			m_stVideoSettings.unLevel = unLevel;
			eH265Level = (EstiH265Level)unLevel;
		}

		const int LUMA_TO_MACROBLOCKS = 256;

		switch (eH265Level)
		{
			case estiH265_LEVEL_1:
				m_unMaxMBps = 552960 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 36864 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_2:
				m_unMaxMBps = 3686400 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 122880 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_2_1:
				m_unMaxMBps = 7372800 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 245760 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_3:
				m_unMaxMBps = 168588800 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 552960 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_3_1:
				m_unMaxMBps = 33177600 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 983040 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_4:
				m_unMaxMBps = 66846720 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 2228224 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_4_1:
				m_unMaxMBps = 133693440 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 2228224 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_5:
				m_unMaxMBps = 267386880 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 8912896 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_5_1:
				m_unMaxMBps = 534773760 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 8912896 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_5_2:
				m_unMaxMBps = 1069547520 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 8912896 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_6:
				m_unMaxMBps = 1069547520 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 8912896 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_6_1:
				m_unMaxMBps = 1069547520 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 35651584 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_6_2:
				m_unMaxMBps = 2139095040 / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 35651584 / LUMA_TO_MACROBLOCKS;
				break;
			case estiH265_LEVEL_8_5:
				m_unMaxMBps = 4278190080UL / LUMA_TO_MACROBLOCKS;
				m_unMaxFS = 35651584 / LUMA_TO_MACROBLOCKS;
				break;
		}

		///This needs to be calculated as part of dec-parallel-cap
		// ex: a=fmtp:98 level-id=93;dec-parallel-cap={w:8; max-lsr = 62668800; max-lps = 2088960}
		//// Do we have upgrades?
		//// Check for Max Macroblock per second upgrade
		//AttributeIt = offer.mAttributes.find("max-lsr");
		//if (AttributeIt != offer.mAttributes.end())
		//{
		//	unsigned int unMaxMBps;
		//	sscanf(AttributeIt->second.c_str(), "%d", &unMaxMBps);
		//	m_unMaxMBps = std::max<unsigned int>(m_unMaxMBps, unMaxMBps / LUMA_TO_MACROBLOCKS);
		//}

		//// Check for Max Framesize upgrade
		//AttributeIt = offer.mAttributes.find("max-lps");
		//if (AttributeIt != offer.mAttributes.end())
		//{
		//	unsigned int unMaxFS;
		//	sscanf(AttributeIt->second.c_str(), "%d", &unMaxFS);
		//	m_unMaxFS = std::max<unsigned int>(m_unMaxFS, unMaxFS / LUMA_TO_MACROBLOCKS);
		//}

		m_stVideoSettings.codec = estiVIDEO_H265;

		CalculateCaptureSize(
			m_unMaxFS,
			m_unMaxMBps,
			m_Settings.nCurrentBitRate,
			m_stVideoSettings.codec,
			&m_stVideoSettings.unColumns,
			&m_stVideoSettings.unRows,
			&m_stVideoSettings.unTargetFrameRate);
	}

	return hResult;
}


/*!
* \brief Return the remote ptz capabilities
*/
int CstiVideoRecord::RemotePtzCapsGet () const
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::RemotePtzCapsGet);
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_nRmtPtzCaps;
}


/*!
 * \brief Get the send rate data from the auto speed RTCPFlowcontrol class
 */
void CstiVideoRecord::FlowControlDataGet (
	uint32_t *minSendRate,
	uint32_t *maxSendRateWithAcceptableLoss,
	uint32_t *maxSendRate,
	uint32_t *aveSendRate) const
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::FlowControlDataGet);
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (m_session->rtcpFeedbackTmmbrEnabledGet())
	{
		*minSendRate = m_flowControl.minRateGet ();
		*maxSendRateWithAcceptableLoss = m_flowControl.sendRateGet ();
		*maxSendRate = m_flowControl.maxRateGet ();
		*aveSendRate = m_flowControl.avgRateGet ();
	}
	else
	{
		*minSendRate = m_autoSpeedFlowControl.MinSendRateGet();
		*maxSendRateWithAcceptableLoss = m_autoSpeedFlowControl.MaxSendRateWithAcceptableLossGet();
		*maxSendRate = m_autoSpeedFlowControl.MaxSendRateGet();
		*aveSendRate = m_autoSpeedFlowControl.AveSendRateGet();
	}
}


/*!
 * \brief Set the preferred display size of the remote endpoint
 *
 * CalculateCaptureSize should be called after setting the remote
 * dsiplay size so that the capture size can be adjusted accordingly.
 */
void CstiVideoRecord::RemoteDisplaySizeSet (
	unsigned int displayWidth,
	unsigned int displayHeight)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	m_remoteDisplayWidth = displayWidth;
	m_remoteDisplayHeight = displayHeight;

	PostEvent (std::bind (&CstiVideoRecord::eventRecalculateCaptureSize, this));
}


/*!
 * \brief Gets the preferred display size of the remote endpoint
 *
 */
void CstiVideoRecord::RemoteDisplaySizeGet (
	unsigned int *remoteDisplayWidth,
	unsigned int *remoteDisplayHeight) const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	*remoteDisplayWidth = m_remoteDisplayWidth;
	*remoteDisplayHeight = m_remoteDisplayHeight;
}


EstiVideoCodec CstiVideoRecord::CodecGet ()
{
	return m_stVideoSettings.codec;
}


#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
/*!
 * \brief Calculate the capture size and verify that it is within the known limits
 * \return stiRESULT_SUCCESS OR stiRESULT_ERROR
 */
stiHResult CstiVideoRecord::CalculateCaptureSize (
	unsigned int unMaxFS,			// The maximum frame size (in macroblocks)
	unsigned int unMaxMBps,			// The maximum macroblocks per second permitted
	int nCurrentDataRate,			// The currently available bandwidth
	EstiVideoCodec eVideoCodec,		// The current Video Codec being used
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate)		// return the calculated frame rate
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::CalculateCaptureSize);


	CalculateCaptureSizeApple(unMaxFS, unMaxMBps, nCurrentDataRate, eVideoCodec, m_remoteDisplayWidth, m_remoteDisplayHeight, punVideoSizeCols, punVideoSizeRows, punFrameRate);


	return stiRESULT_SUCCESS;
}

#elif APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
// Defines for minimum required bitrate for 720p and VGA video on mobile. 0.0556 (1/18) bits per pixel at 30 fps.
#define MIN_BITRATE_FOR_720P 1536000
#define MIN_BITRATE_FOR_VGA 512000
#define MIN_BITRATE_FOR_1080P 3000000 // 0.048 bits per pixel at 30fps. (Encoder is more efficient at higher resolutions.)
#define THRESHOLD_MULTIPLIER 1.25
#define TARGET_FPS 30

stiHResult CstiVideoRecord::CalculateCaptureSize (
	unsigned int unMaxFS,			// The maximum frame size (in macroblocks)
	unsigned int unMaxMBps,			// The maximum macroblocks per second permitted
	int nCurrentDataRate,			// The currently available bandwidth
	EstiVideoCodec eVideoCodec,		// The current Video Codec being used
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate)		// return the calculated frame rate
{

	const int HD1080W = 1920, HD1080H = 1080, HD720W = 1280, HD720H = 720, VGA_WIDTH = 640, VGA_HEIGHT = 480, CIF_WIDTH = 352, CIF_HEIGHT = 288, MBSIZE = 256;

	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoRecord::CalculateCaptureSize);

	static unsigned int hd1080pMB = HD1080W * HD1080H / MBSIZE; // 8100
	static int hd720pMB = HD720W * HD720H / MBSIZE;
	static int vgaMB = VGA_WIDTH * VGA_HEIGHT / MBSIZE; // 1200
	static int cifMB = CIF_WIDTH * CIF_HEIGHT / MBSIZE; // 396

	// Can we do 1080p
	if (unMaxFS >= hd1080pMB && unMaxMBps >= hd1080pMB*30)
	{
		*punFrameRate = 30;
		if (nCurrentDataRate > (int)((float)MIN_BITRATE_FOR_1080P * THRESHOLD_MULTIPLIER) && !m_bTried1080P)
		{
			m_bUse1080P = true;
			m_bTried1080P = true;
		}
		else if (nCurrentDataRate < MIN_BITRATE_FOR_1080P)
		{
			m_bUse1080P = false;
		}
	}
	else
	{
		m_bUse1080P = false;
	}

	// Can we do 720P
	if (unMaxFS >= hd720pMB && unMaxMBps >= hd720pMB*30)
	{
		*punFrameRate = 30;
		if (nCurrentDataRate > (int)((float)MIN_BITRATE_FOR_720P * THRESHOLD_MULTIPLIER) && !m_bTried720P)
		{
			m_bUse720P = true;
			m_bTried720P = true;
		}
		else if (nCurrentDataRate < MIN_BITRATE_FOR_720P)
		{
			m_bUse720P = false;
		}
	}
	else
	{
		m_bUse720P = false;
	}

	// Can we do VGA
	if (unMaxFS >= vgaMB && unMaxMBps >= vgaMB*25)
	{	// can do VGA at 25fps
		*punFrameRate = 25;
		if (nCurrentDataRate > (int)((float)MIN_BITRATE_FOR_VGA * THRESHOLD_MULTIPLIER) && !m_bTried480)
		{
			m_bUse480 = true;
			m_bTried480 = true;
		}
		else if (nCurrentDataRate < MIN_BITRATE_FOR_VGA)
		{
			m_bUse480 = false;
		}
		if (unMaxMBps >= vgaMB*30)
		{	// can do VGA at 30fps
			*punFrameRate = 30;
		}
	}
	else
	{
		m_bUse480 = false;
	}
		// else if we can't do vga at at least 25fps then
		// just do CIF

	if (m_bUse1080P)
	{
		*punVideoSizeCols = HD1080W;
		*punVideoSizeRows = HD1080H;
	}
	else if (m_bUse720P)
	{
		*punVideoSizeCols = HD720W;
		*punVideoSizeRows = HD720H;
	}
	else if (m_bUse480)
	{
		*punVideoSizeCols = VGA_WIDTH;
		*punVideoSizeRows = VGA_HEIGHT;
	}
	else
	{
		// not enough FS or bandwidth to do VGA
		*punVideoSizeCols = CIF_WIDTH;
		*punVideoSizeRows = CIF_HEIGHT;

		if (unMaxMBps >= cifMB*30)
		{
			*punFrameRate = 30;
		}
		else if (unMaxMBps >= cifMB*25)
		{
			*punFrameRate = 25;
		}
		else
		{
			// 15 is the lowest we do
			*punFrameRate = 15;
		}
	}

	// If the remote display is portrait, swap the calculated width and height.
	if (m_remoteDisplayWidth < m_remoteDisplayHeight)
	{
		std::swap (*punVideoSizeCols, *punVideoSizeRows);
	}

	stiDEBUG_TOOL (g_stiCMChannelDebug,
		stiTrace ("MaxFS=%4d MaxMBps=%6d DataRate=%7d.  Calculated %3d x %3d @ %2d fps\n",
			unMaxFS, unMaxMBps, nCurrentDataRate,
			*punVideoSizeCols, *punVideoSizeRows, *punFrameRate);
	);

	return hResult;
}

#elif APPLICATION == APP_MEDIASERVER || APPLICATION == APP_DHV_PC
/*!
* \brief Calculate the capture size and verify that it is within the known limits
* \return stiRESULT_SUCCESS OR stiRESULT_ERROR
*/
stiHResult CstiVideoRecord::CalculateCaptureSize(
	unsigned int unMaxFS,			// The maximum frame size (in macroblocks)
	unsigned int unMaxMBps,			// The maximum macroblocks per second permitted
	int nCurrentDataRate,			// The currently available bandwidth
	EstiVideoCodec eVideoCodec,		// The current Video Codec being used
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate)		// return the calculated frame rate
{
	stiLOG_ENTRY_NAME(CstiVideoRecord::CalculateCaptureSize);
	stiHResult hResult = stiRESULT_SUCCESS;
	int nCols = 352;
	int nRows = 288;
	unsigned int unFrameRate = 30;

	if (nCurrentDataRate < 768000)
	{
		nCols = unstiCIF_COLS; // 352
		nRows = unstiCIF_ROWS; // 288
	}
	else
	{
		nCols = unstiVGA_COLS; // 640
		nRows = unstiVGA_ROWS; // 480
	}

	*punVideoSizeCols = nCols;
	*punVideoSizeRows = nRows;
	*punFrameRate = unFrameRate;

	// If the remote display is portrait, swap the calculated width and height.
	if (m_remoteDisplayWidth < m_remoteDisplayHeight)
	{
		std::swap (*punVideoSizeCols, *punVideoSizeRows);
	}

	return hResult;
}

#elif APPLICATION == APP_NTOUCH_PC
/*!
* \brief Calculate the capture size and verify that it is within the known limits
* \return stiRESULT_SUCCESS OR stiRESULT_ERROR
*/
stiHResult CstiVideoRecord::CalculateCaptureSize (
	unsigned int unMaxFS,			// The maximum frame size (in macroblocks)
	unsigned int unMaxMBps,			// The maximum macroblocks per second permitted
	int nCurrentDataRate,			// The currently available bandwidth
	EstiVideoCodec eVideoCodec,		// The current Video Codec being used
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate)		// return the calculated frame rate
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::CalculateCaptureSize);

	return ntouchPC::CstiNativeVideoLink::CalculateCaptureSize(unMaxFS, unMaxMBps, nCurrentDataRate, eVideoCodec, punVideoSizeCols, punVideoSizeRows, punFrameRate);
}

#elif APPLICATION == APP_HOLDSERVER
SstiCaptureSizeTable g_astH264CaptureSizeTable16x9[] =
{
		{1280, 720, 512000},
		{864, 480, 230400},
		{640, 360, 0},
};
const int g_nH264ArraySize16x9 = sizeof (g_astH264CaptureSizeTable16x9) / sizeof (SstiCaptureSizeTable);


SstiCaptureSizeTable g_astH264CaptureSizeTable4x3[] =
{
		{960, 720, 384000}, // Minimum calculated at 1/18 bits/pixel
		{640, 480, 170666},
		{480, 360, 0},
};
const int g_nH264ArraySize4x3 = sizeof (g_astH264CaptureSizeTable4x3) / sizeof (SstiCaptureSizeTable);

stiHResult CstiVideoRecord::CalculateCaptureSize (
	unsigned int unMaxFS, // The maximum frame size (in macroblocks)
	unsigned int unMaxMBps, // The maximum macroblocks per second permitted
	int nCurrentDataRate, // The currently available bandwidth
	EstiVideoCodec eVideoCodec, // The current Video Codec being used
	unsigned int *punVideoSizeCols, // return the calculated column size
	unsigned int *punVideoSizeRows, // return the calculated row size
	unsigned int *punFrameRate) // return the calculated frame rate
{
	stiLOG_ENTRY_NAME (CstiVideoRecord::CalculateCaptureSize);

	const int MB_SIZE = 256;
	const int EFFECTIVE_FPS = 10;
	const unsigned int FRAME_RATE = 1;

	stiHResult hResult = stiRESULT_SUCCESS;

	bool isWidescreen = true;
	// Use the aspect ratio on mobile if we have it
	if (m_remoteDisplayWidth != 0 && m_remoteDisplayHeight != 0)
	{
		auto aspectRatio = AspectRatioCalculate (m_remoteDisplayWidth, m_remoteDisplayHeight);
		isWidescreen = aspectRatio == EstiAspectRatio_16x9;
	}

	SstiCaptureSizeTable *captureSize{nullptr};
	size_t arraySize;

	if (isWidescreen)
	{
		captureSize = g_astH264CaptureSizeTable16x9;
		arraySize = g_nH264ArraySize16x9;
	}
	else
	{
		captureSize = g_astH264CaptureSizeTable4x3;
		arraySize = g_nH264ArraySize4x3;
	}

	// Defaults to the smallest resolution for this aspect ration if no suitable candidate is found.
	int nCols = (captureSize + (arraySize - 1) )->unCols;
	int nRows = (captureSize + (arraySize - 1) )->unRows;

	for (size_t i = 0; i < arraySize; i++)
	{
		unsigned int MbPerFrame = (captureSize + i)->unCols * (captureSize + i)->unRows / MB_SIZE;
		int dataRateTrigger = (captureSize + i)->nDataRateMin;
		if (nCurrentDataRate >= dataRateTrigger &&
			unMaxFS >= MbPerFrame &&
			unMaxMBps >= MbPerFrame * EFFECTIVE_FPS)
		{
			nCols = (captureSize + i)->unCols;
			nRows = (captureSize + i)->unRows;

			break;
		}
	}

	*punVideoSizeCols = nCols;
	*punVideoSizeRows = nRows;
	*punFrameRate = FRAME_RATE;

	return hResult;
}

#else // APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4

/*!
* \brief Calculate the capture size and verify that it is within the known limits
* \return stiRESULT_SUCCESS OR stiRESULT_ERROR
*/

#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
// Define the resolution table.  Follow these rules:
// 1- Place in descending order (highest resolutions first)
// 2- The lowest resolution must set nDataRateMin to 0  ... Not so. With second table,
// 		this one will be preferred if 16x9 is allowed.  If the data rate drops below the
//		nDataRateMin of the lowest resolution, it will go to the other table.
// 3- Only place resolutions in this array that are of the same aspect ratio (16x9)

// VP3/4 H.264 Tables
std::vector<SstiCaptureSizeTable> g_H264CaptureSizeTable16x9
{
	{1920, 1072, 1536000}, // After 1.25 adjustment: 1920
	{1792, 1008, 1280000}, // After 1.25 adjustment: 1600
	{1536, 864, 1152000}, // After 1.25 adjustment: 1440
	{1280, 720, 960000}, // After 1.25 adjustment: 1200
	{1024, 576, 832000}, // After 1.25 adjustment: 1040
	{768, 432, 640000}, // After 1.25 adjustment: 800
	{512, 288, 448000} // After 1.25 adjustment: 560
};

// Define the resolution table.  Follow these rules:
// 1- Place in descending order (highest resolutions first)
// 2- The lowest resolution must set nDataRateMin to 0
// 3- Only place resolutions in this array that are of the same aspect ratio (4x3)
std::vector<SstiCaptureSizeTable> g_H264CaptureSizeTable4x3
{
	{704, 480, 652000},
	{352, 240, 0}
};

#else // APPLICATION == APP_NTOUCH_VP2

// VP2 H.264 Tables
// See notes above for setting up resolution tables
std::vector<SstiCaptureSizeTable> g_H264CaptureSizeTable16x9
{
	{1920, 1072, 3596615}, // After 1.25 adjustment: 4496
	{1792, 1008, 3164078}, // After 1.25 adjustment: 3955
	{1536, 864, 2395997}, // After 1.25 adjustment: 2995
	{1280, 720, 1709178}, // After 1.25 adjustment: 2136
	{1024, 576, 1114112}, // After 1.25 adjustment: 1392
	{768, 432, 647495}, // After 1.25 adjustment: 809
	{512, 288, 403701} // After 1.25 adjustment: 505
};

std::vector<SstiCaptureSizeTable> g_H264CaptureSizeTable4x3
{
	{704, 480, 653516},  // Minimum calculated at 1/18 bits/pixel
	{352, 240, 0}
};
#endif

// H.265 Tables
// See notes above for setting up resolution tables
std::vector<SstiCaptureSizeTable> g_H265CaptureSizeTable16x9
{
	{1920, 1072, 1280000}, // After 1.25 adjustment: 1600
	{1792, 1008, 1024000}, // After 1.25 adjustment: 1280
	{1536, 864, 896000}, // After 1.25 adjustment: 1120
	{1280, 720, 832000}, // After 1.25 adjustment: 1040
	{1024, 576, 704000}, // After 1.25 adjustment: 880
	{768, 432, 640000}, // After 1.25 adjustment: 800
	{512, 288, 448000} // After 1.25 adjustment: 560
};

std::vector<SstiCaptureSizeTable> g_H265CaptureSizeTable4x3
{
	{704, 480, 652000},
	{352, 240, 0}
};


#define DISABLE_1080_ENCODING_WITH_DEBUG_OVERRIDE
#ifdef DISABLE_1080_ENCODING_WITH_DEBUG_OVERRIDE
void maxResolutionSet (unsigned int maxHeight)
{
#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	g_H264CaptureSizeTable16x9.erase (std::remove_if (g_H264CaptureSizeTable16x9.begin (),
													  g_H264CaptureSizeTable16x9.end (),
													  [maxHeight](SstiCaptureSizeTable &res){return res.unRows > maxHeight;}),
									  g_H264CaptureSizeTable16x9.end ());

	g_H265CaptureSizeTable16x9.erase (std::remove_if (g_H265CaptureSizeTable16x9.begin (),
													  g_H265CaptureSizeTable16x9.end (),
													  [maxHeight](SstiCaptureSizeTable &res){return res.unRows > maxHeight;}),
									  g_H265CaptureSizeTable16x9.end ());
#endif // APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
}
#endif // DISABLE_1080_ENCODING_WITH_DEBUG_OVERRIDE

#define THRESHOLD_MULTIPLIER 1.25
#define TARGET_FPS 30

stiHResult CstiVideoRecord::CalculateCaptureSize (
	unsigned int unMaxFS,			// The maximum frame size (in macroblocks)
	unsigned int unMaxMBps,			// The maximum macroblocks per second permitted
	int nCurrentDataRate,			// The currently available bandwidth
	EstiVideoCodec eVideoCodec,		// The current Video Codec being used
	unsigned int *punVideoSizeCols,	// return the calculated column size
	unsigned int *punVideoSizeRows,	// return the calculated row size
	unsigned int *punFrameRate)		// return the calculated frame rate
{
#ifdef DISABLE_1080_ENCODING_WITH_DEBUG_OVERRIDE
	static bool resolutionsSet {false};
	if (!resolutionsSet && !g_enable1080Encode)
	{
		maxResolutionSet (720);
	}
	resolutionsSet = true;
#endif
	
	stiHResult hResult = stiRESULT_SUCCESS;
	const int nMB_SIZE = 256;
	const unsigned int unHD720pMB = 3600;  // 1280 x 720 / 256

	// Initialize to 30.  Change later if needed.
	*punFrameRate = 30;

	// Set correct capture size tables based on encoder
	SstiCaptureSizeTable *pCaptureSizeTable16x9 {nullptr};
	int arraySize16x9 {};

	SstiCaptureSizeTable *pCaptureSizeTable4x3 {nullptr};
	int arraySize4x3 {};

	if (eVideoCodec == estiVIDEO_H265)
	{
		pCaptureSizeTable16x9 = g_H265CaptureSizeTable16x9.data ();
		arraySize16x9 = g_H265CaptureSizeTable16x9.size ();
		
		pCaptureSizeTable4x3 = g_H265CaptureSizeTable4x3.data ();
		arraySize4x3 = g_H265CaptureSizeTable4x3.size ();
	}
	else
	{
		pCaptureSizeTable16x9 = g_H264CaptureSizeTable16x9.data ();
		arraySize16x9 = g_H264CaptureSizeTable16x9.size ();
		
		pCaptureSizeTable4x3 = g_H264CaptureSizeTable4x3.data ();
		arraySize4x3 = g_H264CaptureSizeTable4x3.size ();
	}
	
	bool bFound = false;
	
	// Was a minimum of 720P negotiated?  This is what we are using as a signal to use 16x9.
	if (unMaxFS >= unHD720pMB && !m_b16x9Disallowed)
	{
		// Loop through the 16x9 array and find the largest capture size permitted.
		for (int i = 0; i < arraySize16x9; i++)
		{
			unsigned int unMbPerFrame = (pCaptureSizeTable16x9 + i)->unCols * (pCaptureSizeTable16x9 + i)->unRows / nMB_SIZE;
			auto dataRateTrigger = static_cast<int>((pCaptureSizeTable16x9 + i)->nDataRateMin * THRESHOLD_MULTIPLIER);
			if (nCurrentDataRate > dataRateTrigger &&
				unMaxFS >= unMbPerFrame &&
				unMaxMBps >= unMbPerFrame * TARGET_FPS)
			{
				*punVideoSizeCols = (pCaptureSizeTable16x9 + i)->unCols;
				*punVideoSizeRows = (pCaptureSizeTable16x9 + i)->unRows;

				m_nCurrentCaptureSizeIndex16x9 = i;
				m_nCurrentCaptureSizeIndex4x3 = -1;
				m_pstCurrentSize = pCaptureSizeTable16x9 + i;

				// Since we have found a match, we can break out of the loop.
				bFound = true;
				break;
			}

			// If this index was previously in use, need to check to see if it is still above the minimum for that resolution
			if (m_nCurrentCaptureSizeIndex16x9 <= i &&
					nCurrentDataRate >= (pCaptureSizeTable16x9 + i)->nDataRateMin &&
					unMaxFS >= unMbPerFrame &&
					unMaxMBps >= unMbPerFrame * TARGET_FPS)
			{
				*punVideoSizeCols = (pCaptureSizeTable16x9 +i)->unCols;
				*punVideoSizeRows = (pCaptureSizeTable16x9 + i)->unRows;

				m_nCurrentCaptureSizeIndex16x9 = i;
				m_pstCurrentSize = pCaptureSizeTable16x9 + i;

				// Since we have found a match, we can break out of the loop.
				bFound = true;
				break;
			}

		}

		// If we didn't find a match but we have used 16x9 before in this call, disallow it from here
		// forward in this call.
		if (!bFound && m_nCurrentCaptureSizeIndex16x9 >= 0)
		{
			m_b16x9Disallowed = true;
		}
	}

	if (!bFound)
	{
		// Loop through the 4x3 array and find the largest capture size permitted.
		for (int i = 0; i < arraySize4x3; i++)
		{
			unsigned int unMbPerFrame = (pCaptureSizeTable4x3 + i)->unCols * (pCaptureSizeTable4x3 + i)->unRows / nMB_SIZE;
			auto dataRateTrigger = static_cast<int>((pCaptureSizeTable4x3 + i)->nDataRateMin * THRESHOLD_MULTIPLIER);
			if (nCurrentDataRate > dataRateTrigger &&
				unMaxFS >= unMbPerFrame &&
				unMaxMBps >= unMbPerFrame * TARGET_FPS)
			{
				*punVideoSizeCols = (pCaptureSizeTable4x3 + i)->unCols;
				*punVideoSizeRows = (pCaptureSizeTable4x3 + i)->unRows;

				m_nCurrentCaptureSizeIndex4x3 = i;
				m_nCurrentCaptureSizeIndex16x9 = -1;
				m_pstCurrentSize = pCaptureSizeTable4x3 + i;

				// Since we have found a match, we can break out of the loop.
				break;
			}

			// If this index was previously in use, need to check to see if it is still above the minimum for that resolution
			if (m_nCurrentCaptureSizeIndex4x3 <= i &&
					nCurrentDataRate >= (pCaptureSizeTable4x3 + i)->nDataRateMin &&
					unMaxFS >= unMbPerFrame &&
					unMaxMBps >= unMbPerFrame * TARGET_FPS)
			{
				*punVideoSizeCols = (pCaptureSizeTable4x3 + i)->unCols;
				*punVideoSizeRows = (pCaptureSizeTable4x3 + i)->unRows;

				m_nCurrentCaptureSizeIndex4x3 = i;
				m_pstCurrentSize = pCaptureSizeTable4x3 + i;

				// Since we have found a match, we can break out of the loop.
				break;
			}

			// We need to always supply the fallback of the smallest capture size in this array.
			{
				*punVideoSizeCols = (pCaptureSizeTable4x3 + arraySize4x3 - 1)->unCols;
				*punVideoSizeRows = (pCaptureSizeTable4x3 + arraySize4x3 - 1)->unRows;
				m_pstCurrentSize = pCaptureSizeTable4x3 + arraySize4x3 - 1;
			}
		}
	}

	// In the event that we ended up with the lowest resolutions of the given aspect ratios,
	// we may need to also reduce the frame rate to stay within the signaled values.
	if (m_pstCurrentSize == pCaptureSizeTable16x9 + arraySize16x9 - 1 ||
		m_pstCurrentSize == pCaptureSizeTable4x3 + arraySize4x3 - 1)
	{
		unsigned int unMbPerFrame = m_pstCurrentSize->unCols * m_pstCurrentSize->unRows / nMB_SIZE;
		unsigned int unCalculatedFrameRate = unMaxMBps / unMbPerFrame;

		if (unCalculatedFrameRate < 30 && unCalculatedFrameRate > 15)
		{
			*punFrameRate = unCalculatedFrameRate;
		}
		else if (unCalculatedFrameRate < 15)
		{	// 15 is the lowest we do
			*punFrameRate = 15;
		}

		// Do we need to reduce frame rate due to the bandwidth?
		if ((unsigned int)nCurrentDataRate < (unMbPerFrame * 256 * *punFrameRate / 18))
		{
			unCalculatedFrameRate = nCurrentDataRate * 18 / unMbPerFrame / 256;

			if (unCalculatedFrameRate < 15)
			{
				*punFrameRate = 15;
			}
			else if (unCalculatedFrameRate >= 15 &&
				unCalculatedFrameRate < *punFrameRate)
			{
				*punFrameRate = unCalculatedFrameRate;
			}
		}
	}

	stiDEBUG_TOOL (g_stiCMChannelDebug,
		stiTrace ("MaxFS=%4d MaxMBps=%6d DataRate=%7d.  Calculated %3d x %3d @ %2d fps\n",
			unMaxFS, unMaxMBps, nCurrentDataRate,
			*punVideoSizeCols, *punVideoSizeRows, *punFrameRate);
	);

	return hResult;
}
#endif
