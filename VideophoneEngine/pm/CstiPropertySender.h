/*!
 * \file CstiPropertySender.h
 * \brief Module for sending property values to the core server.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIPROPERTYSENDER_H
#define CSTIPROPERTYSENDER_H

#include <algorithm>
#include <list>
#include <condition_variable>
#include <mutex>
#include "stiOS.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "CstiSignal.h"
#include "CstiEventQueue.h"
#include "ServiceResponse.h"

class CstiCoreService;
class CstiCoreResponse;
struct ClientAuthenticateResult;


class CstiPropertySender
{
public:

	CstiPropertySender (
		int maxPropertiesToSend,		// Maximum number of properties to send in a batch
		int delayTime,				// Delay time in milliseconds
		int errorDelayTime);			// For an error, delay time in milliseconds

	~CstiPropertySender ();
	
	CstiPropertySender (const CstiPropertySender &other) = delete;
	CstiPropertySender (CstiPropertySender &&other) = delete;
	CstiPropertySender &operator= (const CstiPropertySender &other) = delete;
	CstiPropertySender &operator= (CstiPropertySender &&other) = delete;

	void CoreServiceSet (
		CstiCoreService *pCoreService);
		
	void PropertySend (
		const std::string& name,
		int nType);

	void PropertySendCancel ();
	
	void ResponseCheck(
		const CstiCoreResponse *poResponse);

	void SendProperties();
	
	void PropertySendWait (int timeoutMilliseconds);

private:
	
	enum EstiPropertySendState
	{
		estiPSSPending,
		estiPSSSending
	};

	struct SstiProperty
	{
		std::string Name;
		int nType{};
		EstiPropertySendState state{};
		bool resend{};
	};

	void propertySendHandle (const std::string& name, int nType);

	void TimerSet (
		int delayTime);

	//
	// Event handlers.
	//
	static stiHResult RequestSendCallback (
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam,
		size_t CallbackFromId);

	void requestSendHandle (EstiCmMessage message, unsigned int requestId);

	void responseCheckHandle (unsigned int requestId, CstiCoreResponse::EResponse response, EstiResult responseResult);
		
	void UpdateSendingState(
		EstiResult eResult,
		int nType);

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);
	
	void PropertyRequestEnd ();

	std::list<SstiProperty *> m_PropertyList;

	bool m_bUserAuthenticated;
	int m_maxPropertiesToSend;
	int m_delayTime;
	int m_errorDelayTime;
	bool m_bRequestPending;
	unsigned int m_unUserRequestId;
	unsigned int m_unPhoneRequestId;
	int m_nUserCount;
	int m_nPhoneCount;
	CstiEventQueue& m_eventQueue;
	vpe::EventTimer m_sendTimer;				// Timer for sending properties
	
	// These are used to wait on the properties to be sent.
	std::condition_variable m_sendingCondition;
	std::mutex m_sendingMutex;

	CstiCoreService *m_pCoreService;
	CstiSignalConnection m_clientAuthSignalConnection;
	CstiSignalConnection::Collection m_signalConnections;

};


#endif // CSTIPROPERTYSENDER_H
