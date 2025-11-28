/*!
 * \file CstiTextPacket.h
 * \brief contains CstiTextPacket class and related SsmdTextFrame struct
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "CstiPacket.h"

#include <sstream>

/*!
* \struct SsmdTextFrame
*
*/
struct SsmdTextFrame
{
	EstiTextCodec esmdTextMode {estiTEXT_NONE};	//!  text format of this frame
	uint8_t * pun8Buffer {nullptr};       		//! pointer to the  text frame buffer
	uint32_t unBufferMaxSize {0};  			//! total size of the allocated buffer
	uint32_t unFrameSizeInBytes {0};		//! size of one frame
};

class CstiTextPacket : public CstiPacket
{

public:
	CstiTextPacket () = default;

	~CstiTextPacket () override = default;

	//
	// Disable move/copy constructor and assignment operator
	//
	CstiTextPacket (const CstiTextPacket &other) = delete;
	CstiTextPacket (CstiTextPacket &&other) = delete;
	CstiTextPacket &operator= (const CstiTextPacket &other) = delete;
	CstiTextPacket &operator= (CstiTextPacket &&other) = delete;

	using FrameType = SsmdTextFrame *;

	std::string str() const override
	{
		std::stringstream ss;
		ss << CstiPacket::str();
		ss << "\tText Frame Pointer = " << (void*)&m_frame << '\n';

		return ss.str();
	}

	EstiTextCodec esmdTextMode {estiTEXT_NONE};		// text format of this frame

	FrameType m_frame {nullptr};
};
