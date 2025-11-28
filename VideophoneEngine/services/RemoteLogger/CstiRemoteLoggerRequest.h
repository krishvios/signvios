/*!
 * \file CstiRemoteLoggerRequest.h
 * \brief Definition of the CstiRemoteLoggerRequest class.
 *
 * \date Fri may 31, 2013
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
 
#ifndef CSTIREMOTELOGGERREQUEST_H
#define CSTIREMOTELOGGERREQUEST_H

#include "stiSVX.h"
#include "CstiItemId.h"
#include "CstiVPServiceRequest.h"
//
// Constants
//

//
// Forward Declarations
//


/**
*/
class CstiRemoteLoggerRequest : public CstiVPServiceRequest
{
public:

	CstiRemoteLoggerRequest () = default;
	~CstiRemoteLoggerRequest () override = default;

	CstiRemoteLoggerRequest (const CstiRemoteLoggerRequest &) = delete;
	CstiRemoteLoggerRequest (CstiRemoteLoggerRequest &&) = delete;
	CstiRemoteLoggerRequest &operator= (const CstiRemoteLoggerRequest &) = delete;
	CstiRemoteLoggerRequest &operator= (CstiRemoteLoggerRequest &&) = delete;

	////////////////////////////////////////////////////////////////////////////////
	// The following APIs create each of the supported remote logger requests.
	////////////////////////////////////////////////////////////////////////////////

	/**
	* \brief Requests the Assertion log information be put into the remote logging system
	* \param timeStamp Current  TimeStamp
	* \param dialString The phone number of the device
	* \param mac The MAC address of the device
	* \param logData The device console trace log data
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int RemoteLog (
		const std::string &logType,
		const std::string &timeStamp,
		const std::string &logData);
	
	static void macAddressSet (const std::string& mac);
	static void deviceTokenSet (const std::string& deviceToken);
	static void sessionIdSet (const std::string& sessionId);
	static void vrsCallIdSet (const std::string& callId);
	static void versionSet (const std::string& version);
	static void productNameSet (const std::string& productName);
	static void loggedInPhoneNumberSet (const std::string& phoneNumber);
	static void errorCountSet (int errorCount);
	static void errorCountIncrease ();
	static void coreUserIdSet (const std::string& userId);
	static void coreGroupIdSet (const std::string& groupId);
	static void interfaceModeSet (const std::string& interfaceMode);
	static void networkTypeSet (const std::string& networkType);
	static void networkDataSet (const std::string& networkData);
	
private:
	
	static std::string m_MacAddress;
	static std::string m_deviceToken;
	static std::string m_sessionId;
	static std::string m_vrsCallId;
	
	static std::string m_version;
	static std::string m_productName;
	static std::string m_strLoggedInPhoneNumber;
	
	// Trace message send error count
	static int m_nSendErrorCount;
	
	static std::string m_coreUserId;
	static std::string m_coreGroupId;
	static std::string m_interfaceMode;
	static std::string m_networkType;
	static std::string m_networkData;

	const char *RequestTypeGet () override;
};


#endif // CSTIREMOTELOGGERREQUEST_H
