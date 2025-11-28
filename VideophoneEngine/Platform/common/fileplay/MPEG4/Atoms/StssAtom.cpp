///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	StssAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
//
// Sorenson includes.
//
#include "SMCommon.h"
#include "SMMemory.h"

#include "StssAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::StssAtom_c
//
// Abstract:
//
// Returns:
//
StssAtom_c::StssAtom_c()
{
	m_unType = eAtom4CCstss;

	m_unSampleCount = ~0;	// -1 means all sync sample, 0 means all non-sync
	m_punSyncTable = nullptr;
	m_unAllocated = 0;
	m_unLastSample = 0;
	m_unSyncsToAdd = 0;

} // StssAtom_c::StssAtom_c


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t StssAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unSampleCount = GetInt32(pDataHandler, un64Offset);

	//
	// Make sure the table isn't larger than what is left to read.
	//
	ASSERTRT(m_unSampleCount * sizeof(uint32_t) == un64Length - sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_punSyncTable = (uint32_t *)GetDataPtr(pDataHandler, un64Offset + sizeof(uint32_t), m_unSampleCount * sizeof(uint32_t));

	ASSERTRT(m_punSyncTable != nullptr, SMRES_ALLOC_FAILED);
	
#ifdef SM_LITTLE_ENDIAN
	//
	// Byteswap all of the values.
	//
	unsigned int i;
	for (i = 0; i < m_unSampleCount; i++)
	{
		m_punSyncTable[i] = DWORDBYTESWAP(m_punSyncTable[i]);
	}
#endif // SM_LITTLE_ENDIAN

smbail:

	return SMResult;
} // StssAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t StssAtom_c::GetSize()
{
	uint32_t unSize = 0;

	if (m_unSampleCount != (unsigned)~0)
	{
		unSize += sizeof(uint32_t);

		unSize += m_unSampleCount * sizeof(uint32_t);

		unSize += GetHeaderSize(unSize);
	}

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::AddSyncs
//
// Abstract:
//
// Returns:
//
SMResult_t StssAtom_c::AddSyncs(
	SMHandle hSyncSamples,
	uint32_t unSampleCount)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSyncCount;
	unsigned int i;

	//
	// If the handle is null then all unSampleCount samples are sync samples.
	//
	if (hSyncSamples == nullptr)
	{
		//
		// All samples are sync samples
		//
		if (m_unSampleCount == (unsigned)~0)
		{
			m_unSyncsToAdd += unSampleCount;
		}
		else
		{
			//
			// Since there are samples added to the table then we must add these as well.
			//
			if (m_unSampleCount + unSampleCount >= m_unAllocated)
			{
				m_punSyncTable = (uint32_t *)SMRealloc(m_punSyncTable, (m_unSampleCount + 500 + unSampleCount) * sizeof(uint32_t));

				ASSERTRT(m_punSyncTable != nullptr, SMRES_ALLOC_FAILED);

				m_unAllocated = m_unSampleCount + 500 + unSampleCount;
			}

			for (i = 1; i <= unSampleCount; i++)
			{
				m_punSyncTable[m_unSampleCount++] = i + m_unLastSample;
			}
		}
	}
	else
	{
		unSyncCount = SMGetHandleSize(hSyncSamples) / sizeof(uint32_t);

		if (unSyncCount == 0)
		{
			//
			// If the size of the handle is zero (no sync samples) then unSampleCount samples are not syncs.  We
			// need to allocate the table and populate it with past sync samples (all samples in the past must
			// have been syncs).
			//
			if (m_unSampleCount == (unsigned)~0)
			{
				m_unSampleCount = 0;
			}

			//
			// If there were sync samples in the past then add them now.
			//
			if (m_unSyncsToAdd > 0)
			{
				//
				// There are no sync samples to add.  If there were syncs in the past then
				// we need to add all that were not added until now.
				//
				if (m_unSampleCount + m_unSyncsToAdd >= m_unAllocated)
				{
					m_punSyncTable = (uint32_t *)SMRealloc(m_punSyncTable, (m_unSampleCount + 500 + m_unSyncsToAdd) * sizeof(uint32_t));

					ASSERTRT(m_punSyncTable != nullptr, SMRES_ALLOC_FAILED);

					m_unAllocated = m_unSampleCount + 500 + m_unSyncsToAdd;
				}

				for (i = 1; i <= m_unSyncsToAdd; i++)
				{
					m_punSyncTable[m_unSampleCount++] = i;
				}

				m_unSyncsToAdd = 0;
			}
		}
		else
		{
			if (m_unSampleCount == (unsigned)~0)
			{
				//
				// No syncs have been added yet.
				//
				auto pSync = (uint32_t *)*hSyncSamples;

				for (i = 1; i <= unSyncCount; i++)
				{
					//
					// Check for gaps in the array of sync samples.  If there are any gaps then
					// we need to allocate the table and populate it with sync information.
					//
					if (pSync[i - 1] != i)
					{
						m_unSampleCount = 0;

						//
						// Allocate enough memory to potentially hold the sync samples.
						//
						if (unSyncCount + m_unSyncsToAdd  >= m_unAllocated)
						{
							m_punSyncTable = (uint32_t *)SMRealloc(m_punSyncTable, (500 + unSyncCount + m_unSyncsToAdd) * sizeof(uint32_t));

							ASSERTRT(m_punSyncTable != nullptr, SMRES_ALLOC_FAILED);

							m_unAllocated = 500 + unSyncCount + m_unSyncsToAdd;
						}

						//
						// Add all past samples as syncs
						//
						for (i = 1; i <= m_unSyncsToAdd; i++)
						{
							m_punSyncTable[m_unSampleCount++] = i;
						}

						m_unSyncsToAdd = 0;

						for (i = 0; i < unSyncCount; i++)
						{
							m_punSyncTable[m_unSampleCount++] = *(uint32_t *)*hSyncSamples + m_unLastSample;
						}

						break;
					}
				}

				//
				// No syncs were added to the table so remember how many syncs we need to add when
				// we come across a non-sync sample.
				//
				if (m_unSampleCount == (unsigned)~0)
				{
					m_unSyncsToAdd += unSyncCount;
				}

			}
			else
			{
				//
				// Allocate enough memory to potentially hold the sync samples.
				//
				if (m_unSampleCount + unSyncCount >= m_unAllocated)
				{
					m_punSyncTable = (uint32_t *)SMRealloc(m_punSyncTable, (m_unSampleCount + 500 + unSyncCount) * sizeof(uint32_t));

					ASSERTRT(m_punSyncTable != nullptr, SMRES_ALLOC_FAILED);

					m_unAllocated = m_unSampleCount + 500 + unSyncCount;
				}

				for (i = 0; i < unSyncCount; i++)
				{
					m_punSyncTable[m_unSampleCount++] = *(uint32_t *)*hSyncSamples + m_unLastSample;
				}
			}
		}
	}

	m_unLastSample += unSampleCount;

smbail:

	return SMResult;
} // StssAtom_c::AddSyncs


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void StssAtom_c::FreeAtom()
{
	if (m_punSyncTable)
	{
		SMFreePtr(m_punSyncTable);

		m_punSyncTable = nullptr;
	}

	Atom_c::FreeAtom();
}

///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::GetSyncSampleIndex
//
// Abstract:
//
// Returns:
//

SMResult_t StssAtom_c::GetSyncSampleIndex(
		uint32_t sampleIndex,
		uint32_t *pSyncSampleIndex) const 
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pSyncSampleIndex != nullptr, SMRES_INVALID_DATA_HANDLER);
	ASSERTRT(m_punSyncTable != nullptr, SMRES_INVALID_DATA_HANDLER);

	*pSyncSampleIndex = 1;

	for (uint32_t i = 0; i < m_unSampleCount; i++)
	{
		if (sampleIndex >= m_punSyncTable[i])
		{
			*pSyncSampleIndex = m_punSyncTable[i];
		}
		else if (sampleIndex < m_punSyncTable[i])
		{
			break;
		}
	}

smbail:

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::IsSampleSync
//
// Abstract:
//
// Returns:
//
SMResult_t StssAtom_c::IsSampleSync(
	uint32_t unSampleIndex,
	bool_t *pbSyncSample) const
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int i;

	if (m_unSampleCount == 0)
	{
		*pbSyncSample = eFalse;
	}
	else if (m_unSampleCount == (unsigned)~0)
	{
		*pbSyncSample = eTrue;
	}
	else
	{
		*pbSyncSample = eFalse;

		for (i = 0; i < m_unSampleCount; i++)
		{
			if (unSampleIndex < m_punSyncTable[i])
			{
				break;
			}

			if (unSampleIndex == m_punSyncTable[i])
			{
				*pbSyncSample = eTrue;

				break;
			}
		}
	}

	return SMResult;
} // StssAtom_c::IsSampleSync


///////////////////////////////////////////////////////////////////////////////
//; StssAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t StssAtom_c::Write(
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

	if (m_unSampleCount != (unsigned)~0)
	{
		//
		// Write the atom header.
		//
		SMResult = WriteHeader(pDataHandler, unSize);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_unSampleCount);

		TESTRESULT();

		for (i = 0; i < m_unSampleCount; i++)
		{
			SMResult = pDataHandler->DHWrite4(m_punSyncTable[i]);

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
} // StssAtom_c::Write


