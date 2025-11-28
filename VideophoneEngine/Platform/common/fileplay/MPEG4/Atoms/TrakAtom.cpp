///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	TrakAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
//
// Sorenson includes.
//
#include "SMCommon.h"

#include "MoovAtom.h"
#include "TrakAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; TrackAtom_c::TrakAtom_c
//
// Abstract:
//
// Returns:
//
TrakAtom_c::TrakAtom_c()
{
	m_unType = eAtom4CCtrak;
} // TrakAtom_c::TrakAtom_c

///////////////////////////////////////////////////////////////////////////////
//; TrakAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t TrakAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ParseMovie_r(pDataHandler, un64Offset, un64Length, this);

	TESTRESULT();

	m_pMoov = (MoovAtom_c *)GetParent();
	m_pElst = (ElstAtom_c *)GetChildAtom(eAtom4CCelst);

smbail:

	return SMResult;
} // TrakAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; TrakAtom_c::GetMovie
//
// Abstract:
//
// Returns:
//
Movie_t *TrakAtom_c::GetMovie() const
{
	MoovAtom_c *pMoovAtom;

	pMoovAtom = (MoovAtom_c *)m_pParent;

	return pMoovAtom->GetMovie();
} // TrakAtom_c::GetMovie

