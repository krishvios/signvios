///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UnknownAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef UNKNOWNATOM_H
#define UNKNOWNATOM_H


#include "Atom.h"


class UnknownAtom_c : public Atom_c
{
public:

	UnknownAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

protected:

	uint8_t *m_pucData;
	uint32_t m_unDataLength;

};


#endif // UNKNOWNATOM_H

