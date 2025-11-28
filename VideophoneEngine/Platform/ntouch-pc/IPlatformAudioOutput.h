#pragma once

class IPlatformAudioOutput
{
public:
	virtual void outputSettingsSet (int channels, int sampleRate, int sampleFormat, int alignment) = 0;
};

