///
/// \file CstiRegistrationManager.h
/// \brief See CstiRegistrationManager.cpp
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2016 by Sorenson Communications, Inc. All rights reserved.
///

#include "CstiRegistrationManager.h"

///
///\brief Constructor for the Registration Manager
///
CstiRegistrationManager::CstiRegistrationManager ()
	:
	
	m_Registrations(std::make_shared<CstiRegistrationList> ())
{
	std::string DynamicDataFolder;
	stiOSDynamicDataFolderGet (&DynamicDataFolder);
	std::stringstream registrationDataFile;
	registrationDataFile << DynamicDataFolder << XMLRegistrationFileName;
	SetFilename (registrationDataFile.str ());
}

///
///\brief Destructor for the Registration Manager
///
CstiRegistrationManager::~CstiRegistrationManager ()
{
	m_pdoc->ItemDestroy (0);
	m_pdoc->ListFree ();
}

///
///\brief Initializes the XMLManager
///
stiHResult CstiRegistrationManager::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	lock ();
		XMLManager::init ();
		if (m_pdoc->CountGet () == 0)
		{
			m_pdoc->ItemAdd (m_Registrations);
		}
	unlock ();

	return hResult;
}

///
///\brief Loads the XML from file and places it into m_pRegistrations
///
WillowPM::XMLListItemSharedPtr CstiRegistrationManager::NodeToItemConvert (IXML_Node *pNode)
{
	lock ();
		IXML_NodeList* pNodeList = ixmlNode_getChildNodes (pNode);
		m_Registrations->XMLListProcess (pNodeList);
		ixmlNodeList_free (pNodeList);
	unlock ();
	return m_Registrations;
}

///
///\brief Set the UserPhoneNumbers from AccountLogin
///
stiHResult CstiRegistrationManager::PhoneNumbersSet (SstiUserPhoneNumbers * pPhoneNumbers)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	lock ();
		m_Registrations->PhoneNumbersSet (pPhoneNumbers);
		startSaveDataTimer ();
	unlock ();
	return hResult;
}

///
///\brief Set the UserPhoneNumbers in the passed in value
///
stiHResult CstiRegistrationManager::PhoneNumbersGet (SstiUserPhoneNumbers * pPhoneNumbers)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	lock ();
		m_Registrations->PhoneNumbersGet (pPhoneNumbers);
	unlock ();
	return hResult;
}

///
///\brief Get the SstiRegistrationInfo for the ConferenceParamaters struct.
///
SstiRegistrationInfo CstiRegistrationManager::SipRegistrationInfoGet ()
{
	lock ();
		SstiRegistrationInfo RegistrationInfoResults = m_Registrations->SipRegistrationInfoGet ();
	unlock ();
	return RegistrationInfoResults;
}

///
///\brief Check if the response registrations match our current set of registrations and returns whether the
/// Registrations were updated
///
bool CstiRegistrationManager::RegistrationsUpdate (CstiCoreResponse*  coreResponse)
{
	bool results = false;  //We have not updated our registration
	std::vector <CstiCoreResponse::SRegistrationInfo>::const_iterator i;
	std::vector <CstiCoreResponse::SRegistrationCredentials>::const_iterator j;
	std::vector <CstiCoreResponse::SRegistrationInfo> coreRegistrationInfo;
	coreResponse->RegistrationInfoListGet (&coreRegistrationInfo);
	lock ();
	for (i = coreRegistrationInfo.begin (); i != coreRegistrationInfo.end (); i++)
	{
		SstiRegistrationInfo RegistrationInfoResults;
		RegistrationInfoResults.PublicDomain = (*i).PublicDomain;
		RegistrationInfoResults.PrivateDomain = (*i).PrivateDomain;
		RegistrationInfoResults.PasswordMap.clear ();

		for (j = (*i).Credentials.begin (); j != (*i).Credentials.end (); j++)
		{
			RegistrationInfoResults.PasswordMap[(*j).UserName] = (*j).Password;
		}
		switch ((*i).eType)
		{
			case estiSIP:
				if (!m_Registrations->RegistrationsMatch (&RegistrationInfoResults))
				{
					m_Registrations->RegistrationsUpdate (&RegistrationInfoResults);
					results = true;
				}
				break;
			case estiPROT_UNKNOWN:
			case estiSIPS:
				//We currently don't use these types so they are ignored
				break;
		}
	}
	if (results)
	{
		startSaveDataTimer ();
	}
	unlock ();
	return results;
}

///
///\brief Check if the username and pin are in our credentials list
///
bool CstiRegistrationManager::CredentialsCheck (const char& Username, const char& Pin)
{
	bool results;
	MD5_CTX mdc;
	uint8_t digest[18];
	char strResponse[34];
	MD5_Init (&mdc);
	MD5_Update (&mdc, &Pin, strlen (&Pin));
	MD5_Final ((unsigned char*)digest, &mdc);
	// Changes the digest into a string format.
	for (unsigned int i = 0; i < 16; i++)
	{
		sprintf (&strResponse[i * 2], "%02x", digest[i]);
	}
	lock ();
		results = m_Registrations->CredentialsCheck (Username, (*strResponse));
	unlock ();
	return results;
}

stiHResult CstiRegistrationManager::ClearRegistrations ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	lock ();
	m_Registrations->ClearRegistrations ();
	startSaveDataTimer ();
	unlock ();
	return hResult;
}
