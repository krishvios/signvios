////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CFileDataHandler
//
//  File Name:	CFileDataHandler.cpp
//
//  Owner:
//
//	Abstract: 
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiTrace.h"
#include "CFileDataHandler.h"

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

//
// Locals
//


CFileDataHandler::CFileDataHandler (
#ifdef __SMMac__
	short vRefNum,
	long parID,
	Str255 fileName)
#endif // __SMMac__
	const char *pszFileName)
{
	stiLOG_ENTRY_NAME (CFileDataHandler::CFileDataHandler);

	m_unOpenCount = 0;
		
#ifdef __SMMac__
	m_vRefNum = vRefNum;
	m_parID = parID;
	memcpy(m_fileName, fileName, 256);
	m_fileRefNum = 0;
#endif // __SMMac__

	m_pszFileName = (char *)SMAllocPtr(strlen(pszFileName) + 1);
	if (m_pszFileName)
	{
		memcpy(m_pszFileName, pszFileName, strlen(pszFileName) + 1);
	}

	m_hFile = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::~CFileDataHandler
//
//  Description: Class destructor.
//
//  Abstract:
//
//  Returns: None.
//
CFileDataHandler::~CFileDataHandler ()
{
	if (m_unOpenCount > 0)
	{
		m_unOpenCount = 1;
		
		CloseDataHandler();
	}
	
	if (m_pszFileName)
	{
		SMFreePtr(m_pszFileName);
		
		m_pszFileName = nullptr;
	}
}


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::OpenDataHandlerForWrite
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::OpenDataHandlerForWrite()
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName == 0)
	{
		FSSpec fsspec;

		err = FSMakeFSSpec(m_vRefNum, m_parID, m_fileName, &fsspec);

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

		err = HOpen(m_vRefNum, m_parID, m_fileName, fsRdWrPerm,
					&m_fileName);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);

		ASSERTRT(m_fileName != 0, SMRES_ALLOC_FAILED);
	}
#endif // _SMMac__

	ASSERTRT(m_pszFileName[0] != '\0', SMRES_INVALID_FILENAME);
	
	if (m_hFile == nullptr)
	{
#ifdef WIN32
		m_hFile = CreateFile(m_pszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
										 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
										 NULL);

		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			m_hFile = NULL;
		}
#elif defined(stiLINUX)
		m_hFile = fopen (m_pszFileName, "w+");
#endif // #if defined(stiLINUX)
		
		if (nullptr == m_hFile)
		{
			ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
		}
	}

	m_unOpenCount++;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler:::OpenDataHandlerForRead
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::OpenDataHandlerForRead()
{
	SMResult_t SMResult = SMRES_SUCCESS;
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName == 0)
	{
		FSSpec fsspec;

		err = FSMakeFSSpec(m_vRefNum, m_parID, m_fileName,
						   &fsspec);

		ASSERTRT(err == noErr, SMRES_FILE_OPEN_FAILED);
		
		err = HOpen(m_vRefNum, m_parID, m_fileName, fsRdWrShPerm,
					&m_fileName);

		ASSERTRT(err == noErr, SMRES_FILE_OPEN_FAILED);

		ASSERTRT(m_fileName != 0, SMRES_FILE_OPEN_FAILED);
	}
#endif // _SMMac__

	ASSERTRT(m_pszFileName[0] != '\0', SMRES_INVALID_FILENAME);
	
	if (m_hFile == nullptr)
	{
#ifdef WIN32
		m_hFile = CreateFile(m_pszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
										 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
										 NULL);

		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			m_hFile = CreateFile(m_pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
											 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
											 NULL);

			if (m_hFile == INVALID_HANDLE_VALUE)
			{
				m_hFile = NULL;

				ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
			}
		}
#elif defined(stiLINUX)
		m_hFile = fopen (m_pszFileName, "r+");

		if (m_hFile == nullptr)
		{
			m_hFile = fopen (m_pszFileName, "r");

			if (m_hFile == nullptr)
			{
				ASSERTRT(eFalse, SMRES_FILE_OPEN_FAILED);
			}
		}
#endif // #if defined(stiLINUX)
	}

	m_unOpenCount++;

smbail:

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::CloseDataHandler
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::CloseDataHandler()
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (m_unOpenCount > 0)
	{
		m_unOpenCount--;

#ifdef __SMMac__
		if (m_unOpenCount == 0
	 	&& m_fileName != 0)
		{
			FSClose(m_fileName);

			m_fileName = 0;
		}
#endif // __SMMac__

#ifdef WIN32
		if (m_unOpenCount == 0 && m_hFile)
		{
			CloseHandle(m_hFile);

			m_hFile = NULL;
		}
#endif // WIN32

#if defined(stiLINUX)
		if (m_unOpenCount == 0 && m_hFile)
		{
			fclose (m_hFile);

			m_hFile = nullptr;	
		}
#endif // #if defined(stiLINUX)

	}

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHWrite
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHWrite(
	const void *pData,
	uint32_t unSize) 
{
	SMResult_t SMResult = SMRES_SUCCESS;
#if defined(stiLINUX)
	uint32_t unBytesWritten;
#endif
	
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32

#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		err = FSWrite(m_fileName, (long *)&unSize, pData);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif //__SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		bResult = WriteFile(m_hFile, pData, unSize, (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unSize == unBytesWritten, SMRES_WRITE_ERROR);
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		unBytesWritten = fwrite (pData, 1, unSize, m_hFile);

		ASSERTRT(unSize == unBytesWritten, SMRES_WRITE_ERROR);
	}
#endif // #if defined(stiLINUX)

smbail:

	return SMResult;
} // CFileDataHandler::DHWrite


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHRead
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHRead(
	uint64_t un64Offset,
	uint32_t unLength,
	void *pBuffer,
	bool bReleaseBuffer)  // Default is false
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unNumRead;
	
#if defined(stiLINUX)
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

#ifdef __SMMac__
	ASSERTRT(unLength < 0x800000, SMRES_READ_ERROR);
	
	SMResult = DHSetPosition(un64Offset);
	
	TESTRESULT();
	
	err = FSRead(m_fileName, (long *)&unLength, pBuffer);
	
	ASSERTRT(err == noErr, SMRES_READ_ERROR);
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		unHighOffset = (uint32_t)(un64Offset >> 32);

		dwResult = SetFilePointer(m_hFile, (LONG)un64Offset, (LONG *)&unHighOffset, FILE_BEGIN);

		ASSERTRT(dwResult != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);

		bResult = ReadFile(m_hFile, pBuffer, unLength, (LPDWORD)&unNumRead, NULL);

		ASSERTRT(bResult == TRUE, SMRES_READ_ERROR);
	}
#endif // WIN32

#if defined(stiLINUX)

	if (m_hFile)
	{
	
		nResult = fseek (m_hFile, un64Offset, SEEK_SET);
	
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
	
		unNumRead = fread (pBuffer, 1, unLength, m_hFile); 
		
		ASSERTRT(unNumRead == unLength, SMRES_READ_ERROR);
	}

#endif // #if defined(stiLINUX)



smbail:

	return SMResult;
} // CFileDataHandler::DHRead


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHWrite2
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHWrite2(
	uint16_t usData)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint16_t usTemp;
#if defined(stiLINUX)
	uint32_t unBytesWritten;
#endif
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		usTemp = usData;

		long lSize = sizeof(usTemp);

		err = FSWrite(m_fileName, &lSize, &usTemp);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		usTemp = BYTESWAP(usData);

		bResult = WriteFile(m_hFile, &usTemp, sizeof(usTemp), (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unBytesWritten == sizeof(usTemp), SMRES_WRITE_ERROR);
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		//usTemp = BYTESWAP(usData);
		usTemp = usData;

		unBytesWritten = fwrite (&usTemp, 1, sizeof (usTemp), m_hFile);
		
		ASSERTRT(unBytesWritten == sizeof(usTemp), SMRES_WRITE_ERROR);
	}
#endif // #if defined(stiLINUX)

smbail:

	return SMResult;
}



///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHWrite4
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHWrite4(
	uint32_t unData)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unTemp;
#if defined(stiLINUX)
	uint32_t unBytesWritten;
#endif
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		unTemp = unData;

		long lSize = sizeof(unTemp);

		err = FSWrite(m_fileName, &lSize, &unTemp);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		unTemp = DWORDBYTESWAP(unData);

		bResult = WriteFile(m_hFile, &unTemp, sizeof(unTemp), (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unBytesWritten == sizeof(unTemp), SMRES_WRITE_ERROR);
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		//unTemp = DWORDBYTESWAP(unData);
	
		unTemp = unData;

		unBytesWritten = fwrite (&unTemp, 1, sizeof (unTemp), m_hFile);
		
		ASSERTRT(unBytesWritten == sizeof(unTemp), SMRES_WRITE_ERROR);
		
	}
#endif // #if defined(stiLINUX)

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHWrite8
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHWrite8(
	uint64_t un64Data)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64Temp;
#if defined(stiLINUX)
	uint32_t unBytesWritten;
#endif
#ifdef WIN32
	uint32_t unBytesWritten;
	BOOL bResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		un64Temp = un64Data;

		long lSize = sizeof(un64Temp);

		err = FSWrite(m_fileName, &lSize, &un64Temp);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		un64Temp = QWORDBYTESWAP(un64Data);

		bResult = WriteFile(m_hFile, &un64Temp, sizeof(un64Temp), (LPDWORD)&unBytesWritten, NULL);

		ASSERTRT(bResult == TRUE, SMRES_WRITE_ERROR);
		ASSERTRT(unBytesWritten == sizeof(un64Temp), SMRES_WRITE_ERROR);
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		//un64Temp = QWORDBYTESWAP(un64Data);
		un64Temp = un64Data;

		unBytesWritten = fwrite (&un64Temp, 1, sizeof (un64Temp), m_hFile);
		
		ASSERTRT(unBytesWritten == sizeof(un64Temp), SMRES_WRITE_ERROR);
	
	}
#endif // #if defined(stiLINUX)

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHGetPosition
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHGetPosition(
	uint64_t *pun64Position) const
{
	SMResult_t SMResult = SMRES_SUCCESS;
#if defined(stiLINUX)
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

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		err = GetFPos(m_fileName, (long *)&unPosition);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
		
		*pun64Position = unPosition;
	}
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		unHighPosition = 0;

		unLowPosition = SetFilePointer(m_hFile, 0, (LONG *)&unHighPosition, FILE_CURRENT);

		ASSERTRT(unLowPosition != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);

		*pun64Position = ((uint64_t)unHighPosition << 32) | unLowPosition;
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		unPosition = ftell (m_hFile);
		
		ASSERTRT(unPosition != 0xFFFFFFFF, SMRES_READ_ERROR);
		
		*pun64Position = unPosition;
		
	}
#endif // #if defined(stiLINUX)


smbail:

	return SMResult;
} // CFileDataHandler::DHGetPosition


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHSetPosition
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHSetPosition(
	uint64_t un64Position)
{
	SMResult_t SMResult = SMRES_SUCCESS;
#if defined(stiLINUX)
	int32_t nResult;
#endif
#ifdef WIN32
	uint32_t unHighPosition;
	DWORD dwResult;
#endif // WIN32
#ifdef __SMMac__
	OSErr err;
#endif // __SMMac__

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		err = SetFPos(m_fileName, fsFromStart, (long)un64Position);

		ASSERTRT(err == noErr, SMRES_ALLOC_FAILED);
	}
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		unHighPosition = (uint32_t)(un64Position >> 32);

		dwResult = SetFilePointer(m_hFile, (LONG)un64Position, (LONG *)&unHighPosition, FILE_BEGIN);

		ASSERTRT(dwResult != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		nResult = fseek (m_hFile, un64Position, SEEK_SET);
		
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
	}
#endif // #if defined(stiLINUX)

smbail:

	return SMResult;
} // CFileDataHandler::DHSetPosition


///////////////////////////////////////////////////////////////////////////////
//; CFileDataHandler::DHGetFileSize
//
// Abstract:
//
// Returns:
//
SMResult_t CFileDataHandler::DHGetFileSize(
	uint64_t *pun64Size) const
{
	SMResult_t SMResult = SMRES_SUCCESS;
#if defined(stiLINUX)
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

#ifdef __SMMac__
	if (m_fileName != 0)
	{
		err = GetFPos(m_fileName, (long *)&unOrigPos);
		
		ASSERTRT(err == noErr, -1);
		
		err = SetFPos(m_fileName, fsFromLEOF, 0);

		ASSERTRT(err == noErr, -1);
				
		err = GetFPos(m_fileName, (long *)&unSize);
		
		ASSERTRT(err == noErr, -1);
		
		err = SetFPos(m_fileName, fsFromStart, unOrigPos);
		
		ASSERTRT(err == noErr, -1);
		
		*pun64Size = unSize;
	}
#endif // __SMMac__

#ifdef WIN32
	if (m_hFile)
	{
		unLowSize = GetFileSize(m_hFile, (LPDWORD)&unHighSize);

		ASSERTRT(unLowSize != 0xFFFFFFFF || GetLastError() == NO_ERROR, SMRES_READ_ERROR);

		*pun64Size = ((uint64_t)unHighSize << 32) | unLowSize;
	}
#endif // WIN32

#if defined(stiLINUX)
	if (m_hFile)
	{
		unOrigPos = ftell (m_hFile);
		 
		ASSERTRT(0xFFFFFFFF != unOrigPos, SMRES_READ_ERROR);
		
		nResult = fseek (m_hFile, 0, SEEK_END);
		
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
				
		unSize = ftell (m_hFile);
		
		ASSERTRT(0xFFFFFFFF != unSize, SMRES_READ_ERROR);
		
		nResult = fseek (m_hFile, unOrigPos, SEEK_SET);
		
		ASSERTRT(0 == nResult, SMRES_READ_ERROR);
		
		*pun64Size = unSize;
	
	}
#endif // #if defined(stiLINUX)

smbail:

	return SMResult;
} // CFileDataHandler::DHGetFileSize



