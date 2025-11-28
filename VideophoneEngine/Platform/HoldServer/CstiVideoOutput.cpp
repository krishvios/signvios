#include "CstiVideoOutput.h"
#include "stiError.h"


stiHResult CstiVideoOutput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}
	

stiHResult CstiVideoOutput::CallbackRegister (
	PstiAppGenericCallback pfnCallback,
	size_t CallbackParam)
{

	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiVideoOutput::CallbackUnregister (
	PstiAppGenericCallback pfnCallback,
	size_t CallbackParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


void CstiVideoOutput::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
}


stiHResult CstiVideoOutput::DisplaySettingsSet (
	const SstiDisplaySettings * pDisplaySettings)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::RemoteViewPrivacySet (
	EstiBool bPrivacy)
{
	return stiRESULT_SUCCESS;
}

	
stiHResult CstiVideoOutput::RemoteViewHoldSet (
	EstiBool bHold)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::VideoPlaybackSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}
	

stiHResult CstiVideoOutput::VideoPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


stiHResult CstiVideoOutput::VideoPlaybackPacketPut (
	SsmdVideoPacket * pVideoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}
