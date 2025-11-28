//
//  AppleUtilities.cpp
//  ntouch
//
//  Created by Daniel Shields on 1/12/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

#include "AppleUtilities.h"
#include <VideoToolbox/VideoToolbox.h>
#include "CstiSyncManager.h"
#include "IstiVideoInput.h"

namespace vpe {

struct ParameterSet
{
	size_t size;
	const uint8_t *data;
};

/*
 * Get NAL Unit parameter sets for H264 packets
 */
stiHResult ParameterSetsForH264 (CMFormatDescriptionRef format, std::vector<ParameterSet>& params, int& nalUnitHeaderLength)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;

	size_t count;
	status = CMVideoFormatDescriptionGetH264ParameterSetAtIndex (format, 0, nullptr, nullptr, &count, &nalUnitHeaderLength);
	// If we get this error here, the parameter sets are already in the sample 
	// buffer instead of in the format description (because of software encoding).
	if (status == kCMFormatDescriptionError_InvalidParameter) { 
		return stiRESULT_SUCCESS;
	}
	stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status = %d", status);

	params.resize (count);
	for (int i = 0; i < params.size (); ++i)
	{
		status = CMVideoFormatDescriptionGetH264ParameterSetAtIndex (
			format, i,
			&params[i].data,
			&params[i].size,
			nullptr, nullptr);
		stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status = %d", status);
	}

STI_BAIL:
	return hResult;
}


/*
 * Get NAL Unit parameter sets for H265 packets
 */
stiHResult ParameterSetsForH265 (CMFormatDescriptionRef format, std::vector<ParameterSet>& params, int& nalUnitHeaderLength) NS_AVAILABLE(10_13, 11_0)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;

	size_t count;
	status = CMVideoFormatDescriptionGetHEVCParameterSetAtIndex (format, 0, nullptr, nullptr, &count, &nalUnitHeaderLength);
	// If we get this error here, the parameter sets are already in the sample 
	// buffer instead of in the format description (because of software encoding).
	if (status == kCMFormatDescriptionError_InvalidParameter) { 
		return stiRESULT_SUCCESS;
	}
	stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status = %d", status);

	params.resize (count);
	for (int i = 0; i < params.size (); ++i)
	{
		status = CMVideoFormatDescriptionGetHEVCParameterSetAtIndex (
			format, i,
			&params[i].data,
			&params[i].size,
			nullptr, nullptr);
		stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status = %d", status);
	}

STI_BAIL:
	return hResult;
}


/**
 * \brief Get a list of NAL parameter sets from a CMFormatDescriptionRef
 *
 * The data pointed to by each ParameterSet struct is allocated by the
 * CMFormatDescriptionRef, so the CMFormatDescriptionRef should be retained as
 * long as the ParameterSet structs are needed.
 */
stiHResult ParameterSetsFromFormatDescription (
	CMVideoFormatDescriptionRef format,
	std::vector<ParameterSet>* params,
	int* nalUnitHeaderLength)
{
	// Wrap the encoded frame in a packet
	stiHResult hResult = stiRESULT_SUCCESS;
	switch (CMFormatDescriptionGetMediaSubType (format))
	{
		case kCMVideoCodecType_H263:
			// H.263 doesn't have parameter sets.
			break;
		case kCMVideoCodecType_H264:
			hResult = ParameterSetsForH264 (format, *params, *nalUnitHeaderLength);
			stiTESTRESULT ();
			break;
		case kCMVideoCodecType_HEVC:
			if (@available (iOS 11.0, macOS 10.13, *))
			{
				hResult = ParameterSetsForH265 (format, *params, *nalUnitHeaderLength);
				stiTESTRESULT ();
			}
			else
			{
				stiTHROW (stiRESULT_INVALID_CODEC);
			}
			break;
		default:
			stiTHROW (stiRESULT_INVALID_CODEC);
	}

STI_BAIL:
	return hResult;
}

stiHResult VideoPacketFromSampleBufferH263 (
	CMSampleBufferRef sampleBuffer,
	SstiRecordVideoFrame *videoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	{
		CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray (sampleBuffer, false);
		if (attachments != nullptr)
		{
			CFDictionaryRef attachment = (CFDictionaryRef)CFArrayGetValueAtIndex (attachments, 0);
			CFBooleanRef dependsOnOthers = (CFBooleanRef)CFDictionaryGetValue (attachment, kCMSampleAttachmentKey_DependsOnOthers);
			videoPacket->keyFrame = (dependsOnOthers == kCFBooleanFalse);
		}

		CMTime timeStamp = CMSampleBufferGetPresentationTimeStamp (sampleBuffer);
		videoPacket->timeStamp = CMTimeConvertScale (timeStamp, 1000000000, kCMTimeRoundingMethod_QuickTime).value;

		CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer (sampleBuffer);
		size_t blockSize = CMBlockBufferGetDataLength (blockBuffer);
		videoPacket->frameSize = blockSize;
		videoPacket->eFormat = estiRTP_H263_RFC2190;

		stiTESTCOND (videoPacket != nullptr, stiRESULT_BUFFER_TOO_SMALL);
		stiTESTCOND (videoPacket->buffer != nullptr, stiRESULT_BUFFER_TOO_SMALL);
		stiTESTCOND (videoPacket->unBufferMaxSize >= videoPacket->frameSize, stiRESULT_BUFFER_TOO_SMALL);

		size_t offset = 0;
		CMBlockBufferCopyDataBytes(blockBuffer, 0, blockSize, videoPacket->buffer + offset);
		offset += blockSize;
	}

STI_BAIL:
	return hResult;
}

stiHResult VideoPacketFromSampleBufferNAL (
	CMSampleBufferRef sampleBuffer,
	const std::vector<ParameterSet>& params,
	int nalUnitHeaderSize,
	SstiRecordVideoFrame *videoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Check if the sampleBuffer contains a keyframe.
	CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray (sampleBuffer, false);
	bool isKeyframe = false;
	if (attachments != nullptr)
	{
		CFDictionaryRef attachment = (CFDictionaryRef)CFArrayGetValueAtIndex (attachments, 0);
		CFBooleanRef dependsOnOthers = (CFBooleanRef)CFDictionaryGetValue (attachment, kCMSampleAttachmentKey_DependsOnOthers);
		isKeyframe = (dependsOnOthers == kCFBooleanFalse);
	}

	CMTime timeStamp = CMSampleBufferGetPresentationTimeStamp (sampleBuffer);

	// Get the size of the video packet
	size_t totalParamsSize = 0;
	if (isKeyframe)
	{
		for (const auto& paramSet : params)
		{
			if (paramSet.size > 0)
			{
				totalParamsSize += paramSet.size + nalUnitHeaderSize;
			}
		}
	}

	CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer (sampleBuffer);
	size_t blockSize = CMBlockBufferGetDataLength (blockBuffer);

	size_t frameSize = totalParamsSize + blockSize;
	size_t offset = 0;

	stiTESTCOND (videoPacket != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND (videoPacket->buffer != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND (videoPacket->unBufferMaxSize >= frameSize, stiRESULT_BUFFER_TOO_SMALL);

	// Copy compressed frame into the video packet
	if (isKeyframe)
	{
		for (const auto& paramSet : params)
		{
			if (paramSet.size > 0)
			{
				if (nalUnitHeaderSize == 1)
				{
					uint8_t sizeData = paramSet.size;
					memcpy (videoPacket->buffer + offset, &sizeData, sizeof (uint8_t));
					offset += sizeof (uint8_t);
				}
				else if (nalUnitHeaderSize == 2)
				{
					uint16_t sizeData = CFSwapInt16HostToBig (paramSet.size);
					memcpy (videoPacket->buffer + offset, &sizeData, sizeof (uint16_t));
					offset += sizeof (uint16_t);
				}
				else if (nalUnitHeaderSize == 4)
				{
					uint32_t sizeData = CFSwapInt32HostToBig (paramSet.size);
					memcpy (videoPacket->buffer + offset, &sizeData, sizeof (uint32_t));
					offset += sizeof (uint32_t);
				}

				memcpy (videoPacket->buffer + offset, paramSet.data, paramSet.size);
				offset += paramSet.size;
			}
		}
	}

	CMBlockBufferCopyDataBytes (blockBuffer, 0, blockSize, videoPacket->buffer + offset);
	offset += blockSize;

	videoPacket->frameSize = frameSize;
	videoPacket->keyFrame = isKeyframe ? estiTRUE : estiFALSE;
	videoPacket->eFormat = estiBIG_ENDIAN_PACKED;
	videoPacket->timeStamp = CMTimeConvertScale (timeStamp, 1000000000, kCMTimeRoundingMethod_QuickTime).value;

STI_BAIL:
	return hResult;
}

stiHResult VideoPacketFromSampleBuffer (
	CMSampleBufferRef sampleBuffer,
	SstiRecordVideoFrame *videoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	{
		CMVideoFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription (sampleBuffer);
		stiTESTCONDMSG (formatDescription != nil, stiRESULT_ERROR, "Sample buffer has no format description.");

		switch (CMFormatDescriptionGetMediaSubType (formatDescription)) {
			case kCMVideoCodecType_H263:
				hResult = VideoPacketFromSampleBufferH263 (sampleBuffer, videoPacket);
				stiTESTRESULT ();
				break;

			case kCMVideoCodecType_H264:
			case kCMVideoCodecType_HEVC:
			{
				std::vector<ParameterSet> paramSets;
				int nalUnitHeaderSize;
				hResult = ParameterSetsFromFormatDescription (formatDescription, &paramSets, &nalUnitHeaderSize);
				stiTESTRESULT ();

				hResult = VideoPacketFromSampleBufferNAL (sampleBuffer, paramSets, nalUnitHeaderSize, videoPacket);
				stiTESTRESULT ();
				break;
			}

			default:
				stiTHROW (stiRESULT_INVALID_CODEC);
		}
	}

STI_BAIL:
	return hResult;
}


/*
 * Convert CMVideoCodecType to EstiVideoCodec
 */
EstiVideoCodec ConvertFourCharCodeToVideoCodec (CMVideoCodecType codec)
{
	switch (codec)
	{
		case kCMVideoCodecType_H263:
			return estiVIDEO_H263;
		case kCMVideoCodecType_H264:
			return estiVIDEO_H264;
		case kCMVideoCodecType_HEVC:
			return estiVIDEO_H265;
		default:
			// The engine doesn't support whatever codec this is
			return estiVIDEO_NONE;
	}
}


/*
 * Convert EstiVideoCodec to CMVideoCodecType
 */
CMVideoCodecType ConvertVideoCodecToFourCharCode (EstiVideoCodec codec)
{
	switch (codec)
	{
		case estiVIDEO_H263:
			return kCMVideoCodecType_H263;
		case estiVIDEO_H264:
			return kCMVideoCodecType_H264;
		case estiVIDEO_H265:
			return kCMVideoCodecType_HEVC;
		case estiVIDEO_RTX:
			return 0;
		case estiVIDEO_NONE:
			return 0;
	}
}


stiHResult SampleBufferFromPlaybackFrameGet (
	IstiVideoPlaybackFrame *videoFrame,
	CMVideoFormatDescriptionRef formatDescription,
	CMSampleBufferRef *sampleBufferOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CMBlockBufferRef blockBuffer = nullptr;

	{
		OSStatus status = 0;

		// Create a block buffer that returns the frame to us when deallocated
		CMBlockBufferCustomBlockSource blockSource;
		blockSource.version = kCMBlockBufferCustomBlockSourceVersion;
		blockSource.refCon = (void *)videoFrame;
		blockSource.AllocateBlock = nullptr;
		blockSource.FreeBlock = [](void *refCon, void *block, size_t size)
		{
			IstiVideoPlaybackFrame *frame = static_cast<IstiVideoPlaybackFrame *>(refCon);
			IstiVideoOutput::InstanceGet ()->VideoPlaybackFrameDiscard (frame);
		};

		status = CMBlockBufferCreateWithMemoryBlock (
			 kCFAllocatorDefault,
			 (void *)videoFrame->BufferGet (),
			 videoFrame->BufferSizeGet (),
			 kCFAllocatorNull,
			 &blockSource, 0,
			 videoFrame->DataSizeGet (), 0,
			 &blockBuffer);
		stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status=%d", status);

		// TODO: RTP provides us with enough information to get better timing
		// info, but we don't have that info here. So for now, use the current
		// time as the presentation time.

		CMSampleTimingInfo timingInfo;
		timingInfo.duration = CMTimeMake (1, 30);
		timingInfo.presentationTimeStamp = CMClockGetTime (CMClockGetHostTimeClock ());
		timingInfo.decodeTimeStamp = kCMTimeInvalid;
		size_t sampleSize = CMBlockBufferGetDataLength (blockBuffer);

		status = CMSampleBufferCreate (
			kCFAllocatorDefault,
			blockBuffer,
			true, nullptr, nullptr,
			formatDescription,
			1, 1, &timingInfo,
			1, &sampleSize,
			sampleBufferOut);
		stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status=%d", status);
	}

STI_BAIL:
	if (blockBuffer != nullptr)
	{
		CFRelease (blockBuffer);
	}

	return hResult;
}


/**
 * Returns an error if the NAL units failed to parse. A format description may
 * not be output regardless of success.
 */
stiHResult FormatDescriptionFromH264Get (
	IstiVideoPlaybackFrame *frame,
	CMVideoFormatDescriptionRef *formatDescOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint8_t *data = frame->BufferGet ();
	ptrdiff_t bytesLeft = frame->DataSizeGet ();

	std::vector <uint8_t *> nalUnits;
	std::vector <size_t> nalSizes;
	bool hasSPS = false;
	bool hasPPS = false;

	typedef uint32_t nal_size_t;

	while (bytesLeft > sizeof (nal_size_t))
	{
		nal_size_t nalSize = CFSwapInt32BigToHost (*(nal_size_t *)data);
		uint8_t *nalUnit = data + sizeof (nal_size_t);

		stiTESTCOND (nalSize <= bytesLeft, stiRESULT_ERROR);

		uint8_t nalType = (nalUnit[0] & 0x1F);
		switch (nalType)
		{
			case estiH264PACKET_NAL_SPS:
				hasSPS = true;
				break;
			case estiH264PACKET_NAL_PPS:
				hasPPS = true;
				break;
			case estiH264PACKET_NAL_CODED_SLICE_OF_IDR:
				frame->FrameIsKeyframeSet (true);
				break;
		}

		// Documented supported NAL units for CMVideoFormatDescriptionCreateFromH264ParameterSets
		switch (nalType)
		{
			case estiH264PACKET_NAL_SPS:
			case estiH264PACKET_NAL_PPS:
			case estiH264PACKET_NAL_SPS_EXTENSION:
				nalUnits.push_back (nalUnit);
				nalSizes.push_back (nalSize);
				break;
			default:
				break;
		}

		data += sizeof (nal_size_t) + nalSize;
		bytesLeft -= sizeof (nal_size_t) + nalSize;
	}

	if (hasSPS && hasPPS)
	{
		OSStatus status = 0;
		status = CMVideoFormatDescriptionCreateFromH264ParameterSets (
			kCFAllocatorDefault,
			nalUnits.size (),
			nalUnits.data (),
			nalSizes.data (),
			sizeof (nal_size_t),
			formatDescOut);
		stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status=%d", status);
	}

STI_BAIL:
	return hResult;
}

/**
 * Returns an error if the NAL units failed to parse. A format description may
 * not be output regardless of success.
 */
stiHResult FormatDescriptionFromH265Get (
	IstiVideoPlaybackFrame *frame,
	CMVideoFormatDescriptionRef *formatDescOut) NS_AVAILABLE(10_13, 11_0)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint8_t *data = frame->BufferGet ();
	ptrdiff_t bytesLeft = frame->DataSizeGet ();

	std::vector <uint8_t *> nalUnits;
	std::vector <size_t> nalSizes;
	bool hasVPS = false;
	bool hasSPS = false;
	bool hasPPS = false;

	typedef uint32_t nal_size_t;

	while (bytesLeft > sizeof (nal_size_t))
	{
		uint32_t nalSize = CFSwapInt32BigToHost (*(nal_size_t *)data);
		uint8_t *nalUnit = data + sizeof (nal_size_t);

		stiTESTCOND (nalSize <= bytesLeft, stiRESULT_ERROR);

		uint8_t nalType = ((nalUnit[0] >> 1) & 0x3F);
		switch (nalType)
		{
			case estiH265PACKET_NAL_UNIT_VPS:
				hasVPS = true;
				break;
			case estiH265PACKET_NAL_UNIT_SPS:
				hasSPS = true;
				break;
			case estiH265PACKET_NAL_UNIT_PPS:
				hasPPS = true;
				break;
			case estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_LP:
			case estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_RADL:
			case estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_N_LP:
			case estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_W_RADL:
			case estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_N_LP:
			case estiH265PACKET_NAL_UNIT_CODED_SLICE_CRA:
				frame->FrameIsKeyframeSet (true);
				break;
		}

		// Documented supported NAL units for CMVideoFormatDescriptionCreateFromHEVCParameterSets
		switch (nalType)
		{
			case estiH265PACKET_NAL_UNIT_VPS:
			case estiH265PACKET_NAL_UNIT_SPS:
			case estiH265PACKET_NAL_UNIT_PPS:
			case estiH265PACKET_NAL_UNIT_PREFIX_SEI:
			case estiH265PACKET_NAL_UNIT_SUFFIX_SEI:
				nalUnits.push_back (nalUnit);
				nalSizes.push_back (nalSize);
				break;
			default:
				break;
		}

		data += sizeof (nal_size_t) + nalSize;
		bytesLeft -= sizeof (nal_size_t) + nalSize;
	}

	if (hasVPS && hasSPS && hasPPS)
	{
		OSStatus status = 0;
		status = CMVideoFormatDescriptionCreateFromHEVCParameterSets (
			kCFAllocatorDefault,
			nalUnits.size (),
			nalUnits.data (),
			nalSizes.data (),
			sizeof (nal_size_t),
			nullptr,
			formatDescOut);
		stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status=%d", status);
	}

STI_BAIL:
	return hResult;
}

/**
 * Returns an error if the H263 header failed to parse. A format description may
 * not be output regardless of success.
 */
stiHResult FormatDescriptionFromH263Get (
	IstiVideoPlaybackFrame *frame,
	CMVideoFormatDescriptionRef *formatDescOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint8_t *data = frame->BufferGet ();
	size_t size = frame->DataSizeGet ();

	stiTESTCOND (size >= 5, stiRESULT_ERROR);

	// The picture start code is 22 bits and is equal to 0x000080, the last two
	// bits belong to the temporal reference number.
	if (data[0] == 0x00 &&
		data[1] == 0x00 &&
		(data[2] & (~0x03)) == 0x80)
	{
		uint8_t sourceFormat = (data[4] >> 2) & 0x07;
		uint8_t pictureCodingType = (data[4] >> 1) & 0x01;
		size_t width = 352;
		size_t height = 288;

		// Picture coding type 0 is an I-picture, a keyframe.
		if (pictureCodingType == 0)
		{
			frame->FrameIsKeyframeSet (true);
			switch (sourceFormat)
			{
				case 0:
					width = 0;
					height = 0;
					break;
				case 1:
					width = 128;
					height = 96;
					break;
				case 2:
					width = 176;
					height = 144;
					break;
				case 3:
					width = 352;
					height = 288;
					break;
				case 4:
					width = 704;
					height = 576;
					break;
				case 5:
					width = 1408;
					height = 1152;
					break;
				default:
					break;
			}

			OSStatus status = 0;
			status = CMVideoFormatDescriptionCreate (
				kCFAllocatorDefault,
				kCMVideoCodecType_H263,
				width, height,
				nullptr,
				formatDescOut);
			stiTESTCONDMSG (status == 0, stiRESULT_ERROR, "status=%d", status);
		}
	}
	else
	{
		// This is a badly formed packet.
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}

stiHResult FormatDescriptionFromFrameGet (
	IstiVideoPlaybackFrame *frame,
	EstiVideoCodec codec,
	CMVideoFormatDescriptionRef *formatDesc)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	switch (codec)
	{
		case estiVIDEO_H263:
			hResult = FormatDescriptionFromH263Get (frame, formatDesc);
			stiTESTRESULT ();
			break;
		case estiVIDEO_H264:
			hResult = FormatDescriptionFromH264Get (frame, formatDesc);
			stiTESTRESULT ();
			break;
		case estiVIDEO_H265:
			if (@available(iOS 11.0, macOS 10.13, *))
			{
				hResult = FormatDescriptionFromH265Get (frame, formatDesc);
				stiTESTRESULT ();
			}
			else
			{
				stiTHROW (stiRESULT_INVALID_CODEC);
			}
			break;
		default:
			stiTHROW (stiRESULT_INVALID_CODEC);
	}

STI_BAIL:
	return hResult;
}


}
