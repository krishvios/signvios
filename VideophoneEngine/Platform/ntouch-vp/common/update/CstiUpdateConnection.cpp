// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#include "stiTools.h"
#include "stiHTTPServiceTools.h"   
#include "CstiHostNameResolver.h"
#include "CstiUpdateConnection.h"
#include "stiSSLTools.h"


#define PROT_HTTPS_STR 		"https"
#define PROT_HTTP_STR 		"http"


#define ASSERTRT(t, r)	if (!(t)) { nResult = r; goto bail; }
#define PRINT_REPLIES		1
#define SOCKET_ERROR	   -1
#define GENERAL_ERROR	   -1
#define INVALID_PROTOCOL   -1
#define MEM_ALLOC_ERROR    -2

#define MAX_BUFFER_LENGTH		1024
#define MAX_REPLY_CODE_LENGTH	3
#define OUT_BUFFER_SIZE 		512
#define ERR_NODATA				1
#define TIME_OUT_SECONDS		120


//
// Globals
//


CstiUpdateConnection::~CstiUpdateConnection ()
{
	if (m_pszTmpBuffer)
	{
		delete [] m_pszTmpBuffer;
		m_pszTmpBuffer = nullptr;
	}
}


stiHResult CstiUpdateConnection::HTTPOpen (
	const std::string &url,
	int nStartByte)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!m_pHTTPService)
	{
		m_pHTTPService = new CstiHTTPService;

		std::vector<std::string> serverIPList;
		unsigned short serverPort;

		m_pHTTPService->URLParse(url.c_str (), nullptr, &serverIPList, &serverPort, &m_File);

		if (m_File[0] != '/')
		{
			m_File.insert (0, "/");
		}
	}

	if (nStartByte != -1)
	{
		m_nFileByteOffset = nStartByte;
	}
	else
	{
		m_nFileByteOffset = 0;
	}

	m_pHTTPService->SocketOpen();

	hResult = HTTPGet (nStartByte);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiUpdateConnection::HTTPGet (
	int nStartByte)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nBytesRead = SOCKET_ERROR;
	EHTTPServiceResult Result = eHTTP_TOOLS_COMPLETE;
	unsigned int unHeaderLength = 0;
	const int nBufferSize = 1000;
	auto pszBuffer = new unsigned char[nBufferSize];
	int nEndByte = -1;

	if (nStartByte != -1)
	{
		nEndByte = nStartByte + m_nNumberOfBytesPerRequest - 1;
	}

	hResult = m_pHTTPService->HTTPGetBegin (m_File.c_str (), nStartByte, nEndByte);
	stiTESTRESULT ();

	//
	// Read the complete header so we get the return code
	// and content length.
	//

	Result = m_pHTTPService->HTTPProgress (pszBuffer, nBufferSize, (unsigned int *)&nBytesRead, &unHeaderLength);

	stiTESTCONDMSG (Result != eHTTP_TOOLS_ERROR && Result != eHTTP_TOOLS_CANCELLED, stiRESULT_ERROR, "Result = %d", Result);
	stiTESTCOND (unHeaderLength > 0, stiRESULT_ERROR);

	//
	// Check the HTTP return code for success.
	//
	if (m_pHTTPService->ReturnCodeGet() < stiHTTP_SUCCESS_MIN || m_pHTTPService->ReturnCodeGet() > stiHTTP_SUCCESS_MAX)
	{
		if (m_pHTTPService->ReturnCodeGet() == stiHTTP_416_RANGE_NOT_SATISFIABLE)
		{
			stiTHROW (stiRESULT_INVALID_RANGE_REQUEST);
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}
	}

	//
	// Move any bytes read after the header into a temporary buffer to
	// be retrieved by the next call to HTTPRead.
	//

	// HTTPRead deletes m_pszTmpBuffer, and HTTPGet blindly allocates it.
	// Assert to make sure we don't accidentally get the call order wrong
	// I'd like to see something like a queue of vectors to manage the memory
	// here.  The current approach is prone to error.
	stiASSERT(m_pszTmpBuffer == nullptr);
	m_unTmpBufferLength = nBytesRead - unHeaderLength;
	m_pszTmpBuffer = new char[m_unTmpBufferLength];

	memcpy (m_pszTmpBuffer, pszBuffer + unHeaderLength, m_unTmpBufferLength);

	if (nStartByte != -1)
	{
		m_nTotalRequestBytesToRead = m_pHTTPService->ContentRangeEndGet() - m_pHTTPService->ContentRangeStartGet() + 1;
		m_nTotalFileBytesToRead = m_pHTTPService->ContentRangeTotalGet();
	}
	else
	{
		m_nTotalRequestBytesToRead = m_pHTTPService->ContentLengthGet ();
		m_nTotalFileBytesToRead = m_pHTTPService->ContentLengthGet ();
	}

STI_BAIL:

	if (pszBuffer)
	{
		delete [] pszBuffer;
		pszBuffer = nullptr;
	}

	return hResult;
}


stiHResult CstiUpdateConnection::HTTPRead (
	char *pszBuffer,
	int nSize,
	int *pnBytesRead)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nBytesRead = SOCKET_ERROR;
	EHTTPServiceResult eResult = eHTTP_TOOLS_COMPLETE;
	unsigned int unHeaderLength = 0;
	int nTmpBufferBytes = 0;

	if (m_nTotalRequestBytesToRead == 0)
	{
		if (m_nTotalFileBytesToRead > 0)
		{
			//
			// Open the socket if it is not already open.
			//			
			m_pHTTPService->SocketOpen();

			hResult = HTTPGet (m_nFileByteOffset);
			stiTESTRESULT ();

			hResult = HTTPRead (&pszBuffer[nTmpBufferBytes], nSize - nTmpBufferBytes, &nBytesRead);
			stiTESTRESULT ();
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}
	}
	else
	{
		//
		// If we have a temporary buffer then move the bytes
		// from it into the buffer to be read into.
		//
		if (m_pszTmpBuffer)
		{
			memcpy (pszBuffer, m_pszTmpBuffer, m_unTmpBufferLength);
			nTmpBufferBytes = m_unTmpBufferLength;

			delete [] m_pszTmpBuffer;
			m_pszTmpBuffer = nullptr;

			m_nTotalRequestBytesToRead -= nTmpBufferBytes;
			m_nTotalFileBytesToRead -= nTmpBufferBytes;
			m_nFileByteOffset += nTmpBufferBytes;
		}

		if (m_nTotalRequestBytesToRead > 0)
		{
			//
			// Read the bytes from the HTTP connection.  Adjust the starting position by the number of bytes that were in the temporary buffer.
			//
			eResult = m_pHTTPService->HTTPProgress ((unsigned char *)&pszBuffer[nTmpBufferBytes], nSize - nTmpBufferBytes, (unsigned int *)&nBytesRead, &unHeaderLength);

			//
			// Check the result code.  If there was an error or if the process was cancelled then
			// throw an error. If the result code is complete but no data was read then the remote
			// host shutdown the connection.
			//
			if (eResult == eHTTP_TOOLS_ERROR || eResult == eHTTP_TOOLS_CANCELLED)
			{
				stiTHROW (stiRESULT_ERROR);
			}
			else if (eResult == eHTTP_TOOLS_COMPLETE && nBytesRead == 0)
			{
				stiTHROW (stiRESULT_ERROR);
			}
			else
			{
				m_nTotalRequestBytesToRead -= nBytesRead;
				m_nTotalFileBytesToRead -= nBytesRead;
				m_nFileByteOffset += nBytesRead;

				//
				// If we have read all of the bytes for this request then close the socket.
				//
				if (m_nTotalRequestBytesToRead == 0)
				{
					m_pHTTPService->SocketClose();
				}
			}
		}
		else
		{
			nBytesRead = 0;
		}
	}

	*pnBytesRead = nBytesRead + nTmpBufferBytes;

STI_BAIL:

	return hResult;
}


void CstiUpdateConnection::Close ()
{
	int nError = 0;
	stiUNUSED_ARG(nError);

	//
	// Quit session
	//
	if (m_pHTTPService)
	{
		m_pHTTPService->SocketClose ();

		delete m_pHTTPService;
	}
}


/*
*  \brief calls correct read for update files
*
*/
stiHResult CstiUpdateConnection::Read (
	char *pszBuffer,
	int nSize,
	int *pnBytesRead)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nBytesRead = 0;

	hResult = HTTPRead (pszBuffer, nSize, &nBytesRead);
	stiTESTRESULT ();

	*pnBytesRead = nBytesRead;

STI_BAIL:

	return hResult;
}

									   
									   
/*
*  \brief opens proper protocol connection
*
*/
stiHResult CstiUpdateConnection::Open (
	const std::string &url,
	int nStartByte,
	int nNumberOfBytes)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_nNumberOfBytesPerRequest = nNumberOfBytes;

	hResult = HTTPOpen(url, nStartByte);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


int CstiUpdateConnection::FileSizeGet ()
{	
	int nFileSize = 0;
	
	nFileSize = m_pHTTPService->ContentRangeTotalGet ();

	if (nFileSize == 0)
	{
		nFileSize = m_pHTTPService->ContentLengthGet ();
	}

	return nFileSize;
}
