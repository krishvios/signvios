#ifndef CSTIPLATFORM_H
#define CSTIPLATFORM_H

#include "stiError.h"
#include "BasePlatform.h"
#include <CstiVideoOutput.h>
#include <CstiVideoInput.h>
#include <CstiAudioInput.h>
#include <CstiAudioOutput.h>

class CstiPlatform : public BasePlatform
{
public:
	virtual stiHResult Initialize () override;
	
	virtual stiHResult Uninitialize () override;
		
	virtual stiHResult RestartSystem (EstiRestartReason restartReason) override;

	virtual stiHResult RestartReasonGet (EstiRestartReason * pRestartReason) override;

	virtual void callStatusAdd (std::stringstream &callStatus) override {};

	virtual void ringStart () override {};

	virtual void ringStop () override {};

	CstiVideoInput videoInput;
	CstiVideoOutput videoOutput;
	CstiAudioInput audioInput;
	CstiAudioOutput audioOutput;
};


#endif // CSTIPLATFORM_H
