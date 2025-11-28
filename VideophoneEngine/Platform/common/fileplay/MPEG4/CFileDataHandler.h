////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CFileDataHandler
//
//  File Name:	CFileDataHandler.h
//
//  Owner:
//
//	Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CFILEDATAHANDLER_H
#define CFILEDATAHANDLER_H

//
// Includes
//
#ifdef WIN32
#include <windows.h>
#endif // WIN32

#ifdef __SMMac__
#include "Files.h"
#endif // __SMMac__

#ifdef stiLINUX
#include <cstdio>
#endif // stiLINUX

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMMemory.h"
#include "SMCommon.h"
#include "SMTypes.h"

#include "CDataHandler.h"

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

class CFileDataHandler : public CDataHandler
{
public:

	CFileDataHandler () = default;

	CFileDataHandler (const CFileDataHandler &other) = delete;
	CFileDataHandler (CFileDataHandler &&other) = delete;
	CFileDataHandler &operator = (const CFileDataHandler &other) = delete;
	CFileDataHandler &operator = (CFileDataHandler &&other) = delete;


#if defined (stiLINUX) || defined (WIN32)
	CFileDataHandler (
		const char *pszFileName);
#endif

#ifdef __SMMac__
	CFileDataHandler (
		short vRefNum,
		long parID,
		Str255 fileName);
#endif // __SMMac

	~CFileDataHandler() override;

	SMResult_t OpenDataHandlerForWrite () override;

	SMResult_t OpenDataHandlerForRead () override;

	SMResult_t CloseDataHandler() override;

	SMResult_t DHWrite(
		const void *pData,
		uint32_t unSize) override;

	SMResult_t DHWrite2(
		uint16_t usData) override;

	SMResult_t DHWrite4(
		uint32_t unData) override;

	SMResult_t DHWrite8(
		uint64_t un64Data) override;

	SMResult_t DHRead(
		uint64_t un64Offset,
		uint32_t unLength,
		void *pBuffer,
		bool bReleaseBuffer = false) override;

	SMResult_t DHGetPosition(
		uint64_t *pun64Position) const override;

	SMResult_t DHSetPosition(
		uint64_t un64Position) override;

	SMResult_t DHGetFileSize(
		uint64_t *pun64Size) const override;

private:

	unsigned int m_unOpenCount{0};
	
#ifdef WIN32
	char* m_pszFileName;
	HANDLE m_hFile;
#endif

#if defined(stiLINUX)
	char *m_pszFileName{nullptr};
	FILE* m_hFile{nullptr};
#endif

#ifdef __SMMac__
	short m_vRefNum;
	long m_parID;
	Str255 m_fileName;
	short m_fileRefNum;
#endif
};

#endif // CFILEDATAHANDLER_H
