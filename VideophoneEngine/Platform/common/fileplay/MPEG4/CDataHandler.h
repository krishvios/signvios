////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CDataHandler
//
//  File Name:	CDataHandler.h
//
//  Owner:
//
//	Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CDATAHANDLER_H
#define CDATAHANDLER_H

//
// Includes
//
#include <cstdio>
#include "SMTypes.h"
#include "SMMemory.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//


//
// Globals
//

class CDataHandler
{
public:

	CDataHandler () = default;

	CDataHandler (const CDataHandler &other) = delete;
	CDataHandler (CDataHandler &&other) = delete;
	CDataHandler &operator= (const CDataHandler &other) = delete;
	CDataHandler &operator= (CDataHandler &&other) = delete;

	virtual ~CDataHandler () = default;

	virtual SMResult_t OpenDataHandlerForWrite () = 0;

	virtual SMResult_t OpenDataHandlerForRead () = 0;

	virtual SMResult_t CloseDataHandler() = 0;

	virtual SMResult_t DHAdjustOffsets(
		uint32_t un32AdjustOffset) = 0;

	virtual SMResult_t DHCreatePrefixBuffer(
		uint32_t un32EndingOffset,
		uint32_t un32PrefixBufSize) = 0;

	virtual SMResult_t DHWrite(
		const void *pData,
		uint32_t unSize) = 0;

	virtual SMResult_t DHWrite2(
		uint16_t usData) = 0;

	virtual SMResult_t DHWrite4(
		uint32_t unData) = 0;

	virtual SMResult_t DHWrite8(
		uint64_t un64Data) = 0;

	virtual SMResult_t DHRead(
		uint64_t un64Offset,
		uint32_t unLength,
		void *pBuffer,
		bool bReleaseBuffer = false) = 0;

	virtual SMResult_t DHGetPosition(
		uint64_t *pun64Position) const = 0;

	virtual SMResult_t DHSetPosition(
		uint64_t un64Position) = 0;

	virtual SMResult_t DHGetFileSize(
		uint64_t *pun64Size) const = 0;

};

#endif // CDATAHANDLER_H
