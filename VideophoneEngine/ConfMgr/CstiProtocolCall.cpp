/*
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2009-2011 by Sorenson Communications, Inc. All rights reserved.
 */

#include "CstiProtocolCall.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "rtp.h"
#include "rtcp.h"
#include "CstiAudioPlayback.h"
#include "CstiAudioRecord.h"
#include "CstiTextPlayback.h"
#include "CstiTextRecord.h"
#include "CstiVideoPlayback.h"
#include "CstiVideoRecord.h"
#include "CstiCall.h"
#include "CstiProtocolManager.h"
#include "EndpointMonitor.h"
#include <cctype>
#include "IstiBlockListManager2.h"
#include <cmath>
#include <algorithm>

//
// Constants
//
const int nMIN_SEQ_LOSS = 2;	// Number of sequential losses before sending flow control msg
const int nMIN_LOSS_CONNECTION_DELAY = 2; // Minimum time to wait on performing lost connection
#if defined(stiMVRS_APP)
const uint32_t un32MIN_WAIT_BETWEEN_MSG =	// Seconds (in ticks) to wait between flow
	8 * stiOSSysClkRateGet ();						// control messages (4 sec)
const uint32_t un32MIN_PKT_LOSS = 1;			// Packet loss experienced to be counted for
															// potential flow control message
#if APPLICATION == APP_NTOUCH_MAC
const uint32_t un32BANDWIDTH_TEST_TIME =		// Seconds (in ticks) to test for need to send flow
	30 * stiOSSysClkRateGet ();					// control message for bandwidth adjustments (30 sec)
#else
const uint32_t un32BANDWIDTH_TEST_TIME =		// Seconds (in ticks) to test for need to send flow
	3000 * stiOSSysClkRateGet ();					// control message for bandwidth adjustments (30 sec)
#endif // #if APPLICATION == APP_NTOUCH_MAC
#else
const uint32_t un32MIN_WAIT_BETWEEN_MSG =	// Seconds (in ticks) to wait between flow
	8 * stiOSSysClkRateGet ();						// control messages (8 sec)
const uint32_t un32MIN_PKT_LOSS = 1;			// Packet loss experienced to be counted for
															// potential flow control message
const uint32_t un32BANDWIDTH_TEST_TIME =		// Seconds (in ticks) to test for need to send flow
	30 * stiOSSysClkRateGet ();					// control message for bandwidth adjustments (30 sec)
#endif

bool CstiProtocolCall::isAudioInitialized () const
{
	return m_audioPlayback != nullptr && m_audioRecord != nullptr;
}

CstiProtocolCall::CstiProtocolCall (
	const SstiConferenceParams *pstConferenceParams, EstiProtocol eProtocol)
	:
	m_stConferenceParams (*pstConferenceParams),
	m_eVPMute (estiOFF),
	m_n16VPSizeControllable (estiVP_NONE),
	m_un32SendConnectDetectAtTickCount (0),
	m_unLostConnectionDelay (nMIN_LOSS_CONNECTION_DELAY),
	m_bDisconnectTimerStart (false),
	m_un32AutoBandwidthAdjustStopTime (0),
	m_bAutoBandwidthAdjustComplete (estiFALSE),
	m_un32TimeOfFlowCtrlMsg (0),
	m_bIsHoldable (estiFALSE),
	m_bAllowHangUp (estiTRUE),
	m_abaTargetRate (nEXCEEDINGLY_HIGH_DATA_RATE) // Something larger than we will ever use

{
	stiLOG_ENTRY_NAME (CstiProtocolCall::CstiProtocolCall);

	int i;

	// Make sure data is cleared (Do we really need this??)
	RemoteDataClear ();

	//
	// Create a circular buffer for the storage of Call Statistics.
	//
	for (i = 0; i < nSTATS_ELEMENTS - 1; ++i)
	{
		m_astCallStats[i].pNext = &(m_astCallStats[i + 1]);
	}  // end for

	// Now set the previous pointers
	for (i = nSTATS_ELEMENTS - 1; i > 0; --i)
	{
		m_astCallStats[i].pPrev = &(m_astCallStats[i - 1]);
	}  // end for

	// Set the end elements to point back to each other to create a circular buffer.
	m_astCallStats[nSTATS_ELEMENTS - 1].pNext = &(m_astCallStats[0]);

	m_astCallStats[0].pPrev = &(m_astCallStats[nSTATS_ELEMENTS - 1]);

	// Set the stat pointer head to point at the first element of the array
	m_pStatsElementHead = &(m_astCallStats[0]);

	//
	// Clear the stats.
	//
	StatsClear ();

	m_callRate = std::max (m_stConferenceParams.GetMaxRecvSpeed(eProtocol), m_stConferenceParams.GetMaxSendSpeed(eProtocol));
}

////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::Lock
//
// Description: Locks a mutex to lock the class
//
// Abstract:
//  Locks a mutex to lock access to certain data members of the class
//
// Returns:
//  None
//
stiHResult CstiProtocolCall::Lock() const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto call = m_poCstiCall.lock ();
	
	if (call)
	{
		hResult = call->Lock ();
	}
	
	if (!stiIS_ERROR (hResult))
	{
		m_lockMutex.lock ();
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::Unlock
//
// Description: Unlocks a mutex to lock the class
//
// Abstract:
//  Unlocks a mutex to unlock access to certain data members of the class
//
// Returns:
//  None
//
void CstiProtocolCall::Unlock() const
{	
	auto call = m_poCstiCall.lock ();
	if (call)
	{
		call->Unlock ();
	}
	m_lockMutex.unlock ();
}



////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::RemoteDataClear
//
// Description: Initialize a remote data
//
// Abstract:
//
// Returns:
//  None.
//
void CstiProtocolCall::RemoteDataClear ()
{
	stiLOG_ENTRY_NAME (CstiProtocolCall::RemoteDataClear);

	Lock ();

	m_eRemoteInterfaceMode = estiSTANDARD_MODE;

	auto call = m_poCstiCall.lock ();
	if (call)
	{
		call->ResultSet (estiRESULT_UNKNOWN);
	}

	m_szRemoteDisplayName[0] = '\0';
	m_szRemoteAlternateName[0] = '\0';
	m_szRemoteMacAddress[0] = '\0';
	m_szRemoteVcoCallback[0] = '\0';
	Unlock ();
} // end CstiProtocolCall::RemoteDataClear


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::AudioPlaybackActiveDetermine
//
// Description: Determine if we have an open, active, channel.
//
// Abstract:
//
// Returns:
//  estiTRUE - An active, unmuted channel audio playback channel is present
//  estiFALSE - Otherwise
//
EstiBool CstiProtocolCall::AudioPlaybackActiveDetermine ()
{
	EstiBool bRet = estiFALSE;

	// Do we have a valid channel pointer?
	if (m_audioPlayback &&

		// Is the channel in privacy mode?
		estiOFF == m_audioPlayback->PrivacyModeGet ())
	{
		bRet = estiTRUE;
	}

	return bRet;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::TextPlaybackActiveDetermine
//
// Description: Determine if we have an open, active, channel.
//
// Abstract:
//
// Returns:
//  estiTRUE - An active, unmuted channel text playback channel is present
//  estiFALSE - Otherwise
//
EstiBool CstiProtocolCall::TextPlaybackActiveDetermine ()
{
	EstiBool bRet = estiFALSE;

	// Do we have a valid channel pointer?
	if (m_textPlayback &&

		// Is the channel in privacy mode?
		estiOFF == m_textPlayback->PrivacyModeGet ())
	{
		bRet = estiTRUE;
	}

	return bRet;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::FlowControlNeededDetermine
//
// Description: See if a flow control message needs to be sent for video playback
//
// Abstract:
//	This method is called as an incoming audio/video channel is negotiated.  It
//	checks to see if we need to send a flow control message to the remote
//	endpoint to limit incoming video bandwidth.  If so, the message will be
//	sent.
//
// Returns:
//	none
//
void CstiProtocolCall::FlowControlNeededDetermine ()
{
	stiLOG_ENTRY_NAME (CstiProtocolCall::FlowControlNeededDetermine);

	EstiBool bSendMsg = estiFALSE;

	Lock ();
	
	// Do we currently have a video playback channel?
	if (m_videoPlayback)
	{
		// Get the current FlowControl limits for our video channel
		int nFlowControlRate = m_videoPlayback->FlowControlRateGet ();

		// If the FlowControl limit is greater than the maximum channel rate,
		// change it to be equal.
		if (nFlowControlRate > m_videoPlayback->MaxChannelRateGet ())
		{
			nFlowControlRate = m_videoPlayback->MaxChannelRateGet ();
		} // end if

		// Is the incoming audio channel active (not muted)?
		if (AudioPlaybackActiveDetermine ())
		{
			// Is the negotiated call rate less than the sum of the two data rates?
			if (m_callRate < (nFlowControlRate +  m_audioPlayback->MaxChannelRateGet ()))
			{
				// Yes.  Subtract off the audio data rate from the maximum receive rate
				// to arrive at the target video bandwidth
				nFlowControlRate = m_callRate - m_audioPlayback->MaxChannelRateGet ();
			} // end if

			// Is our maximum receive rate less than the sum of the two data rates?
			if (m_stConferenceParams.GetMaxRecvSpeed (ProtocolGet ()) <
				(nFlowControlRate +  m_audioPlayback->MaxChannelRateGet ()))
			{
				// Yes.  Subtract off the audio data rate from the maximum receive rate
				// to arrive at the target video bandwidth
				nFlowControlRate = m_stConferenceParams.GetMaxRecvSpeed(ProtocolGet()) - m_audioPlayback->MaxChannelRateGet ();
			} // end if
		} // end if

		// Is the calculated FlowControlRate less than the current flow rate for
		// that channel?
		if (nFlowControlRate < static_cast<int> (m_videoPlayback->FlowControlRateGet ()))
		{
			// Yes
			bSendMsg = estiTRUE;
		} // end if

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
		static char msg[256];
		sprintf(msg, "vpEngine::CstiProtocolCall::FlowControlNeededDetermine() - bSendMsg: %s", bSendMsg? "TRUE" : "FALSE");
		DebugTools::instanceGet()->DebugOutput(msg);
#endif
		if (bSendMsg)
		{
			// Send the flow control message for the
			// channel being negotiated.
			FlowControlSend (m_videoPlayback, nFlowControlRate);
		} // end if
	} // end if
	
	Unlock ();
} // end CstiProtocolCall::FlowControlNeededDetermine


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::FlowControlSend
//
// Description: Send a flow control message for the video playback channel
//
// Abstract: In general, bUseOldData will be "True" when called as a result of
// AudioPlayback Mute/Unmute.  bUseOldData will be "False" when called as a
// result of observed DataLoss (from StatsCollect).
//
// Returns:
//	none
//
void CstiProtocolCall::FlowControlSend (
	IN EstiBool bUseOldData,			// Use previously calculated observation data
	IN EstiBool bRemoteIsLegacyVP)
{
	stiLOG_ENTRY_NAME (CstiProtocolCall::FlowControlSend);

	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	stiTESTCOND_NOLOG (m_videoPlayback, stiRESULT_SUCCESS);

	Lock ();
	
#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	DebugTools::instanceGet()->DebugOutput("vpEngine::CstiProtocolCall::FlowControlSend() - Start");
#endif
	int nObservedDataRate;
	if (m_abaTargetRate > m_callRate)
	{
		// We don't want to allow the target rate to be above the call rate
		m_abaTargetRate = m_callRate;
	} // end if

	if (m_abaTargetRate > m_stConferenceParams.GetMaxRecvSpeed( ProtocolGet ()))
	{
		// We can't allow the target rate to exceed our maximum receive rate.
		m_abaTargetRate = m_stConferenceParams.GetMaxRecvSpeed (ProtocolGet ());
	} // end if

	if (bUseOldData)
	{
		// Initialize the observed average rate to the previously calculated
		// value.
		nObservedDataRate = m_abaTargetRate;
	} // end if
	else
	{
		// What is the current observed average rate of both the audio and video
		// channels.
		nObservedDataRate =  ActualPlaybackTotalDataRateGet () * 1024;

		// If remote endpoint is running flow control (not a VP with autospeed
		// set to legacy), do not reduce it's flow rate below 512 kbps.
		if (!bRemoteIsLegacyVP)
		{
			nObservedDataRate = std::max(nObservedDataRate,512000);
		}

		stiDEBUG_TOOL (g_stiRateControlDebug,
			stiTrace ("<Call::FlowControlSend2> Observed avg rate = %d\n", nObservedDataRate);
		);
	} // end else

	// Are we to use old data to recalculate the rate OR is the new observed
	// rate less than the previously calculated "System" target rate?
	if (bUseOldData || nObservedDataRate < static_cast<int> (m_abaTargetRate))
	{
		// Yes.  We are only going to send a flow control if it is less or it
		// will bounce around in some cases.
		// If we are using old data, it is probably because the audio channel
		// just changed its mute state and we are reallocating the bandwidth
		// to the video channel.
		EstiBool bActiveAudio = (AudioPlaybackActiveDetermine ());

		// We should not allow the total data rate to drop below 96K. Calculate
		// what the video data rate would be based on our current settings...
		int nMinDataRate = (96 * 1024) - (bActiveAudio ?
				m_audioPlayback->MaxChannelRateGet () : 0);

		// The new data rate is the larger of the minimum we just calculated,
		// and the observed data rate.
		int nNewUnadjustedRate = std::max (nObservedDataRate, nMinDataRate);

		// The new video channel rate needs to be the averaged combination of the
		// audio and video channels minus the audio rate.
		int nNewRate = nNewUnadjustedRate - (bActiveAudio ?  m_audioPlayback->MaxChannelRateGet () : 0);

		stiDEBUG_TOOL (g_stiRateControlDebug,
			stiTrace ("<Call::FlowControlSend2> New rate = %d, AudioRate = %d %s\n",
			nNewRate, bActiveAudio ?  m_audioPlayback->MaxChannelRateGet () : 0,
			bActiveAudio ? "Active" : "Muted");
		);

		// Is the proposed flow control rate greater than the
		// maximum negotiated channel rate?
		if (nNewRate > m_videoPlayback->MaxChannelRateGet ())
		{
			// It is.  Limit it to the max channel rate.
			nNewRate = m_videoPlayback->MaxChannelRateGet ();
		} // end if

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
		static char msg[256];
		sprintf(msg, "vpEngine::CstiProtocolCall::FlowControlSend() - New Rate: %d", nNewRate);
		DebugTools::instanceGet()->DebugOutput(msg);
#endif
		// Send the flow control message for the existing video channel.
		// This needs to cause the new flow value to be stored in the channel class object too.
		FlowControlSend (m_videoPlayback, nNewRate);

		// Store the new target rate for the whole system
		m_abaTargetRate = nNewUnadjustedRate;
	} // end if

STI_BAIL:

	Unlock ();
} // end CstiProtocolCall::FlowControlSend


////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::StatsPrint
//
// Description: Print call stats to stdio
//
// Abstract:
//
// Returns:
//  None.
//
void CstiProtocolCall::StatsPrint ()
{
	SstiCallStatistics stCallStatistics;

	StatisticsGet(&stCallStatistics);
	EstiProtocol eProtocol = ProtocolGet ();

	stiTrace ("Current Stats:\n");
	stiTrace ("Call Duration: %.0fs\n", stCallStatistics.dCallDuration);
	stiTrace ("\tProtocol = %s\n", estiSIP == eProtocol ? "SIP" : "Unknown");
	stiTrace ("\tVideoCodecs(Sent:%s\tRecv:%s)\n",
		estiVIDEO_NONE == stCallStatistics.Record.eVideoCodec ? "None" :
		estiVIDEO_H263 == stCallStatistics.Record.eVideoCodec ? "H263" :
		estiVIDEO_H264 == stCallStatistics.Record.eVideoCodec ? "H264" :
		estiVIDEO_H265 == stCallStatistics.Record.eVideoCodec ? "H265" : "???",
		estiVIDEO_NONE == stCallStatistics.Playback.eVideoCodec ? "None" :
		estiVIDEO_H263 == stCallStatistics.Playback.eVideoCodec ? "H263" :
		estiVIDEO_H264 == stCallStatistics.Playback.eVideoCodec ? "H264" :
		estiVIDEO_H265 == stCallStatistics.Playback.eVideoCodec ? "H265" : "???");

	stiTrace ("\tAudioCodecs(Sent:%s\tRecv:%s)\n",
		m_audioRecord ?
			(estiAUDIO_NONE == m_audioRecord->CodecGet () ? "None" :
			estiAUDIO_G711_ALAW == m_audioRecord->CodecGet () ? "PCMA" :
			estiAUDIO_G711_MULAW == m_audioRecord->CodecGet () ? "PCMU" :
			estiAUDIO_G722 == m_audioRecord->CodecGet () ? "G.722" : "???") : "???",
		m_audioPlayback ?
			(estiAUDIO_NONE == m_audioPlayback->CodecGet () ? "None" :
			estiAUDIO_G711_ALAW == m_audioPlayback->CodecGet () ? "PCMA" :
			estiAUDIO_G711_MULAW == m_audioPlayback->CodecGet () ? "PCMU" :
			estiAUDIO_G722 == m_audioPlayback->CodecGet () ? "G.722" : "???") : "???");

	stiTrace ("\tVideoFrameSize(Sent:%d x %d\tRecv:%d x %d)\n",
		stCallStatistics.Record.nVideoWidth, stCallStatistics.Record.nVideoHeight,
		stCallStatistics.Playback.nVideoWidth, stCallStatistics.Playback.nVideoHeight);

	stiTrace ("\tTarget FPS = %d\n", stCallStatistics.Record.nTargetFrameRate);
	stiTrace ("\tActual FPS(Sent: %d\tRecv: %d)\n",
		stCallStatistics.Record.nActualFrameRate,
		stCallStatistics.Playback.nActualFrameRate);
	stiTrace ("\tTarget Video Kbps = %d\n", stCallStatistics.Record.nTargetVideoDataRate);
	stiTrace ("\tActual Video Kbps(Sent: %d\tRecv: %d)\n",
		stCallStatistics.Record.nActualVideoDataRate,
		stCallStatistics.Playback.nActualVideoDataRate);
	stiTrace ("\tReceivedPacketLoss = %.3f%%\n", stCallStatistics.fReceivedPacketsLostPercent);

	//
	// Received Video Stats (playback)
	//
	stiTrace ("\nReceived Video Stats:\n");
	stiTrace ("\tTotal packets received = %u\n", stCallStatistics.Playback.totalPacketsReceived);
	stiTrace ("\tTotal frames assembled and sent to decoder = %u\n", stCallStatistics.un32FrameCount);
	stiTrace ("\tLost Packets = %u\n", stCallStatistics.Playback.totalPacketsLost);
	stiTrace ("\tDiscarded frames (No platform frame buffers) = %u\n",stCallStatistics. un32VideoPlaybackFrameGetErrors);
	stiTrace ("\tKeyframes Requested = %u Received = %u\n",
		stCallStatistics.un32KeyframesRequestedVP,
		stCallStatistics.un32KeyframesReceived);
	stiTrace ("\tKeyframe Wait Time: Max = %ums, Min = %ums, Avg = %ums, Total Wait %.3fs\n",
		stCallStatistics.un32KeyframeMaxWaitVP,
		stCallStatistics.un32KeyframeMinWaitVP,
		stCallStatistics.un32KeyframeAvgWaitVP,
		stCallStatistics.fKeyframeTotalWaitVP);

	stiTrace ("\tError Concealment Events = %u\n", stCallStatistics.un32ErrorConcealmentEvents);
	stiTrace ("\tI-Frames discarded as incomplete = %u\n", stCallStatistics.un32IFramesDiscardedIncomplete);
	stiTrace ("\tP-Frames discarded as incomplete = %u\n", stCallStatistics.un32PFramesDiscardedIncomplete);
	stiTrace ("\tGaps in frame numbers = %u\n", stCallStatistics.un32GapsInFrameNumber);

	//
	// Sent Video Stats (record)
	//
	stiTrace ("\nSent Video Stats:\n");
	stiTrace ("\nTotal packets sent\n", stCallStatistics.videoPacketsSent);
	stiTrace ("\tKeyframes Requested(%u) Sent(%u)\n",
		stCallStatistics.un32KeyframesRequestedVR,
		stCallStatistics.un32KeyframesSent);
	stiTrace ("\tKeyframe Wait Time: Max = %ums, Min = %ums, Avg = %ums\n",
		stCallStatistics.un32KeyframeMaxWaitVR,
		stCallStatistics.un32KeyframeMinWaitVR,
		stCallStatistics.un32KeyframeAvgWaitVR);
	stiTrace ("\tFrames lost from RCU = %u\n", stCallStatistics.un32FramesLostFromRcu);
	stiTrace ("\tPartial frames sent = %u\n", stCallStatistics.countOfPartialFramesSent);
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::StatsCollect
//
// Description: Collect and calculate the call statistics.
//
// Abstract:
//
// Returns:
//  None.
//
void CstiProtocolCall::StatsCollect ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	int nElementsCounted = 0;
	int i;
	m_pCurrentStatsElement = m_pStatsElementHead;
	uint32_t un32TotalPktsLost = 0;
	uint32_t un32TotalPktsRecv = 0;
	uint32_t un32VPTotalFramesRecv = 0;
	float fVPAveFramesRecv = 0.0;
	uint32_t un32VRTotalFramesSent = 0;
	float fVRAveFramesSent = 0.0;
	uint64_t un64TotalVideoDataRecv = 0;
	uint64_t un64TotalAudioDataRecv = 0;
	uint64_t un64TotalTextDataRecv = 0;
	uint64_t un64AveVideoDataRecv = 0;
	uint64_t un64AveAudioDataRecv = 0;
	uint64_t un64AveTextDataRecv = 0;
	uint64_t un64TotalVideoDataSent = 0;
	uint64_t un64TotalAudioDataSent = 0;
	uint64_t un64TotalTextDataSent = 0;
	uint64_t un64AveVideoDataSent = 0;
	uint64_t un64AveAudioDataSent = 0;
	uint64_t un64AveTextDataSent = 0;
	uint32_t un32TotalTickCount = 0;
	uint32_t un32CommonDivisor = 0;
	unsigned int unWeightedDivisor = 0;
	uint32_t un32TickCount = stiOSTickGet ();
	uint64_t totalRtxVideoDataSent = 0;
	uint64_t avgRtxVideoDataSent = 0;
	uint16_t totalPlaybackDelay = 0;
	uint16_t avgPlaybackDelay = 0;
	uint32_t totalRtcpRTT = 0;
	uint32_t avgRtcpRTT = 0;
	uint32_t totalRtcpJitter = 0;
	uint32_t avgRtcpJitter = 0;

	Lock ();

	// If we are in a call, go ahead with the statistics update.
	auto call = CstiCallGet ();
	stiTESTCOND (call, stiRESULT_ERROR);

	if (call->StateValidate (esmiCS_CONNECTED) &&
	    call->SubstateValidate (estiSUBSTATE_CONFERENCING))
	{
		// Test for NULL pointer
		stiTESTCOND (m_pStatsElementHead, stiRESULT_ERROR);

		// Get the current tick count and store the delta from the previous
		m_pStatsElementHead->un32TickCount =
			un32TickCount - m_un32PrevStatisticTickCount;

		// Save the current tick count in the member variable
		m_un32PrevStatisticTickCount = un32TickCount;

		// Call methods in the Data tasks to get the data rates

		if (m_videoPlayback)
		{
			CstiVideoPlayback::SstiVPStats stVPStats;

			m_videoPlayback->StatsCollect (&stVPStats);
			
			m_pStatsElementHead->un32VPPktsRecv = stVPStats.packetsReceived;
			m_pStatsElementHead->un32VPPktsLost = stVPStats.packetsLost;
			m_pStatsElementHead->un32VPFramesRecv = stVPStats.un32FrameCount;
			m_pStatsElementHead->un32VPDataRecv = stVPStats.un32ByteCount * 8; // Convert the number of bytes to bits
			m_pStatsElementHead->un32MaxOutOfOrderPackets = stVPStats.un32MaxOutOfOrderPackets;
			
			memcpy (m_pStatsElementHead->aun32PacketPositions, stVPStats.aun32PacketPositions,
					sizeof (stVPStats.aun32PacketPositions));

			m_stCallStatistics.Playback.packetsReceived = stVPStats.packetsReceived;
			m_stCallStatistics.Playback.totalPacketsReceived += stVPStats.packetsReceived;

			m_stCallStatistics.Playback.packetsLost = stVPStats.packetsLost;
			m_stCallStatistics.Playback.actualPacketsLost = stVPStats.actualPacketLoss;
			m_stCallStatistics.Playback.totalPacketsLost += stVPStats.packetsLost;

			m_stCallStatistics.un32OutOfOrderPackets += stVPStats.un32OutOfOrderPackets;
			m_stCallStatistics.un32DuplicatePackets += stVPStats.un32DuplicatePackets;

			m_stCallStatistics.videoPacketQueueEmptyErrors += stVPStats.packetQueueEmptyErrors;
			m_stCallStatistics.videoPacketsRead += stVPStats.packetsRead;
			m_stCallStatistics.videoKeepAlivePackets += stVPStats.keepAlivePackets;
			m_stCallStatistics.videoUnknownPayloadTypeErrors += stVPStats.un32UnknownPayloadTypeErrors;
			m_stCallStatistics.un32PayloadHeaderErrors += stVPStats.un32PayloadHeaderErrors;
			m_stCallStatistics.videoPacketsDiscardedPlaybackMuted += stVPStats.packetsDiscardedMuted;
			m_stCallStatistics.videoPacketsDiscardedReadMuted += stVPStats.packetsDiscardedReadMuted;
			m_stCallStatistics.burstPacketsDropped += stVPStats.burstPacketsDropped;

			m_stCallStatistics.videoTimeMutedByRemote = stVPStats.timeMutedByRemote;

			m_stCallStatistics.videoPacketsDiscardedEmpty += stVPStats.packetsDiscardedEmpty;
			m_stCallStatistics.videoPacketsUsingPreviousSSRC += stVPStats.packetsUsingPreviousSSRC;
			m_stCallStatistics.un32VideoPlaybackFrameGetErrors += stVPStats.un32VideoPlaybackFrameGetErrors;
			m_stCallStatistics.un32KeyframesReceived = stVPStats.totalKeyframesReceived;
			m_stCallStatistics.Playback.keyframes = stVPStats.keyframesReceived;
			m_stCallStatistics.Playback.keyframeRequests = stVPStats.keyframeRequests;
			m_stCallStatistics.un32KeyframeMinWaitVP = stVPStats.un32KeyframeMinWait;
			m_stCallStatistics.un32KeyframeMaxWaitVP = stVPStats.un32KeyframeMaxWait;
			m_stCallStatistics.un32KeyframeAvgWaitVP = stVPStats.un32KeyframeAvgWait;
			m_stCallStatistics.fKeyframeTotalWaitVP = stVPStats.fKeyframeTotalWait;
			m_stCallStatistics.un32ErrorConcealmentEvents += stVPStats.un32ErrorConcealmentEvents;
			m_stCallStatistics.un32IFramesDiscardedIncomplete += stVPStats.un32IFramesDiscardedIncomplete;
			m_stCallStatistics.un32PFramesDiscardedIncomplete += stVPStats.un32PFramesDiscardedIncomplete;
			m_stCallStatistics.un32GapsInFrameNumber += stVPStats.un32GapsInFrameNumber;
			m_stCallStatistics.un32FrameCount += stVPStats.un32FrameCount;
			m_stCallStatistics.countOfCallsToPacketLossCountAdd += stVPStats.countOfCallsToPacketLossCountAdd;

			m_stCallStatistics.videoFramesSentToPlatform += stVPStats.framesSentToPlatform;
			m_stCallStatistics.videoPartialKeyFramesSentToPlatform += stVPStats.partialKeyFramesSentToPlatform;
			m_stCallStatistics.videoPartialNonKeyFramesSentToPlatform += stVPStats.partialNonKeyFramesSentToPlatform;
			m_stCallStatistics.videoWholeKeyFramesSentToPlatform += stVPStats.wholeKeyFramesSentToPlatform;
			m_stCallStatistics.videoWholeNonKeyFramesSentToPlatform += stVPStats.wholeNonKeyFramesSentToPlatform;

			m_stCallStatistics.dCallDuration = CstiCallGet()->CallDurationGet ();
			m_stCallStatistics.Playback.eVideoCodec = m_videoPlayback->CodecGet ();

			m_stCallStatistics.bTunneled = stVPStats.bTunneled;
			
			if (m_stCallStatistics.highestPacketLossSeen < stVPStats.highestPacketLossSeen)
			{
				m_stCallStatistics.highestPacketLossSeen = stVPStats.highestPacketLossSeen;
			}
			
			m_stCallStatistics.Playback.rtxPacketsReceived = stVPStats.rtxPacketsReceived;
			m_stCallStatistics.Playback.totalRtxPacketsReceived += stVPStats.rtxPacketsReceived;
			m_stCallStatistics.Playback.nackRequestsSent = stVPStats.nackRequestsSent;
			m_stCallStatistics.Playback.totalNackRequestsSent += stVPStats.nackRequestsSent;
			m_stCallStatistics.actualVPPacketLoss = stVPStats.actualPacketLoss;
			m_stCallStatistics.rtxFromNoData = stVPStats.rtxFromNoData;
			
			m_pStatsElementHead->playbackDelay = stVPStats.playbackDelay;
			
			m_stCallStatistics.duplicatePacketsReceived = stVPStats.duplicatePacketsReceived;
		}

		if (m_videoRecord)
		{
			CstiVideoRecord::SstiVRStats stVRStats;

			m_videoRecord->StatsCollect (&stVRStats);

			m_stCallStatistics.videoPacketsSent = stVRStats.packetsSent;
			m_stCallStatistics.videoPacketSendErrors = stVRStats.packetSendErrors;

			m_pStatsElementHead->un32VRFramesSent = stVRStats.un32FrameCount;
			m_pStatsElementHead->un32VRDataSent = stVRStats.un32ByteCount;
			m_pStatsElementHead->rtxVideoDataSent = static_cast<uint32_t> (stVRStats.rtxKbpsSent);

			// Multiply by 8 (to convert to bits).
			m_pStatsElementHead->un32VRDataSent *= 8;
			m_pStatsElementHead->rtxVideoDataSent *= 8;

			m_stCallStatistics.un32KeyframesRequestedVR = stVRStats.totalKeyframesRequested;
			m_stCallStatistics.Record.keyframeRequests = stVRStats.keyframesRequested;
			m_stCallStatistics.un32KeyframesSent = stVRStats.totalKeyframesSent;
			m_stCallStatistics.Record.keyframes = stVRStats.keyframesSent;
			m_stCallStatistics.un32KeyframeMaxWaitVR = stVRStats.un32KeyframeMaxWait;
			m_stCallStatistics.un32KeyframeMinWaitVR = stVRStats.un32KeyframeMinWait;
			m_stCallStatistics.un32KeyframeAvgWaitVR = stVRStats.un32KeyframeAvgWait;
			
			m_stCallStatistics.un32FramesLostFromRcu = stVRStats.un32FramesLostFromRcu;
			
			m_stCallStatistics.countOfPartialFramesSent = stVRStats.countOfPartialFramesSent;
			
			m_stCallStatistics.rtxPacketsNotFound = stVRStats.rtxPacketsNotFound;
			m_stCallStatistics.rtxPacketsIgnored = stVRStats.rtxPacketsIgnored;
			m_stCallStatistics.Record.rtxPacketsSent = stVRStats.rtxPacketsSent;
			m_stCallStatistics.Record.totalRtxPacketsSent += stVRStats.rtxPacketsSent;
			m_stCallStatistics.Record.nackRequestsReceived = stVRStats.nackRequestsReceived;
			m_stCallStatistics.Record.totalNackRequestsReceived += stVRStats.nackRequestsReceived;
			
			if (stVRStats.rtcpCount)
			{
				m_pStatsElementHead->rtcpJitter = stVRStats.rtcpTotalJitter / stVRStats.rtcpCount;
				m_pStatsElementHead->rtcpRTT = stVRStats.rtcpTotalRTT / stVRStats.rtcpCount;
			}
		}

		if (m_audioPlayback)
		{
			m_audioPlayback->StatsCollect (
					&m_pStatsElementHead->un32APPktsRecv,
					&m_pStatsElementHead->un32APPktsLost,
					&m_pStatsElementHead->un32APActualPktsLost,
					&m_pStatsElementHead->un32APDataRecv);

			// Multiply by 8 (to convert to bits).
			m_pStatsElementHead->un32APDataRecv *= 8;
			
			m_stCallStatistics.Playback.audioPacketsLost += m_pStatsElementHead->un32APActualPktsLost;
			m_stCallStatistics.Playback.numberAudioPackets += m_pStatsElementHead->un32APPktsRecv;
			m_stCallStatistics.Playback.audioCodec = m_audioPlayback->CodecGet ();
		}

		if (m_audioRecord)
		{
			uint32_t audioPackets = 0;
			m_audioRecord->StatsCollect (
				&m_pStatsElementHead->un32ARDataSent,
				&audioPackets);

			m_stCallStatistics.Record.numberAudioPackets += audioPackets;
			// Multiply by 8 (to convert to bits).
			m_pStatsElementHead->un32ARDataSent *= 8;
		}

#ifndef stiDISABLE_LOST_CONNECTION

		// Check to see that the amount of data received hasn't gone to 0
		if (m_pStatsElementHead->un32APDataRecv == 0 &&
			 m_pStatsElementHead->un32VPDataRecv == 0)
		{
			// If this is the first recognition of going to 0, set the flag to send a connection detection message.
			// Use m_unLostConnectionDelay value unless the value in conference params is shorter.
			if (0 == m_un32SendConnectDetectAtTickCount)
			{
				if (m_unLostConnectionDelay < m_stConferenceParams.unLostConnectionDelay)
				{
					m_un32SendConnectDetectAtTickCount = un32TickCount + (stiOSSysClkRateGet () * m_unLostConnectionDelay);
					m_unLostConnectionDelay += m_unLostConnectionDelay;
					m_bDisconnectTimerStart = false;
				}
				else
				{
					m_un32SendConnectDetectAtTickCount = un32TickCount + (stiOSSysClkRateGet () * m_stConferenceParams.unLostConnectionDelay);
					m_bDisconnectTimerStart = true;
				}
			}
			// If we have previously set this flag, is it time to send a lost connection detection message?
			else if (m_un32SendConnectDetectAtTickCount <= un32TickCount)
			{
				// Cause a message to be sent to the remote system to determine if we still have a connection.
				LostConnectionCheck (m_bDisconnectTimerStart);
				m_un32SendConnectDetectAtTickCount = 0;
			}
		}
		else
		{
			// Since we've received media, reset the time for sending a connection detection message.
			m_un32SendConnectDetectAtTickCount = 0;
			m_unLostConnectionDelay = m_stConferenceParams.unLostConnectionDelay;
		}
#endif // #ifndef stiDISABLE_LOST_CONNECTION

		memset (m_stCallStatistics.aun32PacketPositions, 0,
				sizeof (m_stCallStatistics.aun32PacketPositions));
				
		m_stCallStatistics.un32MaxOutOfOrderPackets = 0;
						
		// Add up all the stored values and give proper weight to them
		// according to the most recently received data.
		for (i = 0; i < nSTATS_ELEMENTS; ++i)
		{
			// If the current elements tickCount is 0, we don't want to
			// include it in the statistics.  This is the case when we first
			// start a call or after a channel has been unmutted.
			if (0 < m_pCurrentStatsElement->un32TickCount)
			{
				un32TotalPktsLost +=
					m_pCurrentStatsElement->un32APPktsLost * (nSTATS_ELEMENTS - i);
				un32TotalPktsLost +=
					m_pCurrentStatsElement->un32VPPktsLost * (nSTATS_ELEMENTS - i);
				un32TotalPktsRecv +=
					m_pCurrentStatsElement->un32APPktsRecv * (nSTATS_ELEMENTS - i);
				un32TotalPktsRecv +=
					m_pCurrentStatsElement->un32VPPktsRecv * (nSTATS_ELEMENTS - i);
				un64TotalAudioDataRecv +=
					m_pCurrentStatsElement->un32APDataRecv * (nSTATS_ELEMENTS - i);
				un64TotalTextDataRecv +=
					m_pCurrentStatsElement->un32APDataRecv * (nSTATS_ELEMENTS - i);
				un64TotalVideoDataRecv +=
					m_pCurrentStatsElement->un32VPDataRecv * (nSTATS_ELEMENTS - i);
				un64TotalAudioDataSent +=
					m_pCurrentStatsElement->un32ARDataSent * (nSTATS_ELEMENTS - i);
				un64TotalTextDataSent +=
					m_pCurrentStatsElement->un32ARDataSent * (nSTATS_ELEMENTS - i);
				un64TotalVideoDataSent +=
					m_pCurrentStatsElement->un32VRDataSent * (nSTATS_ELEMENTS - i);
				un32VPTotalFramesRecv +=
					m_pCurrentStatsElement->un32VPFramesRecv * (nSTATS_ELEMENTS - i);
				nElementsCounted++;
				un32VRTotalFramesSent +=
					m_pCurrentStatsElement->un32VRFramesSent * (nSTATS_ELEMENTS - i);
				un32TotalTickCount += m_pCurrentStatsElement->un32TickCount;
				unWeightedDivisor += (nSTATS_ELEMENTS - i);
				
				totalRtxVideoDataSent +=
					m_pCurrentStatsElement->rtxVideoDataSent * (nSTATS_ELEMENTS - i);
				
				totalPlaybackDelay += m_pCurrentStatsElement->playbackDelay * (nSTATS_ELEMENTS - i);
				
				totalRtcpRTT += m_pCurrentStatsElement->rtcpRTT * (nSTATS_ELEMENTS - i);
				totalRtcpJitter += m_pCurrentStatsElement->rtcpJitter * (nSTATS_ELEMENTS - i);
				
				for (int j = 0; j < MAX_STATS_PACKET_POSITIONS; j++)
				{
					m_stCallStatistics.aun32PacketPositions[j] += m_pCurrentStatsElement->aun32PacketPositions[j];
				}
				
				if (m_pCurrentStatsElement->un32MaxOutOfOrderPackets > m_stCallStatistics.un32MaxOutOfOrderPackets)
				{
					m_stCallStatistics.un32MaxOutOfOrderPackets = m_pCurrentStatsElement->un32MaxOutOfOrderPackets;
				}
			}  // end if

			// Advance to the next element in the circular buffer
			m_pCurrentStatsElement = m_pCurrentStatsElement->pNext;

			// Test for NULL pointer
			stiTESTCOND (m_pCurrentStatsElement, stiRESULT_ERROR);
		}  // end for

		// If we have some data to work with, calculate the statistics
		if (0 < nElementsCounted)
		{
			// Now divide all the totals by the weighted divisor to get the
			// averages then convert it into units of seconds.  First,
			// calculate the common divisor.
			un32CommonDivisor = unWeightedDivisor *
				un32TotalTickCount / stiOSSysClkRateGet () /
				nElementsCounted * 100;

			if (un32CommonDivisor == 0)
			{
				un32CommonDivisor = 1;
			}

			fVPAveFramesRecv = (float)un32VPTotalFramesRecv * 100.0F / (float)un32CommonDivisor;
			fVRAveFramesSent = (float)un32VRTotalFramesSent * 100.0F / (float)un32CommonDivisor;
			un64AveVideoDataRecv = un64TotalVideoDataRecv * 100 / un32CommonDivisor;
			un64AveAudioDataRecv = un64TotalAudioDataRecv * 100 / un32CommonDivisor;
			un64AveTextDataRecv = un64TotalTextDataRecv * 100 / un32CommonDivisor;
			un64AveVideoDataSent = un64TotalVideoDataSent * 100 / un32CommonDivisor;
			un64AveAudioDataSent = un64TotalAudioDataSent * 100 / un32CommonDivisor;
			un64AveTextDataSent = un64TotalTextDataSent * 100 / un32CommonDivisor;
			avgRtxVideoDataSent = totalRtxVideoDataSent * 100 / un32CommonDivisor;
			avgPlaybackDelay = totalPlaybackDelay * 100 / un32CommonDivisor;
			avgRtcpRTT = totalRtcpRTT * 100 / un32CommonDivisor;
			avgRtcpJitter = totalRtcpJitter * 100 / un32CommonDivisor;

			// Divide by 1024 to store and calculate as Kb
			m_stCallStatistics.Playback.nActualVideoDataRate = static_cast<int> (un64AveVideoDataRecv / 1024);
			m_stCallStatistics.Record.nActualVideoDataRate = static_cast<int> (un64AveVideoDataSent / 1024);
			m_stCallStatistics.Playback.nActualAudioDataRate = static_cast<int> (un64AveAudioDataRecv / 1024);
			m_stCallStatistics.Record.nActualAudioDataRate = static_cast<int> (un64AveAudioDataSent / 1024);
			m_stCallStatistics.Playback.nActualTextDataRate = static_cast<int> (un64AveTextDataRecv / 1024);
			m_stCallStatistics.Record.nActualTextDataRate = static_cast<int> (un64AveTextDataSent / 1024);
			m_stCallStatistics.rtxKbpsSent = static_cast<uint32_t> (avgRtxVideoDataSent / 1024);

			m_stCallStatistics.Playback.fActualFrameRate = fVPAveFramesRecv;
			m_stCallStatistics.Playback.nActualFrameRate = static_cast<int> (std::lround(fVPAveFramesRecv));
			m_stCallStatistics.Record.fActualFrameRate = fVRAveFramesSent;
			m_stCallStatistics.Record.nActualFrameRate = static_cast<int> (std::lround(fVRAveFramesSent));
			m_stCallStatistics.avgPlaybackDelay = avgPlaybackDelay;
			m_stCallStatistics.avgRtcpRTT = avgRtcpRTT;
			m_stCallStatistics.avgRtcpJitter = avgRtcpJitter;
			

			// Packets lost is the result of dividing packets lost by the
			// sum of packets lost and packets received.  If total packets
			// received is 0, set the value stored in config data to 0.
			if (0 == un32TotalPktsRecv && 0 == un32TotalPktsLost)
			{
				m_stCallStatistics.un32ReceivedPacketsLostPercent = 0;
				m_stCallStatistics.fReceivedPacketsLostPercent = 0.0;
			}  // end if
			else
			{
				m_stCallStatistics.fReceivedPacketsLostPercent =
					(float)un32TotalPktsLost * 100.0F /
					((float)un32TotalPktsLost + (float)un32TotalPktsRecv);

				m_stCallStatistics.un32ReceivedPacketsLostPercent = static_cast<uint32_t> (std::lround(m_stCallStatistics.fReceivedPacketsLostPercent));

			}  // end else

			// Advance the head pointer to the next element to load
			m_pStatsElementHead = m_pCurrentStatsElement->pPrev;

			// Do AutoBandwidthAdjustment, only if we're not using TMMBR
			if (m_videoPlayback && !m_videoPlayback->tmmbrNegotiatedGet())
			{
			    AutoBandwidthAdjust ();
			}
			
			// Update playback with the amount of data sent. Used to determine if a keepalive is needed.
			// TODO: Remove when keepalive code is moved to record and a record channel is always started if media is not disabled.
			if (m_videoPlayback)
			{
				m_videoPlayback->bytesRecentlySentSet (m_pStatsElementHead->un32VRDataSent);
			}
			
		}  // end if

	} // end if

#ifndef stiDISABLE_LOST_CONNECTION
	// This call is on hold, we should be sending a connection detection message periodically to make sure
	// that we are still connected.  (We know that the call is on hold because CstiCall::StatsCollect tests that
	// the call is either in Connected or one of the Hold states before calling this method).
	else
	{
		// If this is the first recognition of going to 0, set the flag to send a connection detection message.
		if (0 == m_un32SendConnectDetectAtTickCount)
		{
			m_bDisconnectTimerStart = true;
			m_un32SendConnectDetectAtTickCount = un32TickCount + (stiOSSysClkRateGet () * m_stConferenceParams.unLostConnectionDelay);
		}

		// If we have previously set this flag, is it time to send a lost connection detection message?
		else if (m_un32SendConnectDetectAtTickCount <= un32TickCount)
		{
			// Cause a message to be sent to the remote system to determine if we still have a connection.
			LostConnectionCheck (m_bDisconnectTimerStart);
			m_un32SendConnectDetectAtTickCount = 0;
		}
	}
#endif // #ifndef stiDISABLE_LOST_CONNECTION

	
	vpe::EndpointMonitor::instanceGet ()->callStatistics (call);

// Every n times, print out the call stats
	static int nIter = 1;
	if (g_stiCallStats)
	{
		if (0 == nIter++ % 30)  // Only print once every 30 seconds.
		{
			StatsPrint ();
		}
	}

STI_BAIL:

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::StatisticsGet
//
// Description: Retrieve a copy of the call statistics.
//
// Abstract:
//
// Returns:
//  None.
//
void CstiProtocolCall::StatisticsGet (
	SstiCallStatistics *pStats)
{
	Lock ();
	*pStats = m_stCallStatistics;

	uint32_t un32Width = 1;
	uint32_t un32Height = 1;
	pStats->Record.eVideoCodec = estiVIDEO_NONE;

	if (m_videoRecord)
	{
		m_videoRecord->CaptureSizeGet (&un32Width, &un32Height);
		pStats->Record.eVideoCodec = m_videoRecord->CodecGet ();
		pStats->Record.nTargetVideoDataRate = m_videoRecord->CurrentBitRateGet () / 1000;
		pStats->Record.nTargetFrameRate = m_videoRecord->CurrentFrameRateGet ();
		
		//Flow Control Data
		uint32_t un32MinSendRate = 0;
		uint32_t un32MaxSendRateWithAcceptableLoss = 0;
		uint32_t un32MaxSendRate = 0;
		uint32_t un32AveSendRate = 0;
		m_videoRecord->FlowControlDataGet (&un32MinSendRate, &un32MaxSendRateWithAcceptableLoss, &un32MaxSendRate, &un32AveSendRate);
		pStats->un32MinSendRate = un32MinSendRate;
		pStats->un32MaxSendRateWithAcceptableLoss = un32MaxSendRateWithAcceptableLoss;
		pStats->un32MaxSendRate = un32MaxSendRate;
		pStats->un32AveSendRate = un32AveSendRate;
	}

	pStats->Record.nVideoWidth = static_cast<int> (un32Width);
	pStats->Record.nVideoHeight = static_cast<int> (un32Height);

	un32Width = 1;
	un32Height = 1;
	if (m_videoPlayback)
	{
		m_videoPlayback->CaptureSizeGet (&un32Width, &un32Height);

		// if codec is estiVIDEO_NONE skip it to keep last known codec for remote stats logging
		EstiVideoCodec eVideoCodec = m_videoPlayback->CodecGet ();
		if (eVideoCodec != estiVIDEO_NONE)
		{
			pStats->Playback.eVideoCodec = eVideoCodec;
		}
		pStats->Playback.nTargetVideoDataRate = m_videoPlayback->FlowControlRateGet () / 1000;
	}
	
	if (m_audioRecord)
	{
		pStats->Record.audioCodec = m_audioRecord->CodecGet ();
	}
	
	if (m_audioPlayback)
	{
		// If codec is none skip it to keep last known codec for remote stats logging
		auto audioCodec = m_audioPlayback->CodecGet ();
		if (audioCodec != estiAUDIO_NONE)
		{
			pStats->Playback.audioCodec = audioCodec;
		}
	}
	

	pStats->Playback.nVideoWidth = static_cast<int> (un32Width);
	pStats->Playback.nVideoHeight = static_cast<int> (un32Height);

	pStats->Playback.nTargetFrameRate = 0;  // Not sure this one makes sense.  We really don't know what the remote
														// system is targeting.  The best we know is the maximum negotiated MPI
														// for each of the possible capture sizes.

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::AutoBandwidthAdjust
//
// Description: Check for the need to do bandwidth adjustment. If needed, do so.
//
// Abstract:
//
// Returns:
//  None.
//
void CstiProtocolCall::AutoBandwidthAdjust ()
{
	uint32_t un32TickCount = stiOSTickGet ();
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	// Determine if remote endpoint is VP with autospeed set to legacy
	EstiBool bRemoteIsLegacyVP = estiFALSE;
	// Get the remote endpoint's auto speed setting
	EstiAutoSpeedMode eRemoteAutoSpeedSetting = estiAUTO_SPEED_MODE_LEGACY;
	RemoteAutoSpeedSettingGet(&eRemoteAutoSpeedSetting);
	// Get the remote endpoint's product name
	std::string RemoteProductName;
	RemoteProductNameGet(&RemoteProductName);
	// Is remote endpoint a VP in Legacy Auto Speed mode
	if (estiAUTO_SPEED_MODE_LEGACY == eRemoteAutoSpeedSetting &&
		(0 == stiStrICmp ("Sorenson Videophone V3", RemoteProductName.c_str ()) ||
		 0 == stiStrICmp ("Sorenson Videophone V2", RemoteProductName.c_str ())))
	{
		bRemoteIsLegacyVP = estiTRUE;
	}
	
	// Get a pointer to the stats element that was just loaded.
	SstiCallStats *pCurStats = m_pStatsElementHead->pNext;

	stiTESTCOND (pCurStats, stiRESULT_ERROR);

	// Are we supposed to still be doing bandwidth adjustments?
	if (m_un32AutoBandwidthAdjustStopTime >= un32TickCount)
	{
		// Has it been long enough since last flow control message that
		// we need to look again to see if we are experiencing data
		// loss?  We only do adjustments for the first n seconds into
		// a call.  See the constant un32BANDWIDTH_TEST_TIME
		if (un32MIN_WAIT_BETWEEN_MSG <= (un32TickCount - m_un32TimeOfFlowCtrlMsg))
		{
			int i;

			// Look at the statistics to see if we need to send a flow
			// control message to the remote endpoint to narrow the
			// video bandwidth.  Look at the most recent data for a
			// packet loss greater than or equal to un32MIN_PKT_LOSS.
			for (i = 0; i < nMIN_SEQ_LOSS && i < nSTATS_ELEMENTS; ++i)
			{
				// If the packet loss is less than the minimum that we
				// are interested in, bail out.
				if (bRemoteIsLegacyVP)
				{
					// This is the VP legacy flow control decision.
					// If remote endpoint is a VP with auto speed set to legacy.
					if (un32MIN_PKT_LOSS >
						(pCurStats->un32APActualPktsLost +
						pCurStats->un32VPPktsLost))
					{
						break;
					}
				}
				else
				{
					// If remote endpoint is any endpoint other than VP or a VP with autospeed set to
					// auto or limited, flow control via SIP re-invite will be sent in severe loss conditions.
					uint32_t un32PacketsLost = pCurStats->un32APActualPktsLost + pCurStats->un32VPPktsLost;
					uint32_t un32TotalPackets = pCurStats->un32APPktsRecv + pCurStats->un32VPPktsRecv;
					if (un32TotalPackets > 0)
					{
						uint32_t un32PktLostPercent = un32PacketsLost * 100 / un32TotalPackets;
						// If loss percentage is greater than 40% for nMIN_SEQ_LOSS, send flow control
						if (un32PktLostPercent < 40)
						{
							break;
						}
					}
					else
					{
						break;
					}

				}
				pCurStats = pCurStats->pNext;

				stiTESTCOND (pCurStats, stiRESULT_ERROR);
			}  // end for

			// Did we find the number of sequential losses needed to
			// require a flow control message?
			if (nMIN_SEQ_LOSS <= i)
			{
				// We have experienced loss enough to require sending
				// a flow control message for the video channel.
				FlowControlSend (estiFALSE, bRemoteIsLegacyVP);
				
				// If losses are this severe for a remote endpoint other than VP with autospeed set to
				// legacy, limit our data rate to half that of the rate requested of remote endpoint.
				if (!bRemoteIsLegacyVP && m_videoRecord)
				{
					m_videoRecord->CurrentBitRateSet(m_abaTargetRate/2);
				}

				// Save the current tick count as the time we last sent
				// a flow control message.
				m_un32TimeOfFlowCtrlMsg = un32TickCount;
			} // end if
		}  // end if
	}

	// If we haven't started it yet and we are receiving video playback data,
	// begin it now.
	else if (0 == m_un32AutoBandwidthAdjustStopTime &&
				0 < pCurStats->un32VPDataRecv)
	{
		m_un32AutoBandwidthAdjustStopTime = un32TickCount + un32BANDWIDTH_TEST_TIME;
		stiDEBUG_TOOL (g_stiRateControlDebug,
			stiTrace ("<CstiProtocolCall::AutoBandwidthAdjust> Starting Auto Bandwidth Detection...\n");
		);
	}

	// If we haven't yet set the completion flag, do so now if
	// the AutoBandwidthAdjustStopTime has been set.
	else if (!m_bAutoBandwidthAdjustComplete && m_un32AutoBandwidthAdjustStopTime)
	{
		m_bAutoBandwidthAdjustComplete = estiTRUE;

		// Inform the Video Record channel that the Auto Bandwidth Adjustment IS complete
		if (m_videoRecord)
		{
			m_videoRecord->AutoBandwidthAdjustCompleteSet (true);
		}

		stiDEBUG_TOOL (g_stiRateControlDebug,
			stiTrace ("<CstiProtocolCall::AutoBandwidthAdjust> Auto Bandwidth Detection is Off\n");
		);
	}

STI_BAIL:

	return;
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::BandwidthDetectionComplete
//
// Description: Are we in the process of doing bandwidth adjustments.
//
// Abstract:
//
// Returns:
//  Whether or not we are completed with the auto-bandwidth detection
//
EstiBool CstiProtocolCall::BandwidthDetectionComplete ()
{
	return m_bAutoBandwidthAdjustComplete;
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiProtocolCall::BandwidthDetectionBegin
//
// Description: Restart bandwidth detection again.
//
// Abstract:
//
// Returns:
//  estiOK always.
//
stiHResult CstiProtocolCall::BandwidthDetectionBegin ()
{
	Lock ();
	
	m_un32AutoBandwidthAdjustStopTime = 0;
	m_un32TimeOfFlowCtrlMsg = 0;
	m_bAutoBandwidthAdjustComplete = estiFALSE;
	
	// Inform the Video Record channel that the Auto Bandwidth Adjustment IS NOT complete
	if (m_videoRecord)
	{
		m_videoRecord->AutoBandwidthAdjustCompleteSet (false);
	}

	Unlock ();
	
	return stiRESULT_SUCCESS;
}


void CstiProtocolCall::VideoPlaybackSizeGet (uint32_t *pun32Width, uint32_t *pun32Height) const
{
	Lock ();
	if (m_videoPlayback)
	{
		m_videoPlayback->CaptureSizeGet (pun32Width, pun32Height);
	}
	else
	{
		*pun32Width = unstiCIF_COLS;
		*pun32Height = unstiCIF_ROWS;
	}
	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolCall::StatsClear
//
// Description: Clear the statistics and status variables
//
// Abstract:
//
// Returns:
//  none
//
void CstiProtocolCall::StatsClear ()
{
	stiLOG_ENTRY_NAME (CstiProtocolCall::StatsClear);

	Lock ();

	m_stCallStatistics = {};

	m_stCallStatistics.Playback.eVideoCodec = estiVIDEO_NONE;
	m_stCallStatistics.Record.eVideoCodec = estiVIDEO_NONE;
	
	// Reset the data tasks values now as well
	if (m_audioPlayback)
	{
		m_audioPlayback->StatsClear ();
	}
	if (m_audioRecord)
	{
		m_audioRecord->StatsClear ();
	}
	if (m_videoPlayback)
	{
		m_videoPlayback->StatsClear ();
	}
	if (m_videoRecord)
	{
		m_videoRecord->StatsClear ();
	}

	// Update the current tick counter
	m_un32PrevStatisticTickCount = stiOSTickGet ();
	
	Unlock ();
} // end StatsClear


void CstiProtocolCall::VideoRecordSizeGet (
	uint32_t *pun32PixelsWide,
	uint32_t *pun32PixelsHigh) const
{
	Lock ();

#if APPLICATION == APP_NTOUCH_PC || defined(stiMVRS_APP) || APPLICATION == APP_MEDIASERVER
	*pun32PixelsWide = unstiCIF_COLS;
	*pun32PixelsHigh = unstiCIF_ROWS;
#else
	// Initialize to SIF so that when there isn't a record channel, the self-view will look
	// good (not stretched).  This is helpful when connected to Hold Servers that don't allow
	// a record channel from the videophone.
	*pun32PixelsWide = unstiSIF_COLS;
	*pun32PixelsHigh = unstiSIF_ROWS;
#endif //else APPLICATION == APP_NTOUCH_PC
	if (m_videoRecord)
	{
		m_videoRecord->CaptureSizeGet (pun32PixelsWide, pun32PixelsHigh);
	}

	Unlock ();
}


/*!\brief Determines and returns the remote name
 *
 * The remote name is determined by considering the various names
 * in this order:
 * 	Contact Name
 *	Display Name
 *	Alternate Name
 */
void CstiProtocolCall::RemoteNameGet (
	std::string *pRemoteName) const
{
	Lock ();

	auto call = CstiCallGet();

	call->RemoteContactNameGet (pRemoteName);
	if (pRemoteName->empty ())
	{
		if ((estiINCOMING == call->DirectionGet () || call->ConferencedGet())
		 && *m_szRemoteDisplayName && !RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_HOLD_SERVER))
		{
			pRemoteName->assign (m_szRemoteDisplayName);
		}
		else //if (*m_szRemoteCallListName)
		{
			call->RemoteCallListNameGet (pRemoteName);
		}
	}

	Unlock ();
}


/*!\brief Get the remote display name
 *
 */
void CstiProtocolCall::RemoteDisplayNameGet (
	std::string *pRemoteDisplayName) const	///< A pointer to a string to store the remote display name
{
	Lock ();

		pRemoteDisplayName->assign (m_szRemoteDisplayName);

	Unlock ();
}


/*!\brief Set the remote display name
 *
 */
void CstiProtocolCall::RemoteDisplayNameSet (
	const char *pszName) /// a pointer to the display name to be set to
{
	Lock ();

	if (pszName)
	{
		strncpy (m_szRemoteDisplayName, pszName, sizeof (m_szRemoteDisplayName) - 1);
		m_szRemoteDisplayName[sizeof (m_szRemoteDisplayName) - 1] = '\0';

		// Ensure that there are no non-printable characters stored in the name
		for (int i = 0; m_szRemoteDisplayName[i]; ++i)
		{
			if (!static_cast<bool> (isprint (m_szRemoteDisplayName[i])))
			{
				m_szRemoteDisplayName[i] = '?';
			}
		}
	}
	else
	{
		m_szRemoteDisplayName[0] = '\0';
	}

	Unlock ();
}


/*!\brief Get the remote name
 *
 */
void CstiProtocolCall::RemoteAlternateNameGet (
	std::string *pRemoteAlternateName) const ///< A pointer to a string to store the remote alternate name
{
	Lock ();

	//
	// If the remote end point is a hold server then return the display name as the remote alternate name
	//
	// Note:  When connecting with a SIP Hold Server, the remote display name will be blank but the remote alternate
	//  name will contain the name to be displayed (generally "Please Hold ...").
	//
	if (RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER) && m_szRemoteDisplayName[0] != '\0')
	{
		pRemoteAlternateName->assign (m_szRemoteDisplayName);
	}
	else
	{
		//
		// Return the remote alternate name.
		//
		pRemoteAlternateName->assign (m_szRemoteAlternateName);
	}

	Unlock ();
}


/*!\brief Set the remote alternate name
 *
 */
void CstiProtocolCall::RemoteAlternateNameSet (
	const char *pszRemoteAlternateName) ///< A pointer to the alternate name to be set to
{
	Lock ();

	if (nullptr != pszRemoteAlternateName)
	{
		strncpy (m_szRemoteAlternateName, pszRemoteAlternateName, sizeof (m_szRemoteAlternateName) - 1);
		m_szRemoteAlternateName[sizeof (m_szRemoteAlternateName) - 1] = '\0';
	} // end if
	else
	{
		m_szRemoteAlternateName[0] = '\0';
	}

	Unlock ();
};


///\brief Sends a single dtmf tone.
///
///\param eDTMFDigit The digit to send
///
void CstiProtocolCall::DtmfToneSend (EstiDTMFDigit eDTMFDigit)
{
	Lock ();
	if (m_audioRecord)
	{
		m_audioRecord->DtmfToneSend (eDTMFDigit);
	}
	Unlock ();
}


///\brief Returns the call result stored in the CstiCall object.
///
EstiCallResult CstiProtocolCall::ResultGet () const
{
	return (CstiCallGet()->ResultGet());
}


EstiBool CstiProtocolCall::CallerBlocked (
	IstiBlockListManager2 *pBlockListManager)
{
	bool bResult = false;
	
#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER) && (APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL) // The hold server determines blocked calls via the Relay Services, Media Server uses a Web Service
	Lock ();

	auto call = CstiCallGet ();
	
	//
	// We only care to check this if the call is an inbound call.
	//
	if (estiINCOMING == call->DirectionGet ())
	{
		const SstiUserPhoneNumbers *pPhoneNumbers = call->RemoteCallInfoGet ()->UserPhoneNumbersGet ();

		if (pPhoneNumbers->szRingGroupLocalPhoneNumber[0] || pPhoneNumbers->szRingGroupTollFreePhoneNumber[0])
		{
			bResult = pBlockListManager->CallBlocked (pPhoneNumbers->szPreferredPhoneNumber) ||
				pBlockListManager->CallBlocked (pPhoneNumbers->szRingGroupLocalPhoneNumber) ||
				pBlockListManager->CallBlocked (pPhoneNumbers->szRingGroupTollFreePhoneNumber);
		}
		else
		{
			bResult = pBlockListManager->CallBlocked (pPhoneNumbers->szPreferredPhoneNumber) ||
				pBlockListManager->CallBlocked (pPhoneNumbers->szTollFreePhoneNumber) ||
				pBlockListManager->CallBlocked (pPhoneNumbers->szLocalPhoneNumber) ||
				pBlockListManager->CallBlocked (pPhoneNumbers->szHearingPhoneNumber) ||
				pBlockListManager->CallBlocked (pPhoneNumbers->szSorensonPhoneNumber);
		}
	}
	
	Unlock ();
#endif // ifndef stiHOLDSERVER
	return bResult ? estiTRUE : estiFALSE;
}

///\brief Returns the call ID (an empty string in this case)
///
void CstiProtocolCall::CallIdGet (std::string *pCallId)
{
	pCallId->clear();
}


///\brief Returns the original call ID (an empty string in this case)
///
void CstiProtocolCall::OriginalCallIDGet (std::string *pCallId)
{
	pCallId->clear();
}

// eof

