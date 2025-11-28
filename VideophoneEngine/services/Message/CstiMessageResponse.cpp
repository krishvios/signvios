/*!
 * \file CstiMessgeResponse.cpp
 * \brief Implementation of the CstiMessageResponse class.
 *
 * \date Wed Oct 18, 2006
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//

#include "CstiMessageResponse.h"

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

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageResponse::CstiMessageResponse
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiMessageResponse::CstiMessageResponse (
	CstiVPServiceRequest *request,
	EResponse eResponse,
	EstiResult eResponseResult,
	EResponseError eError,
	const char *pszError)
:
	CstiVPServiceResponse(request, eResponseResult, pszError),
	m_eResponse (eResponse),
	m_eError (eError)
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::CstiMessageResponse);
	
	// Clear the contents of the greetingInfoList
	m_GreetingInfoList.clear ();

} // end CstiMessageResponse::CstiMessageResponse


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageResponse::~CstiMessageResponse
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiMessageResponse::~CstiMessageResponse ()
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::~CstiMessageResponse);

	// Was a message list allocated?
	if (m_poMessageList != nullptr)
	{
		// Yes! Free it now.
		delete m_poMessageList;
		m_poMessageList = nullptr;
	} // end if


} // end CstiMessageResponse::~CstiMessageResponse


/**
 * \brief Retrieves the Greeting Info list from the response object. 
 *  
 * \return A referance to a SGreetingInfo
 */
std::vector <CstiMessageResponse::SGreetingInfo> *CstiMessageResponse::GreetingInfoListGet()
{
	return &m_GreetingInfoList;
}

/**
 * \brief Sets the Greeting Info list to the response object.
 * \param pGreetingInfoList Pointer to Greeting Info list to set
 */
void CstiMessageResponse::GreetingInfoListSet (
	const std::vector <SGreetingInfo> &greetingInfoList)
{
	m_GreetingInfoList = greetingInfoList;
}


std::vector<CstiMessageResponse::CstiMessageDownloadURLItem> *CstiMessageResponse::MessageDownloadUrlListGet ()
{
	return &m_MessageDownloadUrlList;
}

void CstiMessageResponse::MessageDownloadUrlListSet (const std::vector<CstiMessageDownloadURLItem> &messageDownloadUrlList)
{
	m_MessageDownloadUrlList = messageDownloadUrlList;
}

// end file CstiMessageResponse.cpp
