////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiPacket
//
//  File Name:	CstiPacket.h
//
//  Owner:		Scot L. Brooksby
//
//	Abstract:
//		ACTION - enter description
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

//
// Includes
//
#include "rtp.h"		// public definitions
#include "stiSVX.h"
#include <sstream>

// RTOS layering:
#include "stiOS.h"
#include "stiTrace.h"
#include "stiAssert.h"
//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiPacket
{

//#define FRAME_PACKET_ANALYSIS
//  Enabling the above define also enables the analysis code.
//  The analysis code is found in VideophoneEngine > ConfMgr > DataTasks > VideoPlayback > CsitVideoPlayackRead.cpp
#ifdef FRAME_PACKET_ANALYSIS
	friend class FramePacketsHistory;
	friend class CstiVideoPlaybackRead;
#endif

public:
	CstiPacket ()
	{
		RvRtpParamConstruct(&m_stRTPParam);
	}

	virtual	~CstiPacket () = default;

	//
	// Disable copy constructor and assignment operator
	//
	CstiPacket (const CstiPacket &) = delete;
	CstiPacket &operator= (const CstiPacket &) = delete;

	virtual std::string str() const
	{
		std::stringstream ss;
		ss << "\tBuffer = " << m_pun8Buffer << '\n';
		ss << "\tunBufferMaxSize = " << m_unBufferMaxSize << '\n';

		return ss.str();
	}

	//
	// Access methods
	//
	inline uint8_t *			RtpHeaderGet () const;
	inline void					RtpHeaderSet (
									uint8_t *pun8RtpHeader);
	
	inline uint8_t *			PayloadGet () const;
	inline void					PayloadSet (
									uint8_t *pun8Payload);
	
	inline uint8_t *			BufferGet ();
	inline RvRtpParam *			RTPParamGet ();
	
	//
	// Update methods
	//
	inline void					BufferSet (
									uint8_t * pun8Buffer);

	uint32_t m_un32ExtSequenceNumber = 0;

	inline bool isKeepAlive() const;

public: // TODO should these be protected?
	RvRtpParam			m_stRTPParam{};
	uint8_t *			m_pun8Buffer = nullptr;
	uint32_t			m_unBufferMaxSize = 0; // NOTE: really only used for video
	bool retransmit {false}; // Indicates the packet was  retransmitted packet

protected:
	uint8_t *			m_pun8RtpHeader = nullptr;
	uint8_t *			m_pun8Payload = nullptr;

private:
};

//:-----------------------------------------------------------------------------
//: Inline Access Methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiPacket::BufferGet
//
// Description: Returns a pointer to the buffer
//
// Abstract:
//
// Returns:
//	uint8_t * - pointer to buffer or NULL 
//
uint8_t * CstiPacket::BufferGet ()
{
	stiLOG_ENTRY_NAME (CstiPacket::BufferGet);

	return (m_pun8Buffer);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiPacket::RTPParamGet
//
// Description: Returns address of RvRtpParam
//
// Abstract:
//
// Returns:
//	RvRtpParam * - address of rtpParam
//
RvRtpParam * CstiPacket::RTPParamGet ()
{
	stiLOG_ENTRY_NAME (CstiPacket::RTPParamGet);

	return (&m_stRTPParam);
}

//:-----------------------------------------------------------------------------
//: Inline Update Methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiPacket::BufferSet
//
// Description: Sets the pointer to the buffer
//
// Abstract:
//
// Returns:
//	none
//
void CstiPacket::BufferSet (
	uint8_t * pun8Buffer)
{
	stiLOG_ENTRY_NAME (CstiPacket::BufferSet);

	m_pun8Buffer = pun8Buffer;
}


uint8_t *CstiPacket::RtpHeaderGet () const
{
	return m_pun8RtpHeader;
}


void CstiPacket::RtpHeaderSet (
	uint8_t *pun8RtpHeader)
{
	//
	// Assert that there is a buffer and that the rtp header pointer will point to somewhere inside it.
	//
	stiASSERT (m_pun8Buffer);
//	stiASSERT (pun8Payload > m_pun8Buffer && pun8Payload < m_pun8Buffer + m_un32Size);
	
	m_pun8RtpHeader = pun8RtpHeader;
}


uint8_t *CstiPacket::PayloadGet () const
{
	return m_pun8Payload;
}


void CstiPacket::PayloadSet (
	uint8_t *pun8Payload)
{
	//
	// Assert that there is a buffer and that the payload pointer will point to somewhere inside it.
	//
	stiASSERT (m_pun8Buffer);
//	stiASSERT (pun8Payload > m_pun8Buffer && pun8Payload < m_pun8Buffer + m_un32Size);
	
	m_pun8Payload = pun8Payload;
}


bool CstiPacket::isKeepAlive() const
{
	// NOTE: when using srtp, stun packets are read as "valid" rtp packets
	// This is probably a bug in the stack
	// in the meantime, do this simple check to check for an rtp header

	// If this has an rtp header, and no payload, it's a keepalive packet
	return m_stRTPParam.sByte != 0 // rtp header check
		&& m_stRTPParam.len - m_stRTPParam.sByte == 0 // empty payload check
		;
}
