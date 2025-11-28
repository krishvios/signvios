///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	FullAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef FULLATOM_H
#define FULLATOM_H


#include "Atom.h"


class FullAtom_c : public Atom_c
{
public:

	FullAtom_c();
	
	~FullAtom_c () override = default;

	FullAtom_c (const FullAtom_c &other) = delete;
	FullAtom_c (FullAtom_c &&other) = delete;
	FullAtom_c &operator = (const FullAtom_c &other) = delete;
	FullAtom_c &operator = (FullAtom_c &&other) = delete;

	uint32_t GetHeaderSize(
		uint64_t un64Size) override;

	SMResult_t ReadHeader(
		CDataHandler* pDataHandler,
		uint64_t *pun64Offset,
		uint64_t *pun64Length);

	SMResult_t WriteHeader(
		CDataHandler* pDataHandler,
		uint64_t un64Size) override;

	uint32_t m_unVersion;
	uint32_t m_unFlags;
};


#endif // FULLATOM_H


