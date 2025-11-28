///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	NmhdAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMCommon.h"
#include "SorensonErrors.h"
#include "SMTypes.h"

#include "NmhdAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; NmhdAtom_c::NmhdAtom_c
//
// Abstract:
//
// Returns:
//
NmhdAtom_c::NmhdAtom_c()
{
	m_unType = eAtom4CCnmhd;

}


///////////////////////////////////////////////////////////////////////////////
//; NmhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t NmhdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(sizeof(uint32_t) == un64Length, SMRES_INVALID_MOVIE);

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; NmhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t NmhdAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; NmhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t NmhdAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Write the atom header.
	//
	SMResult = WriteHeader(pDataHandler, unSize);

	TESTRESULT();

	//
	// Get end position.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);

smbail:

	return SMResult;
}


