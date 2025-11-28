///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	DrefAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

//
// Sorenson includes.
//
#include "SMCommon.h"
#include "SorensonErrors.h"
#include "SMTypes.h"

#include "DrefAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
DrefAtom_c::DrefAtom_c()
{
	m_unType = eAtom4CCdref;
}


///////////////////////////////////////////////////////////////////////////////
//; DrefAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t DrefAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t i;
	AtomList_t *pChild;
	uint32_t unCount;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	unCount = GetInt32(pDataHandler, un64Offset);

	SMResult = ParseMovie_r(pDataHandler, un64Offset + sizeof(uint32_t), un64Length - sizeof(uint32_t), this);

	TESTRESULT();

	//
	// Count the children.
	//
	for (i = 0, pChild = m_ChildAtoms.pNext; pChild != &m_ChildAtoms; pChild = pChild->pNext, i++)
	{
	}

	//
	// Make sure the number of children match the count stored in this atom.
	//
	ASSERTRT(i == unCount, SMRES_INVALID_MOVIE);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; DrefAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t DrefAtom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += sizeof(uint32_t);

	unSize += GetChildrenSize();

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; DrefAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t DrefAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	AtomList_t *pAtomNode;
	uint32_t unCount;

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
	// Count the children.
	//
	for (unCount = 0, pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext, unCount++)
	{
	}

	SMResult = pDataHandler->DHWrite4(unCount);

	TESTRESULT();

	//
	// Write child atoms.
	//
	for (pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext)
	{
		SMResult = pAtomNode->pAtom->Write(pDataHandler);

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


