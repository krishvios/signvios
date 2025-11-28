#ifndef CSTIBSPINTERFACE_H
#define CSTIBSPINTERFACE_H

#include "stiError.h"
#include "BasePlatform.h"
#include "PropertyManager.h"


class CstiBSPInterface : public BasePlatform
{
public:

	CstiBSPInterface ();
	
	~CstiBSPInterface();
	
	virtual stiHResult Initialize () override;
	
	virtual stiHResult Uninitialize () override;
	
	virtual stiHResult RestartSystem (EstiRestartReason eRestartReason) override;
	virtual stiHResult RestartReasonGet (EstiRestartReason *peRestartReason) override;
	
	void callStatusAdd (std::stringstream & callStatus) override;
	
	stiHResult MacAddressWrite (const char *pszMac);
    
    virtual void ringStart () override;
    
    virtual void ringStop () override;
	
#if APPLICATION == APP_NTOUCH_IOS
	
	virtual void geolocationUpdate (
									bool viable,
									double latitude,
									double longitude,
									int uncertainty,
									bool altitudeViable,
									int altitude) override;
	
	virtual void geolocationClear () override;
	
#endif

private:
	
	//CDisplayTask *m_pDisplayTask;
	
	// Callback function for calls to CCI from child threads.
	static stiHResult ThreadCallback(
									 int32_t n32Message, 	///< holds the type of message
									 size_t MessageParam,	///< holds data specific to the message
									 size_t CallbackParam,	///< points to the instantiated CCI object
									 size_t CallbackFromId);
	
	EstiResult ErrorReport (
							const SstiErrorLog *pstErrorLog);

 	stiHResult DisplayTaskStartup ();
};


#endif // CSTIBSPINTERFACE_H
