#pragma once

#include "stiSVX.h"
#include "VideoControl.h"
#include "IstiVideoOutput.h"

class IstiVideoDecoder
{
public:
	enum DecoderFlags
	{
		FLAG_NONE,
		FLAG_FRAME_COMPLETE = 1,
		FLAG_KEYFRAME = 2,
		FLAG_IFRAME_REQUEST = 4,
		FLAG_RESEND_FRAME = 8
	};

	virtual stiHResult AvDecoderInit(EstiVideoCodec esmdVideoCodec) = 0;
	virtual stiHResult AvDecoderClose() = 0;
	virtual stiHResult AvDecoderDecode(uint8_t *pBytes, uint32_t un32Len, uint8_t * flags) = 0;
	virtual stiHResult AvDecoderDecode(IstiVideoPlaybackFrame *pFrame, uint8_t * flags) = 0;
	virtual stiHResult AvDecoderPictureGet(uint8_t *pBytes, uint32_t un32Len, uint32_t *width, uint32_t *height) = 0;
	virtual EstiVideoCodec AvDecoderCodecGet() = 0;

};