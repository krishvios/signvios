/*!
 * \file MediaRecord.h
 * \brief contains MediaRecord class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiDataTaskCommonP.h"
#include "CstiEventQueue.h"
#include "CstiRtpHeaderLength.h"
#include "stiTrace.h"
#include "CstiPacket.h"
#include "stiTools.h"
#include <array>
#include <bitset>

namespace vpe
{

template <typename MediaCodecType, typename RecordSettingsType>
class MediaRecord : public CstiDataTaskCommonP, public CstiEventQueue
{

public:

	using MediaRecordType = MediaRecord<MediaCodecType, RecordSettingsType>;

	enum eMuteReason
	{
		estiMUTE_REASON_HELD = 1,
		estiMUTE_REASON_HOLD = 2,
		estiMUTE_REASON_PRIVACY = 4,
		estiMUTE_REASON_DHV = 8
	};

	enum EChannelObjectCallbackMsg
	{
		estiMSG_PRIVACY_CHANGED = estiMSG_NEXT
	};
	
	struct RtxPacket {
		std::unique_ptr<CstiPacket> rtxPacket;
		uint32_t lastTimeSent = 0;
	};

	MediaRecord () = delete;

	MediaRecord(
		const std::string &taskName,
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t callIndex,
		uint32_t maxRate,
		ECodec codec,
		MediaCodecType codecInit,
		uint32_t bufferSize,
		uint16_t packetSize) :
		CstiDataTaskCommonP (
			session,
			callIndex,
			maxRate,
			codec),
		CstiEventQueue (taskName),
		m_currentCodec(codecInit),
		m_initialCodec(codecInit),
		m_bufferSize (bufferSize),
	    m_packetSize (packetSize)
	{
		// Set the initialize value of the timestamp for this rtp session
		// TODO: the initial value of the timestamp should be random
		m_un32RTPInitTime = stiOSTickGet ();
	}

	MediaRecord (const MediaRecord &) = delete;
	MediaRecord (MediaRecord &&) = delete;
	MediaRecord &operator= (const MediaRecord &) = delete;
	MediaRecord &operator= (MediaRecord &&) = delete;

	~MediaRecord() override = default;

	/*!
	* \brief Initialize and start the data task
	*/
	virtual stiHResult Initialize ()
	{
		stiLOG_ENTRY_NAME (MediaRecord::Initialize);

		FlowControlRateSet (MaxChannelRateGet());

		auto hResult = CstiDataTaskCommonP::Initialize ();
		stiTESTRESULT ();

		Startup ();

	STI_BAIL:
		return hResult;
	}

	/*!
	* \brief Starts the event loop
	*/
	virtual void Startup ()
	{
		StartEventLoop ();
	}

	/*!
	* \brief Shutdown the task synchronously
	*/
	virtual void Shutdown ()
	{
		stiLOG_ENTRY_NAME (Shutdown);

		stiDEBUG_TOOL ( g_stiMediaRecordDebug,
			stiTrace("<MediaRecord::Shutdown>\n");
		);

		DataChannelClose ();

		// TODO: only audio was doing this...?
		// Don't send anything else to the driver.
		m_bChannelOpen = estiFALSE;

		StopEventLoop ();
	}

	/*!
	* \brief Initializes the data channel
	*/
	virtual stiHResult DataChannelInitialize ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		//
		// Set a global indicating the channel is now open.
		//
		m_bChannelOpen = estiTRUE;

		return hResult;
	}

	/*!
	 * \brief Post a message that the channel is closing.
	 *
	 * This method posts a message to the Record task that the channel is
	 * closing.
	 *
	 * \return estiOK when successfull, otherwise estiERROR
	 */
	virtual void DataChannelClose ()
	{
		stiLOG_ENTRY_NAME (MediaRecord::DataChannelClose);

		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		if (m_bChannelOpen)
		{
			// set the current codec type to be NONE
			m_currentCodec = m_initialCodec;

			// Indicate that we have stopped sending data
			m_bSendingMedia = estiFALSE;

			m_nFlushPackets = 0;

			m_bChannelOpen = estiFALSE;
		}
	}

	virtual void EventDataStart () = 0;

	/*!
	* \brief Causes the task to start sending data
	*/
	stiHResult DataStart ()
	{
		stiLOG_ENTRY_NAME (MediaRecord::DataStart);

		PostEvent (
			std::bind(&MediaRecordType::EventDataStart, this));

		return stiRESULT_SUCCESS;
	}


	/*!
	 * \brief Retrieves the privacy state of the data task.
	*/
	virtual bool PrivacyGet () const
	{
		bool bResult = false;
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		if ((m_nMuted & estiMUTE_REASON_PRIVACY) > 0)
		{
			bResult = true;
		}

		return bResult;
	}

	virtual bool payloadIdChanged(const SstiMediaPayload &payload, int8_t secondaryPayloadId) const
	{
		return m_Settings.payloadTypeNbr != payload.n8PayloadTypeNbr;
	}

	/*!
	 * \brief store relevant stuff from offer
	 *
	 * NOTE: secondaryPayloadId is only used for text (red)
	 */
	virtual stiHResult DataInfoStructLoad (const SstiSdpStream &sdpStream, const SstiMediaPayload &offer, int8_t secondaryPayloadId = 0) = 0;

	/*!
	* \brief Open an outgoing channel.  Cause it to be setup.
	*/
	virtual void Open (
		const SstiSdpStream &sdpStream,
		const SstiMediaPayload &payload,
		int8_t redPayloadTypeNbr = 0)
	{
		auto hResult = stiRESULT_SUCCESS;
		bool addressChanged = false;

		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		stiTESTCONDMSG (m_session->isOpen(), stiRESULT_ERROR, "rtp session is not open");

		// TODO:  This should be done in RtpSession
		// NOTE: this was only being done for video, and looks like currently only for ntouch-pc.
		// TODO: ok to do for all media sockets for endpoints that have this defined?
#ifdef stiSOCK_SEND_BUFF_SIZE
		// Set the send buffer size to avoid buffer overflow
		RvRtpSetTransmitBufferSize (m_session->rtpGet(), stiSOCK_SEND_BUFF_SIZE);
#endif

		m_session->remoteAddressSet(
					&sdpStream.RtpAddress,
					&sdpStream.RtcpAddress,
					&addressChanged);

		// If we did not detect an address change from the offer compare it to what we
		// have saved as our current remote address. Fixes Bug #26900.
		if (!addressChanged)
		{
			addressChanged = m_session->remoteAddressCompare (m_currentRemoteRtpAddr, m_currentRemoteRtcpAddr);
		}

		if (addressChanged || payloadIdChanged(payload, redPayloadTypeNbr))
		{
			m_currentRemoteRtpAddr = sdpStream.RtpAddress;
			m_currentRemoteRtcpAddr = sdpStream.RtcpAddress;

			// Close the data channel since it's going to be re-opened
			// This is the mechanism to re-initialize input to handle codec changes
			DataChannelClose();

			// Set up accroding to negotiated settings from offer
			hResult = DataInfoStructLoad (sdpStream, payload, redPayloadTypeNbr);
			stiTESTRESULT ();

			DataChannelInitialize ();
			DataStart ();
		}

	STI_BAIL:
		return;
	}

	/*!
	* \brief Close the channel
	*/
	void Close ()
	{
		return DataChannelClose ();
	}

	/*!
	* \brief Place the task on hold
	*/
	stiHResult Hold (
		EHoldLocation location) override
	{
		auto hResult = stiRESULT_SUCCESS;
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		switch (location)
		{
			case estiHOLD_LOCAL:  hResult = Mute (estiMUTE_REASON_HOLD); break;
			case estiHOLD_REMOTE: hResult = Mute (estiMUTE_REASON_HELD); break;
			case estiHOLD_DHV: hResult = Mute (estiMUTE_REASON_DHV); break;
		}

		return hResult;
	}


	/*!
	* \brief Take the task out of hold, to resume media flow
	*/
	stiHResult Resume (
		EHoldLocation location) override
	{
		auto hResult = stiRESULT_SUCCESS;
		std::lock_guard<std::recursive_mutex> lock (m_execMutex);

		switch (location)
		{
			case estiHOLD_LOCAL:  hResult = Unmute (estiMUTE_REASON_HOLD); break;
			case estiHOLD_REMOTE: hResult = Unmute (estiMUTE_REASON_HELD); break;
			case estiHOLD_DHV: hResult = Unmute (estiMUTE_REASON_DHV); break;
		}

		return hResult;
	}

	stiHResult StatsCollect (
		uint32_t *pun32ByteCount,
		uint32_t *packetCount)
	{
		stiLOG_ENTRY_NAME (MediaRecord::StatsCollect);

		stiHResult hResult = stiRESULT_SUCCESS;

		*pun32ByteCount = ByteCountCollect ();
		*packetCount = PacketCountCollect ();

		return hResult;
	}

	virtual void StatsClear ()
	{
		stiLOG_ENTRY_NAME (MediaRecord::StatsClear);

		ByteCountReset ();
	}

	/*!
	 * \brief Inform the data task to hold
	 *
	 * Inform the data task to resume a specific call which has been put on hold
	 *
	 * \return estiOK if successful, estiERROR if not.
	 */
	virtual stiHResult Mute (
		eMuteReason eReason)
	{
		stiLOG_ENTRY_NAME (MediaRecord::Mute);
		stiHResult hResult = stiRESULT_SUCCESS;

	// 	stiTrace ("Muting TR for reason %d\n", eReason);

		Lock ();

	//	m_nRemainderSize = 0;

		switch (eReason)
		{
			case estiMUTE_REASON_PRIVACY:

				if ((m_nMuted & estiMUTE_REASON_PRIVACY) == 0)
				{
					if (m_nMuted == 0)
					{
						StatsClear ();
					}

					PrivacyModeSet (estiON);
					privacyModeChangedSignal.Emit (true);

					m_nMuted |= estiMUTE_REASON_PRIVACY;

					m_bSendingMedia = estiFALSE;
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

					m_bSendingMedia = estiFALSE;
				}

				break;

			case estiMUTE_REASON_HOLD:
			case estiMUTE_REASON_DHV:

				if ((m_nMuted & eReason) == 0)
				{
					if (m_nMuted == 0)
					{
						StatsClear ();
					}

					DataChannelClose ();

					m_nMuted |= eReason;
				}

				break;
		}

		Unlock ();

		return hResult;
	}

	/*!
	 * \brief Inform the data task to resume a specific call
	 *
	 * Inform the data task to resume a specific call which has been put on hold
	 *
	 * \return estiOK if successful, estiERROR if not.
	 */
	virtual stiHResult Unmute (
		eMuteReason eReason)
	{
		stiLOG_ENTRY_NAME (MediaRecord::Unmute);
		stiHResult hResult = stiRESULT_SUCCESS;

		//	stiTrace ("Unmuting MR for reason %d\n", eReason);

		Lock ();

		switch (eReason)
		{
			case estiMUTE_REASON_PRIVACY:

				if ((m_nMuted & estiMUTE_REASON_PRIVACY) != 0)
				{
					m_nMuted &= ~estiMUTE_REASON_PRIVACY;
					PrivacyModeSet (estiOFF);
					privacyModeChangedSignal.Emit (false);
				}

				break;

			case estiMUTE_REASON_HELD:

				if ((m_nMuted & estiMUTE_REASON_HELD) != 0)
				{
					m_nMuted &= ~estiMUTE_REASON_HELD;
				}

				break;

			case estiMUTE_REASON_HOLD:
			case estiMUTE_REASON_DHV:

				if ((m_nMuted & eReason) != 0)
				{
					m_nMuted &= ~eReason;


						//
						// We need to know if the capture went into privacy during the time we
						// were not getting notifications from it.
						//
						if (PrivacyGet())
						{
							m_nMuted |= estiMUTE_REASON_PRIVACY;
						}
						else
						{
							m_nMuted &= ~estiMUTE_REASON_PRIVACY;
						}

					if ((m_nMuted & estiMUTE_REASON_DHV) == 0 &&
						(m_nMuted & estiMUTE_REASON_HOLD) == 0)
					{
						DataChannelInitialize ();
					}
				}

				break;
		}

		if (m_nMuted == 0)
		{
			m_bSendingMedia = estiTRUE;
		}

		Unlock ();

		return hResult;
	}
	
	virtual void remoteSipVersionSet (int remoteSipVersion)
	{
		m_remoteSipVersion = remoteSipVersion;
	}
	
public:

	CstiSignal<bool> privacyModeChangedSignal;

protected:

	EstiBool m_bChannelOpen = estiFALSE;				// Indicator of channel being open

	RecordSettingsType m_Settings;

	EstiBool m_bSendingMedia = estiFALSE;				// Indicator of sending a packet to socket

	int m_nFlushPackets = 0;					// the number of frames which will be flushed out from driver

	uint32_t m_un32RTPInitTime = 0;			// record timestamp for a RTP packet

	CstiRtpHeaderLength m_RTPHeaderOffset;		// record sBytes for a RTP pacekt

	int m_nTimestampIncrement = 0;				// record timestamp increment

	MediaCodecType m_currentCodec;	// Current codec used for record
	MediaCodecType m_initialCodec;	// initial codec value (TODO: make template parameter?)

	RvAddress m_currentRemoteRtpAddr = {0};
	RvAddress m_currentRemoteRtcpAddr = {0};

	int m_nMuted = 0;
	
	uint32_t m_bufferSize = 0;
	uint16_t m_packetSize = 0;
	
	uint32_t m_packetsSent = 0;
	
	int m_remoteSipVersion{};

};


} // namespace
