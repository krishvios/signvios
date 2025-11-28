#ifndef CSTIPLATFORM_H
#define CSTIPLATFORM_H

#include "stiError.h"
#include "BasePlatform.h"
#include "CstiVideoOutput.h"
#include "IstiVideoInput.h"
#include "CstiAudioInput.h"
#include "CstiAudioOutput.h"
#include "CstiVideoInput.h"
#include "Network.h"

class CstiPlatform : public BasePlatform
{
public:
	stiHResult Initialize () override;

	stiHResult Uninitialize () override;
	
	stiHResult RestartSystem (EstiRestartReason restartReason) override;

	stiHResult RestartReasonGet (EstiRestartReason * pRestartReason) override;

	void callStatusAdd (std::stringstream &callStatus) override;

	void ringStart() override;

	void ringStop() override;

	CstiAudioInput audioInput;
	CstiAudioOutput audioOutput;
	CstiVideoInput videoInput;
	CstiVideoOutput videoOutput;
	Network network;
};


#endif // CSTIPLATFORM_H
