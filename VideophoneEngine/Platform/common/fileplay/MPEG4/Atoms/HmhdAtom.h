///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	HmhdAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef HMHDATOM_H
#define HMHDATOM_H


#include "FullAtom.h"


class HmhdAtom_c : public FullAtom_c
{
public:

	HmhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

protected:

	uint16_t m_usMaxPDUSize;
	uint16_t m_usAvgPDUSize;
	uint32_t m_unMaxBitRate;
	uint32_t m_unAvgBitRate;
	uint32_t m_unSlidingAvgBitRate;
};


#endif // HMHDATOM_H


