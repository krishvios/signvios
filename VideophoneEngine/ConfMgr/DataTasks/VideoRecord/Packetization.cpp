//
//  NALPacketization.cpp
//  ntouch
//
//  Created by Dan Shields on 6/2/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

#include "Packetization.h"
#include "stiTools.h"
#include "CstiDataTaskCommonP.h"
#include "payload.h"
#include "RtpPayloadAddon.h"

namespace vpe {

stiHResult Packetization::nextSliceNALUnitGet (uint8_t **nalUnit, uint32_t *nalUnitSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	//
	// Determine the first byte in the NAL Unit, the length of the NAL Unit, the offset
	// to the next NAL Unit and whether or not this is the last NAL Unit in the frame.
	//
	if (m_frame->eFormat == estiBYTE_STREAM)
	{
		uint32_t headerLength = 0;

		// The first NAL unit
		if (m_nextSlice == nullptr)
		{
			//
			// Look for the first byte stream header.
			//
			ByteStreamNALUnitFind (m_frame->buffer, m_frame->frameSize,
								   nalUnit, &headerLength);

			stiTESTCOND (*nalUnit != nullptr, stiRESULT_ERROR);
		}
		else
		{
			*nalUnit = m_nextSlice;
			headerLength = m_nextHeaderLength;
		}

		*nalUnit += headerLength;

		auto frameEnd = m_frame->buffer + m_frame->frameSize;

		//
		// If we found a byte stream header look for the one following so that
		// we can compute size of the current NAL Unit and determine if this is the
		// last NAL Unit in the frame.
		//
		ByteStreamNALUnitFind (*nalUnit, frameEnd - *nalUnit,
							   &m_nextSlice, &m_nextHeaderLength);

		if (m_nextSlice == nullptr)
		{
			//
			// We didn't find another byte stream header so this must be the last NAL Unit in the frame.
			//
			m_nextSlice = frameEnd;
		}

		*nalUnitSize = m_nextSlice - *nalUnit;
	}
	else
	{
		if (m_nextSlice == nullptr)
		{
			m_nextSlice = m_frame->buffer;
		}
		
		//
		// Get the slice size from the buffer and, if it is in big endian
		// format then byte swap the value.
		//
		*nalUnitSize = *reinterpret_cast<uint32_t *>(m_nextSlice);
		*nalUnit = (uint8_t *)(m_nextSlice + sizeof (uint32_t));

		if (estiBIG_ENDIAN_PACKED == m_frame->eFormat)
		{
			*nalUnitSize = stiDWORD_BYTE_SWAP(*nalUnitSize);
		}

		//
		// Compute the next NAL Unit position.
		//
		switch (m_frame->eFormat)
		{
			case estiLITTLE_ENDIAN_PACKED:
			case estiBIG_ENDIAN_PACKED:
				m_nextSlice += *nalUnitSize + sizeof(uint32_t);
				break;

			case estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED:
				m_nextSlice += ((*nalUnitSize + sizeof (uint32_t) + 3) & ~3);
				break;

			case estiBYTE_STREAM:
				break;

			case estiPACKET_INFO_FOUR_BYTE_ALIGNED:
			case estiPACKET_INFO_PACKED:
			case estiRTP_H263_RFC2190:
			case estiRTP_H263_RFC2190_FOUR_BYTE_ALIGNED:
			case estiRTP_H263_RFC2429:
			case estiRTP_H263_MICROSOFT:

				stiTHROW (stiRESULT_ERROR);

				break;
		}
	}
	
STI_BAIL:
	return hResult;
}

stiHResult Packetization::nextSliceGet(uint8_t **slice, uint32_t *sliceSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	switch (m_frame->eFormat)
	{
		case estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED:
		case estiLITTLE_ENDIAN_PACKED:
		case estiBIG_ENDIAN_PACKED:
		case estiBYTE_STREAM:
			nextSliceNALUnitGet (slice, sliceSize);
			break;
			
		case estiPACKET_INFO_FOUR_BYTE_ALIGNED:
		case estiPACKET_INFO_PACKED:
		case estiRTP_H263_RFC2190:
		case estiRTP_H263_RFC2429:
		case estiRTP_H263_MICROSOFT:
		case estiRTP_H263_RFC2190_FOUR_BYTE_ALIGNED:
			// We no longer support H263
			stiTHROW (stiRESULT_ERROR);
			break;
	}
	
STI_BAIL:
	return hResult;
}

stiHResult Packetization::nextRtpPayloadGet (uint8_t **data, uint32_t *packetSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_fragmentingNalUnit)
	{
		// If we're currently fragmenting a packet, continue fragmenting it.
		hResult = nextFragmentGet (data, packetSize);
		stiTESTRESULT ();
	}
	else {
		// Otherwise read the next NAL unit and fragment it if necessary.
		hResult = nextSliceGet (data, packetSize);
		stiTESTRESULT ();
		
		if (*packetSize > stiMAX_VIDEO_RECORD_PACKET)
		{
			// Start fragmenting the packet
			m_fragmentingNalUnit = *data;
			m_fragmentingNalUnitSize = *packetSize;
			m_nextFragmentOffset = 0;
			
			hResult = nextFragmentGet(data, packetSize);
			stiTESTRESULT ();
		}
	}

STI_BAIL:
	return hResult;
}

stiHResult Packetization::nextFragmentGet (uint8_t **data, uint32_t *packetSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	switch (m_packetizationScheme) {
		case estiPktUnknown:
		case estiH263_Microsoft:
		case estiH263_RFC_2190:
		case estiH263_RFC_2429:
		case estiH264_SINGLE_NAL:
		case estiPktNone:
			stiTHROW (stiRESULT_ERROR);
			break;
			
		case estiH264_NON_INTERLEAVED:
		case estiH264_INTERLEAVED:
			hResult = nextFragmentH264FuAGet(data, packetSize);
			stiTESTRESULT ();
			break;
		
		case estiH265_NON_INTERLEAVED:
			hResult = nextFragmentH265FuGet(data, packetSize);
			stiTESTRESULT ();
			break;
	}
	
STI_BAIL:
	return hResult;
}

stiHResult Packetization::nextFragmentH264FuAGet (uint8_t **data, uint32_t *packetSize)
{
	const int nalHeaderSize = 1;
	const int fuHeaderSize = nalHeaderSize + 1;

	stiHResult hResult = stiRESULT_SUCCESS;
	int sBit = 0;
	int eBit = 0;

	stiTESTCOND (m_fragmentingNalUnit != nullptr, stiRESULT_ERROR);

	// If this is the first fragment
	if (m_nextFragmentOffset == 0)
	{
		//
		// Copy the NAL Unit header, and start at the first byte after the NAL Unit header
		//
		sBit = 1;
		m_fragmentingNalUnitHeader = m_fragmentingNalUnit[0];
		m_nextFragmentOffset += nalHeaderSize;
		
		uint32_t nalUnitSize = m_fragmentingNalUnitSize - m_nextFragmentOffset;
		m_fragmentSize = nalUnitSize / ((nalUnitSize / (stiMAX_VIDEO_RECORD_PACKET - fuHeaderSize)) + 1);
	}
	
	// If this is the last fragment
	if (m_fragmentingNalUnitSize - m_nextFragmentOffset <= stiMAX_VIDEO_RECORD_PACKET - fuHeaderSize)
	{
		eBit = 1;
		m_fragmentSize = m_fragmentingNalUnitSize - m_nextFragmentOffset;
	}

	//
	// Create the H264 payload Header
	//
	m_fragmentingNalUnit[m_nextFragmentOffset - 2] = (m_fragmentingNalUnitHeader & 0x60) | estiH264PACKET_FUA;
	
	//
	// Create the FU Header
	//
	m_fragmentingNalUnit[m_nextFragmentOffset-1] = (sBit << 7) | (eBit << 6) | (m_fragmentingNalUnitHeader & 0x1F);
	
	*data = m_fragmentingNalUnit + m_nextFragmentOffset - fuHeaderSize;
	*packetSize = m_fragmentSize + fuHeaderSize;
	
	m_nextFragmentOffset += m_fragmentSize;
	
	if (m_nextFragmentOffset >= m_fragmentingNalUnitSize)
	{
		// Signal that we're done fragmenting the NAL unit
		m_fragmentingNalUnit = nullptr;
		m_fragmentingNalUnitSize = 0;
		m_fragmentingNalUnitHeader = 0;
		m_fragmentSize = 0;
		m_nextFragmentOffset = 0;
	}

STI_BAIL:
	return hResult;
}

// The H265 NAL Unit header
// The fields in the payload header are set as follows.
// F = forbidden_zero_bit
// Type = nal_unit_type
// LayerId = nuh_layer_id
// TID = nuh_temporal_id_plus1
//
//   0                   1
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |F|   Type    |  LayerId  | TID |
//  +-------------+-----------------+
//

// An FU consists of a payload header (denoted as PayloadHdr), an FU
// header of one octet, a conditional 16-bit DONL field (in network
// byte order), and an FU payload, as shown in Figure 9.
//
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |    PayloadHdr (Type=49)       |   FU header   | DONL (cond)   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
// | DONL (cond)   |                                               |
// |-+-+-+-+-+-+-+-+                                               |
// |                         FU payload                            |
// |                                                               |
// |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                               :...OPTIONAL RTP padding        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//			Figure 9 The structure of an FU


// The H265 payload header
// The fields in the payload header are set as follows.  The Type
// field MUST be equal to 49.  The fields F, LayerId, and TID MUST
// be equal to the fields F, LayerId, and TID, respectively, of the
// fragmented NAL unit.
//
//   0                   1
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |F|   Type    |  LayerId  | TID |
//  +-------------+-----------------+
//
//	 F       = 0
//	 Type    = 49 (fragmentation unit (FU))
//	 LayerId = 0
//	 TID     = 1

// The FU header consists of an S bit, an E bit, and a 6-bit FuType
// field, as shown in Figure 10.
//
// +---------------+
// |0|1|2|3|4|5|6|7|
// +-+-+-+-+-+-+-+-+
// |S|E|  FuType   |
// +---------------+
//
// Figure 10   The structure of FU header
// The semantics of the FU header fields are as follows:
// S: 1 bit
// When set to one, the S bit indicates the start of a fragmented
// NAL unit i.e. the first byte of the FU payload is also the
// first byte of the payload of the fragmented NAL unit.  When
// the FU payload is not the start of the fragmented NAL unit
// payload, the S bit MUST be set to zero.
//
// E: 1 bit
// When set to one, the E bit indicates the end of a fragmented
// NAL unit, i.e. the last byte of the payload is also the last
// byte of the fragmented NAL unit.  When the FU payload is not
// the last fragment of a fragmented NAL unit, the E bit MUST be
// set to zero.
//
// FuType: 6 bits
// The field FuType MUST be equal to the field Type of the
// fragmented NAL unit.

stiHResult Packetization::nextFragmentH265FuGet (uint8_t **data, uint32_t *packetSize)
{
	const int nalHeaderSize = 2;
	const int fuHeaderSize = nalHeaderSize + 1;

	stiHResult hResult = stiRESULT_SUCCESS;
	int sBit = 0;
	int eBit = 0;

	stiTESTCOND (m_fragmentingNalUnit != nullptr, stiRESULT_ERROR);

	// If this is the first fragment
	if (m_nextFragmentOffset == 0)
	{
		//
		// Copy the NAL Unit header, and start at the first byte after the NAL Unit header
		//
		
		sBit = 1;
		m_fragmentingNalUnitHeader = m_fragmentingNalUnit[0];
		m_nextFragmentOffset += nalHeaderSize;
		
		uint32_t nalUnitSize = m_fragmentingNalUnitSize - m_nextFragmentOffset;
		m_fragmentSize = nalUnitSize / ((nalUnitSize / (stiMAX_VIDEO_RECORD_PACKET - fuHeaderSize)) + 1);
	}
	
	// If this is the last fragment
	if (m_fragmentingNalUnitSize - m_nextFragmentOffset <= stiMAX_VIDEO_RECORD_PACKET - fuHeaderSize)
	{
		eBit = 1;
		m_fragmentSize = m_fragmentingNalUnitSize - m_nextFragmentOffset;
	}
	
	//
	// Create the H265 payload Header
	//
	m_fragmentingNalUnit[m_nextFragmentOffset - 3] = estiH265PACKET_FU << 1;
	m_fragmentingNalUnit[m_nextFragmentOffset - 2] = 1;
	
	//
	// Create the FU Header
	//
	m_fragmentingNalUnit[m_nextFragmentOffset - 1] = (sBit << 7) | (eBit << 6) | ((m_fragmentingNalUnitHeader >> 1) & 0x3F); // Set Start bit, End bit, and NAL type
	
	*data = m_fragmentingNalUnit + m_nextFragmentOffset - fuHeaderSize;
	*packetSize = m_fragmentSize + fuHeaderSize;
	
	m_nextFragmentOffset += m_fragmentSize;
	
	if (m_nextFragmentOffset >= m_fragmentingNalUnitSize)
	{
		// Signal that we're done fragmenting the NAL unit
		m_fragmentingNalUnit = nullptr;
		m_fragmentingNalUnitSize = 0;
		m_fragmentingNalUnitHeader = 0;
		m_fragmentSize = 0;
		m_nextFragmentOffset = 0;
	}

STI_BAIL:
	return hResult;
}

}
