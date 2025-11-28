///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UrlAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef URLATOM_H
#define URLATOM_H

#include "FullAtom.h"


class UrlAtom_c : public FullAtom_c
{
public:

	UrlAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	void FreeAtom() override;

protected:

	char *m_pszLocation;
};


#endif // URLATOM_H

