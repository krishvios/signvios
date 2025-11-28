/*!
* \file CstiTextPlaybackRead.h
* \brief See CstiTextPlaybackRead.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "stiOS.h"
#include "CstiRtpSession.h"
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "stiTrace.h"
#include "CstiDataTaskCommonP.h"
#include "stiPayloadTypes.h"
#include "CstiTextPacket.h"
#include "PacketPool.h"
#include "CstiPacketQueue.h"
#include "MediaPlaybackRead.h"
#include <vector>

extern "C"
{
	#include "RvTurn.h"
}
#include <string>
#include <array>


class CstiTextPlaybackRead : public vpe::MediaPlaybackRead<CstiTextPacket, TTextPayloadMap>
{

public:

	// Constructor/Destructor
	CstiTextPlaybackRead () = delete;

	CstiTextPlaybackRead (const std::shared_ptr<vpe::RtpSession> &session);

	CstiTextPlaybackRead (const CstiTextPlaybackRead &other) = delete;
	CstiTextPlaybackRead (CstiTextPlaybackRead &&other) = delete;
	CstiTextPlaybackRead &operator= (const CstiTextPlaybackRead &other) = delete;
	CstiTextPlaybackRead &operator= (CstiTextPlaybackRead &&other) = delete;

	~CstiTextPlaybackRead () override;

	stiHResult Initialize () override;

	stiHResult DataChannelInitialize (
			uint32_t un32InitialRtcpTime,
			const TTextPayloadMap *pPayloadMap) override;

	stiHResult DataChannelClose () override;

	// Data Movement Functions

	stiHResult ReSyncJitterBuffer ();

	void RTPOnReadEvent(RvRtpSession hRTP) override;


private:

	// Event identifiers
	// These enumerations need to start at estiEVENT_NEXT or greater to not collide
	// with enumerations defined by the base classes.
	enum EEventType
	{
		estiRTP_PACKET_RECEIVED = estiEVENT_NEXT,
	};
	
	static const int nT140_TEXT_PACKETS_BUFFERED = 6;

	static const int nTEXT_PACKETS_ALLOCATED = nT140_TEXT_PACKETS_BUFFERED + 2 + nOUTOFORDER_TP_BUFFERS;

	//
	// Private member variables
	//
#ifdef stiDEBUGGING_TOOLS
	int m_nDiscardedTextPackets = 0;			// Number of discarded packets
#endif

	EstiBool m_bSetJitter = estiTRUE;

	int m_nPacketsToHold = 0;

	int m_nPacketsHeld = 0;

	// These buffers allocates space to hold the data read in the RvRtpRead call.
	// These buffers are allocated to the empty queue at initialization
	std::array< std::vector<uint8_t>, nTEXT_PACKETS_ALLOCATED > m_textReadBuffer;

	SsmdTextFrame m_astTextStruct[nTEXT_PACKETS_ALLOCATED]{};

	//
	// Supporting Functions
	//
	void EventRtpSocketDataAvailable () override;
	
	EstiPacketResult PacketRead (
		const std::shared_ptr<CstiTextPacket> &packet);

	stiHResult ReadPacketProcess (
		const std::shared_ptr<CstiTextPacket> &packet);

	stiHResult DataUnPack (
		const std::shared_ptr<CstiTextPacket> &packet,
		bool *pbIsTextPacket);

	void EventRtpPacketReceived (const std::shared_ptr<CstiTextPacket> &packet);

	static void RVCALLCONV TextRTCPReportProcess (
		RvRtcpSession hRTCP,	// The RTCP session for which the data has arrived
		void* pContext,
		RvUint32 ssrc);

	static RvBool RVCALLCONV TextEnumerator (
		RvRtcpSession hRTCP,	// Handle to RadVision RTCP Session
		RvUint32 ssrc,		// Synchronization Source ID
		void *pContext);

	bool m_bRemoteTextMuted = false;
	bool m_dataChannelClosed = true;

}; // end CstiTextPlaybackRead
