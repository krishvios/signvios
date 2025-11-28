#include "CstiRegistrationList.h"


///
///\brief Constructor that turns IXML into a List of Registrations (currently only a Sip Registration Object)
///
CstiRegistrationList::CstiRegistrationList () :
	m_SipRegistration (estiSIP)
{
	memset (&m_sPhoneNumbers, 0, sizeof (m_sPhoneNumbers));
}


///
///\brief Write the Registration Elements to the Root object of the XML file
///
void CstiRegistrationList::Write (FILE *pFile)
{
	fprintf (pFile, "<%s>\n", XMLRegistrationList);
	fprintf (pFile, "  <%s>\n", XMLUserNumbers);
	WriteField (pFile, m_sPhoneNumbers.szLocalPhoneNumber, XMLLocalPhoneNumber);
	WriteField (pFile, m_sPhoneNumbers.szTollFreePhoneNumber, XMLTollFreePhoneNumber);
	WriteField (pFile, m_sPhoneNumbers.szRingGroupLocalPhoneNumber, XMLRingGroupLocalPhoneNumber);
	WriteField (pFile, m_sPhoneNumbers.szRingGroupTollFreePhoneNumber, XMLRingGroupTollFreePhoneNumber);
	WriteField (pFile, m_sPhoneNumbers.szSorensonPhoneNumber, XMLSorensonPhoneNumber);
	fprintf (pFile, "  </%s>\n", XMLUserNumbers);
	//This is a list of one for now.
	m_SipRegistration.Write (pFile);
	fprintf (pFile, "</%s>\n", XMLRegistrationList);
}

///
///\brief Grabs the name of the XMLElement
/// this is actually wrong because there is no XML element for this RegistrationList
/// however if there was an XML element at this level it would have this name.
///
std::string CstiRegistrationList::NameGet () const
{
	return XMLRegistrationList;
}

///
///\brief This should never happen because the XML Element name is a constant
///
void CstiRegistrationList::NameSet (const std::string &name)
{
	stiASSERTMSG (false, "CstiRegistrationList's xml element name is constant");
}

///
///\brief Matches the XMLElement name, for property table use only (does not have a purpose here)
///
bool CstiRegistrationList::NameMatch (const std::string &name)
{
	return XMLRegistrationList == name;
}

///
///\brief Gets the SipRegistration object
///
SstiRegistrationInfo CstiRegistrationList::SipRegistrationInfoGet () const
{
	return m_SipRegistration.RegistrationInfoGet();
}

///
///\Compares the SipRegistration List with another SipRegistrationList for a Match
///
bool CstiRegistrationList::RegistrationsMatch (SstiRegistrationInfo* registrationList)
{	
	//They are both NULL or non of them are NULL and they match
	return registrationList != nullptr && m_SipRegistration.RegistrationMatch (registrationList);
}

///
///\Updates the current Registration with new values
///
void CstiRegistrationList::RegistrationsUpdate (SstiRegistrationInfo* pRegistrationData)
{
	m_SipRegistration.RegistrationUpdate (pRegistrationData);
}

///
///\Checks the Credentials in the SipRegistration for a match to the passed in values.
///
bool CstiRegistrationList::CredentialsCheck (const char& Username, const char& Pin)
{
	return m_SipRegistration.CredentialsCheck (Username, Pin);
}

///
///\brief Sets the UserNumber data to be saved in the XML
///
stiHResult CstiRegistrationList::PhoneNumbersSet (const SstiUserPhoneNumbers * pPhoneNumbers)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	memcpy (&m_sPhoneNumbers, pPhoneNumbers, sizeof (m_sPhoneNumbers));
	return hResult;
}

///
///\brief Returns the UserNumber data by placing it in the passed in SstiUserPhoneNumbers 
///
stiHResult CstiRegistrationList::PhoneNumbersGet (SstiUserPhoneNumbers * pPhoneNumbers) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	memcpy (pPhoneNumbers, &m_sPhoneNumbers, sizeof (m_sPhoneNumbers));
	return hResult;
}

///
///\brief Processe the XML list and places it into the class data
///
stiHResult CstiRegistrationList::XMLListProcess (IXML_NodeList *pNodeList)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	memset (&m_sPhoneNumbers, 0, sizeof (m_sPhoneNumbers));
	const char* nodeName = nullptr;
	IXML_Node *pNodeIterator = nullptr;
	IXML_Node *pTextNode = nullptr;
	IXML_Node *pUserNumberNodeIterator = nullptr;

	//Process a Registration XML Element found in RegistrationInfoGetResult
	pNodeIterator = ixmlNodeList_item (pNodeList, 0);

	while (pNodeIterator)
	{
		nodeName = ixmlNode_getNodeName (pNodeIterator);
		if (stricmp (nodeName, XMLRegistration) == 0)
		{
			EstiProtocol protocol = CstiRegistrationListItem::RegistrationType (pNodeIterator);
			switch (protocol)
			{
			case estiSIP:
				m_SipRegistration.XMLNodeProcess (pNodeIterator, protocol);
				break;
			case estiPROT_UNKNOWN:
			case estiSIPS:
				//These registration types are not implemented
				break;
			}
		}
		else if (stricmp (nodeName, XMLUserNumbers) == 0)
		{
			pUserNumberNodeIterator = nullptr;
			pUserNumberNodeIterator = ixmlNode_getFirstChild (pNodeIterator);

			while (pUserNumberNodeIterator)
			{
				nodeName = ixmlNode_getNodeName (pUserNumberNodeIterator);
				pTextNode = ixmlNode_getFirstChild (pUserNumberNodeIterator);
				if (pTextNode)
				{
					const char * nodeValue = ixmlNode_getNodeValue (pTextNode);
					if (nodeValue)
					{
						if (stricmp (nodeName, XMLLocalPhoneNumber) == 0)
						{
							strcpy (m_sPhoneNumbers.szLocalPhoneNumber, nodeValue);
						}
						else if (stricmp (nodeName, XMLTollFreePhoneNumber) == 0)
						{
							strcpy (m_sPhoneNumbers.szTollFreePhoneNumber, nodeValue);
						}
						else if (stricmp (nodeName, XMLRingGroupLocalPhoneNumber) == 0)
						{
							strcpy (m_sPhoneNumbers.szRingGroupLocalPhoneNumber, nodeValue);
						}
						else if (stricmp (nodeName, XMLRingGroupTollFreePhoneNumber) == 0)
						{
							strcpy (m_sPhoneNumbers.szRingGroupTollFreePhoneNumber, nodeValue);
						}
						else if (stricmp (nodeName, XMLSorensonPhoneNumber) == 0)
						{
							strcpy (m_sPhoneNumbers.szSorensonPhoneNumber, nodeValue);
						}
					}
				}
				pUserNumberNodeIterator = ixmlNode_getNextSibling (pUserNumberNodeIterator);
			}
		}
		pNodeIterator = ixmlNode_getNextSibling (pNodeIterator);
	}
	return hResult;
}


///
///\brief Clears the Registration Data
///
void CstiRegistrationList::ClearRegistrations ()
{
	m_SipRegistration.RegistrationDataClear ();
}
