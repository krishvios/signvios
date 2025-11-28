///
/// \file CstiCallHistoryList.cpp
/// \brief Implements a list of call history items
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2013 by Sorenson Communications, Inc. All rights reserved.
///

//
// Includes
//

#include "stiSVX.h"
#include "CstiCallHistoryList.h"

//
// Constants
//
const char XMLCallHistoryList[] =  "CallHistoryList";
const char XMLCallListType[] = "CallListType";
const char XMLList[] = "List";
const char XMLLastRetrievalTime[] = "LastRetrievalTime";

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
// Locals
//

//
// Function Declarations
//

//:-----------------------------------------------------------------------------
//:	Construction / Destruction
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::CstiCallHistoryList
//
// Description: Default Constructor
//
// Abstract:
//
// Returns:
//	None
//
CstiCallHistoryList::CstiCallHistoryList (CstiCallList::EType eType)
	:
	
	m_eCallListType (eType)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::CstiCallHistoryList);

	Initialize ();
}


CstiCallHistoryList::CstiCallHistoryList (IXML_Node *pNode)
	:
	m_eCallListType (CstiCallList::eTYPE_UNKNOWN)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::CstiCallHistoryList);

	Initialize ();
	PopulateWithIXML_Node (pNode);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::~CstiCallHistoryList
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	None
//
CstiCallHistoryList::~CstiCallHistoryList ()
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::~CstiCallHistoryList);

	ListFree();
}


void CstiCallHistoryList::Initialize ()
{
	m_unMaxCount = 9999;
	m_bNeedsUpdate = false;
	m_bNeedsNewItems = false;
	m_numNewItems = 0;
	m_numUnviewedItems = 0;
	m_ttLastRetrievalTime = 0;
}


void CstiCallHistoryList::ListFree ()
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::ListFree);

	m_List.clear();
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::ItemGet
//
// Description: Returns the indicated item, or NULL
//
// Abstract:
//
// Returns:
//	CstiCallHistoryItemSharedPtr - Pointer to requested item, or NULL on error
//
CstiCallHistoryItemSharedPtr CstiCallHistoryList::ItemGet (
	unsigned int unItem) const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::ItemGet);

	CstiCallHistoryItemSharedPtr spoReturnListItem;

	if (unItem < m_List.size())
	{
		spoReturnListItem = m_List[unItem];
	}

	return (spoReturnListItem);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::ItemRemove
//
// Description: Removes an item from the list
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallHistoryList::ItemRemove (
	unsigned int unItem)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::ItemRemove);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (unItem < m_List.size (), stiRESULT_ERROR);

	m_List.erase(m_List.begin() + unItem);

STI_BAIL:

	return hResult;
}


void CstiCallHistoryList::ItemRemoveByID (const std::string & itemID)
{
	m_List.erase (
		std::remove_if (m_List.begin (), m_List.end (), 
			[itemID](const CstiCallHistoryItemSharedPtr& item){ return item->ItemIdGet() == itemID; })
		, m_List.end ());
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::ItemAdd
//
// Description: Adds an item to the array of CallListItems
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallHistoryList::ItemAdd (
	CstiCallHistoryItemSharedPtr spListItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiCallHistoryList::ItemAdd);

	m_List.push_back(spListItem);

	m_ttLastRetrievalTime = std::max (spListItem->CallTimeGet (), m_ttLastRetrievalTime);

	// NOTE: Currently, ItemAdd() is only used when loading the entire lists from core,
	//  therefore will never cause max list size to be exceeded.
	//  The ItemInsert() function is used to add new list items.

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::ItemInsert
//
// Description: Inserts an item into the array of CallListItems
//
// Abstract:
//
// Returns:
//	stiHResult - stiRESULT_SUCCESS on success, otherwise stiRESULT_ERROR
//
stiHResult CstiCallHistoryList::ItemInsert(
	CstiCallHistoryItemSharedPtr spoListItem,
	unsigned int unPos)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiCallHistoryList::ItemInsert);

	m_List.insert(m_List.begin() + unPos, spoListItem);

	m_ttLastRetrievalTime = std::max (spoListItem->CallTimeGet (), m_ttLastRetrievalTime);

	if (m_List.size() > m_unMaxCount)
	{
		// NOTE: this assumes a newest-to-oldest order, thus delete end of list
		m_List.pop_back();
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallList::ItemFind
//
// Description: Find a specific item based on its ItemId
//
// Abstract:
//
// Returns:
//	CstiCallHistoryItemSharedPtr Pointer to the item
//
CstiCallHistoryItemSharedPtr CstiCallHistoryList::ItemFind (
	const char *pszItemId)
{
	CstiCallHistoryItemSharedPtr spItem;

	for (auto &item: m_List)
	{
		if (item->ItemIdGet() == pszItemId)
		{
			spItem = item;
			break;
		}
	}

	return spItem;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::CountGet
//
// Description: Returns the number of items in the list
//
// Returns:
//	unsigned int - The size of the list
//
unsigned int CstiCallHistoryList::CountGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::CountGet);

	return m_List.size();
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::TypeGet
//
// Description: Returns the type of items in the list
//
// Returns:
//	EType - The type of list
//
CstiCallList::EType CstiCallHistoryList::TypeGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::TypeGet);

	return m_eCallListType;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::TypeSet
//
// Description: Sets the list type
//
// Returns:
//	stiHResult
//
stiHResult CstiCallHistoryList::TypeSet (
	CstiCallList::EType eType)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::TypeSet);

	m_eCallListType = eType;

	return stiRESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::MaxCountGet
//
// Description: Returns the maximum number of items in the list
//
// Returns:
//	unsigned int - The max size of the list
//
unsigned int CstiCallHistoryList::MaxCountGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::MaxCountGet);

	return m_unMaxCount;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::MaxCountSet
//
// Description: Sets the max size of the list
//
// Returns:
//	stiHResult
//
stiHResult CstiCallHistoryList::MaxCountSet (
	unsigned int unMax)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::MaxCountSet);

	m_unMaxCount = unMax;

	return stiRESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::NeedsUpdateGet
//
// Description: Do we need to update the entire list from Core?
//
// Returns:
//	bool
//
bool CstiCallHistoryList::NeedsUpdateGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::NeedsUpdateGet);

	return m_bNeedsUpdate;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::NeedsUpdateSet
//
// Description: Sets whether the list needs updating
//
// Returns:
//	void
//
void CstiCallHistoryList::NeedsUpdateSet (
	bool bUpdate)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::NeedsUpdateSet);

	m_bNeedsUpdate = bUpdate;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::NeedsNewItemsGet
//
// Description: Do we need to get just the latest items from Core?
// NOTE: There's a difference between "new" items and "unviewed" items.
// "New" items are defined as items we've been told about but haven't
// retrieved from Core yet.  "Unviewed" items are items we have in our list
// already but the user hasn't indicated them as viewed yet.
//
// Returns:
//	bool
//
bool CstiCallHistoryList::NeedsNewItemsGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::NeedsNewItemsGet);

	return m_bNeedsNewItems;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::NeedsNewItemsSet
//
// Description: Sets whether the list needs to get latest items
// NOTE: There's a difference between "new" items and "unviewed" items.
// "New" items are defined as items we've been told about but haven't
// retrieved from Core yet.  "Unviewed" items are items we have in our list
// already but the user hasn't indicated them as viewed yet.
//
// Returns:
//	void
//
void CstiCallHistoryList::NeedsNewItemsSet (
	bool bNew)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::NeedsNewItemsSet);

	m_bNeedsNewItems = bNew;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::NewItemCountGet
//
// Description: Gets the number of new items
//
// Returns:
//	int
//
unsigned int CstiCallHistoryList::NewItemCountGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::NewItemCountGet);

	return m_numNewItems;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::NewItemCountSet
//
// Description: Sets the number of new items
//
// Returns:
//	void
//
void CstiCallHistoryList::NewItemCountSet (
	unsigned int num)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::NewItemCountSet);

	m_numNewItems = std::min(num, m_unMaxCount);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::UnviewedItemCountGet
//
// Description: Gets the number of unviewed items
//
// Returns:
//	int
//
unsigned int CstiCallHistoryList::UnviewedItemCountGet () const
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::UnviewedItemCountGet);

	return m_numUnviewedItems;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::UnviewedItemCountSet
//
// Description: Sets the number of unviewed items
//
// Returns:
//	void
//
void CstiCallHistoryList::UnviewedItemCountSet (
	unsigned int num)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryList::UnviewedItemCountSet);

	m_numUnviewedItems = std::min(num, m_unMaxCount);
}


std::string CstiCallHistoryList::NameGet () const
{
	return XMLCallHistoryList;
}


void CstiCallHistoryList::NameSet (const std::string &name)
{
	stiASSERTMSG (false, "CstiCallHistoryList's xml element name is constant");
}


bool CstiCallHistoryList::NameMatch (const std::string &name)
{
	return XMLCallHistoryList == name;
}


time_t CstiCallHistoryList::LastRetrievalTimeGet ()
{
	return m_ttLastRetrievalTime;
}


void CstiCallHistoryList::Write (FILE *pFile)
{
	fprintf (pFile, "<%s>\n", XMLCallHistoryList);
	WriteField (pFile, std::to_string (m_eCallListType), XMLCallListType);
	WriteField (pFile, std::to_string (m_ttLastRetrievalTime), XMLLastRetrievalTime);

	fprintf (pFile, "  <%s>\n", XMLList);

	for (auto &item: m_List)
	{
		item->Write (pFile);
	}

	fprintf (pFile, "  </%s>\n", XMLList);
	fprintf (pFile, "</%s>\n", XMLCallHistoryList);
}


void CstiCallHistoryList::PopulateWithIXML_Node (IXML_Node *pNode)
{// pNode should point to "<CallHistoryList>" initially

	const char *pValue = nullptr;

	pNode = ixmlNode_getFirstChild (pNode);   // the first subtag

	while (pNode)
	{
		const char *pTagName = ixmlNode_getNodeName (pNode);
		IXML_Node *pChildNode = ixmlNode_getFirstChild (pNode);
		pValue = (const char *)ixmlNode_getNodeValue (pChildNode);

		if (strcmp (pTagName, XMLCallListType) == 0)
		{
			int list_type = ::atoi (pValue);
			m_eCallListType = (CstiCallList::EType)list_type;
		}
		else if (strcmp (pTagName, XMLList) == 0)
		{
			IXML_Node *pCallHistoryItemNode = pChildNode;
			while (pCallHistoryItemNode)
			{
				CstiCallHistoryItemSharedPtr item = std::make_shared<CstiCallHistoryItem>(pCallHistoryItemNode);
				item->CallListTypeSet (m_eCallListType);
				ItemAdd (item);
				pCallHistoryItemNode = ixmlNode_getNextSibling (pCallHistoryItemNode);
			}
		}
		else if (strcmp (pTagName, XMLLastRetrievalTime) == 0)
		{
			m_ttLastRetrievalTime = ::atol (pValue);
		}

		pNode = ixmlNode_getNextSibling (pNode);
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCallHistoryList::UnviewedItemCountCalculate
//
// Description: Counts how many items in the list are newer than lastViewedTime.
//
// Returns:
//	void
//
void CstiCallHistoryList::UnviewedItemCountCalculate (time_t lastViewedTime)
{
	m_numUnviewedItems = 0;

	for (auto &&pCallHistoryItem: m_List)
	{
		if (pCallHistoryItem->CallTimeGet () > lastViewedTime)
		{
			m_numUnviewedItems++;
		}
	}
}

