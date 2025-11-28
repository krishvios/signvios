#pragma once

#include "stiSVX.h"
#include "AudioControl.h"
#include "AudioStructures.h"

namespace vpe
{
	class IAudioDecoder
	{
	public:

		virtual stiHResult init (EstiAudioCodec audioCodec, int channels, int sampleRate) = 0;
		virtual stiHResult close () = 0;
		virtual int32_t decode (Audio::Packet &source, Audio::Packet  *destination, Audio::BufferStatus *flags) = 0;
		virtual EstiAudioCodec codecGet () = 0;
		virtual void outputSettingsSet (int channels, int sampleRate, int sampleFormat, int alignment) = 0;
	};
}