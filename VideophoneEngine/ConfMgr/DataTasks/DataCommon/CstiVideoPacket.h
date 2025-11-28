/*!
 * \file CstiVideoPacket.h
 * \brief contains CstiVideoPacket class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiPacket.h"
#include "VideoControl.h"
#include "VideoFrame.h"
#include "IstiVideoOutput.h"

#include <sstream>

class CstiVideoPacket : public CstiPacket
{

public:
	CstiVideoPacket () = default;

		~CstiVideoPacket () override = default;

	//
	// Disable copy constructor and assignment operator
	//
	CstiVideoPacket (const CstiVideoPacket &) = delete;
	CstiVideoPacket &operator= (const CstiVideoPacket &) = delete;

	std::string str() const override
	{
		std::stringstream ss;
		ss << CstiPacket::str();
		ss << "\tVideo Frame Pointer = " << (void*)&m_frame<< '\n';
		// TODO: more?

		return ss.str();
	}

	uint32_t unPacketSizeInBytes = 0;
	uint8_t un8PacketsPerFrame = 0;

	uint32_t unZeroBitsAtStart = 0;		// zero padding bits at packet start (sBit)
	uint32_t unZeroBitsAtEnd = 0;		// zero padding bits at packet end (eBit)

	IstiVideoPlaybackFrame *m_pPlaybackFrame = nullptr;

	using FrameType = vpe::VideoFrame *;

	FrameType m_frame = nullptr;

};

