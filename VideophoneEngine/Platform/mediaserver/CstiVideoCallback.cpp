#include "CstiVideoCallback.h"

IstiVideoCallback* CstiVideoCallback::m_videoCallBack = NULL;

std::unordered_map<uint32_t, std::shared_ptr<IPlatformVideoOutput>> CstiVideoCallback::m_videoOutputs;
std::unordered_map<uint32_t, std::shared_ptr<IPlatformVideoInput>> CstiVideoCallback::m_videoInputs;

void CstiVideoCallback::AssociateInput(uint32_t callIndex, std::shared_ptr<IPlatformVideoInput> videoInput)
{
	m_videoInputs[callIndex] = videoInput;
	m_videoCallBack->AssociateInput(callIndex, videoInput.get ());
}

void CstiVideoCallback::AssociateOutput(uint32_t callIndex, std::shared_ptr<IPlatformVideoOutput> videoOutput)
{
	m_videoOutputs[callIndex] = videoOutput;
	m_videoCallBack->AssociateOutput(callIndex, videoOutput.get());
}


void CstiVideoCallback::DisassociateInput (uint32_t callIndex)
{
	m_videoInputs.erase (callIndex);
}

void CstiVideoCallback::DisassociateOutput (uint32_t callIndex)
{
	m_videoOutputs.erase (callIndex);
}

void CstiVideoCallback::BeginWrite(uint32_t callIndex)
{
	m_videoCallBack->BeginWrite(callIndex);
}

void CstiVideoCallback::EndWrite(const MSPacketContainer &videoContainer)
{
	m_videoCallBack->EndWrite(videoContainer);
}

void CstiVideoCallback::SetRecordSettings(uint32_t callIndex, IstiVideoInput::SstiVideoRecordSettings* settings)
{
	m_videoCallBack->SetRecordSettings(callIndex, settings);
}

void CstiVideoCallback::SetCallback(IstiVideoCallback* callBack)
{
	m_videoCallBack = callBack;
}

stiHResult CstiVideoCallback::StandardCallback(int32_t n32Message, size_t MessageParam, size_t CallbackParam, size_t CallbackFromId)
{
	return m_videoCallBack->StandardCallback(n32Message, MessageParam, CallbackParam, CallbackFromId);
}

