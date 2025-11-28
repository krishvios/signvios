/*!
 * \file CstiAudioPacket.h
 * \brief contains CstiAudioPacket class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiPacket.h"
#include "AudioControl.h"

#include <sstream>

class CstiAudioPacket : public CstiPacket
{

public:
	CstiAudioPacket () = default;

	~CstiAudioPacket () override = default;

	//
	// Disable copy constructor and assignment operator
	//
	CstiAudioPacket (const CstiAudioPacket &) = delete;
	CstiAudioPacket &operator= (const CstiAudioPacket &) = delete;

	std::string str() const override
	{
		std::stringstream ss;
		ss << CstiPacket::str();
		ss << "\tAudio Frame Pointer = " << (void*)&m_frame<< '\n';

		return ss.str();
	}

	using FrameType = SsmdAudioFrame *;

	EstiAudioCodec esmdAudioMode{estiAUDIO_NONE};		// audio format of this frame

	FrameType m_frame = nullptr;

};
