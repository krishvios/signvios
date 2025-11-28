#ifndef CSTIBSPINTERFACE_H
#define CSTIBSPINTERFACE_H

#include "stiError.h"
#include "BasePlatform.h"
#include "CstiNetwork.h"
#include "CstiVideoInput.h"
#include "CstiSoftwareDisplay.h"
#include "CFilePlay.h"

class CstiBSPInterface : public BasePlatform
{
public:

	CstiBSPInterface ();
	
	~CstiBSPInterface();
	
	virtual stiHResult Initialize () override;
	
	virtual stiHResult Uninitialize () override;
	
	virtual stiHResult RestartSystem (
		EstiRestartReason eRestartReason) override;

	virtual stiHResult RestartReasonGet (
			EstiRestartReason *peRestartReason) override;

	stiHResult MacAddressWrite (const char *pszMac);

	virtual void geolocationUpdate (
    								bool viable,
    								double latitude,
    								double longitude,
    								int uncertainty,
    								bool altitudeViable,
    								int altitude) override;

    virtual void geolocationClear () override;
	virtual void callStatusAdd (
			std::stringstream &callStatus) override;

	// Inherited via IstiPlatform
	void ringStart () override;
	void ringStop () override;
	static CstiVideoInput *VideoInputGet();
	static CstiSoftwareDisplay *VideoOutputGet();

private:
	
	//CDisplayTask *m_pDisplayTask;

 	stiHResult DisplayTaskStartup ();
};


#endif // CSTIBSPINTERFACE_H
