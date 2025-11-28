///
/// \file CstiCoreResponse.cpp
/// \brief Definition of the core response class.
///
/// This class has the enumerations, defines, and functions that help in processing core responses.
///
/// \note The "VP Services API" documentation on the Engineering website contains the official
///  documentation for core requests and responses, including error codes.  That documentation
///  should be used for the most up-to-date API information.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///


//
// Includes
//

#include "CstiCoreResponse.h"

#ifdef stiFAVORITES
#include "SstiFavoriteList.h"
#endif

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
// Locals
//

//
// Function Declarations
//

/**
 * \brief Constructor
 * 
 * \param unRequestID The request ID
 * \param eResponse The Response enumeration
 * \param eResponseResult The response result
 * \param eError The error number (if result was an error)
 * \param pszError The error string
 * \param pContext The context pointer
 */
CstiCoreResponse::CstiCoreResponse (
	CstiVPServiceRequest *request,
	EResponse eResponse,
	EstiResult eResponseResult,
	ECoreError eError,
	const char *pszError)
:
	CstiVPServiceResponse(request, eResponseResult, pszError),
	m_eResponse (eResponse),
	m_eError (eError),
	m_poCallList (nullptr),
	m_poContactList (nullptr),
#ifdef stiFAVORITES
	m_poFavoriteList (nullptr),
#endif
	m_poUserAccountInfo (nullptr),
	m_poDirectoryResolveResult (nullptr)
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::CstiCoreResponse);

	// Clear the contents of the relayLanguageList
	m_RelayLanguageList.clear ();
	
	// Clear the contents of the ringGroupInfoList
	m_RingGroupInfoList.clear ();

	// Clear the contents of the RegistrationInfoList
	m_RegistrationInfoList.clear ();

	// Clear the contents of the AgreementList
	m_AgreementList.clear ();

} // end CstiCoreResponse::CstiCoreResponse


/**
 * \brief Destructor
 */
CstiCoreResponse::~CstiCoreResponse ()
{
	stiLOG_ENTRY_NAME (CstiCoreResponse::~CstiCoreResponse);

	// Was a call list allocated?
	if (m_poCallList != nullptr)
	{
		// Yes! Free it now.
		delete m_poCallList;
		m_poCallList = nullptr;
	} // end if

	// Was a call list allocated?
	if (m_poContactList != nullptr)
	{
		// Yes! Free it now.
		delete m_poContactList;
		m_poContactList = nullptr;
	} // end if

#ifdef stiFAVORITES
	if (m_poFavoriteList != nullptr)
	{
		delete m_poFavoriteList;
		m_poFavoriteList = nullptr;
	}
#endif

	// Is there any user account info?
	if (m_poUserAccountInfo != nullptr)
	{
		// Yes! Free it now.
		delete m_poUserAccountInfo;
		m_poUserAccountInfo = nullptr;
	} // end if

	// Is there a directory resolve result?
	if (m_poDirectoryResolveResult != nullptr)
	{
		// Yes! Free it now.
		delete m_poDirectoryResolveResult;
		m_poDirectoryResolveResult = nullptr;
	} // end if
} // end CstiCoreResponse::~CstiCoreResponse


/**
 * \brief Retrieves the relay language list from the response object.
 * \param pRelayLanguageList Pointer to the retrieved language list
 */
void CstiCoreResponse::RelayLanguageListGet (
	std::vector <SRelayLanguage> *pRelayLanguageList) const
{
	*pRelayLanguageList = m_RelayLanguageList;
}


/**
 * \brief Use this method to set the language list to the response object.
 * \param pRelayLanguageList Pointer to language list to set
 */
void CstiCoreResponse::RelayLanguageListSet (
	const std::vector <SRelayLanguage> *pRelayLanguageList)
{
	m_RelayLanguageList = *pRelayLanguageList;
}


/**
 * \brief Retrieves the Ring Group Info list from the response object.
 * \param pRingGroupInfoList Pointer to the retrieved Ring Group Info list
 */
void CstiCoreResponse::RingGroupInfoListGet (
	std::vector <SRingGroupInfo> *pRingGroupInfoList) const
{
	*pRingGroupInfoList = m_RingGroupInfoList;
}


/**
 * \brief Sets the Ring Group Info list to the response object.
 * \param pRingGroupInfoList Pointer to Ring Group Info list to set
 */
void CstiCoreResponse::RingGroupInfoListSet (
	const std::vector <SRingGroupInfo> *pRingGroupInfoList)
{
	m_RingGroupInfoList = *pRingGroupInfoList;
}

/**
 * \brief Retrieves the Registration Info list from the response object.
 * \param pRegistrationInfoList Pointer to the retrieved Registration Info list
 */
void CstiCoreResponse::RegistrationInfoListGet (
	std::vector <SRegistrationInfo> *pRegistrationInfoList) const
{
	*pRegistrationInfoList = m_RegistrationInfoList;
}


/**
 * \brief Sets the Registration info to the response object.
 * \param pRegistrationInfo Pointer to registration Info List to set
 */
void CstiCoreResponse::RegistrationInfoListSet (
	const std::vector <SRegistrationInfo> *pRegistrationInfoList)
{
	m_RegistrationInfoList = *pRegistrationInfoList;
}

/**
 * \brief Retrieves the Registration Info list from the response object.
 * \param pRegistrationInfoList Pointer to the retrieved Registration Info list
 */
void CstiCoreResponse::AgreementListGet (
	std::vector <SAgreement> *pAgreementList) const
{
	*pAgreementList = m_AgreementList;
}


/**
 * \brief Sets the Registration info to the response object.
 * \param pRegistrationInfo Pointer to registration Info List to set
 */
void CstiCoreResponse::AgreementListSet (
	const std::vector <SAgreement> *pAgreementList)
{
	m_AgreementList = *pAgreementList;
}


/**
 * \brief Use this method to retrieve the settings list from the response object.
 *  \param ppList The address to a pointer to a list object
 */
std::vector<SstiSettingItem> CstiCoreResponse::SettingListGet ()
{
	return m_settingList;
}


/**
 * \brief Use this method to set the settings list to the response object.
 * \param pList The address of a settings list object
 */
void CstiCoreResponse::SettingListSet (
	const std::vector<SstiSettingItem> &list)
{
	m_settingList = list;
}


/**
 * \brief Gets the Missing Settings list
 * \param ppList The pointer pointer to the retrieved Missing settings list
 */
void CstiCoreResponse::MissingSettingsListGet (
	std::list <std::string> *list) const
{
	*list = m_MissingSettingList;
}


/**
 * \brief Sets the Missing Settings list
 * \param pList The pointer to the list to set
 */
void CstiCoreResponse::MissingSettingsListSet (
	const std::list <std::string> &list)
{
	m_MissingSettingList = list;
}

	
/**
 * \brief Retrieves the core service contacts
 * \param ServiceContacts The array to retrieve the core service contacts
 */
void CstiCoreResponse::CoreServiceContactsGet (
	std::string ServiceContacts[])
{
	ServiceContacts[0] = m_CoreServiceContacts[0];
	ServiceContacts[1] = m_CoreServiceContacts[1];
}


/**
 * \brief Retrieves the message service contacts
 * \param ServiceContacts The array to retrieve the service contacts
 */
void CstiCoreResponse::MessageServiceContactsGet (
	std::string ServiceContacts[])
{
	ServiceContacts[0] = m_MessageServiceContacts[0];
	ServiceContacts[1] = m_MessageServiceContacts[1];
}


/**
 * \brief Retrieves the conference service contacts
 * \param ServiceContacts The array to retrieve the service contacts
 */
void CstiCoreResponse::ConferenceServiceContactsGet (
	std::string ServiceContacts[])
{
	ServiceContacts[0] = m_ConferenceServiceContacts[0];
	ServiceContacts[1] = m_ConferenceServiceContacts[1];
}


/**
 * \brief Retrieves the state notify service contacts
 * \param ServiceContacts The array to retrieve the service contacts
 */
void CstiCoreResponse::NotifyServiceContactsGet (
	std::string ServiceContacts[])
{
	ServiceContacts[0] = m_NotifyServiceContacts[0];
	ServiceContacts[1] = m_NotifyServiceContacts[1];
}


/**
 * \brief Retrieves the remote logger service contacts
 * \param ServiceContacts The array to retrieve the service contacts
 */
void CstiCoreResponse::RemoteLoggerServiceContactsGet (
	std::string ServiceContacts[])
{
	ServiceContacts[0] = m_RemoteLoggerServiceContacts[0];
	ServiceContacts[1] = m_RemoteLoggerServiceContacts[1];
}


/**
 * \brief Sets the core service contacts
 * \param ServiceContacts An array of service contacts
 */
void CstiCoreResponse::CoreServiceContactsSet (
	std::string ServiceContacts[])
{
	m_CoreServiceContacts[0] = ServiceContacts[0];
	m_CoreServiceContacts[1] = ServiceContacts[1];
}


/**
 * \brief Use this method to set the message service contacts
 * \param ServiceContacts  An array of service contacts
 */
void CstiCoreResponse::MessageServiceContactsSet (
	std::string ServiceContacts[])
{
	m_MessageServiceContacts[0] = ServiceContacts[0];
	m_MessageServiceContacts[1] = ServiceContacts[1];
}


/**
 * \brief Use this method to set the conference service contacts
 * \param ServiceContacts  An array of service contacts
 */
void CstiCoreResponse::ConferenceServiceContactsSet (
	std::string ServiceContacts[])
{
	m_ConferenceServiceContacts[0] = ServiceContacts[0];
	m_ConferenceServiceContacts[1] = ServiceContacts[1];
}


/**
 * \brief Use this method to set the state notify service contacts
 * \param ServiceContacts An array of service contacts
 */
void CstiCoreResponse::NotifyServiceContactsSet (
	std::string ServiceContacts[])
{
	m_NotifyServiceContacts[0] = ServiceContacts[0];
	m_NotifyServiceContacts[1] = ServiceContacts[1];
}

/**
 * \brief Use this method to set the remote logger service contacts
 * \param ServiceContacts  An array of service contacts
 */
void CstiCoreResponse::RemoteLoggerServiceContactsSet (
	std::string ServiceContacts[])
{
	m_RemoteLoggerServiceContacts[0] = ServiceContacts[0];
	m_RemoteLoggerServiceContacts[1] = ServiceContacts[1];
}

/**
 * \brief Retrieves the ScreenSaver Info list from the response 
 *  	  object.
 *  
 * \return A referance to a SScreenSaverInfo
 */
std::vector <CstiCoreResponse::SScreenSaverInfo> *CstiCoreResponse::ScreenSaverInfoListGet()
{
	return &m_ScreenSaverInfoList;
}

/**
 * \brief Sets the ScreenSaver Info list to the response object.
 * \param pScreenSaverInfoList Pointer to ScreenSaver Info list 
 *  	  to set
 */
void CstiCoreResponse::ScreenSaverInfoListSet (
	const std::vector <SScreenSaverInfo> &screenSaverInfoList)
{
	m_ScreenSaverInfoList = screenSaverInfoList;
}
// end file CstiCoreResponse.cpp
