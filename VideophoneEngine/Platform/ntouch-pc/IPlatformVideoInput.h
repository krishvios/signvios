#pragma once
#include "IstiVideoOutput.h"
#include "IVideoDisplayFrame.h"

class IPlatformVideoInput
{
public:
	virtual void SendVideoFrame (UINT8* pBytes, UINT32 nWidth, UINT32 nHeight, EstiColorSpace eColorSpace) = 0;

	virtual void SendVideoFrame (UINT8* pBytes, INT32 nDataLength, UINT32 nWidth, UINT32 nHeight, EstiVideoCodec eCodec) = 0;

	virtual stiHResult testCodecSet (EstiVideoCodec codec) = 0;

	virtual stiHResult CaptureCapabilitiesChanged () = 0;

	virtual bool IsRecording () = 0;

	virtual bool AllowsFUPackets () = 0;
};