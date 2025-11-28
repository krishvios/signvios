///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	Atom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef ATOM_H
#define ATOM_H


//
// Sorenson includes
//
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMMemory.h"

#include "CDataHandler.h"

#ifdef WIN32
// Windows SDK defines AddAtom as AddAtomA
#ifdef AddAtom
#undef AddAtom
#endif
#endif


class Atom_c;


typedef struct AtomList_t
{
	Atom_c *pAtom;
	AtomList_t *pNext;
	AtomList_t *pPrev;
} AtomList_t;


class Atom_c : public SMMemory_c
{
public:
	Atom_c();
	
	virtual ~Atom_c () = default;
	
	Atom_c (const Atom_c &other) = delete;
	Atom_c (Atom_c &&other) = delete;
	Atom_c &operator = (const Atom_c &other) = delete;
	Atom_c &operator = (Atom_c &&other) = delete;

	virtual SMResult_t Parse(
		CDataHandler* pDataHandler, 
		uint64_t un64Offset,
		uint64_t un64Length);

	virtual void FreeAtom();

	SMResult_t AddAtom(
		Atom_c *pAtom);

	virtual uint32_t GetSize();			// Size of atom header and all children.

	virtual uint32_t GetHeaderSize(		// Size of just atom header
		uint64_t un64Size);

	uint32_t GetChildrenSize() const;			// Size of just children

	virtual SMResult_t WriteHeader(
		CDataHandler* pDataHandler,
		uint64_t un64Size);

	virtual SMResult_t Write(
		CDataHandler* pDataHandler);

	Atom_c *GetChildAtom(
		unsigned int unType) const;

	Atom_c *GetAncestorAtom(
		uint32_t unType) const;

	Atom_c *GetParent() const;

	virtual void CloseDataHandler();

	uint32_t m_unType{0};
	
	Atom_c *m_pParent{nullptr};
	AtomList_t m_ChildAtoms{};
	
private:

};

#endif // ATOM_H

