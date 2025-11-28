/*!
 * \file GstreamerBufferElement.h
 * \brief See GstreamerBufferElement.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

//
// Includes
//
#include "stiSVX.h"
#include "IstiVideoInput.h"
#include "CstiBufferElement.h"
#include "GStreamerSample.h"
#include "VideoControl.h"
#include <gst/gst.h>

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

class GstreamerBufferElement : public CBufferElement
{
public:
	GstreamerBufferElement () = default;

	GstreamerBufferElement (const GstreamerBufferElement &other) = delete;
	GstreamerBufferElement (GstreamerBufferElement &&other) = delete;
	GstreamerBufferElement &operator= (const GstreamerBufferElement &other) = delete;
	GstreamerBufferElement &operator= (GstreamerBufferElement &&other) = delete;

	~GstreamerBufferElement () override = default;

	uint32_t VideoWidthGet ();

	void VideoWidthSet (
		uint32_t un32VideoWidth);

	uint32_t VideoHeightGet ();

	void VideoHeightSet (
		uint32_t un32VideoHeight);

	uint32_t VideoRowBytesGet ();

	void VideoRowBytesSet (
		uint32_t un32RowBytes);

	void FifoAssign (CFifo <GstreamerBufferElement> *pFifo)
	{
		m_pFifo = pFifo;
	}

	stiHResult GstreamerBufferSet (
		GStreamerBuffer &gstBuffer,
		bool bKeyFrame,
		EstiVideoCodec eVideoCodec,
		EstiVideoFrameFormat eVideoFrameFormat);

	stiHResult FifoReturn ();

	bool KeyFrameGet ();

	EstiVideoCodec CodecGet ();

	EstiVideoFrameFormat VideoFrameFormatGet ();

	uint64_t TimeStampGet ();

	timeval m_ArrivalTime{};

protected:

	GStreamerBuffer m_gstBuffer;

	bool m_bKeyFrame{false};
	EstiVideoCodec m_eVideoCodec{estiVIDEO_NONE};
	EstiVideoFrameFormat m_eVideoFrameFormat{estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED};
	uint64_t m_timeStamp = 0;

	CFifo <GstreamerBufferElement> *m_pFifo{nullptr};

}; // end GstreamerBufferElement Class
