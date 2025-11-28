//////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	File Name:	SMTypes.h
//
//  Abstract:	File contains typedefs that span all core codecs (MPEG-4, SV3,
//				etc) and all architecture interfaces (QT, VFW, etc).  This
//				header file can be included anywhere and should not contain
//				any architecture interface or core codec specific items.
//				Items other than typedefs (type_t) probably belong in another
//				header file: SMCommon.h
//
//				The definitions here are not intended for the public.  Public
//				definitions should be placed in SorensonTypes.h
//
//////////////////////////////////////////////////////////////////////////////
#ifndef SMTYPES_H
#define SMTYPES_H

#include "SorensonTypes.h"


typedef struct rgbtriple_s 
{
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
} rgbtriple_t;

typedef struct rgbquad_s 
{
	uint8_t rgbAlpha;
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
} rgbquad_t;

typedef struct
{
	uint32_t unYear;
	uint32_t unMonth;
	uint32_t unDay;
	uint32_t unHour;
	uint32_t unMinute;
	uint32_t unSecond;
} DateTime_t;

typedef struct
{
	int nLeft;
	int nTop;
	int nRight;
	int nBottom;
} rect_t;

enum MBSlicePosition_e
{
	eNone			= 0,
	eLeftTop		= 1,
	eTop			= 2,
	eURCorner		= 3,
	eULCorner		= 4
};

//
// Color conversion reccomendations.  These values are used in the calls for 
// initializing the lookup tables.
//
enum ColorRecommendations_e
{
	eRecBT709 = 0,
	eRecFCC = 1,
	eRecBT470_2 = 2,
	eRecSMPTE240M = 3,
};

//
// This enumaration is used for specifying the color ranges and are used for 
// initializing the lookup tables.
//
enum ColorRanges_e
{
	eRange16To235 = 0,
	eRange0To255 = 1,
};

#endif // SMTYPES_H

