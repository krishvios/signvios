////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiHTTPService
//
//	File Name:	CstiHTTPService.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Service Provider for the HTTP component.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiOS.h"			// OS Abstraction
#include "stiOSNet.h"
#include "CstiHTTPService.h"	// Class Definition
#include "stiTrace.h"  // For debugging tools

#include "stiSSLTools.h"
#include <sstream>
#ifndef WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "CstiProfiler.h"
#include "CstiIpAddress.h"
#include "stiTools.h"


//
// Constants
//
//SLB#undef stiLOG_ENTRY_NAME
//SLB#define stiLOG_ENTRY_NAME(name) stiTrace ("-HTTPService: %s\n", #name);
static const int g_nSOCKET_TIMEOUT = 10;	///< Socket timeout value in seconds

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
std::string CstiHTTPService::m_proxyAddress;
uint16_t CstiHTTPService::m_proxyPort = 0;

//
// Function Declarations
//


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::~CstiHTTPService
//
// Description: Virtual Destructor
//
// Abstract:
//
// Returns:
//	None
//
CstiHTTPService::~CstiHTTPService ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::~CstiHTTPService);

	// Do we have a result structure allocated?
	if (nullptr != m_pstResult)
	{
		// Free the structure now.
		delete m_pstResult;
		m_pstResult = nullptr;
	} // end if

	if (m_socket != stiINVALID_SOCKET)
	{
		stiTrace ("CstiHTTPService::~CstiHTTPService: "
			"Error - Object destroyed with open socket!\n");

		SocketClose ();
	} // end if

	// Change to the terminated state.
	StateChange (eHTTP_TERMINATED, nullptr);

	m_ctx = nullptr;
	m_session = nullptr;
	if (nullptr != m_ssl)
	{
		stiSSLConnectionFree (m_ssl);
		m_ssl = nullptr;
	}
} // end CstiHTTPService::~CstiHTTPService


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::ActivityCheck
//
// Description: Returns whether there has been any activity on the socket
//
// Abstract:
//
// Returns:
//	EstiBool - estiTRUE if there has been activity, otherwise estiFALSE
//
EstiBool CstiHTTPService::ActivityCheck (
	int nMaxInactivity)
{
	EstiBool bReturn = estiFALSE;

	stiLOG_ENTRY_NAME (CstiHTTPService::ActivityCheck);

	if (eHTTP_DNS_RESOLVING == StateGet())
	{
		// There has been activity.
		return (estiTRUE);
	}

	// Has there been activity on this socket?
	if (m_bActivity)
	{
		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace (
				"ActivityCheck: Activity detected.\n");
		); // stiDEBUG_TOOL

		// Yes! Reset our count and flag
		m_nInactivityCount = 0;
		m_bActivity = estiFALSE;

		// There has been activity.
		bReturn = estiTRUE;
	} // end if
	else
	{
		stiDEBUG_TOOL (g_stiHTTPTaskDebug,
			stiTrace (
				"ActivityCheck: No activity detected.\n");
		); // stiDEBUG_TOOL

		// No! There has not been any activity. Increment the counter.
		m_nInactivityCount++;

		// Have we exceeded the count?
		if (m_nInactivityCount < nMaxInactivity)
		{
			// No! We can keep signalling activity.
			bReturn = estiTRUE;
		} // end if
	} // end else

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Change the state to cancelled
		StateChange (eHTTP_CANCELLED, nullptr);

		// Signal inactivity no matter what.
		bReturn = estiFALSE;
	} // end if

	return (bReturn);
}// end CstiHTTPService::ActivityCheck

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::Abandon
//
// Description: Signals to the listener that this socket is abandoned
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiError.
//
stiHResult CstiHTTPService::Abandon ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::Abandon);

	// Yes! Change the state to abandoned
	stiHResult hResult = StateChange (eHTTP_ABANDONED, nullptr);

	return (hResult);
}// end CstiHTTPService::Abandon


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::HTTPGetBegin
//
// Description: Begins the HTTP process
//
// Abstract:
//	This function causes the HTTP process to begin by writing the
//	appropriate message to the specified socket.
//
//	If this function returns error, the socket will be closed.
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiError.
//
stiHResult CstiHTTPService::HTTPGetBegin (
	const char *pszFile,	// File
	int nStartByte,			// For Range request: first byte to retrieve (-1 for no range request)
	int nEndByte)			// For Range request: last byte to retrieve (-1 for no range request)
{
	stiLOG_ENTRY_NAME (CstiHTTPService::HTTPGetBegin);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);

		stiTHROW (stiRESULT_ERROR);
	} // end if

	// Were we given a valid file pointer?
	if (pszFile != nullptr && !m_hostName.empty ())
	{
		// Yes! Is there a filename?
		if (0 != *pszFile)
		{
			std::stringstream Request;

			if (!m_proxyAddress.empty () && !m_bTunnelEstablished)
			{
				Request << "GET http://" << m_hostName << pszFile << " HTTP/1.1\r\n";
			}
			else
			{
				Request << "GET " << pszFile << " HTTP/1.1\r\n";
			}

			if (!Request.str().empty ())
			{
				// If start/end byte are specified, add the range request
				if (nStartByte > -1 && nEndByte > -1)
				{
					// Add range request
					Request << "Range: bytes=";
					Request << nStartByte;
					Request << "-";
					Request << nEndByte;
					Request << "\r\n";
				}
				Request << "Host: " << m_hostName << "\r\n";
				Request << "\r\n";

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("HTTP String: ========================\n"
					"%s\n"
					"End HTTP String: ====================\n", Request.str().c_str ());
				); // stiDEBUG_TOOL

				int nResult;
				if (nullptr != m_ctx)
				{
					// Send the auto detect request.
					nResult = stiSSLWrite (
						m_ssl,
						Request.str ().c_str (),
						Request.str ().length ());
				}
				else
				{
					// Send the auto detect request.
					nResult = stiOSWrite (
						m_socket,
						Request.str ().c_str (),
						Request.str ().length ());
				}

				stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

				// Yes! There has now been activity on this socket.
				m_bActivity = estiTRUE;
				m_unResultDataLen = 0;

				// The socket has been opened connected to
				// and a request has been send. Bail out to wait
				// for a reply.
				StateChange (eHTTP_WAITING_4_RESP, nullptr);
			} // end if
		} // end if
	} // end if

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);

		stiTHROW (stiRESULT_ERROR);
	} // end if

STI_BAIL:

	// Was there an error?
	if (stiIS_ERROR (hResult))
	{
		// Yes! Signal error
		StateChange (eHTTP_ERROR, nullptr);
	} // end if

	return hResult;
} // end CstiHTTPService::HTTPGetBegin


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::HTTPFormPostBegin
//
// Description: Begins the HTTP process
//
// Abstract:
//	This function causes the HTTP process to begin by writing the
//	appropriate message to the specified socket.
//
//	If this function returns error, the socket will be closed.
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiError.
//
EstiResult CstiHTTPService::HTTPFormPostBegin (
	IN const char * szFile,		// File
	IN const SHTTPVariable * astVariables)	// Parameters
{
	stiLOG_ENTRY_NAME (CstiHTTPService::HTTPFormPostBegin);

	EstiResult eResult = estiERROR;

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, NULL);
		return (estiERROR);
	} // end if

	// Were we given a valid file pointer?
	if (szFile != NULL && !m_hostName.empty ())
	{
		// Yes! Is there a filename?
		if (0 != *szFile)
		{
			// Yes! Build the request...
			// Count the number of variables that were sent in.
			int nVarCount = 0;
			size_t VarSize = 0;
			while (astVariables[nVarCount].szName[0] != 0)
			{
				VarSize += strlen (astVariables[nVarCount].szName);
				VarSize += strlen (astVariables[nVarCount].szValue);
				nVarCount++;
			} // end while

			char szHeaderTemplate[] =
				"POST /%s HTTP/1.1\r\n"
				"Connection: close\r\n"
				"Content-Type: multipart/form-data; boundary=---------------------------7d4551220162\r\n"
				"Host: %s\r\n"
				"Content-Length: %d\r\n"
				"\r\n";
			char szItemTemplate[] =
				"-----------------------------7d4551220162\r\n"
				"Content-Disposition: form-data; name=\"%s\"\r\n"
				"\r\n"
				"%s\r\n";
			char szEndItemTemplate[] =
				"-----------------------------7d4551220162--\r\n";

			size_t BodySize =
				nVarCount * (strlen (szItemTemplate) - 4) +
				VarSize +
				strlen (szEndItemTemplate);

			size_t RequestSize =
				strlen (szHeaderTemplate) +
				strlen (szFile) +
				m_hostName.size () +
				BodySize +
				10; // fudge factor to account for variablility in the Content-Length

			char * szRequest = (char *)stiHEAP_ALLOC (RequestSize);

			if (NULL != szRequest)
			{
				// Build the header
				sprintf (
					szRequest,
					szHeaderTemplate,
					szFile,
					m_hostName.c_str (),
					BodySize);
				char * szEnd = szRequest + strlen (szRequest);

				// Add the variables
				for (int x = 0; x < nVarCount; x++)
				{
					sprintf (
						szEnd,
						szItemTemplate,
						astVariables[x].szName,
						astVariables[x].szValue);

					szEnd = szRequest + strlen (szRequest);
				} // end for

				// Add the termination
				strcat (szRequest, szEndItemTemplate);

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("HTTP String: ========================\n"
					"%s\n"
					"End HTTP String: ====================\n", szRequest);
				); // stiDEBUG_TOOL

				int nResult;
				if (NULL != m_ctx)
				{
					// Send the request.
					nResult = stiSSLWrite (
						m_ssl,
						szRequest,
						strlen (szRequest));
				}
				else
				{
					// Send the request.
					nResult = stiOSWrite (
						m_socket,
						szRequest,
						strlen (szRequest));
				}

				// Was the write successful?
				if (nResult >= 0)
				{
					// Yes! The socket has been opened, connected,
					// and a request has been send. Bail out to wait
					// for a reply.
					StateChange (eHTTP_WAITING_4_RESP, NULL);

					// There has now been activity on this socket.
					m_bActivity = estiTRUE;

					eResult = estiOK;
				} // end else

				// Free the request we allocated
				stiHEAP_FREE (szRequest);
			} // end if
		} // end if
	} // end if

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, NULL);
		return (estiERROR);
	} // end if

	// Was there an error?
	if (eResult != estiOK)
	{
		// Yes! Signal error
		StateChange (eHTTP_ERROR, NULL);
	} // end if

	return (eResult);
} // end CstiHTTPService::HTTPFormPostBegin
#endif


stiHResult CstiHTTPService::HTTPTunnelEstablish ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult; stiUNUSED_ARG (nResult);
	std::stringstream Request;

	stiASSERT (!m_hostName.empty ());

	Request << "CONNECT " << m_hostName << ":" << STANDARD_HTTPS_PORT << " HTTP/1.1\r\n";
	Request << "Proxy-Connection: keep-alive\r\n";
	Request << "Host: " << m_hostName << "\r\n";
	Request << "\r\n";

	// Send the request.
	nResult = stiOSWrite (
		m_socket,
		Request.str().c_str (),
		Request.str().length());

	PstiHTTPCallback Callback = m_pfnCallback;
	m_pfnCallback = nullptr;

	HTTPProgress2 (nullptr, nullptr, 0, nullptr, nullptr);

	//!\todo: check HTTP result code

	m_pfnCallback = Callback;

	m_bTunnelEstablished = true;

	delete m_pstResult;
	m_pstResult = nullptr;

	if (!m_resultBuffer.empty ())
	{
		m_resultBuffer.clear ();
		m_unResultDataLen = 0;
	}

	m_bHeaderParseDone = estiFALSE;
	m_bActivity = estiFALSE;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::HTTPPostBegin
//
// Description: Begins the HTTP process
//
// Abstract:
//	This function causes the HTTP process to begin by writing the
//	appropriate message to the specified socket.
//
//	If this function returns error, the socket will be closed.
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiError.
//
EstiResult CstiHTTPService::HTTPPostBegin (
	const std::string &file,		// File
	const std::string &bodyData)		// Body Data
{
	stiLOG_ENTRY_NAME (CstiHTTPService::HTTPPostBegin);

	EstiResult eResult = estiERROR;

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
		return (estiERROR);
	} // end if

	// Were we given a valid file pointer?
	if (!file.empty () && !m_hostName.empty ())
	{
		std::stringstream Request;

		if (!m_proxyAddress.empty () && !m_bTunnelEstablished)
		{
			Request << "POST http://" << m_hostName << "/" << file << " HTTP/1.1\r\n";
		}
		else
		{
			Request << "POST /" << file << " HTTP/1.1\r\n";
			Request << "Connection: close\r\n";
		}

		if (!Request.str ().empty ())
		{
			Request << "Content-Type: application/xml; charset=iso-8859-1\r\n";
			Request << "Host: " << m_hostName << "\r\n";
			Request << "Content-Length: " << bodyData.length() << "\r\n";
			Request << "\r\n";

			// Add the body
			Request << bodyData;

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("HTTP String: ========================\n"
				"%s\n"
				"End HTTP String: ====================\n",
				Request.str ().c_str ());
			); // stiDEBUG_TOOL

			int nResult;

			if (nullptr != m_ctx)
			{
				// Send the request.
				nResult = stiSSLWrite (
					m_ssl,
					Request.str ().c_str (),
					Request.str ().length ());
			}
			else
			{
				// Send the request.
				nResult = stiOSWrite (
					m_socket,
					Request.str ().c_str (),
					Request.str ().length ());
			}

			// Was the write successful?
			if (nResult >= 0)
			{
				// Yes! The socket has been opened, connected,
				// and a request has been send. Bail out to wait
				// for a reply.
				StateChange (eHTTP_WAITING_4_RESP, nullptr);

				// There has now been activity on this socket.
				m_bActivity = estiTRUE;

				eResult = estiOK;
			} // end else
		} // end if
	} // end if

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
		return (estiERROR);
	} // end if

	// Was there an error?
	if (eResult != estiOK)
	{
		// Yes! Signal error
		StateChange (eHTTP_ERROR, nullptr);
	} // end if

	return (eResult);
} // end CstiHTTPService::HTTPPostBegin


EHTTPServiceResult CstiHTTPService::HTTPProgress2 (
	SSL *pSsl,
	unsigned char *pBuffer,
	unsigned int unBufferLength,
	unsigned int *punBytesRead,
	unsigned int *punHeaderLength)
{
	// Initialize the return values
	EHTTPServiceResult eResult = eHTTP_TOOLS_ERROR;

	char *pszReadPosition = nullptr;
	char *pszResultBuffer = nullptr;

	// There has now been activity on this socket.
	m_bActivity = estiTRUE;

	if (punHeaderLength)
	{
		*punHeaderLength = 0;
	}

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
		return (eHTTP_TOOLS_CANCELLED);
	} // end if

	if (!pBuffer)
	{
		//
		// The caller has *not* passed in an external buffer to use to read the
		// response into.  We need to make sure our internal buffer has been allocated.
		//
		// Have we already allocated a buffer for the result?
		if (m_resultBuffer.size () - m_unResultDataLen < RTOS_TCP_RCV_SIZE_DFLT + 1)
		{
			m_resultBuffer.resize (m_resultBuffer.size () + RTOS_TCP_RCV_SIZE_DFLT + 1);
		}

		pszReadPosition = &m_resultBuffer.data ()[m_unResultDataLen];
		pszResultBuffer = m_resultBuffer.data ();
		unBufferLength = RTOS_TCP_RCV_SIZE_DFLT;
	}
	else
	{
		//
		// The caller has passed in an external buffer to use to read the
		// response into.
		//
		pszReadPosition = (char *)pBuffer;
		pszResultBuffer = (char *)pBuffer;
	}

	// Have we allocated a result structure?
	if (nullptr == m_pstResult)
	{
		// No! Allocate it now.
		m_pstResult = new SHTTPResult;
		if (nullptr != m_pstResult)
		{
			// Set the requestor information.
			m_pstResult->pRequestor = m_pRequestor;
			m_pstResult->unRequestNum = m_unRequestNum;
		} // end if
	} // end if

	// Was the memory allocation successful
	if (nullptr != pszReadPosition && nullptr != m_pstResult)
	{
		// Yes! Default to continue reading.
		eResult = eHTTP_TOOLS_CONTINUE;

		// Yes. Receive the reply from the socket
		int nRecLen = 0;

		//
		// Read from the socket.
		// Note: we will read one less than the buffer length
		// so that we will have room for the NULL terminator.
		//
		if (pSsl)
		{
			nRecLen = stiSSLRead (pSsl, pszReadPosition, unBufferLength - 1);
		}
		else
		{
			nRecLen = stiOSRead (m_socket, pszReadPosition, unBufferLength - 1);
		}

		// Was there anything to read to complete a record?
		if (nRecLen <= 0)
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("Read returned %d\n", nRecLen);
			); // stiDEBUG_TOOL

			// No! We are done.
			if (pSsl)
			{
				if (nRecLen < 0)
				{
					eResult = eHTTP_TOOLS_COMPLETE;
				}
				else
				{
					eResult = eHTTP_TOOLS_CONTINUE;
				}
			}
			else
			{
				if (nRecLen < 0)
				{
					eResult = eHTTP_TOOLS_ERROR;
				}
				else
				{
					eResult = eHTTP_TOOLS_COMPLETE;
				}
			}
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				pszReadPosition[nRecLen] = 0;
				if (g_stiHTTPTaskDebug >= 3)
				{
					stiTrace ("HTTP Read: %d bytes read:\n", nRecLen);
					stiTrace ("---------------\n%s\n----------------\n", pszReadPosition);
				} // end if
				else
				{
					stiTrace ("HTTP Read: %d bytes read. Set g_stiHTTPTaskDebug = 3 for display\n", nRecLen);
				} // end else
			); // stiDEBUG_TOOL

			// Can we keep going?
			if (eHTTP_TOOLS_CONTINUE == eResult)
			{
				// NULL terminate the reply buffer
				pszReadPosition[nRecLen] = 0;

				// If we haven't yet finished the header...
				if (!m_bHeaderParseDone)
				{
					EstiBool bParseAgain = estiFALSE;

					do
					{
						// Do this only once (unless changed below).
						bParseAgain = estiFALSE;

						// Parse for a result
						EHTTPServiceResult eHeaderResult = HeaderParse (
							pszResultBuffer, m_pstResult);

						// Do we need to read more?
						if (eHeaderResult == eHTTP_TOOLS_CONTINUE)
						{
							stiDEBUG_TOOL (g_stiHTTPTaskDebug,
								stiTrace ("HTTP Read: HeaderParse returned Continue\n");
							); // stiDEBUG_TOOL

							// Yes! Keep going.
							eResult = eHTTP_TOOLS_CONTINUE;
						} // end if

						// Was there an error?
						else if (eHeaderResult == eHTTP_TOOLS_ERROR)
						{
							stiDEBUG_TOOL (g_stiHTTPTaskDebug,
								stiTrace ("HTTP Read: HeaderParse returned ERROR!\n");
							); // stiDEBUG_TOOL

							// Yes! Bail out.
							eResult = eHTTP_TOOLS_ERROR;
						} // end else if

						// We read the header
						else
						{
							// Did we receive a 'continue' response?
							if (m_pstResult->unReturnCode >= 100 &&
								m_pstResult->unReturnCode < 200)
							{
								stiDEBUG_TOOL (g_stiHTTPTaskDebug,
									stiTrace ("HTTP Read: Ignoring Continue (100) response.\n");
								); // stiDEBUG_TOOL

								// Yes! Throw away this response
								memmove (
									pszResultBuffer,
									pszResultBuffer + m_pstResult->unHeaderLen,
									nRecLen - m_pstResult->unHeaderLen +
									1); // for the null terminator

								// Recalculate the buffer length.
								nRecLen -= m_pstResult->unHeaderLen;

								// Clear the length and return code
								m_pstResult->unHeaderLen = 0;
								m_pstResult->unReturnCode = 0;

								// Reparse the header, now that the continue is
								// gone.
								bParseAgain = estiTRUE;
							} // end if
							else
							{
								// No! We are done parsing the header. It won't
								// change from here on out.
								m_bHeaderParseDone = estiTRUE;

								if (punHeaderLength)
								{
									*punHeaderLength = m_pstResult->unHeaderLen;
								}
							} // end else
						} // end else if
					} while (bParseAgain);
				} // end if

				// Have we read all of the body data?
				if (eResult == eHTTP_TOOLS_CONTINUE &&
					m_pstResult->unContentLen != 0 &&
					(m_unResultDataLen + nRecLen >=
						m_pstResult->unContentLen + m_pstResult->unHeaderLen))
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("HTTP Read: All data read.\n");
					); // stiDEBUG_TOOL

					// Yes! Signal that we are done.
					eResult = eHTTP_TOOLS_COMPLETE;
				} // end if
			} // end if
		} // end else

		if (punBytesRead)
		{
			*punBytesRead = nRecLen;

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("Bytes Read = %d\n", *punBytesRead);
			); // stiDEBUG_TOOL
		}

		// Did we successfully drain the socket data?
		if (eHTTP_TOOLS_COMPLETE == eResult)
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				if (g_stiHTTPTaskDebug >= 2)
				{
					if (m_unResultDataLen + nRecLen <= 10000)
					{
						stiTrace ("HTTP Reply: **************************\n"
						"%s\n"
						"End HTTP Reply **********************\n", pszResultBuffer);
					} // end if
					else
					{
						FILE* pFile;
						pFile = fopen ("/tgtsvr/HTTPOut.txt", "wb+");
						if (pFile)
						{
							fwrite (pszResultBuffer,
									sizeof (char),
									m_unResultDataLen + nRecLen,
									pFile);
							fclose (pFile);
						} // end if
						stiTrace ("HTTP Reply too large for display. Wrote %d bytes to /tgtsvr/HTTPOut.txt.\n", m_unResultDataLen + nRecLen);
					} // end else
				} // end if
				else
				{
					stiTrace ("HTTP Read: Done! %d bytes read. Set g_stiHTTPTaskDebug = 2 for display\n", m_unResultDataLen + nRecLen);
				} // end else
			); // stiDEBUG_TOOL

			// Yes! Has a cancel been requested?
			if (m_bRequestCancel)
			{
				// Yes! Cancel out.
				StateChange (eHTTP_CANCELLED, nullptr);
				return (eHTTP_TOOLS_CANCELLED);
			} // end if

			if (!m_resultBuffer.empty ())
			{
				// Calculate a pointer to the result body and store it.
				m_pstResult->body = nonstd::make_observer(m_resultBuffer.data () + m_pstResult->unHeaderLen);

				// Was a content length not given when there is really content?
				if (0 == m_pstResult->unContentLen)
				{
					size_t CalcLen = strlen (m_pstResult->body.get ());

					if (CalcLen > 0)
					{
						// Yes! Store a calculated length if necessary
						m_pstResult->unContentLen = CalcLen;
					} // end if
				}

				// Save the pointer to the response buffer into the result
				// structure
				m_pstResult->resultBuffer = std::move(m_resultBuffer);
			}

			// Now that the buffer has been placed into the response buffer,
			// it no longer belongs to this object directly.
			m_resultBuffer.resize (0);
			m_unResultDataLen = 0;
			m_bHeaderParseDone = estiFALSE;

			// Has the user requested notification of the result?
			if (m_pfnCallback != nullptr)
			{
				// Yes! Make a copy of our result structure pointer
				SHTTPResult * pstResult = m_pstResult;

				// Inform the user about the result
				stiHResult hResult = StateChange (eHTTP_COMPLETE, pstResult);
				
				if (stiIS_ERROR (hResult))
				{
					eResult = eHTTP_TOOLS_ERROR;

					delete m_pstResult;
					m_pstResult = nullptr;
				} // end if
				else
				{
					// The result no longer belongs to this object.
					m_pstResult = nullptr;
				}
			} // end if
			else
			{
				// No! Just change the state to complete.
				StateChange (eHTTP_COMPLETE, nullptr);

			} // end else
		} // end if
		else
		{
			m_unResultDataLen += nRecLen;
		}
	} // end if

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
		return (eHTTP_TOOLS_CANCELLED);
	} // end if

	// Was there an error?
	if (eHTTP_TOOLS_ERROR == eResult)
	{
		// Yes! Signal error.
		StateChange (eHTTP_ERROR, nullptr);
	} // end if

	return (eResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::HTTPProgress
//
// Description: Indicates information is ready to to be read from the socket
//
// Abstract:
//	This function continues the HTTP process by reading the available
//	data from the socket.
//
// Returns:
//	EHTTPServiceResult:
//		eHTTP_TOOLS_ERROR - An Error has occurred. Close the socket and fail.
//		eHTTP_TOOLS_CONTINUE - The result is not complete, keep the socket open.
//		eHTTP_TOOLS_CANCELLED - The action has been cancelled. Close the socket.
//		eHTTP_TOOLS_COMPLETE - The result is complete. Close the socket.
//
EHTTPServiceResult CstiHTTPService::HTTPProgress (
	unsigned char *pBuffer,
	unsigned int unBufferLength,
	unsigned int *punBytesRead,
	unsigned int *punHeaderLength)
{
	stiLOG_ENTRY_NAME (CstiHTTPService::HTTPProgress);

	return HTTPProgress2 (m_ssl, pBuffer, unBufferLength, punBytesRead, punHeaderLength);

} // end CstiHTTPService::HTTPProgress


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::SSLSessionGet
//
// Description: Get a sesson from the current established connection for
// session resumption purpose.
//
// Abstract:
//
// Returns:
//  SSL_SESSION * a copy of the session of the current connection
SSL_SESSION * CstiHTTPService::SSLSessionGet ()
{
	return (stiSSLSessionGet (m_ssl));
}


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::IsSSLSessionReused
//
// Description: Query, whether a reused session was negotiated during the handshake.
//
// Abstract:
//
// Returns:
//  EstiBool - estiTRUE, yes a session was reused. Othersize, a new session was negotiated.
EstiBool CstiHTTPService::IsSSLSessionReused ()
{
	return ((EstiBool) stiSSLSessionReused (m_ssl));
}

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::IsSSLUsed
//
// Description: Query, whether a SSL connection has been used for this http service.
//
// Abstract:
//
// Returns:
//  estiTRUE - Yes, othersize No.
EstiBool CstiHTTPService::IsSSLUsed ()
{
	EstiBool bSSLUsed = estiFALSE;
	if (nullptr != m_ctx)
	{
		bSSLUsed = estiTRUE;
	}

	return bSSLUsed;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::SSLVerifyResultGet
//
// Description: Query, the result of the verification of the X509 certificate
//
// Abstract:
//
// Returns:
//  EstiResult
//		estiOK, OK for the verification of the X509 certificate,
//		estiERROR, error happen during the verification of the X509 certificate.
EstiResult CstiHTTPService::SSLVerifyResultGet ()
{
	EstiResult eResult = estiOK;

	if (0 == stiSSLVerifyResultGet (m_ssl))
	{
		eResult = estiERROR;
	}

	return eResult;
}


/*!\brief Connects the socket for HTTP
 *
 */
stiSocket CstiHTTPService::SocketConnect (
	const std::string &serverIP,
	uint16_t port)
{
	stiSocket socket = stiINVALID_SOCKET;
	const int connectRetry = 3;

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("SocketConnect - IP=%s, Port=%d\n", serverIP.c_str (), port);
	); // stiDEBUG_TOOL

	CstiIpAddress ipAddress = serverIP;

	for (int i = 0; i < connectRetry; ++i)
	{
		if (ipAddress.IpAddressTypeGet () == estiTYPE_IPV6)
		{
			socket = stiOSSocket (AF_INET6, SOCK_STREAM, 0);
		}
		else
		{
			// Open a socket.
			socket = stiOSSocket (AF_INET, SOCK_STREAM, 0);
		}

		// Was the socket open successful?
		if (socket != stiINVALID_SOCKET)
		{
			bool sslAvailable = (nullptr != m_ctx);
			if (sslAvailable)
			{
				stiDEBUG_TOOL (g_stiSSLDebug,
					stiTrace("SocketOpen - using SSL connection\n");
					stiTrace ("\tm_ctx = %p m_session = %p\n", m_ctx, m_session);
				);

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("SocketOpen: open success, socket=%d\n", socket);

				); // stiDEBUG_TOOL

				auto hResult = HttpSocketConnect (socket, serverIP.c_str (), port, g_nSOCKET_TIMEOUT);

				if (!stiIS_ERROR(hResult))
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("SocketOpen: Connect success!\n");
					); // stiDEBUG_TOOL

					//
					// If we are using a proxy and SSL then we need to send a "CONNECT" message
					// first to create an HTTP Tunnel.
					//
					if (!m_proxyAddress.empty () && !m_bTunnelEstablished)
					{
						HTTPTunnelEstablish ();
					}

					// Now we have TCP connection. Start SSL negotiation.
					m_ssl = stiSSLConnectionStart (m_ctx, socket, m_session);
					if (nullptr != m_ssl)
					{
						stiDEBUG_TOOL (g_stiSSLDebug,
							stiTrace ("SocketOpen: stiSSLConnectionStart success\n");
						); // stiDEBUG_TOOL

						break;
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiSSLDebug,
							stiTrace ("SocketOpen: stiSSLConnectionStart Failed\n");
						); // stiDEBUG_TOOL

						// No! Close the socket.
						stiOSClose (socket);
						socket = stiINVALID_SOCKET;
					} // end else
				} // end if
				else
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("SocketOpen: Connect Failed\n");
					); // stiDEBUG_TOOL

					// No! Close the socket.
					stiOSClose (socket);
					socket = stiINVALID_SOCKET;
				} // end else
			} // end if
			else
			{
				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("SocketOpen: non-SSL open success, socket=%d\n", socket);
				); // stiDEBUG_TOOL

				auto hResult = HttpSocketConnect (socket, serverIP.c_str (), port, g_nSOCKET_TIMEOUT);

				if (!stiIS_ERROR(hResult))
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("SocketOpen: Connect success!\n");
					); // stiDEBUG_TOOL

					break;
				} // end if
				else
				{
					stiDEBUG_TOOL (g_stiHTTPTaskDebug,
						stiTrace ("SocketOpen: Connect Failed\n");
					); // stiDEBUG_TOOL

					// No! Close the socket.
					stiOSClose (socket);
					socket = stiINVALID_SOCKET;
				} // end else
			}// end else
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("SocketOpen: open failed, socket=%d, addr=%s:%d\n", socket, serverIP.c_str (), port);
			); // stiDEBUG_TOOL

			socket = stiINVALID_SOCKET;
		} // end else
	}

	return socket;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::SocketOpen
//
// Description: Opens the socket for HTTP
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise EstiError.
//
stiHResult CstiHTTPService::SocketOpen ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::SocketOpen);
	stiHResult hResult = stiRESULT_SUCCESS;
	uint16_t un16Port = 0;
	std::stringstream attemptedAddresses;
	auto recordFailedAddress = [&](const std::string serverIP, uint16_t port)
	{
		if (!attemptedAddresses.str ().empty ())
		{
			attemptedAddresses << ", ";
		}

		attemptedAddresses << serverIP << ":" << port;
	};

	if (m_socket != stiINVALID_SOCKET)
	{
		stiTrace ("****** socket already open?\n");
	}

	if (m_proxyAddress.empty ())
	{
		// Try each ip address in the list in turn
		for (auto serverIP : m_ResolvedServerIPList)
		{
			if (m_nPort != 0)
			{
				un16Port = m_nPort;
			}
			else if (nullptr != m_ctx)
			{
				un16Port = STANDARD_HTTPS_PORT;
			}
			else
			{
				un16Port = STANDARD_HTTP_PORT;
			}

			// Has a cancel been requested?
			if (m_bRequestCancel)
			{
				// Yes! Cancel out.
				StateChange (eHTTP_CANCELLED, nullptr);

				stiTHROW (stiRESULT_ERROR);
			} // end if

			m_socket = SocketConnect (serverIP, un16Port);

			// Record the address that we failed to connect to.
			if (m_socket == stiINVALID_SOCKET)
			{
				recordFailedAddress (serverIP, un16Port);
			}
			else
			{
				m_connectedIP = serverIP;

				// We have a socket so break out of the loop.
				break;
			}
		}
	}
	else
	{
		stiTESTCOND (m_proxyPort != 0, stiRESULT_ERROR);

		m_socket = SocketConnect (m_proxyAddress, m_proxyPort);

		if (m_socket == stiINVALID_SOCKET)
		{
			recordFailedAddress (m_proxyAddress, m_proxyPort);
		}
	}

	// Log which servers we failed to connect with.
	if (m_socket == stiINVALID_SOCKET)
	{
		stiTESTCONDMSG (false, stiRESULT_ERROR, "Host=%s, Addresses=\"%s\"", m_hostName.c_str (), attemptedAddresses.str ().c_str ());
	}
	else if (!attemptedAddresses.str ().empty ())
	{
		stiASSERTMSG (false, "Host=%s, Addresses=\"%s\"", m_hostName.c_str (), attemptedAddresses.str ().c_str ());
	}

	// Our state is now Sending
	StateChange (eHTTP_SENDING, nullptr);

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
		stiTHROW (stiRESULT_ERROR);
	} // end if

STI_BAIL:

	// Was there an error?
	if (stiIS_ERROR (hResult))
	{
		// Yes! Signal an error.
		StateChange (eHTTP_ERROR, nullptr);
	} // end if

	return hResult;
} // end CstiHTTPService::SocketOpen


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::SocketClose
//
// Description: Closes the socket for HTTP
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise EstiError.
//
EstiResult CstiHTTPService::SocketClose ()
{
	stiLOG_ENTRY_NAME (CstiHTTPService::SocketClose);

	stiDEBUG_TOOL (g_stiHTTPTaskDebug,
		stiTrace ("CstiHTTPService::SocketClose\n");
	); // stiDEBUG_TOOL

	if (nullptr != m_ssl)
	{
		stiSSLConnectionShutdown (m_ssl);
		stiSSLConnectionFree (m_ssl);
		m_ssl = nullptr;
	}

	// Do we have an open socket?
	if (m_socket != stiINVALID_SOCKET)
	{
		// Properly shutdown and flush the socket
		stiOSSocketShutdown (m_socket);

		// Close it now.
		stiOSClose (m_socket);

		m_socket = stiINVALID_SOCKET;
	} // end if

	// Has a cancel been requested, but not acted upon?
	if (m_bRequestCancel && m_eHTTPState != eHTTP_CANCELLED)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
	} // end if

	return (estiOK);
} // end CstiHTTPService::SocketClose


////////////////////////////////////////////////////////////////////////////////
//; CstiHTTPService::URLParse
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
EURLParseResult CstiHTTPService::URLParse (
	IN const char * pszURL,
	IN const char * pszUrlAlt,
	OUT std::vector<std::string> *pServerIPList,
	OUT unsigned short *pnServerPort,
	OUT std::string *pFile)
{
	stiLOG_ENTRY_NAME (CstiHTTPService::URLParse);

	EURLParseResult	eResult = eDNS_FAIL;

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		StateChange (eHTTP_CANCELLED, nullptr);
		return (eDNS_CANCELLED);
	} // end if

	// Was a good url sent?
	if (nullptr != pszURL)
	{
		// Yes!	Continue to parse the url.
		StateChange (eHTTP_DNS_RESOLVING, nullptr);
		eResult = stiURLParse (
			pszURL,
			&m_ResolvedServerIPList,
			&m_nPort,
			&m_File,
			&m_hostName,
			nullptr,
			pszUrlAlt);
		if (eURL_OK == eResult)
		{
			*pServerIPList = m_ResolvedServerIPList;
			*pnServerPort = m_nPort;
			*pFile = m_File;
		} // end if
	} // end if

	// Has a cancel been requested?
	if (m_bRequestCancel)
	{
		// Yes! Cancel out.
		if (eDNS_FAIL == eResult)
		{
			StateChange (eHTTP_DNS_ERROR, nullptr);
		}

		StateChange (eHTTP_CANCELLED, nullptr);
		return (eDNS_CANCELLED);
	} // end if

	// Was there an error?
	if (eURL_OK != eResult)
	{
		// Yes signal the error condition.
		StateChange (eHTTP_DNS_ERROR, nullptr);

		// NOTE: eHTTP_DNS_ERROR invokes the same error handling as eHTTP_ERROR, so no need to do both
		//StateChange (eHTTP_ERROR, NULL);
	} // end if

	return (eResult);
} // end CstiHTTPService::URLParse


stiHResult CstiHTTPService::ServerAddressSet (
	const char *pszHostName,
	const char *pszServerAddress,
	uint16_t un16ServerPort)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_connectedIP.clear ();
	m_ResolvedServerIPList.clear ();
	m_ResolvedServerIPList.push_back (pszServerAddress);
	m_hostName = pszHostName;

//	m_szPort = un16ServerPort;

	return hResult;
}


stiHResult CstiHTTPService::ProxyAddressSet (
	const std::string &address,
	uint16_t port)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_proxyAddress = address;
	m_proxyPort = port;

	return hResult;
}


stiHResult CstiHTTPService::ProxyAddressGet (
	std::string *address,
	uint16_t *port)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*address = m_proxyAddress;
	*port = m_proxyPort;

	return hResult;
}


int CstiHTTPService::ContentLengthGet ()
{
	return (m_pstResult->unContentLen);
}


int CstiHTTPService::ContentRangeStartGet ()
{
	return m_pstResult->unContentRangeStart;
}


int CstiHTTPService::ContentRangeEndGet ()
{
	return m_pstResult->unContentRangeEnd;
}


int CstiHTTPService::ContentRangeTotalGet ()
{
	return m_pstResult->unContentRangeTotal;
}


int CstiHTTPService::ReturnCodeGet ()
{
	return (m_pstResult->unReturnCode);
}


//:-----------------------------------------------------------------------------
//:	Utility methods
//:-----------------------------------------------------------------------------

// end file CstiHTTPService.cpp
