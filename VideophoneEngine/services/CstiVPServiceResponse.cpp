////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiVPServiceResponse
//
//	File Name:	CstiVPServiceResponse.cpp
//
//	Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include "CstiVPServiceResponse.h"
#include "CstiVPServiceRequest.h"


CstiVPServiceResponse::CstiVPServiceResponse(
	CstiVPServiceRequest *request,
	EstiResult eResponseResult,
	const char *pszError)
	:
	m_eResponseResult (eResponseResult),
	m_unResponseValue (0)
{
	if (request)
	{
		m_unRequestID = request->requestIdGet();
		m_pContext = request->contextGet();
		m_callbacks = request->m_callbacks;
	}

	if (pszError != nullptr)
	{
		ErrorStringSet (pszError);
	} // end if
}


void CstiVPServiceResponse::destroy ()
{
	if (m_shared_ptr != nullptr)
	{
		m_shared_ptr.reset ();
	}
	else
	{
		delete this;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ResponseStringGet
//
// Description: Get a string saved in the response from the response
//
// Abstract:
//
// Returns:
//
std::string CstiVPServiceResponse::ResponseStringGet (
	int nIndex) const
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ResponseStringGet);

	if (nIndex < MAX_RESPONSE_STRINGS)
	{
		return m_responseString[nIndex];
	}

	return {};
} // end CstiVPServiceResponse::ResponseStringGet


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ResponseStringSet
//
// Description: Save a string from the response for use later
//
// Abstract:
//
// Returns:
//
stiHResult CstiVPServiceResponse::ResponseStringSet (
	int nIndex,
	const char *pszResponseString)
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ResponseStringSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (nIndex < MAX_RESPONSE_STRINGS, stiRESULT_ERROR);

	m_responseString[nIndex] = pszResponseString;

STI_BAIL:

	return hResult;
} // end CstiVPServiceResponse::ResponseStringSet


bool CstiVPServiceResponse::hasCallbackType (const std::type_info& t)
{
	auto hash = t.hash_code ();
	auto matches = std::find_if (m_callbacks.begin (), m_callbacks.end (),
		[hash](const std::shared_ptr<ServiceCallbackContextBase>& callback)
		{
			return callback->contentTypeIdGet () == hash;
		});
	return matches != m_callbacks.end ();
}
