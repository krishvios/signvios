/*!
 * \file CstiRingGroupManager.h
 * \brief See CstiRingGroupManager.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIRINGGROUPMANAGER_H
#define CSTIRINGGROUPMANAGER_H

//
// Includes
//
#include "IstiRingGroupManager.h"
#include "CstiCoreResponse.h"
#include "CstiStateNotifyResponse.h"
#include "CstiCoreService.h"
#include "XMLManager.h"
#include "XMLList.h"
#include <list>


//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
class CstiEventQueue;

//
// Globals
//

class CstiRingGroupManager : public WillowPM::XMLManager, public IstiRingGroupManager
{
public:
	CstiRingGroupManager (
		ERingGroupDisplayMode eMode,
		CstiCoreService *pCoreService,
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam
	);

	virtual ~CstiRingGroupManager ();

	CstiRingGroupManager (const CstiRingGroupManager &other) = delete;
	CstiRingGroupManager (CstiRingGroupManager &&other) = delete;
	CstiRingGroupManager &operator= (const CstiRingGroupManager &other) = delete;
	CstiRingGroupManager &operator= (CstiRingGroupManager &&other) = delete;

	EstiBool CoreResponseHandle (
		CstiCoreResponse *pCoreResponse);


	EstiBool RemovedUponCommunicationErrorHandle (
		CstiVPServiceResponse *response);


	EstiBool StateNotifyEventHandle (
		CstiStateNotifyResponse *pStateNotifyResponse);


	void Initialize ();


	ERingGroupDisplayMode ModeGet () override { return m_eMode; }

	void ModeSet (ERingGroupDisplayMode eMode) override;

	bool IsInRingGroup () override { return m_pdoc != nullptr && m_pdoc->CountGet () > 0; }

	bool IsGroupCreator (const std::string& memberNumber) override;

	stiHResult ItemAdd (
		const char *pszMemberNumber,
		const char *pszDescription,
		unsigned int *punRequestId) override;

	stiHResult ItemEdit (
		const char *pszMemberNumber,
		const char *pszNewDescription,
		unsigned int *punRequestId) override;

	stiHResult ItemDelete (
		const char *pszMemberNumber,
		unsigned int *punRequestId) override;

	EstiBool ItemGetByNumber (
		const char *pszMemberNumber,
		std::string *pDescription,
		ERingGroupMemberState *pMemberState) const override;

	EstiBool ItemGetByDescription (
		const char *pszDescription,
		std::string *pMemberNumber,
		ERingGroupMemberState *pMemberState) const override;

	stiHResult ItemGetByIndex (
		int nIndex,
		std::string *pMemberLocalNumber,
		std::string *pMemberTollFreeNumber,
		std::string *pDescription,
		ERingGroupMemberState *pMemberState) const override;

	int ItemCountGet () const override;

	void RingGroupInfoGet () override;

	inline stiHResult Lock () const override
	{
		return stiRESULT_SUCCESS;
	}

	inline void Unlock () const override
	{
	}

#ifdef stiRING_GROUP_MANAGER_v2
	stiHResult InvitationGet (
		std::string *pLocalNumber,
		std::string *pTollFreeNumber,
		std::string *pDescription) const override;

	stiHResult InvitationAccept () override;
	stiHResult InvitationReject () override;

	stiHResult PasswordValidate (
		const char *pszNumber,
		const char *pszPassword) override;

	stiHResult PasswordChange (
		const char *pszPassword) override;

	stiHResult RingGroupCreate (
		const char *pszDescription,
		const char *pszPassword) override;

#endif

private:
	
	class  CstiRingGroupMemberInfo : public WillowPM::XMLListItem
	{
	public:
		
		//
		// Constructor
		//
		CstiRingGroupMemberInfo () = default;
		CstiRingGroupMemberInfo (IXML_Node* pNode);
		~CstiRingGroupMemberInfo () = default;

		CstiRingGroupMemberInfo (const CstiRingGroupMemberInfo &Other) = default;
		CstiRingGroupMemberInfo (CstiRingGroupMemberInfo &&other) = default;
		CstiRingGroupMemberInfo &operator= (const CstiRingGroupMemberInfo &other) = default;
		CstiRingGroupMemberInfo &operator= (CstiRingGroupMemberInfo &&other) = default;

		std::string Description;		/*!< Descriptive name (or alias) of device */
		std::string LocalPhoneNumber;	/*!< Private local phone number of device */
		std::string TollFreePhoneNumber;	/*!< Private toll-free phone number of device */
		ERingGroupMemberState eState{eRING_GROUP_STATE_UNKNOWN};	/*!< Member state (e.g. Active, Pending) */
		bool bIsGroupCreator{false};	/*!< True if group creator */

		void Write (FILE* pFile) override;
		std::string NameGet () const override;
		void NameSet (const std::string& name) override;
		bool NameMatch (const std::string& name) override;
		
		static constexpr const char * NodeName = "RingGroupItem";
	};

	const char* m_fileName = "RingGroup.xml";

	bool m_bInitialized;
	ERingGroupDisplayMode m_eMode;
	CstiCoreService *m_pCoreService;
	EstiBool m_bAuthenticated;
	unsigned int m_nMaxLength;
	PstiObjectCallback m_pfnVPCallback;
	size_t m_CallbackParam;
	size_t m_CallbackFromId;
	std::list<unsigned int> m_PendingCookies;
	std::string m_ItemAddNumber;
	std::string m_ItemAddDescription;
	unsigned int m_unItemAddRequestId;
	CstiSignalConnection::Collection m_signalConnections;
	CstiEventQueue& m_eventQueue;

#ifdef stiRING_GROUP_MANAGER_v2
	std::string m_InviteName;
	std::string m_InviteLocalNumber;
	std::string m_InviteTollFreeNumber;
#endif

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);

	stiHResult AuthenticatedSet (
		EstiBool bAuthenticated);


	void CallbackMessageSend (
		EstiCmMessage eMessage,
		size_t Parammak);


	stiHResult CoreRequestSend (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId);


	EstiBool ItemGet (
		const std::string& memberNumber,
		CstiRingGroupMemberInfo *pMemberInfo) const;


	EstiBool RemoveRequestId (
		unsigned int unRequestId);

	void RingGroupInfoListGetFromResponse (
		CstiCoreResponse *poResponse);

	int RingGroupInfoListCountGet (
		CstiCoreResponse *poResponse);

	stiHResult ItemAddCoreRequestSend ();

	WillowPM::XMLListItemSharedPtr NodeToItemConvert (IXML_Node* pNode) override;

	void clear ();
};

#endif //#ifndef CSTIRINGGROUPMANAGER_H
