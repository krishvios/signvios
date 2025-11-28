//*****************************************************************************
//
// FileName:       CstiFFMpegEncoder.cpp
//
// Abstract:       Implementation of the CstiFFMpegDecoder class which handles: 
//				   - Opening and decodeing H.264 files
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiFFMpegEncoder.h"
#include "SMCommon.h"

#define DEFAULT_VIDEO_WIDTH 320
#define DEFAULT_VIDEO_HEIGHT 240

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CstiFFMpegEncoder::CstiFFMpegEncoder( )
	:
	m_un32Bitrate (0),
	m_n32VideoHeight (0),
	m_n32VideoWidth (0),
	m_nIFrame (0),
	m_nOutputBufferSize(0),
	m_pun8OutputBuffer(NULL),
	m_pAvCodecContext(NULL),
	m_pAvFormatContext(NULL),
	m_pAvFrame(NULL),
	m_pAvOutputFormat (NULL),
	m_pAvVideoStream(NULL)
{
	m_avFrameRate.den = 0;
	m_avFrameRate.num = 0;
	m_avTimeBase.den = 0;
	m_avTimeBase.num = 0;
}

CstiFFMpegEncoder::~CstiFFMpegEncoder()
{
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::CreateFFMpegEncoder
//
//  Description: Static function used to create a CAvDecoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CstiFFMpegEncoder is created.
//		stiRESULT_MEMORY_ALLOC_ERROR if CstiFFMpegEncoder creation failes.
//
stiHResult CstiFFMpegEncoder::CreateFFMpegEncoder(CstiFFMpegEncoder **pFFMpegEncoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pFFMpegEncoder = new CstiFFMpegEncoder();
	stiTESTCOND(pFFMpegEncoder != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult CstiFFMpegEncoder::AvVideoCompressFrame(AVFrame *pAvInputFrame,
												   uint64_t un64CurrentTime,
												   AVPacket *pAvCompressedPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	AVCodecContext *pAVContext = m_pAvVideoStream->codec;
	av_init_packet(pAvCompressedPacket);

	if (pAvInputFrame)
	{
		memcpy(m_pAvFrame->data[0], pAvInputFrame->data[0], pAvInputFrame->linesize[0] * m_n32VideoHeight);
		memcpy(m_pAvFrame->data[1], pAvInputFrame->data[1], pAvInputFrame->linesize[1] * m_n32VideoHeight / 2);
		memcpy(m_pAvFrame->data[2], pAvInputFrame->data[2], pAvInputFrame->linesize[2] * m_n32VideoHeight / 2);
		m_pAvFrame->linesize[0] = pAvInputFrame->linesize[0];
		m_pAvFrame->linesize[1] = pAvInputFrame->linesize[1];
		m_pAvFrame->linesize[2] = pAvInputFrame->linesize[2];

		// Calculate the time to display the frame.  CurrentTime is in 100 nanoseconds, 
		// the avcoded wants time units that are in seconds devided by the time base.
		if (un64CurrentTime == 0)
		{
			m_pAvFrame->pts = AV_NOPTS_VALUE;
		}
		else
		{
			m_pAvFrame->pts = (un64CurrentTime * pAVContext->time_base.den) / (pAVContext->time_base.num * 10000000);
		}
	}

	// Clear the buffer.
	memset(m_pun8OutputBuffer, 0x00, m_nOutputBufferSize);
	 
	// Encode the frame.
	uint32_t un32UsedOutputSize = avcodec_encode_video(pAVContext, 
														m_pun8OutputBuffer, 
														m_nOutputBufferSize, 
														pAvInputFrame != NULL ? m_pAvFrame : NULL);

	// if zero size, it means the image was buffered 
	if (un32UsedOutputSize > 0)
	{

		if ((unsigned int)pAVContext->coded_frame->pts != AV_NOPTS_VALUE)
		{
			pAvCompressedPacket->pts = av_rescale_q(pAVContext->coded_frame->pts, 
													 pAVContext->time_base, 
													 m_pAvVideoStream->time_base);
		}
		if(pAVContext->coded_frame->key_frame)
		{
			pAvCompressedPacket->flags |= PKT_FLAG_KEY;
		}

		uint8_t *pun8Temp = m_pun8OutputBuffer;

		// Reset the frame start indicator (00 00 00 01) with the size of the frame.
		if (*pun8Temp == 0x00 &&
			*(pun8Temp + 1) == 0x00 &&
			*(pun8Temp + 2) == 0x00 &&
			*(pun8Temp + 3) == 0x01)
		{
			*((uint32_t *)pun8Temp) = un32UsedOutputSize - 4;
		}

		// Check for header info that we don't want. (00 00 01)
		else if (*pun8Temp == 0x00 &&
				 *(pun8Temp + 1) == 0x00 &&
				 *(pun8Temp + 2) == 0x01)
		{
			// Set the temp pointer to point to the first position after the 00 00 01 sequence.
			pun8Temp += 3;

			//Search for the beginning of the data. (Look for a 00 00 00 01)
			for (uint32_t i = 1; i <= un32UsedOutputSize; i++, pun8Temp++)
			{
				if (*pun8Temp == 0x00 &&
					*(pun8Temp + 1) == 0x00 &&
					*(pun8Temp + 2) == 0x00 &&
					*(pun8Temp + 3) == 0x01)
				{
					un32UsedOutputSize -= i;
					*((uint32_t *)pun8Temp) = un32UsedOutputSize - 4;
					break;
				}
			}
		}

		pAvCompressedPacket->stream_index= m_pAvVideoStream->index;
		pAvCompressedPacket->data= pun8Temp;
		pAvCompressedPacket->size= un32UsedOutputSize;
	}
	else
	{
		pAvCompressedPacket->data = NULL;
		pAvCompressedPacket->size = 0;
	}

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvEncoderClose
//
//  Description: Closes the Video file.
//
//  Abstract:
// 
//  Returns:
//		S_OK if file is closed sucessfully.
//		S_FALSE if closing file failes.
//
stiHResult CstiFFMpegEncoder::AvEncoderClose()
{
	stiHResult hResult = stiRESULT_SUCCESS;
//	int nAvError = 0;

//	nAvError = av_write_trailer(m_pAvFormatContext);
//	stiTESTCOND(nAvError == 0, stiRESULT_MEMORY_ALLOC_ERROR);

	// Clean up.
	avcodec_close(m_pAvVideoStream->codec);
//	av_free(m_pAvVideoStream);
//	m_pAvVideoStream = NULL;

	av_free(m_pAvFrame);
	m_pAvFrame = NULL;

//	url_fclose(m_pAvFormatContext->pb);
	av_free(m_pAvFormatContext);
	m_pAvFormatContext = NULL;

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvVideoFrameRateSet
//
//  Description: Static function used to create a CAvDecoder
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
void CstiFFMpegEncoder::AvVideoFrameRateSet(float fFrameRate)
{
	// We store the frame rate as fps but the AVCodec stores
	// it as a time based value 1/fps.  So we have to invert
	// the framerate.

	// Are we 25fps or 29.97fps?
	if (fFrameRate >= 25.0f &&
		fFrameRate < 26.0f)
	{
		m_avFrameRate.den = 25;
		m_avFrameRate.num = 1;
	}
	else
	{
		m_avFrameRate.den = 30000;
		m_avFrameRate.num = 1001;
	}
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvInitializeEncoder
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CAvEncoder is initialized.
//		stiRESULT_ERROR if CAvEncoder initialize failes.
//
stiHResult CstiFFMpegEncoder::AvInitializeEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure avcodec is initialized (This must be called first!)
	avcodec_init();

	// Register all of the avcodec's formats and codecs.
	av_register_all();

	// Get the outputformat.
	m_pAvOutputFormat = av_guess_format("ipod", NULL, NULL);
	stiTESTCOND(m_pAvOutputFormat != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

	// Allocate the outputformat context.
	m_pAvFormatContext = avformat_alloc_context();
	stiTESTCOND(m_pAvFormatContext != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

	// Assign the outputformat to the outputforamt context.
	m_pAvFormatContext->oformat = m_pAvOutputFormat;

	// Add the video stream.
	if (m_pAvOutputFormat->video_codec != CODEC_ID_NONE)
	{
		m_pAvVideoStream = av_new_stream(m_pAvFormatContext, 0);
		stiTESTCOND(m_pAvVideoStream != NULL, stiRESULT_MEMORY_ALLOC_ERROR);
		
		m_pAvCodecContext = m_pAvVideoStream->codec;
	}
	stiTESTCOND(m_pAvFormatContext != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

STI_BAIL:

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvVideoSequenceParamSetGet
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if settings are updated
//		stiRESULT_ERROR if updating of settings fails.
//
stiHResult CstiFFMpegEncoder::AvVideoNalUnitsGet(uint8_t *pun8Buffer,
												 uint32_t un32BufSize,
												 uint32_t *pun32BufUsed)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32BufNeeded = 0;
	uint8_t *pun8SequenceParam = NULL;
	uint8_t *pun8PictureParam = NULL;
	uint8_t *pun8Temp = NULL;
	uint8_t *pun8Index = pun8Buffer;
	EstiBool bHaveSequenceParam = estiFALSE;
	uint32_t un32Size = 0;
	*pun32BufUsed = 0;

	AVCodecContext *pAvContext = m_pAvVideoStream->codec;
	stiTESTCOND(pun8Buffer != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

	// The buffer needs at least extradata_size + 16 bytes for 
	// Sequence Header info and the data size.
	un32BufNeeded = pAvContext->extradata_size + 16;
	stiTESTCOND(un32BufNeeded < un32BufSize, stiRESULT_BUFFER_TOO_SMALL);

	// Search the nal header string for "00 00 00 01" The first indicates the start of the 
	// sequence paramater set and the second one indicates the picture parameter set.
	pun8Temp = pAvContext->extradata;
	for (int i = 0; i < pAvContext->extradata_size - 8; i++, pun8Temp++)
	{
		if (*pun8Temp == 0 &&
			*(pun8Temp + 1) == 0 &&
			*(pun8Temp + 2) == 0 &&
			*(pun8Temp + 3) == 1)
		{
			pun8Temp += 4;
			if (bHaveSequenceParam == estiFALSE)
			{
				pun8SequenceParam = pun8Temp;
				bHaveSequenceParam = estiTRUE;
			}
			else
			{
				pun8PictureParam = pun8Temp;
				break;
			}
		}
	}

	// Calculate the size of the sequence parameter set.  
	// The size will be the distance between the param pointers minus 8 for the separator.
	un32Size = pun8PictureParam - pun8SequenceParam - 4;

	// Copy the sequence param size.
	*((uint32_t *)pun8Index) = un32Size;
	pun8Index += 4;

	// Copy the sequence param set to the buffer.
	memcpy(pun8Index, pun8SequenceParam, un32Size);

	// Move the temp pointer to the end of the sequence Param.
	pun8Index += un32Size;
	
	// Calculate the size of the picture param header.
	un32Size = pAvContext->extradata + pAvContext->extradata_size - pun8PictureParam;
	*((uint32_t *)pun8Index) = un32Size;
	pun8Index += 4;

	// Copy the picture param to the buffer.
	memcpy(pun8Index, pun8PictureParam, un32Size);
	pun8Index += un32Size;
	 
	*pun32BufUsed = pun8Index - pun8Buffer;

STI_BAIL:

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvUpdateEncoderSettings
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if settings are updated
//		stiRESULT_ERROR if updating of settings fails.
//
stiHResult CstiFFMpegEncoder::AvUpdateEncoderSettings()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nAvError = 0;
	AVCodec *pAvCodec = NULL;
	int tempWidth = 0;
	int frameSize = 0;
	uint8_t *pFrameBuf = NULL;

	stiTESTCOND(m_pAvFormatContext != NULL, stiRESULT_ERROR);

	// Default changes
//	m_pAvCocecContext->coder_type = 1;
	m_pAvCodecContext->flags |= CODEC_FLAG_LOOP_FILTER;
	m_pAvCodecContext->me_cmp = FF_CMP_CHROMA;
	m_pAvCodecContext->partitions |= X264_PART_I8X8 | X264_PART_P8X8 | X264_PART_B8X8;
	m_pAvCodecContext->me_method = 7;
	m_pAvCodecContext->me_subpel_quality = 7;
	m_pAvCodecContext->me_range = 16;
	m_pAvCodecContext->keyint_min = 5; //25
	m_pAvCodecContext->scenechange_threshold = 40;
	m_pAvCodecContext->i_quant_factor = 0.71f;
//	m_pAvCodecContext->b_frame_strategy = 1; 
	m_pAvCodecContext->qcompress = 0.6f;
	m_pAvCodecContext->qmin = 1;
	m_pAvCodecContext->qmax = 51;
	m_pAvCodecContext->max_qdiff = 4;
	m_pAvCodecContext->max_b_frames = 0; //3
	m_pAvCodecContext->directpred = 1;
	m_pAvCodecContext->trellis = 1;
	m_pAvCodecContext->flags2 |= CODEC_FLAG2_FASTPSKIP | CODEC_FLAG2_MBTREE;
	m_pAvCodecContext->weighted_p_pred = 0;
	// Set for general videos (baseline)
//	m_pAvCodecContext->flags2 |= CODEC_FLAG2_MBTREE;
	m_pAvCodecContext->coder_type = 0;

	// Set for current video.
	m_pAvCodecContext->codec_id = m_pAvOutputFormat->video_codec;
	m_pAvCodecContext->codec_type = CODEC_TYPE_VIDEO;

	m_pAvCodecContext->bit_rate = m_un32Bitrate;
	m_pAvCodecContext->bit_rate_tolerance = m_un32Bitrate * 2;
	m_pAvCodecContext->height = m_n32VideoHeight;
	m_pAvCodecContext->width = m_n32VideoWidth;
	m_pAvCodecContext->time_base.den = m_avTimeBase.den != 0 ? m_avTimeBase.den : m_avFrameRate.den;
	m_pAvCodecContext->time_base.num = m_avTimeBase.num != 0 ? m_avTimeBase.num : m_avFrameRate.num;

	// If m_nIFrmae = 0 what we want is no I frames, but setting gop_size to
	// 0 will give us only I frames so set it to a very large number.
	m_pAvCodecContext->gop_size = m_nIFrame != 0 ? m_nIFrame : 100 * m_avFrameRate.den / m_avFrameRate.num;
	m_pAvCodecContext->pix_fmt = PIX_FMT_YUV420P;

	// Check to see if the stream headers need to be separate.
	if(m_pAvFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		m_pAvCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	// Set the output parameters (required).
	nAvError = av_set_parameters(m_pAvFormatContext, NULL);
	stiTESTCOND(nAvError >= 0, stiRESULT_ERROR);
	
	// Debug info.
//	dump_format(m_pAvFormatContext, 0, ptszOutputFile, 1);
	
	// Open the video codec.
	if (m_pAvVideoStream &&
		m_pAvFrame == NULL)
	{
		pAvCodec = avcodec_find_encoder(m_pAvCodecContext->codec_id);
		stiTESTCOND(pAvCodec != NULL, stiRESULT_ERROR);
	
		nAvError = avcodec_open(m_pAvCodecContext, pAvCodec);
		stiTESTCOND(nAvError >= 0, stiRESULT_ERROR);
	
		// Make sure that the output buffer is large enough.
		m_nOutputBufferSize = m_n32VideoWidth * m_n32VideoHeight * 3;
		m_pun8OutputBuffer = new uint8_t[m_nOutputBufferSize];
		stiTESTCOND(m_pun8OutputBuffer != NULL, stiRESULT_MEMORY_ALLOC_ERROR);
	
		// Intialize the video frame.
		m_pAvFrame = avcodec_alloc_frame();
		stiTESTCOND(m_pAvFrame != NULL, stiRESULT_MEMORY_ALLOC_ERROR);
		tempWidth = m_n32VideoWidth + (int)(m_n32VideoWidth * 0.3f);
		frameSize = avpicture_get_size(PIX_FMT_YUV420P, 
									   tempWidth, 
									   m_n32VideoHeight);
	
		pFrameBuf = (uint8_t *)av_malloc(frameSize);
		stiTESTCOND(pFrameBuf != NULL, stiRESULT_MEMORY_ALLOC_ERROR);
		avpicture_fill((AVPicture *)m_pAvFrame, 
					   pFrameBuf,
					   PIX_FMT_YUV420P, 
					   tempWidth,
					   m_n32VideoHeight);
	}
	
#if 0
	nAvError  = url_fopen(&m_pAvFormatContext->pb, "/home/dfrye/Videos/ff-out.h264", URL_WRONLY);
	stiTESTCOND(nAvError == 0, stiRESULT_MEMORY_ALLOC_ERROR);

	nAvError = av_write_header(m_pAvFormatContext);
	stiTESTCOND(nAvError == 0, stiRESULT_MEMORY_ALLOC_ERROR);
#endif
	// ? I dont' think we need this.
	// Write the stream header
//	av_write_header(m_pAvFormatContext);
	
STI_BAIL:
	
	return hResult;
}


