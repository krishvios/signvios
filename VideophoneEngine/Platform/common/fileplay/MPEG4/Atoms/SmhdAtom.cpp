///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	SmhdAtom.cpp
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

#include "SmhdAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; SmhdAtom_c::SmhdAtom_c
//
// Abstract:
//
// Returns:
//
SmhdAtom_c::SmhdAtom_c()
{
	m_unType = eAtom4CCsmhd;

	m_unReserved1 = 0;

}


///////////////////////////////////////////////////////////////////////////////
//; SmhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t SmhdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	//
	// Reserved 32-bit value
	//
		
smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; SmhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t SmhdAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += sizeof(uint32_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; SmhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t SmhdAtom_c::Write(
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
	// Write atom data.
	//
	SMResult = pDataHandler->DHWrite4(m_unReserved1);

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


