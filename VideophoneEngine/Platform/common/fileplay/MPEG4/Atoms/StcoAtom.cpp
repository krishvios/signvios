///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	StcoAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMCommon.h"
#include "SMMemory.h"

#include "StcoAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c::StcoAtom_c
//
// Abstract:
//
// Returns:
//
StcoAtom_c::StcoAtom_c()
{
	m_unType = eAtom4CCstco;

	m_unCount = 0;
	m_punTable = nullptr;
	m_pun64Table = nullptr;
	m_unAllocated = 0;

}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t StcoAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unCount = GetInt32(pDataHandler, un64Offset);

	if (m_unType == eAtom4CCco64)
	{
		ASSERTRT(m_unCount * sizeof(uint64_t) == un64Length - sizeof(uint32_t), SMRES_INVALID_MOVIE);

		m_pun64Table = (uint64_t *)GetDataPtr(pDataHandler, un64Offset + sizeof(uint32_t), m_unCount * sizeof(uint64_t));

		ASSERTRT(m_pun64Table != nullptr, SMRES_ALLOC_FAILED);
		
#ifdef SM_LITTLE_ENDIAN
		//
		// Byteswap all of the values.
		//
		unsigned int i;
		for (i = 0; i < m_unCount; i++)
		{
			m_pun64Table[i] = QWORDBYTESWAP(m_pun64Table[i]);
		}
#endif // SM_LITTLE_ENDIAN
	}
	else
	{
		ASSERTRT(m_unCount * sizeof(uint32_t) == un64Length - sizeof(uint32_t), SMRES_INVALID_MOVIE);

		m_punTable = (uint32_t *)GetDataPtr(pDataHandler, un64Offset + sizeof(uint32_t), m_unCount * sizeof(uint32_t));

		ASSERTRT(m_punTable != nullptr, SMRES_ALLOC_FAILED);

#ifdef SM_LITTLE_ENDIAN
		//
		// Byteswap all of the values.
		//
		unsigned int i;
		for (i = 0; i < m_unCount; i++)
		{
			m_punTable[i] = DWORDBYTESWAP(m_punTable[i]);
		}
#endif // SM_LITTLE_ENDIAN
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t StcoAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += sizeof(uint32_t);

	if (m_unType == eAtom4CCco64)
	{
		unSize += m_unCount * sizeof(uint64_t);
	}
	else
	{
		unSize += m_unCount * sizeof(uint32_t);
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c::ConvertTo64Bit
//
// Abstract:
//
// Returns:
//
SMResult_t StcoAtom_c::ConvertTo64Bit()
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t i;

	//
	// Need to convert the 32-bit table to a 64-bit table.
	//
	m_pun64Table = (uint64_t *)SMRealloc(m_pun64Table, (m_unCount + 500) * sizeof(uint64_t));

	ASSERTRT(m_pun64Table != nullptr, SMRES_ALLOC_FAILED);

	m_unAllocated = m_unCount + 500;

	for (i = 0; i < m_unCount - 1; i++)
	{
		m_pun64Table[i] = m_punTable[i];
	}

	//
	// Free the old table.
	//
	SMFreePtr(m_punTable);

	m_punTable = nullptr;

	//
	// Change the type of the atom.
	//
	m_unType = eAtom4CCco64;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c::AddChunk
//
// Abstract:
//
// Returns:
//
SMResult_t StcoAtom_c::AddChunk(
	uint64_t un64Offset)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	m_unCount++;

	if (m_unType == eAtom4CCco64)
	{
		if (m_unCount > m_unAllocated)
		{
			m_pun64Table = (uint64_t *)SMRealloc(m_pun64Table, (m_unCount + 500) * sizeof(uint64_t));

			ASSERTRT(m_pun64Table != nullptr, SMRES_ALLOC_FAILED);

			m_unAllocated = m_unCount + 500;
		}

		m_pun64Table[m_unCount - 1] = un64Offset;
	}
	else
	{
		if (un64Offset > 0xFFFFFFFF)
		{
			SMResult = ConvertTo64Bit();

			TESTRESULT();

			m_pun64Table[m_unCount - 1] = un64Offset;
		}
		else
		{
			if (m_unCount > m_unAllocated)
			{
				m_punTable = (uint32_t *)SMRealloc(m_punTable, (m_unCount + 500) * sizeof(uint32_t));

				ASSERTRT(m_punTable != nullptr, SMRES_ALLOC_FAILED);

				m_unAllocated = m_unCount + 500;
			}

			m_punTable[m_unCount - 1] = (uint32_t)un64Offset;
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
void StcoAtom_c::FreeAtom()
{
	if (m_punTable)
	{
		SMFreePtr(m_punTable);

		m_punTable = nullptr;
	}

	if (m_pun64Table)
	{
		SMFreePtr(m_pun64Table);

		m_pun64Table = nullptr;
	}

	Atom_c::FreeAtom();
}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c;:GetChunkOffset
//
// Abstract:
//
// Returns:
//
SMResult_t StcoAtom_c::GetChunkOffset(
	uint32_t unChunkIndex,
	uint64_t *pun64Offset) const
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(unChunkIndex < m_unCount, SMRES_INVALID_INDEX);

	if (m_unType == eAtom4CCco64)
	{
		*pun64Offset = m_pun64Table[unChunkIndex];
	}
	else
	{
		*pun64Offset = m_punTable[unChunkIndex];
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c;:GetChunkOffset
//
// Abstract:
//
// Returns:
//
SMResult_t StcoAtom_c::SetChunkOffset(
	uint32_t unChunkIndex,
	uint64_t un64Offset)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(unChunkIndex < m_unCount, SMRES_INVALID_INDEX);

	if (m_unType == eAtom4CCco64)
	{
		m_pun64Table[unChunkIndex] = un64Offset;
	}
	else
	{
		if (un64Offset > 0xFFFFFFFF)
		{
			SMResult = ConvertTo64Bit();

			TESTRESULT();

			m_pun64Table[unChunkIndex] = un64Offset;
		}
		else
		{
			m_punTable[unChunkIndex] = (uint32_t)un64Offset;
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; StcoAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t StcoAtom_c::Write(
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

	if (m_unType == eAtom4CCco64)
	{
		for (i = 0; i < m_unCount; i++)
		{
			SMResult = pDataHandler->DHWrite8(m_pun64Table[i]);

			TESTRESULT();
		}
	}
	else
	{
		for (i = 0; i < m_unCount; i++)
		{
			SMResult = pDataHandler->DHWrite4(m_punTable[i]);

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


