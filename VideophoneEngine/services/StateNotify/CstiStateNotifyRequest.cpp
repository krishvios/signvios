////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiStateNotifyRequest
//
//  File Name:	CstiStateNotifyRequest.cpp
//
//	Abstract:
//		Constructs XML files send the StateNotifyRequest requests to
//		the State Notification Service
//
////////////////////////////////////////////////////////////////////////////////

#ifdef stiLINUX
#include "stiMem.h"
#endif // ifdef stiLINUX

#include "CstiStateNotifyRequest.h"
#include "stiCoreRequestDefines.h"     // for the SNRQ_..., TAG_... defines, etc.
#include "CstiStateNotifyResponse.h"
#include <cstdlib>

#ifdef stiLINUX
using namespace std;
#endif


////////////////////////////////////////////////////////////////////////////////
// Example XML: the StateNotifyRequest for Heartbeat
// <StateNotifyRequest Id="0001" ClientToken="">
//   <Heartbeat>
//   </Heartbeat>
// </StateNotifyRequest>
//
int CstiStateNotifyRequest::heartbeat()
{
	auto hResult = AddRequest(SNRQ_Heartbeat);
	stiTESTRESULT ();

	ExpectedResultAdd(CstiStateNotifyResponse::eHEARTBEAT_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for StateNotifyRequest for publicIPGet
// <StateNotifyRequest Id="0001" ClientToken="">
//   <PublicIPGet></PublicIPGet>
// </StateNotifyRequest>
//
int CstiStateNotifyRequest::publicIPGet()
{
	// create the Request to get Public IP
	auto hResult = AddRequest(CRQ_PublicIPGet);
	stiTESTRESULT ();

	ExpectedResultAdd(CstiStateNotifyResponse::ePUBLIC_IP_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// XML for StateNotifyRequest for timeGet
// <StateNotifyRequest Id="0001" ClientToken="">
//   <TimeGet></TimeGet>
// </StateNotifyRequest>
//
int CstiStateNotifyRequest::timeGet()
{
	auto hResult = AddRequest(CRQ_TimeGet);
	stiTESTRESULT ();

	ExpectedResultAdd(CstiStateNotifyResponse::eTIME_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for StateNotifyRequest for ServiceContact
// <StateNotifyRequest Id="0001" ClientToken="">
//   <ServiceContact>
//     <ServicePriority>0</ServicePriority>
//     <ServiceUrl>core.sorensonvrs.com/core.aspx</ServiceUrl>
//   </ServiceContact>
// </StateNotifyRequest>
//
int CstiStateNotifyRequest::serviceContact(
	const char *svcPriority, 
	const char *svcUrl)
{
	auto hResult = AddRequest(CRQ_ServiceContact);
	stiTESTRESULT ();

	ExpectedResultAdd(CstiStateNotifyResponse::eSERVICE_CONTACT_RESULT);
	
	hResult = AddSubElementToRequest(TAG_ServicePriority, svcPriority);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(TAG_ServiceUrl, svcUrl);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


const char *CstiStateNotifyRequest::RequestTypeGet()
{
	return SNRQ_StateNotifyRequest;
}


bool CstiStateNotifyRequest::UseUniqueIDAsAlternate ()
{
	return true;
}
