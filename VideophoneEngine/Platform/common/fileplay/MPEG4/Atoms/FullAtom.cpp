///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	FullAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

#include "SMCommon.h"
#include "SorensonErrors.h"

#include "MP4Common.h"
#include "FullAtom.h"



///////////////////////////////////////////////////////////////////////////////
//; FullAtom_c::FullAtom_c
//
// Abstract:
//
// Returns:
//
FullAtom_c::FullAtom_c()
{
	m_unVersion = 0;
	m_unFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////
//; FullAtom_c::GetHeaderSize
//
// Abstract:
//
// Returns:
//
uint32_t FullAtom_c::GetHeaderSize(
	uint64_t un64Size)
{
	uint32_t unHeaderSize;

	//
	// Add in amount for flags and version.
	//
	un64Size += sizeof(uint32_t);

	unHeaderSize = Atom_c::GetHeaderSize(un64Size) + sizeof(uint32_t);

	return unHeaderSize;
}


///////////////////////////////////////////////////////////////////////////////
//; FullAtom_c::ReadHeader
//
// Abstract:
//
// Returns:
//
SMResult_t FullAtom_c::ReadHeader(
	CDataHandler* pDataHandler,
	uint64_t *pun64Offset,
	uint64_t *pun64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(*pun64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unFlags = GetInt32(pDataHandler, *pun64Offset);

	m_unVersion = (m_unFlags >> 24);
	m_unFlags &= 0x00FFFFFF;

	*pun64Offset += sizeof(uint32_t);
	*pun64Length -= sizeof(uint32_t);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; FullAtom_c::WriteHeader
//
// Abstract:
//
// Returns:
//
SMResult_t FullAtom_c::WriteHeader(
	CDataHandler* pDataHandler,
	uint64_t un64Size)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	Atom_c::WriteHeader(pDataHandler, un64Size);

	//
	// Write version and flags.
	//
	SMResult = pDataHandler->DHWrite4((m_unVersion << 24) | (m_unFlags & 0x00FFFFFF));

	TESTRESULT();

smbail:

	return SMResult;
}


