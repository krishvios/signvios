/*!
* \file CstiTextRecord.h
* \brief See CstiTextRecord.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "CstiRtpSession.h"
#include "MediaRecord.h"
#include "CstiTextPacket.h"
#include "stiTrace.h"
#include "RvSipMid.h"
#include <vector>

//
// Constants
//

//
// Typedefs
//

/*!
 * \struct  SstiTextRecordSettings
 * 
 * \brief Setting needed for text recording
 */
struct SstiTextRecordSettings
{
	EstiTextCodec codec = estiTEXT_NONE; //! Which Codec to use
	int8_t payloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;	//! Payload type (This will always be the one used in the RTP header)
	int8_t redPayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;	//! Redundant payload type (This will always be the one used in the redundant blocks)
	int8_t rtxPayloadType = INVALID_DYNAMIC_PAYLOAD_TYPE;	//! RTX Payload type
};

//
// Forward Declarations
//

//
// Globals
//

////////////////////////////////////////////////////////////////////////////////
// class CstiTextRecord
//
class CstiTextRecord : public vpe::MediaRecord<EstiTextCodec, SstiTextRecordSettings>
{
public:

	CstiTextRecord () = delete;

	CstiTextRecord (
		std::shared_ptr<vpe::RtpSession> session,
		uint32_t maxRate,
		ECodec codec,
		RvSipMidMgrHandle sipMidMgr);  // class constructor
	
	CstiTextRecord (const CstiTextRecord &other) = delete;
	CstiTextRecord (CstiTextRecord &&other) = delete;
	CstiTextRecord &operator= (const CstiTextRecord &other) = delete;
	CstiTextRecord &operator= (CstiTextRecord &&other) = delete;

	~CstiTextRecord () override; // class destructor

	//
	//	Data flow functions - called by Conference Manager
	//

	bool PrivacyGet () const override;

	stiHResult TextSend (const uint16_t *pun16Text);

	// Override to handle redundant text payload id as well
	bool payloadIdChanged(const SstiMediaPayload &offer, int8_t redPayloadType) const override;

private:

	stiHResult DataInfoStructLoad (
		const SstiSdpStream &sdpStream,
		const SstiMediaPayload &offer,
		int8_t redPayloadTypeNbr) override;
	
	stiHResult DataChannelInitialize () override;
	
	struct SRedundant
	{
		uint8_t *pun8Buffer = nullptr;
		uint32_t un32TimeStamp = 0;
		int32_t n32Length = 0;
	};

	struct SSendBuffer
	{
		uint8_t *pun8Buffer = nullptr;
		int nLength = 0;
	};

	//
	// Supporting functions
	//
	stiHResult PacketSend ();

	EstiPacketResult ReadPacket (
		CstiTextPacket *poPacket);

	stiHResult SetDeviceTextCodec (
		EstiTextCodec eMode);

	//
	// Event Handlers
	//
	void EventDataStart () override;
	void EventTextSend (const uint16_t *buffer, int length);
	void EventSendTimerTimeout ();

private:
	std::list<SRedundant *> m_lRedBuffer;		// Buffers already sent that need to be sent again as redundant buffers
	std::list<SSendBuffer *> m_lTextToSend;		// New received text to send for the first time

	struct timeval m_TimeLastSent{};				// The most recent time data was sent.


#if 0 // Currently no sending initialization string as part of defect 20705.
	EstiBool m_bSentByteOrderMark = estiFALSE;
#endif
	
	vpe::EventTimer m_sendTimer;

	CstiSignalConnection::Collection m_signalConnections;
};
