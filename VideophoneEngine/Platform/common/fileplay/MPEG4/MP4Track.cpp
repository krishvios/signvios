///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Track.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes
//
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMCommon.h"

#include "SorensonMP4.h"
#include "ElstAtom.h"
#include "TrakAtom.h"
#include "TkhdAtom.h"
#include "MdiaAtom.h"
#include "MdhdAtom.h"
#include "StblAtom.h"
#include "StsdAtom.h"
#include "HdlrAtom.h"
#include "MinfAtom.h"
#include "VmhdAtom.h"
#include "SmhdAtom.h"
#include "NmhdAtom.h"
#include "HmhdAtom.h"
#include "DinfAtom.h"
#include "DrefAtom.h"
#include "SttsAtom.h"
#include "CttsAtom.h"
#include "StszAtom.h"
#include "StscAtom.h"
#include "StcoAtom.h"
#include "StssAtom.h"
#include "TrefAtom.h"
#include "UdtaAtom.h"
#include "UrlAtom.h"
#include "MP4Common.h"

///////////////////////////////////////////////////////////////////////////////
//; MP4NewTrackMedia
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewTrackMedia(
	Track_t *pTrack,
	uint32_t unMediaType,
	Media_t **ppMedia)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia = nullptr;
	MdhdAtom_c *pMdhd;
	TrakAtom_c *pTrak;
	StblAtom_c *pStbl;
	StsdAtom_c *pStsd;
	Movie_t *pMovie;
	HdlrAtom_c *pHdlr;
	MinfAtom_c *pMinf;
	VmhdAtom_c *pVmhd;
	SmhdAtom_c *pSmhd;
	NmhdAtom_c *pNmhd;
	HmhdAtom_c *pHmhd;
	DinfAtom_c *pDinf;
	DrefAtom_c *pDref;
	SttsAtom_c *pStts;
	CttsAtom_c *pCtts;
	StszAtom_c *pStsz;
	StscAtom_c *pStsc;
	StcoAtom_c *pStco;
	StssAtom_c *pStss;
	UrlAtom_c *pUrl;

	ASSERTRT(pTrack != nullptr, SMRES_INVALID_MOVIE);

	pTrak = (TrakAtom_c *)pTrack;

	//
	// Create the media atom.
	//
	pMdia = new MdiaAtom_c;

	ASSERTRT(pMdia != nullptr, SMRES_ALLOC_FAILED);

	//
	// Create the media header atom.
	//
	pMdhd = new MdhdAtom_c;

	ASSERTRT(pMdhd != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pMdia->AddAtom(pMdhd);

	TESTRESULT();

	//
	// Create the handler atom.
	//
	pHdlr = new HdlrAtom_c;

	ASSERTRT(pHdlr != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pMdia->AddAtom(pHdlr);

	TESTRESULT();

	switch (unMediaType)
	{
		case eMediaTypeVisual:

			pHdlr->SetHandlerType(eMediaTypeVisual, "vide");

			break;

		case eMediaTypeAudio:

			pHdlr->SetHandlerType(eMediaTypeAudio, "soun");

			break;

		case eMediaTypeScene:

			pHdlr->SetHandlerType(eMediaTypeScene, "sdsm");

			break;

		case eMediaTypeObject:

			pHdlr->SetHandlerType(eMediaTypeObject, "odsm");

			break;

		case eMediaTypeHint:

			pHdlr->SetHandlerType(eMediaTypeHint, "hint");

			break;

		default:

			ASSERTRT(eFalse, SMRES_INVALID_MEDIA_TYPE);
	}

	//
	// Create the media information atom
	//
	pMinf = new MinfAtom_c;

	ASSERTRT(pMinf != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pMdia->AddAtom(pMinf);

	TESTRESULT();

	//
	// Create the media information header atom.
	//
	switch (unMediaType)
	{
		case eMediaTypeVisual:

			pVmhd = new VmhdAtom_c;

			ASSERTRT(pVmhd != nullptr, SMRES_ALLOC_FAILED);

			SMResult = pMinf->AddAtom(pVmhd);

			TESTRESULT();

			break;

		case eMediaTypeAudio:

			pSmhd = new SmhdAtom_c;

			ASSERTRT(pSmhd != nullptr, SMRES_ALLOC_FAILED);

			SMResult = pMinf->AddAtom(pSmhd);

			TESTRESULT();

			break;

		case eMediaTypeScene:
		case eMediaTypeObject:

			pNmhd = new NmhdAtom_c;

			ASSERTRT(pNmhd != nullptr, SMRES_ALLOC_FAILED);

			SMResult = pMinf->AddAtom(pNmhd);

			TESTRESULT();

			break;

		case eMediaTypeHint:

			pHmhd = new HmhdAtom_c;

			ASSERTRT(pHmhd != nullptr, SMRES_ALLOC_FAILED);

			SMResult = pMinf->AddAtom(pHmhd);

			TESTRESULT();

			break;

		default:

			ASSERTRT(eFalse, SMRES_INVALID_MEDIA_TYPE);
	}

	//
	// Create the data information atom.
	//
	pDinf = new DinfAtom_c;

	ASSERTRT(pDinf != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pMinf->AddAtom(pDinf);

	TESTRESULT();

	//
	// Create the data reference atom.
	//
	pDref = new DrefAtom_c;

	ASSERTRT(pDref != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pDinf->AddAtom(pDref);

	TESTRESULT();

	//
	// Create url atom.
	//
	pUrl = new UrlAtom_c;

	ASSERTRT(pUrl != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pDref->AddAtom(pUrl);

	TESTRESULT();

	pUrl->m_unFlags = 1;

	//
	// Create the sample table atom.
	//
	pStbl = new StblAtom_c;

	ASSERTRT(pStbl != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pMinf->AddAtom(pStbl);

	TESTRESULT();

	//
	// Create the sample description atom.
	//
	pStsd = new StsdAtom_c;

	ASSERTRT(pStsd != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pStsd);

	TESTRESULT();

	//
	// Create the time-to-sample atom.
	//
	pStts = new SttsAtom_c;

	ASSERTRT(pStts != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pStts);

	TESTRESULT();

	//
	// Create the composition time-to-sample atom.
	//
	pCtts = new CttsAtom_c;

	ASSERTRT(pCtts != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pCtts);

	TESTRESULT();

	//
	// Create the sample-to-chunk atom.
	//
	pStsc = new StscAtom_c;

	ASSERTRT(pStsc != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pStsc);

	TESTRESULT();

	//
	// Create the sample size atom.
	//
	pStsz = new StszAtom_c;

	ASSERTRT(pStsz != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pStsz);

	TESTRESULT();

	//
	// Create the chunk offset atom.
	//
	pStco = new StcoAtom_c;

	ASSERTRT(pStco != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pStco);

	TESTRESULT();

	//
	// Create the sync sample atom.
	//
	pStss = new StssAtom_c;

	ASSERTRT(pStss != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pStbl->AddAtom(pStss);

	TESTRESULT();

	//
	// Let the media atom know about the movies default data handler.
	//
	pMovie = pTrak->GetMovie();

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);

	SMResult = pMdia->SetDataHandler(pMovie->pDataHandler);

	TESTRESULT();

	//
	// Add the media as a child of the track.
	//
	SMResult = pTrak->AddAtom(pMdia);

	TESTRESULT();

	*ppMedia = (Media_t *)pMdia;

smbail:
	if (SMResult != SMRES_SUCCESS &&
		pMdia != nullptr)
	{
		pMdia->FreeAtom();
	}

	return SMResult;
} // MP4NewTrackMedia


///////////////////////////////////////////////////////////////////////////////
//; MP4AddTrackReference
//
// Abstract:
//
// Returns:
//
SMResult_t MP4AddTrackReference(
	Track_t *pTrack,
	Track_t *pRefTrack,
	uint32_t unRefType,
	uint32_t *punRefIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrakAtom;
	TrakAtom_c *pRefTrakAtom;
	TrefAtom_c *pTrefAtom;

	ASSERTRT(pTrack != nullptr, SMRES_INVALID_TRACK);

	pTrakAtom = (TrakAtom_c *)pTrack;

	ASSERTRT(pRefTrack != nullptr, SMRES_INVALID_TRACK);

	pRefTrakAtom = (TrakAtom_c *)pRefTrack;

	pTrefAtom = (TrefAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCtref);

	if (pTrefAtom == nullptr)
	{
		pTrefAtom = new TrefAtom_c;

		ASSERTRT(pTrefAtom != nullptr, SMRES_ALLOC_FAILED);

		SMResult = pTrakAtom->AddAtom(pTrefAtom);

		TESTRESULT();
	}

	SMResult = pTrefAtom->AddReference(pRefTrakAtom, unRefType, punRefIndex);

	TESTRESULT();

smbail:

	return SMResult;
} // MP4AddTrackReference


///////////////////////////////////////////////////////////////////////////////
//; MP4InsertMediaIntoTrack
//
// Abstract:
//
// Returns:
//
SMResult_t MP4InsertMediaIntoTrack(
	Track_t *pTrack,
	uint32_t unTrackStart,
	uint32_t unMediaTime,
	uint64_t un64MediaDuration,
	uint32_t unMediaRate)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrakAtom;
	TkhdAtom_c *pTkhdAtom;
	MoovAtom_c *pMoovAtom;
	MvhdAtom_c *pMvhdAtom;
	MdiaAtom_c *pMdiaAtom;
	MdhdAtom_c *pMdhdAtom;

	ASSERTRT(pTrack != nullptr, SMRES_INVALID_TRACK);

	pTrakAtom = (TrakAtom_c *)pTrack;

	pTkhdAtom = (TkhdAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCtkhd);

	ASSERTRT(pTkhdAtom != nullptr, SMRES_INVALID_TRACK);

	pMoovAtom = (MoovAtom_c *)pTrakAtom->GetParent();

	ASSERTRT(pMoovAtom != nullptr, SMRES_INVALID_TRACK);

	pMvhdAtom = (MvhdAtom_c *)pMoovAtom->GetChildAtom(eAtom4CCmvhd);

	ASSERTRT(pMvhdAtom != nullptr, SMRES_INVALID_TRACK);

	pMdiaAtom = (MdiaAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCmdia);

	ASSERTRT(pMdiaAtom != nullptr, SMRES_INVALID_TRACK);

	pMdhdAtom = (MdhdAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CCmdhd);

	ASSERTRT(pMdhdAtom != nullptr, SMRES_INVALID_TRACK);

	//
	// This function should cause the edit list to become modified.
	//

	pTkhdAtom->SetDuration(un64MediaDuration * pMvhdAtom->GetTimeScale() / pMdhdAtom->GetTimeScale());

	pMoovAtom->UpdateDuration();

smbail:

	return SMResult;
} // MP4InsertMediaIntoTrack


///////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackReference
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackReference(
	Track_t *pTrack,
	uint32_t unType,
	uint32_t unIndex,
	uint32_t *punRef)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrak;
	TrefAtom_c *pTref;

	pTrak = (TrakAtom_c *)pTrack;

	pTref = (TrefAtom_c *)pTrak->GetChildAtom(eAtom4CCtref);

	ASSERTRT(pTref != nullptr, SMRES_INVALID_MOVIE);

	SMResult = pTref->GetReference(unType, unIndex, punRef);

	TESTRESULT();

smbail:

	return SMResult;
} // MP4GetTrackReference


//////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackDuration
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackDuration(
	Track_t *pTrack,
	MP4TimeValue *pDuration)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrak;
	TkhdAtom_c *pTrackHeader;

	pTrak = (TrakAtom_c *)pTrack;

	pTrackHeader = (TkhdAtom_c *)pTrak->GetChildAtom(eAtom4CCtkhd);

	ASSERTRT(pTrackHeader != nullptr, SMRES_INVALID_MOVIE);

	*pDuration = pTrackHeader->GetDuration();

smbail:

	return SMResult;
} // MP4GetTrackDuration


//////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackDimensions
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackDimensions(
	Track_t *pTrack,
	uint32_t *punWidth,
	uint32_t *punHeight)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrak;
	TkhdAtom_c *pTrackHeader;

	pTrak = (TrakAtom_c *)pTrack;

	pTrackHeader = (TkhdAtom_c *)pTrak->GetChildAtom(eAtom4CCtkhd);

	ASSERTRT(pTrackHeader != nullptr, SMRES_INVALID_MOVIE);

	pTrackHeader->GetDimensions(punWidth, punHeight);

smbail:

	return SMResult;
} // MP4GetTrackDimensions


//////////////////////////////////////////////////////////////////////////////
//; MP4TrackTimeToMediaTime
//
// Abstract:
//
// Returns:
//
SMResult_t MP4TrackTimeToMediaTime(
	Track_t *pTrack,
	MP4TimeValue TrackTime,
	MP4TimeValue *pMediaTime)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Movie_t *pMovie;
	Media_t *pMedia;
	TrakAtom_c *pTrak;
	MoovAtom_c *pMoov;
	MvhdAtom_c *pMovieHeader;
	ElstAtom_c *pEditList;
	MP4TimeValue MediaTime = -1;
	MP4TimeScale MediaTimeScale;
	MP4TimeScale TrackTimeScale;
	MP4TimeValue CurrentTime;
	EditEntry_t *pTable;
	unsigned int i;
	MP4TimeValue MediaDuration;

	MP4GetTrackMedia(pTrack, &pMedia);

	pTrak = (TrakAtom_c *)pTrack;

	pMovie = pTrak->GetMovie();

	pMoov = pMovie->pMoovAtom;

	pMovieHeader = pMoov->m_pMvhd;

	ASSERTRT(pMovieHeader != nullptr, SMRES_INVALID_MOVIE);

	TrackTimeScale = pMovieHeader->GetTimeScale();

	MP4GetMediaTimeScale(pMedia, &MediaTimeScale);

	pEditList = (ElstAtom_c *)pTrak->GetChildAtom(eAtom4CCelst);

	if (pEditList)
	{
		pTable = pEditList->m_pEntries;

		CurrentTime = 0;

		for (i = 0; i < pEditList->m_unCount; i++)
		{
			if (CurrentTime + pTable->n64Duration > TrackTime)
			{
				if (pTable->nMediaRate == 0)
				{
					MediaTime = pTable->n64MediaTime;
				}
				else
				{
					ASSERTRT(pTable->nMediaRate == 1, SMRES_INVALID_MOVIE);

					MediaTime = ((TrackTime - CurrentTime) * MediaTimeScale / TrackTimeScale)
								+ pTable->n64MediaTime;
				}

				break;
			}

			CurrentTime += pTable->n64Duration;
		}
	}
	else
	{
		MediaTime = TrackTime * MediaTimeScale / TrackTimeScale;

		MP4GetMediaDuration(pMedia, &MediaDuration);

		if (MediaTime >= (signed)MediaDuration)
		{
			MediaTime = -1;
		}
	}

	*pMediaTime = MediaTime;

smbail:

	return SMResult;
} // MP4TrackTimeToMediaTime


///////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackMedia
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackMedia(
	Track_t *pTrack,
	Media_t **ppMedia)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MdiaAtom_c *pMdia;
	TrakAtom_c *pTrak;

	pTrak = (TrakAtom_c *)pTrack;

	pMdia = (MdiaAtom_c *)pTrak->GetChildAtom(eAtom4CCmdia);

	*ppMedia = (Media_t *)pMdia;

	return SMResult;
} // MP4GetTrackMedia


///////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackByID
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackByID(
	Movie_t *pMovie,
	unsigned int unTrackID,
	Track_t **ppTrack)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Track_t *pTrack = nullptr;
	uint32_t unNumTracks;
	uint32_t unID;
	uint32_t i;

	SMResult = MP4GetTrackCount(pMovie, &unNumTracks);

	TESTRESULT();

	for (i = 1; i <= unNumTracks; i++)
	{
		SMResult = MP4GetTrackByIndex(pMovie, i, &pTrack);

		TESTRESULT();

		if (pTrack != nullptr)
		{
			SMResult = MP4GetTrackID(pTrack, &unID);

			TESTRESULT();

			if (unID == unTrackID)
			{
				break;
			}

			pTrack = nullptr;
		}
	}

	*ppTrack = pTrack;

smbail:

	return SMResult;
} // MP4GetTrackByID


///////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackID
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackID(
	Track_t *pTrack,
	uint32_t *punTrackID)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrak;
	TkhdAtom_c *pTkhd;

	pTrak = (TrakAtom_c *)pTrack;

	pTkhd = (TkhdAtom_c *)pTrak->GetChildAtom(eAtom4CCtkhd);

	ASSERTRT(pTkhd != nullptr, SMRES_INVALID_MOVIE);

	*punTrackID = pTkhd->GetTrackID();
	
smbail:

	return SMResult;
} // MP4GetTrackID


///////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackByIndex
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackByIndex(
	Movie_t *pMovie,
	unsigned int unIndex,
	Track_t **ppTrack)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	AtomList_t *pAtom;
	Atom_c *pMoov;
	unsigned int unCount = 1;
	Atom_c *pTrack = nullptr;
	
	pMoov = pMovie->pMoovAtom;

	if (pMoov)
	{
		for (pAtom = pMoov->m_ChildAtoms.pNext; pAtom != &pMoov->m_ChildAtoms; pAtom = pAtom->pNext)
		{
			if (pAtom->pAtom->m_unType == eAtom4CCtrak)
			{
				if (unIndex == unCount)
				{
					pTrack = pAtom->pAtom;

					break;
				}

				unCount++;
			}
		}
	}

	*ppTrack = (Track_t *)pTrack;

	return SMResult;
} // MP4GetTrackByIndex


///////////////////////////////////////////////////////////////////////////////
//; MP4GetTrackCount
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetTrackCount(
	Movie_t *pMovie,
	uint32_t *punTrackCount)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	AtomList_t *pAtom;
	Atom_c *pMoov;
	unsigned int unCount = 0;

	pMoov = pMovie->pMoovAtom;

	ASSERTRT(pMoov != nullptr, SMRES_INVALID_MOVIE);

	for (pAtom = pMoov->m_ChildAtoms.pNext; pAtom != &pMoov->m_ChildAtoms; pAtom = pAtom->pNext)
	{
		if (pAtom->pAtom->m_unType == eAtom4CCtrak)
		{
			unCount++;
		}
	}

	*punTrackCount = unCount;

smbail:

	return SMResult;
} // MP4GetTrackCount


///////////////////////////////////////////////////////////////////////////////
//; MP4AddTrackUserData
//
// Abstract:
//
// Returns:
//
SMResult_t MP4AddTrackUserData(
	Track_t *pTrack,
	uint32_t unType,
	const uint8_t *pucData,
	uint32_t unLength)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrakAtom;
	UdtaAtom_c *pUdtaAtom;

	ASSERTRT(pTrack != nullptr, SMRES_INVALID_MOVIE);

	pTrakAtom = (TrakAtom_c *)pTrack;

	pUdtaAtom = (UdtaAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCudta);

	if (pUdtaAtom == nullptr)
	{
		pUdtaAtom = new UdtaAtom_c;

		ASSERTRT(pUdtaAtom != nullptr, SMRES_ALLOC_FAILED);

		pTrakAtom->AddAtom(pUdtaAtom);
	}

	pUdtaAtom->AddUserData(unType, pucData, unLength);

smbail:

	return SMResult;
} // MP4AddTrackUserData



