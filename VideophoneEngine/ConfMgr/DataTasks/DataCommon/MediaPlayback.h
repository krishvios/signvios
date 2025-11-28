/*!
 * \file MediaPlayback.h
 * \brief contains MediaPlayback class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiDataTaskCommonP.h"
#include "CstiEventQueue.h"
#include "CstiSyncManager.h"

#include "RvSipMid.h"

#include <atomic>
#include <bitset>

namespace vpe
{

struct PlaybackSettings
{
	int8_t n8KeepAlivePayloadTypeNbr = 0;  // Used with H.460.19
	int16_t n16KeepAliveInterval = 0;  // Used with H.460.19
};

template <typename PlaybackReadType, typename MediaPacketType, typename MediaCodecType, typename PayloadMapType>
class MediaPlayback : public CstiEventQueue, public CstiDataTaskCommonP
{

public:

	using MediaPlaybackType = MediaPlayback<PlaybackReadType, MediaPacketType, MediaCodecType, PayloadMapType>;

	// TODO: will this prevent sub classes from being copyable?  Assuming yes?
	MediaPlayback (const MediaPlayback &) = delete;
	MediaPlayback (MediaPlayback &&) = delete;
	MediaPlayback &operator= (const MediaPlayback &) = delete;
	MediaPlayback &operator= (MediaPlayback &&) = delete;

	// TODO: combine this with MediaRecord?
	enum eMutedReason
	{
		estiMUTED_REASON_HELD = 1,
		estiMUTED_REASON_HOLD = 2,
		estiMUTED_REASON_PRIVACY = 4,
		estiMUTED_REASON_DHV = 8
	};

	MediaPlayback(
		const std::string &taskName,
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t callIndex,
		uint32_t maxRate,
		bool rtpKeepAliveEnable,
		RvSipMidMgrHandle sipMidMgr,
		MediaCodecType initialCodec,
		ECodec defaultInitCodec,
		int remoteSipVersion) :
		CstiEventQueue (taskName),
		CstiDataTaskCommonP (
			session,
			callIndex,
			maxRate,
			estiCODEC_UNKNOWN),
		m_playbackRead(session),
		m_keepAliveTimer (0, this),
		m_currentCodec(initialCodec),
		m_initialNoneCodec(initialCodec),
		m_defaultInitCodec(defaultInitCodec),
		m_outputDeviceReady(false)
	{
		stiLOG_ENTRY_NAME (MediaPlayback);

		if (sipMidMgr)
		{
			// Notify Radvision that we'll be using our event loop's thread to
			// communicate with its SIP APIs (must be called first thing from
			// the event loop thread)
			m_signalConnections.push_back (startedSignal.Connect (
					[this, sipMidMgr]{ RvSipMidAttachThread (sipMidMgr, m_taskName.c_str()); }));

			// Notify Radvision that we're done using the event loop's thread
			// to communicate with their SIP APIs
			m_signalConnections.push_back (shutdownSignal.Connect (
					[sipMidMgr]{ RvSipMidDetachThread (sipMidMgr); }));
		}

		// NOTE: there has been discussion about whether this is necessary or not
		// it was determined it's necessary for the tunneled media case...
		m_signalConnections.push_back (m_keepAliveTimer.timeoutSignal.Connect (
				std::bind(&MediaPlayback::EventKeepAliveTimerTimeout, this)));

		if (rtpKeepAliveEnable)
		{
			m_Settings.n8KeepAlivePayloadTypeNbr = (remoteSipVersion >= webrtcSSV) ? RTP_KEEPALIVE_PAYLOAD_ID : OLD_RTP_KEEPALIVE_PAYLOAD_ID;
			m_Settings.n16KeepAliveInterval = RTP_TIMEOUT_SECONDS;
		}
		
		m_playbackRead.m_remoteSipVersion = remoteSipVersion;
	}

	~MediaPlayback() override = default;

	virtual stiHResult Initialize(const PayloadMapType &payloadMap)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		Uninitialize ();

		FlowControlRateSet (MaxChannelRateGet());
		NegotiatedCodecSet (m_defaultInitCodec);

		// Get instance of synchronization manager
		m_pSyncManager = CstiSyncManager::GetInstance();
		stiTESTCOND (nullptr != m_pSyncManager, stiRESULT_TASK_INIT_FAILED);

		// TODO: call playbackRead.Initialize before DataTaskCommonP::Initialize? (also for video)

		//
		// Call the base class intitialization function
		//
		hResult = CstiDataTaskCommonP::Initialize ();
		stiTESTRESULT ();

		PayloadMapSet (payloadMap);

		// initialize the m_playbackRead subtask
		hResult = m_playbackRead.Initialize ();
		stiTESTRESULT ();

		m_bInitialized = true;

		hResult = Startup ();
		stiTESTRESULT ();

	STI_BAIL:

		return hResult;
	}

	/*!
	 * \brief Final cleanup of the playback task.
	 *
	 * \return none
	 */
	virtual void Uninitialize ()
	{
		stiLOG_ENTRY_NAME (MediaPlayback::Uninitialize);

		//
		// Cleanup the class-specific variables
		//

		m_bInitialized = false;
	}

	/*!
	 * \brief set the payload map
	 *
	 * \retval The codec that is being used for the received media
	 */
	virtual void PayloadMapSet (
		const PayloadMapType &pPayloadMap)
	{
		std::lock_guard<std::recursive_mutex> lk (m_execMutex);

		m_PayloadMap = pPayloadMap;

		m_playbackRead.PayloadMapSet (pPayloadMap);

		// TODO: only video currently does this
		// Reset the current payload so it forces reinitialization of TMMBR
		//m_nCurrentRTPPayload  = -1;
	}

	// DataTaskCommon provides each of the audio/video clockrate functions,
	// but use the derived class to pick the one so they can be used here
	// TODO: better way to do that? mpl?
	virtual int ClockRateGet() const = 0;

	/*!
	 * \brief Start the playback task
	 *
	 * \return estiOK (Success) or estiERROR (Failure)
	 */
	virtual stiHResult Startup ()
	{
		stiLOG_ENTRY_NAME (MediaPlayback::Startup);

		stiHResult hResult = stiRESULT_SUCCESS;

		// NOTE: text was the only one that did this
		if (estiFALSE == m_bInitialized)
		{
			stiASSERTMSG (false, "Startup called before Initialize\n");
			hResult = Initialize (m_PayloadMap);
			stiTESTRESULT ();
		}

		// Initialize the Playback Read Task and give it the clock rate
		// NOTE: audio had StartEventLoop before playbackRead.Startup
		hResult = m_playbackRead.Startup (ClockRateGet ());

		StartEventLoop ();

	STI_BAIL:

		return (hResult);

	}

	/*!
	 * \brief Starts the shutdown process of the Playback task
	 *
	 * \return stiRESULT_SUCCESS if successful
	 *
	 * NOTE: there was slightly different behavior in the
	 * audioplayback task for shutting down
	 */
	virtual stiHResult Shutdown ()
	{
		m_keepAliveTimer.stop ();

		DataChannelClose ();

		StopEventLoop ();

		return stiRESULT_SUCCESS;
	}

	/*!
	 * \brief Sets up for the initialization of a data channel
	 *
	 * This function is called at the time of negotiation of the data
	 * channel.  The Conference Manager will call this function,
	 * passing a structure of information that is needed to prepare
	 * for the sending of data through the playback channel. This
	 * is the second level of startup initialization.
	 *
	 * \return estiOK when successful, otherwise estiERROR
	 */
	virtual stiHResult DataChannelInitialize (bool bStartKeepAlives)
	{
		stiLOG_ENTRY_NAME (MediaPlayback::DataChannelInitialize);

		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		stiHResult hResult = stiRESULT_SUCCESS;

		m_un32InitialRtcpTime = 0;

		DataChannelInitialize ();

		if (bStartKeepAlives)
		{
			KeepAlivesStart ();
		}

		return hResult;
	}

	// TODO: these are pretty different between media types
	virtual stiHResult DataChannelResume () = 0;

	virtual stiHResult DataChannelInitialize ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		hResult = DataChannelResume ();
		stiTESTRESULT ();

		//
		// Initialize the playback read
		//
		hResult = m_playbackRead.DataChannelInitialize (m_un32InitialRtcpTime,
			&m_PayloadMap);
		stiTESTRESULT ();

		//
		// Set a data member indicating the channel is now open.
		//
		m_bDataChannelClosed = estiFALSE;

	STI_BAIL:

		return hResult;
	}

	/*!
	* \brief Close the channel
	*/
	virtual stiHResult Close ()
	{
		stiHResult hResult = DataChannelClose ();
		stiASSERT(!stiIS_ERROR(hResult));

		hResult = Shutdown ();
		stiASSERT(!stiIS_ERROR(hResult));

		Uninitialize ();

		return hResult;
	}

	/*!
	 * \brief Post a message that the channel is closing
	 *
	 * This method posts a message to the Playback task that the
	 * channel is closing.
	 *
	 * \return estiOK when successful, otherwise estiERROR
	 */
	virtual stiHResult DataChannelClose ()
	{
		stiLOG_ENTRY_NAME (MediaPlayback::DataChannelClose);

		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL ((1 & g_stiMediaPlaybackDebug),
			stiTrace ("MediaPlayback::DataChannelClose\n");
		);

		if (!m_bDataChannelClosed)
		{
			DataChannelHold ();

			//
			// Tell the read task that we are closing
			//
			m_playbackRead.DataChannelClose ();

			m_bDataChannelClosed = estiTRUE;
		}

		return hResult;
	}

	/*!
	 * NOTE: this base doesn't do much... could probably be more
	 * consolidation
	 */
	virtual stiHResult DataChannelHold ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		// TODO: not sure if this mutex is necessary, but just to be safe
		std::lock_guard<std::recursive_mutex> lk(m_execMutex);

		//
		// Tell the read task that we are closing
		//
		m_playbackRead.RtpHaltedSet (true);

		m_nCurrentRTPPayload = -1;

		m_bChannelOpen = estiFALSE;

		return hResult;
	}


	/*!
	 * \brief Get the currently active codec being used
	 *
	 * \return The codec that is being used for the received media
	 */
	MediaCodecType CodecGet () const
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		MediaCodecType codec = m_initialNoneCodec;
		typename PayloadMapType::const_iterator i;

		i = m_PayloadMap.find (m_nCurrentRTPPayload);

		if (i != m_PayloadMap.end ())
		{
			codec = i->second.eCodec;
		}

		return codec;
	}

	/*!
	* \brief Handler for when keepAliveTimer fires
	*/
	void EventKeepAliveTimerTimeout ()
	{
		if (!m_session->rtpGet() || !m_bChannelOpen)
		{
			return;
		}

		StatsLock ();
		auto bytesSent = m_bytesRecentlySent;
		m_bytesRecentlySent = 0;
		StatsUnlock ();
		
		// Only send a keep alive if we haven't recently sent data.
		if  (!bytesSent)
		{
			KeepAliveSend ();
		}

		// Restart the timer
		// At the beginning of a call, send keep alive messages more regularly to be sure to get the firewall
		// pinhole open.
		auto timeoutMs = m_Settings.n16KeepAliveInterval * 1000;

		if (m_nKeepAlivesToSendAtStart)
		{
			timeoutMs = 500;  // 1/2-second
			m_nKeepAlivesToSendAtStart--;
		}

		m_keepAliveTimer.timeoutSet (timeoutMs);
		m_keepAliveTimer.restart ();
	}

	stiHResult KeepAliveSend ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		if (m_session->rtpGet() && m_bChannelOpen)
		{
			RvRtpParam stRtpParam;
			RvRtpParamConstruct(&stRtpParam);
			stRtpParam.payload = m_Settings.n8KeepAlivePayloadTypeNbr;
			stiDEBUG_TOOL (2 <= g_stiNATTraversal,
				stiTrace ("<VP::TimerHandle> Calling RvRtpSendKeepAlivePacket with PT = %d\n", m_Settings.n8KeepAlivePayloadTypeNbr);
			);

			RvRtpSessionSendKeepAlive(m_session->rtpGet(), stRtpParam.payload);
		}

		return hResult;
	}

	/*!
	 * \brief : Starts sending keep alive messages towards the remote endpoint
	 *
	 * Before this is called, data coming in will still be processed,
	 * but afterwards we are expecting data, so we will start doing
	 * things like sending RTP keepalives.
	 *
	 * \return stiHResult
	 */
	stiHResult KeepAlivesStart ()
	{
		if (m_Settings.n8KeepAlivePayloadTypeNbr > 0)
		{
			// If the keepAliveInterval is 0, set it to 28 seconds
			if (m_Settings.n16KeepAliveInterval == 0)
			{
				m_Settings.n16KeepAliveInterval = 28;
			}

			m_nKeepAlivesToSendAtStart = stiKEEPALIVE_BURST_NUMBER;

			//
			// Send one keep alive immediately and then set the timer to send another in 100 milliseconds
			//
			KeepAliveSend ();

			m_keepAliveTimer.timeoutSet (100);
			m_keepAliveTimer.restart ();
		}
		else
		{
			m_keepAliveTimer.stop ();
		}

		return stiRESULT_SUCCESS;
	}


	virtual stiHResult SetDeviceCodec (MediaCodecType codec) = 0;

	/*!
	 * \brief Collects the stats since the last collection
	 *
	 * \return Return Value: stiHResult - success or failure
	 *
	 * NOTE: not used by video
	 */
	stiHResult StatsCollect (
		uint32_t *pun32PacketsReceived, 	///< The number of packets received
		uint32_t *pun32PacketsLost,		///< The number of visable packets lost
		uint32_t *pun32ActualPacketsLost,	///< Actual number of packets lost
		uint32_t *pun32ByteCount)			///< The number of bytes.
	{
		stiLOG_ENTRY_NAME (MediaPlayback::StatsCollect);

		stiHResult hResult = stiRESULT_SUCCESS;

		StatsLock ();

		// Yes! Collect the PacketLoss count, and reset it.
		*pun32PacketsLost = m_un32PacketLossCount;
		*pun32ActualPacketsLost = m_un32ActualPacketLossCount;
		m_un32PacketLossCount = 0;
		m_un32ActualPacketLossCount = 0;

		// Was the last packet received less than the first packet received?
		if (m_un32LastPacketReceived < m_un32FirstPacketReceived)
		{
			// Yes! Account for the rollover, and do the math
			*pun32PacketsReceived =
				uint32_t(0xFFFF + m_un32LastPacketReceived) - m_un32FirstPacketReceived;
		} // end if
		else
		{
			// No! Just subtract the first from the last.
			*pun32PacketsReceived =
				m_un32LastPacketReceived - m_un32FirstPacketReceived;
		} // end else

		// Reset the packet counters.
		m_un32FirstPacketReceived = 0,
		m_un32LastPacketReceived = 0,

		*pun32ByteCount = ByteCountCollect ();

		StatsUnlock ();

		return hResult;
	}


	/*!
	 * \brief Resets the PacketLoss counter
	 *
	 * Returns: None
	 */
	virtual void StatsClear ()
	{
		stiLOG_ENTRY_NAME (MediaPlayback::StatsClear);

		StatsLock ();

		// Yes! Reset the PacketLoss count.
		m_un32PacketLossCount = 0;
		m_un32ActualPacketLossCount = 0;

		// Reset the packet counters.
		m_un32FirstPacketReceived = 0;
		m_un32LastPacketReceived = 0;

		ByteCountReset ();

		StatsUnlock ();
	}

	virtual stiHResult DataInfoStructLoad () = 0;

	/*!
	* \brief Accept the offered channel.  Cause it to be setup.
	*/
	virtual stiHResult Answer (bool startKeepAlives)
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		m_startKeepAlives = startKeepAlives;

		m_answered = true;

		auto hResult = DataInfoStructLoad ();
		stiTESTRESULT ();

		DataChannelInitialize (startKeepAlives);

	STI_BAIL:
		return hResult;
	}

	bool AnsweredGet () const
	{
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		return m_answered;
	}

	// TODO: slight differences in media types, but they really should
	// be more similar
	virtual stiHResult Muted (eMutedReason eReason) = 0;
	virtual stiHResult Unmuted (eMutedReason eReason) = 0;

	/*!
	* \brief Place the data task on hold
	*/
	stiHResult Hold (EHoldLocation location) override
	{
		stiLOG_ENTRY_NAME (MediaPlayback::Hold);

		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		auto hResult = CstiDataTaskCommonP::Hold (location);
		stiTESTRESULT ();

		switch (location)
		{
			case estiHOLD_LOCAL:  Muted (estiMUTED_REASON_HOLD); break;
			case estiHOLD_REMOTE: Muted (estiMUTED_REASON_HELD); break;
			case estiHOLD_DHV: Muted (estiMUTED_REASON_DHV); break;
		}

	STI_BAIL:
		return hResult;
	}

	/*!
	* \brief Take the data task out of the hold state
	*/
	stiHResult Resume (EHoldLocation location) override
	{
		stiLOG_ENTRY_NAME (MediaPlayback::Resume);

		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		stiHResult hResult = CstiDataTaskCommonP::Resume (location);
		stiTESTRESULT ();

		switch (location)
		{
			case estiHOLD_LOCAL:  hResult = Unmuted (estiMUTED_REASON_HOLD); break;
			case estiHOLD_REMOTE: hResult = Unmuted (estiMUTED_REASON_HELD); break;
			case estiHOLD_DHV: hResult = Unmuted (estiMUTED_REASON_DHV); break;
		}

	STI_BAIL:
		return hResult;
	}

	/*!
	* \brief Set the radvision transports manually
	*/
	// TODO:  I'd like to see this changed so that SipCall sets the transports
	//        directly in the session, and the session emits signals so we can
	//        respond to them.  That would remove some null checking code
	//        in SipCall, and remove the need for a transportsSet method in each DT
	stiHResult transportsSet (const CstiMediaTransports &transports)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		std::lock_guard<std::recursive_mutex> lk (m_execMutex);
		
		if (m_session->rtpTransportGet () != transports.RtpTransportGet () ||
			m_session->rtcpTransportGet () != transports.RtcpTransportGet ())
		{
			DataChannelClose ();
			
			m_session->transportsSet (transports);
			
			if (m_answered)
			{
				hResult = Answer (m_startKeepAlives);
				stiTESTRESULT ();
			}
		}

	STI_BAIL:
		return hResult;
	}

	struct NackInfo
	{
		NackInfo () {};
		NackInfo (uint32_t createdAtTime)
			:
			createdAtTime (createdAtTime),
			sentAtTime (0),
			retries (0)
		{};

		uint32_t createdAtTime {0};
		uint32_t sentAtTime {0};
		int retries {0};
	};
	
	/*!
	 * \brief Adds missing sequence numbers to list used by NACK
	 */
	virtual void missingSeqNumStore (uint32_t seqNum, const NackInfo& nackInfo)
	{
		m_missingSeqNumbers[seqNum] = nackInfo;
	}
	
	/*!
	 * \brief Creates list of sequence numbers from the missing sequence numbers we have.
	 * We ensure that the sequence numbers we will be sending a NACK for have not recently had a NACK sent
	 * and that we do not send a NACK for data that is old.
	 */
	virtual uint32_t missingPacketsNACK (
		uint32_t lastReceivedSeqNum,
		uint32_t ssrc)
	{
		// Remove all sequence numbers before which playback has already played
		if (lastReceivedSeqNum != 0xFFFFFFFF)
		{
			m_missingSeqNumbers.erase (
				m_missingSeqNumbers.begin (),
				m_missingSeqNumbers.lower_bound (lastReceivedSeqNum));
		}
		
		uint32_t nackRequestsSent = 0;
		auto timeNow = stiOSTickGet ();
		std::list<uint32_t> missingPackets;
#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
		const int maxNackRetries = 1;
#else
		const int maxNackRetries = 2;
#endif
		const int delayBetweenNack = 120; // rtt estimate (ms)

		auto position = m_missingSeqNumbers.begin();
		while (position != m_missingSeqNumbers.end())
		{
			if ((timeNow - position->second.sentAtTime) > delayBetweenNack)
			{
				missingPackets.push_back (position->first);
				position->second.sentAtTime = timeNow;
				position->second.retries += 1;

				if (position->second.retries >= maxNackRetries)
				{
					position = m_missingSeqNumbers.erase (position);
				}
				else
				{
					++position;
				}

				continue;
			}
			++position;
		}
		
		nackRequestsSent = nackSend (ssrc, missingPackets);
		
		return nackRequestsSent;
	}
	
	/*!
	 * \brief Sends NACK message for sequence numbers from the provided list.
	 */
	virtual uint32_t nackSend (uint32_t ssrc, const std::list<uint32_t> &missingPackets)
	{
		uint32_t startSeq = 0;
		bool pidSet = false;
		std::bitset<NACK_BITMASK_SIZE> bitMask;
		uint8_t nackRequestsSent = 0;
		
		for (auto missingSeqNum : missingPackets)
		{
			if (!pidSet)
			{
				startSeq = missingSeqNum;
				pidSet = true;
			}
			else
			{
				if (((missingSeqNum - startSeq) - 1) < NACK_BITMASK_SIZE)
				{
					bitMask.set(missingSeqNum - startSeq - 1);
				}
				else
				{
					// This packet is outside the current NACK range. Send what we
					// have and start a new NACK.
					uint16_t pid = startSeq & 0x0000FFFFUL;
					RvRtcpFbAddNACK (m_session->rtcpGet(), ssrc, pid, static_cast<RvUint16>(bitMask.to_ulong()));
					nackRequestsSent++;
					bitMask.reset ();
					startSeq = missingSeqNum;
				}
			}
		}
		
		if (pidSet)
		{
			uint16_t pid = startSeq & 0x0000FFFFUL;
			RvRtcpFbAddNACK (m_session->rtcpGet(), ssrc, pid, static_cast<RvUint16>(bitMask.to_ulong()));
			nackRequestsSent++;
			
			RvRtcpFbSend (m_session->rtcpGet(), RtcpFbModeImmediate);
		}
		
		return nackRequestsSent;
	}

	
	/*!
	 * \brief Updatest the amount of data we have recently sent.
	 */
	void bytesRecentlySentSet (uint32_t bystesSent)
	{
		m_bytesRecentlySent += bystesSent;
	}

protected: // methods

	/*!
	 * \brief Adds to the PacketLoss counter
	 *
	 * Returns: None
	 */
	void PacketLossCountAdd (int32_t n32AmountToAdd)
	{
		stiLOG_ENTRY_NAME (MediaPlayback::PacketLossCountAdd);

		StatsLock ();

		// Yes! Set the PacketLoss count.
		// Is the amount to add a positive value?
		if (0 <= n32AmountToAdd)
		{
			// Yes.  Add it to both the actual and visable values
			m_un32PacketLossCount += n32AmountToAdd;
			m_un32ActualPacketLossCount += n32AmountToAdd;
		}

		// Is the amount to add a negative value that when added to the packet
		// loss count will not make the count itself negative?

		else if ((uint32_t)(-1 * n32AmountToAdd) <= m_un32PacketLossCount)
		{
			m_un32PacketLossCount += n32AmountToAdd;
		}

		StatsUnlock ();
	}

	/*!
	 * \brief Adds to the PacketsReceived counter
	 *
	 * \brief None
	 */
	void PacketsReceivedCountAdd (uint16_t un16SequenceNumber)
	{
		stiLOG_ENTRY_NAME (MediaPlayback::PacketsReceivedCountAdd);

		StatsLock ();

		// Yes! Is this the first packet we have recieved?
		if (0 == m_un32FirstPacketReceived &&
			0 == m_un32LastPacketReceived)
		{
			// Yes! Store the first packet number
			m_un32FirstPacketReceived = un16SequenceNumber;
		}
		else
		{
			// No! Save the last packet number
			m_un32LastPacketReceived = un16SequenceNumber;
		}

		StatsUnlock ();
	}

	/*!
	 * \brief Retrieves a filled packet from the read task
	 *
	 * \return MediaPacket - pointer to filled packet
	 */
	std::shared_ptr<MediaPacketType> MediaPacketFullGet ()
	{
		stiLOG_ENTRY_NAME (MediaPlayback::MediaPacketFullGet);

		// Get the oldest packet in the full queue
		auto packet = m_playbackRead.MediaPacketFullGet (0);

		return packet;
	}

	/*!\brief Returns duration of the current muted time
	 *
	 */
	std::chrono::milliseconds MutedDurationGet ()
	{
		auto now = std::chrono::steady_clock::now ();

		return std::chrono::duration_cast<std::chrono::milliseconds> (now - m_mutedStartTime);
	}

protected: // data members

	PlaybackSettings m_Settings;
	PayloadMapType m_PayloadMap;

	// playback read task
	// NOTE: the video playback task was allocated as necessary (not sure if that's an issue)
	PlaybackReadType m_playbackRead;

	CstiSyncManager* m_pSyncManager = nullptr;

	bool m_bInitialized = false; // NOTE: this doesn't seem to be used for video

	// For keeping open an RTP port (see H.460.19)
	vpe::EventTimer m_keepAliveTimer;

	MediaCodecType m_currentCodec;					// Current codec used for playback
	MediaCodecType m_initialNoneCodec;				// initial none codec value
	ECodec m_defaultInitCodec;						// default codec to be initialized to

	EstiBool m_bFirstPacket = estiTRUE;				// Indicator of first packet in call
	EstiBool m_bChannelOpen = estiFALSE;			// Indicator of channel being open

	//
	// Variables used for generating byte count, data rate, and loss statistics
	//
	uint32_t m_un32FirstPacketReceived = 0;
	uint32_t m_un32LastPacketReceived = 0;
	uint32_t m_un32PacketLossCount = 0;
	uint32_t m_un32ActualPacketLossCount = 0;

	int m_nThrowAwayCount = 0;

	// NOTE: this wasn't used by audio, and video and text had different init values... ?
	//uint32_t m_un32ExpectedPacket = 0; // text initial value
	uint32_t m_un32ExpectedPacket = 0x00010000;	// The sequence number we expect to receive
	// NOTE: audio playback has m_un16ExpectedPacket that could probably be consolidated with this

	// Will be true when output device is ready for more data
	std::atomic<bool> m_outputDeviceReady;

	int m_nCurrentRTPPayload = -1;
	int m_nMuted = 0;

	int m_nKeepAlivesToSendAtStart = stiKEEPALIVE_BURST_NUMBER;	// Keep Alives are used with H.460 to create a pinhole in the firewall.

	uint32_t m_un32InitialRtcpTime = 0;

	EstiBool m_bDataChannelClosed = estiTRUE;

	// Playback common
	bool m_startKeepAlives = false;
	bool m_answered = false;

	// TODO: video and audio have packet queues, but not text
	// could probably use some consolidation
	// along with associated routines to empty the queues

	std::chrono::milliseconds m_timeMutedByRemote = std::chrono::milliseconds::zero ();
	std::chrono::steady_clock::time_point m_mutedStartTime = {};

	std::map<uint32_t,NackInfo> m_missingSeqNumbers;
	
	bool m_channelResumed = false;

private:
	CstiSignalConnection::Collection m_signalConnections;
	
	uint32_t m_bytesRecentlySent = 0;
};

} // namespace
