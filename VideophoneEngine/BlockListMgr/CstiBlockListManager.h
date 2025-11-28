/*!
 * \file CstiBlockListManager.h
 * \brief See CstiBlockListManager.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIBLOCKLISTMANAGER_H
#define CSTIBLOCKLISTMANAGER_H

//
// Includes
//
#include "IstiBlockListManager.h"
#include "IstiBlockListManager2.h"
#include "CstiBlockListItem.h"
#include "XMLManager.h"
#include "CstiCoreResponse.h"
#include "CstiStateNotifyResponse.h"
#include "CstiCoreService.h"
#include "CstiSignal.h"
#include <list>

class CstiEventQueue;

namespace WillowPM
{

//
// Constants
//

//
// Typedefs
//

//
// Globals
//

class CstiBlockListManager : public XMLManager, public IstiBlockListManager,
							 public IstiBlockListManager2
{
public:
	CstiBlockListManager (
		bool bEnabled,
		CstiCoreService* pCoreService,
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam
	);


	~CstiBlockListManager () override = default;

	CstiBlockListManager (const CstiBlockListManager &other) = delete;
	CstiBlockListManager (CstiBlockListManager &&other) = delete;
	CstiBlockListManager &operator= (CstiBlockListManager &other) = delete;
	CstiBlockListManager &operator= (CstiBlockListManager &&other) = delete;


	EstiBool CoreResponseHandle (
		CstiCoreResponse *pCoreResponse);


	EstiBool RemovedUponCommunicationErrorHandle (
		CstiVPServiceResponse *response);


	EstiBool StateNotifyEventHandle (
		CstiStateNotifyResponse *pStateNotifyResponse);


	bool CallBlocked (const char *pszNumber) override;


	stiHResult Initialize ();


	stiHResult ItemAdd (
		const char *pszBlockedNumber,
		const char *pszDescription,
		unsigned int *punRequestId) override;


	stiHResult ItemDelete (
		const char *pszBlockedNumber,
		unsigned int *punRequestId) override;


	stiHResult ItemEdit (
		const char *pszItemId,
		const char *pszNewBlockedNumber,
		const char *pszNewDescription,
		unsigned int *punRequestId) override;


	bool ItemGetByNumber (
		const char *pszBlockedNumber,
		std::string *pItemId,
		std::string *pDescription) const override;


	bool ItemGetById (
		const char *pszItemId,
		std::string *pBlockedNumber,
		std::string *pDescription) const override;


	stiHResult ItemGetByIndex (
		int nIndex,
		std::string *pItemId,
		std::string *pBlockedNumber,
		std::string *pDescription) const override;


	inline stiHResult Lock () const override
	{
		lock ();
		return stiRESULT_SUCCESS;
	}


	inline void Unlock () const override
	{
		unlock ();
	}

	void Refresh () override;
	void PurgeItems() override;
	
	stiHResult MaximumLengthSet (
		int nMaxLength) override;

	unsigned int MaximumLengthGet () const override
	{
		return m_nMaxLength;
	}


	bool EnabledGet () override { return m_bEnabled; }


	void EnabledSet (bool bEnabled) override;

	ISignal<IstiBlockListManager::Response, stiHResult, unsigned int, const std::string &>& errorSignalGet () override;


private:
	bool m_bEnabled;
	bool m_bInitialized;
	CstiCoreService *m_pCoreService;
	EstiBool m_bAuthenticated;
	unsigned int m_nMaxLength;
	PstiObjectCallback m_pfnVPCallback;
	size_t m_CallbackParam;
	size_t m_CallbackFromId;
	std::list<unsigned int> m_PendingCookies;
	CstiSignalConnection::Collection m_signalConnections;
	CstiEventQueue& m_eventQueue;

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);

	stiHResult AuthenticatedSet (
		EstiBool bAuthenticated);


	void CallbackMessageSend (
		EstiCmMessage eMessage,
		size_t Parammak = 0);


	void errorSend (
		CstiCoreResponse *coreResponse,
		EstiCmMessage message);


	Response coreResponseConvert(
		CstiCoreResponse::EResponse responseType);


	int CoreListItemsModify (
		CstiCoreResponse *pCoreResponse,
		EstiCmMessage eMessage);


	stiHResult CoreRequestSend (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId);


	CstiBlockListItemSharedPtr ItemGet (
		const char *pszBlockedNumber) const;


	CstiBlockListItemSharedPtr ItemGetById (
		const char *pszItemId) const;


	XMLListItemSharedPtr NodeToItemConvert (
		IXML_Node *pNode) override;


	EstiBool RemoveRequestId (
		unsigned int unRequestId);

	//Signals
	CstiSignal<IstiBlockListManager::Response, stiHResult, unsigned int, const std::string &> errorSignal;
};

} // end namespace

#endif //#ifndef CSTIBLOCKLISTMANAGER_H
