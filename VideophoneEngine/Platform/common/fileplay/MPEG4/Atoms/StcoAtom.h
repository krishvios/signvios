// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STCOATOM_H
#define STCOATOM_H

#include "FullAtom.h"


class StcoAtom_c : public FullAtom_c
{
public:
	
	StcoAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t AddChunk(
		uint64_t unOffset);

	void FreeAtom() override;

	SMResult_t GetChunkOffset(
		uint32_t unChunkIndex,
		uint64_t *pun64Offset) const;

	SMResult_t SetChunkOffset(
		uint32_t unChunkIndex,
		uint64_t un64Offset);

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	uint32_t m_unCount;

private:

	SMResult_t ConvertTo64Bit();

	uint32_t *m_punTable;
	uint64_t *m_pun64Table;
	uint32_t m_unAllocated;
};


#endif // STCOATOM_H

