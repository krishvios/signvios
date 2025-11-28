//
// Sorenson includes
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "SMCommon.h"
#include "SorensonErrors.h"

#include "MdiaAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; MdiaAtom_c::MdiaAtom_c
//
// Abstract:
//
// Returns:
//
MdiaAtom_c::MdiaAtom_c()
{
	m_unType = eAtom4CCmdia;
	m_pDataHandler = nullptr;

	m_unSampleDescrIndex = ~0;

	m_unSampleIndex = ~0;
	m_unSampleSize = 0;
	m_unLastSampleInChunk = ~0;
	m_unChunkIndex = ~0;
	m_unLastChunk = ~0;
	m_unSampleToChunkIndex = ~0;
	m_un64SampleOffset = ~0;
}


///////////////////////////////////////////////////////////////////////////////
//; MdiaAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t MdiaAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	m_pDataHandler = pDataHandler;

	SMResult = ParseMovie_r(pDataHandler, un64Offset, un64Length, this);

	TESTRESULT();

	m_pMdhd = (MdhdAtom_c *)GetChildAtom(eAtom4CCmdhd);
	m_pMinf = (MinfAtom_c *)GetChildAtom(eAtom4CCminf);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MdiaAtom_c::SetDataHandler
//
// Abstract:
//
// Returns:
//
SMResult_t MdiaAtom_c::SetDataHandler(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	m_pDataHandler = pDataHandler;

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MdiaAtom_c::CloseDataHandler
//
// Abstract:
//
// Returns:
//
void MdiaAtom_c::CloseDataHandler()
{
	Atom_c::CloseDataHandler();
}


