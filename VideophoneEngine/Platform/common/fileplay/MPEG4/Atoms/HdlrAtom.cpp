///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	HdlrAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <cstring>

//
// Sorenson includes.
//
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMCommon.h"

#include "HdlrAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::HdlrAtom_c
//
// Abstract:
//
// Returns:
//
HdlrAtom_c::HdlrAtom_c()
{
	m_unType = eAtom4CChdlr;

	m_unReserved1 = 0;
	m_unHandlerType = 0;
	memset(&m_unReserved2, 0, sizeof(m_unReserved2));
	m_pszName = nullptr;

} // HdrlAtom_c::HdlrAtom_c


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t HdlrAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= 2 * sizeof(uint32_t), SMRES_INVALID_MOVIE);

	//
	// Skip 32-bit reserved value.
	//

	//
	// Read hanlder type.
	//
	m_unHandlerType = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t));

	//
	// Skip twelve 8-bit reserved values.
	//

	//
	// Read the handler string.
	//
	m_pszName = (char *)GetDataPtr(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + 12 * sizeof(uint8_t),
								   (uint32_t)(un64Length - 2 * sizeof(uint32_t) - 12 * sizeof(uint8_t)));

	ASSERTRT(m_pszName != nullptr, SMRES_ALLOC_FAILED);

smbail:

	return SMResult;
} // HdlrAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t HdlrAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += 2 * sizeof(uint32_t) + 12 * sizeof(uint8_t) + strlen(m_pszName) + sizeof(uint8_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void HdlrAtom_c::FreeAtom()
{
	if (m_pszName != nullptr)
	{
		SMFreePtr(m_pszName);

		m_pszName = nullptr;
	}

	Atom_c::FreeAtom();
} // HdlrAtom_c::FreeAtom


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::SetHandlerType
//
// Abstract:
//
// Returns:
//
SMResult_t HdlrAtom_c::SetHandlerType(
	uint32_t unHandlerType,
	const char *pszName)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	m_unHandlerType = unHandlerType;

	if (m_pszName != nullptr)
	{
		SMFreePtr(m_pszName);

		m_pszName = nullptr;
	}

	if (pszName != nullptr)
	{
		m_pszName = (char *)SMAllocPtr(strlen(pszName) + 1);
		ASSERTRT(m_pszName != nullptr, SMRES_ALLOC_FAILED);

		memcpy(m_pszName, pszName, strlen(pszName) + 1);
	}

smbail:

	return SMResult;
} /// HdlrAtom_c::SetHandlerType


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::GetHandlerType
//
// Abstract:
//
// Returns:
//
SMResult_t HdlrAtom_c::GetHandlerType(
	uint32_t *punHandlerType)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	*punHandlerType = m_unHandlerType;

	return SMResult;
} // HdlrAtom_c::GetHandlerType


///////////////////////////////////////////////////////////////////////////////
//; HdlrAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t HdlrAtom_c::Write(
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

	SMResult = pDataHandler->DHWrite4(m_unHandlerType);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite(m_unReserved2, sizeof(m_unReserved2));

	TESTRESULT();

	if (m_pszName == nullptr)
	{
		SMResult = pDataHandler->DHWrite(&m_pszName, 1);

		TESTRESULT();
	}
	else
	{
		SMResult = pDataHandler->DHWrite(m_pszName, strlen(m_pszName) + 1);

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
} // HdlrAtom_c::Write


