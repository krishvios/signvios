///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UrlAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <cstring>

#include "SMCommon.h"
#include "SorensonErrors.h"

#include "MP4Common.h"
#include "UrlAtom.h"


///////////////////////////////////////////////////////////////////////////////
//; UrlAtom_c::UrlAtom_c
//
// Abstract:
//
// Returns:
//
UrlAtom_c::UrlAtom_c()
{
	m_unType = eAtom4CCurl;

	m_pszLocation = nullptr;

}


///////////////////////////////////////////////////////////////////////////////
//; UrlAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t UrlAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	if ((m_unFlags & 0x01) == 0)
	{
		ASSERTRT(un64Length > 1, SMRES_INVALID_MOVIE);

		m_pszLocation = GetDataPtr(pDataHandler, un64Offset, (uint32_t)un64Length);

		ASSERTRT(m_pszLocation != nullptr, SMRES_ALLOC_FAILED);

	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; UrlAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t UrlAtom_c::GetSize()
{
	uint32_t unSize = 0;

	if ((m_unFlags & 0x01) == 0)
	{
		unSize += strlen(m_pszLocation) + 1;
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; UrlAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void UrlAtom_c::FreeAtom()
{
	if (m_pszLocation != nullptr)
	{
		SMFreePtr(m_pszLocation);

		m_pszLocation = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; UrlAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t UrlAtom_c::Write(
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

	if ((m_unFlags & 0x01) == 0)
	{
		SMResult = pDataHandler->DHWrite(m_pszLocation, strlen(m_pszLocation) + 1);

		TESTRESULT();
	}

	//
	// Get end position.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);

smbail:

	return SMResult;
}


