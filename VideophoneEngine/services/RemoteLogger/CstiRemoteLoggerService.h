/*!
 * \file CstiRemoteLoggeService.h
 * \brief Definition of the CstiRemoteLoggeService class.
 *
 * Processes RemoteLoggerService requests and responses.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2013 – 2013 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef CSTIREMOTELOGGERSERVICE_H
#define CSTIREMOTELOGGERSERVICE_H

//
// Includes
//
#include "CstiEvent.h"
#include "CstiOsTaskMQ.h"
#include "stiSVX.h"
#include "stiEventMap.h"
#include "CstiRemoteLoggerRequest.h"
#include "CstiVPService.h"

//
// Constants
//

//
// Typedefs
//
#define  RLS_MAX_BUF_LEN	  				4096
#define  RLS_BUFFER_SEND_INTERVAL			(10 * 1000) //Seconds
#define  RLS_REMOTE_LOGGING_TIME_INTERVAL 	(60 * 1000) //One Minute - used as a multiplier
#define  RLS_BIG_INT_MAX_LEN				20 //Max 64 bit unsined int chars
//
// Forward Declarations
//
typedef struct _IXML_Node IXML_Node;
typedef struct _IXML_Document IXML_Document;

//
// Globals
//

//
// Class Declaration
//
class CstiRemoteLoggerService : public CstiVPService
{
public:
	
	CstiRemoteLoggerService (
		CstiHTTPTask *pHTTPTask,
		PstiObjectCallback pAppCallback,
		size_t CallbackParam);
	
	virtual stiHResult Initialize (
			const std::string *pServiceContacts,
			const std::string &uniqueID,
			const std::string &version,
			ProductType productType,
			int nRemoteTraceLogging,
			int nRemoteAssertLogging,
			int nRemoteEventLogging);

	CstiRemoteLoggerService (const CstiRemoteLoggerService &other) = delete;
	CstiRemoteLoggerService (CstiRemoteLoggerService &&other) = delete;
	CstiRemoteLoggerService &operator= (const CstiRemoteLoggerService &other) = delete;
	CstiRemoteLoggerService &operator= (CstiRemoteLoggerService &&other) = delete;

	~CstiRemoteLoggerService () override;

	//
	// Task Functions
	//
	static void RemoteTraceSend (size_t *pParm,
			const char *pszBuff);

	static void RemoteAssertSend (size_t *pParm,
			const std::string &error,
			const std::string &fileAndLine);

	void RemoteCallStatsSend (const char *pszBuff);
	
	void SipStackStatsSend (const char *pszBuff);

	void EndPointDiagnosticsSend (const char *pszBuff);

	static stiHResult RemoteLogEventSend(size_t *pParm,
			const char *pszBuff);

	stiHResult RemoteLoggerTraceEnable(int nRemoteLoggerTraceValue);

	stiHResult RemoteLoggerAssertEnable(int nRemoteLoggerAssertValue);

	stiHResult RemoteLoggerEventEnable(int nRemoteLoggingEventValue);
    
    stiHResult RemoteCallStatsLoggingEnable(int nRemoteCallStatsLoggingValue);

	stiHResult RequestSend (
		CstiRemoteLoggerRequest *poRemoteLoggerRequest,
		unsigned int *punRequestId = nullptr);

	stiHResult RequestSendEx (
		CstiRemoteLoggerRequest *poRemoteLoggerRequest,
		unsigned int *punRequestId = nullptr);
    
    stiHResult RemoteDebugLogSend (
        const std::string &message);

	void LoggedInPhoneNumberSet(const char *pszLoggedInPhoneNumber);
	void MacAddressSet(const char *pszMacAddressSet);
	void NetworkTypeSet(const std::string& networkType);
	void DeviceTokenSet (const std::string& deviceToken);
	void SessionIdSet (const std::string& sessionId);

	stiHResult VRSCallIdSet(const char *pszVRSCallId);
	
	void InterfaceModeSet (EstiInterfaceMode mode);
	void NetworkTypeAndDataSet (NetworkType type, const std::string& data);
	void CoreUserIdsSet (const std::string& userId, const std::string& groupId);
	
	CstiSignalConnection::Collection m_signalConnections;

private:

	using CstiVPService::Initialize;

	stiHResult StateSet (
		EState eState) override;

	stiHResult RequestRemovedOnError(
		CstiVPServiceRequest *request) override;

	void SSLFailedOver() override;
	
	stiHResult RemoteLogSend (
		const std::string &message,
		const std::string &type);
	
	void RemoteLoggingTraceSend ();
	static int RemoteLoggingSendTimerCallback (
		size_t param);
	static int RemoteLoggingTimerCallback (
		size_t param);

	//
	// Utility Methods
	//
	stiHResult ResponseNodeProcess (
		IXML_Node * pNode,
		CstiVPServiceRequest *pRequest,
		unsigned int unRequestId,
		int nPriority,
		EstiBool * pbDatabaseAvailable,
		EstiBool * pbLoggedOutNotified,
		int *pnSystemicError,
		EstiRequestRepeat *peRepeatRequest) override;

	stiHResult ProcessRemainingResponse (
		CstiVPServiceRequest *pRequest,
		int nResponse,
		int nSystemicError) override;
	
	stiHResult Restart () override;
	
#ifdef stiDEBUGGING_TOOLS
	EstiBool IsDebugTraceSet (const char **ppszService) const override;
#endif
	
	const char *ServiceIDGet () const override;
	
	bool saveErrorAndSendLogNow (const std::string &fileAndLine, const std::string &error);
	void logDuplicateErrors ();

	stiWDOG_ID m_wdRemoteLogSend; //!< watch dog timer for sending remote logging buffer
	stiWDOG_ID m_wdRemoteLogging; //!< watch dog timer for time remote logging

	char m_szTraceBuf[RLS_MAX_BUF_LEN]{};

	EstiBool m_bRemoteLogSendTimer;
	EstiBool m_bRemoteLogTimer;

	EstiBool m_bRemoteTraceLogging; // Is logging on
	int m_nRemoteLoggingTraceValue;

	EstiBool m_bRemoteAssertLogging; // Is logging on
	int m_nRemoteLoggingAssertValue;

	EstiBool m_bRemoteEventLogging; // Is logging on
	int m_nRemoteLoggingEventValue;
	   
	EstiBool m_bRemoteCallStatsLogging; // Log remote call stats: On / Off

	unsigned int m_unRequestId;
	
	std::map<std::string, std::pair<std::string, uint32_t>> m_errorMap;
	vpe::EventTimer m_logDuplicateErrorsTimer;
	std::mutex m_mapMutex;
};

#endif // CSTIREMOTELOGGERSERVICE_H

// end file CstiRemoteLoggerService.h
