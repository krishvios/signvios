// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "DinfAtom.h"
#include "SMCommon.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
DinfAtom_c::DinfAtom_c()
{
	m_unType = eAtom4CCdinf;
}


///////////////////////////////////////////////////////////////////////////////
//; DinfAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t DinfAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ParseMovie_r(pDataHandler, un64Offset, un64Length, this);

	TESTRESULT();

smbail:

	return SMResult;
}


