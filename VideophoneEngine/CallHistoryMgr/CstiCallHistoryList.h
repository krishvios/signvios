///
/// \file CstiCallHistoryList.h
/// \brief Declaration of the class that implements a list of call history items
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2013 by Sorenson Communications, Inc. All rights reserved.
///
#ifndef CSTICALLHISTORYLIST_H
#define CSTICALLHISTORYLIST_H

//
// Includes
//
#include "stiDefs.h"
#include "stiTools.h"
#include "ixml.h"
#include "CstiCallList.h"
#include "CstiCallHistoryItem.h"
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

//
// Globals
//

//
// Class Declaration
//
class CstiCallHistoryList : public WillowPM::XMLListItem
{
public:
	CstiCallHistoryList (CstiCallList::EType eType);
	CstiCallHistoryList (IXML_Node *pNode);

	CstiCallHistoryList (const CstiCallHistoryList &other) = delete;
	CstiCallHistoryList (CstiCallHistoryList &&other) = delete;
	CstiCallHistoryList &operator= (const CstiCallHistoryList &other) = delete;
	CstiCallHistoryList &operator= (CstiCallHistoryList &&other) = delete;

	~CstiCallHistoryList () override;

	void Initialize ();

	stiHResult ItemAdd (
		CstiCallHistoryItemSharedPtr spoListItem);

	stiHResult ItemInsert (
		CstiCallHistoryItemSharedPtr spoListItem,
		unsigned int unPos);

	stiHResult ItemRemove (
		unsigned int nItem);

	void ItemRemoveByID (
		const std::string & itemID);

	void ListFree ();

	CstiCallHistoryItemSharedPtr ItemGet (
		unsigned int nItem) const;

	CstiCallHistoryItemSharedPtr ItemFind (
		const char *pszItemId);

	unsigned int CountGet () const;

	unsigned int MaxCountGet () const;

	stiHResult MaxCountSet (
		unsigned int nMax);

	CstiCallList::EType TypeGet () const;

	stiHResult TypeSet (
		CstiCallList::EType eType);

	bool NeedsUpdateGet () const;

	void NeedsUpdateSet (
		bool bUpdate);

	bool NeedsNewItemsGet () const;

	void NeedsNewItemsSet (
		bool bNew);

	void UnviewedItemCountCalculate (
		time_t lastViewedTime);

	unsigned int UnviewedItemCountGet () const;

	void UnviewedItemCountSet (
		unsigned int num);

	unsigned int NewItemCountGet () const;

	void NewItemCountSet (
		unsigned int num);

	void Write (FILE *pFile) override;
	std::string NameGet () const override;
	void NameSet (const std::string &name) override;
	bool NameMatch (const std::string &name) override;
	void PopulateWithIXML_Node (IXML_Node *pNode);
	time_t LastRetrievalTimeGet ();

protected:
	std::vector<CstiCallHistoryItemSharedPtr> m_List;
	CstiCallList::EType m_eCallListType;
	unsigned int m_unMaxCount{0};

	bool m_bNeedsUpdate{false};
	bool m_bNeedsNewItems{false};
	unsigned int m_numNewItems{0};
	unsigned int m_numUnviewedItems{0};
	time_t m_ttLastRetrievalTime{0};
};

using CstiCallHistoryListSharedPtr = std::shared_ptr<CstiCallHistoryList>;

#endif // CSTICALLHISTORYLIST_H

// end file CstiCallHistoryList.h
