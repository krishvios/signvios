///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	DataHandler.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef DATAHANDLER_H
#define DATAHANDLER_H


#include "SMTypes.h"

#include "SMMemory.h"

#if defined (stiLINUX) && defined (CIRCULAR_BUFFERING)
// Forward Declarations
class CMP4File;
#endif // CIRCULAR_BUFFERING

typedef struct DataHandler_t DataHandler_t;

#if defined (stiLINUX) && defined (CIRCULAR_BUFFERING)
DataHandler_t *CreateDataHandlerWithBuffer(
	CMP4File *poMP4File);
#endif // CIRCULAR_BUFFERING

#ifdef stiLINUX
DataHandler_t *CreateDataHandler(
	const char *pszFileName);
#endif // stiLINUX

#ifdef WIN32
DataHandler_t *CreateDataHandler(
	const char *pszFileName);
#endif // WIN32


#ifdef __SMMac__
DataHandler_t *CreateDataHandler(
	short vRefNum,
	long parID,
	Str255 fileName);
#endif // __SMMac


void DestroyDataHandler(
	DataHandler_t *pDataHandler);


SMResult_t OpenDataHandlerForWrite(
	DataHandler_t *pDataHandler);


SMResult_t OpenDataHandlerForRead(
	DataHandler_t *pDataHandler);


SMResult_t CloseDataHandler(
	DataHandler_t *pDataHandler);


SMResult_t DHWrite(
	DataHandler_t *pDataHandler,
	const void *pData,
	uint32_t unSize);


SMResult_t DHWrite2(
	DataHandler_t *pDataHandler,
	uint16_t usData);


SMResult_t DHWrite4(
	DataHandler_t *pDataHandler,
	uint32_t unData);


SMResult_t DHWrite8(
	DataHandler_t *pDataHandler,
	uint64_t un64Data);


SMResult_t DHRead(
	DataHandler_t *pDataHandler,
	uint64_t un64Offset,
	uint32_t unLength,
	void *pBuffer,
	bool bReleaseBuffer = false);


SMResult_t DHGetPosition(
	DataHandler_t *pDataHandler,
	uint64_t *pun64Position);


SMResult_t DHSetPosition(
	DataHandler_t *pDataHandler,
	uint64_t un64Position);


SMResult_t DHGetFileSize(
	DataHandler_t *pDataHandler,
	uint64_t *pun64Size);

#endif // DATAHANDLER_H

