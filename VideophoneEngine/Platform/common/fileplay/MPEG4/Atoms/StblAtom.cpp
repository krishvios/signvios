// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

//
// Sorenson includes
//
#include "SMCommon.h"
#include "SorensonErrors.h"

#include "StblAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; StblAtom_c::StblAtom_c
//
// Abstract:
//
// Returns:
//
StblAtom_c::StblAtom_c()
{
	m_unType = eAtom4CCstbl;
}


///////////////////////////////////////////////////////////////////////////////
//; StblAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t StblAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ParseMovie_r(pDataHandler, un64Offset, un64Length, this);

	TESTRESULT();

	m_pStsc = (StscAtom_c *)GetChildAtom(eAtom4CCstsc);
	m_pStco = (StcoAtom_c *)GetChildAtom(eAtom4CCstco);
	m_pStsz = (StszAtom_c *)GetChildAtom(eAtom4CCstsz);
	m_pStts = (SttsAtom_c *)GetChildAtom(eAtom4CCstts);

smbail:

	return SMResult;
}


