/*!
* \file CstiAudioPlaybackRead.h
* \brief See CstiAudioPlaybackRead.cpp
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
#include "CstiAudioPacket.h"
#include "stiPayloadTypes.h"
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


//
// Forward Declarations
//
class CstiSyncManager;

class CstiAudioPlaybackRead : public vpe::MediaPlaybackRead<CstiAudioPacket, TAudioPayloadMap>
{

public:

	// Constructor/Destructor
	CstiAudioPlaybackRead () = delete;

	CstiAudioPlaybackRead (const std::shared_ptr<vpe::RtpSession> &session);

	CstiAudioPlaybackRead (const CstiAudioPlaybackRead &other) = delete;
	CstiAudioPlaybackRead (CstiAudioPlaybackRead &&other) = delete;
	CstiAudioPlaybackRead &operator= (const CstiAudioPlaybackRead &other) = delete;
	CstiAudioPlaybackRead &operator= (CstiAudioPlaybackRead &&other) = delete;

	~CstiAudioPlaybackRead () override;

	stiHResult Initialize () override;

	stiHResult DataChannelInitialize (
		uint32_t un32InitialRtcpTime,
		const TAudioPayloadMap *pPayloadMap) override;

	stiHResult DataChannelClose () override;

	// Data Movement Functions

	EstiResult ReSyncJitterBuffer ();
	
	void RTPOnReadEvent(RvRtpSession hRTP) override;

public:
	CstiSignal<EstiDTMFDigit> dtmfReceivedSignal;

private:
	static const int nAUDIO_PACKETS_BUFFERED = 6;

	static const int nAUDIO_PACKETS_ALLOCATED = nAUDIO_PACKETS_BUFFERED + 2 + nOUTOFORDER_AP_BUFFERS;

	//
	// Private member variables
	//
#ifdef stiDEBUGGING_TOOLS
	int m_nDiscardedAudioPackets = 0;			// Number of discarded packets
#endif

	EstiBool m_bSetJitter = estiTRUE;

	int m_nPacketsToHold = 0;

	int m_nPacketsHeld = 0;

	// These buffers allocates space to hold the data read in the RvRtpRead call.
	// These buffers are allocated to the empty queue at initialization
	std::array< std::vector<uint8_t>, nAUDIO_PACKETS_ALLOCATED > m_audioReadBuffer;

	SsmdAudioFrame m_astAudioStruct[nAUDIO_PACKETS_ALLOCATED]{};


	void EventRtpSocketDataAvailable () override;
	void EventReceivedPacketsProcess ();

	EstiResult ProcessDTMF (
		const std::shared_ptr<CstiAudioPacket> &packet);

	EstiPacketResult PacketRead (
		const std::shared_ptr<CstiAudioPacket> &packet);

	EstiResult ReadPacketProcess (
		const std::shared_ptr<CstiAudioPacket> &packet);

	EstiResult DataUnPack (
		const std::shared_ptr<CstiAudioPacket> &packet,
		bool *pbIsAudioPacket);

	static void RVCALLCONV AudioRTCPReportProcess (
		RvRtcpSession hRTCP,	// The RTCP session for which the data has arrived
		void* pContext,
		RvUint32 ssrc);

	static RvBool RVCALLCONV AudioEnumerator (
		RvRtcpSession hRTCP,	// Handle to RadVision RTCP Session
		RvUint32 ssrc,		// Synchronization Source ID
		void *pContext);

	CstiSyncManager* m_pSyncManager = nullptr;

	uint32_t m_unDtmfTimestamp = 0;
#ifndef stiNO_LIP_SYNC
	uint32_t m_un32MostNTPTimestamp = 0;
	uint32_t m_un32LeastNTPTimestamp = 0;
	int m_nNTPSolved = 0;
	EstiBool m_bIsNTPSwapped = estiFALSE;
#endif	
}; // end CstiAudioPlaybackRead
