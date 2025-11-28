/*!\brief Audio pipeline overrides with platform specific data (Platform)
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPipeline.h"


template <typename Platform>
class AudioPipelineOverrides : virtual public AudioPipeline
{
public:
    AudioPipelineOverrides () = default;
    ~AudioPipelineOverrides () override = default;

    AudioPipelineOverrides (const AudioPipelineOverrides &other) = delete;
	AudioPipelineOverrides (AudioPipelineOverrides &&other) = delete;
	AudioPipelineOverrides &operator= (const AudioPipelineOverrides &other) = delete;
	AudioPipelineOverrides &operator= (AudioPipelineOverrides &&other) = delete;  

    int audioInputRateGet () const override
    {
        return platform.audioInputRate;
    }
    
    int headphoneSampleRateGet () const override
    {
        return platform.headphoneSampleRate;
    }

    int bluetoothSampleRateGet() const override
    {
        return platform.bluetoothSameRate;
    }

    std::string micInputDeviceGet () const override
    {
        return platform.micInputDevice;
    }

    std::string headsetInputDeviceGet () const override
    {
        return platform.headsetInputDevice;
    }

    std::string defaultOutputDeviceGet () const override
    {
        return platform.defaultOutputDevice;
    }

    std::string headsetOutputDeviceGet () const override
    {
        return platform.headsetOutputDevice;
    }

    std::string speakerOutputDeviceGet () const override
    {
        return platform.speakerOutputDevice;
    }

    std::string filterTypeGet () const override
    {
        return platform.filterType;
    }

    int alawFormatIdGet () const override
    {
        return platform.alawFormatId;
    }

    int mulawFormatIdGet () const override
    {
        return platform.mulawFormatId;
    }

    std::string audioTypeGet () const override
    {
        return platform.audioType;
    }

    std::string g722EncoderNameGet () const override
    {
        return platform.g722EncoderName;
    }

    std::string g722DecoderNameGet () const override
    {
        return platform.g722DecoderName;
    }

    std::string aacEncoderNameGet () const override
    {
        return platform.aacEncoderName;
    }
    
    std::string aacDecoderNameGet () const override
    {
        return platform.aacDecoderName;
    }

    std::string headphoneEnableStringGet () const override
    {
        return platform.headphoneEnableString;
    }

    std::string speakerEnableStringGet () const override
    {
        return platform.speakerEnableString;
    }

private:
    Platform platform;
};

