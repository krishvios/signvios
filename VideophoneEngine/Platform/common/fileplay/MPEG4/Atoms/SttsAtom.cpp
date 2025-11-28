///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	SttsAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMCommon.h"
#include "SMMemory.h"

#include "SorensonMP4.h"
#include "SttsAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; SttsAtom_c::SttsAtom_c
//
// Abstract:
//
// Returns:
//
SttsAtom_c::SttsAtom_c()
{
	m_unType = eAtom4CCstts;

	m_unCount = 0;
	m_punDeltaTable = nullptr;

	unLoopStart = 0;
	FoundTime = (~uint64_t(0)) >> 1;
	CurrentTime = 0;
	unSampleNumber = 0;

}


///////////////////////////////////////////////////////////////////////////////
//; SttsAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t SttsAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);
		
	m_unCount = GetInt32(pDataHandler, un64Offset);

	ASSERTRT(m_unCount * sizeof(DeltaEntry_t) == un64Length - sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_punDeltaTable = (DeltaEntry_t *)GetDataPtr(pDataHandler, un64Offset + sizeof(uint32_t), m_unCount * sizeof(DeltaEntry_t));

	ASSERTRT(m_punDeltaTable != nullptr, SMRES_ALLOC_FAILED);
	
#ifdef SM_LITTLE_ENDIAN
	//
	// Byteswap all of the values.
	//
	unsigned int i;
	for (i = 0; i < m_unCount; i++)
	{
		m_punDeltaTable[i].unEntryCount = DWORDBYTESWAP(m_punDeltaTable[i].unEntryCount);
		m_punDeltaTable[i].unDelta = DWORDBYTESWAP(m_punDeltaTable[i].unDelta);
	}
#endif // SM_LITTLE_ENDIAN

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; SttsAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t SttsAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += sizeof(uint32_t);

	unSize += m_unCount * sizeof(DeltaEntry_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; SttsAtom_c::AddSampleDelta
//
// Abstract:
//
// Returns:
//
SMResult_t SttsAtom_c::AddSampleDelta(
	uint32_t unDelta)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (m_punDeltaTable == nullptr)
	{
		m_punDeltaTable = (DeltaEntry_t *)SMAllocPtr(sizeof(DeltaEntry_t));

		ASSERTRT(m_punDeltaTable != nullptr, SMRES_ALLOC_FAILED);

		m_punDeltaTable[0].unEntryCount = 1;
		m_punDeltaTable[0].unDelta = unDelta;

		m_unCount = 1;
	}
	else
	{
		if (m_punDeltaTable[m_unCount - 1].unDelta == unDelta)
		{
			m_punDeltaTable[m_unCount - 1].unEntryCount++;
		}
		else
		{
			m_unCount++;

			m_punDeltaTable = (DeltaEntry_t *)SMRealloc(m_punDeltaTable, m_unCount * sizeof(DeltaEntry_t));

			ASSERTRT(m_punDeltaTable != nullptr, SMRES_ALLOC_FAILED);

			m_punDeltaTable[m_unCount - 1].unEntryCount = 1;
			m_punDeltaTable[m_unCount - 1].unDelta = unDelta;
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void SttsAtom_c::FreeAtom()
{
	if (m_punDeltaTable)
	{
		SMFreePtr(m_punDeltaTable);

		m_punDeltaTable = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; SttsAtom_c::GetDuration
//
// Abstract:
//
// Returns:
//
uint64_t SttsAtom_c::GetDuration() const
{
	uint64_t un64Duration = 0;
	uint32_t i;

	if  (m_punDeltaTable != nullptr)
	{
		for (i = 0; i < m_unCount; i++)
		{
			un64Duration += m_punDeltaTable[i].unEntryCount * m_punDeltaTable[i].unDelta;
		}
	}

	return un64Duration;
}


///////////////////////////////////////////////////////////////////////////////
//; SttsAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t SttsAtom_c::Write(
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
		SMResult = pDataHandler->DHWrite4(m_punDeltaTable[i].unEntryCount);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_punDeltaTable[i].unDelta);

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


