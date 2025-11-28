////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiStateNotifyRequest
//
//  File Name:	CstiStateNotifyRequest.h
//
//	Abstract:
//		see CstiStateNotifyRequest.cpp
//
////////////////////////////////////////////////////////////////////////////////

#ifndef CSTISTATENOTIFYREQUEST_H
#define CSTISTATENOTIFYREQUEST_H


#include "ixml.h"
#include "CstiVPServiceRequest.h"

//
// Constants
//

/**
*/
class CstiStateNotifyRequest : public CstiVPServiceRequest
{
public:
	CstiStateNotifyRequest() = default;
	~CstiStateNotifyRequest() override = default;

	CstiStateNotifyRequest (const CstiStateNotifyRequest &) = delete;
	CstiStateNotifyRequest (CstiStateNotifyRequest &&) = delete;
	CstiStateNotifyRequest &operator= (const CstiStateNotifyRequest &) = delete;
	CstiStateNotifyRequest &operator= (CstiStateNotifyRequest &&) = delete;

	////////////////////////////////////////////////////////////////////////////////
// The following APIs are called when submitting the request to the core server.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// The following APIs create each of the supported core requests.
////////////////////////////////////////////////////////////////////////////////

	/*!
	* \brief Creates the Heartbeat StateNotifyRequest
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int heartbeat();

	/**
	* \brief Calls the server to obtain the public IP addr for this phone
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int publicIPGet();

	/**
	* \brief Indicates to service whether a fail-over has occurred, and how
	*  the service was contacted.
	* \param svcPriority the service priority
	* \param svcUrl the service url
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int serviceContact(
		const char *svcPriority, 
		const char *svcUrl);

	/**
	* \brief Retrieves the current date/time from the server
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int timeGet();

////////////////////////////////////////////////////////////////////////////////
// Protected APIs and variables internal to creating state notify requests
////////////////////////////////////////////////////////////////////////////////

private:

	const char *RequestTypeGet() override;

	bool UseUniqueIDAsAlternate () override;
};


#endif // CSTISTATENOTIFYREQUEST_H
