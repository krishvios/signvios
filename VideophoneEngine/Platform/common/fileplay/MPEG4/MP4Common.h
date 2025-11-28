///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Common.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef MP4COMMON_H
#define MP4COMMON_H


//
// Sorenson includes
//
#include "SMTypes.h"

#include "MoovAtom.h"
#include "MdatAtom.h"
#include "CDataHandler.h"
#include "stiOS.h"


typedef struct FileSpace_t
{
	uint32_t unType;
	uint64_t un64Offset;
	uint64_t un64Length;

	FileSpace_t *pPrev;
	FileSpace_t *pNext;
} FileSpace_t;


typedef struct Movie_t
{
	MoovAtom_c *pMoovAtom;
	FtypAtom_c *pFtypAtom;

	CDataHandler* pDataHandler;

	FileSpace_t FileSpaceList;
	
	stiMUTEX_ID Mutex;
} Movie_t;


void MP4GetCurrentTime(
	uint64_t *pun64Time);


char *GetDataPtr(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint32_t unSize);


int8_t GetInt8(
	CDataHandler* pDataHandler,
	uint64_t un64Offset);


int16_t GetInt16(
	CDataHandler* pDataHandler,
	uint64_t un64Offset);


int32_t GetInt24(
	CDataHandler* pDataHandler,
	uint64_t un64Offset);


int32_t GetInt32(
	CDataHandler* pDataHandler,
	uint64_t un64Offset);


int64_t GetInt64(
	CDataHandler* pDataHandler,
	uint64_t un64Offset);


SMResult_t WriteMediaData(
	Movie_t *pMovie,
	const uint8_t *pData,
	uint32_t unLength,
	uint64_t *pun64Offset);


SMResult_t WriteFileType(
	const Movie_t *pMovie);


SMResult_t WriteMetaData(
	Movie_t *pMovie);


SMResult_t WriteFileSpaceSizes(
	const Movie_t *pMovie);


SMResult_t MP4ReserveMoovSpace(
	const Movie_t *pMovie,
	uint32_t unLength);


#if !defined(stiLINUX) && !defined(WIN32)
SMResult_t MP4CreateNewMovieFile(
	Movie_t *pMovie,
#ifdef __SMMac__
	short vRefNum,
	long parID,
	Str255 fileName
#endif // __SMMac__
#ifdef WIN32
	const char *pszFileName
#endif // WIN32
	);
#endif //#if !defined(stiLINUX)

#endif // MP4COMMON_H

