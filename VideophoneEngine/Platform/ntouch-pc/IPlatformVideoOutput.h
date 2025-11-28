#pragma once
#include "IstiVideoOutput.h"
#include "IVideoDisplayFrame.h"

class IPlatformVideoOutput
{
public:
	virtual void returnFrame (std::shared_ptr<IVideoDisplayFrame> frame) = 0;
	virtual void testCodecSet (EstiVideoCodec testCodec) = 0;
};