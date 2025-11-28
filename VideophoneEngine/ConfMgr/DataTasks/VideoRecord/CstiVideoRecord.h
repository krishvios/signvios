/*!
* \file CstiVideoRecord.h
* \brief See CstiVideoRecord.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2000 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

//
// Includes
//
#include <array>
#include "MediaRecord.h"
#include "CstiVideoPacket.h"
#include "stiTrace.h"
#include "IstiVideoInput.h"
#include "PacketPool.h"
#include "CstiPacketQueue.h"
#include "RvSipMid.h"
#include "CstiRtpSession.h"
#include "RTCPFlowControl.h"   // RTCP RR flow control (non-TMMBR)
#include "RecordFlowControl.h" // TMMBR enabled flow control
#include "RtpPayloadAddon.h"
#include "RTCPFlowControl.h"
#include "RtpSender.h"
#include "RtxSender.h"


//
// Constants
//

// According to Radvision the *maximum* packet overhead is 76 bytes, plus header extensions.
// Usually we're well below that value though, at 12 bytes
#define stiMAX_RTP_PACKET_OVERHEAD 80

// The maximum number of packets stored in RtxSender plus approximately 1/3 second of packets for RtpSender (if each frame had 10 packets)
#if DEVICE == DEV_NTOUCH_VP4
//TODO: mlong - need to determine why this requires an increase for Lumina2 100->500
#define stiMAX_VR_BUFFERS (nMAX_VP_BUFFERS + 500)
#else
#define stiMAX_VR_BUFFERS (nMAX_VP_BUFFERS + 100)
#endif //DEVICE == DEV_NTOUCH_VP4

//
// Typedefs
//
struct SstiVideoRecordSettings
{
	int nCurrentBitRate = 256000;
	int8_t payloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
	int8_t rtxPayloadType = INVALID_DYNAMIC_PAYLOAD_TYPE;
};

enum EstiAspectRatio
{
	EstiAspectRatio_4x3,  //4x3 (Common Full Resolution)
	EstiAspectRatio_16x9, //16x9 (Common Wide Resolution)
	EstiAspectRatio_11x9, //CIF Resolutions
	EstiAspectRatio_22x15 //SIF Resolutions
};

enum EstiStatusFramePacketType
{
	EstiStatusFrameHold,
	EstiStatusFramePrivacy
};

struct SstiVideoPacket
{
public:
	uint32_t n32Length;
	uint8_t* pData;
	EstiBool bIsKeyFrame;
	EstiVideoFrameFormat eEndianFormat;
	EstiAspectRatio eAspectRatio;
	EstiVideoCodec eCodec;
	EstiStatusFramePacketType ePacketType;
};


//
// Forward Declarations
//

//
// Globals
//

//
// Class Declarations
//

class CstiVideoRecord : public vpe::MediaRecord<EstiVideoCodec, SstiVideoRecordSettings>
{
public:

	CstiVideoRecord () = delete;

	CstiVideoRecord (
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t callIndex,
		uint32_t initialMaxRate,
		ECodec codec,
		EstiPacketizationScheme packetizationScheme,
		RvSipMidMgrHandle sipMidMgr,
		const RTCPFlowcontrol::Settings &flowControlSettings,
		const std::string &callId,
		EstiInterfaceMode localInterfaceMode,
		uint32_t reservedBitRate);

	CstiVideoRecord (const CstiVideoRecord &other) = delete;
	CstiVideoRecord (CstiVideoRecord &&other) = delete;
	CstiVideoRecord &operator= (const CstiVideoRecord &other) = delete;
	CstiVideoRecord &operator= (CstiVideoRecord &&other) = delete;

	~CstiVideoRecord () override;

	stiHResult Initialize(
		bool sorensonPrivacy,
		bool mcuConnection);

	void Close ();
	void DataChannelClose () override;
	
	void Startup () override;
	void Shutdown () override;

	struct SstiVRStats
	{
		// The values in this section are reset after each call to StatsCollect
		uint32_t packetsSent = 0;
		uint32_t packetSendErrors = 0;
		uint32_t un32FrameCount = 0;
		uint32_t un32ByteCount = 0;
		
		// The values in this section are relative to the whole call (or since StatsClear was last called).
		int keyframesRequested{0};
		int totalKeyframesRequested{0};
		int keyframesSent{0};
		uint32_t totalKeyframesSent{0};
		uint32_t un32KeyframeMaxWait{0};  // In milliseconds
		uint32_t un32KeyframeMinWait{0};  // In milliseconds
		uint32_t un32KeyframeAvgWait{0};  // In milliseconds
		uint32_t un32FramesLostFromRcu{0};
		uint32_t countOfPartialFramesSent{0};
		
		uint32_t rtxPacketsNotFound = 0;
		uint32_t rtxPacketsIgnored = 0;
		uint32_t rtxPacketsSent = 0;
		uint32_t nackRequestsReceived = 0;
		uint64_t rtxKbpsSent = 0;
		
		uint32_t rtcpTotalJitter = 0; // Total sum of jitter calculated on call used later to find average.
		uint32_t rtcpTotalRTT = 0; // Total sum of round trip time (RTT) calculated on call used later to find average.
		uint32_t rtcpCount = 0; // Amount of times we processed RTCP used to find average.
	};
	
	void SorensonPrivacySet (bool bSorensonPrivacy);
	bool SorensonPrivacyGet () const;

	stiHResult RequestKeyFrame ();

	stiHResult StatsCollect (SstiVRStats *pStats);
	
	void StatsClear () override;

#ifdef stiHOLDSERVER
	stiHResult HSEndService(EstiMediaServiceSupportStates eMediaServiceSupportState);
	void HSServiceStartStateSet(EstiMediaServiceSupportStates eMediaServiceSupportState);
#endif

	void FlowControlRateSet (int rate) override;

	stiHResult Hold (EHoldLocation location) override;
	stiHResult Resume (EHoldLocation location) override;

	void AutoBandwidthAdjustCompleteSet (bool complete) { m_bAutoBandwidthDetectionComplete = complete; };
	void CaptureSizeGet (uint32_t *pixelWidth, uint32_t *pixelHeight) const;
	int CurrentBitRateGet () const;
	void CurrentBitRateSet (int bitRate);
	int CurrentFrameRateGet () const;
	stiHResult LocalPrivacyComplete ();
	int RemotePtzCapsGet () const;

	void FlowControlDataGet (
			uint32_t *minSendRate,
			uint32_t *maxSendRateWithAcceptableLoss,
			uint32_t *maxSendRate,
			uint32_t *aveSendRate) const;

	void RemoteDisplaySizeSet (
		unsigned int displayWidth,
		unsigned int displayHeight);

	void RemoteDisplaySizeGet (
		unsigned int *remoteDisplayWidth,
		unsigned int *remoteDisplayHeight) const;
	
	void RtcpFeedbackEventsSet ();
	
	void ReserveBitRate (const uint32_t &reservedBitRate);

	EstiVideoCodec CodecGet ();

public:
	CstiSignal<> holdserverVideoFileCompletedSignal;

private:

	// Since this class does not override the virtual method Initialize, it needs
	// to be hidden so it is not accessible.
	using vpe::MediaRecord<EstiVideoCodec, SstiVideoRecordSettings>::Initialize;

	bool PrivacyGet () const override;

	uint32_t AveragePacketSizeSentGet ();

	stiHResult CalculateCaptureSize (
		unsigned int unMaxFS,
		unsigned int unMaxMBps,
		int nCurrentDataRate,
		EstiVideoCodec eVideoCoded,
		unsigned int *punVideoSizeCols,
		unsigned int *punVideoSizeRows,
		unsigned int *punFrameRate);

	stiHResult Mute (
		eMuteReason eReason) override;

	stiHResult Unmute (
		eMuteReason eReason) override;

	stiHResult DataInfoStructLoad (const SstiSdpStream &sdpStream, const SstiMediaPayload &offer, int8_t secondaryPayloadId) override;

	void privacyModeSetAndNotify (
	    EstiSwitch enabled);

	EstiPacketizationScheme PacketizationSchemeGet () const;

	void PacketizationSchemeSet (
	    EstiPacketizationScheme packetizationScheme);

private:

	struct SstiStats : SstiVRStats
	{
		uint32_t un32Keyframe_TotalTicksWaited = 0;  // Sum of the wait times for all keyframe requests
		uint32_t un32KeyframeRequestedBeginTick = 0; // When the current request started
	};
	
	stiHResult DataChannelInitialize () override;
		
	// Instance for video device controller
	IstiVideoInput *m_pVideoInput = nullptr;

	IstiVideoInput::SstiVideoRecordSettings m_stVideoSettings;

	EstiBool m_bKeyFrameRequested = estiFALSE;
	uint32_t m_un32PrevTime = 0;
	int m_nFlushFrames = 0;
	EstiBool m_bCheckKeyFrame = estiFALSE;
	EstiBool m_bCaptureVideo = estiFALSE;				// Indicator of sending a packet to socket
	EstiBool m_bVideoRecordStart = estiFALSE;

	bool m_bSorensonPrivacy = estiFALSE;
	bool m_bMCUConnection = estiFALSE;
	int m_nPacketLossExceededCount = 0;
	int m_nHoldPrivacyPacketBurst = 0;
	// For Sending Hold / Privacy Video
	vpe::EventTimer m_holdPrivacyTimer;

	std::array<uint8_t, stiMAX_VIDEO_RECORD_BUFFER + 1> m_videoBuffer {};

	//
	// Structure used for generating byte count, data rate, and loss statistics
	//
	SstiStats m_Stats;
	
	EstiBool m_bFirstVideoNTPTimestamp{estiFALSE};

	//
	// Supporting Functions
	//
	void TimerStart();

	stiHResult PacketFromDriverProcess ();

	stiHResult PacketConfigure (std::shared_ptr<vpe::RtpPacket> packet, bool isKeyframe, bool isLastInFrame, uint32_t timestamp);
	stiHResult PacketsProcess (SstiRecordVideoFrame *frame, uint32_t rtpTimestamp, bool *partialSend);

	stiHResult HoldPrivacyPacketProcess();
	
	void FrameCountAdd (
		uint32_t un32AmountToAdd);

	static stiHResult CallbackFromVideoInput (
		int32_t n32Message,	// holds the type of message
		size_t MessageParam,	// holds data specific to the function to call
		size_t CallbackParam,	// points to the instantiated VideoPlayback object
		size_t CallbackFromId);

	void ConnectSignals ();
	void DisconnectSignals ();

	//
	// Event Handlers
	//
	void EventHoldPrivacyTimer ();
	void EventFlowControlTimer ();
	void EventAutoSpeedTimer ();
	void EventTmmbrRequest (uint32_t maxBitRate);
	void EventVideoPrivacySet (bool enable);
	void EventPacketAvailable ();
	void EventDataStart () override;
	void EventKeyframeRequested (uint32_t ssrcAddressee);
	void EventReserveBitRate (uint32_t reservedBitRate);
	void eventCurrentBitRateSet (int bitRate);
	void eventRecalculateCaptureSize ();

#ifdef stiHOLDSERVER
	void EventHSEndService (EstiMediaServiceSupportStates state);
	void EventHoldserverVideoComplete();
#endif

	void videoSettingsSet (
		const IstiVideoInput::SstiVideoRecordSettings &settings);

	static RvStatus RtcpFbFirCallback (
			RvRtcpSession rtcpSession,
			RvUint32 ssrcAddressee,
			RvUint32 ssrcRequested);

	static RvStatus RtcpFbPliCallback (
			RvRtcpSession rtcpSession,
			RvUint32 ssrcAddressee);

	static RvStatus RtcpFbAfbCallback (
			RvRtcpSession,
			RvUint32,		/* addressee ssrc */
			RvUint32*,		/* fci            */
			RvUint32);		/* fciLen         */

	static RvStatus RtcpFbTmmbrCallback (
			RvRtcpSession rtcpSession,
			RvUint32 ssrcAddressee,
			RvUint32 maxBitrate,
			RvUint32 measuredOverhead);
	
	static RvStatus RtcpFbNACKCallback (
			RvRtcpSession rtcpSession,
			RvUint32 ssrcAddressee,
			RvUint16 packetID,
			RvUint16 bitMask);

	static void RVCALLCONV VideoRTCPReportProcess (
		RvRtcpSession hRTCP,	// The RTCP session for which the data has arrived
		void* pContext,
		RvUint32 ssrc);

	static RvBool RVCALLCONV VideoEnumerator (
		RvRtcpSession hRTCP,	// Handle to RadVision RTCP Session
		RvUint32 ssrc,		// Synchronization Source ID
		void *pContext);


	stiHResult Setup ();

	//Hold Video
	SstiVideoPacket* m_pHoldPrivacyPacket = nullptr;

	RvUint32 m_un32PrevSequenceRR = 0;
	int m_nNumPackets = 0;
#ifndef stiNO_LIP_SYNC
	uint32_t m_un32MostNTPTimestamp = 0;
	uint32_t m_un32LeastNTPTimestamp = 0;
	int m_nNTPSolved = 0;
	EstiBool m_bIsNTPSwapped = estiFALSE;
#endif
	
	uint32_t m_un32RTPPacketSizeSum = 0;
	uint32_t m_un32RTPPacketCount = 0;

	stiHResult HoldPrivacyVideoPacketGet(SstiRecordVideoFrame *pVideoPacket, EstiStatusFramePacketType ePacketType);

	EstiAspectRatio AspectRatioCalculate(unsigned int Width, unsigned int Height);

	stiHResult HoldVideoFilenameGet(EstiAspectRatio ratio, EstiVideoCodec, std::string* filename);
	stiHResult PrivacyVideoFilenameGet(EstiAspectRatio ratio, EstiVideoCodec, std::string* filename);
	stiHResult VideoDataLoadFromFile(std::string* filename, SstiVideoPacket* pOutPacket);
	void ByteCountAdd (uint32_t un32AmountToAdd) override;

private:
	std::shared_ptr<vpe::RtpSession> m_session = nullptr;
	
	std::unique_ptr<vpe::RtpSender> m_rtpSender;
 	std::shared_ptr<vpe::RtpSendQueue> m_sendQueue;
	std::unique_ptr<vpe::RtxSender> m_rtxSender;
 	vpe::RtpPacketPool m_packetPool;
 	CstiSignalConnection m_packetSendSignalConnection;
 	CstiSignalConnection m_packetDropSignalConnection;

	CstiSignalConnection m_videoPrivacySetSignalConnection;
	CstiSignalConnection m_packetAvailableSignalConnection;
	CstiSignalConnection m_captureCapabilitiesChangedSignalConnection;
#if APPLICATION == APP_HOLDSERVER
	CstiSignalConnection m_holdserverVideoCompleteSignalConnection;
#endif
	// TMMBR flow control
	vpe::EventTimer m_flowControlTimer;
	vpe::RecordFlowControl m_flowControl;

	// AutoSpeed flow control
	vpe::EventTimer m_autoSpeedTimer;
	std::list<SstiRTCPInfo> m_receiverReports;
	std::mutex m_reportMutex;
	uint32_t m_remb {0};

	RTCPFlowcontrol m_autoSpeedFlowControl;
	bool m_bVRCifCapable = false; // Can we use this capture size
	bool m_bVRSifCapable = false; // Can we use this capture size
	bool m_bVRQCifCapable = false; // Can we use this capture size
	bool m_bVRSQCifCapable = false; // Can we use this capture size
	int m_nRmtPtzCaps = 0;
	uint32_t m_nCallIndex = 0;
	unsigned int m_unMaxMBps = 0;
	unsigned int m_unMaxFS = 0;
	bool m_bUnmuting = false;
	EstiInterfaceMode m_localInterfaceMode = {};
	bool m_bAutoBandwidthDetectionComplete = false;
	int m_nCurrentCaptureSizeIndex16x9 = -1;
	int m_nCurrentCaptureSizeIndex4x3 = -1;
	bool m_b16x9Disallowed = false;
	SstiCaptureSizeTable *m_pstCurrentSize = nullptr;
	unsigned int m_remoteDisplayWidth = 0;
	unsigned int m_remoteDisplayHeight = 0;

	CstiSignalConnection::Collection m_signalConnections;
	
	uint32_t m_previousTimeStamp = 0;

	std::chrono::steady_clock::time_point m_lastPacketSendTime;

#ifdef stiMVRS_APP
	bool m_bUse1080P = false;
	bool m_bUse1008 = false;
	bool m_bUse864 = false;
	bool m_bUse720P = false;
	bool m_bUse576 = false;
	bool m_bUse480 = false;
	bool m_bUse432 = false;
	bool m_bUse288 = false;
	bool m_bTried1080P = false;
	bool m_bTried720P = false;
	bool m_bTried480 = false;
	bool m_bTried16x9 = false;
	bool m_bLastCapture16x9 = false;
#endif
};

