//////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	File Name:	SMCommon.h
//
//  Abstract:	File contains macros, defines, enumerated types, structures,
//				etc. that span all core codecs (MPEG-4, SV3, etc) and all
//				architecture interfaces (QT, VFW, etc).  This header file can
//				be included anywhere and should not contain any architecture
//				interface or core codec specific items.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef SMCOMMON_H
#define SMCOMMON_H

//
// System includes
//
#include <cstdlib>
#include <cstring>

//
// Local includes
//
#include "SMTypes.h"


//
// Macros
//
#define CLIP(v, l, h) ((v) < (l) ? (l) : ((v) > (h) ? (h) : (v)))

#define CLIP255(a) ((((a) & 0xFFFFFF00) == 0) ? (a) : (((a) < 0) ? 0 : 255))

#define MEDIAN(a, b, c) ((a) < (b) ? ((a) < (c) ? ((b) < (c) ? (b) : (c)) : (a)) : ((a) < (c) ? (a) : ((b) < (c) ? (c) : (b))))

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef WIN32
#define ABS(x) abs(x)
#else
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#endif

#define SIGN(x) (((int)(x) >> 31) | 1)

#define APPLY_SIGN(s, v) ((s) >= 0 ? (v) : -(v))

// Divide unsigned integer a by integer b, rounding to nearest integer.
// Ties round toward +infinity.  MPEG "//" for positive a and b.
#define DIVRND_INF_UNSIGNED(a, b) (((a) + ((b) >> 1)) / (b))

// Divide signed integer a by positive integer b, rounding to nearest integer.
// Ties round away from zero.  MPEG "//" for integer a and positive integer b.
#define DIVRND_INF_SIGNED(a, b) ((a) < 0 ? ((a) - ((b) >> 1)) / (b) : ((a) + ((b) >> 1)) / (b))

// Same as DIVRND_INF_SIGNED but for a non-negative power-of-2 divisor, 2^bs = (1 << bs).
// MPEG "//" for power of 2 divisor.
#define DIVRND_INF_SIGNED2(a, bs) ((a) < 0 ? ((a) - (1 << ((bs) - 1))) / (1 << (bs)) : \
											 ((a) + (1 << ((bs) - 1))) / (1 << (bs)))

// Same as DIVRND_INF_UNSIGNED for positive a.  It appears theres no rounding for negative a.
#define DIVRND_POSINF(a, b) ((a) < 0 ? ((a) / (b)) : ((a) + ((b) >> 1)) / (b))

// Divide by 2^n, rounding to nearest integer.  Ties round toward +infinity.
#define RND_POSINF_RSN(x, n) ((1 & ((x) >> (n)-1)) + ((x) >> (n)))

#define DIVRND_NEGINF(a, b) ((a) < 0 ? ((a) - ((b) >> 1)) / (b) : ((a) / (b))

//
// This macro only works with 32 bit unsigned values.
//
#define BYTESWAP(Val) (((Val) >> 8) | ((Val) << 8))

#define DWORDBYTESWAP(Val) (((Val) >> 24) | (((Val) & 0x00FF0000) >> 8) | \
						   (((Val) & 0x0000FF00) << 8) | ((Val) << 24))

#define QWORDBYTESWAP(Val) (((Val) >> 56) | (((Val) & 0x00FF000000000000ULL) >> 40) | \
						   (((Val) & 0x0000FF0000000000ULL) >> 24) | (((Val) & 0x000000FF00000000ULL >> 8)) | \
						   (((Val) & 0x00000000FF000000ULL) << 8) | (((Val) & 0x0000000000FF0000ULL) << 24) | \
						   (((Val) & 0x000000000000FF00ULL) << 40) | ((Val) << 56))

/////////////////////////////////////////////////////////////////////
// BigEndian8
//
// Abstract:
//
// Returns:
//
inline uint64_t BigEndian8(
	uint64_t un64Data)
{
#ifdef SM_LITTLE_ENDIAN
	return QWORDBYTESWAP(un64Data);
#else
	return un64Data;
#endif // SM_LITTLE_ENDIAN
} // BigEndian4


/////////////////////////////////////////////////////////////////////
// BigEndian4
//
// Abstract:
//
// Returns:
//
inline uint32_t BigEndian4(
	uint32_t unData)
{
#ifdef SM_LITTLE_ENDIAN
	return DWORDBYTESWAP(unData);
#else
	return unData;
#endif // SM_LITTLE_ENDIAN
} // BigEndian4


/////////////////////////////////////////////////////////////////////
// BigEndian2
//
// Abstract:
//
// Returns:
//
inline uint16_t BigEndian2(
	uint16_t usData)
{
#ifdef SM_LITTLE_ENDIAN
	return BYTESWAP(usData);
#else
	return usData;
#endif // SM_LITTLE_ENDIAN
} // BigEndian2


#define ASSERTRT(c, r)	\
	if (!(c)) \
	{ \
		SMResult = r; \
		goto smbail; \
	}

#define TESTRESULT() \
	if (SMResult != SMRES_SUCCESS) \
	{ \
		goto smbail; \
	}


//
// Defines
//
#ifndef NULL
#define NULL	0
#endif // NULL


//
// Enumerations
//
enum boolean_e
{
	eFalse = 0,
	eTrue = 1
};

enum BlockSizes_e
{
	eBlock16 = 16,
	eBlock8 = 8,
	eBlock4 = 4
};


///////////////////////////////////////////////////////////////////////////////
//; MaxOf1AndLog2Ceil
//
// Abstract:
//	This function computes i > 0, such that 2^i >= n.
//
// Returns:
//	MAX(1, ceil(log2(n)).
//
inline uint32_t MaxOf1AndLog2Ceil(
	uint32_t unValue)
{
	int i;

	if (unValue <= (1 << 30))					// This test prevents and endless loop
	{											// for 2^30 < n < 2^31.
		for (i = 1; ((unsigned)1 << i) < unValue; i++)
		{
		}
	}
	else
	{
		i = 31;
	}

	return i;
} // MaxOf1AndLog2Ceil


///////////////////////////////////////////////////////////////////////////////
//; Log2Floor
//
// Abstract:
//	This function computes the log base 2 of the input value.
//
// Returns:
//	floor(log2(n)).
//
inline uint32_t Log2Floor(
	uint32_t unValue)
{
	uint32_t i = ~0;

	if (unValue > 0)
	{
		if (unValue < (1 << 30))
		{
			for (i = 0; (unValue >> i) > 1; i++)
			{
			}
		}
		else
		{
			i = 31;
		}
	}

	return i;
}


///////////////////////////////////////////////////////////////////////////////
//; Log2Ceil
//
// Abstract:
//	This function computes the integer greater than or equal to the base two
//  logarithm of the positive integer argument (i.e., the ceiling function of
//  log2(n), for n > 0).  Endless loop will occur if 2^31 < n < 2^32.
//
// Returns:
//	ceil(log2(n)) for n positive, 0 otherwise.
//
inline uint32_t Log2Ceil(
	uint32_t n)
{
	uint32_t i;

	if (n >= 2)
	{
		for(i = 1; ((uint32_t)1 << i) < n; i++)
		{
		}
	}
	else
	{
		i = 0;
	}

	return i;
} // Log2Ceil


////////////////////////////////////////////////////////////////////////////////
//; AliasSymbol
//
// Abstract:
//	The function limits symbol values according to the allowed range.
//	If the input value is out of range, and alias is returned that is in the
//	range.  VLC symbol aliasing allows VLC tables for coding a range of integer
//	values to be the size of the range, rather than twice the size of the range.
//	The smaller VLC is slightly more efficient because of shorter codewords being
//	used for some symbols than are possible without aliasing.
//
// Returns:
//	Unmodifed symbol or its alias.
//
inline int AliasSymbol(
	int nSymbol,					// Input symbol
	int nLow,						// Low end of range
	int nHigh,						// High end of range
	int nRange)						// nRange must be equal to nHigh - nLow + 1
{ 
	if (nSymbol < nLow)
	{
		nSymbol += nRange;
	}
	else if (nSymbol > nHigh)
	{
		nSymbol -= nRange;
	}
	return nSymbol;
}

#endif // SMCOMMON_H
