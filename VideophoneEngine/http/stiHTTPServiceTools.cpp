////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	stiHTTPServiceTools
//
//	File Name:	stiHTTPServiceTools.cpp
//
//	Owner:		Eugene R. Christensen
//
//	Abstract:
//		Tools for the HTTP component.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiHTTPServiceTools.h"
#include <cctype>			// For isdigit
#include "stiDefs.h"
#include "stiTools.h"		// For IPAddressValidate
#include "stiOS.h"			// OS Abstraction
#include "stiOSNet.h"
#include "stiTrace.h"  // For debugging tools
#include "CstiHostNameResolver.h"
#include "CstiIpAddress.h"
#include <algorithm>
#include <cstdio>
#include <chrono>

using namespace std;

void PrintSocketOpenError (
	const char* pszSubsystem,
	int nErrno,
	int nHandle,
	const char *pszServerIP,
	int nPort,
	const std::string &fileName,
	int lineNumber)
{
	stiASSERTMSG(false, "%s: ERROR %d, \"%s\" on file handle %d, %s:%d at %s:%d\n",
			pszSubsystem, nErrno, strerror (nErrno), nHandle, pszServerIP, nPort,
			fileName.c_str (), lineNumber);
}


////////////////////////////////////////////////////////////////////////////////
//; HeaderReturnCodeGet
//
// Description: Reads the HTTP header from the passed in socket and parses it
//
// Abstract:  This is not guaranteed to read the body as well. Call HttpRead if
// you need the body of the response.
//
// Returns:
//	stiResult:
//		stiRESULT_SUCCESS - The header parsing was a success.
//		stiRESULT_ERROR - An Error has occurred while reading or parsing the header. 
//
stiHResult HttpHeaderRead (
	const int nSocket,
	const int nTimeOut,
	char *pHeaderBuffer,
	unsigned int nHeaderBufferSize,
	unsigned int *punBytesReceived,
	SHTTPResult *pstResult)
{
	EHTTPServiceResult eServiceResult = eHTTP_TOOLS_CONTINUE;
	stiHResult hResult = stiRESULT_SUCCESS;
	fd_set SReadFds;
	int nBytesReceived;
	struct timeval stTimeOut{};
	struct timeval *pstTimeOut = nullptr;
	auto start = std::chrono::steady_clock::now();
	auto elapsedTime = 0;

	stiFD_ZERO (&SReadFds);
	stiFD_SET(nSocket, &SReadFds);

	stTimeOut.tv_sec = nTimeOut;
	stTimeOut.tv_usec = 0;
	pstTimeOut = &stTimeOut;

	// wait for data to come in on the socket
	auto result = stiOSSelect (stiFD_SETSIZE, &SReadFds, nullptr, nullptr, pstTimeOut);
	stiTESTCONDMSG (result > 0, stiRESULT_ERROR, "HttpHeaderRead failed with result %d", result);

	nBytesReceived = stiOSRead (nSocket, pHeaderBuffer, nHeaderBufferSize);

	// Did we get a response?
	stiTESTCOND (0 < nBytesReceived, stiRESULT_SRVR_DATA_HNDLR_DOWNLOAD_ERROR);

	// Parse the HTTP header but first null terminate the string.
	*(pHeaderBuffer + nBytesReceived) = '\0';
	eServiceResult = HeaderParse (pHeaderBuffer, pstResult);
	
	elapsedTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count());

	// If the parse returned "CONTINUE", it signifies that the full header isn't yet here.
	// Loop until we get the whole header.
	int nAddBytesReceived;
	int nBuffSize;
	
	while (eHTTP_TOOLS_CONTINUE == eServiceResult &&
		   (nTimeOut > elapsedTime))
	{
		nBuffSize = nHeaderBufferSize - nBytesReceived;

		// If nBuffSize is 0 then we filled the buffer but didn't get the entire header.
		// We need more space so return an error.
		stiTESTCOND (0 < nBuffSize, stiRESULT_ERROR);
		
		stiFD_ZERO (&SReadFds);
		stiFD_SET(nSocket, &SReadFds);
		stTimeOut.tv_sec = nTimeOut - elapsedTime;
		stTimeOut.tv_usec = 0;
		
		auto result = stiOSSelect (stiFD_SETSIZE, &SReadFds, nullptr, nullptr, &stTimeOut);
		stiTESTCONDMSG (result > 0, stiRESULT_ERROR, "HttpHeaderRead failed to get full header result = %d received = %d", result, nBytesReceived);
		
		elapsedTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count());

		nAddBytesReceived = stiOSRead (nSocket, (char *)(pHeaderBuffer + nBytesReceived), nBuffSize - 1);

		// Did we get some more data?
		stiTESTCOND (0 < nAddBytesReceived, stiRESULT_ERROR);

		nBytesReceived += nAddBytesReceived;
		*(pHeaderBuffer + nBytesReceived) = '\0';
		eServiceResult = HeaderParse ((char*)pHeaderBuffer, pstResult);
	}

	// Header was succesfully parsed.
	stiTESTCONDMSG (eHTTP_TOOLS_COMPLETE == eServiceResult, stiRESULT_ERROR, "HttpHeaderRead failed to get full header only received = %d", nBytesReceived);

	if (punBytesReceived)
	{
		*punBytesReceived = nBytesReceived;
	}

STI_BAIL:

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; HeaderFieldParse
//
// Description: Parses a single HTTP Header field
//
// Abstract:
//
// Returns:
//	CstiHTTPService::EFieldResult:
//		eFIELD_EMPTY - An empty field encountered (i.e. the end of the header)
//		eFIELD_COMPLETE - The field has been correctly parsed
//		eFIELD_ERROR - An error has occurred
//		eFIELD_INCOMPLETE - End of buffer reached before field end reached
//
EFieldResult HeaderFieldParse (
	IN const char * szReply,
	OUT const char *& szFieldBegin,
	OUT const char *& szValueBegin,
	OUT const char *& szFieldEnd,
	OUT const char *& szNextChar,
	OUT unsigned int & unFieldLen)
{
	EFieldResult eResult = eFIELD_INCOMPLETE;
	stiLOG_ENTRY_NAME (HeaderFieldParse);

	// Initialize the return values.
	szFieldBegin = nullptr;
	szValueBegin = nullptr;
	szFieldEnd = nullptr;
	szNextChar = szReply;
	unFieldLen = 0;

	// Is our current position at a CR or LF?
	if ('\r' == *szReply || '\n' == *szReply)
	{
		// Yes! Can we advance past the end of line?
		EstiResult eLineResult = EndLineFind (
			szReply, szReply);
		if (eLineResult != estiERROR)
		{
			// Yes! This field is empty.
			szNextChar = szReply;
			eResult = eFIELD_EMPTY;
		} // end if
	} // end if
	else
	{
		const char * szNextFieldChar = nullptr;

		// Look for the line end
		EstiResult eLineResult = EndLineFind (
			szReply, szNextFieldChar);

		// Did we find a line end?
		if (estiERROR != eLineResult)
		{
			// Yes! Set the begin and end pointers
			szFieldBegin = szReply;
			szFieldEnd = szNextFieldChar;

			// Back up from the end of the field to skip over the CRLF
			do
			{
				szFieldEnd--;
			} while ('\r' == *szFieldEnd || '\n' == *szFieldEnd);

			// Set the other return values
			szNextChar = szNextFieldChar;
			unFieldLen = (unsigned int)(szFieldEnd - szFieldBegin);

			// Try to find the value in the field...
			const char * szValueFind = szFieldBegin;
			while (szValueFind <= szFieldEnd)
			{
				// Is this the colon?
				if (':' == *szValueFind)
				{
					// Yes! the value is the next non-space char, so skip
					// the whitespace...
					do
					{
						szValueFind++;
					} while (' ' == *szValueFind || '\t' == *szValueFind);

					// Found the value. Store it.
					szValueBegin = szValueFind;
					break;
				} // end if

				szValueFind++;
			} // end while

			eResult = eFIELD_COMPLETE;
		} // end if
	} // end else

	return (eResult);
} // end HeaderFieldParse

////////////////////////////////////////////////////////////////////////////////
//; HeaderParse
//
// Description: Parses the HTTP header
//
// Abstract:
//
// Returns:
//	EHTTPServiceResult:
//		eHTTP_TOOLS_COMPLETE - The header parsing is complete.
//		eHTTP_TOOLS_ERROR - An Error has occurred. Close the socket and fail.
//		eHTTP_TOOLS_CONTINUE - The header is not complete, keep reading from the socket.
//
EHTTPServiceResult HeaderParse (
	IN const char * szBuffer,
	OUT SHTTPResult * pstResult)
{
	const char szCONTENT_LENGTH[] = "Content-Length";
	const char szCONTENT_RANGE[] = "Content-Range";
	EHTTPServiceResult eResult = eHTTP_TOOLS_CONTINUE;
	stiLOG_ENTRY_NAME (HeaderParse);

	const char * szReply = szBuffer;
	unsigned int unReturnCode = 0;

	// Parse the return code.
	EFieldResult eFieldResult = ReturnCodeParse (
		szReply, unReturnCode, szReply);

	// Did we get the return code OK?
	if (eFieldResult == eFIELD_COMPLETE)
	{
		// Yes! Set the return code.
		pstResult->unReturnCode = unReturnCode;

		// Parse all of the header fields
		do
		{
			const char * szFieldBegin;
			const char * szValueBegin;
			const char * szFieldEnd;
			unsigned int unFieldLen;

			eFieldResult = HeaderFieldParse (
				szReply,
				szFieldBegin,
				szValueBegin,
				szFieldEnd,
				szReply,
				unFieldLen);

			// Was there an error or have we already parsed the whole header?
			if (eFIELD_COMPLETE != eFieldResult)
			{
				break;
			}
			// Was this the content length tag?
			if (0 == stiStrNICmp (szFieldBegin, szCONTENT_LENGTH, strlen (szCONTENT_LENGTH)))
			{
				// Yes! Read the length from the value.
				if (szValueBegin != nullptr)
				{
					pstResult->unContentLen = atoi (szValueBegin);
				} // end if
			} // end if
			//
			// Was the Content-Range tag?
			//
			else if (0 == stiStrNICmp (szFieldBegin, szCONTENT_RANGE, strlen (szCONTENT_RANGE)))
			{
				// Yes! Read the start byte, end byte, and total length from the value.
				if (szValueBegin != nullptr)
				{
					sscanf (szValueBegin, "bytes %u-%u/%u", &pstResult->unContentRangeStart, &pstResult->unContentRangeEnd, &pstResult->unContentRangeTotal);
				} // end if
			}
		}
		while (eFIELD_COMPLETE == eFieldResult);

		// Did we reach the end of the header?
		if (eFIELD_EMPTY == eFieldResult)
		{
			// Yes! Set the return values.
			pstResult->unHeaderLen = (unsigned int)(szReply - szBuffer);
			eResult = eHTTP_TOOLS_COMPLETE;
		} // end if

		// Was there an error?
		if (eFIELD_ERROR == eFieldResult)
		{
			// Yes! Set the return value.
			eResult = eHTTP_TOOLS_ERROR;
		} // end if
	} // end if

	return (eResult);
} // end CstiHTTPService::HeaderParse

////////////////////////////////////////////////////////////////////////////////
//; HttpSocketConnect
//
// Description: Sets socket options and makes the connection to the HTTP server.
//
// Abstract:
//
// Returns:
//	EstiOK upon success, EstiERROR otherwise.
//
stiHResult HttpSocketConnect (
	IN stiSocket Sock,  					///< The socket to connect on
	IN const char* pszServerIP,		///< The ip address of the HTTP server
	IN const int nPort, 			///< A port other than the default
	IN const int nTimeOut)			///< Timeout value in seconds
{
	stiLOG_ENTRY_NAME (HttpSocketConnect);
	stiHResult hResult = stiRESULT_SUCCESS;

	struct linger       Linger{};
	int 				yes{TRUE};
	struct timeval  	stTimeOut{};
	int					nAddrLength{0};

#ifdef IPV6_ENABLED
	struct sockaddr_storage serverAddr;
	hResult = stiSockAddrStorageCreate (pszServerIP, nPort, SOCK_STREAM, &serverAddr);
	if (hResult == stiRESULT_SUCCESS)
	{
		nAddrLength = serverAddr.ss_len;
	}
#else
	struct sockaddr_in serverAddr{};
	// Continue to connect to the socket.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (nPort);
	inet_pton (AF_INET, pszServerIP, &serverAddr.sin_addr);
	nAddrLength = sizeof(serverAddr);
#endif

	// Turn on the linger option TCP will simply drop the connection
	// by sending out a RST
	Linger.l_onoff = TRUE;
	Linger.l_linger = 0;

	// Set the timeout value
	stTimeOut.tv_sec = nTimeOut;
	stTimeOut.tv_usec = 0;


	// Were the socket options successfully set?
	/*	Windows expects MILLISECONDS to be specified for the SO_SNDTIMEO and the SO_RCVTIMEO timeouts.
		These are corrected in stiOSWin32Net.cpp |stiOSSetsockopt
	*/
	if (0 == stiOSSetsockopt (
		Sock, SOL_SOCKET, SO_LINGER,
		(const char *)&Linger, sizeof (Linger)) &&
		0 == stiOSSetsockopt (
			Sock, IPPROTO_TCP, TCP_NODELAY,
			(const char *)&yes, sizeof (yes)) &&
		0 == stiOSSetsockopt (
			Sock, SOL_SOCKET, SO_KEEPALIVE,
			(const char *)&yes, sizeof (yes)) &&
		0 == stiOSSetsockopt (
			Sock, SOL_SOCKET, SO_SNDTIMEO,
			(const char *)&stTimeOut, sizeof (stTimeOut)) &&
		0 == stiOSSetsockopt (
			Sock, SOL_SOCKET, SO_RCVTIMEO,
			(const char *)&stTimeOut, sizeof (stTimeOut)))
	{
		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace ("SocketOpen: Options set.\n");
		); // stiDEBUG_TOOL

		// Make the socket non-blocking so we can have better
		// control over timeouts using a select statement. Once
		// the socket is connected it will be switched back to blocking.
		socketNonBlockingSet (Sock);

		hResult = stiOSConnect (
			Sock,
			(struct sockaddr *)
			&serverAddr,
			nAddrLength);

		if (stiIS_ERROR (hResult))
		{
			PrintSocketOpenError ("SocketOpen", errno, Sock, pszServerIP, nPort, __FILE__, __LINE__);
			stiTHROW (stiRESULT_ERROR);
		}
		else if (stiRESULT_CONNECTION_IN_PROGRESS == hResult)
		{
			fd_set sWaitForHandles;
			struct timeval tv {};

			tv.tv_sec = 3; // 3 seconds should be plenty of time
			tv.tv_usec = 0;
			FD_ZERO (&sWaitForHandles);
			FD_SET (Sock, &sWaitForHandles);
			int nResult = select (Sock + 1, nullptr, &sWaitForHandles, nullptr, &tv);

			if ((nResult < 0) && (errno != EINTR))
			{
				// We couldn't wait for the connection for some reason.
				PrintSocketOpenError ("Select", errno, Sock, pszServerIP, nPort, __FILE__, __LINE__);
				stiTHROW (stiRESULT_ERROR);
			}
			else if (nResult > 0)
			{
				// Open has completed.  Check its error code.
				socklen_t nSize = sizeof (int);

				nResult = stiOSGetsockopt (Sock, SOL_SOCKET, SO_ERROR, &errno, &nSize);

				if (nResult < 0)
				{
					PrintSocketOpenError ("SocketOpen Thread", errno, Sock, pszServerIP, nPort, __FILE__, __LINE__);
					stiTHROW (stiRESULT_ERROR);
				}
			}
			else
			{
				// Socket open did not return within the time allowed.
				PrintSocketOpenError ("SocketOpen Timeout", errno, Sock, pszServerIP, nPort, __FILE__, __LINE__);
				stiTHROW (stiRESULT_ERROR);
			}
		}
	}

STI_BAIL:

	// Return the socket back to a blocking socket
	socketBlockingSet (Sock);

	return (hResult);
} // end HttpSocketConnect


////////////////////////////////////////////////////////////////////////////////
//; ReturnCodeParse
//
// Description: Parses the HTTP Header result field
//
// Abstract:
//	Reads the result code from an HTTP Header.
//
// Returns:
//	EFieldResult:
//	eFIELD_EMPTY - An empty field encountered (i.e. the end of the header)
//	eFIELD_COMPLETE - The field has been correctly parsed
//	eFIELD_ERROR - An error has occurred
//	eFIELD_INCOMPLETE - End of buffer reached before field end reached
//
EFieldResult ReturnCodeParse (
	IN const char * szReply,
	OUT unsigned int &unReturnCode,
	OUT const char *& szNextChar)
{
	EFieldResult eResult = eFIELD_INCOMPLETE;
	stiLOG_ENTRY_NAME (ReturnCodeParse);

	// Initialize the return values
	unReturnCode = 0;
	szNextChar = szReply;

	// Can we find the "HTTP/" string in the return?
	szReply = strstr (szReply, "HTTP/");

	if (nullptr != szReply)
	{
		// Yes! Skip over the protocol number.
		szReply += strlen ("HTTP/");
		while (isdigit (*szReply) || '.' == *szReply)
		{
			szReply++;
		} // end while

		// Skip over white space
		while (' ' == *szReply || '\t' == *szReply)
		{
			szReply++;
		} // end while

		// Scan the number
		int nResultLen = 0;
		while (isdigit (*szReply))
		{
			unReturnCode *= 10;
			unReturnCode += (*szReply) - '0';
			nResultLen++;
			szReply++;
		} // end while

		// Did we get a valid result?
		if (nResultLen != 3)
		{
			// No! Are we at the end of the buffer?
			if (*szReply != 0)
			{
				// No! There was an error reading the header.
				eResult = eFIELD_ERROR;
			} // end if
		} // end if
		else
		{
			// Move to the end of the line
			EstiResult eLineResult = EndLineFind (szReply, szReply);

			// Were we able to find the end of the line?
			if (estiOK == eLineResult)
			{
				// Yes! Return what we parsed.
				eResult = eFIELD_COMPLETE;
				szNextChar = szReply;
			} // end if
		} // end else
	} // end if

	return (eResult);
} // end ReturnCodeParse

////////////////////////////////////////////////////////////////////////////////
//; EndLineFind
//
// Description: Skips to the end of a header line
//
// Abstract:
//	Skips all characters until an end of line (LF) is reached. Handles
//	optional CR characters.
//
// Returns:
//	EstiResult:
//		estiOK if end of line was found.
//		estiERROR if end of buffer was encountered before end of line.
//
EstiResult EndLineFind (
	IN const char * szReply,
	OUT const char *& szNextChar)
{
	EstiResult eResult = estiERROR;
	stiLOG_ENTRY_NAME (EndLineFind);

	// Initialize return values
	szNextChar = szReply;

	// Skip over anything except CRLF and null
	while ('\r' != *szReply && '\n' != *szReply && 0 != *szReply)
	{
		szReply++;
	} // end while

	// Skip over a single carriage return (if necessary)
	if ('\r' == *szReply)
	{
		szReply++;
	} // end if

	// Did we find a line feed?
	if ('\n' == *szReply)
	{
		// Yes! We have success!
		szReply++;
		eResult = estiOK;
		szNextChar = szReply;
	} // end if

	return (eResult);
} // end EndLineFind



////////////////////////////////////////////////////////////////////////////////
//; stiURLParsePrep
//
// Description: Parses and prepares the URL to pass to stiDNSGetHostByName
//
void stiURLParsePrep (
	const char * pszURL,
	URIScheme *scheme,
	std::string *pServer,
	unsigned short* pnPort,
	std::string* pFile)
{

	EstiBool bFoundFile = estiFALSE;			// Found file flag

	// Does the URL begin with "HTTP:"?
	const char szHTTP[] = "HTTP:";
	if (0 == stiStrNICmp (pszURL, szHTTP, strlen (szHTTP)))
	{
		// Yes! Advance the pointer past it.
		pszURL += strlen (szHTTP);

		if (scheme)
		{
			*scheme = URIScheme::HTTP;
		}
	} // end if

	// Does the URL begin with "HTTPS:"?
	const char szHTTPS[] = "HTTPS:";
	if (0 == stiStrNICmp (pszURL, szHTTPS, strlen (szHTTPS)))
	{
		// Yes! Advance the pointer past it.
		pszURL += strlen (szHTTPS);

		if (scheme)
		{
			*scheme = URIScheme::HTTPS;
		}
	} // end if

	// Does the URL begin with "//"?
	const char szSlashes[] = "//";
	if (0 == stiStrNICmp (pszURL, szSlashes, strlen (szSlashes)))
	{
		// Yes! Advance the pointer past it.
		pszURL += strlen (szSlashes);
	} // end if
	
	// Does the Host begin with "["?
	if (*pszURL == '[')
	{
		// Yes! Advance the pointer past it.
		pszURL ++;
	} // end if

	size_t Length = strlen (pszURL);
	const char * szPortStart = nullptr;
	const char * szFileStart = nullptr;
	int nNumColons = 0;

	// Separate the file name and port from the server
	for (size_t i = 0; i < Length; i++)
	{
		// If a colon was found, the port follows
		if (pszURL[i] == ':')
		{
			// Retrieve the server
			if (pServer)
			{
				if (pszURL[i-1] == ']')
				{
					pServer->assign (pszURL, i-1);
				}
				else
				{
					pServer->assign (pszURL, i);
				}
			}

			if (nNumColons > 0) // ipv6 address
			{
				if(pszURL[i-1]==']') // if there is a port, the ip is enclosed in []
				{
					szPortStart = &pszURL[i+1];
				}
				else // this is not a port, but an ipv6 hex segment
				{
					szPortStart = nullptr;
				}
			}
			else // ipv4 address or domain name
			{
				szPortStart = &pszURL[i+1];
			}
			++nNumColons;
		} // end if

		// If a backslash or forward slash was found, the file follows
		if (pszURL[i] == '\\' || pszURL[i] == '/')
		{
			// Found a file
			szFileStart = &pszURL[i+1];
			bFoundFile = estiTRUE;

			// Did we find a port?
			if (szPortStart != nullptr)
			{
				// Yes! Retrieve the port
				std::string ServerPort;
				size_t sizCount = (int)(&pszURL[i] - szPortStart);
				ServerPort.assign (szPortStart, sizCount);
				*pnPort = atoi (ServerPort.c_str ());
			} // end if
			else
			{
				// No port was found.
				*pnPort = 0;

				if (pServer)
				{
					// Retrieve the server
					if (pszURL[i] == ']')
					{
						pServer->assign (pszURL, i - 1);
					}
					else
					{
						pServer->assign (pszURL, i);
					}
				} // end else
			}

			// Retrieve the file
			*pFile = szFileStart;
			break;
		} // end if
	} // end for

	// Was a file found?
	if (bFoundFile)
	{
		// Yes! Replace the remaining backslashes with forward slashes.
		std::replace (pFile->begin(), pFile->end(), '\\', '/');
	} // end if
	else
	{
		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace ("URLParse: No File found\n");
		); // stiDEBUG_TOOL

		if (pServer)
		{
			// No file was found. The entire URL was the server
			*pServer = pszURL;
		}
	} // end else
}
	

////////////////////////////////////////////////////////////////////////////////
//; stiURLParse
//
// Description: Parses the HTTP URL
//
// Abstract:
//	Parses the pieces of the HTTP URL. The ip address of the server
//	and the filename (or file URL) of the request must be returned.
//
// Returns:
//	EURLParseResult:
//		eURL_OK -  Everything OK
//		eINVALID_URL - The specified URL is invalid
//		eDNS_FAIL - The DNS resolve failed
//		eDNS_CANCELLED - The action has been cancelled
//
EURLParseResult stiURLParse (
	IN const char * pszURL,
	OUT std::vector<std::string> *pServerIPList,
	OUT unsigned short *pnPort,
	OUT std::string *pFile,
	OUT std::string *pServerName,
	URIScheme *scheme,
	IN const char *pszURLalt)	/// Alternate URL for DNS robustness support, may be nullptr
{
	stiLOG_ENTRY_NAME (stiURLParse);

	EURLParseResult	eResult = eDNS_FAIL;
	
	// Was a good url sent?
	if (nullptr != pszURL)
	{
		std::string ServerName;
		std::string ServerNameAlt;

		// We will require this information even if the caller does not
		if (!pServerName)
		{
			pServerName = &ServerName;
		}

		stiURLParsePrep(
			pszURL,
			scheme,
			pServerName,
			pnPort,
			pFile);
		
		if (nullptr != pszURLalt)
		{
			// There was an alternate URL passed in... get the server from that also

			// NOTE: since we won't return the alternate path info, use local values
			unsigned short nPortAlt;
			std::string FileAlt;

			stiURLParsePrep(
				pszURLalt,
				scheme,
				&ServerNameAlt,
				&nPortAlt,
				&FileAlt);
				
			if (pServerName->empty ())
			{
				// Move this to the primary
				*pServerName = ServerNameAlt;
				ServerNameAlt.clear ();
			}
		}

		// Did anything end up in the server field?
		if (pServerName->empty ())
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("URLParse: Invalid Server!\n");
			); // stiDEBUG_TOOL

			// No! This is an invalid URL.
			eResult = eDNS_FAIL;
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("URLParse: Server = %s\n", pServerName->c_str ());
			); // stiDEBUG_TOOL

			// Yes! Can we resolve the server into an IP address?
			stiHResult hResult;
			addrinfo *pstAddrInfo = nullptr;
			
			CstiHostNameResolver *poResolver = CstiHostNameResolver::getInstance ();
			
			poResolver->Resolve (pServerName->c_str (), ServerNameAlt.empty () ? nullptr : ServerNameAlt.c_str (), &pstAddrInfo);
			if (nullptr != pstAddrInfo)
			{
				hResult = poResolver->IpListFromAddrinfoCreate (pstAddrInfo, pServerIPList);
				if (!stiIS_ERROR (hResult) && !pServerIPList->empty())
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("URLParse: Success!\n");
					); // stiDEBUG_TOOL

					// Yes! We have succeeded. Return the requested items.
					eResult = eURL_OK;
				} // end if
			} // end if
		} // end else
	} // end if

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("stiURLParse: In: %s\n", pszURL);
		for (auto it = pServerIPList->begin (); it != pServerIPList->end (); ++it)
		{
			stiTrace ("stiURLParse: Server: %s\n", it->c_str ());
		}
		stiTrace ("stiURLParse: Port: %d\n", *pnPort);
		stiTrace ("stiURLParse: File: %s\n", pFile->c_str ());
	); // stiDEBUG_TOOL

	return (eResult);
} // end stiURLParse


// THIS IS DEPRECATED.
EURLParseResult stiURLParse (
	IN const char * pszURL,
	OUT char * pszServerIP,
	IN int nServerIPLen,
	OUT char * pszServerPort,
	IN int nServerPortLen,
	OUT char * pszFile,
	IN int nFileLen,
	OUT char *pszServerName,
	IN int nServerNameLen,
	IN const char * pszURLalt)
{
	unsigned short nPort = 0;
	std::string File;
	std::string ServerName;
	std::vector<std::string> ServerIPList;
	
	EURLParseResult eResult = stiURLParse (
		pszURL,
		&ServerIPList,
		&nPort,
		&File,
		&ServerName,
		nullptr,
		pszURLalt);
	
	if (eResult == eURL_OK)
	{
		if (pszServerPort)
		{
			snprintf (pszServerPort, nServerPortLen, "%d", nPort);
		}

		if (pszFile)
		{
			strncpy (pszFile, File.c_str (), nFileLen - 1);
			pszFile[nFileLen - 1] = '\0';
		}

		if (pszServerName)
		{
			strncpy (pszServerName, ServerName.c_str (), nServerNameLen - 1);
			pszServerName[nServerNameLen - 1] = '\0';
		}

		if (pszServerIP && !ServerIPList.empty ())
		{
			strncpy (pszServerIP, ServerIPList[0].c_str (), nServerIPLen - 1);
			pszServerIP[nServerIPLen - 1] = '\0';
		}
	}
	
	return eResult;
}


/*!\brief Called to read an HTTP response header and all.
 *
 *  NOTE: This will continue to call select and read until either the HTTP header
 *	and data length specified in the content is read or time out is reached.
 *
 * \param socket	Socket to perform read/select on.
 * \param timeOut	Time to wait for data.
 * \param dataBuffer	Buffer to read into
 * \param dataBufferSize	Size of buffer data will be read into
 * \param bytesRecieved	Bytes read in
 * \param httpResult	Structure for HTTP data
 *
 *  \retval stiRESULT_SUCCESS If the header and full content lenght of data was read.
 */
stiHResult HttpResponseRead (
	const int socket,
	const int timeOut,
	char *dataBuffer,
	unsigned int dataBufferSize,
	unsigned int *bytesRecieved,
	SHTTPResult *httpResult)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int received = 0;
	
	auto start = std::chrono::steady_clock::now();
	auto elapsedTime = 0;
	
	hResult = HttpHeaderRead (socket, timeOut, dataBuffer, dataBufferSize, &received, httpResult);
	stiTESTRESULT();
	
	elapsedTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count());
	
	if (received < (httpResult->unContentLen + httpResult->unHeaderLen))
	{
		while ((timeOut > elapsedTime) &&
			   received < (httpResult->unContentLen + httpResult->unHeaderLen))
		{
			fd_set readFd;
			stiFD_ZERO (&readFd);
			stiFD_SET(socket, &readFd);
			struct timeval timeToWait = {0};
			
			timeToWait.tv_sec = timeOut - elapsedTime;
			timeToWait.tv_usec = 0;
			
			// wait for data to come in on the socket
			auto result = stiOSSelect (stiFD_SETSIZE, &readFd, nullptr, nullptr, &timeToWait);
			stiTESTCONDMSG (result > 0, stiRESULT_ERROR, "Failed to receive all data, result = %d \nreceived = %d \nContentLength = %d \nHeaderLength = %d", result, received, httpResult->unContentLen, httpResult->unHeaderLen);
			
			elapsedTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count());
		
			received += stiOSRead (socket, &dataBuffer[httpResult->unHeaderLen], httpResult->unContentLen);
		}
		*(dataBuffer + received) = '\0';
	}
	
	stiASSERTMSG (received == (httpResult->unContentLen + httpResult->unHeaderLen), "HttpResponseRead expected = %d data but received = %d", httpResult->unContentLen + httpResult->unHeaderLen, received);
	
	if (bytesRecieved)
	{
		*bytesRecieved = received;
	}

STI_BAIL:
	
	return hResult;
}

// end file stiHTTPServiceTools.cpp

