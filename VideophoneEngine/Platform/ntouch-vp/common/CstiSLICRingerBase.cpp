// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#include "CstiSLICRingerBase.h"


CstiSLICRingerBase::CstiSLICRingerBase ()
	:
	CstiEventQueue ("CstiSLICRinger")
{
}


CstiSLICRingerBase::~CstiSLICRingerBase ()
{
	CstiEventQueue::StopEventLoop ();
}


stiHResult CstiSLICRingerBase::start()
{
	PostEvent ([this]{eventRingerStart ();});
	return stiRESULT_SUCCESS;
}


stiHResult CstiSLICRingerBase::stop()
{
	PostEvent ([this]{eventRingerStop ();});
	return stiRESULT_SUCCESS;
}


void CstiSLICRingerBase::deviceDetect ()
{
	PostEvent ([this]{eventDeviceDetect ();});
}


stiHResult CstiSLICRingerBase::startup ()
{
	CstiEventQueue::StartEventLoop ();
	return stiRESULT_SUCCESS;
}
