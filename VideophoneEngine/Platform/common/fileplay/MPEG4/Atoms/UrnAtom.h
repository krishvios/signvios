///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UrnAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef URNATOM_H
#define URNATOM_H


#include "FullAtom.h"


class UrnAtom_c : public FullAtom_c
{
public:

	UrnAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	void FreeAtom() override;

protected:

	char *m_pszName;
	char *m_pszLocation;
};


#endif // URNATOM_H

