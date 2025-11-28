///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	HmhdAtom.cpp
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

#include "HmhdAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; HmhdAtom_c::HmhdAtom_c
//
// Abstract:
//
// Returns:
//
HmhdAtom_c::HmhdAtom_c()
{
	m_unType = eAtom4CChmhd;

	m_usMaxPDUSize = 0;
	m_usAvgPDUSize = 0;
	m_unMaxBitRate = 0;
	m_unAvgBitRate = 0;
	m_unSlidingAvgBitRate = 0;
}


///////////////////////////////////////////////////////////////////////////////
//; HmhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t HmhdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(3 * sizeof(uint32_t) + 2 * sizeof(uint16_t) == un64Length, SMRES_INVALID_MOVIE);

	m_usMaxPDUSize = GetInt16(pDataHandler, un64Offset);
	m_usAvgPDUSize = GetInt16(pDataHandler, un64Offset + sizeof(uint16_t));
	m_unMaxBitRate = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint16_t));
	m_unAvgBitRate = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unSlidingAvgBitRate = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + 2 * sizeof(uint16_t));

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; HmhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t HmhdAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += 3 * sizeof(uint32_t) + 2 * sizeof(uint16_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; HmhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t HmhdAtom_c::Write(
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
	// Write PDU sizes and bit rates.
	//
	SMResult = pDataHandler->DHWrite2(m_usMaxPDUSize);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite2(m_usAvgPDUSize);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unMaxBitRate);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unAvgBitRate);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unSlidingAvgBitRate);

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


