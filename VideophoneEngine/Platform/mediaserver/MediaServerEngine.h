/*!
* \file MediaServerEngine.h
* \brief See MediaServerEngine.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once
#include "IMediaServerEngine.h"
#include "CstiConferenceManager.h"

class MediaServerEngine :
	public IMediaServerEngine
{
public:
	MediaServerEngine(	PstiObjectCallback appCallback, 
						size_t callbackParam);
	~MediaServerEngine();
	virtual stiHResult initialize(	ProductType productType,
									const char *version,
									const SstiConferenceParams *conferenceParams,
									const SstiConferencePorts *conferencePorts);
	virtual stiHResult callObjectRemove(IstiCall * call);
	virtual int callObjectsCountGet();
	virtual stiHResult shutdown();
	virtual stiHResult reinvite (IstiCall * call);

private:
	PstiObjectCallback applicationCallback;
	size_t applicationCallbackParam;
	CstiConferenceManager* conferenceManager;
	CstiSignalConnection::Collection m_signalConnections;
};

