/*!\brief Overrides for the VP4 audio pipeline variables
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include <string>
#include "AudioPipelineUtility.h"

#if 1
#define REDIRECT_SPEC " 2>&1 >/dev/null ; "
#else
#define REDIRECT_SPEC " ; "
#endif

#define MIC_CARD	"default"
#define SPKR_CARD	"default"
#define HDMI_CARD	"audiohdmi"

struct X86Overrides
{
	int audioInputRate {48000};
    int headphoneSampleRate {48000};
    int bluetoothSameRate {16000};
    std::string micInputDevice {};
    std::string headsetInputDevice {};
    std::string defaultOutputDevice {};
    std::string headsetOutputDevice {};
	std::string speakerOutputDevice {};
    std::string filterType {"audio/x-raw"};
    int alawFormatId {2};
    int mulawFormatId {1};
    std::string audioType {"audio/x-raw"};
    std::string g722EncoderName {"avenc_g722"};
    std::string g722DecoderName {"avdec_g722"};
    std::string aacEncoderName {"avenc_aac"};
    std::string aacDecoderName {"avdec_aac"};
    std::string headphoneEnableString {};
    std::string speakerEnableString {
		"amixer -c " SPKR_CARD " cset name='Lineout Playback Switch' on"	REDIRECT_SPEC
		"amixer -c " SPKR_CARD " cset name='Lineout Playback Volume' 31,31"	REDIRECT_SPEC};

	X86Overrides ()
	{
		using namespace APUtility;

		micInputDevice = hardwareIdGet (Direction::Record, MIC_CARD);
		//headsetInputDevice = hardwareIdGet (Direction::Record, "");
    	defaultOutputDevice = hardwareIdGet (Direction::Play, HDMI_CARD);
    	//headsetOutputDevice = hardwareIdGet (Direction::Play, "");
		speakerOutputDevice = hardwareIdGet (Direction::Play, SPKR_CARD);
	}
};
