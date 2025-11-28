///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	ElstAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMCommon.h"

#include "ElstAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; ElstAtom_c::ElstAtom_c
//
// Abstract:
//
// Returns:
//
ElstAtom_c::ElstAtom_c()
{
	m_unType = eAtom4CCelst;

	m_unCount = 0;
	m_pEntries = nullptr;
}


///////////////////////////////////////////////////////////////////////////////
//; ElstAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t ElstAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int i;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unCount = GetInt32(pDataHandler, un64Offset);

	un64Offset += sizeof(uint32_t);
	un64Length -= sizeof(uint32_t);

	m_pEntries = (EditEntry_t *)SMAllocPtr(m_unCount * sizeof(EditEntry_t));

	ASSERTRT(m_pEntries != nullptr, SMRES_ALLOC_FAILED);
	
	switch (m_unVersion)
	{
		case 0:

			ASSERTRT(m_unCount * sizeof(uint32_t) * 3 == un64Length, SMRES_INVALID_MOVIE);

			for (i = 0; i < m_unCount; i++)
			{
				m_pEntries[i].n64Duration = GetInt32(pDataHandler, un64Offset + i * 3 * sizeof(int32_t));
				m_pEntries[i].n64MediaTime = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t) + i * 3 * sizeof(int32_t));
				m_pEntries[i].nMediaRate = GetInt16(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + i * 3 * sizeof(int32_t));
			}

			break;

		case 1:

			ASSERTRT(m_unCount * (sizeof(uint64_t) * 2 + sizeof(uint32_t)) == un64Length, SMRES_INVALID_MOVIE);
			 
			for (i = 0; i < m_unCount; i++)
			{
				m_pEntries[i].n64Duration = GetInt64(pDataHandler, un64Offset + i * (2 * sizeof(int64_t) + sizeof(int32_t)));
				m_pEntries[i].n64MediaTime = GetInt64(pDataHandler, un64Offset + sizeof(uint64_t) + i * (2 * sizeof(int64_t) + sizeof(int32_t)));
				m_pEntries[i].nMediaRate = GetInt16(pDataHandler, un64Offset + 2 * sizeof(uint64_t) + i * (2 * sizeof(int64_t) + sizeof(int32_t)));
			}

			break;

		default:

			ASSERTRT(eFalse, SMRES_INVALID_MOVIE);
	}
	
smbail:

	return SMResult;
}


void ElstAtom_c::FreeAtom()
{
	if (m_pEntries)
	{
		SMFreePtr(m_pEntries);

		m_pEntries = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; ElstAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t ElstAtom_c::GetSize()
{
	uint32_t unSize = 0;
	
	unSize += sizeof(uint32_t);

	switch (m_unVersion)
	{
		case 0:

			unSize += m_unCount * sizeof(uint32_t) * 3;

			break;

		case 1:

			unSize += m_unCount * (sizeof(uint32_t) + 2 * sizeof(uint64_t));

			break;
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; ElstAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t ElstAtom_c::Write(
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

	SMResult = pDataHandler->DHWrite4(m_unCount);

	TESTRESULT();

	switch (m_unVersion)
	{
		case 0:

			for (i = 0; i < m_unCount; i++)
			{
				SMResult = pDataHandler->DHWrite4((int32_t)m_pEntries[i].n64Duration);

				TESTRESULT();

				SMResult = pDataHandler->DHWrite4((int32_t)m_pEntries[i].n64MediaTime);

				TESTRESULT();

				SMResult = pDataHandler->DHWrite2((int16_t)m_pEntries[i].nMediaRate);

				TESTRESULT();

				SMResult = pDataHandler->DHWrite2(0);

				TESTRESULT();
			}

			break;

		case 1:

			for (i = 0; i < m_unCount; i++)
			{
				SMResult = pDataHandler->DHWrite8(m_pEntries[i].n64Duration);

				TESTRESULT();

				SMResult = pDataHandler->DHWrite8(m_pEntries[i].n64MediaTime);

				TESTRESULT();

				SMResult = pDataHandler->DHWrite2((int16_t)m_pEntries[i].nMediaRate);

				TESTRESULT();

				SMResult = pDataHandler->DHWrite2(0);

				TESTRESULT();
			}


			break;

		default:

			ASSERTRT(eFalse, SMRES_INVALID_MOVIE);
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


