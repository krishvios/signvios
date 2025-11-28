// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef ELSTATOM_H
#define ELSTATOM_H

//
// Sorenson includes.
//
#include "SMTypes.h"

#include "FullAtom.h"


typedef struct EditEntry_t
{
	int64_t n64Duration;
	int64_t n64MediaTime;
	int32_t nMediaRate;
} EditEntry_t;



class ElstAtom_c : public FullAtom_c
{
public:

	ElstAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;
	
	void FreeAtom() override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	EditEntry_t *m_pEntries;
	uint32_t m_unCount;

private:

};

#endif // ELSTATOM_H

