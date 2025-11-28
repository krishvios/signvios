//*****************************************************************************
//
// FileName:    FFAudioDecoder.cpp
//
// Abstract:    Implementation of the FFAudioDecoder class which handles:
//				- Opening and decoding audio data
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "FFAudioDecoder.h"

using namespace vpe;
using namespace vpe::Audio;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FFAudioDecoder::FFAudioDecoder ()
{
	avcodec_register_all ();
}

FFAudioDecoder::~FFAudioDecoder ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////
//  FFAudioDecoder::close
//
//  Description: Cleans up allocated objects.
//
//  Abstract:
//
//  Returns:
//		stiRESULT_SUCCESS
//

stiHResult FFAudioDecoder::close ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_audioCodec = EstiAudioCodec::estiAUDIO_NONE;

	if (m_codecContext)
	{
		avcodec_free_context (&m_codecContext);
	}

	if (m_resampleContext)
	{
		swr_free (&m_resampleContext);
	}

	return hResult;
}

stiHResult FFAudioDecoder::init (EstiAudioCodec audioCodec, int channels, int sampleRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int ffmpegError = 0;

	hResult = close ();
	stiTESTRESULT ();

	switch (audioCodec)
	{
		case EstiAudioCodec::estiAUDIO_G711_MULAW:
			m_codec = avcodec_find_decoder (AV_CODEC_ID_PCM_MULAW);
			break;
		case EstiAudioCodec::estiAUDIO_G711_ALAW:
			m_codec = avcodec_find_decoder (AV_CODEC_ID_PCM_ALAW);
			break;
		default:
			stiTHROW (stiRESULT_INVALID_CODEC);
	}

	stiTESTCOND (m_codec != nullptr, stiRESULT_ERROR);

	m_codecContext = avcodec_alloc_context3 (m_codec);
	stiTESTCOND (m_codecContext != nullptr, stiRESULT_ERROR);

	m_codecContext->channels = channels;
	m_codecContext->sample_rate = sampleRate;
	m_codecContext->channel_layout = channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;


	ffmpegError = avcodec_open2 (m_codecContext, m_codec, nullptr);
	stiTESTCOND (ffmpegError >= 0, stiRESULT_ERROR);

	m_audioCodec = audioCodec;

	m_resampleContext = swr_alloc_set_opts (m_resampleContext,
		m_outputChannelLayout,
		m_outputSampleFormat,
		m_outputSampleRate,
		m_codecContext->channel_layout,
		(AVSampleFormat)m_codec->sample_fmts[0],
		m_codecContext->sample_rate,
		0,
		NULL);

	ffmpegError = swr_init (m_resampleContext);
	stiTESTCOND (ffmpegError >= 0, stiRESULT_ERROR);

STI_BAIL:


	if (stiIS_ERROR (hResult))
	{
		char errorBuffer[128];
		if (av_strerror (ffmpegError, errorBuffer, sizeof(errorBuffer)) > 0)
		{
			stiTrace ("FFAudioDecoder: %s\n", errorBuffer);
		}
	}
	return hResult;
}

int32_t FFAudioDecoder::decode (Packet  &source, Packet  *destination, BufferStatus *flags)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	AVFrame *decodedFrame = av_frame_alloc ();
	AVFrame *outFrame = av_frame_alloc ();
	int gotFrame = 0;
	int outputSamples = 0;
	int bytesDecoded = -1;
	AVPacket packet = { 0 };
	int result = -1;
	constexpr int AV_RESULT_SUCCESS = 0;

	stiTESTCOND_NOLOG (m_outputChannelLayout, stiRESULT_ERROR);
	stiTESTCOND_NOLOG (m_outputSampleFormat, stiRESULT_ERROR);
	stiTESTCOND_NOLOG (m_outputSampleRate, stiRESULT_ERROR);
	stiTESTCOND_NOLOG (m_outputAlignment, stiRESULT_ERROR);

	outFrame->channel_layout = m_outputChannelLayout;
	outFrame->format = m_outputSampleFormat;
	outFrame->sample_rate = m_outputSampleRate;
		
	stiTESTCOND_NOLOG (decodedFrame != NULL, stiRESULT_ERROR);
	av_init_packet (&packet);
	packet.data = source.data;
	packet.size = source.dataLength;

	result = avcodec_send_packet (m_codecContext, &packet);
	stiTESTCOND_NOLOG (AV_RESULT_SUCCESS == result, stiRESULT_ERROR);

	result = avcodec_receive_frame (m_codecContext, decodedFrame);
	stiTESTCOND_NOLOG (AV_RESULT_SUCCESS == result, stiRESULT_ERROR);

	bytesDecoded = decodedFrame->pkt_size;

	gotFrame = swr_convert_frame (m_resampleContext, outFrame, decodedFrame);
	stiTESTCOND_NOLOG (AV_RESULT_SUCCESS == gotFrame, stiRESULT_ERROR);
	destination->maxLength = outFrame->linesize[0];
	destination->dataLength = av_samples_get_buffer_size (NULL, outFrame->channels, outFrame->nb_samples, m_outputSampleFormat, m_outputAlignment);
	destination->data = new uint8_t[destination->maxLength];
	destination->sampleRate = outFrame->sample_rate;
	destination->sampleCount = outFrame->nb_samples;
	destination->channels = outFrame->channels;

	memcpy (destination->data, outFrame->data[0], destination->dataLength);

	*flags = BufferStatus::FrameComplete;

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		char errorBuffer[128];
		if (av_strerror (result, errorBuffer, sizeof(errorBuffer)) > 0)
		{
			stiTrace ("FFAudioDecoder: %s\n", errorBuffer);
		}
		bytesDecoded = -1;
	}

	if (outFrame)
	{
		av_frame_free (&outFrame);
	}

	if (decodedFrame)
	{
		av_frame_free (&decodedFrame);
	}
	return bytesDecoded;
}

EstiAudioCodec FFAudioDecoder::codecGet ()
{
	return m_audioCodec;
}

void FFAudioDecoder::outputSettingsSet (int channels, int sampleRate, int sampleFormat, int alignment)
{
	int ffmpegError = 0;
	m_outputChannelLayout = channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	m_outputSampleRate = sampleRate;
	m_outputSampleFormat = sampleFormat == 16 ? AVSampleFormat::AV_SAMPLE_FMT_S16 : AVSampleFormat::AV_SAMPLE_FMT_FLT;
	m_outputAlignment = alignment;
	if (m_resampleContext)
	{
		swr_close (m_resampleContext);

		m_resampleContext = swr_alloc_set_opts (m_resampleContext,
			m_outputChannelLayout,
			m_outputSampleFormat,
			m_outputSampleRate,
			m_codecContext->channel_layout,
			(AVSampleFormat)m_codec->sample_fmts[0],
			m_codecContext->sample_rate,
			0,
			nullptr);

		ffmpegError = swr_init (m_resampleContext);
	}
}