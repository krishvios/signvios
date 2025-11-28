//*****************************************************************************
//
// FileName:     FFAudioDecoder.h
//
// Abstract:     Declairation of the CstiFFMpegDecoder class used to decode
//				 audio.
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#pragma once

#include "IAudioDecoder.h"
#include "stiError.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

#include "CstiFFMpegContext.h"
namespace vpe
{
	class FFAudioDecoder : public IAudioDecoder
	{
	public:
		FFAudioDecoder ();
		~FFAudioDecoder ();

		stiHResult init (EstiAudioCodec audioCodec, int channels, int sampleRate) override;
		stiHResult close () override;
		int32_t decode (Audio::Packet  &source, Audio::Packet  *destination, Audio::BufferStatus *flags) override;
		EstiAudioCodec codecGet () override;
		void outputSettingsSet (int channels, int sampleRate, int sampleFormat, int alignment) override;

	private:
		AVCodec *m_codec = nullptr;
		AVCodecContext *m_codecContext = nullptr;
		SwrContext *m_resampleContext = nullptr;
		EstiAudioCodec m_audioCodec = EstiAudioCodec::estiAUDIO_NONE;

		int m_outputChannelLayout = 0;
		int m_outputSampleRate = 0;
		AVSampleFormat m_outputSampleFormat = AVSampleFormat::AV_SAMPLE_FMT_NONE;
		int m_outputAlignment = 0;

	};
}