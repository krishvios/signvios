#ifndef CSTIPLATFORM_H
#define CSTIPLATFORM_H

#include "stiError.h"
#include "BasePlatform.h"
#include "AppleVideoInput.h"
#include "AppleVideoOutput.h"
#include "AppleNetwork.h"
#include "AudioIO.h"

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
	
	vpe::AppleVideoInput videoInput;
	vpe::AppleVideoOutput videoOutput;
	vpe::AudioIO audioIO;
	vpe::AppleNetwork network;
};


#endif // CSTIPLATFORM_H
