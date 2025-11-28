///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	StsdAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <cstring>

//
// Sorenson includes
//
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMCommon.h"

#include "StsdAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"
#include "TrakAtom.h"
#include "TkhdAtom.h"

//#include "Objects.h"


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::StsdAtom_c
//
// Abstract:
//
// Returns:
//
StsdAtom_c::StsdAtom_c()
{
	m_unType = eAtom4CCstsd;

	m_unCount = 0;

	m_DescrList.pNext = &m_DescrList;
	m_DescrList.pPrev = &m_DescrList;
} // StsdAtom_c::StsdAtom_c


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t StsdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	SMHandle hDescr = nullptr;
	unsigned int i;
	unsigned int unSize;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

	m_unCount = GetInt32(pDataHandler, un64Offset);

	un64Offset += sizeof(uint32_t);
	un64Length -= sizeof(uint32_t);

	hDescr = SMNewHandle(8);

	ASSERTRT(hDescr != nullptr, SMRES_ALLOC_FAILED);

	for (i = 0; i < m_unCount; i++)
	{
		ASSERTRT(un64Length >= sizeof(uint32_t), SMRES_INVALID_MOVIE);

		SMResult = pDataHandler->DHRead(un64Offset, sizeof(uint32_t), &unSize);

		TESTRESULT();

		unSize = BigEndian4(unSize);

		//
		// Read the rest of the description (subtract off the size and type 32 bit values from the size).
		//
		ASSERTRT(unSize <= un64Length, SMRES_INVALID_MOVIE);

		SMResult = SMSetHandleSize(hDescr, unSize);

		TESTRESULT();
		
		*(uint32_t *)*hDescr = BigEndian4(unSize);

		SMResult = pDataHandler->DHRead(un64Offset + sizeof(uint32_t), unSize - sizeof(uint32_t), *hDescr + sizeof(uint32_t));

		TESTRESULT();

		SMResult = AddDescr(hDescr);

		TESTRESULT();

		un64Offset += unSize;
		un64Length -= unSize;
	}

	ASSERTRT(un64Length == 0, SMRES_INVALID_MOVIE);

smbail:

	if (hDescr)
	{
		SMDisposeHandle(hDescr);

		hDescr = nullptr;
	}
	
	return SMResult;
} // StsdAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t StsdAtom_c::GetSize()
{
	DescrList_t *pDescrNode;
	uint32_t unSize = 0;

	unSize += sizeof(uint32_t);

	for (pDescrNode = m_DescrList.pNext; pDescrNode != &m_DescrList; pDescrNode = pDescrNode->pNext)
	{
		unSize += SMGetHandleSize(pDescrNode->hDescr);
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t StsdAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	DescrList_t *pDescrNode;

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

	//
	// Write sample descriptions.
	//
	for (pDescrNode = m_DescrList.pNext; pDescrNode != &m_DescrList; pDescrNode = pDescrNode->pNext)
	{
		SMResult = pDataHandler->DHWrite(*pDescrNode->hDescr, SMGetHandleSize(pDescrNode->hDescr));

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
} // StsdAtom_c::Write


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void StsdAtom_c::FreeAtom()
{
	DescrList_t *pDescrNode;
	DescrList_t *pDescrNodeNext;

	for (pDescrNode = m_DescrList.pNext; pDescrNode != &m_DescrList; pDescrNode = pDescrNodeNext)
	{
		pDescrNodeNext = pDescrNode->pNext;

		pDescrNode->pNext->pPrev = pDescrNode->pPrev;
		pDescrNode->pPrev->pNext = pDescrNode->pNext;

		if (pDescrNode->hDescr)
		{
			SMDisposeHandle(pDescrNode->hDescr);

			pDescrNode->hDescr = nullptr;
		}

		delete pDescrNode;
	}

	Atom_c::FreeAtom();
} // StsdAtom_c::FreeAtom


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::AddSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t StsdAtom_c::AddSampleDescription(
	SMHandle hSampleDescription,
	unsigned int *punIndex)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = AddDescr(hSampleDescription);

	TESTRESULT();

	*punIndex = ++m_unCount;

smbail:

	return SMResult;
} // StsdAtom_c::AddSampleDescription


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::GetSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t StsdAtom_c::GetSampleDescription(
	uint32_t unIndex,
	SMHandle hSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	DescrList_t *pDescrNode;
	unsigned int i;
	unsigned int unSize;

	i = 1;

	for (pDescrNode = m_DescrList.pNext; pDescrNode != &m_DescrList; pDescrNode = pDescrNode->pNext)
	{
		if (i == unIndex)
		{
			unSize = SMGetHandleSize(pDescrNode->hDescr);

			SMResult = SMSetHandleSize(hSampleDescription, unSize);

			TESTRESULT();

			memcpy(*hSampleDescription, *pDescrNode->hDescr, unSize);

			break;
		}

		i++;
	}

	ASSERTRT(pDescrNode != &m_DescrList, SMRES_INVALID_INDEX);

smbail:

	return SMResult;
} // StsdAtom_c::GetSampleDescription


///////////////////////////////////////////////////////////////////////////////
//; SetSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t StsdAtom_c::SetSampleDescription(
	uint32_t unIndex,
	SMHandle hSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	DescrList_t *pDescrNode;
	unsigned int i;
	unsigned int unSize;

	i = 1;

	for (pDescrNode = m_DescrList.pNext; pDescrNode != &m_DescrList; pDescrNode = pDescrNode->pNext)
	{
		if (i == unIndex)
		{
			unSize = SMGetHandleSize(hSampleDescription);

			SMResult = SMSetHandleSize(pDescrNode->hDescr, unSize);

			TESTRESULT();

			memcpy(*pDescrNode->hDescr, *hSampleDescription, unSize);

			break;
		}

		i++;
	}

smbail:

	return SMResult;
} // StsdAtom_c::SetSampleDescription


///////////////////////////////////////////////////////////////////////////////
//; StsdAtom_c::AddDescr
//
// Abstract:
//
// Returns:
//
SMResult_t StsdAtom_c::AddDescr(
	SMHandle hDescr)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	DescrList_t *pNewDescrList;
	uint32_t unSize;

	//
	// Create the description list node.
	//
	pNewDescrList = new DescrList_t;

	ASSERTRT(pNewDescrList != nullptr, SMRES_ALLOC_FAILED);

	unSize = SMGetHandleSize(hDescr);

	pNewDescrList->hDescr = SMNewHandle(unSize);

	ASSERTRT(pNewDescrList->hDescr != nullptr, SMRES_ALLOC_FAILED);
		
	memcpy(*pNewDescrList->hDescr, *hDescr, unSize);

	//
	// Add the list node to the list.
	//
	pNewDescrList->pNext = &m_DescrList;
	pNewDescrList->pPrev = m_DescrList.pPrev;

	m_DescrList.pPrev->pNext = pNewDescrList;
	m_DescrList.pPrev = pNewDescrList;

smbail:
	if (SMResult != SMRES_SUCCESS &&
		pNewDescrList)
	{
		delete pNewDescrList;
		pNewDescrList = nullptr;
	}

	return SMResult;
} /// StsdAtom_c::AddDescr


