#pragma once
#include "SstiMSPacket.h"
#include "MSPacketContainer.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "IstiVideoInput.h"
#include "IPlatformVideoInput.h"
#include "IPlatformVideoOutput.h"
#include <unordered_map>

class IstiVideoCallback
{
	public:
		virtual void AssociateInput(uint32_t, IPlatformVideoInput*) = 0;
		virtual void AssociateOutput(uint32_t, IPlatformVideoOutput*) = 0;
		virtual void BeginWrite(uint32_t) = 0;
		virtual void EndWrite(const MSPacketContainer &videoContainer) = 0;
		virtual void SetRecordSettings(uint32_t, IstiVideoInput::SstiVideoRecordSettings*) = 0;
		virtual stiHResult StandardCallback(int32_t n32Message, size_t MessageParam, size_t CallbackParam, size_t CallbackFromId) = 0;
};




