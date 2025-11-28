/*!
 * \file AppleUtilities.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once
#include "IstiVideoOutput.h"
#include <CoreMedia/CoreMedia.h>
#include <vector>

struct SstiRecordVideoFrame;

namespace vpe {

/**
 * \brief Convert a FourCharCode (e.g. 'hevc') to an EstiVideoCodec
 *
 * Returns estiVIDEO_NONE if there is no equivalent EstiVideoCodec enum.
 */
EstiVideoCodec ConvertFourCharCodeToVideoCodec (CMVideoCodecType codec);


/**
 * \brief Convert an EstiVideoCodec to a FourCharCode (e.g. 'hevc')
 *
 * Returns 0 if `codec` is estiVIDEO_NONE.
 */
CMVideoCodecType ConvertVideoCodecToFourCharCode (EstiVideoCodec codec);


/**
 * \brief Create a video frame from a CMSampleBufferRef
 */
stiHResult VideoPacketFromSampleBuffer (
	CMSampleBufferRef sampleBuffer,
	SstiRecordVideoFrame *videoPacket);


/**
 * \brief Create a CMVideoFormatDescriptionRef from a video packet.
 *
 * Also sets the keyframe flag of the IstiVideoPlaybackFrame if parsing the
 * packet reveals that the frame is a keyframe.
 *
 * Note that a format description may not always be created on success (for
 * example, if the video frame is not a keyframe and thus does not have the
 * needed parameter sets).
 */
stiHResult FormatDescriptionFromFrameGet (
	IstiVideoPlaybackFrame *frame,
	EstiVideoCodec codec,
	CMVideoFormatDescriptionRef *formatDesc);


/**
 * \brief Create a CMSampleBufferRef with appropriate format and timing info.
 *
 * Creates a CMBlockBufferRef that returns the IstiVideoPlaybackFrame to
 * IstiVideoOutput for reuse after deallocation via VideoPlaybackDiscardFrame.
 */
stiHResult SampleBufferFromPlaybackFrameGet (
	IstiVideoPlaybackFrame *frame,
	CMVideoFormatDescriptionRef formatDescription,
	CMSampleBufferRef *sampleBuffer);


}
