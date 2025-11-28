// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STTSATOM_H
#define STTSATOM_H

#include "FullAtom.h"


typedef struct DeltaEntry_t
{
	uint32_t unEntryCount;
	uint32_t unDelta;
} DeltaEntry_t;


class SttsAtom_c : public FullAtom_c
{
public:
	
	SttsAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;
	
	uint32_t GetSize() override;

	SMResult_t AddSampleDelta(
		uint32_t unDelta);

	void FreeAtom() override;

	uint64_t GetDuration() const;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	DeltaEntry_t *m_punDeltaTable;
	uint32_t m_unCount;

	MP4TimeValue FoundTime;
	MP4TimeValue CurrentTime;
	DeltaEntry_t *pTable{nullptr};
	unsigned int unSampleNumber;
	unsigned int unLoopStart;

private:

};


#endif // STTSATOM_H


