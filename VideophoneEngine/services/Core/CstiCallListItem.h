///
/// \file CstiCallListItem.h
/// \brief Declaration of the class that implements a single call list item.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///
#ifndef CSTI_CALL_LIST_ITEM_H
#define CSTI_CALL_LIST_ITEM_H

//
// Includes
// 
#include <ctime>
#include <deque>
#include "stiDefs.h"
#include "stiSVX.h"
#include "CstiListItem.h"
#include "CstiString.h"
#include "AgentHistoryItem.h"

//
// Constants
//

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
// Class Declaration
//

class CstiCallListItem : public CstiListItem
{
public:
	
	CstiCallListItem () = default;
	~CstiCallListItem () override = default;
	
	const char * NameGet () const;
	void NameSet (const char *pszName);
	const char * DialStringGet () const;
	void DialStringSet (const char *pszDialString);
	const char * ItemIdGet () const;
	void ItemIdSet (const char *pszItemId);
	EstiDialMethod DialMethodGet () const;
	void DialMethodSet (EstiDialMethod eDialMethod);
	time_t CallTimeGet () const;
	void CallTimeSet (time_t ttCallTime);
    int RingToneGet () const;
    void RingToneSet (int nRingTone);
    EstiBool InSpeedDialGet () const;
	void InSpeedDialSet (EstiBool bInSpeedDial);
	EstiBool BlockedCallerIDGet () const;
    void BlockedCallerIDSet (EstiBool bBlockedCallerID);
	std::string vrsCallIdGet () const;
	void vrsCallIdSet (const std::string &vrsCallId);
	
	const char * CallPublicIdGet () const;
	void CallPublicIdSet (const char *pszCallPublicId);
	const char * IntendedDialStringGet () const;
	void IntendedDialStringSet (const char *pszIntendedDialString);
	
	void DurationSet (
		int nLength);
		
	int DurationGet () const;

	std::deque<AgentHistoryItem> m_agentHistory;
	
private:
	
	CstiString m_Name;
	CstiString m_DialString;
	CstiString m_ItemId;
	EstiDialMethod m_eDialMethod{estiDM_UNKNOWN};
	time_t m_ttCallTime{0};
    int m_nRingTone{0};
    EstiBool m_bInSpeedDial{estiFALSE};
	CstiString m_CallPublicId;
	CstiString m_IntendedDialString;
	EstiBool m_bBlockedCallerID{estiFALSE};
	std::string m_vrsCallId;

	int m_nDuration{0};
		
}; // end class CstiCallListItem

using CstiCallListItemSharedPtr = std::shared_ptr<CstiCallListItem>;

#endif // CSTI_CALL_LIST_ITEM_H

// end file CstiCallListItem.h
