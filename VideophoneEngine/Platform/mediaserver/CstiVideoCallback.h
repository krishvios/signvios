#pragma once
#include "IstiVideoCallback.h"

class CstiVideoCallback
{
public:

	static void AssociateInput (uint32_t callIndex, std::shared_ptr<IPlatformVideoInput> videoInput);
	static void AssociateOutput (uint32_t callIndex, std::shared_ptr<IPlatformVideoOutput> videoOutput);
	static void DisassociateInput (uint32_t callIndex);
	static void DisassociateOutput (uint32_t callIndex);
	static void BeginWrite (uint32_t callIndex);
	static void EndWrite (const MSPacketContainer &videoContainer);
	static void SetRecordSettings (uint32_t callIndex, IstiVideoInput::SstiVideoRecordSettings* settings);
	static void SetCallback (IstiVideoCallback* callBack);
	static stiHResult StandardCallback (int32_t n32Message, size_t MessageParam, size_t CallbackParam, size_t CallbackFromId);

private:
	static std::unordered_map<uint32_t, std::shared_ptr<IPlatformVideoOutput>> m_videoOutputs;
	static std::unordered_map<uint32_t, std::shared_ptr<IPlatformVideoInput>> m_videoInputs;
	static IstiVideoCallback* m_videoCallBack;
	CstiVideoCallback () {};
};