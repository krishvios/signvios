////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	stiHTTPServiceTools
//
//	File Name:	stiHTTPServiceTools.h
//
//	Owner:		Eugene R. Christensen
//
//	Abstract:
//		See stiHTTPServiceTools.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STIHTTPSERVICETOOLS_H
#define STIHTTPSERVICETOOLS_H

//
// Includes
//
#include "stiSVX.h"
#include "nonstd/observer_ptr.h"
#include <string>
#include <vector>

//
// Constants
//
#ifndef TRUE
#define TRUE 1
#endif

#define stiHTTP_100_CONTINUE				100
#define stiHTTP_200_OK						200
#define stiHTTP_400_BAD_REQUEST				400
#define stiHTTP_404_NOT_FOUND				404
#define stiHTTP_416_RANGE_NOT_SATISFIABLE	416
#define stiHTTP_503_SERVICE_UNAVAILABLE		503

#define stiHTTP_SUCCESS_MIN					200
#define stiHTTP_SUCCESS_MAX					299

//
// Typedefs
//
struct SHTTPResult
{
	SHTTPResult () = default;

	~SHTTPResult () = default;

	SHTTPResult (const SHTTPResult &other) = delete;
	SHTTPResult (SHTTPResult &&other) = delete;
	SHTTPResult &operator= (const SHTTPResult &other) = delete;
	SHTTPResult &operator= (SHTTPResult &&other) = delete;

	void *pRequestor{nullptr};
	unsigned int unRequestNum{0};
	unsigned int unReturnCode{0};
	unsigned int unHeaderLen{0};
	unsigned int unContentLen{0};
	unsigned int unContentRangeStart{0};
	unsigned int unContentRangeEnd{0};
	unsigned int unContentRangeTotal{0};
	nonstd::observer_ptr<char> body;
	std::vector<char> resultBuffer;

};

typedef enum EHTTPServiceResult
{
	eHTTP_TOOLS_COMPLETE,	// The result is complete.
	eHTTP_TOOLS_ERROR,		// An Error has occurred. Close the socket and fail.
	eHTTP_TOOLS_CANCELLED,	// The action has been cancelled
	eHTTP_TOOLS_CONTINUE,	// The result is not complete, keep the socket open.
} EHTTPServiceResult;

typedef enum EFieldResult
{
	eFIELD_EMPTY,		// An empty field encountered (i.e. the end of the header)
	eFIELD_COMPLETE,	// The field has been correctly parsed
	eFIELD_ERROR,		// An error has occurred
	eFIELD_INCOMPLETE,	// End of buffer reached before field end
} EFieldResult;

typedef enum EURLParseResult
{
	eURL_OK,			// Everything OK
	eINVALID_URL,	// The specified URL is invalid
	eDNS_FAIL,		// The DNS resolve failed
	eDNS_CANCELLED,	// The action has been cancelled
} EURLParseResult;

enum class URIScheme
{
	HTTP,
	HTTPS
};

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//
stiHResult HttpHeaderRead (
	int nSocket,
	int nTimeOut,
	char *pDataBuffer,
	unsigned int nDataBufferSize,
	unsigned int *pnBytesRecieved,
	SHTTPResult *pstResult);

EHTTPServiceResult HeaderParse (
	IN const char * szBuffer,
	OUT SHTTPResult * pstResult);

EFieldResult ReturnCodeParse (
	IN const char * szReply,
	OUT unsigned int &unReturnCode,
	OUT const char *& szNextChar);

EstiResult EndLineFind (
	IN const char * szReply,
	OUT const char *& szNextChar);

stiHResult HttpSocketConnect (
	IN stiSocket Sock,
	IN const char* pszServerIP, 
	IN int nServerPort,
	IN int nTimeoutValue);

EURLParseResult stiURLParse (
	IN const char * pszURL,
	OUT std::vector<std::string> *pServerIPList,
	OUT unsigned short *pnPort,
	OUT std::string *pFile,
	OUT std::string *pServerName,
	URIScheme *protocol,
	IN const char *pszURLalt);
	
// THIS IS DEPRECATED.
EURLParseResult stiURLParse (
	IN const char * pszURL,
	OUT char * pszServerIP,
	IN int nServerIPLen,
	OUT char * pszServerPort,
	IN int nServerPortLen,
	OUT char * pszFile,
	IN int nFileLen,
	OUT char *pszServerName = nullptr,
	IN int nServerNameLen = 0,
	IN const char * pszURLalt = nullptr);

stiHResult HttpResponseRead (
	int socket,
	int timeOut,
	char *dataBuffer,
	unsigned int dataBufferSize,
	unsigned int *bytesRecieved,
	SHTTPResult *result);
	

#endif // CSTIHTTPSERVICETOOLS_H
// end file stiHTTPServiceTools.h
