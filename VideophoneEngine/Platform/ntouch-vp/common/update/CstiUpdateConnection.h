// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef UPDATEPUTILS_H
#define UPDATEPUTILS_H

#include "CstiHTTPService.h"

#define INVALID_SOCKET	   (-1)

class CstiUpdateConnection
{
public:

	CstiUpdateConnection () = default;

	CstiUpdateConnection (const CstiUpdateConnection &other) = delete;
	CstiUpdateConnection (CstiUpdateConnection &&other) = delete;
	CstiUpdateConnection &operator= (const CstiUpdateConnection &other) = delete;
	CstiUpdateConnection &operator= (CstiUpdateConnection &&other) = delete;

	~CstiUpdateConnection ();

	stiHResult Open (
		const std::string &url,
		int nStartByte = -1,
		int nNumberOfBytes = -1);

	void Close ();

	stiHResult Read (
		char* pszBuffer,
		int nSize,
		int *pnBytesRead);
	
	int FileSizeGet ();

private:

	stiHResult HTTPOpen (
		const std::string &url,
		int nStartByte);

	stiHResult HTTPGet (
		int nStartByte);

	stiHResult HTTPRead (
		char *pszBuffer,
		int nSize,
		int *pnBytesRead);

	int HTTPProgress (
		char *pszBuffer,
		int nSize);

	CstiHTTPService *m_pHTTPService{nullptr};
	char *m_pszTmpBuffer{nullptr};
	unsigned int m_unTmpBufferLength{0};
	int m_nNumberOfBytesPerRequest{0};
	int m_nTotalRequestBytesToRead{0};
	int m_nTotalFileBytesToRead{0};
	int m_nFileByteOffset{0};
	std::string m_File;
};

#define UNKNOWN_BYTES2READ 	(-1)
#define DOWNLOAD_BUFFER_LENGTH	4096

#endif // UPDATEPROTOCOL_H

