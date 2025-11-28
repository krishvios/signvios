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

struct VP2Overrides
{
	int audioInputRate {16000};
    int headphoneSampleRate {16000};
    int bluetoothSameRate {48000};
    std::string micInputDevice {"hw:0,4"};
    std::string headsetInputDevice {"hw:0,0"};
    std::string defaultOutputDevice {"hw:0,1"};
    std::string headsetOutputDevice {"hw:0,0"};
	std::string speakerOutputDevice {"hw:0,0"};
    std::string filterType {"audio/x-raw-int"};
    int alawFormatId {3};
    int mulawFormatId {2};
    std::string audioType {"audio/x-raw-int"};
    std::string g722EncoderName {"ffenc_g722"};
    std::string g722DecoderName {"ffdec_g722"};
    std::string aacEncoderName {"ffenc_aac"};
    std::string aacDecoderName {"ffdec_aac"};
    std::string headphoneEnableString {
		"amixer -c 0 cset name='Line Out Switch' off"                 REDIRECT_SPEC
		"amixer -c 0 cset name='Line Out Volume' off"                 REDIRECT_SPEC
		"amixer -c 0 cset name='Speaker Switch' off"                  REDIRECT_SPEC
		"amixer -c 0 cset name='Speaker Volume' off"                  REDIRECT_SPEC
		"amixer -c 0 cset name='Headphone Switch' on"                 REDIRECT_SPEC
		"amixer -c 0 cset name='Headphone Volume' 60"                 REDIRECT_SPEC
		"amixer -c 0 cset name='Left Speaker Mixer DACL Switch' off"  REDIRECT_SPEC
		"amixer -c 0 cset name='Left Speaker Mixer DACR Switch' off"  REDIRECT_SPEC
		"amixer -c 0 cset name='Right Speaker Mixer DACL Switch' off" REDIRECT_SPEC
		"amixer -c 0 cset name='Right Speaker Mixer DACR Switch' off" REDIRECT_SPEC
		"amixer -c 0 cset name='Left Output Mixer DACL Switch' on"    REDIRECT_SPEC
		"amixer -c 0 cset name='Left Output Mixer DACR Switch' off"   REDIRECT_SPEC
		"amixer -c 0 cset name='Right Output Mixer DACL Switch' off"  REDIRECT_SPEC
		"amixer -c 0 cset name='Right Output Mixer DACR Switch' on"   REDIRECT_SPEC};
    std::string speakerEnableString {
		"amixer -c 0 cset name='Line Out Switch' off"                 REDIRECT_SPEC
		"amixer -c 0 cset name='Line Out Volume' off"                 REDIRECT_SPEC
		"amixer -c 0 cset name='Headphone Switch' off"                REDIRECT_SPEC
		"amixer -c 0 cset name='Headphone Volume' off"                REDIRECT_SPEC
		"amixer -c 0 cset name='Speaker Switch' on"                   REDIRECT_SPEC
		"amixer -c 0 cset name='Speaker Volume' 44"                   REDIRECT_SPEC
		"amixer -c 0 cset name='Left Speaker Mixer DACL Switch' on"   REDIRECT_SPEC
		"amixer -c 0 cset name='Left Speaker Mixer DACR Switch' off"  REDIRECT_SPEC
		"amixer -c 0 cset name='Right Speaker Mixer DACL Switch' off" REDIRECT_SPEC
		"amixer -c 0 cset name='Right Speaker Mixer DACR Switch' on"  REDIRECT_SPEC
		"amixer -c 0 cset name='Left Output Mixer DACL Switch' off"   REDIRECT_SPEC
		"amixer -c 0 cset name='Left Output Mixer DACR Switch' off"   REDIRECT_SPEC
		"amixer -c 0 cset name='Right Output Mixer DACL Switch' off"  REDIRECT_SPEC
		"amixer -c 0 cset name='Right Output Mixer DACR Switch' off"  REDIRECT_SPEC};

	VP2Overrides ()
	{
	}
};

