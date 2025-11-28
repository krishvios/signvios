////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiHTTPTask
//
//  File Name:	CstiHTTPTask.h
//
//  Owner:		Scot L. Brooksby
//
//	Abstract:
//		See CstiHTTPTask.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIHTTPTASK_H
#define CSTIHTTPTASK_H

//
// Includes
//
#include "CstiURLResolve.h"

#include "CstiEvent.h"
#include "CstiOsTaskMQ.h"
#include "stiSVX.h"
#include "stiHTTPServiceTools.h"
#include "stiEventMap.h"
#include "CstiHTTPService.h"

#include "stiSSLTools.h"

#include <map>
#include <mutex>

//
// Forward References
//
class CstiHTTPTask;

//
// Globals
//
//extern CstiHTTPTask g_oHTTPTask;

//
// Typedefs
//

//
// Class Declaration
//
class CstiHTTPTask : public CstiOsTaskMQ
{
public:
	// Event identifiers
	// These enumerations need to start at estiEVENT_NEXT or greater to not collide
	// with enumerations defined by the base classes.
	enum EEventType
	{
		estiHTTP_TASK_FORM_POST = estiEVENT_NEXT,	// Begin HTTP Form Post Transaction
		estiHTTP_TASK_POST,				// Begin HTTP Post Transaction
		estiHTTP_TASK_GET,				// Begin HTTP Get Transaction
		estiHTTP_TASK_CANCEL,			// Cancel a pending task
		estiHTTP_TASK_URL_RESOLVE_SUCCESS,
		estiHTTP_TASK_URL_RESOLVE_FAIL,
	};

	CstiHTTPTask (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam);

	stiHResult Initialize ();
	
	~CstiHTTPTask () override;

	CstiHTTPTask (const CstiHTTPTask &other) = delete;
	CstiHTTPTask (CstiHTTPTask &&other) = delete;
	CstiHTTPTask &operator= (const CstiHTTPTask &other) = delete;
	CstiHTTPTask &operator= (CstiHTTPTask &&other) = delete;

	//
	// Core HTTP Methods
	//
	void HTTPCancel (
		void *pRequestor,
		unsigned int unRequestNum);

#if 0
	EstiResult HTTPGet (
		void *pRequestor,
		unsigned int unRequestNum,
		const char * szURL,
		PstiHTTPCallback pfnCallback = NULL,
		void * pParam1 = NULL,
		EstiBool bUseSSL = estiFALSE);
#endif

#if 0
	EstiResult HTTPFormPost (
		void *pRequestor,
		unsigned int unRequestNum,
		const char * szURL,
		const SHTTPVariable * astVariables,
		PstiHTTPCallback pfnCallback = NULL,
		void * pParam1 = NULL,
		EstiBool bUseSSL = estiFALSE);
#endif

	stiHResult HTTPPost (
		void *pRequestor,
		unsigned int unRequestNum,
		const char * pszURL,
		const char * pszURLalt,
		const char * pszData,
		PstiHTTPCallback pfnCallback = nullptr,
		void * pParam1 = nullptr,
		EstiBool bUseSSL = estiFALSE);

	EstiResult HTTPServiceRemove (
		IN CstiHTTPService * poHTTPService);

	EstiResult HTTPUrlResolveSuccess (
		IN SHTTPPostServiceData * poServiceData);

	EstiResult HTTPUrlResolveFail (
		IN SHTTPPostServiceData * poServiceData);

	std::string SecurityExpirationDateGet (CertificateType certificateType) const;

	//
	// Task Functions
	//

	stiHResult Startup () override;

private:

	// Callback function for calls to CCI from child threads.
	static stiHResult UrlResolveCallback(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	int Task () override;

	stiDECLARE_EVENT_MAP (CstiHTTPTask);
	stiDECLARE_EVENT_DO_NOW (CstiHTTPTask);

	stiHResult stiEVENTCALL ShutdownHandle (
		CstiEvent *poEvent);

#if 0
	EstiResult stiEVENTCALL HTTPGetHandle (
		CstiEvent *poEvent);
#endif

#if 0
	EstiResult stiEVENTCALL HTTPFormPostHandle (
		CstiEvent *poEvent);
#endif

	EstiResult stiEVENTCALL HTTPPostHandle (
		CstiEvent *poEvent);

	EstiResult stiEVENTCALL HTTPUrlResolveSuccessHandle (
		CstiEvent *poEvent);

	EstiResult stiEVENTCALL HTTPUrlResolveFailHandle (
		CstiEvent *poEvent);

	//
	// Other message handlers
	//
	EstiResult stiEVENTCALL HeartbeatHandle (
		CstiEvent *poEvent);

	EstiResult stiEVENTCALL HTTPCancelHandle (
		CstiEvent *poEvent);

	EstiResult stiEVENTCALL TimerHandle (
		CstiEvent *poEvent);

	char *m_szPort {nullptr};							// Port on server
	char *m_szFile {nullptr};							// Name of file to request

	std::unique_ptr<CstiURLResolve> m_poUrlResolveTask; 	// the current task for URL resolving

	SSL_CTX *m_ctx {nullptr};

	std::function<void (SSL_SESSION *)> SessionDeleter = [](SSL_SESSION *p){stiSSLSessionFree(p);};
	using SSLSessionType = std::unique_ptr<SSL_SESSION, decltype(SessionDeleter)>;
	std::map<std::string, SSLSessionType> m_sslSessions;

	stiWDOG_ID m_wdidActivityTimer {nullptr};

	std::mutex m_serviceMutex;
	std::list<std::unique_ptr<CstiHTTPService>> m_apoHTTPService;

	//
	// Utility Methods
	//
	EstiResult HTTPReplyReceive (
		std::unique_ptr<CstiHTTPService> &service);

#if 0
	EURLParseResult HTTPURLResolve (
		CstiHTTPService *poService,
		const char *pszURL);
#endif

	CstiHTTPService * ServiceAdd (
		void *pRequestor,
		unsigned int unRequestNum,
		PstiHTTPCallback pfnCallback,
		void * pParam1,
		EstiBool bUseSSL);

	EstiResult ServiceRemove (
		CstiHTTPService *poService);

	std::string reuseSessionFind (
		CstiHTTPService *poHTTPService,
		const std::vector<std::string> &serverList);

	void sslSessionReuse (
		CstiHTTPService *poHTTPService,
		const std::string &foundIP,
		const std::string &serverIP);
};

#endif // CSTIHTTPTASK_H

// end file CstiHTTPTask.h
