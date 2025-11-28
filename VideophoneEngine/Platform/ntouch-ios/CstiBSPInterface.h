#ifndef CSTIBSPINTERFACE_H
#define CSTIBSPINTERFACE_H

#include "stiError.h"
#include "IstiPlatform.h"


class CstiPlatform : public IstiPlatform
{
public:

	CstiPlatform ();
	
	~CstiPlatform();
	
	virtual stiHResult Initialize ();
	
	virtual stiHResult Uninitialize ();
	
	virtual stiHResult RestartSystem ();
	
private:
	
	//CDisplayTask *m_pDisplayTask;

 	stiHResult DisplayTaskStartup ();
};


#endif // CSTIBSPINTERFACE_H
