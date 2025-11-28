///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	StszAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMCommon.h"
#include "SMMemory.h"

#include "StszAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; StszAtom_c::StszAtom_c
//
// Abstract:
//
// Returns:
//
StszAtom_c::StszAtom_c()
{
	m_unType = eAtom4CCstsz;

	m_unDefaultSize = 0;
	m_unSampleCount = 0;
	m_punSizeTable = nullptr;
	m_unAllocated = 0;

}


///////////////////////////////////////////////////////////////////////////////
//; StszAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t StszAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= 2 * sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unDefaultSize = GetInt32(pDataHandler, un64Offset);
	m_unSampleCount = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t));

	if (m_unDefaultSize == 0)
	{
		ASSERTRT(m_unSampleCount * sizeof(uint32_t) == un64Length - 2 * sizeof(uint32_t), SMRES_INVALID_MOVIE);

		m_punSizeTable = (uint32_t *)GetDataPtr(pDataHandler, un64Offset + 2 * sizeof(uint32_t), m_unSampleCount * sizeof(uint32_t));

		ASSERTRT(m_punSizeTable != nullptr, SMRES_ALLOC_FAILED);
		
#ifdef SM_LITTLE_ENDIAN
		//
		// Byteswap all of the values.
		//
		unsigned int i;
		for (i = 0; i < m_unSampleCount; i++)
		{
			m_punSizeTable[i] = DWORDBYTESWAP(m_punSizeTable[i]);
		}
#endif // SM_LITTLE_ENDIAN

	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StszAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t StszAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += 2 * sizeof(uint32_t);

	if (m_unDefaultSize == 0)
	{
		unSize += m_unSampleCount * sizeof(uint32_t);
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; StszAtom_c::GetSampleCount
//
// Abstract:
//
// Returns:
//
SMResult_t StszAtom_c::GetSampleCount(
	uint32_t *punSampleCount) const
{
	SMResult_t SMResult = SMRES_SUCCESS;

	*punSampleCount = m_unSampleCount;

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t StszAtom_c::GetDefaultSize(
	uint32_t *punDefaultSize) const 
{
	SMResult_t SMResult = SMRES_SUCCESS;

	*punDefaultSize = m_unDefaultSize;

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t StszAtom_c::GetSampleSize(
	uint32_t unSampleIndex,
	uint32_t *punSampleSize) const
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (m_unDefaultSize == 0)
	{
		*punSampleSize = m_punSizeTable[unSampleIndex - 1];
	}
	else
	{
		*punSampleSize = m_unDefaultSize;
	}

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t StszAtom_c::AddSampleSize(
	uint32_t unSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int i;

	m_unSampleCount++;

	if (m_unSampleCount == 1)
	{
		m_unDefaultSize = unSize;
	}
	else
	{
		if (unSize != m_unDefaultSize)
		{
			//
			// Check to see if the table needs to grow.
			//
			if (m_unSampleCount > m_unAllocated)
			{
				//
				// Need to grow the table.
				//
				m_punSizeTable = (uint32_t *)SMRealloc(m_punSizeTable, (m_unSampleCount + 500) * sizeof(uint32_t));

				ASSERTRT(m_punSizeTable != nullptr, SMRES_ALLOC_FAILED);

				m_unAllocated = m_unSampleCount + 500;

				//
				// Populate the table with past entries if this is the first time
				// the table was allocated.
				//
				if (m_unDefaultSize != 0)
				{
					for (i = 0; i < m_unSampleCount - 1; i++)
					{
						m_punSizeTable[i] = m_unDefaultSize;
					}

					m_unDefaultSize = 0;
				}
			}

			m_punSizeTable[m_unSampleCount - 1] = unSize;
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; FreeAtom
//
// Abstract:
//
// Returns:
//
void StszAtom_c::FreeAtom()
{
	if (m_punSizeTable)
	{
		SMFreePtr(m_punSizeTable);

		m_punSizeTable = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t StszAtom_c::Write(
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

	SMResult = pDataHandler->DHWrite4(m_unDefaultSize);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unSampleCount);

	TESTRESULT();

	if (m_unDefaultSize == 0)
	{
		for (i = 0; i < m_unSampleCount; i++)
		{
			SMResult = pDataHandler->DHWrite4(m_punSizeTable[i]);

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


