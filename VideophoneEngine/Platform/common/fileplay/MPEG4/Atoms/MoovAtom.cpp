// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

//
// Sorenson includes
//
#include "SMCommon.h"
#include "SMTypes.h"
#include "SorensonErrors.h"

#include "MP4Parser.h"
#include "MoovAtom.h"
#include "TkhdAtom.h"


///////////////////////////////////////////////////////////////////////////////
//; MoovAtom_c
//
// Abstract:
//
// Returns:
//
MoovAtom_c::MoovAtom_c()
{
	m_unType = eAtom4CCmoov;
}


///////////////////////////////////////////////////////////////////////////////
//; MoovAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t MoovAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ParseMovie_r(pDataHandler, un64Offset, un64Length, this);

	TESTRESULT();

	m_pMvhd = (MvhdAtom_c *)GetChildAtom(eAtom4CCmvhd);
	m_pIods = (IodsAtom_c *)GetChildAtom(eAtom4CCiods);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MoovAtom_c::UpdateDuration
//
// Abstract:
//
// Returns:
//
SMResult_t MoovAtom_c::UpdateDuration()
{
	SMResult_t SMResult = SMRES_SUCCESS;
	AtomList_t *pAtomNode;
	uint64_t un64Duration = 0;
	TrakAtom_c *pTrakAtom;
	TkhdAtom_c *pTkhdAtom;
	MvhdAtom_c *pMvhdAtom;

	pMvhdAtom = (MvhdAtom_c *)GetChildAtom(eAtom4CCmvhd);

	ASSERTRT(pMvhdAtom != nullptr, SMRES_INVALID_MOVIE);

	//
	// Look for trak child atoms to find the longest duration amongst them.  This duration
	// becomes the movie's duration.
	//
	for (pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext)
	{
		if (pAtomNode->pAtom->m_unType == eAtom4CCtrak)
		{
			pTrakAtom = (TrakAtom_c *)pAtomNode->pAtom;

			pTkhdAtom = (TkhdAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCtkhd);

			ASSERTRT(pTkhdAtom != nullptr, SMRES_INVALID_TRACK);

			if (pTkhdAtom->GetDuration() > un64Duration)
			{
				un64Duration = pTkhdAtom->GetDuration();
			}
		}
	}

	pMvhdAtom->SetDuration(un64Duration);

smbail:

	return SMResult;
}


