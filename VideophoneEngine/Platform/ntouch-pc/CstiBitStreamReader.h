//*****************************************************************************
//
// FileName:        CstiBitStreamReader.cpp
//
// Abstract:        A class used to read data one bit or several bits at a time.  Also
//					has support for Golomb encoding.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <assert.h>

class CstiBitStreamReader
{
public:
	CstiBitStreamReader (const uint8_t *pBuffer, uint32_t nLength);
	uint32_t getBits (std::size_t nCount);
	void skipBits (std::size_t nCount);

	uint32_t getGolombU ();
	int32_t getGolombS ();

private:
	const uint8_t              *m_pBuffer;
	uint32_t                 m_nLength;
	uint64_t                m_nPosition;
};

