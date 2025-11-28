// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIBSPINTERFACE_H
#define CSTIBSPINTERFACE_H

#include "stiError.h"
#include "BasePlatform.h"
#include "CstiSignal.h"


class CstiBSPInterface : public BasePlatform
{
public:

	CstiBSPInterface ();
	
	~CstiBSPInterface();
	
	virtual stiHResult Initialize ();
	
	virtual stiHResult Uninitialize ();
	
	virtual stiHResult RestartSystem (
		EstiRestartReason eRestartReason);
	
	virtual stiHResult RestartReasonGet (
		EstiRestartReason *peRestartReason);
	
	virtual void callStatusAdd (
		std::stringstream &callStatus) {};

	virtual void ringStart () {};
	
	virtual void ringStop () {};

	virtual ISignal<const std::string&>& geolocationChangedSignalGet ()
	{
		return geolocationChangedSignal;
	}

	virtual ISignal<>& geolocationClearSignalGet ()
	{
		return geolocationClearSignal;
	}


	virtual ISignal<int>& hdmiInStatusChangedSignalGet ()
	{
		return hdmiInStatusChangedSignal;
	}


	virtual ISignal<const PlatformCallStateChange &>& callStateChangedSignalGet ()
	{
		return callStateChangedSignal;
	}



private:
	
	//CDisplayTask *m_pDisplayTask;

	stiHResult DisplayTaskStartup ();
	
	CstiSignal <const std::string&> geolocationChangedSignal;
	CstiSignal <> geolocationClearSignal;
	CstiSignal <int> hdmiInStatusChangedSignal;
	CstiSignal <const PlatformCallStateChange &> callStateChangedSignal;
};


#endif // CSTIBSPINTERFACE_H
