///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	DataHandler.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
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

#include "DataHandler.h"

#if defined (stiLINUX) && defined (CIRCULAR_BUFFERING)
#include "CMP4File.h"
#endif //CIRCULAR_BUFFERING

typedef struct DataHandler_t
{
#ifdef __SMMac__
	short vRefNum;
	long parID;
	Str255 fileName;
	short fileRefNum;
#endif // __SMMac__

#ifdef WIN32
	char *m_pszFileName;
	HANDLE hFile;
#endif // WIN32

#ifdef stiLINUX

#ifdef CIRCULAR_BUFFERING
	CMP4File *poFile;
#endif // CIRCULAR_BUFFERING

	char *m_pszFileName;
	FILE* hFile;
#endif // stiLINUX
	unsigned int m_unOpenCount;

} DataHandler_t;


#if defined (stiLINUX) && defined (CIRCULAR_BUFFERING)
///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
DataHandler_t *CreateDataHandlerWithBuffer(
	CMP4File *poMP4File)
{
	DataHandler_t *pDataHandler;

	pDataHandler = (DataHandler_t *)SMAllocPtrClear(sizeof(DataHandler_t));

	pDataHandler->poFile = poMP4File;

	pDataHandler->m_unOpenCount = 0;

	return pDataHandler;
}
#endif // CIRCULAR_BUFFERING

///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
DataHandler_t *CreateDataHandler(
#ifdef __SMMac__
	short vRefNum,
	long parID,
	Str255 fileName)
#endif // __SMMac__
#ifdef WIN32
	const char *pszFileName)
#endif // WIN32
#ifdef stiLINUX
	const char *pszFileName)
#endif // stiLINUX
{
	DataHandler_t *pDataHandler;

	pDataHandler = (DataHandler_t *)SMAllocPtrClear(sizeof(DataHandler_t));

#ifdef __SMMac__
	pDataHandler->vRefNum = vRefNum;
	pDataHandler->parID = parID;
	memcpy(pDataHandler->fileName, fileName, 256);
	pDataHandler->fileRefNum = 0;
#endif // __SMMac__

#ifdef WIN32
	pDataHandler->m_pszFileName = (char *)SMAllocPtr(strlen(pszFileName) + 1);
	memcpy(pDataHandler->m_pszFileName, pszFileName, strlen(pszFileName) + 1);

	pDataHandler->hFile = NULL;
#endif // WIN32

#ifdef stiLINUX
	pDataHandler->m_pszFileName = (char *)SMAllocPtr(strlen(pszFileName) + 1);
	memcpy(pDataHandler->m_pszFileName, pszFileName, strlen(pszFileName) + 1);

	pDataHandler->hFile = NULL;
#endif // stiLINUX

	pDataHandler->m_unOpenCount = 0;

	return pDataHandler;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
void DestroyDataHandler(
	DataHandler_t *pDataHandler)
{
	if (pDataHandler)
	{
		if (pDataHandler->m_unOpenCount > 0)
		{
			pDataHandler->m_unOpenCount = 1;

			CloseDataHandler(pDataHandler);
		}
#ifdef WIN32
		if (pDataHandler->m_pszFileName)
		{
			SMFreePtr(pDataHandler->m_pszFileName);

			pDataHandler->m_pszFileName = NULL;
		}
#endif // WIN32

#ifdef stiLINUX
		if (pDataHandler->m_pszFileName)
		{
			SMFreePtr(pDataHandler->m_pszFileName);
		
			pDataHandler->m_pszFileName = NULL;
		}
#endif // stiLINUX

		SMFreePtr(pDataHandler);
	}
}


///////////////////////////////////////////////////////////////////////////////
//; OpenDataHandlerForWrite
//
// Abstract:
//
// Returns:
//
SMResult_t OpenDataHandlerForWrite(
	DataHandler_t *pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum == 0)
	{
		FSSpec fsspec;

		err = FSMakeFSSpec(pDataHandler->vRefNum, pDataHandler->parID, pDataHandler->fileName,
						   &fsspec);

		if (err == noErr)
		{
			//
			// The file must exist if we got here so delete it.
			//
			err = FSpDelete(&fsspec);

			ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
		}
		else
		{
			ASSERTRT(err == fnfErr, SMRES_ALLOC_FAILED);
		}

		err = FSpCreate(&fsspec, 'TVOD', 'mpg4', -1);
		
		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);

		err = HOpen(pDataHandler->vRefNum, pDataHandler->parID, pDataHandler->fileName, fsRdWrPerm,
					&pDataHandler->fileRefNum);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);

		ASSERTRT(pDataHandler->fileRefNum != 0, SMRES_ALLOC_FAILED);
	}
#endif // _SMMac__

#ifdef WIN32
	ASSERTRT(pDataHandler->m_pszFileName[0] != '\0', SMRES_INVALID_FILENAME);

	if (pDataHandler->hFile == NULL)
	{
		pDataHandler->hFile = CreateFile(pDataHandler->m_pszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
										 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
										 NULL);

		if (pDataHandler->hFile == INVALID_HANDLE_VALUE)
		{
			pDataHandler->hFile = NULL;

			ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
		}
	}
#endif // WIN32

#ifdef stiLINUX
	ASSERTRT(pDataHandler->m_pszFileName[0] != '\0', SMRES_INVALID_FILENAME);

	if (pDataHandler->hFile == NULL)
	{
		pDataHandler->hFile = fopen (pDataHandler->m_pszFileName, "w+");
		
		if (NULL == pDataHandler->hFile)
		{
			ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
		}
		
		
	}
#endif // stiLINUX

	pDataHandler->m_unOpenCount++;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; OpenDataHandlerForRead
//
// Abstract:
//
// Returns:
//
SMResult_t OpenDataHandlerForRead(
	DataHandler_t *pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum == 0)
	{
		FSSpec fsspec;

		err = FSMakeFSSpec(pDataHandler->vRefNum, pDataHandler->parID, pDataHandler->fileName,
						   &fsspec);

		ASSERTRT(err == noErr, SMRES_FILE_OPEN_FAILED);
		
		err = HOpen(pDataHandler->vRefNum, pDataHandler->parID, pDataHandler->fileName, fsRdWrShPerm,
					&pDataHandler->fileRefNum);

		ASSERTRT(err == noErr, SMRES_FILE_OPEN_FAILED);

		ASSERTRT(pDataHandler->fileRefNum != 0, SMRES_FILE_OPEN_FAILED);
	}
#endif // _SMMac__

#ifdef WIN32
	ASSERTRT(pDataHandler->m_pszFileName[0] != '\0', SMRES_INVALID_FILENAME);

	if (pDataHandler->hFile == NULL)
	{
		pDataHandler->hFile = CreateFile(pDataHandler->m_pszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
										 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
										 NULL);

		if (pDataHandler->hFile == INVALID_HANDLE_VALUE)
		{
			pDataHandler->hFile = CreateFile(pDataHandler->m_pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
											 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
											 NULL);

			if (pDataHandler->hFile == INVALID_HANDLE_VALUE)
			{
				pDataHandler->hFile = NULL;

				ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
			}
		}
	}
#endif // WIN32

#ifdef stiLINUX
	ASSERTRT(pDataHandler->m_pszFileName[0] != '\0', SMRES_INVALID_FILENAME);

	if (pDataHandler->hFile == NULL)
	{
		pDataHandler->hFile = fopen (pDataHandler->m_pszFileName, "r+");

		if (pDataHandler->hFile == NULL)
		{
			pDataHandler->hFile = fopen (pDataHandler->m_pszFileName, "r");

			if (pDataHandler->hFile == NULL)
			{
				ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
			}
		}
		
	}
#endif // stiLINUX

	pDataHandler->m_unOpenCount++;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CloseDataHandler
//
// Abstract:
//
// Returns:
//
SMResult_t CloseDataHandler(
	DataHandler_t *pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

	if (pDataHandler->m_unOpenCount > 0)
	{
		pDataHandler->m_unOpenCount--;

#ifdef __SMMac__
		if (pDataHandler->m_unOpenCount == 0
	 	&& pDataHandler->fileRefNum != 0)
		{
			FSClose(pDataHandler->fileRefNum);

			pDataHandler->fileRefNum = 0;
		}
#endif // __SMMac__

#ifdef WIN32
		if (pDataHandler->m_unOpenCount == 0
		 && pDataHandler->hFile)
		{
			CloseHandle(pDataHandler->hFile);

			pDataHandler->hFile = NULL;
		}
#endif // WIN32

#ifdef stiLINUX

#ifdef CIRCULAR_BUFFERING
		if (pDataHandler->m_unOpenCount == 0
			&& pDataHandler->poFile)
		{
			pDataHandler->poFile = NULL;
		}

#else
		if (pDataHandler->m_unOpenCount == 0
		 && pDataHandler->hFile)
		{
			fclose (pDataHandler->hFile);

			pDataHandler->hFile = NULL;	
		}
#endif // CIRCULAR_BUFFERING
#endif // stiLINUX


	}
		
smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; DHWrite
//
// Abstract:
//
// Returns:
//
SMResult_t DHWrite(
	DataHandler_t *pDataHandler,
	void *pData,
	uint32_t unSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef stiLINUX
	uint32_t unBytesWritten;
#endif
	
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32

#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		err = FSWrite(pDataHandler->fileRefNum, (long *)&unSize, pData);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif //__SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		bResult = WriteFile(pDataHandler->hFile, pData, unSize, (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unSize == unBytesWritten, SMRES_WRITE_ERROR);
	}
#endif // WIN32

#ifdef stiLINUX
	if (pDataHandler->hFile)
	{
		unBytesWritten = fwrite (pData, 1, unSize, pDataHandler->hFile);

		ASSERTRT(unSize == unBytesWritten, SMRES_WRITE_ERROR);
	}
#endif // stiLINUX

smbail:

	return SMResult;
} // DHWrite


///////////////////////////////////////////////////////////////////////////////
//; DHRead
//
// Abstract:
//
// Returns:
//
SMResult_t DHRead(
	DataHandler_t *pDataHandler,
	uint64_t un64Offset,
	uint32_t unLength,
	void *pBuffer,
	bool bReleaseBuffer)  // Default is false
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unNumRead;
	
#ifdef stiLINUX
	int32_t nResult;
#endif	

#ifdef WIN32
	uint32_t unHighOffset;
	DWORD dwResult;
	BOOL bResult;
#endif // WIN32

#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	ASSERTRT(unLength < 0x800000, SMRES_READ_ERROR);
	
	SMResult = DHSetPosition(pDataHandler, un64Offset);
	
	TESTRESULT();
	
	err = FSRead(pDataHandler->fileRefNum, (long *)&unLength, pBuffer);
	
	ASSERTRT(err == noErr, SMRES_READ_ERROR);
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		unHighOffset = (uint32_t)(un64Offset >> 32);

		dwResult = SetFilePointer(pDataHandler->hFile, (LONG)un64Offset, (LONG *)&unHighOffset, FILE_BEGIN);

		ASSERTRT(dwResult != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);

		bResult = ReadFile(pDataHandler->hFile, pBuffer, unLength, (LPDWORD)&unNumRead, NULL);

		ASSERTRT(bResult == TRUE, SMRES_READ_ERROR);
	}
#endif // WIN32

#ifdef stiLINUX
#ifdef CIRCULAR_BUFFERING
	if (pDataHandler->poFile)
	{
		uint8_t *pReadBuffer;
		CMP4File::EstiBufferResult eResult = (pDataHandler->poFile)->GetBuffer(un64Offset, unLength, &pReadBuffer); 
		
		ASSERTRT(CMP4File::estiBUFFER_OK == eResult, SMRES_READ_ERROR);

		memcpy (pBuffer, pReadBuffer, unLength);

		if (bReleaseBuffer)
		{
			(pDataHandler->poFile)->ReleaseBuffer (un64Offset, unLength);
		}
	}
#else
	if (pDataHandler->hFile)
	{
	
		nResult = fseek (pDataHandler->hFile, un64Offset, SEEK_SET);
	
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
	
		unNumRead = fread (pBuffer, 1, unLength, pDataHandler->hFile); 
		
		ASSERTRT(unNumRead == unLength, SMRES_READ_ERROR);
	}
#endif // CIRCULAR_BUFFERING
#endif // stiLINUX



smbail:

	return SMResult;
} // DHRead


///////////////////////////////////////////////////////////////////////////////
//; DHWrite2
//
// Abstract:
//
// Returns:
//
SMResult_t DHWrite2(
	DataHandler_t *pDataHandler,
	uint16_t usData)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint16_t usTemp;
#ifdef stiLINUX
	uint32_t unBytesWritten;
#endif
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		usTemp = usData;

		long lSize = sizeof(usTemp);

		err = FSWrite(pDataHandler->fileRefNum, &lSize, &usTemp);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		usTemp = BYTESWAP(usData);

		bResult = WriteFile(pDataHandler->hFile, &usTemp, sizeof(usTemp), (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unBytesWritten == sizeof(usTemp), SMRES_WRITE_ERROR);
	}
#endif // WIN32

#ifdef stiLINUX
	if (pDataHandler->hFile)
	{
		//usTemp = BYTESWAP(usData);
		usTemp = usData;

		unBytesWritten = fwrite (&usTemp, 1, sizeof (usTemp), pDataHandler->hFile);
		
		ASSERTRT(unBytesWritten == sizeof(usTemp), SMRES_WRITE_ERROR);
	}
#endif // stiLINUX

smbail:

	return SMResult;
}



///////////////////////////////////////////////////////////////////////////////
//; DHWrite4
//
// Abstract:
//
// Returns:
//
SMResult_t DHWrite4(
	DataHandler_t *pDataHandler,
	uint32_t unData)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unTemp;
#ifdef stiLINUX
	uint32_t unBytesWritten;
#endif
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		unTemp = unData;

		long lSize = sizeof(unTemp);

		err = FSWrite(pDataHandler->fileRefNum, &lSize, &unTemp);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		unTemp = DWORDBYTESWAP(unData);

		bResult = WriteFile(pDataHandler->hFile, &unTemp, sizeof(unTemp), (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unBytesWritten == sizeof(unTemp), SMRES_WRITE_ERROR);
	}
#endif // WIN32

#ifdef stiLINUX
	if (pDataHandler->hFile)
	{
		//unTemp = DWORDBYTESWAP(unData);
	
		unTemp = unData;

		unBytesWritten = fwrite (&unTemp, 1, sizeof (unTemp), pDataHandler->hFile);
		
		ASSERTRT(unBytesWritten == sizeof(unTemp), SMRES_WRITE_ERROR);
		
	}
#endif // stiLINUX

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; DHWrite8
//
// Abstract:
//
// Returns:
//
SMResult_t DHWrite8(
	DataHandler_t *pDataHandler,
	uint64_t un64Data)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64Temp;
#ifdef stiLINUX
	uint32_t unBytesWritten;
#endif
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		un64Temp = un64Data;

		long lSize = sizeof(un64Temp);

		err = FSWrite(pDataHandler->fileRefNum, &lSize, &un64Temp);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		un64Temp = QWORDBYTESWAP(un64Data);

		bResult = WriteFile(pDataHandler->hFile, &un64Temp, sizeof(un64Temp), (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unBytesWritten == sizeof(un64Temp), SMRES_WRITE_ERROR);
	}
#endif // WIN32

#ifdef stiLINUX
	if (pDataHandler->hFile)
	{
		//un64Temp = QWORDBYTESWAP(un64Data);
		un64Temp = un64Data;

		unBytesWritten = fwrite (&un64Temp, 1, sizeof (un64Temp), pDataHandler->hFile);
		
		ASSERTRT(unBytesWritten == sizeof(un64Temp), SMRES_WRITE_ERROR);
	
	}
#endif // stiLINUX

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; DHGetPosition
//
// Abstract:
//
// Returns:
//
SMResult_t DHGetPosition(
	DataHandler_t *pDataHandler,
	uint64_t *pun64Position)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef stiLINUX
	uint32_t unPosition;
#endif
#ifdef WIN32
	uint32_t unLowPosition;
	uint32_t unHighPosition;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
	uint32_t unPosition;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		err = GetFPos(pDataHandler->fileRefNum, (long *)&unPosition);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
		
		*pun64Position = unPosition;
	}
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		unHighPosition = 0;

		unLowPosition = SetFilePointer(pDataHandler->hFile, 0, (LONG *)&unHighPosition, FILE_CURRENT);

		ASSERTRT(unLowPosition != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);

		*pun64Position = ((uint64_t)unHighPosition << 32) | unLowPosition;
	}
#endif // WIN32

#ifdef stiLINUX
	if (pDataHandler->hFile)
	{
		unPosition = ftell (pDataHandler->hFile);
		
		ASSERTRT(unPosition != 0xFFFFFFFF, SMRES_READ_ERROR);
		
		*pun64Position = unPosition;
		
	}
#endif // stiLINUX


smbail:

	return SMResult;
} // DHGetPosition


///////////////////////////////////////////////////////////////////////////////
//; DHSetPosition
//
// Abstract:
//
// Returns:
//
SMResult_t DHSetPosition(
	DataHandler_t *pDataHandler,
	uint64_t un64Position)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef stiLINUX
	int32_t nResult;
#endif
#ifdef WIN32
	uint32_t unHighPosition;
	DWORD dwResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		err = SetFPos(pDataHandler->fileRefNum, fsFromStart, (long)un64Position);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		unHighPosition = (uint32_t)(un64Position >> 32);

		dwResult = SetFilePointer(pDataHandler->hFile, (LONG)un64Position, (LONG *)&unHighPosition, FILE_BEGIN);

		ASSERTRT(dwResult != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);
	}
#endif // WIN32

#ifdef stiLINUX
	if (pDataHandler->hFile)
	{
		nResult = fseek (pDataHandler->hFile, un64Position, SEEK_SET);
		
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
	}
#endif // stiLINUX

smbail:

	return SMResult;
} // DHSetPosition


///////////////////////////////////////////////////////////////////////////////
//; DHGetFileSize
//
// Abstract:
//
// Returns:
//
SMResult_t DHGetFileSize(
	DataHandler_t *pDataHandler,
	uint64_t *pun64Size)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef stiLINUX
	int32_t nResult;
	uint32_t unSize;
	uint32_t unOrigPos;
#endif
#ifdef WIN32
	uint32_t unLowSize;
	uint32_t unHighSize;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
	uint32_t unSize;
	uint32_t unOrigPos;
#endif // __SMMac__

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

#ifdef __SMMac__
	if (pDataHandler->fileRefNum != 0)
	{
		err = GetFPos(pDataHandler->fileRefNum, (long *)&unOrigPos);
		
		ASSERTRT(err == noErr, -1);
		
		err = SetFPos(pDataHandler->fileRefNum, fsFromLEOF, 0);

		ASSERTRT(err == noErr, -1);
				
		err = GetFPos(pDataHandler->fileRefNum, (long *)&unSize);
		
		ASSERTRT(err == noErr, -1);
		
		err = SetFPos(pDataHandler->fileRefNum, fsFromStart, unOrigPos);
		
		ASSERTRT(err == noErr, -1);
		
		*pun64Size = unSize;
	}
#endif // __SMMac__

#ifdef WIN32
	if (pDataHandler->hFile)
	{
		unLowSize = GetFileSize(pDataHandler->hFile, (LPDWORD)&unHighSize);

		ASSERTRT(unLowSize != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);

		*pun64Size = ((uint64_t)unHighSize << 32) | unLowSize;
	}
#endif // WIN32

#ifdef stiLINUX
#ifdef CIRCULAR_BUFFERING

	if (pDataHandler->poFile)
	{
		*pun64Size = (pDataHandler->poFile)->GetFileSize ();
	}
#else
	if (pDataHandler->hFile)
	{
		unOrigPos = ftell (pDataHandler->hFile);
		 
		ASSERTRT(0xFFFFFFFF != unOrigPos, SMRES_READ_ERROR);
		
		nResult = fseek (pDataHandler->hFile, 0, SEEK_END);
		
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
				
		unSize = ftell (pDataHandler->hFile);
		
		ASSERTRT(0xFFFFFFFF != unSize, SMRES_READ_ERROR);
		
		nResult = fseek (pDataHandler->hFile, unOrigPos, SEEK_SET);
		
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
		
		*pun64Size = unSize;
	
	}
#endif // CIRCULAR_BUFFERING
#endif // stiLINUX

smbail:

	return SMResult;
} // DHGetFileSize

