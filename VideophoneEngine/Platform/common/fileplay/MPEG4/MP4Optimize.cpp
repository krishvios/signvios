///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Optimize.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include "SorensonTypes.h"
#include "SorensonErrors.h"
#include "SMCommon.h"

#include "SorensonMP4.h"
#include "MP4Common.h"

#include "MdiaAtom.h"
#include "StcoAtom.h"
#include "StblAtom.h"
#include "MinfAtom.h"
#include "StscAtom.h"
#include "StszAtom.h"
#include "SttsAtom.h"



typedef struct
{
	Media_t *pMedia;
	MP4TimeScale TimeScale;
	StcoAtom_c *pStcoAtom;
	StscAtom_c *pStscAtom;
	StszAtom_c *pStszAtom;
	SttsAtom_c *pSttsAtom;
	StcoAtom_c *pNewStco;
	uint32_t unSampleIndex;
	uint32_t unMaxSampleIndex;
} TrackData_t;


///////////////////////////////////////////////////////////////////////////////
//; GetMediaAtoms
//
// Abstract:
//
// Returns:
//
static SMResult_t GetMediaAtoms(
	Media_t *pMedia,
	StcoAtom_c **ppStcoAtom,
	StscAtom_c **ppStscAtom,
	StszAtom_c **ppStszAtom,
	SttsAtom_c **ppSttsAtom)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdiaAtom;
	MinfAtom_c *pMinfAtom;
	StblAtom_c *pStblAtom;
	StcoAtom_c *pStcoAtom;
	StszAtom_c *pStszAtom;
	StscAtom_c *pStscAtom;
	SttsAtom_c *pSttsAtom;

	*ppStcoAtom = nullptr;

	pMdiaAtom = (MdiaAtom_c *)pMedia;

	pMinfAtom = (MinfAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCminf);

	ASSERTRT(pMinfAtom != nullptr, SMRES_INVALID_MEDIA);

	pStblAtom = (StblAtom_c *)pMinfAtom->GetChildAtom(eAtom4CCstbl);

	ASSERTRT(pStblAtom != nullptr, SMRES_INVALID_MEDIA);

	pStcoAtom = (StcoAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstco);

	ASSERTRT(pStcoAtom != nullptr, SMRES_INVALID_MEDIA);

	pStszAtom = (StszAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsz);

	ASSERTRT(pStszAtom != nullptr, SMRES_INVALID_MEDIA);

	pStscAtom = (StscAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstsc);

	ASSERTRT(pStscAtom != nullptr, SMRES_INVALID_MEDIA);

	pSttsAtom = (SttsAtom_c *)pStblAtom->GetChildAtom(eAtom4CCstts);

	ASSERTRT(pSttsAtom != nullptr, SMRES_INVALID_MEDIA);

	*ppStcoAtom = pStcoAtom;
	*ppStscAtom = pStscAtom;
	*ppStszAtom = pStszAtom;
	*ppSttsAtom = pSttsAtom;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4OptimizeMovieFile
//
// Abstract:
//
// Returns:
//
SMResult_t MP4OptimizeAndWriteMovieFile(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrackData_t *pTrackData = nullptr;
	TrackData_t *pSelTrackData;
	uint32_t unNumTracks;
	uint32_t i;
	Track_t *pTrack;
	Media_t *pMedia;
	StcoAtom_c *pStcoAtom = nullptr;
	StscAtom_c *pStscAtom = nullptr;
	StszAtom_c *pStszAtom = nullptr;
	SttsAtom_c *pSttsAtom = nullptr;
	uint32_t un32MoovLength;
	uint64_t un64Offset;
	uint32_t j;
	uint32_t un32CurrentSampleIndex;

	un32MoovLength = pMovie->pMoovAtom->GetSize();

	un32MoovLength += pMovie->pFtypAtom->GetSize();

	//
	// Find all the chunk offset atoms.
	//
	SMResult = MP4GetTrackCount(pMovie, &unNumTracks);

	TESTRESULT();

	pTrackData = (TrackData_t *)SMAllocPtr(unNumTracks * sizeof(TrackData_t));

	ASSERTRT(pTrackData != nullptr, SMRES_ALLOC_FAILED);

	for (i = 1; i <= unNumTracks; i++)
	{
		MP4GetTrackByIndex(pMovie, i, &pTrack);

		MP4GetTrackMedia(pTrack, &pMedia);

		GetMediaAtoms(pMedia, &pStcoAtom, &pStscAtom, &pStszAtom, &pSttsAtom);

		pTrackData[i - 1].pMedia = pMedia;
		MP4GetMediaTimeScale(pMedia, &pTrackData[i - 1].TimeScale);
		pTrackData[i - 1].pStcoAtom = pStcoAtom;
		pTrackData[i - 1].pStscAtom = pStscAtom;
		pTrackData[i - 1].pStszAtom = pStszAtom;
		pTrackData[i - 1].unSampleIndex = 1;

		MP4GetMediaSampleCount(pMedia, &pTrackData[i - 1].unMaxSampleIndex);
	}

	//
	// Copy chunks from the old movie to the new movie.
	//
	for (j = 0; j < unNumTracks; j++)
	{
		pSelTrackData = &pTrackData[j];

		for (un32CurrentSampleIndex = 0; 
			 un32CurrentSampleIndex < pTrackData[j].unMaxSampleIndex;
			 un32CurrentSampleIndex++)
		{
			// Get the sample offest.
			pSelTrackData->pStcoAtom->GetChunkOffset(un32CurrentSampleIndex, 
													 &un64Offset);

			un64Offset += un32MoovLength;

			//
			// Set the offset correctly.
			//
			pSelTrackData->pStcoAtom->SetChunkOffset(un32CurrentSampleIndex, 
													 un64Offset);
		}

	}

	// Update the offsets in the ring buffer.
	SMResult = (pMovie->pDataHandler)->DHAdjustOffsets(un32MoovLength);
	TESTRESULT();

	SMResult = (pMovie->pDataHandler)->DHCreatePrefixBuffer(un32MoovLength,
															pMovie->pMoovAtom->GetSize());
	TESTRESULT();

	SMResult = MP4MovieWriteMoovAtom(pMovie);
	TESTRESULT();

	SMResult = (pMovie->pDataHandler)->DHCreatePrefixBuffer(pMovie->pFtypAtom->GetSize(),
															pMovie->pFtypAtom->GetSize());
	TESTRESULT();

	SMResult = MP4MovieWriteFtypAtom(pMovie);
	TESTRESULT();

smbail:

	if (pTrackData)
	{
		SMFreePtr(pTrackData);
		
		pTrackData = nullptr;
	}

	return SMResult;
}
