/*!
* \file CstiVideoPlayback.h
* \brief See CstiVideoPlayback.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2000-2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

//
// Includes
//
#include "stiSVX.h"
#include "stiTrace.h"
#include "CstiRtpSession.h"
#include "PlaybackFlowControl.h"
#include "CstiSignal.h"
#include "CstiVideoPlaybackRead.h"
#include "MediaPlayback.h"
#include "stiPayloadTypes.h"
#include "IstiVideoOutput.h"
#include "CstiVideoFrame.h"
#include "RvSipMid.h"
#include <vector>
#include <chrono>
#include <mutex>

// Enable the missing packet detection for sorenson high quality video
//#define stiNO_H264_MISSING_PACKET_DETECTION
#define stiNO_H263_MISSING_PACKET_DETECTION

class IstiVideoOutput;

class CstiVideoPlayback : public vpe::MediaPlayback<CstiVideoPlaybackRead, CstiVideoPacket, EstiVideoCodec, TVideoPayloadMap>
{
public:
	CstiVideoPlayback () = delete;

	CstiVideoPlayback (
	    std::shared_ptr<vpe::RtpSession> session,
		uint32_t callIndex,
		uint32_t maxRate,
		EstiAutoSpeedMode autoSpeedMode,
		bool bRtpKeepAliveEnable,
		RvSipMidMgrHandle sipMidMgr,
		int remoteSipVersion);

	CstiVideoPlayback (const CstiVideoPlayback &other) = delete;
	CstiVideoPlayback (CstiVideoPlayback &&other) = delete;
	CstiVideoPlayback &operator= (const CstiVideoPlayback &other) = delete;
	CstiVideoPlayback &operator= (CstiVideoPlayback &&other) = delete;

	~CstiVideoPlayback() override = default;

	int ClockRateGet() const override;

	stiHResult Initialize (
		const TVideoPayloadMap &payloadMap) override;

	void Uninitialize () override;

	stiHResult Shutdown () override;

	stiHResult Answer (
		bool startKeepAlives) override;

	// Data flow functions
	stiHResult DataChannelInitialize (
		bool bStartKeepAlives) override;

	stiHResult DataChannelClose () override;

	stiHResult DataChannelHold () override;
	stiHResult DataChannelResume () override;

	void PrivacyModeSet (
		EstiSwitch mode);

	struct SstiVPStats
	{
		uint32_t packetsDiscardedEmpty = 0;			// Number of packets with no payload
		uint32_t packetsUsingPreviousSSRC = 0;			// Number of packets using the previous SSRC
		uint32_t packetsReceived = 0;
		uint32_t packetsLost = 0;					// Packets not received but waited for
		uint32_t un32FrameCount = 0;
		uint32_t un32ByteCount = 0;

		uint32_t packetsRead = 0;						// Actual packets read by VideoPlaybackRead task
		uint32_t keepAlivePackets = 0;
		uint32_t packetQueueEmptyErrors = 0;
		uint32_t un32UnknownPayloadTypeErrors = 0;
		uint32_t un32PayloadHeaderErrors = 0;
		uint32_t packetsDiscardedMuted = 0;
		uint32_t packetsDiscardedReadMuted = 0;

		std::chrono::milliseconds timeMutedByRemote = std::chrono::milliseconds::zero ();	// Total time video has been muted by either privacy or hold
		uint32_t un32OutOfOrderPackets = 0;
		uint32_t un32MaxOutOfOrderPackets = 0;
		uint32_t un32DuplicatePackets = 0;
		uint32_t un32VideoPlaybackFrameGetErrors = 0;	// Frames discarded because the platform had no available frame buffers
		uint32_t un32ErrorConcealmentEvents = 0;
		uint32_t un32PFramesDiscardedIncomplete = 0;
		uint32_t un32IFramesDiscardedIncomplete = 0;
		uint32_t un32GapsInFrameNumber = 0;
		uint32_t aun32PacketPositions[MAX_STATS_PACKET_POSITIONS] = {};

		// The values in this section are relative to the whole call (or since StatsClear was last called).
		int keyframeRequests {0};
		int totalKeyframesRequested {0};	// Number of keyframes requested
		int keyframesReceived {0};
		uint32_t totalKeyframesReceived = 0;		// Number of keyframes received
		uint32_t un32KeyframeMaxWait = 0;  // In milliseconds
		uint32_t un32KeyframeMinWait = 0;  // In milliseconds
		uint32_t un32KeyframeAvgWait = 0;  // In milliseconds
		float     fKeyframeTotalWait = 0.0F;  // In seconds
		bool      bTunneled = false;
		uint32_t highestPacketLossSeen = 0;
		uint32_t countOfCallsToPacketLossCountAdd = 0;
		uint32_t duplicatePacketsReceived = 0;

		uint32_t framesSentToPlatform = 0;
		uint32_t partialKeyFramesSentToPlatform = 0;
		uint32_t partialNonKeyFramesSentToPlatform = 0;
		uint32_t wholeKeyFramesSentToPlatform = 0;
		uint32_t wholeNonKeyFramesSentToPlatform = 0;
		
		uint32_t nackRequestsSent = 0;
		uint32_t rtxPacketsReceived = 0;
		uint32_t actualPacketLoss = 0; // Number of packets missing before repair/retransmission.
		uint32_t playbackDelay = 0;
		uint32_t rtxFromNoData = 0;
		uint32_t burstPacketsDropped = 0;
	};
	
	EstiResult StatsCollect (
		SstiVPStats *pStats);

	void StatsClear () override;

	EstiResult CaptureSizeGet (
		uint32_t *pun32PixelsWide,
		uint32_t *pun32PixelsHigh) const;

	void PayloadMapSet (
		const TVideoPayloadMap &pPayloadMap) override;
	
	void flowControlReset ();

public:
	CstiSignal<> keyframeRequestSignal;
	CstiSignal<bool> privacyModeChangedSignal;

private:

	struct SstiStats : SstiVPStats
	{
		uint32_t un32KeyframeTotalTicksWaited = 0;  // Sum of the wait times for all keyframe requests
		uint32_t un32KeyframeRequestedBeginTick = 0; // When the current request started
	};
	
	struct ReceivedPackets
	{
		std::deque<std::shared_ptr<CstiVideoPacket>> processedPackets;
		bool markerSet = false;
		bool frameComplete = false;
	};
	
	struct PlaybackDelayTime
	{
		uint32_t firstPacketTimestamp = 0;
		uint32_t arrivalOfFirstPacket = 0;
	};

	stiHResult Muted (
		eMutedReason eReason) override;

	stiHResult Unmuted (
		eMutedReason eReason) override;

	void VideoStreamStop ();
	void VideoStreamStart ();

	// Only added for the interface, doesn't need to do anything
	stiHResult DataInfoStructLoad () override { return stiRESULT_SUCCESS; }

	stiHResult DataChannelInitialize () override;

	EstiResult VideoFramePut (
		CstiVideoFrame *poPacket);
		
	CstiVideoFrame * VideoFrameGet ();

	void VideoFrameInitialize (
		CstiVideoFrame *poFrame,
		const std::shared_ptr<CstiVideoPacket> &packet);
	
	EstiResult VideoPacketQueueEmpty ();

	EstiResult KeyframeReceived (uint32_t un32Timestamp);

	EstiResult KeyframeRequest (uint32_t un32Timestamp);

	EstiResult ProcessAvailableData ();

	void FirstPacketSetup (const std::shared_ptr<CstiVideoPacket> &packet);
	
	static stiHResult CallbackFromRead (
		int32_t n32Message, 	///< type of message
		size_t MessageParam,	///< holds data specific to the function to call
		size_t CallbackParam);	///< points to the instantiated VideoPlayback object

	static stiHResult CallbackFromVideoOutput (
		int32_t n32Message, 	///< type of message
		size_t MessageParam,	///< holds data specific to the function to call
		size_t CallbackParam);	///< points to the instantiated VideoPlayback object

	EstiResult SetVideoSyncInfo (
		uint32_t unNTP32sec,
		uint32_t un32NTPfrac,
		uint32_t un32Timestamp);

	//
	// Supporting functions
	//
	void FrameCountAdd (
		bool frameIsComplete,
		bool frameIsKeyFrame);
	void PacketLossCountAdd (
		uint32_t un32AmountToAdd);
	void PacketsReceivedCountAdd (int numPackets);
	stiHResult SetDeviceCodec (EstiVideoCodec eMode) override;

	EstiResult VideoPacketToSyncQueue (
		bool bFrameIsComplete,
		std::deque<std::shared_ptr<CstiVideoPacket>> *packets);
	EstiResult SyncPlayback ();

	//
	// Packet handling functions
	//
	void FrameClear ();
	void SyncQueueClear ();

	EstiBool PacketCheckPictureStartCode (const std::shared_ptr<CstiVideoPacket> &packet);

	EstiPacketResult PacketMerge (
		CstiVideoFrame *poFrame,
		const std::shared_ptr<CstiVideoPacket> &srcPacket);

	stiHResult NALUnitHeaderAdd (
		CstiVideoFrame *poFrame,
		uint32_t un32Size);

	EstiResult ReadPacketProcess (const std::shared_ptr<CstiVideoPacket> &packet);
	EstiResult PutPacketIntoQueue (const std::shared_ptr<CstiVideoPacket> &packet);
	EstiResult PacketProcess (std::shared_ptr<CstiVideoPacket> packet);
	stiHResult H263PacketProcess (const std::shared_ptr<CstiVideoPacket> &packet);
	EstiResult H264PacketProcess (const std::shared_ptr<CstiVideoPacket> &packet);
	EstiResult H265PacketProcess (const std::shared_ptr<CstiVideoPacket> &packet);
	EstiResult PacketQueueProcess (const std::shared_ptr<CstiVideoPacket> &readPacket, bool *pbReadPacketFlushed);

	EstiPacketResult H264FUAToVideoFrame (
		CstiVideoFrame * poFrame,	// Destination Packet
		const std::shared_ptr<CstiVideoPacket> &srcPacket);	// Source Packet

	EstiPacketResult H264STAPAOrH265APToVideoFrame (
		CstiVideoFrame * poFrame,
		const std::shared_ptr<CstiVideoPacket> &srcPacket,
		const uint8_t headerSize);

	EstiPacketResult H265FUToVideoFrame (
		CstiVideoFrame * poFrame,	// Destination Packet
		const std::shared_ptr<CstiVideoPacket> &srcPacket);	// Source Packet

	EstiPacketResult H265APToVideoFrame (
	   CstiVideoFrame * poFrame,
	   const std::shared_ptr<CstiVideoPacket> &srcPacket);

	void ConnectSignals ();
	void DisconnectSignals ();
	void KeyframeRequestNoLock ();

	bool payloadChange (
		const TVideoPayloadMap &payloadMap,
		int8_t payload);


	//
	// Event Handlers
	//
	void EventKeyframeTimer ();
	void EventKeepAliveTimer ();
	void EventTmmbrFlowControlTimer ();
	void EventDataAvailable ();
	void EventVideoOutputReadyChanged (bool ready);
	void EventKeyframeRequest (uint32_t timestamp);
	void EventVideoPlaybackSizeChanged (uint32_t width, uint32_t height);
	void EventNackTimer ();
	void EventCheckForCompleteFrame ();
	void EventSyncPlayback ();
	void EventJitterCalculate ();
	
	void assembleAndSendFrame ();
	void dumpJitterBuffer ();
	void jitterCalculate ();
	void oldestProcessedPacketsToFrame (bool frameVerifiedComplete);
	void checkOldestPacketsSendCompleteFrame ();
	bool packetsCompleteFrame (const std::unique_ptr<ReceivedPackets> &packetsToCheck);
	uint32_t nextTimeToSend (const std::map<uint32_t, std::unique_ptr<ReceivedPackets>> &frames);

private:
	int m_nMuted = 0; 		///< This is a bit mask of the muted reasons.

	// Instance for video device controller
	IstiVideoOutput *m_pVideoOutput = nullptr;

	EstiBool m_bLastFrameGetSuccessful = estiTRUE;

	// For re-requesting a keyframe
	vpe::EventTimer m_keyFrameTimer;

	uint32_t m_un32sSrc = 0;
	uint32_t m_un32sSrcPrevious = 0;
	uint32_t m_un32TicksWhenLastPlayed = 0;
	bool m_bStartWithOldest = false;
	bool m_bKeyframeSentToPlatform = false;
											// in the next video packet.
#if !defined(stiNO_H263_MISSING_PACKET_DETECTION) || !defined(stiNO_H264_MISSING_PACKET_DETECTION)
	EstiBool m_bMissingPacket = estiFALSE;
#endif
	EstiBool m_bKeyFrameRequestPending = estiFALSE;
	EstiBool m_bKeyFrameRequested = estiFALSE;			// Key frame has been requested?

	CFifo<CstiVideoFrame> m_oSyncQueue;

	CFifo<CstiVideoFrame> m_oFrameQueue;		// Holding buffer which will use to build frame
	CstiVideoFrame * m_poFrame = nullptr;					// The packet object which contains the frame
												// we are currently building
	CstiVideoFrame *m_poPendingPlaybackFrame = nullptr;
	EstiBool m_bFrameComplete = estiFALSE;				// Indicator of completeing a frame

	uint32_t m_un32LastReceivedSequence = (~0);
	uint32_t m_un32PreviousFrameLastSq = 0;

	// for H264
	uint32_t m_un32FNALExpectedPacket = 0;
	bool m_bStartedFNAL = false;
	uint32_t m_un32NALUnitStart = 0;

	vpe::VideoFrame m_astVideoFrameStruct[nMAX_FRAME_BUFFERS]{};

	//
	// Structure used for generating byte count, frame rate, data rate, and loss statistics
	//
	SstiStats m_Stats;

#ifdef stiDEBUGGING_TOOLS
	int m_nPacketsPerFrame = 0;
#endif

	bool m_bVideoPlaybackStarted = false;

	uint32_t m_un32KeyframeNeededForTimestamp = 0;

	uint32_t m_unLastKnownWidth = 0;
	uint32_t m_unLastKnownHeight = 0;
	int m_nFirSeqNbr = 1;

	// These objects allow us to disconnect from the signals
	CstiSignalConnection m_keyframeNeededSignalConnection;
	CstiSignalConnection m_decodeSizeChangedSignalConnection;

	CstiSignalConnection::Collection m_signalConnections;

	// Packet stats since last m_flowControlTimer timeout
	struct FlowControlStats
	{
		std::chrono::steady_clock::time_point startTime;  // Start of this time period
		uint32_t packetsReceived = 0;  // Number of packets received during this time period
		uint32_t packetsLost = 0;      // Number of packets lost during this time period
		uint32_t payloadSizeSum = 0;   // Sum of packet payload sizes received during this time period
		uint32_t overheadSizeSum = 0;  // Sum of packet overhead sizes received during this time period
		uint32_t actualPacketsLost = 0;  // requests we have sent.
	} m_flowControlStats;

	// AutoSpeed algorithm for providing rate changes
	vpe::PlaybackFlowControl m_flowControl;

	// For TMMBR rate control
	vpe::EventTimer m_flowControlTimer;

	// These reflect whether the current payload supports these feedback mechanisms
	bool m_rtcpFeedbackFirEnabled = false;
	bool m_rtcpFeedbackPliEnabled = false;
	bool m_rtcpFeedbackTmmbrEnabled = false;
	bool m_rtcpFeedbackNackRtxEnabled = false;

	// Video Playback Channel
	EstiAutoSpeedMode m_autoSpeedMode = estiAUTO_SPEED_MODE_AUTO;
	
	std::map<uint32_t, std::unique_ptr<ReceivedPackets>> m_processedPackets;
	std::map<uint32_t, std::unique_ptr<ReceivedPackets>> m_jitterBuffer;
	uint32_t m_lastSeqNumInBuffer = 0;
	uint32_t m_lastProcessedPacket = 0;
	uint32_t m_previousFrameCount = 0;
	uint32_t m_lastAssembledFrameTS = 0;
#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	uint8_t m_framesToBuffer = 4;
#else
	uint8_t m_framesToBuffer = 8;
#endif
	vpe::EventTimer m_nackTimer;
	vpe::EventTimer m_jitterTimer;

	PlaybackDelayTime m_playbackDelayTime;
};
