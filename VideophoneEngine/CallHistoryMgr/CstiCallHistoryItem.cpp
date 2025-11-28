	///
/// \file CstiCallHistoryItem.cpp
/// \brief Definition of the class that implements a single call history item.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2013 by Sorenson Communications, Inc. All rights reserved.
///


//
// Includes
//
#include "CstiCallHistoryItem.h"
#include "CstiCallHistoryList.h"

using namespace WillowPM;

//
// Constants
//
const char XMLCallHistoryItem[] = "CallHistoryItem";
const char XMLId[] = "Id";
const char XMLDialMethod[] = "DialMethod";
const char XMLDialString[] = "DialString";
const char XMLName[] = "Name";
const char XMLCallTime[] = "CallTime";
const char XMLCallPublicId[] = "CallPublicId";
const char XMLRingTone[] = "RingTone";
const char XMLInSpeedDial[] = "InSpeedDial";
const std::string XMLVrsCallId = "VrsCallId";
const std::string XMLAgentHistory = "AgentHistory";
const std::string XMLCallJoinUtc = "CallJoinUtc";


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

/**
 * \brief Constructor
 */
CstiCallHistoryItem::CstiCallHistoryItem (IXML_Node *pNode)
{
	stiLOG_ENTRY_NAME (CstiCallHistoryItem::CstiCallHistoryItem);

	PopulateWithIXML_Node (pNode);
}


/**
 * \brief Destructor
 */
CstiCallHistoryItem::~CstiCallHistoryItem ()
{
	stiLOG_ENTRY_NAME (CstiCallHistoryItem::~CstiCallHistoryItem);

	delete m_pGroupedItems;

} // end CstiCallHistoryItem::~CstiCallHistoryItem


/*!
 * \brief Assignment operator.
 * \param rhs Reference to a CstiCallHistoryItem to copy.
 * \return Reference to the new CstiCallHistoryItem filed with the data from rhs.
 */
CstiCallHistoryItem& CstiCallHistoryItem::operator = (const CstiCallHistoryItem &rhs)
{
	if (this != &rhs)
	{
		NameSet(rhs.NameGet());
		DialStringSet(rhs.DialStringGet());
		ItemIdSet(rhs.ItemIdGet());
		DialMethodSet(rhs.DialMethodGet());
		CallTimeSet(rhs.CallTimeGet());
		RingToneSet(rhs.RingToneGet());
		InSpeedDialSet(rhs.InSpeedDialGet());
		CallPublicIdSet(rhs.CallPublicIdGet());
		IntendedDialStringSet(rhs.IntendedDialStringGet());
		DurationSet(rhs.DurationGet());
		PhoneNumberTypeSet(rhs.PhoneNumberTypeGet());
		ImageIdSet(rhs.ImageIdGet());
		CallListTypeSet(rhs.CallListTypeGet());
		m_agentHistory = rhs.m_agentHistory;

		if (rhs.m_pGroupedItems)
		{
			for (unsigned int i = 0; i < rhs.m_pGroupedItems->CountGet(); i++)
			{
				GroupedItemAdd(rhs.m_pGroupedItems->ItemGet(i));
			}
		}
	}

	return *this;
}

/*!
 * \brief Assignment operator.
 * \param rhs Reference to a CstiCallHistoryItem to copy.
 * \return Reference to the new CstiCallHistoryItem filed with the data from rhs.
 */
CstiCallHistoryItem& CstiCallHistoryItem::operator = (const CstiCallListItem &rhs)
{
	NameSet(rhs.NameGet());
	DialStringSet(rhs.DialStringGet() ? rhs.DialStringGet() : "");
	ItemIdSet(rhs.ItemIdGet() ? rhs.ItemIdGet() : "");
	DialMethodSet(rhs.DialMethodGet());
	CallTimeSet(rhs.CallTimeGet());
	RingToneSet(rhs.RingToneGet());
	InSpeedDialSet(rhs.InSpeedDialGet());
	CallPublicIdSet(rhs.CallPublicIdGet() ? rhs.CallPublicIdGet() : "");
	IntendedDialStringSet(rhs.IntendedDialStringGet() ? rhs.IntendedDialStringGet() : "");
	DurationSet(rhs.DurationGet());
	vrsCallIdSet (rhs.vrsCallIdGet ());
	m_agentHistory = rhs.m_agentHistory;

	return *this;
}


/**
 * \brief Generates a CstiCallListItem object to send to Core
 * \return The Name value
 */
CstiCallListItemSharedPtr CstiCallHistoryItem::CoreItemCreate(bool bBlockedCallerID)
{
	CstiCallListItemSharedPtr item = std::make_shared<CstiCallListItem> ();

	item->NameSet(NameGet().c_str ());
	item->DialStringSet(DialStringGet().c_str ());
	item->ItemIdSet(ItemIdGet().c_str ());
	item->DialMethodSet(DialMethodGet());
	item->CallTimeSet(CallTimeGet());
	item->RingToneSet(RingToneGet());
	item->InSpeedDialSet(InSpeedDialGet());
	item->CallPublicIdSet(CallPublicIdGet().c_str ());
	item->IntendedDialStringSet(IntendedDialStringGet().c_str ());
	item->DurationSet(DurationGet());
	item->BlockedCallerIDSet(bBlockedCallerID ? estiTRUE : estiFALSE);
	item->vrsCallIdSet (vrsCallIdGet ());
	item->m_agentHistory = m_agentHistory;

	return item;
}

/**
 * \brief Adds an item to our group of similar items
 * \return
 */
void CstiCallHistoryItem::GroupedItemAdd(const CstiCallHistoryItemSharedPtr &pItem)
{
	if (!m_pGroupedItems)
	{
		// This is the first time we're adding an item so we need to create the list
		m_pGroupedItems = new CstiCallHistoryList(m_eCallListType);

		// We also need to add ourself to the list as the first entry
		CstiCallHistoryItemSharedPtr spThis = std::make_shared<CstiCallHistoryItem>();
		*spThis = *this;
		m_pGroupedItems->ItemAdd(spThis);
	}

	if (pItem->m_pGroupedItems)
	{
		// The item we're adding is already a group itself.
		// So just copy each item and then remove the old group.
		for (unsigned int i = 0; i < pItem->m_pGroupedItems->CountGet(); i++)
		{
			GroupedItemAdd(pItem->m_pGroupedItems->ItemGet(i));
		}

		pItem->GroupedItemsClear();
	}
	else
	{
		// Add the new item
		CstiCallHistoryItemSharedPtr spNew = std::make_shared<CstiCallHistoryItem>();
		*spNew = *pItem;
		m_pGroupedItems->ItemAdd(spNew);
	}
}

void CstiCallHistoryItem::GroupedItemsClear ()
{
	m_pGroupedItems->ListFree();

	delete m_pGroupedItems;
	m_pGroupedItems = nullptr;
}

CstiCallHistoryItemSharedPtr CstiCallHistoryItem::GroupedItemGet (unsigned int unIndex)
{
	CstiCallHistoryItemSharedPtr spItem;

	if (m_pGroupedItems)
	{
		spItem = m_pGroupedItems->ItemGet(unIndex);
	}

	return spItem;
}

unsigned int CstiCallHistoryItem::GroupedItemsCountGet ()
{
	unsigned int unCount = 0;

	if (m_pGroupedItems)
	{
		unCount = m_pGroupedItems->CountGet();
	}

	return unCount;
}

/**
 * \brief Gets the CallListItem name value
 * \return The Name value
 */
std::string CstiCallHistoryItem::NameGet () const
{
	return m_Name;
}

/**
 * \brief Sets the CallListItem name value
 * \param pszName Name value
 */
void CstiCallHistoryItem::NameSet (const std::string &name)
{
	m_Name = name;
}

/**
 * \brief Gets the CallListItem DialString value
 * \return The dial string value
 */
std::string CstiCallHistoryItem::DialStringGet () const
{
	return m_DialString;
}

/**
 * \brief Sets the CallListItem DialString value
 * \param pszDialString The dial string value
 */
void CstiCallHistoryItem::DialStringSet (const std::string &dialString)
{
	m_DialString = dialString;
}

/**
 * \brief Gets the CallListItem ItemId value
 * \return The item id value
 */
std::string CstiCallHistoryItem::ItemIdGet () const
{
	return m_ItemId;
}

/**
 * \brief Sets the CallListItem ItemId value
 * \param pszDialString The item id value
 */
void CstiCallHistoryItem::ItemIdSet (const std::string &itemId)
{
	m_ItemId = itemId;
}

/**
 * \brief Gets the CallListItem DialMethod value
 * \return The dial method
 */
EstiDialMethod CstiCallHistoryItem::DialMethodGet () const
{
	return m_eDialMethod;
}

/**
 * \brief Sets the CallListItem DialMethod value
 * \param eDialMethod The dial method value
 */
void CstiCallHistoryItem::DialMethodSet (EstiDialMethod eDialMethod)
{
	m_eDialMethod = eDialMethod;
}

/**
 * \brief Gets the CallListItem call time value
 * \return The ttCallTime
 */
time_t CstiCallHistoryItem::CallTimeGet () const
{
	return m_ttCallTime;
}

/**
 * \brief Sets the CallListItem call time value
 * \param ttCallTime The call time value
 */
void CstiCallHistoryItem::CallTimeSet (time_t ttCallTime)
{
	m_ttCallTime = ttCallTime;
}

/**
 * \brief Gets the CallListItem RingTone value
 * \return The RingTone
 */
int CstiCallHistoryItem::RingToneGet () const
{
	return m_nRingTone;
}

/**
 * \brief Sets the CallListItem RingTone value
 * \param nRingTone The ring tone value
 */
void CstiCallHistoryItem::RingToneSet (int nRingTone)
{
	m_nRingTone = nRingTone;
}

/**
 * \brief Gets the InSpeedDial setting
 * \return estiTRUE If InSpeedDial, otherwise estiFALSE
 */
EstiBool CstiCallHistoryItem::InSpeedDialGet () const
{
	return m_bInSpeedDial;
}

/**
 * \brief Sets the InSpeedDial setting
 * \param bInSpeedDial The boolean InSpeedDial value
 */
void CstiCallHistoryItem::InSpeedDialSet (EstiBool bInSpeedDial)
{
	m_bInSpeedDial = bInSpeedDial;
}


/**
 * \brief Gets the CallPublicId
 * \return The CallPublicId
 */
std::string CstiCallHistoryItem::CallPublicIdGet () const
{
	return m_CallPublicId;
}

/**
 * \brief Sets the CallPublicId
 * \param pszCallPublicId The CallPublicId value
 */
void CstiCallHistoryItem::CallPublicIdSet (const std::string &callPublicId)
{
	m_CallPublicId = callPublicId;
}


/**
 * \brief Gets the IntendedDialString
 * \return The IntendedDialString
 */
std::string CstiCallHistoryItem::IntendedDialStringGet () const
{
	return m_IntendedDialString;
}

/**
 * \brief Sets the IntendedDialString
 */
void CstiCallHistoryItem::IntendedDialStringSet (const std::string &intendedDialString)
{
	m_IntendedDialString = intendedDialString;
}


/*!
 * \brief Sets the duration
 */
void CstiCallHistoryItem::DurationSet (
	int nDuration)		///< Duration in seconds
{
	m_nDuration = nDuration;
}


/*!
 * \brief Gets the duration
 */
int CstiCallHistoryItem::DurationGet () const
{
	return m_nDuration;
}

/**
 * \brief Gets the CallListItem VRS Call ID value
 * \return The VRS Call ID value
 */
std::string CstiCallHistoryItem::vrsCallIdGet () const
{
	return m_vrsCallId;
}

/**
 * \brief Sets the CallListItem VRS Call ID value
 * \param vrsCallId VRS Call ID value
 */
void CstiCallHistoryItem::vrsCallIdSet (const std::string &vrsCallId)
{
	m_vrsCallId = vrsCallId;
}

/**
 * \brief Gets the CstiCallHistoryItem Agent ID value
 * \return The AgentId value
 */
std::string CstiCallHistoryItem::agentIdGet ()
{
	if (m_agentHistory.empty ())
	{
		return std::string ();
	}

	std::sort (m_agentHistory.begin (), m_agentHistory.end (),
		[](const AgentHistoryItem &lhs, const AgentHistoryItem &rhs)
		{
			return lhs.m_callJoinUtc > rhs.m_callJoinUtc;
		}
	);

	return m_agentHistory.begin ()->m_agentId;
}

/*!
 * \brief Sets the phone number type
 */
void CstiCallHistoryItem::PhoneNumberTypeSet (
	int nType)		///< Phone number type
{
	m_nNumberType = nType;
}


/*!
 * \brief Gets the phone number type
 */
int CstiCallHistoryItem::PhoneNumberTypeGet () const
{
	return m_nNumberType;
}

/**
 * \brief Gets the Image ID
 * \return The Image ID
 */
std::string CstiCallHistoryItem::ImageIdGet () const
{
	return m_ImageId;
}

/**
 * \brief Sets the Image ID
 */
void CstiCallHistoryItem::ImageIdSet (const std::string &ImageId)
{
	m_ImageId = ImageId;
}

/**
 * \brief Gets the type of Call List this item belongs to
 * \return The list type
 */
CstiCallList::EType CstiCallHistoryItem::CallListTypeGet () const
{
	return m_eCallListType;
}

/**
 * \brief Sets the type of Call List this item belongs to
 */
void CstiCallHistoryItem::CallListTypeSet (CstiCallList::EType eType)
{
	m_eCallListType = eType;
}

/**
 * \brief Gets the BlockedCallerID setting
 * \return estiTRUE If BlockedCallerID, otherwise estiFALSE
 */
EstiBool CstiCallHistoryItem::BlockedCallerIDGet () const
{
	return m_bBlockedCallerID;
}

/**
 * \brief Sets the BlockedCallerID setting
 * \param bBlockedCallerID The boolean BlockedCallerID value
 */
void CstiCallHistoryItem::BlockedCallerIDSet (EstiBool bBlockedCallerID)
{
	m_bBlockedCallerID = bBlockedCallerID;
}

int CstiCallHistoryItem::callIndexGet () const
{
	return m_callIndex;
}

void CstiCallHistoryItem::callIndexSet (int value)
{
	m_callIndex = value;
}


void CstiCallHistoryItem::Write (FILE *pFile)
{
	fprintf (pFile, "    <%s>\n", XMLCallHistoryItem);
	WriteField (pFile, m_ItemId, XMLId);
	WriteField (pFile, std::to_string (m_eDialMethod), XMLDialMethod);
	WriteField (pFile, m_DialString, XMLDialString);
	WriteField (pFile, m_Name, XMLName);
	WriteField (pFile, std::to_string (m_ttCallTime), XMLCallTime);
	WriteField (pFile, m_CallPublicId, XMLCallPublicId);
	WriteField (pFile, std::to_string (m_nRingTone), XMLRingTone);
	WriteField (pFile, std::to_string (m_bInSpeedDial), XMLInSpeedDial);
	if (!m_vrsCallId.empty ())
	{
		WriteField (pFile, m_vrsCallId, XMLVrsCallId);
	}
	if (!m_agentHistory.empty ())
	{
		fprintf (pFile, "      <%s>\n", XMLAgentHistory.c_str());
		for (auto& item : m_agentHistory)
		{
			WriteElement (pFile, item);
		}
		fprintf (pFile, "      </%s>\n", XMLAgentHistory.c_str ());
	}
	fprintf (pFile, "    </%s>\n", XMLCallHistoryItem);
}

void CstiCallHistoryItem::WriteElement (FILE * pFile, const AgentHistoryItem & item)
{
	std::stringstream ss;
	ss << "        <Agent " << XMLId << "=\"" << item.m_agentId << "\" " << XMLCallJoinUtc << "=\"" << item.m_callJoinUtc << "\"/>\n";
	fprintf (pFile, "%s", ss.str().c_str());
}


void CstiCallHistoryItem::PopulateWithIXML_Node (IXML_Node *pNode)
{// pNode should point to "<CallHistoryItem>" initially

	const char *pValue = nullptr;

	pNode = ixmlNode_getFirstChild (pNode);   // the first subtag

	while (pNode)
	{
		const char *pTagName = ixmlNode_getNodeName (pNode);
		auto child = ixmlNode_getFirstChild (pNode);
		pValue = (const char *)ixmlNode_getNodeValue (child);

		if (pValue)
		{
			if (strcmp (pTagName, XMLId) == 0)
			{
				ItemIdSet (pValue);
			}
			else if (strcmp (pTagName, XMLDialMethod) == 0)
			{
				int dial_type = ::atoi (pValue);
				DialMethodSet ((EstiDialMethod)dial_type);
			}
			else if (strcmp (pTagName, XMLDialString) == 0)
			{
				DialStringSet (pValue);
			}
			else if (strcmp (pTagName, XMLName) == 0)
			{
				NameSet (pValue);
			}
			else if (strcmp (pTagName, XMLCallTime) == 0)
			{
				long time = ::atol (pValue);
				CallTimeSet (time);
			}
			else if (strcmp (pTagName, XMLCallPublicId) == 0)
			{
				CallPublicIdSet (pValue);
			}
			else if (strcmp (pTagName, XMLRingTone) == 0)
			{
				int ringTone = ::atoi (pValue);
				RingToneSet (ringTone);
			}
			else if (strcmp (pTagName, XMLInSpeedDial) == 0)
			{
				int inPhonebook = ::atoi (pValue);
				InSpeedDialSet ((EstiBool)inPhonebook);
			}
		}
		else if (pTagName == XMLVrsCallId)
		{
			m_vrsCallId = pValue;
		}
		else if (pTagName == XMLAgentHistory)
		{
			while (child != nullptr)
			{
				std::string id;
				time_t callJoinUtc {};
				for (auto attr = ixmlNode_getAttributes (child); attr != nullptr; attr = attr->next)
				{
					std::string nodeName = ixmlNode_getNodeName (attr->nodeItem);
					std::string nodeValue = ixmlNode_getNodeValue (attr->nodeItem);
					if (XMLId == nodeName)
					{
						id = nodeValue;
					}
					else if (XMLCallJoinUtc == nodeName)
					{
						callJoinUtc = std::stoll (nodeValue);
					}
				}
				if (!id.empty ())
				{
					m_agentHistory.emplace_back (id, callJoinUtc);
				}
				child = ixmlNode_getNextSibling (child);
			}
		}

		pNode = ixmlNode_getNextSibling (pNode);
	}
}


bool CstiCallHistoryItem::NameMatch(const std::string &name)
{
	return (m_Name == name);
}
// end file CstiCallHistoryItem.cpp
