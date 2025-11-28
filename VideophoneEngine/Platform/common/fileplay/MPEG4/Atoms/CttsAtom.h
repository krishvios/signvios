// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CTTSATOM_H
#define CTTSATOM_H

#include "FullAtom.h"


typedef struct CttsEntry_t
{
	uint32_t unEntryCount;
	uint32_t unDelta;
} CttsEntry_t;


class CttsAtom_c : public FullAtom_c
{
public:
	
	CttsAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;
	
	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t AddSampleDelta(
		uint32_t unDelta);

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint32_t m_unCount;
	CttsEntry_t *m_punDeltaTable;
};


#endif // CTTSATOM_H

