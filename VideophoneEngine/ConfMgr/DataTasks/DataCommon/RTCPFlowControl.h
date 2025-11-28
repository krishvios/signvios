//
//  CstiRTCPInfo.h
//  ntouchMac
//
//  Created by Dennis Muhlestein on 11/9/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2024 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef CstiRTCPFlowControl_h
#define CstiRTCPFlowControl_h

#include "stiDefs.h"
#include "stiSVX.h"
#include <string>
#include <map>
#include <mutex>

#define PASSED_BW_ARRAY_SIZE	3
#define FRACTION_LOST_ARRAY_SIZE	20
#ifndef AUTOSPEED_MIN_BIT_RATE
	#define AUTOSPEED_MIN_BIT_RATE 512000
#endif

/**
 * Simple struct for holding RTCP info used in flow control
 **/
struct SstiRTCPInfo 
{
	uint32_t un32PacketsLost; // Fraction of packets lost << 8 per RFC 3550
	int32_t  n32CumulativePacketsLost;
	uint32_t un32SequenceNumber;
	uint32_t un32Jitter;
	uint32_t un32LastSR;
	uint32_t un32DelaySinceLastSR;
	uint32_t un32RTTms; // Round trip time in milliseconds
};

struct SstiNetworkInfo
{
	uint32_t	un32LastRate;
	double		dLastAverageLoss;
};

struct SstiMaximumLossesAtRate
{
	uint32_t un32DataRate;
	double dMaxAcceptableLoss; // The maximum continuous loss acceptable at the associated data rate
	double dMaxOneTimeLoss; // The maximum loss allowed for one consecutive RTCP report at the associated data rate
};

class RTCPFlowcontrol
{
public:
	struct Settings
	{
		EstiAutoSpeedMode autoSpeedSetting{stiAUTO_SPEED_MODE_DEFAULT};
		uint32_t autoSpeedMinStartSpeed{stiAUTO_SPEED_MIN_START_SPEED_DEFAULT};
		bool remoteFlowControlLogging{false};
		std::string publicIPv4;
	};

	RTCPFlowcontrol (
		uint32_t un32CurrentBitrate,
		const Settings &settings,
		const std::string &callId);

	~RTCPFlowcontrol ();

	uint32_t FlowControlRecommend ( SstiRTCPInfo *pRTCPInfo, uint32_t un32AverageRTPPacketSize,
									uint32_t un32CurrentBitrate, uint32_t un32MaxBitrate, uint32_t remb );
	stiHResult	StoredNetworkInfoGet ();
	uint32_t	initialDataRateGet ();
	double		ThisNetworkAverageLossGet ();
	void		VideoRecordMutedNotify ();
	uint32_t	MinSendRateGet() const;
	uint32_t	MaxSendRateWithAcceptableLossGet() const;
	uint32_t	MaxSendRateGet() const;
	uint32_t	AveSendRateGet() const;
	void		LastRateSet (const uint32_t &lastRate);

	EstiAutoSpeedMode autoSpeedModeGet ()
	{
		return m_settings.autoSpeedSetting;
	}

private:

	uint32_t	m_un32PreviousFlow; //For 4.0 algorithm
	stiHResult	NetworkInfoStore (const uint32_t &lastRate);
	void		MaxLossesGet (uint32_t un32CurrentBitrate, double *p_dMaxAcceptableLoss, double *p_dMaxOneTimeLoss);
	uint32_t	MaxRateAtLossGet (double dFractionLost, uint32_t un32MaxBitrate);

	static std::recursive_mutex m_LockMutex;
	std::map<std::string, SstiNetworkInfo> m_NetworkInfo;

	uint32_t	m_un32BitRateLast;
	uint32_t	m_un32SequenceNumberLast {0};
	int32_t	m_n32CumulativePacketsLostLast {0};
	double		m_dPacketsLostFractionLast {0.0};
	double		m_ntpSecondsLast {0.0};
	uint16_t	m_lsrSecondsLast {0};
	uint32_t	m_lsrSecondsUpper16 {0x10000};
	double		m_dMinBitrateReduction {0.2};
	uint32_t	m_un32NoLossCounter {0};
	uint32_t	m_un32LostPacketsCounter {0};
	uint32_t	m_PassedBandwidthArray[PASSED_BW_ARRAY_SIZE] {};
	double		m_dFractionLostArray[FRACTION_LOST_ARRAY_SIZE] {};
	double		m_dFractionLostAverageLast {0.0};
	EstiBool	m_bRateIncreasedLastUpdate {estiFALSE};
	double		m_dRateIncreaseMultiplier {0.08};
	uint32_t	m_un32RTCPSkipCount {0};
	uint32_t	m_un32BandwidthLimit {20000000};
	uint32_t	m_un32MinBandwidthLimit {20000000};
	uint32_t	m_un32MaxBandwidthLimit {0};
	uint32_t	m_un32BandwidthLimitSum {0};
	uint32_t	m_un32BandwidthLimitChangeCounter {0};
	uint32_t	m_un32MinTargetBitrateSent {20000000};
	uint32_t	m_un32MaxTargetBitrateSentWithAcceptableLoss {0};
	uint32_t	m_un32MaxTargetBitrateSent {0};
	uint32_t	m_un32BitrateSentSum {0};
	uint32_t	m_un32BitrateSentCounter {0};
	EstiBool	m_bPacketsLostEquivalentLast {estiFALSE};
	std::string	m_CallId;
	uint64_t	m_un64InitialRTCPTimeStamp {0};
	uint32_t	m_un32BandwidthMultiplierChangeCount {0};
	double		m_dBandwidthIncreaseMultiplier {1.0226};
	static const uint32_t minBitrate {AUTOSPEED_MIN_BIT_RATE};

	Settings m_settings;
};


#endif
