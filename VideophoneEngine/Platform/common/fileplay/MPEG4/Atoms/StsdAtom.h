///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	StsdAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STSDATOM_H
#define STSDATOM_H

#include "FullAtom.h"
#include "SorensonMP4.h"


typedef struct DescrList_t
{
	SMHandle hDescr;
	DescrList_t *pNext;
	DescrList_t *pPrev;
} DescrList_t;


class StsdAtom_c : public FullAtom_c
{
public:

	StsdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t SetDecoderInformation(
		uint32_t unIndex,
		uint32_t unBufferSize,
		uint32_t unMaxBitRate,
		uint32_t unAvgBitRate);

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	SMResult_t AddSampleDescription(
		SMHandle hSampleDescription,
		unsigned int *punIndex);

	SMResult_t GetSampleDescription(
		uint32_t unIndex,
		SMHandle hSampleDescription);

	SMResult_t SetSampleDescription(
		uint32_t unIndex,
		SMHandle hSampleDescription);

private:

	unsigned int m_unCount;
	DescrList_t m_DescrList{};

	SMResult_t FindDescription(
		SMHandle hSampleDescription,
		unsigned int *punIndex);

	SMResult_t AddDescr(
		SMHandle hDescr);
};


#endif // STSDATOM_H

