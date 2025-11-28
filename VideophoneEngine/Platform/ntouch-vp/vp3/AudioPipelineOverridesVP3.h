/*!\brief Overrides for the VP3 audio pipeline variables
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include <string>

#if 1
#define REDIRECT_SPEC " 2>&1 >/dev/null ; "
#else
#define REDIRECT_SPEC " ; "
#endif

struct VP3Overrides
{
	int audioInputRate {48000};
    int headphoneSampleRate {48000};
    int bluetoothSameRate {16000};
    std::string micInputDevice {"hw:0,7"};
    std::string headsetInputDevice {"hw:0,4"};
    std::string defaultOutputDevice {"hw:0,5"};
    std::string headsetOutputDevice {"hw:0,4"};
	std::string speakerOutputDevice {"hw:0,4"};
    std::string filterType {"audio/x-raw"};
    int alawFormatId {2};
    int mulawFormatId {1};
    std::string audioType {"audio/x-raw"};
    std::string g722EncoderName {"avenc_g722"};
    std::string g722DecoderName {"avdec_g722"};
    std::string aacEncoderName {"avenc_aac"};
    std::string aacDecoderName {"avdec_aac"};
    std::string headphoneEnableString {
		"amixer -c0 cset name='Headphone Playback Switch' on"  REDIRECT_SPEC
		"amixer -c0 cset name='Headphone Playback Volume' 87"  REDIRECT_SPEC
		"amixer -c0 cset name='Speaker Playback Switch' off"   REDIRECT_SPEC
		"amixer -c0 cset name='Speaker Playback Volume' off"   REDIRECT_SPEC
		"amixer -c0 cset name='Master Playback Volume' 80"     REDIRECT_SPEC};
    std::string speakerEnableString {
		"amixer -c0 cset name='Headphone Playback Switch' off" REDIRECT_SPEC
		"amixer -c0 cset name='Headphone Playback Volume' off" REDIRECT_SPEC
		"amixer -c0 cset name='Speaker Playback Switch' on"    REDIRECT_SPEC
		"amixer -c0 cset name='Speaker Playback Volume' 87"    REDIRECT_SPEC
		"amixer -c0 cset name='Master Playback Volume' 81"     REDIRECT_SPEC};

	VP3Overrides ()
	{
	}
};

