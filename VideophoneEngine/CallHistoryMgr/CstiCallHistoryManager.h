/*!
 * \file CstiCallHistoryManager.h
 * \brief See CstiCallHistoryManager.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTICALLHISTORYMANAGER_H
#define CSTICALLHISTORYMANAGER_H

//
// Includes
//
#include "IstiCallHistoryManager.h"
#include "CstiCallHistoryItem.h"
#include "CstiCallHistoryList.h"
#include "XMLManager.h"
#include "XMLList.h"
#include "CstiSignal.h"
#include "CstiVPServiceResponse.h"
#include "ServiceResponse.h"
#include <sstream>
#include <vector>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
class CstiCallListItem;
class CstiCallList;
class CstiCoreResponse;
class CstiStateNotifyResponse;
class CstiCoreService;
struct ClientAuthenticateResult;
class CstiCoreRequest;
struct SstiRemovedRequest;
class IstiContactManager;
class CstiEventQueue;

//
// Globals
//

class CstiCallHistoryManager : public WillowPM::XMLManager, public IstiCallHistoryManager
{
public:
	CstiCallHistoryManager (
		bool bEnabled,
		CstiCoreService *pCoreService,
		IstiContactManager *pContactMgr,
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam
	);

	stiHResult Initialize ();

	CstiCallHistoryManager (const CstiCallHistoryManager &other) = delete;
	CstiCallHistoryManager (CstiCallHistoryManager &&other) = delete;
	CstiCallHistoryManager &operator= (const CstiCallHistoryManager &other) = delete;
	CstiCallHistoryManager &operator= (CstiCallHistoryManager &&other) = delete;

	~CstiCallHistoryManager () override;

	CstiCallHistoryItemSharedPtr ItemCreate() override;

	EstiBool CoreResponseHandle (
		CstiCoreResponse *pCoreResponse);

	EstiBool RemovedUponCommunicationErrorHandle (
		CstiVPServiceResponse *response);

	EstiBool StateNotifyEventHandle (
		CstiStateNotifyResponse *pStateNotifyResponse);

	stiHResult ItemAdd (
		CstiCallList::EType eCallListType,
		CstiCallHistoryItemSharedPtr spCallListItem,
		bool bBlockedCallerID,
		unsigned int *punRequestId) override;

	stiHResult ItemEdit (
		CstiCallList::EType eCallListType,
		CstiCallHistoryItemSharedPtr spCallListItem,
		bool bBlockedCallerID,
		unsigned int *punRequestId);

	stiHResult ItemDelete (
		CstiCallList::EType eCallListType,
		const CstiItemId &ItemId,
		unsigned int *punRequestId) override;


	stiHResult ItemGetByIndex (
		CstiCallList::EType eCallListType,
		int nIndex,
		CstiCallHistoryItemSharedPtr *pspCallListItem) const override;

	CstiCallHistoryItemSharedPtr itemGetByCallIndex (int callIndex, CstiCallList::EType callListType);

	stiHResult ListUpdate (
		CstiCallList::EType eCallListType);

	stiHResult ListUpdateAll () override;

	stiHResult ListClear (
		CstiCallList::EType eCallListType) override;

	stiHResult ListClearAll () override;

	stiHResult MissedCallsClear () override;

	unsigned int ListCountGet (
		CstiCallList::EType eType) override;

	unsigned int UnviewedItemCountGet(
		CstiCallList::EType eCallListType) override;

	stiHResult RecentListCompress ();


	stiHResult MaxCountSet (
		CstiCallList::EType eCallListType,
		int nMaxLength) override;


	bool EnabledGet () { return m_bEnabled; }


	void EnabledSet (bool bEnabled);

	WillowPM::XMLListItemSharedPtr NodeToItemConvert (IXML_Node *pNode) override;


	stiHResult Lock () const override;

	void Unlock () const override;

	void CachePurge () override;

	void Refresh () override;

	stiHResult CoreRequestSend (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId,
		CstiCallList::EType eCallListType);

	CstiSignal<CstiCallHistoryItemSharedPtr> callHistoryAdded;

private:
	bool m_bEnabled;
	bool m_bInitialized;
	CstiCoreService *m_pCoreService;
	EstiBool m_bAuthenticated;
	PstiObjectCallback m_pfnVPCallback;
	size_t m_CallbackParam;
	size_t m_CallbackFromId;
	time_t m_LastMissedTime;
	unsigned int m_unOnlyNewItemsCookie;
	std::list<unsigned int> m_PendingCookies[CstiCallList::eTYPE_LAST];
	CstiSignalConnection::Collection m_signalConnections;
	CstiEventQueue& m_eventQueue;

	std::vector<CstiCallHistoryListSharedPtr> m_pCallLists;
	
	IstiContactManager *m_pContactMgr;

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);

	stiHResult AuthenticatedSet (
		EstiBool bAuthenticated);


	void CallbackMessageSend (
		EstiCmMessage eMessage,
		size_t Param = 0);


	CstiCallHistoryItemSharedPtr CoreListItemConvert(
		CstiCallList::EType eCallListType,
		const CstiCallListItemSharedPtr &coreItem);

	stiHResult CoreListItemsAdd (
		CstiCoreResponse *pCoreResponse,
		CstiCallList::EType eCallListType);

	stiHResult CoreListItemsRemove (
		CstiCoreResponse *pCoreResponse,
		CstiCallList::EType eCallListType);

	stiHResult CoreListItemsGet (
		CstiCoreResponse *pCoreResponse,
		CstiCallList::EType eCallListType);

	CstiCallHistoryItemSharedPtr ItemGetById (
		CstiCallList::EType eCallListType,
		const char *pszItemId) const;


	CstiCallList::EType RemoveRequestId (
		unsigned int unRequestId);


	stiHResult RecentListRebuild ();


	stiHResult ItemInsert (
		CstiCallList::EType eCallListType,
		CstiCallHistoryItemSharedPtr spItem);
};


#endif //#ifndef CSTICALLHISTORYMANAGER_H
