///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	StscAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMCommon.h"
#include "SMTypes.h"
#include "SMMemory.h"

#include "StscAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; StscAtom_c::StscAtom_c
//
// Abstract:
//
// Returns:
//
StscAtom_c::StscAtom_c()
{
	m_unType = eAtom4CCstsc;

	m_unChunkIndex = 0;

	m_unCount = 0;
	m_pTable = nullptr;
	m_unAllocated = 0;

	m_unFoundIndex = ~0;

}


///////////////////////////////////////////////////////////////////////////////
//; StscAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t StscAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unCount = GetInt32(pDataHandler, un64Offset);

	ASSERTRT(m_unCount * sizeof(SampleToChunk_t) == un64Length - sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_pTable = (SampleToChunk_t *)GetDataPtr(pDataHandler, un64Offset + sizeof(uint32_t), m_unCount * sizeof(SampleToChunk_t));

	ASSERTRT(m_pTable != nullptr, SMRES_ALLOC_FAILED);
	
#ifdef SM_LITTLE_ENDIAN
	//
	// Byteswap all of the values.
	//
	unsigned int i;
	for (i = 0; i < m_unCount; i++)
	{
		m_pTable[i].unFirstChunk = DWORDBYTESWAP(m_pTable[i].unFirstChunk);
		m_pTable[i].unSamplesPerChunk = DWORDBYTESWAP(m_pTable[i].unSamplesPerChunk);
		m_pTable[i].unSampleDescrIndex = DWORDBYTESWAP(m_pTable[i].unSampleDescrIndex);
	}
#endif // SM_LITTLE_ENDIAN

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StscAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t StscAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += sizeof(uint32_t);

	unSize += m_unCount * sizeof(SampleToChunk_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; StscAtom_c::AddChunk
//
// Abstract:
//
// Returns:
//
SMResult_t StscAtom_c::AddChunk(
	uint32_t unSamplesPerChunk,
	uint32_t unSampleDescrIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	m_unChunkIndex++;

	if (m_unCount >= m_unAllocated)
	{
		m_pTable = (SampleToChunk_t *)SMRealloc(m_pTable, (m_unCount + 500) * sizeof(SampleToChunk_t));

		ASSERTRT(m_pTable != nullptr, SMRES_ALLOC_FAILED);

		m_unAllocated = m_unCount + 500;
	}

	if (m_unCount == 0)
	{
		m_pTable[0].unFirstChunk = m_unChunkIndex;
		m_pTable[0].unSamplesPerChunk = unSamplesPerChunk;
		m_pTable[0].unSampleDescrIndex = unSampleDescrIndex;

		m_unCount = 1;
	}
	else if (unSamplesPerChunk != m_pTable[m_unCount - 1].unSamplesPerChunk
	 || unSampleDescrIndex != m_pTable[m_unCount - 1].unSampleDescrIndex)
	{
		m_pTable[m_unCount].unFirstChunk = m_unChunkIndex;
		m_pTable[m_unCount].unSamplesPerChunk = unSamplesPerChunk;
		m_pTable[m_unCount].unSampleDescrIndex = unSampleDescrIndex;

		m_unCount++;
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StscAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void StscAtom_c::FreeAtom()
{
	if (m_pTable)
	{
		SMFreePtr(m_pTable);

		m_pTable = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; StscAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t StscAtom_c::Write(
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

	for (i = 0; i < m_unCount; i++)
	{
		SMResult = pDataHandler->DHWrite4(m_pTable[i].unFirstChunk);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_pTable[i].unSamplesPerChunk);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_pTable[i].unSampleDescrIndex);

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


