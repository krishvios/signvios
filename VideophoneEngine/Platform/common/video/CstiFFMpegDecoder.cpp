//*****************************************************************************
//
// FileName:    CstiFFMpegDecoder.cpp
//
// Abstract:    Implementation of the CstiFFMpegDecoder class which handles: 
//				- Opening and decodeing H.264 files
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiFFMpegDecoder.h"

#define DEFAULT_VIDEO_WIDTH 320
#define DEFAULT_VIDEO_HEIGHT 240

#define MAX_CONSECUTIVE_FAILS 50 // just a wild guess

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CstiFFMpegDecoder::CstiFFMpegDecoder( ) 
	:
	m_esmdVideoCodec(estiVIDEO_NONE),
	m_pAvCodecContext(NULL),
	m_pAvFrame(NULL),
	m_un32ErrorCount(0),
	m_un32VideoWidth(0),
	m_un32VideoHeight(0)
{
}

CstiFFMpegDecoder::~CstiFFMpegDecoder()
{
	AvDecoderClose();
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegDecoder::AvDecoderClose
//
//  Description: Cleans up allocated objects.
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS 
//

stiHResult CstiFFMpegDecoder::AvDecoderClose()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_pAvFrame)
	{
		av_free(m_pAvFrame);
		m_pAvFrame = NULL;
	}


	if (m_pAvCodecContext)
	{
		avcodec_close(m_pAvCodecContext);
		av_free(m_pAvCodecContext);
		m_pAvCodecContext = NULL;
	}
	
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegDecoder::CreateFFMpegDecoder
//
//  Description: Static function used to create a CFFMpegDecoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CFFMpegDecoder is created.
//		stiRESULT_MEMORY_ALLOC_ERROR if CDecoder creation failes.
//

stiHResult CstiFFMpegDecoder::CreateFFMpegDecoder(IstiVideoDecoder **pFFMpegDecoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pFFMpegDecoder = new CstiFFMpegDecoder();
	stiTESTCOND(pFFMpegDecoder != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult CstiFFMpegDecoder::AvDecoderInit(EstiVideoCodec esmdVideoCodec) {
	stiHResult hResult = stiRESULT_SUCCESS;

	avcodec_register_all();
	
	switch (esmdVideoCodec) {
  case estiVIDEO_H263:
		m_pAvCodec = avcodec_find_decoder(AV_CODEC_ID_H263);
    break;
	case estiVIDEO_H264:
		m_pAvCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
		break;
#ifdef stiENABLE_H265_DECODE
	case estiVIDEO_H265:
		m_pAvCodec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
		break;
#endif
  default:
		return stiRESULT_INVALID_CODEC;
    break;
	}
	m_esmdVideoCodec=esmdVideoCodec;
	
	stiTESTCOND(m_pAvCodec != NULL, stiRESULT_ERROR);

	m_pAvCodecContext = avcodec_alloc_context3(m_pAvCodec);
	stiTESTCOND(m_pAvCodecContext != NULL, stiRESULT_ERROR);

//      error_count=0;
//      m_lastIFrame = PTime(); // reset to current time
//  m_pAvCodecContext->errors_detected = 0;
		
	m_pAvCodecContext->err_recognition = true;
	//_context->skip_loop_filter = AVDISCARD_ALL;
	m_pAvCodecContext->flags = m_pAvCodecContext->flags | CODEC_FLAG_PSNR;
	m_pAvCodecContext->skip_frame = AVDISCARD_NONREF;

  
	m_pAvFrame = av_frame_alloc ();
	stiTESTCOND(m_pAvFrame != NULL, stiRESULT_ERROR);
		
	if (avcodec_open2(m_pAvCodecContext, m_pAvCodec, NULL) < 0)
	{
		hResult = stiRESULT_ERROR;
	}

STI_BAIL:
	return hResult;
}

stiHResult CstiFFMpegDecoder::AvDecoderDecode(IstiVideoPlaybackFrame *pFrame, uint8_t * flags) {
	return CstiFFMpegDecoder::AvDecoderDecode(pFrame->BufferGet(), pFrame->DataSizeGet(), flags);
}

stiHResult CstiFFMpegDecoder::AvDecoderDecode(uint8_t *pBytes, uint32_t un32Len, uint8_t *flags) {
	stiHResult hResult = stiRESULT_SUCCESS;

	static unsigned int consecutive_fails=0;

	bool decodedFrame = false;
	AVPacket packet;
	av_init_packet(&packet);
	packet.data=const_cast<uint8_t*>(pBytes);
	packet.size=un32Len;

	int result = avcodec_send_packet (m_pAvCodecContext, &packet);
	
	if (!result)
	{
		// Success
		result = avcodec_receive_frame (m_pAvCodecContext, m_pAvFrame);

		if (!result)
		{
			// Success
			decodedFrame = true;
		}
		else
		{
			// Failed to decode
			*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
		}
	}
	else
	{
		// Failed to decode
		*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
	}
	
	if (!decodedFrame) {
		consecutive_fails++;
	} else {
		consecutive_fails=0;
	}
	if (consecutive_fails > MAX_CONSECUTIVE_FAILS) {
		// decoder can abort under certain circomstances where we keep feeding it data
		// but it hasn't decoded any frames.
		*flags |= FLAG_IFRAME_REQUEST;
		consecutive_fails=0;
		avcodec_close(m_pAvCodecContext);
		if (avcodec_open2(m_pAvCodecContext, m_pAvCodec,NULL) < 0)
			hResult = stiRESULT_ERROR;
	}
	
	if (decodedFrame) {
		if (m_pAvFrame->key_frame) {
			// signal is a key frame
			*flags |= FLAG_KEYFRAME;
		}
		m_un32VideoWidth = m_pAvCodecContext->width;
		m_un32VideoHeight = m_pAvCodecContext->height;
		*flags |= FLAG_FRAME_COMPLETE;
	}

	return hResult;
}

stiHResult CstiFFMpegDecoder::AvDecoderPictureGet ( uint8_t *pBytes, uint32_t un32Len, uint32_t *width, uint32_t *height ) {

  stiHResult hResult = stiRESULT_SUCCESS;

	size_t ySize = m_un32VideoHeight * m_un32VideoWidth;
	size_t uvSize = ySize/4;
	size_t toLen = ySize+uvSize*2;
	*width = m_un32VideoWidth;
	*height = m_un32VideoHeight;

	uint8_t * src[3] = { m_pAvFrame->data[0], m_pAvFrame->data[1], m_pAvFrame->data[2] };
	uint8_t * dst[3] = { pBytes, dst[0] + ySize, dst[1] + uvSize };
	size_t dstLineSize[3] = { static_cast<size_t>(m_pAvFrame->width), static_cast<size_t>(m_pAvFrame->width/2), static_cast<size_t>(m_pAvFrame->width/2) };

	stiTESTCOND(toLen<=un32Len, stiRESULT_BUFFER_TOO_SMALL);


	for (int y = 0; y < m_pAvFrame->height; ++y) {
		for (int plane = 0; plane < 3; ++plane) {
			if ((y&1) == 0 || plane == 0) {
				memcpy(dst[plane], src[plane], dstLineSize[plane]);
				src[plane] += m_pAvFrame->linesize[plane];
				dst[plane] += dstLineSize[plane];
			}
		}
	}

STI_BAIL:
	return hResult;
}
