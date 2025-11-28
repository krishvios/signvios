#pragma once

class IPlatformAudioInput
{
public:
	virtual void PacketDeliver (short *ipBuffer, unsigned int inLen) = 0;
};

