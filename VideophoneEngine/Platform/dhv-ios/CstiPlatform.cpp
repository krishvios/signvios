// Uncomment this to turn off the use of FFMPEG
#define DISABLE_FFMPEG 1

#include "CstiPlatform.h"

static CstiPlatform platformInterface;

IstiPlatform *IstiPlatform::InstanceGet ()
{
	return &platformInterface;
}

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	return &platformInterface.videoOutput;
}

IstiVideoInput *IstiVideoInput::InstanceGet ()
{
	return &platformInterface.videoInput;
}

IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	return &platformInterface.audioIO;
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return &platformInterface.audioIO;
}

IstiNetwork *IstiNetwork::InstanceGet ()
{
	return &platformInterface.network;
}

stiHResult CstiPlatform::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
#ifndef NO_AUDIO_WAVELLO
	hResult = audioIO.Initialize();
#endif
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}


stiHResult CstiPlatform::Uninitialize ()
{
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
