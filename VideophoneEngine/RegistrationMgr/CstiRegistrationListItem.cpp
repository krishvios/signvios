///e
/// \file CstiRegistrationList.cpp
/// \brief Implements a list of registration items
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2016 by Sorenson Communications, Inc. All rights reserved.
///

#include "CstiRegistrationListItem.h"

///
///\brief Return the Registration type from the XML object
///
EstiProtocol CstiRegistrationListItem::RegistrationType (IXML_Node * pNode)
{
	EstiProtocol eType;
	const char *pszValue = nullptr;
	pszValue = ixmlElement_getAttribute ((IXML_Element *)pNode, (DOMString)"Type");
	if (0 == stiStrICmp (pszValue, "sip"))
	{
		eType = estiSIP;
	}
	else
	{
		eType = estiPROT_UNKNOWN;
		stiASSERTMSG (estiFALSE, "CstiRegistrationListItem: Unknown Protocol Format %s", pszValue);
	}
	return eType;
}

///
///\brief Constructor for a Registration Item
///
CstiRegistrationListItem::CstiRegistrationListItem (EstiProtocol eType) : 
	m_eType(eType)
{
}

///
///\brief Function that turns IXML into a Registration Object Data
///
stiHResult CstiRegistrationListItem::XMLNodeProcess (IXML_Node * pNode, EstiProtocol eType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	//Grab the XML type of this RegistrationList
	const char* nodeName = nullptr;
	const char *pszValue = nullptr;
	IXML_Node *pNodeIterator = nullptr;
	IXML_Node *pTextNode = nullptr;

	m_eType = eType;
	//Process a Registration XML Element found in RegistrationInfoGetResult
	pNodeIterator = ixmlNode_getFirstChild (pNode);
	while (pNodeIterator)
	{
		nodeName = ixmlNode_getNodeName (pNodeIterator);
		if (stricmp (nodeName, XMLPublicDomain) == 0)
		{
			pTextNode = ixmlNode_getFirstChild (pNodeIterator);
			pszValue = ixmlNode_getNodeValue (pTextNode);
			if (pszValue)
			{
				m_sRegInfo.PublicDomain = pszValue;
			}
		}
		else if (stricmp (nodeName, XMLPrivateDomain) == 0)
		{
			pTextNode = ixmlNode_getFirstChild (pNodeIterator);
			pszValue = ixmlNode_getNodeValue (pTextNode);
			if (pszValue)
			{
				m_sRegInfo.PrivateDomain = pszValue;
			}
		}
		else if (stricmp (nodeName, XMLCredentialList) == 0)
		{
			//We found a credential list and now we need to parse through it.
			IXML_Node *pCredentialListIterator = ixmlNode_getFirstChild (pNodeIterator);
			while (pCredentialListIterator)
			{
				const char* psUserName = nullptr;
				const char* psPassword = nullptr;
				IXML_Node *pCredentialsIterator = ixmlNode_getFirstChild (pCredentialListIterator);
				while (pCredentialsIterator)
				{
					nodeName = ixmlNode_getNodeName (pCredentialsIterator);
					if (stricmp (nodeName, XMLUsername) == 0)
					{
						pTextNode = ixmlNode_getFirstChild (pCredentialsIterator);
						psUserName = ixmlNode_getNodeValue (pTextNode);
					}
					else if (stricmp (nodeName, XMLPassword) == 0)
					{
						pTextNode = ixmlNode_getFirstChild (pCredentialsIterator);
						psPassword = ixmlNode_getNodeValue (pTextNode);
					}
					else
					{
						stiASSERTMSG (estiFALSE, "CstiRegistrationListItem: Unknown Credential XML Element (%s)", nodeName);
					}
					pCredentialsIterator = ixmlNode_getNextSibling (pCredentialsIterator);
				}
				// Put this SIP credential into the list
				if (psUserName != nullptr && psPassword != nullptr)
				{
					m_sRegInfo.PasswordMap[psUserName] = psPassword;
				}
				//Get the Next CredentialListItem
				pCredentialListIterator = ixmlNode_getNextSibling (pCredentialListIterator);
			}
		}
		else
		{
			stiASSERTMSG (estiFALSE, "CstiRegistrationListItem: Unknown Xml Element (%s)", nodeName);
		}
		pNodeIterator = ixmlNode_getNextSibling (pNodeIterator);
	}
	return hResult;
}


///
///\brief Write the Registration Element Data to the XML file
///
void CstiRegistrationListItem::Write (FILE *pFile)
{
	switch (m_eType)
	{
	case estiSIP:
		fprintf (pFile, "  <%s Type=\"%s\">\n", XMLRegistrationListItem, XMLSIP);
		break;
	case estiSIPS:
	case estiPROT_UNKNOWN:
		fprintf (pFile, "  <%s Type=\"%s\">\n", XMLRegistrationListItem, XMLUNKNOWN);
		break;
	}

	WriteField (pFile, m_sRegInfo.PublicDomain, XMLPublicDomain);
	WriteField (pFile, m_sRegInfo.PrivateDomain, XMLPrivateDomain);

	fprintf (pFile, "    <%s>\n", XMLCredentialList);

	for (auto it = m_sRegInfo.PasswordMap.begin (); it != m_sRegInfo.PasswordMap.end (); it++)
	{
		fprintf (pFile, "      <%s>\n", XMLCredentialListItem);
		WriteField (pFile, (*it).first, XMLUsername);
		WriteField (pFile, (*it).second, XMLPassword);
		fprintf (pFile, "      </%s>\n", XMLCredentialListItem);
	}

	fprintf (pFile, "    </%s>\n", XMLCredentialList);
	fprintf (pFile, "  </%s>\n", XMLRegistrationListItem);
}

///
///\brief Grabs the name of the XMLElement
///
std::string CstiRegistrationListItem::NameGet () const
{
	return XMLRegistrationListItem;
}

///
///\brief This should never happen because the XML Element name is a constant
///
void CstiRegistrationListItem::NameSet (const std::string &name)
{
	stiASSERTMSG (false, "CstiRegistrationListItems's xml element name is constant");
}

///
///\brief Matches the XMLElement name
///
bool CstiRegistrationListItem::NameMatch (const std::string &name)
{
	return XMLRegistrationListItem == name;
}

///
///\brief Returns the Registration Protocol associated with this registration
///
EstiProtocol CstiRegistrationListItem::TypeGet ()
{
	return m_eType;
}

///
///\brief Returns a pointer to the registration info contained in this class
///
SstiRegistrationInfo CstiRegistrationListItem::RegistrationInfoGet () const
{
	return m_sRegInfo;
}

///
///\brief Checks for an exact match of the registration list items.
///
bool CstiRegistrationListItem::RegistrationMatch (SstiRegistrationInfo* pRegistrationData)
{
	return pRegistrationData->PasswordMap.size () == m_sRegInfo.PasswordMap.size () &&
	std::equal (pRegistrationData->PasswordMap.begin (), pRegistrationData->PasswordMap.end (), m_sRegInfo.PasswordMap.begin ()) &&
	m_sRegInfo.PublicDomain == pRegistrationData->PublicDomain &&
	m_sRegInfo.PrivateDomain == pRegistrationData->PrivateDomain;
}

///
///\Update the registration data with the provided data
///
void CstiRegistrationListItem::RegistrationUpdate (SstiRegistrationInfo * pRegistrationData)
{
	m_sRegInfo.PublicDomain = pRegistrationData->PublicDomain;
	m_sRegInfo.PrivateDomain = pRegistrationData->PrivateDomain;
	m_sRegInfo.PasswordMap.clear ();

	auto iterCurrent = pRegistrationData->PasswordMap.begin ();
	for (; iterCurrent != pRegistrationData->PasswordMap.end (); iterCurrent++)
	{
		m_sRegInfo.PasswordMap[(*iterCurrent).first] = (*iterCurrent).second;
	}
}

///
///\brief Checks if the credentials provided match any in the credentials list.
///
bool CstiRegistrationListItem::CredentialsCheck (const char& Username, const char& Pin)
{
	auto iterCurrent = m_sRegInfo.PasswordMap.find (&Username);
	return iterCurrent != m_sRegInfo.PasswordMap.end () && stricmp (iterCurrent->second.c_str (), &Pin) == 0;
}

///
///\brief Removes the registration data of this object.
///
void CstiRegistrationListItem::RegistrationDataClear ()
{
	m_sRegInfo.PublicDomain.clear ();
	m_sRegInfo.PrivateDomain.clear ();
	m_sRegInfo.PasswordMap.clear ();
}
