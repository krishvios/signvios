///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	VmhdAtom.cpp
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

#include "VmhdAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; VmhdAtom_c::VmhdAtom_c
//
// Abstract:
//
// Returns:
//
VmhdAtom_c::VmhdAtom_c()
{
	m_unType = eAtom4CCvmhd;

	m_un64Reserved1 = 0;

	m_unFlags = 1;
}



///////////////////////////////////////////////////////////////////////////////
//; VmhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t VmhdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	//
	// Reserved 64-bit value.
	//

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; VmhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t VmhdAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += sizeof(uint64_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; VmhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t VmhdAtom_c::Write(
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
	SMResult = pDataHandler->DHWrite8(m_un64Reserved1);

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



