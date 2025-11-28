////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiHTTPService
//
//	File Name:	CstiHTTPService.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		See CstiHTTPService.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIHTTPSERVICE_H
#define CSTIHTTPSERVICE_H

//
// Includes
//

#include "stiSVX.h"
#include "stiHTTPServiceTools.h"
#include <string>
#include <vector>

#include <openssl/ssl.h>

//
// Constants
//

//
// Typedefs
//

typedef struct SHTTPVariable
{
	char szName[un8stiUSER_ID_LENGTH];
	char szValue[un8stiEMAIL_ADDRESS_LENGTH];
}SHTTPVariable;

enum EstiHTTPState
{
//	eHTTP_INVALID,
	eHTTP_DNS_ERROR,  
	eHTTP_ERROR,
	eHTTP_CANCELLED,
	eHTTP_ABANDONED,
	eHTTP_IDLE,
	eHTTP_DNS_RESOLVING,
	eHTTP_SENDING,
	eHTTP_WAITING_4_RESP,
	eHTTP_COMPLETE,
	eHTTP_TERMINATED
};

typedef stiHResult (*PstiHTTPCallback) (
	uint32_t un32RequestNum,
	EstiHTTPState eState,
	SHTTPResult *pstResult,
	void *pParam1);

struct SHTTPFormPost
{
	void *pRequestor {nullptr};
	unsigned int unRequestNum {0};
	char szURL[un8stiMAX_URL_LENGTH] {};
	SHTTPVariable * astVariables {nullptr};
	PstiHTTPCallback pfnCallback {nullptr};
	void *pParam1 {nullptr};
	EstiBool bUseSSL {estiTRUE};
};

struct SHTTPPost
{
	void *pRequestor {nullptr};
	unsigned int unRequestNum {0};
	std::string url;
	std::string urlAlt;
	std::string bodyData;
	PstiHTTPCallback pfnCallback {nullptr};
	void * pParam1 {nullptr};
	EstiBool bUseSSL {estiTRUE};
};

struct SHTTPGet
{
	void *pRequestor {nullptr};
	unsigned int unRequestNum {0};
	char szURL[un8stiMAX_URL_LENGTH] {};
	PstiHTTPCallback pfnCallback {nullptr};
	void * pParam1 {nullptr};
	EstiBool bUseSSL {estiTRUE};
};


//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiHTTPService
{
public:
	CstiHTTPService () = default;
	~CstiHTTPService ();

	CstiHTTPService (const CstiHTTPService &other) = delete;
	CstiHTTPService (CstiHTTPService &&other) = delete;
	CstiHTTPService &operator= (const CstiHTTPService &other) = delete;
	CstiHTTPService &operator= (CstiHTTPService &&other) = delete;

	stiHResult Abandon ();

	EstiBool ActivityCheck (
		IN int nMaxInactivity);

	inline void Cancel ();
	inline EstiBool Cancelled ();

	stiHResult HTTPGetBegin (
		const char *pszFile,
		int nStartByte = -1,
		int nEndByte = -1);
	
	int ContentLengthGet ();

	int ContentRangeStartGet ();

	int ContentRangeEndGet ();

	int ContentRangeTotalGet ();

	int ReturnCodeGet ();

#if 0
	EstiResult HTTPFormPostBegin (
		IN const char * szFile,
		IN const SHTTPVariable * astVariables);
#endif

	EstiResult HTTPPostBegin (
		const std::string &file,
		const std::string &bodyData);

	EHTTPServiceResult HTTPProgress (
		unsigned char *pBuffer,
		unsigned int unBufferLength,
		unsigned int *punBytesRead,
		unsigned int *punHeaderLength);

	inline void RequestorInfoSet (
		void *pRequestor,
		unsigned int unRequestNum,
		PstiHTTPCallback pfnCallback,
		void * pParam1);

	inline EstiBool RequestorInfoMatch (
		void *pRequestor,
		unsigned int unRequestNum);

	std::string ServerAddressGet() const
	{
		return m_connectedIP;
	}

	stiHResult ServerAddressSet (
		const char *pszHostName,
		const char *pszServerAddress,
		uint16_t un16ServerPort);

	static stiHResult ProxyAddressSet (
		const std::string &address,
		uint16_t port);

	static stiHResult ProxyAddressGet (
		std::string *address,
		uint16_t *port);

	EstiResult SocketClose ();

	inline stiSocket SocketGet ();

	stiHResult SocketOpen ();

	inline EstiHTTPState StateGet ();

	inline void StateSet (EstiHTTPState eNewState);

	EURLParseResult URLParse (
		IN const char * pszURL,
		IN const char * pszUrlAlt,
		OUT std::vector<std::string> *pServerIPList,
		OUT unsigned short *pnServerPort,
		OUT std::string *pFile);

	inline EstiBool WaitingCheck ();

	void SSLCtxSet (SSL_CTX * ctx) {m_ctx = ctx;}
	void SSLSessionReusedSet (SSL_SESSION * session) {m_session = session;}     
	SSL_SESSION * SSLSessionGet ();
	EstiBool IsSSLSessionReused ();
	EstiBool IsSSLUsed ();
	EstiResult SSLVerifyResultGet (); 
	EHTTPServiceResult HTTPProgressSSL ();  

private:
	
	stiSocket SocketConnect (
		const std::string &serverIP,
		uint16_t port);

	inline stiHResult StateChange (
		EstiHTTPState eState,
		SHTTPResult * pstResult);

	stiHResult HTTPTunnelEstablish ();

	EHTTPServiceResult HTTPProgress2 (
		SSL *pSsl,
		unsigned char *pBuffer,
		unsigned int unBufferLength,
		unsigned int *punBytesRead,
		unsigned int *punHeaderLength);

	SSL_CTX * m_ctx{nullptr};
	SSL_SESSION * m_session{nullptr};  
	SSL * m_ssl{nullptr};
	EstiBool m_bFirstSSLRead{estiTRUE};  

	static std::string	m_proxyAddress;
	static uint16_t	m_proxyPort;

	bool 				m_bTunnelEstablished{false};
	void 				*m_pRequestor{nullptr};
	unsigned int 		m_unRequestNum{0};
	PstiHTTPCallback	m_pfnCallback{nullptr};
	void * 				m_pParam1{nullptr};
	stiSocket			m_socket {stiINVALID_SOCKET};
	EstiHTTPState		m_eHTTPState{eHTTP_IDLE};		// State of Service
	std::vector<std::string> m_ResolvedServerIPList;
	std::string 		m_connectedIP;
	std::string			m_hostName;
	unsigned short 		m_nPort{0};
	std::string 		m_File;
	EstiBool			m_bActivity{estiFALSE};
	int					m_nInactivityCount{0};
	EstiBool			m_bRequestCancel{estiFALSE}; 	// Flag to indicate if user has cancelled request
	SHTTPResult *		m_pstResult{nullptr};
	std::vector<char>	m_resultBuffer;
	unsigned int		m_unResultDataLen{0};
	EstiBool			m_bHeaderParseDone{estiFALSE};
};

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::Cancel
//
// Description: Signals that the request has been cancelled.
//
// Abstract:
//
// Returns:
//	None
//
inline void CstiHTTPService::Cancel ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::Cancel);

	m_bRequestCancel = estiTRUE;
} // end CstiHTTPService::Cancel


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::Cancelled
//
// Description: Checks if the request has been cancelled.
//
// Abstract:
//
// Returns:
//	EstiBool - estiTRUE if cancelled, otherwise estiFALSE
//
inline EstiBool CstiHTTPService::Cancelled ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::Cancelled);

	return (m_bRequestCancel);
} // end CstiHTTPService::Cancelled


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::RequestorInfoSet
//
// Description: Sets the requestor's information
//
// Abstract:
//
// Returns:
//	None
//
void CstiHTTPService::RequestorInfoSet (
	void *pRequestor,
	unsigned int unRequestNum,
	PstiHTTPCallback pfnCallback,
	void * pParam1)
{
	stiLOG_ENTRY_NAME (CstiHTTPService::RequestorInfoSet);

	m_pRequestor = pRequestor;
	m_unRequestNum = unRequestNum;
	m_pfnCallback = pfnCallback;
	m_pParam1 = pParam1;
} // end CstiHTTPService::RequestorInfoSet


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::RequestorInfoMatch
//
// Description: Returns estiTRUE if the requestor's information matches
//
// Abstract:
//
// Returns:
//	EstiBool
//
EstiBool CstiHTTPService::RequestorInfoMatch (
	void *pRequestor,
	unsigned int unRequestNum)
{
	EstiBool bReturn = estiFALSE;

	stiLOG_ENTRY_NAME (CstiHTTPService::RequestorInfoMatch);

	// Does the information match?
	if (m_pRequestor == pRequestor &&
		m_unRequestNum == unRequestNum)
	{
		// Yes! return true.
		bReturn = estiTRUE;
	} // end if

	return (bReturn);
} // end CstiHTTPService::RequestorInfoMatch


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::SocketGet
//
// Description: Returns the socket file descriptor
//
// Abstract:
//
// Returns:
//	int - file descriptor of socket
//
stiSocket CstiHTTPService::SocketGet ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::SocketGet);

	return (m_socket);
} // end CstiHTTPService::SocketGet


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::StateGet
//
// Description: Returns the Service state
//
// Abstract:
//
// Returns:
//	EstiHTTPState - Service state enumeration
//
EstiHTTPState CstiHTTPService::StateGet ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::StateGet);

	return (m_eHTTPState);
} // end CstiHTTPService::StateGet


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::StateSet
//
// Description: Sets the Service state
//
// Abstract:
//
// Returns:
//	None
//
void CstiHTTPService::StateSet (
	IN EstiHTTPState eNewState)
{
	stiLOG_ENTRY_NAME (CstiHTTPService::StateSet);

	m_eHTTPState = eNewState;
} // end CstiHTTPService::StateSet


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::WaitingCheck
//
// Description: Checks if the socket is waiting for a response.
//
// Abstract:
//
// Returns:
//	EstiBool - estiBOOL if the socket is waiting, otherwise estiFALSE
//
inline EstiBool CstiHTTPService::WaitingCheck ()
{
	EstiBool bReturn = estiFALSE;
	stiLOG_ENTRY_NAME (CstiHTTPService::WaitingCheck);

	if (m_eHTTPState == eHTTP_WAITING_4_RESP)
	{
		bReturn = estiTRUE;
	} // end if

	return (bReturn);
} // end CstiHTTPService::WaitingCheck

//:
//: Private Inline functions
//:


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::StateChange
//
// Description: Changes the state of the service.
//
// Abstract:
//
// Returns:
//	EstiResult
//
inline stiHResult CstiHTTPService::StateChange (
	EstiHTTPState eState,	// The new state
	SHTTPResult * pstResult)// The result structure
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiHTTPService::StateChangeNotify);

	// Change the internal state
	m_eHTTPState = eState;

	// Do we have a callback method?
	if (m_pfnCallback != nullptr)
	{
		// Yes! Call it to notify of the state change.
		hResult = m_pfnCallback (m_unRequestNum, m_eHTTPState, pstResult, m_pParam1);
	} // end if

	return (hResult);
} // end CstiHTTPService::StateChangeNotify


#endif // CSTIHTTPSERVICE_H
// end file CstiHTTPService.h
