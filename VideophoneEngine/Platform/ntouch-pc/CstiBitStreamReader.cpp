//*****************************************************************************
//
// FileName:        CstiBitStreamReader.h
//
// Abstract:        A class used to read data one bit or several bits at a time.  Also
//					has support for Golomb encoding.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiBitStreamReader.h"

CstiBitStreamReader::CstiBitStreamReader (const uint8_t *pBuffer, uint32_t nLength) :
	m_pBuffer (pBuffer),
	m_nLength (nLength),
	m_nPosition (0)
{
}

uint32_t CstiBitStreamReader::getBits (std::size_t num)
{
	if (num == 0)
	{
		return 0;
	}
	int remainder = m_nPosition % 8;
	int startindex = m_nPosition / 8;
	uint32_t result = 0;
	if (startindex + 5 < m_nLength)
	{
		assert (num <= 32);
		result = m_pBuffer[startindex] << (24 + remainder) | m_pBuffer[startindex + 1] << (16 + remainder) | m_pBuffer[startindex + 2] << (8 + remainder) | m_pBuffer[startindex + 3] << remainder | m_pBuffer[startindex + 4] >> (8 - remainder);
		//result = (*(uint32_t*)&m_pBuffer[startindex]) << remainder | m_pBuffer[startindex + 5] >> (8 - remainder);
		result = result >> 32 - num;
	}
	else
	{
		result = (*(uint32_t*)&m_pBuffer[m_nLength - 4]);
		int shiftValue = 8 * (m_nLength - startindex);
		assert (num <= 32);
		result = result << shiftValue;
		result = result >> (32 - num);
	}
	skipBits (num);
	return result;
}


void CstiBitStreamReader::skipBits (std::size_t num)
{
	assert ((num + m_nPosition) < (m_nLength * 8));
	m_nPosition += num;
}


uint32_t CstiBitStreamReader::getGolombU ()
{
	long leadingZeroBits = -1;
	for (uint32_t bit = 0; !bit; leadingZeroBits++)
	{
		bit = getBits (1);
	}
	
	if (leadingZeroBits >= 32)
	{
		return 0;
	}
	return (1 << leadingZeroBits) + getBits (leadingZeroBits) - 1;
}

int32_t CstiBitStreamReader::getGolombS ()
{
	int32_t buf = getGolombU ();

	if (buf & 1)
	{
		buf = (buf + 1) >> 1;
	}
	else
	{
		buf = -(buf >> 1);
	}
	return buf;
}

