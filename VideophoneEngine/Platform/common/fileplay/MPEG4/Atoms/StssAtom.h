// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STSSATOM_H
#define STSSATOM_H

#include "FullAtom.h"
#include "SorensonMP4.h"

class StssAtom_c : public FullAtom_c
{
public:
	
	StssAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t AddSyncs(
		SMHandle hSyncSamples,
		uint32_t unSampleCount);

	void FreeAtom() override;

	SMResult_t GetSyncSampleIndex(
		uint32_t sampleIndex,
		uint32_t *pSyncSampleIndex) const;

	SMResult_t IsSampleSync(
		uint32_t unSampleIndex,
		bool_t *pbSyncSample) const;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	uint32_t m_unSampleCount;
	uint32_t *m_punSyncTable;

private:

	uint32_t m_unAllocated;
	uint32_t m_unLastSample;
	uint32_t m_unSyncsToAdd;
};


#endif // STSSATOM_H

