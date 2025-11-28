#ifndef CSTIBSPINTERFACE_H
#define CSTIBSPINTERFACE_H

#include "stiError.h"
#include "IstiPlatform.h"


class CstiBSPInterface : public IstiPlatform
{
public:

	CstiBSPInterface ();
	
	~CstiBSPInterface();
	
	virtual stiHResult Initialize ();
	
	virtual stiHResult Uninitialize ();
	
	virtual stiHResult RestartSystem ();
	
	stiHResult MacAddressWrite (const char *pszMac);
	
private:
	
	//CDisplayTask *m_pDisplayTask;

 	stiHResult DisplayTaskStartup ();
};


#endif // CSTIBSPINTERFACE_H
