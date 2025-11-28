///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	TrefAtom.cpp
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
#include "SMMemory.h"

#include "MP4Common.h"
#include "TrefAtom.h"
#include "TkhdAtom.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::TrefAtom_c
//
// Abstract:
//
// Returns:
//
TrefAtom_c::TrefAtom_c()
{
	m_unType = eAtom4CCtref;

	MpodRef.unType = eAtom4CCmpod;
	MpodRef.pTrakRef = nullptr;
	MpodRef.unCount = 0;
	MpodRef.unAllocated = 0;

	DpndRef.unType = eAtom4CCdpnd;
	DpndRef.pTrakRef = nullptr;
	DpndRef.unCount = 0;
	DpndRef.unAllocated = 0;

	SyncRef.unType = eAtom4CCsync;
	SyncRef.pTrakRef = nullptr;
	SyncRef.unCount = 0;
	SyncRef.unAllocated = 0;

	HintRef.unType = eAtom4CChint;
	HintRef.pTrakRef = nullptr;
	HintRef.unCount = 0;
	HintRef.unAllocated = 0;

	IpirRef.unType = eAtom4CCipir;
	IpirRef.pTrakRef = nullptr;
	IpirRef.unCount = 0;
	IpirRef.unAllocated = 0;
} // TrefAtom_c::TrefAtom_c


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t TrefAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t aunData[2];
	uint32_t unSize;
	uint32_t unType;
	Ref_t *pRef;

	for (;;)
	{
		pDataHandler->DHRead(un64Offset, 2 * sizeof(uint32_t), aunData);

		unSize = BigEndian4(aunData[0]);
		unType = BigEndian4(aunData[1]);

		switch (unType)
		{
			case eAtom4CCmpod:

				pRef = &MpodRef;

				break;

			case eAtom4CCdpnd:

				pRef = &DpndRef;

				break;

			case eAtom4CCsync:

				pRef = &SyncRef;

				break;

			case eAtom4CChint:

				pRef = &HintRef;

				break;

			case eAtom4CCipir:

				pRef = &IpirRef;
				// FALLTHRU
			default:

				ASSERTRT(eFalse, SMRES_INVALID_MOVIE);
		}

		ASSERTRT(unSize <= un64Length, SMRES_INVALID_MOVIE);

		pRef->pTrakRef = (uint32_t *)GetDataPtr(pDataHandler, un64Offset + 2 * sizeof(uint32_t), unSize - 2 * sizeof(uint32_t));

		ASSERTRT(pRef->pTrakRef != nullptr, SMRES_ALLOC_FAILED);

		pRef->unCount = (unSize - 2 * sizeof(uint32_t)) / sizeof(uint32_t);

#ifdef SM_LITTLE_ENDIAN
		uint32_t i;
		for (i = 0; i < pRef->unCount; i++)
		{
			pRef->pTrakRef[i] = DWORDBYTESWAP(pRef->pTrakRef[i]);
		}
#endif // SM_LITTLE_ENDIAN

		un64Offset += unSize;
		un64Length -= unSize;

		if (un64Length <= 0)
		{
			break;
		}
	}

smbail:

	return SMResult;
} // TrefAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t TrefAtom_c::GetSize()
{
	uint32_t unSize = 0;

	if (MpodRef.unCount > 0)
	{
		unSize += 2 * sizeof(uint32_t);

		unSize += MpodRef.unCount * sizeof(uint32_t);
	}

	if (DpndRef.unCount > 0)
	{
		unSize += 2 * sizeof(uint32_t);

		unSize += DpndRef.unCount * sizeof(uint32_t);
	}

	if (SyncRef.unCount > 0)
	{
		unSize += 2 * sizeof(uint32_t);

		unSize += SyncRef.unCount * sizeof(uint32_t);
	}

	if (HintRef.unCount > 0)
	{
		unSize += 2 * sizeof(uint32_t);

		unSize += HintRef.unCount * sizeof(uint32_t);
	}

	if (IpirRef.unCount > 0)
	{
		unSize += 2 * sizeof(uint32_t);

		unSize += IpirRef.unCount * sizeof(uint32_t);
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::GetReference
//
// Abstract:
//
// Returns:
//
SMResult_t TrefAtom_c::GetReference(
	uint32_t unRefType,
	uint32_t unRefIndex,
	uint32_t *punTrackID)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Ref_t *pRef;

	switch (unRefType)
	{
		case eMP4ODTrackRefType:

			pRef = &MpodRef;

			break;

		case eMP4DependRefType:

			pRef = &DpndRef;

			break;

		case eMP4SyncRefType:

			pRef = &SyncRef;

			break;

		case eMP4HintRefType:

			pRef = &HintRef;

			break;

		case eMP4IpirRefType:

			pRef = &IpirRef;

			break;

		default:

			ASSERTRT(eFalse, SMRES_INVALID_REFRENCE);
	}

	ASSERTRT(unRefIndex <= pRef->unCount, SMRES_INVALID_REFRENCE);

	*punTrackID = pRef->pTrakRef[unRefIndex - 1];

smbail:

	return SMResult;
} // TrefAtom_c::GetReference


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::AddReference
//
// Abstract:
//
// Returns:
//
SMResult_t TrefAtom_c::AddReference(
	TrakAtom_c *pTrakAtom,
	uint32_t unRefType,
	uint32_t *punRefIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Ref_t *pRef;
	TkhdAtom_c *pTkhdAtom;
	unsigned int i;

	ASSERTRT(pTrakAtom != nullptr, SMRES_INVALID_TRACK);

	pTkhdAtom = (TkhdAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCtkhd);

	ASSERTRT(pTkhdAtom != nullptr, SMRES_INVALID_TRACK);

	switch (unRefType)
	{
		case eMP4ODTrackRefType:

			pRef = &MpodRef;

			break;

		case eMP4DependRefType:

			pRef = &DpndRef;

			break;

		case eMP4SyncRefType:

			pRef = &SyncRef;

			break;

		case eMP4HintRefType:

			pRef = &HintRef;

			break;

		case eMP4IpirRefType:

			pRef = &IpirRef;

			break;

		default:

			ASSERTRT(eFalse, SMRES_INVALID_REFRENCE);
	}

	//
	// Check to see if this track is already referenced.
	//
	if (pRef->pTrakRef == nullptr)
	{
		pRef->pTrakRef = (uint32_t *)SMAllocPtr(sizeof(uint32_t));

		ASSERTRT(pRef->pTrakRef != nullptr, SMRES_ALLOC_FAILED);

		pRef->unAllocated = 1;
		pRef->unCount = 1;

		pRef->pTrakRef[0] = pTkhdAtom->GetTrackID();

		if (punRefIndex)
		{
			*punRefIndex = 1;
		}
	}
	else
	{
		for (i = 0; i < pRef->unCount; i++)
		{
			if (pRef->pTrakRef[i] == pTkhdAtom->GetTrackID())
			{
				break;
			}
		}

		if (i == pRef->unCount)
		{
			//
			// Track was not found in the table so add the track reference.
			//

			pRef->unCount++;

			if (pRef->unCount > pRef->unAllocated)
			{
				pRef->pTrakRef = (uint32_t *)SMRealloc(pRef->pTrakRef, pRef->unCount * sizeof(uint32_t));

				ASSERTRT(pRef->pTrakRef != nullptr, SMRES_ALLOC_FAILED);

				pRef->unAllocated++;
			}

			pRef->pTrakRef[pRef->unCount - 1] = pTkhdAtom->GetTrackID();
		}

		if (punRefIndex)
		{
			*punRefIndex = i + 1;
		}
	}

smbail:

	return SMResult;
} // TrefAtom_c::AddReference


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void TrefAtom_c::FreeAtom()
{
	if (MpodRef.pTrakRef)
	{
		SMFreePtr(MpodRef.pTrakRef);

		MpodRef.pTrakRef = nullptr;
	}

	if (DpndRef.pTrakRef)
	{
		SMFreePtr(DpndRef.pTrakRef);

		DpndRef.pTrakRef = nullptr;
	}

	if (SyncRef.pTrakRef)
	{
		SMFreePtr(SyncRef.pTrakRef);

		SyncRef.pTrakRef = nullptr;
	}

	if (HintRef.pTrakRef)
	{
		SMFreePtr(HintRef.pTrakRef);

		HintRef.pTrakRef = nullptr;
	}

	if (IpirRef.pTrakRef)
	{
		SMFreePtr(IpirRef.pTrakRef);

		IpirRef.pTrakRef = nullptr;
	}

	Atom_c::FreeAtom();
} // TrefAtom_c::FreeAtom


///////////////////////////////////////////////////////////////////////////////
//; WriteTrefType
//
// Abstract:
//
// Returns:
//
static SMResult_t WriteTrefType(
	CDataHandler* pDataHandler,
	Ref_t *pRef)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	unsigned int i;

	if (pRef->unCount > 0)
	{
		//
		// Write the size.
		//
		SMResult = pDataHandler->DHWrite4(2 * sizeof(uint32_t) + pRef->unCount * sizeof(uint32_t));

		TESTRESULT();

		//
		// Write atom type.
		//
		SMResult = pDataHandler->DHWrite4(pRef->unType);

		TESTRESULT();

		//
		// Write atom data.
		//
		for (i = 0; i < pRef->unCount; i++)
		{
			SMResult = pDataHandler->DHWrite4(pRef->pTrakRef[i]);

			TESTRESULT();
		}
	}

smbail:

	return SMResult;
} // WriteTrefType


///////////////////////////////////////////////////////////////////////////////
//; TrefAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t TrefAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Write the atom header.
	//
	SMResult = WriteHeader(pDataHandler, unSize);

	TESTRESULT();

	//
	// Write atom data.
	//
	SMResult = WriteTrefType(pDataHandler, &MpodRef);

	TESTRESULT();

	SMResult = WriteTrefType(pDataHandler, &DpndRef);

	TESTRESULT();

	SMResult = WriteTrefType(pDataHandler, &SyncRef);

	TESTRESULT();

	SMResult = WriteTrefType(pDataHandler, &HintRef);

	TESTRESULT();

	SMResult = WriteTrefType(pDataHandler, &IpirRef);

	TESTRESULT();

	//
	// Get end position.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);

smbail:

	return SMResult;
} // TrefAtom_c::Write


