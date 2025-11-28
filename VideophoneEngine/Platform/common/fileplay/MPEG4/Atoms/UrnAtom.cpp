///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UrnAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <cstring>

#include "SMCommon.h"
#include "SorensonErrors.h"

#include "MP4Common.h"
#include "UrnAtom.h"


///////////////////////////////////////////////////////////////////////////////
//; UrnAtom_c::UrnAtom_c
//
// Abstract:
//
// Returns:
//
UrnAtom_c::UrnAtom_c()
{
	m_unType = eAtom4CCurn;

	m_pszName = nullptr;
	m_pszLocation = nullptr;

}


///////////////////////////////////////////////////////////////////////////////
//; UrnAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t UrnAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	char *pStrings = nullptr;
	uint32_t unNameLength;
	uint32_t unLocationLength;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length > 2, SMRES_INVALID_MOVIE);

	pStrings = GetDataPtr(pDataHandler, un64Offset, (uint32_t)un64Length);

	ASSERTRT(pStrings != nullptr, SMRES_ALLOC_FAILED);

	unNameLength = strlen(pStrings);
	unLocationLength = strlen(&pStrings[unNameLength + 1]);

	ASSERTRT(unNameLength + unLocationLength == un64Length, SMRES_INVALID_MOVIE);

	m_pszName = (char *)SMAllocPtr(unNameLength + 1);

	ASSERTRT(m_pszName != nullptr, SMRES_ALLOC_FAILED);

	memcpy(m_pszName, pStrings, unNameLength + 1);

	m_pszLocation = (char *)SMAllocPtr(unLocationLength + 1);

	ASSERTRT(m_pszLocation != nullptr, SMRES_ALLOC_FAILED);

	memcpy(m_pszLocation, &pStrings[unNameLength + 1], unLocationLength + 1);

smbail:

	if (pStrings)
	{
		SMFreePtr(pStrings);

		pStrings = nullptr;
	}
	
	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; UrnAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t UrnAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += strlen(m_pszName) + 1;

	unSize += strlen(m_pszLocation) + 1;
	
	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; UrnAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void UrnAtom_c::FreeAtom()
{
	if (m_pszName != nullptr)
	{
		SMFreePtr(m_pszName);

		m_pszName = nullptr;
	}

	if (m_pszLocation != nullptr)
	{
		SMFreePtr(m_pszLocation);

		m_pszLocation = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; UrnAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t UrnAtom_c::Write(
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

	SMResult = pDataHandler->DHWrite(m_pszName, strlen(m_pszName) + 1);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite(m_pszLocation, strlen(m_pszLocation) + 1);

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


