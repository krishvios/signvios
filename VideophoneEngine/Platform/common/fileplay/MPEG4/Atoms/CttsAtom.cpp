///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	CttsAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMCommon.h"
#include "SMMemory.h"

#include "CttsAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
CttsAtom_c::CttsAtom_c()
{
	m_unType = eAtom4CCctts;

	m_unCount = 0;
	m_punDeltaTable = nullptr;
}



///////////////////////////////////////////////////////////////////////////////
//; CttsAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t CttsAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unCount = GetInt32(pDataHandler, un64Offset);

	//
	// Make sure the table isn't larger than what is left to read.
	//
	ASSERTRT(m_unCount * sizeof(CttsEntry_t) == un64Length - sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_punDeltaTable = (CttsEntry_t *)GetDataPtr(pDataHandler, un64Offset + sizeof(uint32_t), m_unCount * sizeof(CttsEntry_t));

	ASSERTRT(m_punDeltaTable != nullptr, SMRES_ALLOC_FAILED);
	
#ifdef SM_LITTLE_ENDIAN
	//
	// Byteswap all of the values.
	//
	uint32_t i;
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
//; CttsAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t CttsAtom_c::GetSize()
{
	uint32_t unSize = 0;

	if (m_unCount > 1)
	{
		unSize += m_unCount * sizeof(CttsEntry_t) + sizeof(uint32_t);

		unSize += GetHeaderSize(unSize);
	}

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; CttsAtom_c::AddSampleData
//
// Abstract:
//
// Returns:
//
SMResult_t CttsAtom_c::AddSampleDelta(
	uint32_t unDelta)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (m_punDeltaTable == nullptr)
	{
		m_punDeltaTable = (CttsEntry_t *)SMAllocPtr(sizeof(CttsEntry_t));

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

			m_punDeltaTable = (CttsEntry_t *)SMRealloc(m_punDeltaTable, m_unCount * sizeof(CttsEntry_t));

			ASSERTRT(m_punDeltaTable != nullptr, SMRES_ALLOC_FAILED);

			m_punDeltaTable[m_unCount - 1].unEntryCount = 1;
			m_punDeltaTable[m_unCount - 1].unDelta = unDelta;
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CttsAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void CttsAtom_c::FreeAtom()
{
	if (m_punDeltaTable)
	{
		SMFreePtr(m_punDeltaTable);

		m_punDeltaTable = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; CttsAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t CttsAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	unsigned int i;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Only write the atom when there is useful information to write.  If there is one or no entries
	// then do not write the atom.
	//
	if (m_unCount > 1)
	{
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

