// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIPROFILER_H
#define CSTIPROFILER_H

#ifndef WIN32
#include <sys/time.h>
#endif

class CstiProfiler
{
public:

	CstiProfiler (
		int nMaxSamples,
		const char *pszMessage);
	CstiProfiler (const CstiProfiler &) = delete;
	CstiProfiler &operator= (const CstiProfiler &) = delete;

	~CstiProfiler ();
	
	void Start ();

	void Stop ();
	
	void Stop (
		const timeval &Start);
	
private:
	
	timeval m_Start{};
	int m_TotalSeconds;
	int m_TotalUSeconds;
	int m_nTotalSamples;
	int m_nMaxSamples;
	char *m_pszMessage;
};


#endif // CSTIPROFILER_H
