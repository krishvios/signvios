/*!
* \file CstiAudioRecord.h
* \brief See CstiAudioRecord.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

//
// Includes
//
#include "IstiAudioInput.h"
#include "MediaRecord.h"
#include "CstiRtpSession.h"
#include "CstiAudioPacket.h"
#include "stiTrace.h"
#include "RvSipMid.h"

#include <memory>
#include <vector>
#include <atomic>

//
// Constants
//

//
// Typedefs
//

/*!
 * \struct  SstiAudioRecordSettings
 * 
 * \brief Setting needed for audio recording
 */
struct SstiAudioRecordSettings
{
	EstiAudioCodec codec = estiAUDIO_NONE; //! Which Codec to use
	int nMaxAudioFrames = 0;                     //! Maximum number of audio frames
	int8_t payloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;    //! Payload type
	int8_t dtmfEventPayloadNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;  //! DTMF event type
	int8_t rtxPayloadType = INVALID_DYNAMIC_PAYLOAD_TYPE;	//! RTX Payload type
};

//
// Forward Declarations
//

//
// Globals
//

////////////////////////////////////////////////////////////////////////////////
// class CstiAudioRecord
//
class CstiAudioRecord : public vpe::MediaRecord<EstiAudioCodec, SstiAudioRecordSettings>
{
public:

	CstiAudioRecord () = delete;

	CstiAudioRecord (
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t maxRate,
		ECodec codec,
		RvSipMidMgrHandle hSipMidMgr);  // class constructor
	
	CstiAudioRecord (const CstiAudioRecord &other) = delete;
	CstiAudioRecord (CstiAudioRecord &&other) = delete;
	CstiAudioRecord &operator= (const CstiAudioRecord &other) = delete;
	CstiAudioRecord &operator= (CstiAudioRecord &&other) = delete;

	~CstiAudioRecord () override; // class destructor

	stiHResult Initialize () override;
	
	void Startup () override;

	void DataChannelClose () override;

	//
	//	Audio record settings
	//

	bool PrivacyGet() const override;

	stiHResult AudioInputDeviceSet(
	    const std::shared_ptr<IstiAudioInput> &audioInput);
	
	void DtmfToneSend (EstiDTMFDigit eDTMFDigit);
	EstiBool DtmfSupportedGet () const;

	EstiAudioCodec CodecGet ();

private:
	void ConnectSignals ();
	void DisconnectSignals ();

	stiHResult DataInfoStructLoad (
		const SstiSdpStream &sdpStream,
		const SstiMediaPayload &offer, int8_t secondaryPayloadId) override;

private:

	stiHResult DataChannelInitialize () override;
	
	// Instance for audio device controller
	std::shared_ptr<IstiAudioInput> m_pAudioInput = nullptr;
	
	uint8_t *m_pRemainderBuffer = nullptr;           // Buffer for storing the remainder of driver packets
	uint16_t m_nRemainderSize = 0;             // Number of bytes of m_pRemainderBuffer in use

	// This buffer allocates space to hold the data read from audio device driver.
	std::vector<uint8_t> m_currentAudioBuffer;
	// This struct will hold the current audio packet buffer.
	SsmdAudioFrame m_stCurrentAudioFrame{};
	CstiAudioPacket m_oCurrentAudioPacket;
	uint32_t m_un32LastTimestampRecvdFromAudioInput = 0;
	double m_dNextSendTime = 0.0;
	
	// dtmf event struct
	typedef struct DtmfEvent {
		EstiDTMFDigit dtmfDigit = (EstiDTMFDigit)0;
		uint32_t duration = 0;
		uint32_t current = 0;
		uint8_t lastPacketCount = 0;
	} DtmfEvent;
	
	RvUint32 m_dtmfTimeStamp {0};
	RvUint32 m_dtmfSSRC {0};

	vpe::EventTimer m_dtmfTimer;
	std::vector<DtmfEvent> m_DtmfEvents;
	bool m_dtmfInProgress = false;
	
	vpe::EventTimer m_delaySendTimer;
	int m_nCurrentOffset = 0;

	CstiSignalConnection::Collection m_signalConnections;

	//
	// Supporting functions
	//
	bool DelayTimerNeeded (uint32_t rtpTimeStamp);
	EstiResult PacketProcessFromDriver ();
	stiHResult PacketSend ();
	EstiResult PacketSend (RvRtpParam *pRTPParam, const uint8_t *pszDataBuffer);
	
	EstiPacketResult ReadPacket (
		CstiAudioPacket *poPacket);
	
	EstiResult SetDeviceAudioCodec (
		EstiAudioCodec eMode);
		
	EstiBool DtmfToneSendInternal(
		EstiDTMFDigit newEvent,
		uint32_t duration,
		EstiBool bFirstPacket,
		EstiBool bEndPackets);

	//
	// Event Handlers
	//
	void EventDtmfTimerTimeout ();
	void EventDelaySendTimerTimeout ();
	void EventDtmfToneSend (EstiDTMFDigit digit);
	void EventDataStart () override;
	void EventAudioInputDeviceSet (
	    std::shared_ptr<IstiAudioInput> audioInput);
	void EventPacketsProcess ();

	std::atomic<bool> m_audioInputPacketReady;
	CstiSignalConnection m_audioInputPacketReadyStateSignalConnection;
	CstiSignalConnection m_audioPrivacySignalConnection;
};
