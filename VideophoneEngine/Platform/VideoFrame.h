/*!
 * \file VideoFrame.h
 * \brief contains VideoFrame class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

namespace vpe
{

/*!
 * \brief base for video frame types to share commonalities
 */
struct VideoFrame
{
	uint8_t *buffer{nullptr};		//!< pointer to the video frame data
	uint32_t frameSize{0};		//! size of this frame in bytes
	bool keyFrame{false};			//!< is this frame a key frame?
};

} //namespace
