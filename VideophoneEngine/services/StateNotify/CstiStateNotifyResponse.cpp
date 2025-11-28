////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiStateNotifyResponse
//
//	File Name:	CstiStateNotifyResponse.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		ACTION - enter description
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "CstiStateNotifyResponse.h"

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
//; CstiStateNotifyResponse::CstiStateNotifyResponse
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiStateNotifyResponse::CstiStateNotifyResponse (
	CstiVPServiceRequest *request,
	EResponse eResponse,
	EstiResult eResponseResult,
	EStateNotifyError eError,
	const char *pszError)
	:
	CstiVPServiceResponse(request, eResponseResult, pszError),
	m_eResponse (eResponse),
	m_eError (eError)
{
	stiLOG_ENTRY_NAME (CstiStateNotifyResponse::CstiStateNotifyResponse);

} // end CstiStateNotifyResponse::CstiStateNotifyResponse


// end file CstiStateNotifyResponse.cpp
