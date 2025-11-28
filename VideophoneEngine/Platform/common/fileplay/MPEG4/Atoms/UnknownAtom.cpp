///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UnknownAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

#include "SMCommon.h"
#include "SorensonErrors.h"

#include "MP4Common.h"
#include "UnknownAtom.h"


///////////////////////////////////////////////////////////////////////////////
//; UnknownAtom_c::UnknownAtom_c
//
// Abstract:
//
// Returns:
//
UnknownAtom_c::UnknownAtom_c()
{
	m_pucData = nullptr;
	m_unDataLength = 0;
}


///////////////////////////////////////////////////////////////////////////////
//; UnknownAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t UnknownAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (un64Length > 0)
	{
		ASSERTRT(un64Length <= 0xFFFFFFFF, SMRES_UNSUPPORTED_SIZE);

		m_pucData = (uint8_t *)GetDataPtr(pDataHandler, un64Offset, (uint32_t)un64Length);

		ASSERTRT(m_pucData != nullptr, SMRES_ALLOC_FAILED);

		m_unDataLength = (uint32_t)un64Length;
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; UnknownAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t UnknownAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += m_unDataLength;

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; UnknownAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void UnknownAtom_c::FreeAtom()
{
	if (m_pucData != nullptr)
	{
		SMFreePtr(m_pucData);

		m_pucData = nullptr;
		m_unDataLength = 0;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; UnknownAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t UnknownAtom_c::Write(
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

	SMResult = pDataHandler->DHWrite(m_pucData, m_unDataLength);

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


