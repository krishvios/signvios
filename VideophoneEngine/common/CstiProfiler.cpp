// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiProfiler.h"
#include "stiTools.h"



CstiProfiler::CstiProfiler (
	int nMaxSamples,
	const char *pszMessage)
:
	m_TotalSeconds (0),
	m_TotalUSeconds (0),
	m_nTotalSamples (0),
	m_nMaxSamples (nMaxSamples)
{
	m_pszMessage = nullptr;
	
	if (pszMessage)
	{
		m_pszMessage = new char [strlen (pszMessage) + 1];
		strcpy (m_pszMessage, pszMessage);
	}
}


CstiProfiler::~CstiProfiler()
{
	if (m_pszMessage)
	{
		delete [] m_pszMessage;
		m_pszMessage = nullptr;
	}
}


void CstiProfiler::Start ()
{
	gettimeofday (&m_Start, nullptr);
}


void CstiProfiler::Stop ()
{
	Stop (m_Start);
}


void CstiProfiler::Stop(
	const timeval &Start)
{
	timeval End{};
	timeval Diff{};
	
	gettimeofday (&End, nullptr);

	timevalSubtract (&Diff, &End, &Start);

	m_TotalSeconds += Diff.tv_sec;
	m_TotalUSeconds += Diff.tv_usec;
	++m_nTotalSamples;

	if (m_nTotalSamples > m_nMaxSamples)
	{
		///\todo: still need to consider seconds
		stiTrace ("%s = %d\n", m_pszMessage, m_TotalUSeconds / m_nTotalSamples);
		
		m_TotalSeconds = 0;
		m_TotalUSeconds = 0;
		m_nTotalSamples = 0;
	}
}
