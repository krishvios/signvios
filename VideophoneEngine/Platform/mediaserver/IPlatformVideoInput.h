#pragma once
#include "IstiVideoPacket.h"

class IPlatformVideoInput : public virtual IstiVideoInput
{
public:
	virtual bool NextFrame (Common::VideoPacket * packet) = 0;

	virtual EstiVideoCodec VideoRecordCodecGet () = 0;


};