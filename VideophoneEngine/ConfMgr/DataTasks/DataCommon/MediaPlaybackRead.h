/*!
 * \file MediaPlaybackRead.h
 * \brief contains MediaPlaybackRead class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "rtp.h"
#include "rtcp.h"

#include "stiTrace.h"

namespace vpe
{

struct MediaPlaybackReadStats
{
	uint32_t packetQueueEmptyErrors = 0;
	uint32_t packetsRead = 0;
	uint32_t keepAlivePackets = 0;
	uint32_t unknownPayloadTypeErrors = 0;
	uint32_t payloadHeaderErrors = 0;
	uint32_t packetsDiscardedMuted = 0;
	uint32_t rtxPacketsReceived = 0;
};


/*!
 * \brief base class for playback read common functions
 */
template <typename MediaPacketType, typename PayloadMapType>
class MediaPlaybackRead : public CstiEventQueue
{
public:

	// typedef for convenience
	using PlaybackReadType = vpe::MediaPlaybackRead<MediaPacketType, PayloadMapType>;

	MediaPlaybackRead () = delete;

	MediaPlaybackRead(const std::string &taskName, const std::shared_ptr<RtpSession> &session) :
		CstiEventQueue (taskName),
		m_rtpSession(session)
	{
		m_rtpSession->rtpEventHandlerSet (PlaybackReadType::RTPOnReadEventCB, this);
	}

	MediaPlaybackRead (const MediaPlaybackRead &) = delete;
	MediaPlaybackRead (MediaPlaybackRead &&) = delete;
	MediaPlaybackRead &operator= (const MediaPlaybackRead &) = delete;
	MediaPlaybackRead &operator= (MediaPlaybackRead &&) = delete;

	~MediaPlaybackRead() override
	{
		if (m_rtpSession->rtpGet())
		{
			RvRtpSetEventHandler (m_rtpSession->rtpGet (), nullptr, nullptr); // Clear any pending handlers
			RvRtpResume (m_rtpSession->rtpGet ()); // Cancel out of any blocking read's
		}
	}

	void StatsCollect (MediaPlaybackReadStats *pStats)
	{
		*pStats = m_Stats;

		StatsClear ();
	}

	void StatsClear ()
	{
		m_Stats = {};
	}

	/*!
	 * \brief Causes the rtp packets to be thrown away when set to true
	 *
	 * \param bHalted If true the packets are thrown away otherwise they are processed normally.
	 */
	stiHResult RtpHaltedSet (
		bool bHalted)
	{
		Lock ();

		m_bRtpHalted = bHalted;

		Unlock ();

		return stiRESULT_SUCCESS;
	}

	/*!
	 * \brief Sets up for the initialization of a data channel.
	 *
	 * This function is called at the time of negotiation of the data
	 * channel.  The Conference Manager will call this function,
	 * passing a structure of information that is needed to prepare
	 * for the sending of data through the Video playback
	 * channel. This is the second level of startup initialization.
	 *
	 * \return estiOK when successful, otherwise estiERROR
	 */
	virtual stiHResult DataChannelInitialize (
		uint32_t un32InitialRtcpTime,
		const PayloadMapType *pPayloadMap)
	{
		stiLOG_ENTRY_NAME (MediaPlaybackRead::DataChannelInitialize);

		stiHResult hResult = stiRESULT_SUCCESS;

		Lock ();

		if (m_dataChannelClosed)
		{
			if (pPayloadMap)
			{
				m_PayloadMap = *pPayloadMap;
			}
			else
			{
				m_PayloadMap.clear ();
			}

			// TODO: don't do rtcp report handling because video does it differently
			// RvRtcpSetRTCPRecvEventHandler
			// TODO: combine handling of that as well?
#if 0
			if (nullptr != m_rtpSession->rtcpGet())
			{
				RvRtcpSetRTCPRecvEventHandler(m_rtpSession->rtcpGet(), RTCPReportProcess, this);
			}
#endif

			if (m_rtpSession->rtpGet() && m_rtpSession->rtpTransportGet() == nullptr && !m_rtpSession->tunneledGet())
			{
				// Monitor this socket for data
				int socketFd = RvRtpSessionGetSocket (m_rtpSession->rtpGet());
				FileDescriptorAttach (socketFd, std::bind(&MediaPlaybackRead::EventRtpSocketDataAvailable, this));
			}

			// Get a baseline timestamp for use in calls to RvRtcpRTPPacketRecv()
			m_un32InitialRTCPTime = un32InitialRtcpTime;

			// Clear the packet queues returning all packets to the empty pool.
			m_oRecvdQueue.clear ();
			m_oFullQueue.clear ();

			m_bRtpHalted = false;
			m_dataChannelClosed = false;
			m_packetDiscarding = false;
		}

		Unlock ();

		return hResult;
	}

	/*!
	 * \brief base function to close channels
	 */
	virtual stiHResult DataChannelClose ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		Lock ();

		if (!m_dataChannelClosed)
		{
			m_bRtpHalted = true;

			if (m_rtpSession->rtpTransportGet() || m_rtpSession->tunneledGet())
			{
				if (m_rtpSession->rtpGet())
				{
					RvRtpSetEventHandler (m_rtpSession->rtpGet(), nullptr, nullptr);
				}
			}
			else
			{
				if (m_rtpSession->rtpGet() && !m_rtpSession->rtpTransportGet() && !m_rtpSession->tunneledGet())
				{
					// Stop monitoring this socket for data
					int socketFd = RvRtpSessionGetSocket (m_rtpSession->rtpGet());
					FileDescriptorDetach (socketFd);
				}
			}

			if (m_rtpSession->rtcpGet())
			{
				RvRtcpSetRTCPRecvEventHandler (m_rtpSession->rtcpGet(), nullptr, nullptr);
			}

			m_dataChannelClosed = true;
		}

		Unlock ();

		return hResult;
	}

	// NOTE: this isn't referenced in this file, but it's a reminder
	// that at some point it would be nice to consolidate the
	// initialization functions
	virtual stiHResult Initialize () = 0;

	/*!
	 * \brief Start the playback read task.
	 *
	 * \return estiOK (Success) or estiERROR (Failure)
	 */
	stiHResult Startup (int mediaClockRate)
	{
		stiLOG_ENTRY_NAME (MediaPlaybackRead::Startup);

		// Set the media clock rate from the event passed in.
		m_mediaClockRate = mediaClockRate;

		return StartEventLoop() ?
					stiRESULT_SUCCESS :
					stiRESULT_ERROR;
	}

	/*!
	* \brief Shuts the task down cleanly
	* \return stiHResult
	*/
	virtual void Shutdown ()
	{
		stiLOG_ENTRY_NAME (MediaPlaybackRead::Shutdown);

		stiDEBUG_TOOL ( g_stiPlaybackReadDebug,
			// TODO: put media type string here?
			stiTrace ("MediaPlaybackRead::Shutdown\n");
		);

		DataChannelClose ();

		StopEventLoop ();
	}

	/*!
	 * \brief Retrieves the numbers of packets in Full queue
	 *
	 * \return number of packets in Full queue
	 */
	int NumPacketAvailable ()
	{
		stiLOG_ENTRY_NAME (NumPacketAvailable);

		return m_oFullQueue.CountGet ();
	}

	/*!
	 * \brief Retrieves the numbers of packets in empty queue
	 *
	 * \return numbers of packets in empty queue
	 */
	int NumEmptyBufferAvailable ()
	{
		stiLOG_ENTRY_NAME (NumEmptyBufferAvailable);

		return m_emptyPacketPool.count ();
	}


	/*!
	 * \brief Retrieves a filled media packet from the read task
	 *
	 * \return MediaPacketType - pointer to filled media packet
	 *
	 * NOTE: expectedPacket param is only used for video
	 */
	virtual std::shared_ptr<MediaPacketType> MediaPacketFullGet (uint16_t /*expectedPacket*/)
	{
		stiLOG_ENTRY_NAME (PacketFullGet);

		// Get the oldest packet in the full queue
		auto packet = m_oFullQueue.OldestGet ();

		if (packet)
		{
			// Now remove that packet from the queue
			m_oFullQueue.Remove (packet);
		}

		return packet;
	}


	/*!
	 * \return m_un32InitialRTCPTime
	 */
	uint32_t InitialRTCPTimeGet ()
	{
		return m_un32InitialRTCPTime;
	}

	/*!
	 * \brief Set the list of allowed codecs
	 *
	 * \return estiOK
	 */
	stiHResult PayloadMapSet (
		const PayloadMapType &pPayloadMap)
	{
		Lock ();
		m_PayloadMap = pPayloadMap;
		Unlock ();

		return stiRESULT_SUCCESS;
	}
	
	/*!
	 * \brief Processes a RTX packet converting it into a useable media packet.
	 */
	void RtxPacketProcess (
		std::shared_ptr<MediaPacketType> packet,
		const uint32_t ssrc)
	{
		// Following RFC 4588 we are looking for the original sequence number (OSN).
		// This is found in the first two octets after the RTP header.
		uint16_t originalSeqNum = 0;
		++m_Stats.rtxPacketsReceived;
		auto payload = packet->BufferGet () + packet->RTPParamGet()->sByte;

		memcpy (&originalSeqNum, payload, sizeof(uint16_t));
		
		// TODO: Remove if check and always convert from network to host when no longer supporting older SSV.
		if (m_remoteSipVersion > 4)
		{
			originalSeqNum = ntohs (originalSeqNum);
		}
		
		// Set the buffer to no longer include the OSN and update the length in the RTP params to match this.
		memmove(payload, payload + sizeof(uint16_t), packet->RTPParamGet()->len - sizeof(uint16_t) - packet->RTPParamGet()->sByte);
		packet->RTPParamGet()->len -= sizeof(uint16_t);
		
		// The payload needs to be changed from the RTX payload to the one we are using for this media.
		for (auto &payload : m_PayloadMap)
		{
			if (payload.second.rtxPayloadType == packet->RTPParamGet()->payload)
			{
				packet->RTPParamGet()->payload = payload.first;
				break;
			}
		}
		
		packet->RTPParamGet()->sequenceNumber = originalSeqNum;
		packet->RTPParamGet()->sSrc = ssrc;
	}
	
	void playbackSsrcSet (const uint32_t ssrc)
	{
		m_ssrc = ssrc;
	}

public:

	CstiSignal<> dataAvailableSignal;
	int m_remoteSipVersion{};

protected:

	virtual void RTPOnReadEvent(RvRtpSession hRTP) = 0;
	virtual void EventRtpSocketDataAvailable () = 0;

	/*!
	 * \brief provide base static callback for rtp read events
	 *
	 * TODO: there's probably a slick way to use a lambda instead
	 * of having this static method...
	 */
	static void RTPOnReadEventCB(RvRtpSession hRtp, void * context)
	{
		static_cast<PlaybackReadType *>(context)->RTPOnReadEvent(hRtp);
	}
	
	// Used to calculate extended sequence numbers per stream
	struct extendedSNVars
	{
		uint16_t maxSN = 0;
		uint32_t snCycles = 0x10000;
		
		extendedSNVars (uint16_t seqNum)
		:
		maxSN (seqNum)
		{
		}
	};
	
	/*!
	 * \brief Determines the extended sequence number
	 *
	 * This method determines the 32 bit extended sequence number of a
	 * packet based on the 16 bit sequence number given in the RTP header.
	 * This is done per stream (ssrc) received in the session.
	 *
	 * \return stiHResult
	 */
	stiHResult extendedSequenceNumberDetermine (std::shared_ptr<MediaPacketType> packet)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		
		stiTESTCOND (packet, stiRESULT_INVALID_PARAMETER);
		
		hResult = extendedSequenceNumberDetermine (
					packet->RTPParamGet ()->sSrc,
					packet->RTPParamGet ()->sequenceNumber,
					&packet->m_un32ExtSequenceNumber);
		
	STI_BAIL:
		
		return hResult;
	}

#define USE_ORIGINAL_EXTENDED_SN_ALGORITHM
	stiHResult extendedSequenceNumberDetermine (
		uint32_t ssrc,
		uint16_t packetSN,
		uint32_t *extendedSN)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		
		// Check if this stream (ssrc) has already been received
		auto streamIter = m_streamSNData.find (ssrc);
		
		stiTESTCOND (extendedSN, stiRESULT_INVALID_PARAMETER);
		
		if (streamIter != m_streamSNData.end ())
		{
#ifdef USE_ORIGINAL_EXTENDED_SN_ALGORITHM
			//
			// Identical to algorithm when moved from Playback except status is
			// maintained locally in m_streamSNData instead of m_un32ExpectedPacket
			//
			// Is the sequence number equal or greater than we were expecting?
			if (packetSN >= streamIter->second.maxSN)
			{
				// Yes! Our sequence number was greater than or equal to that expected.
				// Is there a possibility that the sequence number has wrapped?
				if (streamIter->second.maxSN <= 0x000000ff &&
					packetSN >= HIGH_SEQUENCE_RANGE_CHECK)
				{
					// Subtract one from the upper sixteen bits to indicate a wrap around.
					*extendedSN = (streamIter->second.snCycles - (uint32_t)0x10000) | packetSN;
				}
				else
				{
					*extendedSN = streamIter->second.snCycles | packetSN;
					streamIter->second.maxSN = packetSN;
				}
			}
			else
			{
				// No! Our sequence number was less than expected.
				// Is there a possibility that the sequence number has wrapped?
				if (streamIter->second.maxSN >= HIGH_SEQUENCE_RANGE_CHECK &&
					packetSN <= LOW_SEQUENCE_RANGE_CHECK)
				{
					// Add one to the upper sixteen bits to indicate a wrap around.
					streamIter->second.snCycles += (uint32_t)0x10000;
					streamIter->second.maxSN = packetSN;
				}

				*extendedSN = streamIter->second.snCycles | packetSN;
			}
#else
			//
			// A modification of RFC3550 with no MAX_DROPOUT
			// If a packet is not within MAX_MISORDER behind, it is assumed to be ahead
			//
			uint16_t snDiff = packetSN - streamIter->second.maxSN;
			
			// Is this packet behind and within MAX_MISORDER of latest?
			if (snDiff > 0xffff - MAX_MISORDER &&
				packetSN > streamIter->second.maxSN)
			{
				// This packet is out of order after recently wrapping.
				*extendedSN = (streamIter->second.snCycles - (uint32_t)0x10000) | packetSN;
			}
			else
			{
				if (packetSN < streamIter->second.maxSN)
				{
					// Sequence number wrapped. Increase sequence number cycles.
					streamIter->second.snCycles += (uint32_t)0x10000;
				}
				
				*extendedSN = streamIter->second.snCycles | packetSN;
				streamIter->second.maxSN = packetSN;
			}
#endif
		}
		else
		{
			// New stream (ssrc)
			m_streamSNData.emplace (ssrc, extendedSNVars (packetSN));
			*extendedSN = (uint32_t)0x10000 | packetSN;
		}
		
	STI_BAIL:
		
		return hResult;
	}

protected:

	MediaPlaybackReadStats m_Stats;

	uint32_t m_un32InitialRTCPTime = 0;			// Initial time of call for generating RTCP statistics

	int m_nNumPackets = 0;

	int m_mediaClockRate = 0;					// Media Clock Rate

	std::shared_ptr<vpe::RtpSession> m_rtpSession;
	PayloadMapType m_PayloadMap;

	bool m_bRtpHalted = true;

	bool m_dataChannelClosed = true;
	bool m_packetDiscarding = false;
	
	uint32_t m_ssrc = 0;

	// Packet Queues
	PacketPool<MediaPacketType> m_emptyPacketPool;
	CstiPacketQueue<MediaPacketType> m_oRecvdQueue;
	CstiPacketQueue<MediaPacketType> m_oFullQueue;
	
#ifdef USE_ORIGINAL_EXTENDED_SN_ALGORITHM
	static const uint16_t HIGH_SEQUENCE_RANGE_CHECK = 0xffff - 0x3fff;
	static const uint16_t LOW_SEQUENCE_RANGE_CHECK = 0x3fff;
#else
	// MAX_MISORDER identifies how far a packet can be behind and
	// still be considered behind (instead of significantly ahead)
	// Calculated as 2 seconds of high bitrate video (33 fps, 100 packets per frame)
	static const uint16_t MAX_MISORDER = 6600;
#endif
	std::map<uint32_t, extendedSNVars> m_streamSNData;

};


} // namespace
