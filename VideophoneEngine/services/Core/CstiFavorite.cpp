/*!
 * \file CstiFavorite.cpp
 * \brief Defines an individual Favorite item
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2014 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#include "CstiFavorite.h"

using namespace WillowPM;

//
// Constants
//
const char XMLFavorite[] = "Favorite";
const char XMLPosition[] = "Position";
const char XMLIsContact[] = "IsContact";
const char XMLItemId[] = "ItemId";

CstiFavorite::CstiFavorite (IXML_Node *item)
{
	populate (item);
}

CstiItemId CstiFavorite::PhoneNumberIdGet () const
{
	return m_PhoneNumberId;
}

void CstiFavorite::PhoneNumberIdSet(
	const CstiItemId &PhoneNumberId)
{
	m_PhoneNumberId = PhoneNumberId;
}

bool CstiFavorite::IsContact () const
{
	return m_bIsContact;
}

void CstiFavorite::IsContactSet(
	bool bContact)
{
	m_bIsContact = bContact;
}

int CstiFavorite::PositionGet () const
{
	return m_nPosition;
}

void CstiFavorite::PositionSet(
	int nPos)
{
	m_nPosition = nPos;
}


void CstiFavorite::Write (FILE *file)
{
	fprintf (file, " <%s>\n", XMLFavorite);
	WriteField (file, std::to_string (m_nPosition), XMLPosition);
	WriteField (file, std::to_string (m_bIsContact ? 1 : 0), XMLIsContact);
	char itemId[255];
	m_PhoneNumberId.ItemIdGet (itemId, 255);
	WriteField (file, itemId, XMLItemId);
	fprintf (file, " </%s>\n", XMLFavorite);
}


void CstiFavorite::populate (IXML_Node *node)
{
	const char *value = nullptr;

	node = ixmlNode_getFirstChild (node);   // the first subtag

	while (node)
	{
		const char *tagName = ixmlNode_getNodeName (node);
		IXML_Node *childNode = ixmlNode_getFirstChild (node);
		value = (const char *)ixmlNode_getNodeValue (childNode);

		if (strcmp (tagName, XMLPosition) == 0)
		{
			m_nPosition = ::atoi (value);
		}
		else if (strcmp (tagName, XMLItemId) == 0)
		{
			m_PhoneNumberId.ItemIdSet (value);
		}
		else if (strcmp (tagName, XMLIsContact) == 0)
		{
			m_bIsContact = (bool)::atoi (value);
		}

		node = ixmlNode_getNextSibling (node);
	}
}


std::string CstiFavorite::NameGet () const
{
	return XMLFavorite;
}


void CstiFavorite::NameSet (const std::string &name)
{
	stiASSERTMSG (false, "CstiFavorite's xml element name is constant");
}


bool CstiFavorite::NameMatch (const std::string &name)
{
	return NameGet () == name;
}
