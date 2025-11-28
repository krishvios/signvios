//*****************************************************************************
//
// FileName:    CstiFFMpegDecoder.cpp
//
// Abstract:    Implementation of the CstiFFMpegDecoder class which handles: 
//				- Opening and decodeing H.264 files
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//*****************************************************************************

#include "CstiFFMpegDecoder.h"

#define DEFAULT_VIDEO_WIDTH 320
#define DEFAULT_VIDEO_HEIGHT 240

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CstiFFMpegDecoder::CstiFFMpegDecoder( ) 
	:
	m_pAvFormatContext (NULL),
	m_pAvCodecContext(NULL),
	m_pAvFrame(NULL),
	m_pAvFrameConvert(NULL),
	m_pAvResizeFrame(NULL),
	m_un32VideoWidth(0),
	m_un32VideoHeight(0),
	m_nVideoStream (-1)
{
}

CstiFFMpegDecoder::~CstiFFMpegDecoder()
{
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

	if (m_pAvResizeFrame)
	{
		av_free(m_pAvResizeFrame);
		m_pAvFrame = NULL;
	}

	if (m_pAvCodecContext)
	{
		avcodec_close(m_pAvCodecContext);
		m_pAvCodecContext = NULL;
	}

	if (m_pAvFormatContext)
	{
		av_close_input_file(m_pAvFormatContext);
		m_pAvFormatContext = NULL;
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

stiHResult CstiFFMpegDecoder::CreateFFMpegDecoder(CstiFFMpegDecoder **pFFMpegDecoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pFFMpegDecoder = new CstiFFMpegDecoder();
	stiTESTCOND(pFFMpegDecoder != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

STI_BAIL:

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegDecoder::OpenInputFile
//
//  Description: Opens the input file and initializes the decoder.
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if input file is opened.
//		stiRESULT_MEMORY_ALLOC_ERROR if input file failes.
//
stiHResult CstiFFMpegDecoder::OpenInputFile(const char *ptszInputFile)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nAvError = 0;
	AVCodec *pAvCodec = NULL;

	// Make sure avcodec is initialized (This must be called first!)
	avcodec_init();

	// Register all of the avcodec's formats and codecs.
	av_register_all();

	// Open the vido file. (The NULLs and 0 tell AVCodec to figure out
	// what the formate is and use a default buffer size).
	nAvError = av_open_input_file(&m_pAvFormatContext,
								 ptszInputFile,
								 NULL,
								 0,
								 NULL);
	stiTESTCOND(nAvError == 0, stiRESULT_INVALID_MEDIA);

	// Find the video stream.
	nAvError = av_find_stream_info(m_pAvFormatContext);
	stiTESTCOND(nAvError >= 0, stiRESULT_INVALID_MEDIA);

	m_nVideoStream = -1;
	for (unsigned int i = 0; i < m_pAvFormatContext->nb_streams; i++)
	{
		if (m_pAvFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
		{
			m_nVideoStream = i;
			break;
		}
	}
	stiTESTCOND(m_nVideoStream != -1, stiRESULT_INVALID_MEDIA);

	// Get the codec context and the codec.
	m_pAvCodecContext = m_pAvFormatContext->streams[m_nVideoStream]->codec;
	pAvCodec = avcodec_find_decoder(m_pAvCodecContext->codec_id);
	stiTESTCOND(pAvCodec != NULL, stiRESULT_INVALID_MEDIA);

	// Open the codec.
	nAvError = avcodec_open(m_pAvCodecContext, pAvCodec);
	stiTESTCOND(nAvError == 0, stiRESULT_INVALID_MEDIA);
	
	// Allocate a video frame.
	m_pAvFrame = avcodec_alloc_frame();

	// Set the default frame size if one has not already been set.
	if (m_un32VideoWidth == 0 || 
		m_un32VideoHeight == 0 ||
		m_un32VideoWidth > (unsigned int)m_pAvCodecContext->width ||
		m_un32VideoHeight > (unsigned int)m_pAvCodecContext->height)
	{
		m_un32VideoWidth = m_pAvCodecContext->width;
		m_un32VideoHeight = m_pAvCodecContext->height;
	}

	// Check the frame size.  If the desired size is smaller than the frame size
	// we need to create a resize frame.
#if 0
	else if (m_pAvCodecContext->width > m_n32VideoWidth ||
			 m_pAvCodecContext->height > m_n32VideoHeight)
	{
		AvVideoFrameResize(m_n32VideoWidth,
						   m_n32VideoHeight);
	}
#endif

STI_BAIL:
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegDecoder::AvFrameGet
//
//  Description: Returns the current frame.
//
//  Abstract:
// 
//  Returns:
//		S_OK if CAvDecoder is created.
//		S_FALSE if CAvDecoder creation failes.
//
stiHResult CstiFFMpegDecoder::AvFrameGet(AVFrame **pAvFrameSample,
										 uint64_t *pun64FrameSampleTime)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nAvError = 0;
	int nFrameFinished = 0;
	AVPacket avPacket;
	*pAvFrameSample = NULL;
	*pun64FrameSampleTime = 0;
	
	do
	{
		nAvError = av_read_frame(m_pAvFormatContext, &avPacket);
		if (nAvError < 0)
		{
			// Assume that we hit the end of the file.  Reset back to the front 
			// and call av_read_frame again.
			nAvError = av_seek_frame(m_pAvFormatContext, m_nVideoStream,
									 0, AVSEEK_FLAG_ANY);
			stiTESTCOND(nAvError >= 0, stiRESULT_INVALID_MEDIA);

			nAvError = av_read_frame(m_pAvFormatContext, &avPacket);
		}
		stiTESTCOND(nAvError >= 0, stiRESULT_INVALID_MEDIA);
		
		if (avPacket.stream_index == m_nVideoStream)
		{
			nAvError = avcodec_decode_video2(m_pAvCodecContext, 
											 m_pAvFrame, 
											 &nFrameFinished,
											 &avPacket);
			if (nAvError < 0)
			{
				av_free_packet(&avPacket);
				stiTHROW (stiRESULT_ERROR);
			}
		}

		// Return the sample time in 100 nano-second units.
//		*pun64FrameSampleTime = (uint64_st)(((float)(avPacket.pts * m_pAvCodecContext->time_base.num * 10000000)) / 
//								 (float)m_pAvCodecContext->time_base.den);
		*pun64FrameSampleTime = (uint64_t)(((float)(avPacket.pts * m_pAvFormatContext->streams[m_nVideoStream]->time_base.num * 10000000)) / 
								(float)(m_pAvFormatContext->streams[m_nVideoStream]->time_base.den));

		// Free the packet.
		av_free_packet(&avPacket);
	}while (nFrameFinished == 0);
	
	if (m_pAvResizeFrame &&
		m_pAvFrameConvert)
	{
		nAvError = sws_scale(m_pAvFrameConvert,
							 m_pAvFrame->data,
							 m_pAvFrame->linesize, 
							 0,
							 m_pAvCodecContext->height,
							 m_pAvResizeFrame->data,
							 m_pAvResizeFrame->linesize);
		stiTESTCOND(nAvError >= 0, stiERROR);

		*pAvFrameSample = m_pAvResizeFrame;
	}
	else
	{
		*pAvFrameSample = m_pAvFrame;
	}

STI_BAIL:

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegDecoder::AvVideoFrameResize
//
//  Description: Sets up interal members used to resize the 
//				 output frame.
//
//  Abstract:
// 
//  Returns:
//		S_OK if the frame can be resized.
//		S_FALSE if the frame cannot be resized.
//
stiHResult CstiFFMpegDecoder::AvVideoFrameResize(
		uint32_t un32FrameWidth,
		uint32_t un32FrameHeight)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Check the frame size.  If the desired size is smaller than the frame size
	// we need to create a resize frame.
	if (un32FrameWidth < m_un32VideoWidth &&
		un32FrameHeight < m_un32VideoHeight)
	{
		m_un32VideoWidth = un32FrameWidth;
		m_un32VideoHeight = un32FrameHeight;

		m_pAvResizeFrame = avcodec_alloc_frame();
		stiTESTCOND(m_pAvResizeFrame != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

		// Allocate the buffer for the resize frame.  
		int nNumFrameBytes = avpicture_get_size(PIX_FMT_YUV420P, 
												m_un32VideoWidth,
												m_un32VideoHeight);
		stiTESTCOND(nNumFrameBytes < 0, stiRESULT_INVALID_PARAMETER);

		uint8_t *pun8FrameBuffer = new uint8_t[nNumFrameBytes];
		stiTESTCOND(pun8FrameBuffer != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

		// Assign the buffer parts to the image planes.
		avpicture_fill((AVPicture *)m_pAvResizeFrame, 
						pun8FrameBuffer, 
						PIX_FMT_YUV420P,
						m_un32VideoWidth,
						m_un32VideoHeight);

		// Create the context used to resize the frame.
		m_pAvFrameConvert = sws_getContext(m_pAvCodecContext->width,
										   m_pAvCodecContext->height,
										   m_pAvCodecContext->pix_fmt,
										   m_un32VideoWidth,
										   m_un32VideoHeight,
										   PIX_FMT_YUV420P,
										   SWS_LANCZOS,
										   NULL, NULL, NULL);
		stiTESTCOND(m_pAvFrameConvert != NULL, stiRESULT_MEMORY_ALLOC_ERROR);
	}
	else if (un32FrameWidth > m_un32VideoWidth ||
			 un32FrameHeight > m_un32VideoHeight)
	{
		hResult = stiRESULT_INVALID_PARAMETER;
	}

STI_BAIL:

	return hResult;
}

#if 0
//////////////////////////////////////////////////////////////////////
//; CAvDecoder::AvFpsGet
//
//  Description: Gets the frame rate of the current video.
//
//  Abstract:
// 
//  Returns:
//		The frame rate as a float
//
float CAvDecoder::AvVideoFpsGet()
{
	HRESULT hResult = S_OK;
	float fFrameRate = 0.0;
	if (m_pAvFormatContext)
	{
//		fFrameRate = (float)(m_pAvCodecContext->time_base.den) / (float)(m_pAvCodecContext->time_base.num);
		fFrameRate = (float)(m_pAvFormatContext->streams[m_nVideoStream]->r_frame_rate.num) / (float)(m_pAvFormatContext->streams[m_nVideoStream]->r_frame_rate.den);
	}
	return fFrameRate;
}

//////////////////////////////////////////////////////////////////////
//; CAvDecoder::AvVideoBitRateGet
//
//  Description: Returns the bitrate of the open video
//
//  Abstract:
// 
//  Returns:
//		Bitrate of the open file.
//
uint32_st CAvDecoder::AvVideoBitRateGet()
{
	uint32_st nBitRate = 0;
	if (m_pAvFormatContext)
	{
		nBitRate = m_pAvFormatContext->bit_rate;
	}
	return nBitRate;
}


//////////////////////////////////////////////////////////////////////
//; CAvDecoder::AvVideoTimeBaseGet
//
//  Description: Returns the time base of the open video
//
//  Abstract:
// 
//  Returns:
//		None.
//
void CAvDecoder::AvVideoTimeBaseGet(int *pnTimeNum,
									int *pnTimeDem)
{
	*pnTimeNum = m_pAvFormatContext->streams[m_nVideoStream]->time_base.num;
	*pnTimeDem = m_pAvFormatContext->streams[m_nVideoStream]->time_base.den;
}
#endif
