//
//  Packetization.h
//  ntouch
//
//  Created by Dan Shields on 6/2/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#pragma once

#include "CstiVideoPacket.h"
#include "Packetization.h"
#include "IstiVideoInput.h"
#include "CstiRtpHeaderLength.h"

namespace vpe {

class Packetization
{
public:
	Packetization (
		uint32_t rtpTimestamp,
		const SstiRecordVideoFrame *frame,
		EstiPacketizationScheme packetizationScheme)
		: m_rtpTimestamp(rtpTimestamp),
		  m_frame(frame),
		  m_packetizationScheme(packetizationScheme)
	{}
	
	bool isKeyframeGet () const
	{
		return m_frame->keyFrame;
	}
	
	uint32_t rtpTimestampGet () const
	{
		return m_rtpTimestamp;
	}
	
	bool isEmptyGet () const
	{
		// We're empty if we have no slices left and we're not currently fragmenting any slices
		return m_nextSlice != nullptr && (m_nextSlice - m_frame->buffer) >= static_cast<int32_t>(m_frame->frameSize) && m_fragmentingNalUnit == nullptr;
	}
	
	stiHResult nextRtpPayloadGet (uint8_t **data, uint32_t *packetSize);
	
private:
	stiHResult nextSliceGet (uint8_t **slice, uint32_t *sliceSize);
	stiHResult nextFragmentGet (uint8_t **data, uint32_t *packetSize);
	
	stiHResult nextSliceNALUnitGet (uint8_t **nalUnit, uint32_t *nalUnitSize);
	stiHResult nextFragmentH264FuAGet (uint8_t **data, uint32_t *packetSize);
	stiHResult nextFragmentH265FuGet (uint8_t **data, uint32_t *packetSize);
	
	uint32_t m_rtpTimestamp;
	const SstiRecordVideoFrame *m_frame;
	EstiPacketizationScheme m_packetizationScheme;
	
	uint32_t m_nextHeaderLength {0};
	uint8_t *m_nextSlice {nullptr};
	
	// Fragmenting slices
	uint8_t *m_fragmentingNalUnit {nullptr};
	int32_t m_fragmentingNalUnitSize {0};
	uint8_t m_fragmentingNalUnitHeader {0};
	uint32_t m_fragmentSize {0};
	int32_t m_nextFragmentOffset {0};
};

}
