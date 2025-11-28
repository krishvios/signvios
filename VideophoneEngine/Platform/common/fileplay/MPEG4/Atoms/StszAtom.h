// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STSZATOM_H
#define STSZATOM_H

#include "FullAtom.h"


class StszAtom_c : public FullAtom_c
{
public:
	
	StszAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;
	
	uint32_t GetSize() override;

	SMResult_t AddSampleSize(
		uint32_t unSize);

	void FreeAtom() override;

	SMResult_t GetSampleCount(
		uint32_t *punSampleCount) const;

	SMResult_t GetDefaultSize(
		uint32_t *punDefaultSize) const;

	SMResult_t GetSampleSize(
		uint32_t unSampleIndex,
		uint32_t *punSampleSize) const;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint32_t m_unDefaultSize;
	uint32_t m_unSampleCount;
	uint32_t *m_punSizeTable;
	uint32_t m_unAllocated;
};


#endif // STSZATOM_H


