#ifndef CSTIPLATFORM_H
#define CSTIPLATFORM_H

#include "stiError.h"
#include "BasePlatform.h"
#include "Network.h"

class CstiPlatform : public BasePlatform
{
public:

	CstiPlatform ();
	
	~CstiPlatform();

	CstiSignal<EstiRestartReason> platformRestartSignal;

	stiHResult Initialize (int nMaxCalls);

	virtual stiHResult Initialize ();
	
	virtual stiHResult Uninitialize ();
	
	virtual stiHResult RestartSystem ();

	virtual stiHResult RestartSystem(
		EstiRestartReason eRestartReason);

	virtual stiHResult RestartReasonGet(
		EstiRestartReason *peRestartReason);

	stiHResult MacAddressWrite (const char *pszMac);

	virtual void callStatusAdd(
		std::stringstream &callStatus);

	Network network;

//	static IstiBSPInterface *IstiBSPInterface::InstanceGet ();
private:
	
	//CDisplayTask *m_pDisplayTask;

 	stiHResult DisplayTaskStartup ();
	
	// Provide stub for pure virtuals
	virtual void ringStart () {};
	virtual void ringStop () {};
};

#endif // CSTIPLATFORM_H
