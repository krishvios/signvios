// Uncomment this to turn off the use of FFMPEG
#define DISABLE_FFMPEG 1

#include "CstiPlatform.h"

static std::unique_ptr<CstiPlatform> g_platformInterface;

IstiPlatform *IstiPlatform::InstanceGet ()
{
	if (g_platformInterface == nullptr)
	{
		g_platformInterface = std::make_unique<CstiPlatform>();
	}

	return g_platformInterface.get();
}

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	return &g_platformInterface->videoOutput;
}

IstiAudioOutput *IstiAudioOutput::InstanceCreate (uint32_t nCallIndex)
{
	CstiAudioOutput* Instance_AudioOut = new CstiAudioOutput (nCallIndex);
	return Instance_AudioOut;
}

void IstiAudioOutput::InstanceDelete (IstiAudioOutput* audioOutput)
{
	delete(audioOutput);
}

IstiVideoInput *IstiVideoInput::InstanceGet ()
{
	return &g_platformInterface->videoInput;
}

IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	return &g_platformInterface->audioInput;
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return &g_platformInterface->audioOutput;
}

IstiNetwork* IstiNetwork::InstanceGet()
{
	return &g_platformInterface->network;
}

stiHResult CstiPlatform::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = videoInput.Initialize();
	stiTESTRESULT();

	hResult = videoOutput.Initialize();
	stiTESTRESULT();

STI_BAIL:

	return hResult;
}


stiHResult CstiPlatform::Uninitialize ()
{
	stiHResult hresult = stiRESULT_SUCCESS;

	g_platformInterface = nullptr;

	return stiRESULT_SUCCESS;
}

stiHResult CstiPlatform::RestartSystem (EstiRestartReason restartReason)
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiPlatform::RestartReasonGet (EstiRestartReason * pRestartReason)
{
	*pRestartReason = (EstiRestartReason::estiRESTART_REASON_UNKNOWN);
	return stiRESULT_SUCCESS;
}


void CstiPlatform::callStatusAdd (std::stringstream &callStatus)
{
	//This function is currently only called from CCI which is not used by the Media Server.
}

void CstiPlatform::ringStart()
{
	//This function is currently only called from CCI which is not used by the Media Server.
}

void CstiPlatform::ringStop()
{
	//This function is currently only called from CCI which is not used by the Media Server.
}

