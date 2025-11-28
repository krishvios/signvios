///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MdhdAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMCommon.h"

#include "MP4Common.h"
#include "MdhdAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; MdhdAtom_c::MdhdAtom_c
//
// Abstract:
//
// Returns:
//
MdhdAtom_c::MdhdAtom_c()
{
	m_unType = eAtom4CCmdhd;

	MP4GetCurrentTime(&m_un64CreationTime);
	
	m_un64ModificationTime = m_un64CreationTime;

	m_unTimeScale = 600;
	m_un64Duration = 0;

	m_usLanguage = ((('u' - 0x60) & 0x1F) << 10) | ((('n' - 0x60) & 0x1F) << 5) | (('d' - 0x60) & 0x1F);
	m_usReserved1 = 0;

}


///////////////////////////////////////////////////////////////////////////////
//; MdhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t MdhdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	switch (m_unVersion)
	{
		case 0:
		
			ASSERTRT(4 * sizeof(uint32_t) < un64Length, SMRES_INVALID_MOVIE);

			m_un64CreationTime = GetInt32(pDataHandler, un64Offset);
			m_un64ModificationTime = GetInt32(pDataHandler, un64Offset + 1 * sizeof(uint32_t));
			m_unTimeScale = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint32_t));
			m_un64Duration = GetInt32(pDataHandler, un64Offset + 3 * sizeof(uint32_t));

			un64Offset += 4 * sizeof(uint32_t);
			un64Length -= 4 * sizeof(uint32_t);

			break;
			
		case 1:
		
			ASSERTRT(sizeof(uint32_t) + 3 * sizeof(uint64_t) < un64Length, SMRES_INVALID_MOVIE);

			m_un64CreationTime = GetInt64(pDataHandler, un64Offset);
			m_un64ModificationTime = GetInt64(pDataHandler, un64Offset + sizeof(uint64_t));
			m_unTimeScale = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint64_t));
			m_un64Duration = GetInt64(pDataHandler, un64Offset + sizeof(uint32_t) + 2 * sizeof(uint64_t));
		
			un64Offset += sizeof(uint32_t) + 3 * sizeof(uint64_t);
			un64Length -= sizeof(uint32_t) + 3 * sizeof(uint64_t);

			break;
	
		default:
		
			ASSERTRT(eFalse, SMRES_INVALID_MOVIE);		
	}	

	ASSERTRT(2 * sizeof(uint16_t) == un64Length, SMRES_INVALID_MOVIE);

	m_usLanguage = GetInt16(pDataHandler, un64Offset);
	m_usReserved1 = GetInt16(pDataHandler, un64Offset + sizeof(uint16_t));

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MdhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t MdhdAtom_c::GetSize()
{
	uint32_t unSize = 0;

	switch (m_unVersion)
	{
		case 0:

			unSize += 4 * sizeof(uint32_t);

			break;

		case 1:

			unSize += sizeof(uint32_t) + 3 * sizeof(uint64_t);

			break;
	}

	unSize += 2 * sizeof(uint16_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; MdhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t MdhdAtom_c::Write(
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

	SMResult = pDataHandler->DHWrite4((uint32_t)m_un64CreationTime);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4((uint32_t)m_un64ModificationTime);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unTimeScale);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4((uint32_t)m_un64Duration);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite2(m_usLanguage & 0x7FFF);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite2(m_usReserved1);

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

