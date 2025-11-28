///
/// \file CstiCallHistoryItem.h
/// \brief Declaration of the class that implements a single call history item.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2013 by Sorenson Communications, Inc. All rights reserved.
///
#ifndef CSTICALLHISTORYITEM_H
#define CSTICALLHISTORYITEM_H

//
// Includes
//
#include "CstiCallList.h"
#include "ixml.h"
#include "XMLListItem.h"
#include "CstiItemId.h"
#include <string>
#include <algorithm>
#include "stiTools.h"
#include <memory>
#include "AgentHistoryItem.h"
#include <deque>

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
class CstiCallHistoryList;
class CstiCallHistoryItem;

// Smart Pointer to a CstiCallHistoryItem object
using CstiCallHistoryItemSharedPtr = std::shared_ptr<CstiCallHistoryItem>;

//
// Globals
//

//
// Class Declaration
//

class CstiCallHistoryItem : public WillowPM::XMLListItem
{
public:

	CstiCallHistoryItem () = default;
	CstiCallHistoryItem (IXML_Node *pNode);

	CstiCallHistoryItem& operator =(const CstiCallHistoryItem &pRHSCallListItem);
	CstiCallHistoryItem& operator =(const CstiCallListItem &pRHSCallListItem);

	CstiCallListItemSharedPtr CoreItemCreate(bool bBlockedCallerID);

	void GroupedItemAdd (const CstiCallHistoryItemSharedPtr &pItem);
	void GroupedItemsClear ();
	CstiCallHistoryItemSharedPtr GroupedItemGet (unsigned int unIndex);
	unsigned int GroupedItemsCountGet ();
	bool IsGroup () { return (m_pGroupedItems != nullptr); }

	std::string NameGet () const override;
	void NameSet (const std::string &name) override;

	std::string DialStringGet () const;
	void DialStringSet (const std::string &dialString);

	std::string ItemIdGet () const;
	void ItemIdSet (const std::string &itemId);

	EstiDialMethod DialMethodGet () const;
	void DialMethodSet (EstiDialMethod eDialMethod);

	time_t CallTimeGet () const;
	void CallTimeSet (time_t ttCallTime);

	int RingToneGet () const;
	void RingToneSet (int nRingTone);

	EstiBool InSpeedDialGet () const;
	void InSpeedDialSet (EstiBool bInSpeedDial);

	std::string CallPublicIdGet () const;
	void CallPublicIdSet (const std::string &callPublicId);

	std::string IntendedDialStringGet () const;
	void IntendedDialStringSet (const std::string &intendedDialString);

	void DurationSet (int nLength);
	int DurationGet () const;

	std::string vrsCallIdGet () const;
	void vrsCallIdSet (const std::string &vrsCallId);
	std::string agentIdGet ();
	std::deque<AgentHistoryItem> m_agentHistory;

	int PhoneNumberTypeGet() const;
	void PhoneNumberTypeSet(int nType);

	std::string ImageIdGet() const;
	void ImageIdSet(const std::string &ImageId);

	CstiCallList::EType CallListTypeGet() const;
	void CallListTypeSet(CstiCallList::EType eType);

	// Remove these methods once core stops sending us the number back
	// in the response to an add request with a blocked caller ID
	EstiBool BlockedCallerIDGet () const;
	void BlockedCallerIDSet (EstiBool bBlockedCallerID);

	int callIndexGet () const;
	void callIndexSet (int value);

    void Write(FILE *pFile) override;
	void WriteElement (FILE *pFile, const AgentHistoryItem& item);

	~CstiCallHistoryItem () override;

private:

	std::string m_Name;                     //!< Caller ID name
	std::string m_DialString;               //!< Caller ID phone number
	std::string m_ItemId;                   //!< Core's ID for this item
	EstiDialMethod m_eDialMethod{estiDM_BY_DS_PHONE_NUMBER};           //!< Dial method
	time_t m_ttCallTime{};                  //!< Time at the start of the call
	int m_nRingTone{0};                     //!< Light Ring pattern
	EstiBool m_bInSpeedDial{estiFALSE};     //!< Whether this number is in the phonebook
	std::string m_CallPublicId;             //!< Core's public ID for this item
	std::string m_IntendedDialString;       //!< Intended Dial String
	int m_nDuration{0};                     //!< This is probably never used for calls
	std::string m_vrsCallId;

	// Properties used for displaying the data
	int m_nNumberType{0};                      //!< Phone number type (Home, Work, Cell)
	std::string m_ImageId;                  //!< Image GUID if it's in the contact list
	CstiCallList::EType m_eCallListType{CstiCallList::eTYPE_UNKNOWN};    //!< Call list type (Missed, Dialed, Received, etc.)

	CstiCallHistoryList *m_pGroupedItems{nullptr};   //!< List of items grouped under this item

	EstiBool m_bBlockedCallerID{estiFALSE};             //!< Whether the caller ID item is blocked

	int m_callIndex{0};

	bool NameMatch(const std::string &name) override;
	void PopulateWithIXML_Node (IXML_Node *pNode);
}; // end class CstiCallHistoryItem

#endif // CSTICALLHISTORYITEM_H

// end file CstiCallHistoryItem.h

