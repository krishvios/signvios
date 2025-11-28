// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STSCATOM_H
#define STSCATOM_H

#include "FullAtom.h"


typedef struct SampleToChunk_t
{
	uint32_t unFirstChunk;
	uint32_t unSamplesPerChunk;
	uint32_t unSampleDescrIndex;
} SampleToChunk_t;


class StscAtom_c : public FullAtom_c
{
public:
	
	StscAtom_c();

	SMResult_t Parse(
		CDataHandler *pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t AddChunk(
		uint32_t unSamplesPerChunk,
		uint32_t unSampleDescrIndex);

	void FreeAtom() override;

	SMResult_t Write(
		CDataHandler *pDataHandler) override;

	uint32_t m_unFoundIndex;
	uint32_t m_unCurrentSample{0};
	uint32_t m_unLoopStart{0};
	SampleToChunk_t *m_pLastTablePos{nullptr};
	SampleToChunk_t *m_pTable;
	uint32_t m_unCount;

private:

	uint32_t m_unChunkIndex;
	uint32_t m_unAllocated;
};


#endif // STSCATOM_H

