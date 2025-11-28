#pragma once
#include "IstiVideoOutput.h"

class IPlatformVideoOutput : virtual public IstiVideoOutput
{
public:
	virtual void BeginRecordingFrames () = 0;

	virtual bool hasKeyFrameGet () = 0;

	virtual EstiVideoCodec VideoPlaybackCodecGet () = 0;

	virtual void RequestKeyFrame () = 0;

};