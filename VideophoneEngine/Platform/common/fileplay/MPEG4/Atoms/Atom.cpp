///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	Atom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMCommon.h"

#include "Atom.h"

void MyTest ()
{
	
}

///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
Atom_c::Atom_c()
{
	m_ChildAtoms.pNext = &m_ChildAtoms;
	m_ChildAtoms.pPrev = &m_ChildAtoms;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t Atom_c::AddAtom(
	Atom_c *pAtom)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	
	AtomList_t *pAtomList;
	AtomList_t *pNewAtom;

	pAtomList = &m_ChildAtoms;

	pNewAtom = new AtomList_t;

	ASSERTRT(pNewAtom != nullptr, SMRES_ALLOC_FAILED);
	
	pNewAtom->pAtom = pAtom;
	
	pNewAtom->pNext = pAtomList;
	pNewAtom->pPrev = pAtomList->pPrev;

	pAtomList->pPrev->pNext = pNewAtom;
	pAtomList->pPrev = pNewAtom;

	pAtom->m_pParent = this;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t Atom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	return SMRES_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void Atom_c::FreeAtom()
{
	AtomList_t *pAtomNode;
	AtomList_t *pNextNode = m_ChildAtoms.pNext;

	for (pAtomNode = pNextNode; pAtomNode != &m_ChildAtoms; pAtomNode = pNextNode)
	{
		pAtomNode->pAtom->FreeAtom();
		pNextNode = pAtomNode->pNext;
		delete pAtomNode;
	}

	delete this;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::GetChildrenSize
//
// Abstract:
//
// Returns:
//
uint32_t Atom_c::GetChildrenSize() const
{
	uint32_t unSize = 0;
	AtomList_t *pAtomNode;

	for (pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext)
	{
		unSize += pAtomNode->pAtom->GetSize();
	}

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::GetHeaderSize
//
// Abstract:
//
// Returns:
//
uint32_t Atom_c::GetHeaderSize(
	uint64_t un64Size)
{
	uint32_t unHeaderSize;

	if (un64Size + 2 * sizeof(uint32_t) > 0xFFFFFFFF)
	{
		unHeaderSize = 2 * sizeof(uint32_t) + sizeof(uint64_t);
	}
	else
	{
		unHeaderSize = 2 * sizeof(uint32_t);
	}

	return unHeaderSize;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t Atom_c::GetSize()
{
	uint32_t unSize = 0;

	unSize += GetChildrenSize();

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::WriteHeader
//
// Abstract:
//
// Returns:
//
SMResult_t Atom_c::WriteHeader(
	CDataHandler* pDataHandler,
	uint64_t un64Size)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	if (un64Size > 0xFFFFFFFF)
	{
		SMResult = pDataHandler->DHWrite4(1);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_unType);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite8(un64Size);

		TESTRESULT();
	}
	else
	{
		SMResult = pDataHandler->DHWrite4((uint32_t)un64Size);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_unType);

		TESTRESULT();
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t Atom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	AtomList_t *pAtomNode;
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

	for (pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext)
	{
		SMResult = pAtomNode->pAtom->Write(pDataHandler);

		TESTRESULT();
	}

	//
	// Get end position to check if size was correct.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);


smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::CloseDataHandler
//
// Abstract:
//
// Returns:
//
void Atom_c::CloseDataHandler()
{
	AtomList_t *pAtomNode;

	for (pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext)
	{
		pAtomNode->pAtom->CloseDataHandler();
	}

}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::GetChildAtom
//
// Abstract:
//
// Returns:
//
Atom_c *Atom_c::GetChildAtom(
	unsigned int unType) const
{
	AtomList_t *pAtomNode;
	Atom_c *pChildAtom = nullptr;

	for (pAtomNode = m_ChildAtoms.pNext; pAtomNode != &m_ChildAtoms; pAtomNode = pAtomNode->pNext)
	{
		if (pAtomNode->pAtom->m_unType == unType)
		{
			pChildAtom = pAtomNode->pAtom;

			break;
		}
	}

	return pChildAtom;
}


///////////////////////////////////////////////////////////////////////////////
//; Atom_c::GetParent
//
// Abstract:
//
// Returns:
//
Atom_c *Atom_c::GetParent() const
{
	return m_pParent;
}


Atom_c *Atom_c::GetAncestorAtom(
	uint32_t unType) const
{
	Atom_c *pParent;

	for (pParent = m_pParent; pParent != nullptr; pParent = pParent->m_pParent)
	{
		if (pParent->m_unType == unType)
		{
			break;
		}
	}

	return pParent;
}

