//
//  RTCPFlowControl.cpp
//  ntouchMac
//
//  Created by Dennis Muhlestein on 11/12/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2024 Sorenson Communications, Inc. -- All rights reserved
//

#include "RTCPFlowControl.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "stiOS.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include "stiRemoteLogEvent.h"

//#define MAX_RATE_AT_CONSTANT_LOSS
#define RTCP_LOSS_FRACTION_MULTIPLIER	256.0 // See RFC3550, 6.4.1, fraction_lost
#define FRACTION_OF_BW	0.8 // When a bandwidth is detected, rate is multiplied by this value
#define SECONDS_TO_INCREASE_BW_LIMIT	60 // When a bandwidth is set, how often to raise the bandwidth limit..multiple of 5
#define RTCP_INTERVAL_SEC	5 // Per RFC3550
#define RTCP_PACKET_SKIP_COUNT 2 // Do not set to value less than 2

const char szDATA_RATE_PERSISTENT_FILE[] = "data_rates.dat";

std::recursive_mutex RTCPFlowcontrol::m_LockMutex{};

// The following table is nVP specific without partial frames.
// Data rates are kbps. Loss rates are fraction of packets lost, i.e. 1% loss is 0.01.
// Each sequential entry in this table must meet the following requirements:
//		- the data rate must be lower than the data rate in the previous entry
//		- each loss rate must be greater than or equal to the loss rate in the previous entry
SstiMaximumLossesAtRate g_astMaximumLossTable [] =
{
	{1536000, 0.0025, 0.0100},
	{1024000, 0.0025, 0.0175},
	{ 512000, 0.0100, 0.0250},
	{ 256000, 0.0200, 0.0350}
};
const int g_nLossTableArraySize = sizeof (g_astMaximumLossTable) / sizeof (g_astMaximumLossTable[0]);

const uint32_t RTCPFlowcontrol::minBitrate;

RTCPFlowcontrol::RTCPFlowcontrol(
	uint32_t un32CurrentBitrate,
	const Settings &settings,
	const std::string &callId)
:
	m_un32BitRateLast (un32CurrentBitrate),
	m_CallId(callId),
	m_settings (settings)
{
	m_un32PreviousFlow = un32CurrentBitrate; //For 4.0 algorithm
	
#if APPLICATION != APP_MEDIASERVER 
	StoredNetworkInfoGet();
#endif

	for (int i = 0; i < PASSED_BW_ARRAY_SIZE; i++)
	{
		m_PassedBandwidthArray[i] = 0;
	}
#ifdef MAX_RATE_AT_CONSTANT_LOSS
	double dLastCallAverageLoss = ThisNetworkAverageLossGet();
#endif
	for (int i = 0; i < FRACTION_LOST_ARRAY_SIZE; i++)
	{
#ifdef MAX_RATE_AT_CONSTANT_LOSS
		m_dFractionLostArray[i] = dLastCallAverageLoss;
#else
		m_dFractionLostArray[i] = 0;
#endif
	}
}


RTCPFlowcontrol::~RTCPFlowcontrol()
{
	NetworkInfoStore(0);
	m_NetworkInfo.clear ();
}


////////////////////////////////////////////////////////////////////////////////
//; RTCPFlowcontrol::FlowControlRecommend
//
// Description: Based on RTCP Receive Reports, calculates available bandwidth.
//
// Abstract:
//	Receives data from the remote endpoint via RTCP RR and calculates an estimate
//	of the available bandwidth based on packets lost and the current bandwidth estimate.
//
// Returns:
//	an estimate of the bandwidth available to the video encoder
//
uint32_t RTCPFlowcontrol::FlowControlRecommend ( SstiRTCPInfo *pRTCPInfo, uint32_t un32AverageRTPPacketSize,
												 uint32_t un32CurrentBitrate, uint32_t un32MaxBitrate, uint32_t remb )
{
	stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
		stiTrace ("Entering RTCPFlowControl::FlowControlRecommend\n");
	);

	uint32_t un32NewBitrate = 0;
	EstiBool bPacketsLostEquivalent = estiFALSE;
	double dFractionLostAverage = 0.0;
	uint32_t un32AvePassedBandwidth = 0;

	uint32_t un32PacketsLost = (pRTCPInfo->n32CumulativePacketsLost <= m_n32CumulativePacketsLostLast) ? (uint32_t)0 :
								(uint32_t)(pRTCPInfo->n32CumulativePacketsLost - m_n32CumulativePacketsLostLast); // Due to duplicate packets (see RFC3550)
	
	double dPacketsLostFraction = (m_un32SequenceNumberLast == 0) ? (double)(pRTCPInfo->un32PacketsLost) / RTCP_LOSS_FRACTION_MULTIPLIER :
									 (double)(un32PacketsLost) / (double)(pRTCPInfo->un32SequenceNumber-m_un32SequenceNumberLast);

	// Calculate NTP send time of this RTCP packet
	double ntpSeconds {0.0};
	{
		uint16_t lsrSeconds = pRTCPInfo->un32LastSR >> 16;

		// Handle potential overflow of LSR
		if (lsrSeconds < m_lsrSecondsLast && (uint16_t)(lsrSeconds - m_lsrSecondsLast) < (uint16_t)0xfff)
		{
			m_lsrSecondsUpper16 += 0x10000;
		}

		ntpSeconds = (double)(m_lsrSecondsUpper16 + lsrSeconds);
		
		// Handle potential out of order of LSR
		if ((uint16_t)(lsrSeconds - m_lsrSecondsLast) > (uint16_t)0xefff)
		{
			ntpSeconds -= (double)0x10000;
		}

		ntpSeconds += (double)(pRTCPInfo->un32LastSR << 16) / (double)0x100000000;
		stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug > 1,
			stiTrace ("\n\tNTP Seconds (LSR): %f\n", ntpSeconds);
		);

		// Calculate delay since LSR
		double delay = (double)pRTCPInfo->un32DelaySinceLastSR / 65535.0;
		stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug > 1,
			stiTrace ("\tDelay since LSR NTP: %f\n", delay);
		);

		// NTP send time of RTCP packet
		ntpSeconds += delay;
		stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug > 1,
			stiTrace ("\tNTP of RTCP Packet: %f\n\n", ntpSeconds);
		);
	}

	uint32_t un32RTCPDataRate {un32CurrentBitrate};
	if (m_un32SequenceNumberLast > 0 && ntpSeconds > 0.0 && m_ntpSecondsLast > 0.0)
	{
		un32RTCPDataRate = (uint32_t)((double)(((pRTCPInfo->un32SequenceNumber - m_un32SequenceNumberLast) - un32PacketsLost) * un32AverageRTPPacketSize)
											  / (ntpSeconds - m_ntpSecondsLast) * 8.0); //Convert to bps
	}

	if (m_un64InitialRTCPTimeStamp == 0 && ntpSeconds > 0.0)
	{
		m_un64InitialRTCPTimeStamp = (uint64_t)(ntpSeconds);
	}

	// When outgoing video is muted, two RTCP packets are skipped; the packet received immediately following
	// the video being muted and the first packet after video resumes. This is done because the calculations
	// above are dependent on RTCP continuity and, sometimes, these packets report errantly high packet loss.
	if (m_un32RTCPSkipCount)
	{
		// Decrement counter
		m_un32RTCPSkipCount--;
		
		// Save pertinent data from latest RTCP for continuity
		m_un32SequenceNumberLast = pRTCPInfo->un32SequenceNumber;
		m_n32CumulativePacketsLostLast = pRTCPInfo->n32CumulativePacketsLost;
		m_ntpSecondsLast = ntpSeconds;
		
		// Set the new bit rate to the current bit rate
		un32NewBitrate = un32CurrentBitrate;
	}
	else // Calculate new data rate
	{
		//Calculate average passed bandwidth over last three RTCP RR
		for (int i = PASSED_BW_ARRAY_SIZE - 1; i > 0; i--)
		{
			m_PassedBandwidthArray[i] = m_PassedBandwidthArray[i-1];
			un32AvePassedBandwidth += m_PassedBandwidthArray[i];
		}
		m_PassedBandwidthArray[0] = un32RTCPDataRate;
		un32AvePassedBandwidth += m_PassedBandwidthArray[0];
		un32AvePassedBandwidth /= PASSED_BW_ARRAY_SIZE;

		//Calculate average fraction of lost packets over last twenty RTCP RR
		for (int i = FRACTION_LOST_ARRAY_SIZE - 1; i > 0; i--)
		{
			m_dFractionLostArray[i] = m_dFractionLostArray[i-1];
			dFractionLostAverage += m_dFractionLostArray[i];
		}
		m_dFractionLostArray[0] = std::min (dPacketsLostFraction, 0.05); //Limit fraction lost to 5% for averaging purposes...helps to correctly identify constant loss
		dFractionLostAverage += m_dFractionLostArray[0];
		dFractionLostAverage /= FRACTION_LOST_ARRAY_SIZE;

		if (remb)
		{
			stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
				stiTrace ("\tSetting rate based on REMB\n");
				stiTrace ("\tCurrent Rate: %u, REMB: %u, Fraction Lost: %1.3f\n", un32CurrentBitrate, remb, dPacketsLostFraction);
			);
			if (dPacketsLostFraction < 0.02)
			{
				// Increase rate
				un32NewBitrate = 1.05 * (un32CurrentBitrate + 1000.0);
				stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
					stiTrace ("\tIncreasing rate to %u kbps\n", un32NewBitrate);
				);
			}
			else if (dPacketsLostFraction < 0.10)
			{
				// Maintain current rate
				un32NewBitrate = un32CurrentBitrate;
				stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
					stiTrace ("\tMaintaining rate at %u kbps\n", un32NewBitrate);
				);
			}
			else // Losses >= 10%
			{
				// Slow rate
				un32NewBitrate = un32CurrentBitrate * (1.0 - (0.5 * dPacketsLostFraction));
				stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
					stiTrace ("\tDecreasing rate to %u kbps\n", un32NewBitrate);
				);
			}

			if (un32NewBitrate > remb)
			{
				un32NewBitrate = remb;
				stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
					stiTrace ("\tLimiting rate at %u kbps, REMB: %u\n", un32NewBitrate, remb);
				);
			}

			// Checked after REMB because REMB should not reduce the rate lower than minBitrate
			if (un32NewBitrate < minBitrate)
			{
				un32NewBitrate = minBitrate;
				stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
					stiTrace ("\tSetting rate to minimum bitrate: %u kbps\n", un32NewBitrate);
				);
			}

			// Check MaxBitrate last. It supercedes minBitrate because it is set either as a 
			// max rate via VDM or is a max rate as negotiated with the remote endpoint.
			if (un32NewBitrate > un32MaxBitrate)
			{
				un32NewBitrate = un32MaxBitrate;
				stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
					stiTrace ("\tLimiting rate at %u kbps, Max Rate: %u\n", un32NewBitrate, un32MaxBitrate);
				);
			}
		}
		//If AutoSpeed is enabled
		else if (m_settings.autoSpeedSetting != estiAUTO_SPEED_MODE_LEGACY)
		{
			double dMaxAcceptableLoss = 0.0;
			double dMaxOneTimeLoss = 0.0;
			MaxLossesGet (un32CurrentBitrate, &dMaxAcceptableLoss, &dMaxOneTimeLoss);

			if (dPacketsLostFraction > dMaxAcceptableLoss)
			{
				m_un32LostPacketsCounter++;
				bPacketsLostEquivalent = estiTRUE;
				
#ifdef MAX_RATE_AT_CONSTANT_LOSS
				//Constant rate loss. Consider as packets not lost.
				if (dPacketsLostFraction < dFractionLostAverage * 2.0 && dPacketsLostFraction < dFractionLostAverage + 2.0)
				{
					m_un32LostPacketsCounter--;
					bPacketsLostEquivalent = estiFALSE;

					if (m_dPacketsLostFractionLast < dFractionLostAverage * 2.0 && dPacketsLostFraction < dFractionLostAverage + 2.0)
					{
						//Only reset counter if we have had two constant loss reports in a row
						m_un32LostPacketsCounter = 0;
					}
				}
#endif
			}
			else
			{
				bPacketsLostEquivalent = estiFALSE;
				if (m_dPacketsLostFractionLast < dMaxAcceptableLoss) //Only reset counter if we have had two acceptable loss reports in a row
				{
					m_un32LostPacketsCounter = 0;
				}
			}

			if (bPacketsLostEquivalent)
			{
				bool bMaintainCurrentRate = true;
				// If this is not our first loss or if the loss is greater than max one time loss, look at setting bandwidth and reducing rate
				if (m_un32LostPacketsCounter > 1 || dPacketsLostFraction > dMaxOneTimeLoss)
				{
					// If sufficient stability has been established (no losses for 3 RTCP RR), set bandwidth limit
					if (m_un32NoLossCounter >= 3 && un32AvePassedBandwidth < (1.15 * un32CurrentBitrate) && m_un32BandwidthLimit > minBitrate
						// Don't set a bandwidth limit if this is constant loss. Rate will still be reduced below.
						// What margin over lost average is ok? 1.25 is a WAG.
						&& dPacketsLostFraction > dFractionLostAverage * 1.25
						)
					{
						//Set bandwidth to FRACTION_OF_BANDWIDTH of the previous bitrate
						m_un32BandwidthLimit = (uint32_t)((double)un32CurrentBitrate * FRACTION_OF_BW / (1.0 + m_dRateIncreaseMultiplier));
						m_dRateIncreaseMultiplier = std::max (m_dRateIncreaseMultiplier * 0.75, 0.02); //Reduce multiplier but limit to minimum of 0.02
					
						if (m_un32BandwidthLimit < minBitrate)
						{
							m_un32BandwidthLimit = minBitrate;
						}

						stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
							stiTrace ("\n\tSetting bandwidth limit: %u\n", m_un32BandwidthLimit);
						);
					
						// Set m_dBWIncreaseMultiplier for later increases in bandwidth when there are no losses
						// If TIME_TO_CHALLENGE_MIN is the time interval to challenge an estimated bandwidth, the bandwidth multipler is calculated:
						// m_dBandwidthIncreaseMultiplier = (1 / FRACTION_OF_BW) ^ (1 / (TIME_TO_CHALLENGE_MIN * 60 / RTCP_INTERVAL_SEC))
						// For example, if TIME_TO_CHALLENGE_MIN = 10: m_dBandwidthIncreaseMultiplier = (1/.8) ^ (1/10) = 1.25 ^ .1 = 1.0226
						switch (m_un32BandwidthMultiplierChangeCount)
						{
							case 0:
							{
								// Ten minutes
								m_dBandwidthIncreaseMultiplier = 1.0226;
								break;
							}
							case 1:
							{
								// Thirty minutes
								m_dBandwidthIncreaseMultiplier = 1.0075;
								break;
							}
							default:
							{
								// Sixty minutes
								m_dBandwidthIncreaseMultiplier = 1.0037;
								break;
							}
						}
						m_un32BandwidthMultiplierChangeCount++;
						
						// If enabled, log setting bandwidth limit
						if (m_settings.remoteFlowControlLogging)
						{
							stiRemoteLogEventSend("\n\tEventType=FlowControl\n\tReason=BandwidthSet\n\tBandwidthLimit=%u\n\tFractionLost=%f"
												  "\n\tFractionLostAverage=%f\n\tNoLossCounter=%u\n\tCallId=$s\n\tSecondsInCall=%u\n",
												  m_un32BandwidthLimit, dPacketsLostFraction, dFractionLostAverage, m_un32NoLossCounter,
												  m_CallId.c_str(), (uint64_t)(ntpSeconds) - m_un64InitialRTCPTimeStamp);
						}
					
						//Stats
						m_un32BandwidthLimitChangeCounter++;
						m_un32BandwidthLimitSum += m_un32BandwidthLimit;
						if (m_un32BandwidthLimit > m_un32MaxBandwidthLimit)
						{
							m_un32MaxBandwidthLimit = m_un32BandwidthLimit;
						}
						if (m_un32BandwidthLimit < m_un32MinBandwidthLimit)
						{
							m_un32MinBandwidthLimit = m_un32BandwidthLimit;
						}
					}
					m_un32NoLossCounter = 0; //Reset no loss consecutive counter
				
					//If current rate is greater than minBitrate, reduce rate
					if (un32CurrentBitrate > minBitrate)
					{
						bMaintainCurrentRate = false;

						double dBitrateReduction;
						if (dPacketsLostFraction < m_dMinBitrateReduction)
						{
							dBitrateReduction = m_dMinBitrateReduction;
						}
						else //Otherwise lower bit rate proportional to fraction of lost packets
						{
							dBitrateReduction = dPacketsLostFraction;
						}
						// Reduce bitrate...not more than the bandwidth limit and not less than minBitrate
						un32NewBitrate = std::max (std::min ((uint32_t)((1.0-dBitrateReduction)*(double)un32CurrentBitrate), m_un32BandwidthLimit), (uint32_t)minBitrate);
						// Ensure new rate is not greater than un32MaxBitrate (supercedes minBitrate)
						un32NewBitrate = std::min (un32NewBitrate, un32MaxBitrate);

						stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
							stiTrace ("\n\tReducing bitrate: %u\n", un32NewBitrate);
						);
					
						// If enabled, log decreasing data rate
						if (m_settings.remoteFlowControlLogging)
						{
							stiRemoteLogEventSend("\n\tEventType=FlowControl\n\tReason=RateDecrease\n\tPreviousRate=%u\n\tNewRate=%u\n\tMaxRate=%u\n\tFractionLost=%f"
												  "\n\tFractionLostAverage=%f\n\tLostPacketsCounter=%u\n\tCallId=%s\n\tSecondsInCall=%u\n",
												  un32CurrentBitrate, un32NewBitrate, un32MaxBitrate, dPacketsLostFraction, dFractionLostAverage, m_un32LostPacketsCounter,
												  m_CallId.c_str(), (uint64_t)(ntpSeconds) - m_un64InitialRTCPTimeStamp);
						}
					}
				}

				if (bMaintainCurrentRate)
				{
					// Maintain current rate unless un32MaxBitrate or minBitrate require otherwise
					// If current bit rate is less than minBitrate, set un32NewBitrate to minBitrate
					un32NewBitrate = std::max (un32CurrentBitrate, (uint32_t)minBitrate);
					
					//If result above is greater than un32MaxBitrate, set un32NewBitrate to un32MaxBitrate (supercedes minBitrate)
					un32NewBitrate = std::min (un32NewBitrate, un32MaxBitrate);
					
					// If enabled, log changing rate
					if (m_settings.remoteFlowControlLogging && un32NewBitrate != un32CurrentBitrate)
					{
						stiRemoteLogEventSend("\n\tEventType=FlowControl\n\tReason=RateSet1\n\tPreviousRate=%u\n\tNewRate=%u\n\tMaxRate=%u\n\tFractionLost=%f"
											  "\n\tFractionLostAverage=%f\n\tLostPacketsCounter=%u\n\tCallId=%s\n\tSecondsInCall=%u\n",
											  un32CurrentBitrate, un32NewBitrate, un32MaxBitrate, dPacketsLostFraction, dFractionLostAverage, m_un32LostPacketsCounter,
											  m_CallId.c_str(), (uint64_t)(ntpSeconds) - m_un64InitialRTCPTimeStamp);
					}
					
				}
			}
			else
			{
				m_un32NoLossCounter++;
				m_dMinBitrateReduction = 0.1; // Once we have had acceptable loss, set minimum rate decrease to 10%
				
				if (un32CurrentBitrate > m_un32MaxTargetBitrateSentWithAcceptableLoss && un32CurrentBitrate <= un32MaxBitrate) //CurrentBitrate may be higher than MaxBitrate on the first time through.
				{	// Set maximum bitrate...successfully attained at zero packet loss
					m_un32MaxTargetBitrateSentWithAcceptableLoss = un32CurrentBitrate;
				}
				
				//For extended periods of no loss, allow bandwidth to increase. This helps to offset bandwidth reductions for random losses.
				//If we have had 0 losses for some time (60 seconds) and we have set a bandwidth limit, increase bandwidth
				if (m_un32NoLossCounter % (SECONDS_TO_INCREASE_BW_LIMIT / RTCP_INTERVAL_SEC) == 0 &&
					un32CurrentBitrate == m_un32BandwidthLimit && m_un32BandwidthMultiplierChangeCount)
				{
					m_un32BandwidthLimit = (double)m_un32BandwidthLimit * m_dBandwidthIncreaseMultiplier;

					stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
						stiTrace ("\n\tIncreasing bandwidth limit: %u\n", m_un32BandwidthLimit);
					);
				
					//Stats
					m_un32BandwidthLimitChangeCounter++;
					m_un32BandwidthLimitSum += m_un32BandwidthLimit;
					if (m_un32BandwidthLimit > m_un32MaxBandwidthLimit)
					{
						m_un32MaxBandwidthLimit = m_un32BandwidthLimit;
					}
				}

				//Only increase rate if there have been two reports in a row without loss and we are using more than 85% of allotted bandwidth
				double dRateIncreasePercentage = .85;
				//Endpoints using variable bitrate should use the define to set a lower value
#ifdef MINIMUM_PERCENTAGE_OF_ACTUAL_VS_TARGET_BITRATE_TO_FLOW_UP
				dRateIncreasePercentage = MINIMUM_PERCENTAGE_OF_ACTUAL_VS_TARGET_BITRATE_TO_FLOW_UP;
#endif
				if (!m_bPacketsLostEquivalentLast && un32AvePassedBandwidth > (uint32_t)(dRateIncreasePercentage * un32CurrentBitrate) && !m_bRateIncreasedLastUpdate &&
					un32CurrentBitrate < un32MaxBitrate && un32CurrentBitrate < m_un32BandwidthLimit)
				{
					m_bRateIncreasedLastUpdate = estiTRUE;

					// Calculate new rate but limit by max acceptable rate at the current loss percentage
					un32NewBitrate = std::min ((uint32_t)((double)un32CurrentBitrate * (1.0 + m_dRateIncreaseMultiplier)),
											   MaxRateAtLossGet(dPacketsLostFraction, un32MaxBitrate));
					
					// Limit bandwidth by bandwidth limit or max negotiated bitrate
					if (un32NewBitrate > un32MaxBitrate || un32NewBitrate > m_un32BandwidthLimit)
					{
						un32NewBitrate = std::min (m_un32BandwidthLimit, un32MaxBitrate);
					}

					stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
						stiTrace ("\n\tIncreasing bitrate: %u\n", un32NewBitrate);
					);

					// If enabled, log increasing data rate
					if (m_settings.remoteFlowControlLogging)
					{
						stiRemoteLogEventSend("\n\tEventType=FlowControl\n\tReason=RateIncrease\n\tPreviousRate=%u\n\tNewRate=%u\n\tMaxRate=%u\n\tBandwidthLimit=%u"
											  "\n\tFractionLost=%f\n\tFractionLostAverage=%f\n\tNoLossCounter=%u\n\tCallId=%s\n\tSecondsInCall=%u\n",
											  un32CurrentBitrate, un32NewBitrate, un32MaxBitrate, m_un32BandwidthLimit, dPacketsLostFraction, dFractionLostAverage,
											  m_un32NoLossCounter, m_CallId.c_str(), (uint64_t)(ntpSeconds) - m_un64InitialRTCPTimeStamp);
					}
				}
				else
				{
					m_bRateIncreasedLastUpdate = estiFALSE;
					
					// Handle external changes to un32MaxBitrate and un32CurrentBitrate
					// If current bit rate is less than minBitrate, set un32NewBitrate to minBitrate
					un32NewBitrate = std::max (un32CurrentBitrate, (uint32_t)minBitrate);
					
					//If result above is greater than un32MaxBitrate or m_un32BandwidthLimit, set un32NewBitrate to lesser of these (supercedes minBitrate)
					if (un32NewBitrate > un32MaxBitrate || un32NewBitrate > m_un32BandwidthLimit)
					{
						un32NewBitrate = std::min (m_un32BandwidthLimit, un32MaxBitrate);
					}
					
					// If enabled, log changing rate
					if (m_settings.remoteFlowControlLogging && un32NewBitrate != un32CurrentBitrate)
					{
						stiRemoteLogEventSend("\n\tEventType=FlowControl\n\tReason=RateSet2\n\tPreviousRate=%u\n\tNewRate=%u\n\tMaxRate=%u\n\tFractionLost=%f"
											  "\n\tFractionLostAverage=%f\n\tLostPacketsCounter=%u\n\tCallId=%s\n\tSecondsInCall=%u\n",
											  un32CurrentBitrate, un32NewBitrate, un32MaxBitrate, dPacketsLostFraction, dFractionLostAverage, m_un32LostPacketsCounter,
											  m_CallId.c_str(), (uint64_t)(ntpSeconds) - m_un64InitialRTCPTimeStamp);
					}
					
					
				}
			}
			
		}
		else //Using 4.0 bandwidth algorithm
		{
			un32NewBitrate = un32CurrentBitrate;
			int32_t FlowDifference = m_un32PreviousFlow - un32CurrentBitrate;
			if(FlowDifference > 10000 || FlowDifference < -10000)
			{
				m_un32PreviousFlow = un32CurrentBitrate;
			}
			auto lostPercent = pRTCPInfo->un32PacketsLost / RTCP_LOSS_FRACTION_MULTIPLIER;
			
			if (lostPercent > 0.02F)
			{
				m_un32PreviousFlow = std::max<uint32_t>(static_cast<uint32_t>(un32CurrentBitrate * (1.0 - lostPercent)), minBitrate);
				stiTrace("Lowering bitrate to %d\n", m_un32PreviousFlow);
				un32NewBitrate = m_un32PreviousFlow;
			}
			else
			{
				if (un32CurrentBitrate > m_un32MaxTargetBitrateSentWithAcceptableLoss && un32CurrentBitrate <= un32MaxBitrate)
				{	// Set maximum bitrate sent
					m_un32MaxTargetBitrateSentWithAcceptableLoss = un32CurrentBitrate;
				}
				if (un32CurrentBitrate < un32MaxBitrate)
				{
			
					m_un32PreviousFlow += static_cast<uint32_t>((0.02 - lostPercent) * (un32MaxBitrate - un32CurrentBitrate));
					int32_t FlowChange = m_un32PreviousFlow - un32CurrentBitrate;
					if(FlowChange > 10000)
					{
						un32NewBitrate = m_un32PreviousFlow;
					}
				}
			}
			// Make sure the new bitrate does not exceed the max bitrate.
			if (un32NewBitrate > un32MaxBitrate)
			{
				un32NewBitrate = m_un32PreviousFlow = un32MaxBitrate;
			}
		}
		
		m_un32BitrateSentCounter++;
		m_un32BitrateSentSum += (un32NewBitrate + 500) / 1000; // Add 500 to round to nearest 1000. Divide by 1000 to keep sum in bounds (kbps)?
		if (un32NewBitrate > m_un32MaxTargetBitrateSent)
		{
			m_un32MaxTargetBitrateSent = un32NewBitrate;
		}
		if (un32NewBitrate < m_un32MinTargetBitrateSent)
		{
			m_un32MinTargetBitrateSent = un32NewBitrate;
		}
		
		stiDEBUG_TOOL (g_stiVideoRecordFlowControlDebug,
			stiTrace ("\n\tBandwidth, Max: %d   Min: %d   Ave: %d\n", m_un32MaxBandwidthLimit, m_un32MinBandwidthLimit, (m_un32BandwidthLimitChangeCounter > 0) ? m_un32BandwidthLimitSum/m_un32BandwidthLimitChangeCounter : 0);
			stiTrace ("\tBandwidth changes = %d\n", m_un32BandwidthLimitChangeCounter);
			stiTrace ("\tAverage packet loss = %f\n", dFractionLostAverage);
			stiTrace ("\tConsecutive lost packets counter = %u\n", m_un32LostPacketsCounter);
			stiTrace ("\tNo loss counter = %u\n", m_un32NoLossCounter);
			stiTrace ("\tRTCP SR time interval = %f\n", ntpSeconds - m_ntpSecondsLast);
			stiTrace ("\tAverage data rate received at other endpoint over RTCP interval = %u bps\n", un32RTCPDataRate);
			stiTrace ("\tJitter = %u\n", pRTCPInfo->un32Jitter);
			stiTrace ("\tun32PacketsLost = %u\n", un32PacketsLost);
			stiTrace ("\tCalculated fraction lost = %f\n", dPacketsLostFraction);
			stiTrace ("\tNew Bitrate = %u, Max Bitrate = %u, Bandwidth Limit = %u\n\n", un32NewBitrate, un32MaxBitrate, m_un32BandwidthLimit);
		);
		
		m_un32BitRateLast = un32CurrentBitrate;
		m_un32SequenceNumberLast = pRTCPInfo->un32SequenceNumber;
		m_n32CumulativePacketsLostLast = pRTCPInfo->n32CumulativePacketsLost;
		m_ntpSecondsLast = ntpSeconds;
		m_dPacketsLostFractionLast = dPacketsLostFraction;
		m_bPacketsLostEquivalentLast = bPacketsLostEquivalent;
		m_dFractionLostAverageLast = dFractionLostAverage;
	}
	
	return un32NewBitrate;
}

////////////////////////////////////////////////////////////////////////////////
//; RTCPFlowcontrol::StoredNetworkInfoGet
//
// Description: Retrieves ip address, data rate and loss information and stores it in a map
//
// Abstract:
//	This function is called in the constructor of RTCPFlowcontrol to create
//	a map of ip addresses and their associated last known good data rates and average
//  losses. Data rate information is used as the starting bandwidth of subsequent calls.
//
// Returns:
//	stiRESULT_SUCCESS if file was successfully opened, otherwise an error
//
stiHResult RTCPFlowcontrol::StoredNetworkInfoGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);
	
	FILE *fp = nullptr;
	char szPublicIP [48];
	uint32_t un32LastRate;
	double dLastAverageLoss;
	int nRet;
	std::string oFile;
	stiOSDynamicDataFolderGet (&oFile);
	oFile += szDATA_RATE_PERSISTENT_FILE;
	
	fp = fopen (oFile.c_str (), "r");
	stiTESTCOND_NOLOG (fp, stiRESULT_UNABLE_TO_OPEN_FILE);

	do
	{
		nRet = fscanf (fp, "%47s\t%u\t%lf\n", szPublicIP, &un32LastRate, &dLastAverageLoss);
		if (nRet == 3)
		{
			// We obtained a record.  Allocate the structures and store the read data
			if (IPAddressValidate(szPublicIP))
			{
				m_NetworkInfo[(std::string)szPublicIP].un32LastRate = un32LastRate;
				m_NetworkInfo[(std::string)szPublicIP].dLastAverageLoss = dLastAverageLoss;
			}
		}
		else
		{
			break;
		}
	} while (nRet != EOF);
	
STI_BAIL:
	
	if (fp)
	{
		fclose (fp);
		fp = nullptr;
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; RTCPFlowcontrol::NetworkInfoStore
//
// Description: Writes data file to maintain ip address, data rate and loss information.
//
// Abstract:
//	This function is called in the destructor of RTCPFlowcontrol to save the
//	<ip address, (data rate, loss)> map to be used as the starting speed of subsequent calls.
//
// Returns:
//	stiRESULT_SUCCESS if file was successfully opened, otherwise an error
//
stiHResult RTCPFlowcontrol::NetworkInfoStore (const uint32_t &lastRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);
	
	std::map <std::string, SstiNetworkInfo>::iterator it;
	FILE *fp = nullptr;
	std::string oFile;
	stiOSDynamicDataFolderGet (&oFile);
	oFile += szDATA_RATE_PERSISTENT_FILE;
	
	fp = fopen (oFile.c_str (), "w");
	stiTESTCOND (fp, stiRESULT_UNABLE_TO_OPEN_FILE);
	
	if (lastRate)
	{
		m_NetworkInfo[m_settings.publicIPv4].un32LastRate = lastRate;
	}
	else if (m_un32BandwidthLimitChangeCounter > 0)
	{
		//Reset stored data rate if bandwidth limit was reached. Otherwise leave at what it was.
		//...Don't allow other endpoint to effect stored data rate if low bit rate is because they required it.
		m_NetworkInfo[m_settings.publicIPv4].un32LastRate = m_un32BandwidthLimitSum/m_un32BandwidthLimitChangeCounter;
	}
	else if (m_un32BitRateLast > m_NetworkInfo[m_settings.publicIPv4].un32LastRate)
	{
		//If the last data rate is faster than what is in the map, save it.
		m_NetworkInfo[m_settings.publicIPv4].un32LastRate = m_un32BitRateLast / 32000 * 32000;
	}
	
	m_NetworkInfo[m_settings.publicIPv4].dLastAverageLoss = m_dFractionLostAverageLast;
	
	for (it = m_NetworkInfo.begin (); it != m_NetworkInfo.end (); it++)
	{
		if (IPv4AddressValidate (it->first.c_str ()))
		{
			fprintf (fp, "%s\t%u\t%lf\n", it->first.c_str (), it->second.un32LastRate, it->second.dLastAverageLoss);
		}
	}
	
STI_BAIL:
	
	if (fp)
	{
		fclose (fp);
		fp = nullptr;
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; RTCPFlowcontrol::LastRateSet
//
// Description: Sets the last known rate into the network store.
//
void RTCPFlowcontrol::LastRateSet (const uint32_t &lastRate)
{
	NetworkInfoStore (lastRate);
}

////////////////////////////////////////////////////////////////////////////////
//; RTCPFlowcontrol::initialDataRateGet
//
// Description: Returns the initial rate based on the current network.
//
// Abstract:
//	This function is called to retrieve the initial rate using the last known data rate on the calling
//	endpoint's network. It iterates through the <ipaddress, data rate> map and returns
//	the data rate if an IP address match is found. Otherwise, it returns a minimum
//	value (256 kbps). The data rate is used as the starting point of subsequent calls.
//
// Returns:
//	initial data rate if ip address is in the map, otherwise
//	returns a minimum value
//
uint32_t RTCPFlowcontrol::initialDataRateGet ()
{
	auto lastRate = 512000U;

#if APPLICATION != APP_MEDIASERVER 
	auto it = m_NetworkInfo.find (m_settings.publicIPv4);
	
	if (it != m_NetworkInfo.end())
	{
		//Public IP is already in map, return stored data rate
		lastRate = it->second.un32LastRate;
	}
#endif
	//Default startup data rate. What should this be?
	return std::max(lastRate, m_settings.autoSpeedMinStartSpeed);
}

////////////////////////////////////////////////////////////////////////////////
//; RTCPFlowcontrol::ThisNetworkAverageLossGet
//
// Description: Returns average loss from last call on the calling network.
//
// Abstract:
//	This function is called to retrieve the average loss from the last call on the calling
//	endpoint's network. It iterates through the <ipaddress, (data rate, average loss)> map and returns
//	the average loss if an IP address match is found. Otherwise, it returns 0.
//  The average loss is assumed to be that at the start of the call.
//
// Returns:
//	average loss from last call on the network if ip address is in the map, otherwise
//	returns 0
//
double RTCPFlowcontrol::ThisNetworkAverageLossGet ()
{
#if APPLICATION != APP_MEDIASERVER 
	std::map <std::string, SstiNetworkInfo>::iterator it;
	
	it = m_NetworkInfo.find (m_settings.publicIPv4);
	
	if (it != m_NetworkInfo.end())
	{
		//Public IP is already in map, return stored data rate
		return it->second.dLastAverageLoss;
	}
#endif
	//Default startup data rate. What should this be?
	return 0.0;
}

void RTCPFlowcontrol::VideoRecordMutedNotify ()
{
	// Setting these counters to zero resets FlowControlRecommend in effect while
	// maintaining the history of packet loss in this call.
	m_un32LostPacketsCounter = 0;
	m_un32NoLossCounter = 0;
	if (m_un32RTCPSkipCount == 0)
	{
		// If video record has been muted, skip the last RTCP packet after being muted and
		// the first after resuming video. This must be done only when the counter is zero
		// because this function is called repeatedly by a 30 second timer when on hold.
		m_un32RTCPSkipCount = RTCP_PACKET_SKIP_COUNT;
	}
}

uint32_t	RTCPFlowcontrol::MinSendRateGet() const
{
	return m_un32MinTargetBitrateSent;
}

uint32_t	RTCPFlowcontrol::MaxSendRateWithAcceptableLossGet() const
{
	return m_un32MaxTargetBitrateSentWithAcceptableLoss;
}

uint32_t	RTCPFlowcontrol::MaxSendRateGet() const
{
	return m_un32MaxTargetBitrateSent;
}

uint32_t	RTCPFlowcontrol::AveSendRateGet() const
{
	return (!m_un32BitrateSentCounter) ? 0 : m_un32BitrateSentSum / m_un32BitrateSentCounter;
}

void RTCPFlowcontrol::MaxLossesGet (uint32_t un32CurrentBitrate, double *p_dMaxAcceptableLoss, double *p_dMaxOneTimeLoss)
{
	bool bComplete = false;
	
	// Iterate through table looking for entries to interpolate between.
	int i = 0;
	for (; i < g_nLossTableArraySize; i++)
	{
		if (un32CurrentBitrate >= g_astMaximumLossTable[i].un32DataRate)
		{
			if (i == 0)
			{
				// Given rate is greater than fastest data rate in the table.
				// Set max losses equal to first table entry.
				*p_dMaxAcceptableLoss = g_astMaximumLossTable[i].dMaxAcceptableLoss;
				*p_dMaxOneTimeLoss = g_astMaximumLossTable[i].dMaxOneTimeLoss;
			}
			else
			{
				// Interpolate
				double dMultiplier = (double)(un32CurrentBitrate - g_astMaximumLossTable[i].un32DataRate) /
				(double)(g_astMaximumLossTable[i-1].un32DataRate - g_astMaximumLossTable[i].un32DataRate);
				
				*p_dMaxAcceptableLoss = dMultiplier * (g_astMaximumLossTable[i-1].dMaxAcceptableLoss - g_astMaximumLossTable[i].dMaxAcceptableLoss) +
				g_astMaximumLossTable[i].dMaxAcceptableLoss;
				
				*p_dMaxOneTimeLoss = dMultiplier * (g_astMaximumLossTable[i-1].dMaxOneTimeLoss - g_astMaximumLossTable[i].dMaxOneTimeLoss) +
				g_astMaximumLossTable[i].dMaxOneTimeLoss;
			}
			
			// Interpolation is complete. Break out of loop.
			bComplete = true;
			break;
		}
	}
	
	if (!bComplete)
	{
		// Given rate is less than slowest data rate in the table.
		// Set max losses equal to last table entry.
		*p_dMaxAcceptableLoss = g_astMaximumLossTable[i-1].dMaxAcceptableLoss;
		*p_dMaxOneTimeLoss = g_astMaximumLossTable[i-1].dMaxOneTimeLoss;
	}
}

uint32_t RTCPFlowcontrol::MaxRateAtLossGet (double dFractionLost, uint32_t un32MaxBitrate)
{
	uint32_t un32MaxRateAtLoss = minBitrate;
	
	for (int i = 0; i < g_nLossTableArraySize; i++)
	{
		if (dFractionLost <= g_astMaximumLossTable[i].dMaxAcceptableLoss)
		{
			if (i == 0)
			{
				un32MaxRateAtLoss = un32MaxBitrate;
			}
			else
			{
				un32MaxRateAtLoss =  (uint32_t)((dFractionLost - g_astMaximumLossTable[i].dMaxAcceptableLoss) /
												 (g_astMaximumLossTable[i-1].dMaxAcceptableLoss - g_astMaximumLossTable[i].dMaxAcceptableLoss) *
												 (double)(g_astMaximumLossTable[i-1].un32DataRate - g_astMaximumLossTable[i].un32DataRate))
									 + g_astMaximumLossTable[i].un32DataRate;
			}
			break;
		}
	}
	
	return un32MaxRateAtLoss;
}
