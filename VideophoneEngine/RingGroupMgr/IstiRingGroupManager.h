/*!
 *
 *\file IstiRingGroupManager.h
 *\brief Declaration of the RingGroupManager interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTIRINGGROUPMANAGER_H
#define ISTIRINGGROUPMANAGER_H


#include <string>
#include "stiSVX.h"
#include "IstiRingGroupMgrResponse.h"


/*!
 * \ingroup VideophoneEngineLayer 
 * \brief Manages the ring group. 
 *  
 */
class IstiRingGroupManager
{
public:

	// Enum for Ring Group member state
	enum ERingGroupMemberState
	{
		eRING_GROUP_STATE_UNKNOWN,
		eRING_GROUP_STATE_ACTIVE,
		eRING_GROUP_STATE_PENDING,
	};

	// Enum for Ring Group display mode (for RingGroupDisplayMode property stored on Core)
	enum ERingGroupDisplayMode
	{
		eRING_GROUP_DISABLED = -1,
		eRING_GROUP_DISPLAY_MODE_HIDDEN,
		eRING_GROUP_DISPLAY_MODE_READ_ONLY,
		eRING_GROUP_DISPLAY_MODE_READ_WRITE
	};
	
	virtual ERingGroupDisplayMode ModeGet () = 0;

	virtual void ModeSet (ERingGroupDisplayMode eMode) = 0;

	virtual bool IsInRingGroup () = 0;

	virtual bool IsGroupCreator (const std::string& memberNumber) = 0;

	virtual stiHResult ItemAdd (
		const char *pszMemberNumber,
		const char *pszDescription,
		unsigned int *punRequestId) = 0;

	virtual stiHResult ItemEdit (
		const char *pszMemberNumber,
		const char *pszNewDescription,
		unsigned int *punRequestId) = 0;

	virtual stiHResult ItemDelete (
		const char *pszMemberNumber,
		unsigned int *punRequestId) = 0;

	virtual EstiBool ItemGetByNumber (
		const char *pszMemberNumber,
		std::string *pDescription,
		ERingGroupMemberState *pMemberState) const = 0;

	virtual EstiBool ItemGetByDescription (
		const char *pszDescription,
		std::string *pMemberNumber,
		ERingGroupMemberState *pMemberState) const = 0;

	virtual stiHResult ItemGetByIndex (
		int nIndex,
		std::string *pMemberLocalNumber,
		std::string *pMemberTollFreeNumber,
		std::string *pDescription,
		ERingGroupMemberState *pMemberState) const = 0;
		
	virtual int ItemCountGet () const = 0;

	virtual void RingGroupInfoGet () = 0;

	virtual stiHResult Lock () const = 0;

	virtual void Unlock () const = 0;

#ifdef stiRING_GROUP_MANAGER_v2
	virtual stiHResult InvitationGet (
		std::string *pLocalNumber,
		std::string *pTollFreeNumber,
		std::string *pDescription) const = 0;

	virtual stiHResult InvitationAccept () = 0;

	virtual stiHResult InvitationReject () = 0;

	virtual stiHResult PasswordValidate (
		const char *pszNumber,
		const char *pszPassword) = 0;

	virtual stiHResult PasswordChange (
		const char *pszPassword) = 0;

	virtual stiHResult RingGroupCreate (
		const char *pszDescription,
		const char *pszPassword) = 0;

#endif
};

#endif // ISTIRINGGROUPMANAGER_H
