////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Module Name: CstiDataTaskCommonP
//
//  File Name: CstiDataTaskCommonP.h
//
//	Abstract:
//		Data tasks common (pipe style) definitions
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIDATATASKCOMMONP_H
#define CSTIDATATASKCOMMONP_H

//
// Includes
//
#include "stiSVX.h"
#include "rvrtcpfb.h"
#include "RvSipCallLeg.h"
#include "stiRtpChannelDefs.h"
#include "CstiRtpSession.h"
#include <memory>
#include <mutex>

//
// Constants
//

// Size allocated for each video buffer. This includes RTP header, Payload
// header, and viodeo data. The video data includes all packets (slices) for
// one video frame.
#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_MEDIASERVER || APPLICATION == APP_HOLDSERVER
// This value is approximately double the largest encoded frame size seen for
// 1920x1080 @ ~3Mbps
#define stiMAX_VIDEO_RECORD_BUFFER 307200
#else
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
#define stiMAX_VIDEO_RECORD_BUFFER (1920 * 1080)
#else
#define stiMAX_VIDEO_RECORD_BUFFER (352 * 288)
#endif
#endif


// Room needed for the RTP header
#define stiRTP_HEADER_SIZE 12

// Room needed for the largest payload header we use
#define stiH263_MODE_B_PAYLOAD_HEADER_SIZE 12

// The maximum size of a video packet (one slice) produced by the driver.
#define stiMAX_VIDEO_RECORD_PACKET_BUFFER 1500
// The maximum packet size has been set to 1300 to work well with certain networks.  Field
// testing has proven out that 1300 works better than 1500.
#define stiMAX_VIDEO_RECORD_PACKET	1300

#define stiMINIMUM_RTCP_INTERVAL 5 // Number of seconds as the minimum rtcp interval

// Make sure that the video record packet buffer size is large enough for everything
// needed to fit into it
#if ((stiMAX_VIDEO_RECORD_PACKET + \
		stiRTP_HEADER_SIZE + \
		stiH263_MODE_B_PAYLOAD_HEADER_SIZE) > stiMAX_VIDEO_RECORD_PACKET_BUFFER)
	#error Video Record Packet Buffer Size is too small!
#endif

const int nMAX_THROW_COUNT = 10;

const int nAUDIO_CLOCK_RATE = 8000;		// Audio clock rate (8 kHz)
const int nTEXT_CLOCK_RATE = 1000;		// Text clock rate (1 kHz)
const int nVIDEO_CLOCK_RATE = 90000;	// Video clock rate (90 kHz)

const int nMAX_VIDEO_PLAYBACK_PACKET_BUFFER = 1500;

const int nMAX_FRAME_BUFFERS = 15;

const int nTASK_VP_BUFFERS = 8;			// # of structures needed for our own internal
										// handling of video

const int nDRIVER_VP_BUFFERS = 0;		// # of structures needed (pass to driver - this
										// number may not be changed unless the number
										// of buffers required by the video driver changes)

const int nMAX_VP_BUFFERS = 			// # of video playback buffers needed for driver
	nDRIVER_VP_BUFFERS + 				// + the number needed for SRV (and out-of-order
	stiOUTOFORDER_VP_BUFFERS +			// packet handling)
	nTASK_VP_BUFFERS;

const int nMIN_VP_BUFFERS = 3;			// # of video playback buffers needed before we will
										// stop dropping packets after dropping packets
										// due to saturation.

const int nDRIVER_VR_BUFFERS = 0; 		// # of structures needed (pass to driver - this
										// number may not be changed unless the number
										// of buffers required by the video driver
										// changes)
const int nMAX_VR_BUFFERS = 1;


#define nMIN_RECORD_AUDIO_FRAME_MS 10
#define nMAX_RECORD_AUDIO_FRAME_MS (IstiAudioInput::InstanceGet ()->MaxPacketDurationGet ())
#define nMIN_PLAYBACK_AUDIO_FRAME_MS 10
#define nMAX_PLAYBACK_AUDIO_FRAME_MS (IstiAudioOutput::InstanceGet ()->MaxPacketDurationGet ())
#define nG711_HEADER_SIZE 12
#define G711_MS_TO_SIZE(x) ((x)*8)
#define G711_SIZE_TO_MS(x) ((x)/8)
#define G722_MS_TO_SIZE(x) ((x)*8)
#define G722_SIZE_TO_MS(x) ((x)/8)

const int nOUTOFORDER_AP_BUFFERS = 4;
const int nUNMUTE_FLUSH_AR_BUFFERS = 4;

// Set the audio record bytes large enough to record at sample sizes up to 
// MAX_AUDIO_FRAME_MS for G.711, This is the size used to get audio data from driver
#define nMAX_AUDIO_RECORD_BYTES (G711_MS_TO_SIZE (nMAX_RECORD_AUDIO_FRAME_MS))

// Set the audio record buffer bytes large enough to hold the
// 2 * nMAX_AUDIO_RECORD_BYTES (SRA XOR data) + RTP header
// This is the size used for AppendSRAData
#define nMAX_AUDIO_RECORD_BUFFER_BYTES ((2 * nMAX_AUDIO_RECORD_BYTES) + nG711_HEADER_SIZE)

// Set the audio playback bytes large enough to hold data at sample sizes up to 
// nMAX_PLAYBACK_AUDIO_FRAME_MS for G.711,
#define nMAX_AUDIO_PLAYBACK_BYTES (G722_MS_TO_SIZE (nMAX_PLAYBACK_AUDIO_FRAME_MS))

// Set the audio playback buffer bytes large enough to hold the
// 2 * MAX_AUDIO_PLAYBACK_BYTES (SRA XOR data) + RTP header
// This is size used to read in audio packet from rtp channel
#define nMAX_AUDIO_PLAYBACK_BUFFER_BYTES ((2 * nMAX_AUDIO_PLAYBACK_BYTES) + nG711_HEADER_SIZE)

// Hold up to 5 * nMAX_AUDIO_PLAYBACK_BYTES (regarding to the SRA algorithm) + RTP header
// This is the size used for ExtractSRAData()
#define nMAX_AUDIO_PLAYBACK_BUFFER_BYTES_SRA ((5 * nMAX_AUDIO_PLAYBACK_BYTES) + nG711_HEADER_SIZE)


#define nT140_HEADER_SIZE (12 + 9) // rtp header is 12, redundant block headers will take up (4 x (n-1)) + 1 bytes,
											 // where n is the number of levels of redundancy.  We are recording at n = 3.

const int nTEXT_FRAMES_IN_PACKET = 1;  // Number of text frames in one packet getting from hardware
const int nOUTOFORDER_TP_BUFFERS = 4;
const int nUNMUTE_FLUSH_TR_BUFFERS = 4;

// Set the text record bytes large enough to record at sample sizes up to 
// MAX_TEXT_FRAME_MS for T.140, This is the size used to get text data from driver
#define nMAX_TEXT_RECORD_BYTES 30 // (T140_MS_TO_SIZE (nMAX_RECORD_TEXT_FRAME_MS))

// Set the text record buffer bytes large enough to hold the
// nMAX_TEXT_RECORD_BYTES (SRA XOR data) + RTP header
// This is the size used for AppendData
#define nMAX_TEXT_RECORD_BUFFER_BYTES ((nMAX_TEXT_RECORD_BYTES) + nT140_HEADER_SIZE)

// Set the text playback bytes large enough to hold data at sample sizes up to 
// nMAX_PLAYBACK_TEXT_FRAME_MS for T140,
#define nMAX_TEXT_PLAYBACK_BYTES 300 // Max of 30 cps averaged over 10 seconds.

// Set the text playback buffer bytes large enough to hold an incoming text.  Theoritically, this shouldn't be
// too large, perhaps (2 * MAX_TEXT_PLAYBACK_BYTES + RTP header).  Since we can't control what the other endpoint
// may send us, however, we will set this to something larger than MTU size on the socket and then the worst thing
// that will happen is we may not display everything that is sent but we won't freeze.
// This is size used to read in text packet from rtp channel
#define nMAX_TEXT_PLAYBACK_BUFFER_BYTES 1500

#define	stiKEEPALIVE_BURST_NUMBER	6

// This is used by both the record and playback tasks
enum EstiPacketResult
{
	ePACKET_OK,
	ePACKET_ERROR,
	ePACKET_CONTINUE
};

const int NACK_BITMASK_SIZE = 16; // Size of the Bitmask portion of a NACK message.
// Maximum packets we can request in a single NACK is NACK_BITMASK_SIZE + 1 for starting packet ID.
constexpr int MAX_NUM_PACKETS_IN_NACK = NACK_BITMASK_SIZE + 1;


class CstiDataTaskCommonP
{
public:

	typedef struct SAudioVideoSyncInfo
	{
		uint32_t unNTP32sec;
		uint32_t un32NTPfrac;
		uint32_t un32Timestamp;
	} SAudioVideoSyncInfo ;

	CstiDataTaskCommonP () = delete;

	CstiDataTaskCommonP (
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t callIndex,
		uint32_t maxRate,
		ECodec codec);

	CstiDataTaskCommonP (const CstiDataTaskCommonP &other) = delete;
	CstiDataTaskCommonP (CstiDataTaskCommonP &&other) = delete;
	CstiDataTaskCommonP &operator= (const CstiDataTaskCommonP &other) = delete;
	CstiDataTaskCommonP &operator= (CstiDataTaskCommonP &&other) = delete;

	virtual ~CstiDataTaskCommonP ();

	stiHResult Initialize ();

	// From to RtpChannel
	ECodec NegotiatedCodecGet () const;
	void NegotiatedCodecSet (
		ECodec codec);

	int FlowControlRateGet () const;
	virtual void FlowControlRateSet (
		int rate);

	int MaxChannelRateGet () const;
	void MaxChannelRateSet (
		int rate);

	EstiSwitch PrivacyModeGet () const;
	void PrivacyModeSet (
		EstiSwitch mode);

	int8_t PayloadTypeGet () const;
	void PayloadTypeSet (
		int8_t payloadTypeId);

	virtual stiHResult Hold (
		EHoldLocation location);

	virtual stiHResult Resume (
		EHoldLocation location);

	stiHResult LocalAddressesGet (
		std::string *rtpAddress,
		int *rtpPort,
		std::string *rtcpAddress,
		int *rtcpPort);

	bool tmmbrNegotiatedGet () const;


protected:

	void StatsLock ();
	void StatsUnlock ();
	
	void ByteCountReset ();

	uint32_t ByteCountCollect ();

	int GetVideoClockRate () const
	{
		return m_nVideoClockRate;

	}

	int GetAudioClockRate () const
	{
		return m_nAudioClockRate;
	}

	int GetTextClockRate () const
	{
		return m_nTextClockRate;
	}

	virtual void ByteCountAdd (
		uint32_t un32AmountToAdd);
	
	void PacketCountAdd ();
	uint32_t PacketCountCollect ();
	void PacketCountReset ();

protected:

	std::recursive_mutex m_StatsLockMutex;  // protects byte count

	uint32_t m_un32ByteCount = 0;  // byte count since last collection
	uint32_t m_packetsSent = 0;

	EstiBool  m_bFirstPacket = estiTRUE;

	// One copy of these members is shared among all instances of CstiDataTaskCommon
	static int m_nVideoClockRate;
	static int m_nAudioClockRate;
	static int m_nTextClockRate;

	std::shared_ptr<vpe::RtpSession> m_session;
	uint32_t m_callIndex = 0;
	uint32_t m_maxRate = stiMAX_RECV_SPEED_DEFAULT;
	ECodec m_negotiatedCodec = estiCODEC_UNKNOWN;
	EstiSwitch m_privacyMode = estiOFF;
	int8_t m_payloadTypeNbr = -1;
	int m_flowControlRate = stiMAX_RECV_SPEED_DEFAULT;

private:
	
	bool m_abCurrentHoldState[3]{false};
};

#endif
