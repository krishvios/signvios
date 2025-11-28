#ifndef CSTIPLATFORMINTERFACE_H
#define CSTIPLATFORMINTERFACE_H

#include "stiError.h"
#include "BasePlatform.h"
#include "CstiVideoOutput.h"
#include "CstiAudioInput.h"
#include "CstiAudioOutput.h"
#include "CstiVideoInput.h"
#include "CstiAudibleRinger.h"
#include "CstiNetwork.h"
#include "ICameraInformation.h"
#include "CFilePlay.h"


class CstiPlatformInterface : public BasePlatform, public ICameraInformation
{
public:	
	stiHResult Initialize () override;	
	stiHResult CstiPlatformInterface::Uninitialize () override;
	stiHResult RestartSystem (EstiRestartReason eRestartReason) override;
	stiHResult RestartReasonGet (EstiRestartReason *peRestartReason) override;
	void callStatusAdd (std::stringstream & callStatus) override;

	void cameraInformationSet(const CameraInformation& cameraInfo) override;

	CstiAudioInput audioInput;
	CstiAudioOutput audioOutput;
	CstiVideoInput videoInput;
	CstiVideoOutput videoOutput;
	CstiAudibleRinger audibleRinger;
	CstiNetwork network;
	CFilePlay filePlay;

	// Inherited via IstiPlatform
	void ringStart () override;
	void ringStop () override;

private:
	CameraInformation m_currentCameraInfo{};
};


#endif // CSTIPLATFORMINTERFACE_H
