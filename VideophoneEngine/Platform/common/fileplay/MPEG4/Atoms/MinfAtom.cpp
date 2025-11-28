// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "MinfAtom.h"
#include "SMCommon.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; MinfAtom_c::MinfAtom_c
//
// Abstract:
//
// Returns:
//
MinfAtom_c::MinfAtom_c()
{
	m_unType = eAtom4CCminf;
}


///////////////////////////////////////////////////////////////////////////////
//; MinfAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t MinfAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ParseMovie_r(pDataHandler, un64Offset, un64Length, this);

	TESTRESULT();

	m_pStbl = (StblAtom_c *)GetChildAtom(eAtom4CCstbl);

smbail:

	return SMResult;
}


