//////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	FtypAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
//
// Sorenson includes.
//
#include "SMCommon.h"

#include "FtypAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; FtypAtom_c::FtypAtom_c
//
// Abstract:
//
// Returns:
//
FtypAtom_c::FtypAtom_c()
{
	m_unType = eAtom4CCftyp;

	m_punBrands = nullptr;
	m_unBrandLength = 0;

} // FtypAtom_c::FtypAtom_c


///////////////////////////////////////////////////////////////////////////////
//; FreeAtom
//
// Abstract:
//
// Returns:
//
void FtypAtom_c::FreeAtom()
{
	if (m_punBrands)
	{
		SMFreePtr(m_punBrands);

		m_punBrands = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; FtypAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t FtypAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int i;

	//
	// The list of brands must have at least two entries.
	//
	ASSERTRT(un64Length >= 2 * sizeof(uint32_t), SMRES_INVALID_MOVIE);

	ASSERTRT(un64Length <= 0xFFFFFFFF, -1);

	m_unBrandLength = (uint32_t)un64Length / sizeof(uint32_t);

	m_punBrands = (uint32_t *)SMAllocPtr(m_unBrandLength * sizeof(uint32_t));

	ASSERTRT(m_punBrands != nullptr, SMRES_ALLOC_FAILED);

	for (i = 0; i < m_unBrandLength; i++)
	{
		m_punBrands[i] = (uint32_t)GetInt32(pDataHandler, un64Offset);

		un64Offset += sizeof(uint32_t);
	}

smbail:

	return SMResult;
} // FtypAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; FtypAtom_c::GetBrands
//
// Abstract:
//
// Returns:
//
void FtypAtom_c::GetBrands(
	uint32_t **ppBrands,
	unsigned int *punBrandLength) const
{
	*ppBrands = m_punBrands;
	*punBrandLength = m_unBrandLength;
}


///////////////////////////////////////////////////////////////////////////////
//; FtypAtom_c::SetBrands
//
// Abstract:
//
// Returns:
//
SMResult_t FtypAtom_c::SetBrands(
	const uint32_t *pBrands,
	unsigned int unBrandLength)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (m_punBrands)
	{
		SMFreePtr(m_punBrands);
	}

	m_punBrands = (uint32_t *)SMAllocPtr(unBrandLength * sizeof(uint32_t));

	ASSERTRT(m_punBrands != nullptr, SMRES_ALLOC_FAILED);

	memcpy(m_punBrands, pBrands, unBrandLength * sizeof(uint32_t));

	m_unBrandLength = unBrandLength;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; FtypAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t FtypAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize = m_unBrandLength * sizeof(uint32_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; FtypAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t FtypAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	unsigned int i;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Write the atom header.
	//
	SMResult = WriteHeader(pDataHandler, unSize);

	TESTRESULT();

	for (i = 0; i < m_unBrandLength; i++)
	{
		SMResult = pDataHandler->DHWrite4(m_punBrands[i]);

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

