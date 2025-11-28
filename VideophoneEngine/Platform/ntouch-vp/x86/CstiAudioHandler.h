#pragma once

#include "stiSVX.h"
#include "CstiAudioHandlerBase.h"
#include "AudioRecordPipelineX86.h"
#include "AudioPlaybackPipelineX86.h"
#include "AudioPipelineOverridesX86.h"


class CstiAudioHandler : public CstiAudioHandlerBase<
	AudioRecordPipelineX86,
	AudioPlaybackPipelineX86,
	AudioFileSourcePipelinePlatform<X86Overrides>,
	TestAudioInputPipelinePlatform<X86Overrides>>
{
public:

	CstiAudioHandler ();
	~CstiAudioHandler () override = default;

	CstiAudioHandler (const CstiAudioHandler &other) = delete;
	CstiAudioHandler (CstiAudioHandler &&other) = delete;
	CstiAudioHandler &operator= (const CstiAudioHandler &other) = delete;
	CstiAudioHandler &operator= (CstiAudioHandler &&other) = delete;

protected:

private:

};
