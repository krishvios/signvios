///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Media.cpp
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


#include "SorensonMP4.h"
#include "MP4Common.h"
#include "HdlrAtom.h"
#include "MdiaAtom.h"
#include "MdhdAtom.h"
#include "MinfAtom.h"
#include "StblAtom.h"
#include "StsdAtom.h"
#include "SttsAtom.h"
#include "CttsAtom.h"
#include "StszAtom.h"
#include "StscAtom.h"
#include "StcoAtom.h"
#include "StssAtom.h"
#include "CDataHandler.h"
#include "CServerDataHandler.h"
#include "CFileDataHandler.h"


///////////////////////////////////////////////////////////////////////////////
//; MP4BeginMediaEdits
//
// Abstract:
//
// Returns:
//
SMResult_t MP4BeginMediaEdits(
	Media_t *pMedia)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	if (pMdiaAtom->m_pDataHandler)
	{
		SMResult = (pMdiaAtom->m_pDataHandler)->OpenDataHandlerForWrite();

		TESTRESULT();
	}

smbail:

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4EndMediaEdits
//
// Abstract:
//
// Returns:
//
SMResult_t MP4EndMediaEdits(
	Media_t *pMedia)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MdhdAtom_c *pMdhdAtom;
	MinfAtom_c *pMinfAtom;
	StblAtom_c *pStblAtom;
	SttsAtom_c *pSttsAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MEDIA);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MEDIA);

	pSttsAtom = (SttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pSttsAtom != nullptr, SMRES_INVALID_MEDIA);

	pMdhdAtom = (MdhdAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCmdhd);

	ASSERTRT(pMdhdAtom != nullptr, SMRES_INVALID_MEDIA);

	pMdhdAtom->SetDuration(pSttsAtom->GetDuration());

	if (pMdiaAtom->m_pDataHandler)
	{
		(pMdiaAtom->m_pDataHandler)->CloseDataHandler();
	}

smbail:
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4UpdateMediaDuration
//
// Abstract:
//
// Returns:
//
SMResult_t MP4UpdateMediaDuration(
	Media_t *pMedia)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MdhdAtom_c *pMdhdAtom;
	MinfAtom_c *pMinfAtom;
	StblAtom_c *pStblAtom;
	SttsAtom_c *pSttsAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MEDIA);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MEDIA);

	pSttsAtom = (SttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pSttsAtom != nullptr, SMRES_INVALID_MEDIA);

	pMdhdAtom = (MdhdAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCmdhd);

	ASSERTRT(pMdhdAtom != nullptr, SMRES_INVALID_MEDIA);

	pMdhdAtom->SetDuration(pSttsAtom->GetDuration());

smbail:
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4SetMediaTimeScale
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SetMediaTimeScale(
	Media_t *pMedia,
	uint32_t unTimeScale)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MdhdAtom_c *pMdhdAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMdhdAtom = (MdhdAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCmdhd);

	ASSERTRT(pMdhdAtom != nullptr, SMRES_INVALID_MEDIA);

	pMdhdAtom->SetTimeScale(unTimeScale);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaTimeScale
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaTimeScale(
	Media_t *pMedia,
	MP4TimeScale *punTimeScale)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MdhdAtom_c *pMdhdAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMdhdAtom = (MdhdAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCmdhd);

	ASSERTRT(pMdhdAtom != nullptr, SMRES_INVALID_MEDIA);

	*punTimeScale = pMdhdAtom->GetTimeScale();

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t MP4AddMediaSamples(
	Media_t *pMedia,
	const SMHandle hSampleData, 
	SMHandle hSampleDescription,
	unsigned int unSampleCount,
	SMHandle hSampleDurations,
	SMHandle hSampleSizes,
	SMHandle hCompositionOffsets,
	SMHandle hSyncSamples)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t i;
	MdiaAtom_c *pMdiaAtom;
	MinfAtom_c *pMinfAtom;
	StblAtom_c *pStblAtom;
	StsdAtom_c *pStsdAtom;
	SttsAtom_c *pSttsAtom;
	CttsAtom_c *pCttsAtom;
	StszAtom_c *pStszAtom;
	StscAtom_c *pStscAtom;
	StcoAtom_c *pStcoAtom;
	StssAtom_c *pStssAtom;
	uint64_t un64Offset;
	TrakAtom_c *pTrakAtom;
	Movie_t *pMovie;
	uint32_t unTotalSize;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MEDIA);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MEDIA);

	pSttsAtom = (SttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pSttsAtom != nullptr, SMRES_INVALID_MEDIA);

	pCttsAtom = (CttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCctts);

	ASSERTRT(pCttsAtom != nullptr, SMRES_INVALID_MEDIA);

	pStszAtom = (StszAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsz);

	ASSERTRT(pStszAtom != nullptr, SMRES_INVALID_MEDIA);

	pStscAtom = (StscAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsc);

	ASSERTRT(pStscAtom != nullptr, SMRES_INVALID_MEDIA);

	pStcoAtom = (StcoAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstco);

	ASSERTRT(pStcoAtom != nullptr, SMRES_INVALID_MEDIA);

	pStssAtom = (StssAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstss);

	ASSERTRT(pStssAtom != nullptr, SMRES_INVALID_MEDIA);

	pTrakAtom = (TrakAtom_c *)pMdiaAtom->GetParent();

	ASSERTRT(pTrakAtom != nullptr, SMRES_INVALID_MEDIA);

	pMovie = pTrakAtom->GetMovie();

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MEDIA);

	if (hSampleDescription)
	{
		pStsdAtom = (StsdAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsd);

		ASSERTRT(pStsdAtom != nullptr, SMRES_INVALID_MEDIA);

		((SampleDescr_t *)*hSampleDescription)->usDataRefIndex = BigEndian2(1);

		SMResult = pStsdAtom->AddSampleDescription(hSampleDescription,
												   &pMdiaAtom->m_unSampleDescrIndex);
	}

	ASSERTRT(pMdiaAtom->m_unSampleDescrIndex != (unsigned)~0, SMRES_INVALID_MEDIA);

	//
	// Determine how big this media data is...
	//
	unTotalSize = 0;

	for (i = 0; i < unSampleCount; i++)
	{
		unTotalSize += ((uint32_t *)*hSampleSizes)[i];
	}

	//
	// Write the sample data to the file.
	//
	SMResult = WriteMediaData(pMovie, (uint8_t *)*hSampleData, unTotalSize, &un64Offset);

	TESTRESULT();

	//
	// Store the sample deltas and sizes.
	//
	for (i = 0; i < unSampleCount; i++)
	{
		//
		// Add the duration to the time-to-sample table.
		//
		pSttsAtom->AddSampleDelta(((uint32_t *)*hSampleDurations)[i]);

		//
		// Add the composition offset if given.
		//
		if (hCompositionOffsets)
		{
			pCttsAtom->AddSampleDelta(((uint32_t *)*hCompositionOffsets)[i]);
		}

		//
		// Add the size to the sample size table.
		//
		pStszAtom->AddSampleSize(((uint32_t *)*hSampleSizes)[i]);
	}

	//
	// Add the chunk to the sample to chunk table.
	//
	pStscAtom->AddChunk(unSampleCount, pMdiaAtom->m_unSampleDescrIndex);

	//
	// Add the chunk to the chunk offset table.
	//
	pStcoAtom->AddChunk(un64Offset);

	//
	// Process sync information.
	//
	SMResult = pStssAtom->AddSyncs(hSyncSamples, unSampleCount);

	TESTRESULT();

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaDuration(
	Media_t *pMedia,
	MP4TimeValue *pMediaDuration)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MdhdAtom_c *pMdhdAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMdhdAtom = (MdhdAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCmdhd);

	ASSERTRT(pMdhdAtom != nullptr, SMRES_INVALID_MEDIA);

	*pMediaDuration = pMdhdAtom->GetDuration();
	
smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaSampleDescription(
	Media_t *pMedia,
	uint32_t unIndex,
	SMHandle hSampleDescr)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	MinfAtom_c *pMinf;
	StblAtom_c *pStbl;
	StsdAtom_c *pStsd;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdia = (MdiaAtom_c *)pMedia;

	pMinf = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinf != nullptr, SMRES_INVALID_MEDIA);

	pStbl = (StblAtom_c *)pMinf->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStbl != nullptr, SMRES_INVALID_MEDIA);

	pStsd = (StsdAtom_c *)pStbl->GetChildAtom(eAtom4CCstsd);

	ASSERTRT(pStsd != nullptr, SMRES_INVALID_MEDIA);

	SMResult = pStsd->GetSampleDescription(unIndex, hSampleDescr);

	TESTRESULT();

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4SetMediaSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SetMediaSampleDescription(
	Media_t *pMedia,
	uint32_t unIndex,
	SMHandle hSampleDescr)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	MinfAtom_c *pMinf;
	StblAtom_c *pStbl;
	StsdAtom_c *pStsd;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdia = (MdiaAtom_c *)pMedia;

	pMinf = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinf != nullptr, SMRES_INVALID_MEDIA);

	pStbl = (StblAtom_c *)pMinf->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStbl != nullptr, SMRES_INVALID_MEDIA);

	pStsd = (StsdAtom_c *)pStbl->GetChildAtom(eAtom4CCstsd);

	ASSERTRT(pStsd != nullptr, SMRES_INVALID_MEDIA);

	((SampleDescr_t *)*hSampleDescr)->usDataRefIndex = BigEndian2(1);

	pStsd->SetSampleDescription(unIndex, hSampleDescr);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4MediaTimeToSampleIndex
//
// Abstract:
//
// Returns:
//
SMResult_t MP4MediaTimeToSampleIndex(
	Media_t *pMedia,
	MP4TimeValue Time,
	uint32_t *punIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int unSampleIndex = ~0;
	MdiaAtom_c *pMdia;
	StblAtom_c *pSampleTable;
	MinfAtom_c *pMediaInfo;
	SttsAtom_c *pTimeToSample;
	unsigned int i;
	DeltaEntry_t *pTable;
	MP4TimeValue CurrentTime;
	unsigned int unSampleNumber;

	pMdia = (MdiaAtom_c *)pMedia;

	pMediaInfo = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMediaInfo != nullptr, SMRES_INVALID_MOVIE);

	pSampleTable = (StblAtom_c *)pMediaInfo->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pSampleTable != nullptr, SMRES_INVALID_MOVIE);

	pTimeToSample = (SttsAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pTimeToSample != nullptr, SMRES_INVALID_MOVIE);

	//
	// Find the entry that the requested time falls in.
	//
	if (Time >= pTimeToSample->FoundTime)
	{
		pTable = pTimeToSample->pTable;
		CurrentTime = pTimeToSample->CurrentTime;
		unSampleNumber = pTimeToSample->unSampleNumber;
		i = pTimeToSample->unLoopStart;
	}
	else
	{
		pTable = pTimeToSample->m_punDeltaTable;
		CurrentTime = 0;
		unSampleNumber = 1;
		i = 0;
	}

	for (; i < pTimeToSample->m_unCount; i++)
	{
		if ((MP4TimeValue)(CurrentTime + pTable->unEntryCount * pTable->unDelta) > Time)
		{
			unSampleIndex = unSampleNumber + (unsigned int)((Time - CurrentTime) / pTable->unDelta);

			//
			// Store some values for a faster start next time
			// if the movie is playing in the forward direction.
			//
			pTimeToSample->FoundTime = Time;
			pTimeToSample->pTable = pTable;
			pTimeToSample->CurrentTime = CurrentTime;
			pTimeToSample->unSampleNumber = unSampleNumber;
			pTimeToSample->unLoopStart = i;

			break;
		}

		CurrentTime += pTable->unEntryCount * pTable->unDelta;
		unSampleNumber += pTable->unEntryCount;

		pTable++;
	}

	if (i == pTimeToSample->m_unCount)
	{
		unSampleIndex = unSampleNumber - 1;
	}

	*punIndex = unSampleIndex;

smbail:
	
	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4SampleIndexToMediaTime
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SampleIndexToMediaTime(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	MP4TimeValue *punTime,
	MP4TimeValue *punDuration)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	StblAtom_c *pStblAtom;
	MinfAtom_c *pMinfAtom;
	SttsAtom_c *pSttsAtom;
	uint32_t unSampleCount;
	MP4TimeValue unMediaTime;
	uint32_t i;

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MOVIE);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MOVIE);

	pSttsAtom = (SttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pSttsAtom != nullptr, SMRES_INVALID_MOVIE);

	unSampleCount = 0;
	unMediaTime = 0;

	for (i = 0; i < pSttsAtom->m_unCount; i++)
	{
		unSampleCount += pSttsAtom->m_punDeltaTable[i].unEntryCount;

		if (unSampleIndex <= unSampleCount)
		{
			unMediaTime += (unSampleIndex - (unSampleCount - pSttsAtom->m_punDeltaTable[i].unEntryCount) - 1) * pSttsAtom->m_punDeltaTable[i].unDelta;

			*punTime = unMediaTime;
			*punDuration = pSttsAtom->m_punDeltaTable[i].unDelta;

			break;
		}

		unMediaTime += pSttsAtom->m_punDeltaTable[i].unDelta * pSttsAtom->m_punDeltaTable[i].unEntryCount;
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetSampleDuration
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetSampleDuration(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	uint32_t *punDuration)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	StblAtom_c *pSampleTable;
	MinfAtom_c *pMediaInfo;
	SttsAtom_c *pTimeToSample;
	unsigned int i;
	DeltaEntry_t *pTable;
	unsigned int unSampleNumber;
	unsigned int unSampleDuration = 0;

	pMdia = (MdiaAtom_c *)pMedia;

	pMediaInfo = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMediaInfo != nullptr, SMRES_INVALID_MOVIE);

	pSampleTable = (StblAtom_c *)pMediaInfo->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pSampleTable != nullptr, SMRES_INVALID_MOVIE);

	pTimeToSample = (SttsAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pTimeToSample != nullptr, SMRES_INVALID_MOVIE);

	//
	// Find the entry that the requested time falls in.
	//
	pTable = pTimeToSample->m_punDeltaTable;
	unSampleNumber = 0;
	i = 0;

	for (; i < pTimeToSample->m_unCount; i++)
	{
		if (pTable->unEntryCount + unSampleNumber > unSampleIndex)
		{
			unSampleDuration = pTable->unDelta;

			break;
		}

		unSampleNumber += pTable->unEntryCount;

		pTable++;
	}

	*punDuration = unSampleDuration;

smbail:
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; GetMediaChunk
//
// Abstract:
//
// Returns:
//
static SMResult_t GetMediaChunk(
	MdiaAtom_c *pMdia,
	StcoAtom_c *pChunkOffset,
	StszAtom_c *pSampleSize,
	unsigned int unSampleIndex,
	SampleToChunk_t *pTable,
	unsigned int unCurrentSample,
	SMHandle hData,
	uint64_t *pun64SampleOffset,
	uint32_t *punSampleSize,
	bool_t bReleaseBuffer)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	unsigned int unChunkIndex;
	uint64_t un64SampleOffset;
	uint32_t unSampleSize;
	unsigned int j;
	unsigned int unFirstSampleInChunk;

	//
	// Only get the data if a non-null handle is provided.
	//
	if (hData || pun64SampleOffset || punSampleSize)
	{
		unChunkIndex = (pTable->unFirstChunk - 1) + (unSampleIndex - unCurrentSample) / pTable->unSamplesPerChunk;
	
		unFirstSampleInChunk = (unChunkIndex - (pTable->unFirstChunk - 1)) * pTable->unSamplesPerChunk + unCurrentSample;

		//
		// Get the sample offset.
		//
		pChunkOffset->GetChunkOffset(unChunkIndex, &un64SampleOffset);

		pSampleSize->GetDefaultSize(&unSampleSize);

		if (unSampleSize > 0)
		{
			un64SampleOffset += (unSampleIndex - unFirstSampleInChunk) * unSampleSize;
		}
		else
		{
			for (j = unFirstSampleInChunk; j < unSampleIndex; j++)
			{
				pSampleSize->GetSampleSize(j, &unSampleSize);

				un64SampleOffset += unSampleSize;
			}

			pSampleSize->GetSampleSize(j, &unSampleSize);
		}

		if (pun64SampleOffset)
		{
			*pun64SampleOffset = un64SampleOffset;
		}

		if (punSampleSize)
		{
			*punSampleSize = unSampleSize;
		}

		if (hData)
		{
			SMSetHandleSize(hData, unSampleSize);

			SMResult = pMdia->m_pDataHandler->DHRead(un64SampleOffset, unSampleSize, *hData, bReleaseBuffer);

			TESTRESULT ();
		}
		
		pMdia->m_unSampleIndex = unSampleIndex;
		pMdia->m_unSampleSize = unSampleSize;
		pMdia->m_unLastSampleInChunk = unFirstSampleInChunk + pTable->unSamplesPerChunk - 1;
		pMdia->m_unChunkIndex = unChunkIndex;
		pMdia->m_un64SampleOffset = un64SampleOffset;
	}

smbail:

	return SMResult;
} // GetMediaChunk

///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaSampleByIndex
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaSampleByIndex(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	SMHandle hData,
	uint64_t *pun64SampleOffset,
	uint32_t *punSampleSize,
	bool_t *pbSyncSample,
	uint32_t *punSampleDescIndex,
	bool_t bReleaseBuffer)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	MinfAtom_c *pMediaInfo;
	StblAtom_c *pSampleTable;
	StscAtom_c *pSampleToChunk;
	SampleToChunk_t *pTable;
	StcoAtom_c *pChunkOffset;
	StszAtom_c *pSampleSize;
	StssAtom_c *pStss;
	unsigned int unCurrentSample;
	unsigned int i;
	unsigned int unNumChunks;

	pMdia = (MdiaAtom_c *)pMedia;

	pMediaInfo = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMediaInfo != nullptr, SMRES_INVALID_MOVIE);

	pSampleTable = (StblAtom_c *)pMediaInfo->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pSampleTable != nullptr, SMRES_INVALID_MOVIE);

	pSampleToChunk = (StscAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstsc);

	ASSERTRT(pSampleToChunk != nullptr, SMRES_INVALID_MOVIE);

	pChunkOffset = (StcoAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstco);

	ASSERTRT(pChunkOffset != nullptr, SMRES_INVALID_MOVIE);

	pSampleSize = (StszAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstsz);

	ASSERTRT(pSampleSize != nullptr, SMRES_INVALID_MOVIE);

	if (unSampleIndex == pMdia->m_unSampleIndex + 1)
	{
		//
		// We are getting the next sample after the one previously retrieved.
		// In this case we need to increment the sample index and then check to see
		// if the sample falls into the next chunk.  If it does then we need to increment the
		// chunk index and check to if the chunk is associated with the next
		// "sample to chunk" table entry.  If it is then we increment the index
		// to the next "sample to chunk" table entry.  We then get the chunk offset wich
		// is the same as the sample offset that we are after.  We also set the "last
		// sample in chunk" value so that we know when we transition from one chunk to the next.
		//
		pMdia->m_unSampleIndex++;
		
		//
		// Are we referring to a sample in the next chunk?
		// 
		if (pMdia->m_unSampleIndex > pMdia->m_unLastSampleInChunk)
		{
			//
			// We are so increment the chunk index
			//
			pMdia->m_unChunkIndex++;
			
			//
			// Is this chunk in the next "sample to chunk" table entry?
			// We have to add 1 to the ChunkIndex because it is 0 based and the 
			// value in the LastChunk is 1 based.
			//
			if (pMdia->m_unChunkIndex + 1 > pMdia->m_unLastChunk)
			{
				//
				// Yes, so increment the "sample to chunk" index.
				//
				pMdia->m_unSampleToChunkIndex++;
				
				//
				// Determine what the last chunk is in the now current "sample to chunk" table entry
				// We must add 1 to the SampleToChunkIndex because it is 0 based and the SampleToChunk
				// entry count is 1 based.
				//
				if (pMdia->m_unSampleToChunkIndex + 1 == pSampleToChunk->m_unCount)
				{
					pMdia->m_unLastChunk = pChunkOffset->m_unCount;
				}
				else
				{
					pMdia->m_unLastChunk =
						pSampleToChunk->m_pTable[pMdia->m_unSampleToChunkIndex + 1].unFirstChunk - 1;
				}
			}
				
			//
			// Get the next chunk offset which is the same as the sample offset we are after.
			//
			pChunkOffset->GetChunkOffset(pMdia->m_unChunkIndex, &pMdia->m_un64SampleOffset);
				
			//
			// Determine the last sample in the now current chunk
			//
			pMdia->m_unLastSampleInChunk += pSampleToChunk->m_pTable[pMdia->m_unSampleToChunkIndex].unSamplesPerChunk;
		}
		else
		{
			//
			// Simply add the size of the sample to get the offset of the sample we
			// are after.
			//
			pMdia->m_un64SampleOffset += pMdia->m_unSampleSize;
		}
	
		//
		// Return the sample offset if requested.
		//
		if (pun64SampleOffset)
		{
			*pun64SampleOffset = pMdia->m_un64SampleOffset;
		}

		//
		// Get the size for the sample.  First check if all 
		// samples have the same size.  If not then then get
		// the size specific to this sample.
		//
		pSampleSize->GetDefaultSize(&pMdia->m_unSampleSize);

		if (pMdia->m_unSampleSize == 0)
		{
			pSampleSize->GetSampleSize(pMdia->m_unSampleIndex, &pMdia->m_unSampleSize);
		}
		
		//
		// Return the sample size if requested.
		//
		if (punSampleSize)
		{
			*punSampleSize = pMdia->m_unSampleSize;
		}

		//
		// Return the sample data if requested.
		//
		if (hData)
		{
			SMSetHandleSize(hData, pMdia->m_unSampleSize);

			SMResult = pMdia->m_pDataHandler->DHRead(pMdia->m_un64SampleOffset,
				 pMdia->m_unSampleSize, *hData, bReleaseBuffer);

			TESTRESULT ();
		}
	
		//
		// Return the sample description index if requested.
		//
		if (punSampleDescIndex)
		{
			*punSampleDescIndex = pSampleToChunk->m_pTable[pMdia->m_unSampleToChunkIndex].unSampleDescrIndex;
		}
	
	}
	else
	{
		if (unSampleIndex >= pSampleToChunk->m_unFoundIndex)
		{
			pTable = pSampleToChunk->m_pLastTablePos;
			unCurrentSample = pSampleToChunk->m_unCurrentSample;
			i = pSampleToChunk->m_unLoopStart;
		}
		else
		{
			pTable = pSampleToChunk->m_pTable;
			unCurrentSample = 1;
			i = 0;
		}
	
		for (; i < pSampleToChunk->m_unCount - 1; i++)
		{
			unNumChunks = pTable[1].unFirstChunk - pTable[0].unFirstChunk;
	
			if (unSampleIndex < unCurrentSample + unNumChunks * pTable[0].unSamplesPerChunk)
			{
				if (punSampleDescIndex)
				{
					*punSampleDescIndex = pTable->unSampleDescrIndex;
				}
	
				SMResult = GetMediaChunk(pMdia, pChunkOffset, pSampleSize, unSampleIndex,
							pTable, unCurrentSample, hData, 
							pun64SampleOffset, punSampleSize, bReleaseBuffer);
	
				TESTRESULT ();
	
				//
				// Save off some values
				// so we can find the next one quicker the next time this function is 
				// called.
				//
				pSampleToChunk->m_unFoundIndex = unSampleIndex;
				pSampleToChunk->m_pLastTablePos = pTable;
				pSampleToChunk->m_unCurrentSample = unCurrentSample;
				pSampleToChunk->m_unLoopStart = i;
	
				pMdia->m_unLastChunk = pTable[1].unFirstChunk - 1;
				pMdia->m_unSampleToChunkIndex = i;
				
				break;
			}
	
			unCurrentSample += unNumChunks * pTable->unSamplesPerChunk;
	
			pTable++;
		}
	
		//
		// For the last entry in the table we must use the count of chunks 
		// from the chunk offset table to determine the number of chunks.
		// 
		if (i == pSampleToChunk->m_unCount - 1)
		{
			unNumChunks = pChunkOffset->m_unCount - pTable->unFirstChunk + 1;
	
			if (unSampleIndex < unCurrentSample + unNumChunks * pTable->unSamplesPerChunk)
			{
				if (punSampleDescIndex)
				{
					*punSampleDescIndex = pTable->unSampleDescrIndex;
				}
	
				SMResult = GetMediaChunk(pMdia, pChunkOffset, pSampleSize, unSampleIndex,
							pTable, unCurrentSample, hData, 
							pun64SampleOffset, punSampleSize, bReleaseBuffer);
	
				TESTRESULT ();
	
				//
				// Save off some values
				// so we can find the next one quicker the next time this function is 
				// called.
				//
				pSampleToChunk->m_unFoundIndex = unSampleIndex;
				pSampleToChunk->m_pLastTablePos = pTable;
				pSampleToChunk->m_unCurrentSample = unCurrentSample;
				pSampleToChunk->m_unLoopStart = i;
				
				pMdia->m_unLastChunk = pChunkOffset->m_unCount;
				pMdia->m_unSampleToChunkIndex = i;
			}
		}
	}
	
	//
	// If the pointer is not null then we need to determine if the sample
	// at the requested index is a sync sample.
	//
	if (pbSyncSample)
	{
		*pbSyncSample = eTrue;

		pStss = (StssAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstss);

		if (pStss)
		{
			pStss->IsSampleSync(unSampleIndex, pbSyncSample);
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; GetMediaPrevSync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaPrevSync(
	Media_t *pMedia,
	unsigned int unIndex,
	bool_t bIncludeCurrent,
	uint32_t *punSyncIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	MinfAtom_c *pMediaInfo;
	StblAtom_c *pSampleTable;
	StssAtom_c *pSync;
	unsigned int i;
	unsigned int unPrevSync = 1;

	pMdia = (MdiaAtom_c *)pMedia;

	pMediaInfo = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMediaInfo != nullptr, SMRES_INVALID_MOVIE);

	pSampleTable = (StblAtom_c *)pMediaInfo->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pSampleTable != nullptr, SMRES_INVALID_MOVIE);

	pSync = (StssAtom_c *)pSampleTable->GetChildAtom(eAtom4CCstss);

	if (pSync)
	{
		if (bIncludeCurrent)
		{
			for (i = 0; i < pSync->m_unSampleCount; i++)
			{
				if (unIndex < pSync->m_punSyncTable[i])
				{
					break;
				}

				unPrevSync = pSync->m_punSyncTable[i];
			}
		}
		else
		{
			for (i = 0; i < pSync->m_unSampleCount; i++)
			{
				if (unIndex <= pSync->m_punSyncTable[i])
				{
					break;
				}

				unPrevSync = pSync->m_punSyncTable[i];
			}
		}
	}
	else
	{
		//
		// If there is no sync table then all samples are sync samples.
		//
		if (unIndex > 1)
		{
			if (bIncludeCurrent)
			{
				unPrevSync = unIndex;
			}
			else
			{
				unPrevSync = unIndex - 1;
			}
		}
	}

	*punSyncIndex = unPrevSync;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaEditTime
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaEditTime(
	Media_t *pMedia,
	MP4TimeValue MediaTime,
	uint32_t unFlags,
	MP4TimeValue *pTime,
	MP4TimeValue *pDuration)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MinfAtom_c *pMinfAtom;
	StblAtom_c *pStblAtom;
	StscAtom_c *pStscAtom;
	StcoAtom_c *pStcoAtom;
	StszAtom_c *pStszAtom;
	SttsAtom_c *pSttsAtom;
	SampleToChunk_t *pTable;
	DeltaEntry_t *pTimeTable;
	unsigned int i, j, k;
	unsigned int unNumChunks;
	unsigned int unSamplesLeft;
	MP4TimeValue CurrentTime;
	MP4TimeValue ChunkDuration;

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	ASSERTRT(pMdiaAtom != nullptr, -1);

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MOVIE);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MOVIE);

	pStscAtom = (StscAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsc);

	ASSERTRT(pStscAtom != nullptr, SMRES_INVALID_MOVIE);

	pStcoAtom = (StcoAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstco);

	ASSERTRT(pStcoAtom != nullptr, SMRES_INVALID_MOVIE);

	pStszAtom = (StszAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsz);

	ASSERTRT(pStszAtom != nullptr, SMRES_INVALID_MOVIE);

	pSttsAtom = (SttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pSttsAtom != nullptr, SMRES_INVALID_MOVIE);

	pTable = pStscAtom->m_pTable;
	i = 0;

	pTimeTable = pSttsAtom->m_punDeltaTable;
	unSamplesLeft = pTimeTable->unEntryCount;

	CurrentTime = 0;

	for (; i < pStscAtom->m_unCount - 1; i++)
	{
		unNumChunks = (pTable + 1)->unFirstChunk - pTable->unFirstChunk;

		if (pTable->unSamplesPerChunk * unNumChunks <= unSamplesLeft)
		{
			//
			// All the chunks in this table entry have the same duration.
			//
			ChunkDuration = pTable->unSamplesPerChunk * pTimeTable->unDelta;

			if (MediaTime - CurrentTime < ChunkDuration * unNumChunks)
			{
				int nNumChunksInSegment;

				nNumChunksInSegment = (int)((MediaTime - CurrentTime) / ChunkDuration);

				*pTime = CurrentTime + nNumChunksInSegment * ChunkDuration + ChunkDuration;

				break;
			}
			
			CurrentTime += ChunkDuration * unNumChunks;

			unSamplesLeft -= pTable->unSamplesPerChunk * unNumChunks;

			if (unSamplesLeft == 0)
			{
				pTimeTable++;

				unSamplesLeft = pTimeTable->unEntryCount;
			}
		}
		else
		{
			for (j = 0; j < unNumChunks; j++)
			{
				ChunkDuration = 0;

				k = pTable->unSamplesPerChunk;

				for (;;)
				{
					if (unSamplesLeft <= k)
					{
						ChunkDuration += unSamplesLeft * pTimeTable->unDelta;

						k -= unSamplesLeft;

						pTimeTable++;

						unSamplesLeft = pTimeTable->unEntryCount;
					}
					else
					{
						ChunkDuration += k * pTimeTable->unDelta;

						unSamplesLeft -= k;

						break;
					}
				}

				if (CurrentTime + ChunkDuration > MediaTime)
				{
					*pTime = CurrentTime + ChunkDuration;

					goto smbail;
				}

				CurrentTime += ChunkDuration;
			}
		}

		pTable++;
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaSampleCount
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaSampleCount(
	Media_t *pMedia,
	uint32_t *punNumSamples)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	MinfAtom_c *pMinf;
	StblAtom_c *pStbl;
	StszAtom_c *pStsz;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_PARAMETER);

	pMdia = (MdiaAtom_c *)pMedia;

	pMinf = (MinfAtom_c *)pMdia->GetChildAtom(eAtom4CCminf);
	ASSERTRT(pMinf != nullptr, SMRES_INVALID_MEDIA);

	pStbl = (StblAtom_c *)pMinf->GetChildAtom(eAtom4CCstbl);
	ASSERTRT(pStbl != nullptr, SMRES_INVALID_MEDIA);

	pStsz = (StszAtom_c *)pStbl->GetChildAtom(eAtom4CCstsz);
	ASSERTRT(pStsz != nullptr, SMRES_INVALID_MEDIA);

	pStsz->GetSampleCount(punNumSamples);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaHandlerType
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaHandlerType(
	Media_t *pMedia,
	uint32_t *punHandlerType)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	HdlrAtom_c *pHdlr;

	pMdia = (MdiaAtom_c *)pMedia;

	pHdlr = (HdlrAtom_c *)pMdia->GetChildAtom(eAtom4CChdlr);

	if (pHdlr)
	{
		pHdlr->GetHandlerType(punHandlerType);
	}

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaDataSize
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaDataSize(
	Media_t *pMedia,
	MP4TimeValue StartTime,
	MP4TimeValue EndTime,
	uint64_t *pun64Size)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MinfAtom_c *pMinfAtom;
	StblAtom_c *pStblAtom;
	StszAtom_c *pStszAtom;
	uint32_t unStartIndex;
	uint32_t unEndIndex;
	uint64_t un64Size;
	uint32_t i;
	uint32_t unSampleSize;

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	ASSERTRT(pMdiaAtom != nullptr, -1);

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MOVIE);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MOVIE);

	pStszAtom = (StszAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsz);

	ASSERTRT(pStszAtom != nullptr, SMRES_INVALID_MOVIE);

	SMResult = MP4MediaTimeToSampleIndex(pMedia, StartTime, &unStartIndex);

	TESTRESULT();

	SMResult = MP4MediaTimeToSampleIndex(pMedia, EndTime, &unEndIndex);

	TESTRESULT();

	un64Size = 0;

	for (i = unStartIndex; i <= unEndIndex; i++)
	{
		SMResult =  pStszAtom->GetSampleSize(i, &unSampleSize);

		TESTRESULT();

		un64Size += unSampleSize;
	}

	*pun64Size = un64Size;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaNextTime
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaNextTime(
	Media_t *pMedia,
	MP4TimeValue Time,
	int32_t nDirection,
	MP4TimeValue *NextTime)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = SMRES_UNSUPPORTED_FEATURE;

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaSampleAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaSampleAsync(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	SMHandle hFrame)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	if (pMdiaAtom->m_pDataHandler)
	{
		((CServerDataHandler *) (pMdiaAtom->m_pDataHandler))->FrameGet (pMedia, unSampleIndex, hFrame);
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetIFrameIndex
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMediaSyncSampleIndex(
	Media_t *pMedia,
	uint32_t sampleIndex,
	uint32_t *pSyncSampleIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	auto pMdiaAtom = (MdiaAtom_c *)pMedia;
	MinfAtom_c *pMinfAtom = nullptr;
	StblAtom_c *pStblAtom = nullptr; 
	StssAtom_c *pStssAtom = nullptr;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);
	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MOVIE);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);
	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MOVIE);

	pStssAtom = (StssAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstss);
	ASSERTRT(pStssAtom != nullptr, SMRES_INVALID_MOVIE);

	SMResult = pStssAtom->GetSyncSampleIndex(sampleIndex, (uint32_t *)pSyncSampleIndex);

smbail:

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4GetMediaSampleAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4ReadyForDownloadUpdate(
	Media_t *pMedia)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;

	ASSERTRT(pMedia != nullptr, SMRES_INVALID_MEDIA);

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	if (pMdiaAtom->m_pDataHandler)
	{
		((CServerDataHandler *) (pMdiaAtom->m_pDataHandler))->ReadyForDownloadUpdate();
	}

smbail:

	return SMResult;
}

