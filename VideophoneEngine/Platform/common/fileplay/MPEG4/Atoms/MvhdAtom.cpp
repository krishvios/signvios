///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MvhdAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

#include <cstring>

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMCommon.h"
#include "SMTypes.h"

#include "MP4Common.h"
#include "MvhdAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; MvhdAtom_c
//
// Abstract:
//
// Returns:
//
MvhdAtom_c::MvhdAtom_c()
{
	m_unType = eAtom4CCmvhd;

	MP4GetCurrentTime(&m_un64CreationTime);
	
	m_un64ModificationTime = m_un64CreationTime;

	m_unTimeScale = 600;
	m_un64Duration = 0;

	m_unReserved1 = 0x00010000;
	m_usReserved2 = 0x0100;
	m_usReserved3 = 0;
	memset(m_unReserved4, 0, sizeof(m_unReserved4));

	m_unReserved5[0] = 0x00010000;
	m_unReserved5[1] = 0;
	m_unReserved5[2] = 0;
	m_unReserved5[3] = 0;
	m_unReserved5[4] = 0x00010000;
	m_unReserved5[5] = 0;
	m_unReserved5[6] = 0;
	m_unReserved5[7] = 0;
	m_unReserved5[8] = 0x40000000;

	memset(&m_unReserved6, 0, sizeof(m_unReserved6));

	m_unNextTrackID = 1;

}


///////////////////////////////////////////////////////////////////////////////
//; MvhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t MvhdAtom_c::Parse(
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
		
			ASSERTRT(5 * sizeof(uint32_t) < un64Length, SMRES_INVALID_MOVIE);

			m_un64CreationTime = GetInt32(pDataHandler, un64Offset);
			m_un64CreationTime = m_un64CreationTime & 0x00000000FFFFFFFF;

			m_un64ModificationTime = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t));
			m_un64ModificationTime = m_un64ModificationTime & 0x00000000FFFFFFFF;

			m_unTimeScale = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint32_t));

			m_un64Duration = GetInt32(pDataHandler, un64Offset + 3 * sizeof(uint32_t));
			m_un64Duration = m_un64Duration & 0x00000000FFFFFFFF;

			un64Offset += 4 * sizeof(uint32_t);
			un64Length -= 4 * sizeof(uint32_t);

			break;
			
		case 1:
		
			ASSERTRT(2 * sizeof(uint32_t) + 3 * sizeof(uint64_t) < un64Length, SMRES_INVALID_MOVIE);

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

	ASSERTRT(19 * sizeof(uint32_t) + 2 * sizeof(uint16_t) == un64Length, SMRES_INVALID_MOVIE);

	m_unReserved1 = GetInt32(pDataHandler, un64Offset);
	m_usReserved2 = GetInt16(pDataHandler, un64Offset + sizeof(uint32_t));
	m_usReserved3 = GetInt16(pDataHandler, un64Offset + sizeof(uint32_t) + sizeof(uint16_t));
	m_unReserved4[0] = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved4[1] = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + 2 * sizeof(uint16_t));

	m_unReserved5[0] = GetInt32(pDataHandler, un64Offset + 3 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[1] = GetInt32(pDataHandler, un64Offset + 4 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[2] = GetInt32(pDataHandler, un64Offset + 5 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[3] = GetInt32(pDataHandler, un64Offset + 6 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[4] = GetInt32(pDataHandler, un64Offset + 7 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[5] = GetInt32(pDataHandler, un64Offset + 8 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[6] = GetInt32(pDataHandler, un64Offset + 9 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[7] = GetInt32(pDataHandler, un64Offset + 10 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved5[8] = GetInt32(pDataHandler, un64Offset + 11 * sizeof(uint32_t) + 2 * sizeof(uint16_t));

	m_unReserved6[0] = GetInt32(pDataHandler, un64Offset + 12 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved6[1] = GetInt32(pDataHandler, un64Offset + 13 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved6[2] = GetInt32(pDataHandler, un64Offset + 14 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved6[3] = GetInt32(pDataHandler, un64Offset + 15 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved6[4] = GetInt32(pDataHandler, un64Offset + 16 * sizeof(uint32_t) + 2 * sizeof(uint16_t));
	m_unReserved6[5] = GetInt32(pDataHandler, un64Offset + 17 * sizeof(uint32_t) + 2 * sizeof(uint16_t));

	m_unNextTrackID = GetInt32(pDataHandler, un64Offset + 18 * sizeof(uint32_t) + 2 * sizeof(uint16_t));

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MvhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t MvhdAtom_c::GetSize()
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

	unSize += 19 * sizeof(uint32_t) + 2 * sizeof(uint16_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; MvhdAtom_c::GetNextTrackID
//
// Abstract:
//
// Returns:
//
uint32_t MvhdAtom_c::GetNextTrackID()
{
	return m_unNextTrackID++;
}


///////////////////////////////////////////////////////////////////////////////
//; MvhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t MvhdAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	int i;

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

	SMResult = pDataHandler->DHWrite4(m_unReserved1);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite2(m_usReserved2);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite2(m_usReserved3);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unReserved4[0]);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unReserved4[1]);

	TESTRESULT();

	for (i = 0; i < 9; i++)
	{
		SMResult = pDataHandler->DHWrite4(m_unReserved5[i]);

		TESTRESULT();
	}

	for (i = 0; i < 6; i++)
	{
		SMResult = pDataHandler->DHWrite4(m_unReserved6[i]);

		TESTRESULT();
	}

	SMResult = pDataHandler->DHWrite4(m_unNextTrackID);

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

