/*!
 * \file CstiRemoteLoggerRequest.cpp
 * \brief Implementation of the CstiRemoteLoggerRequest class.
 *
 * Constructs XML RemoteLoggerRequest files to send to the RemoteLogger Service.
 *
 * \date Fri may 31, 2013
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include "ixml.h"
#include "CstiRemoteLoggerRequest.h"
#include "stiCoreRequestDefines.h"     // for the TAG_... defines, etc.
#include <cstdlib>
#include <ctime>
#include <sstream>

#ifdef stiLINUX
using namespace std;
#endif


std::string CstiRemoteLoggerRequest::m_MacAddress;
std::string CstiRemoteLoggerRequest::m_deviceToken;
std::string CstiRemoteLoggerRequest::m_sessionId;
std::string CstiRemoteLoggerRequest::m_vrsCallId;
std::string CstiRemoteLoggerRequest::m_version;
std::string CstiRemoteLoggerRequest::m_productName;
std::string CstiRemoteLoggerRequest::m_strLoggedInPhoneNumber;
int CstiRemoteLoggerRequest::m_nSendErrorCount;
std::string CstiRemoteLoggerRequest::m_coreUserId;
std::string CstiRemoteLoggerRequest::m_coreGroupId;
std::string CstiRemoteLoggerRequest::m_interfaceMode;
std::string CstiRemoteLoggerRequest::m_networkType;
std::string CstiRemoteLoggerRequest::m_networkData;


////////////////////////////////////////////////////////////////////////////////
//
// MessageRequest functions
//  NOTE: see doxygen comments in the CstiRemoteLoggerRequest.h file
//


////////////////////////////////////////////////////////////////////////////////
// Example XML for RemoteLoggerRequest for RemoteLogAssert
// <RemoteLoggerRequest Id="0001">
//   <RemoteLogAssert>
//     <RemoteLogIdentity>
//       <TimeStamp>1234</TimeStamp>
//		 <DialString>15555551212</DialString>
//       <MAC>000872032A43</MAC>
//		 <LastKnownCallId>34543456</LastKnownCallId>
//		 <Version>4.0.0.343</Version>
//		 <ProductName>Sorenson Videophone V3</ProductName>
//       <Data>
//
//
//
//		Assert Data
//
//
//
//
//		</Data>
//     </RemoteLogIdentity>
//   </RemoteLogAssert>
// </RemoteLoggerRequest>
//
int CstiRemoteLoggerRequest::RemoteLog(
	const std::string &logType,
	const std::string &timeStamp,
	const std::string &logData)
{
	int retVal = AddRequest(logType.c_str ());
	
	//
	// Constant strings representing the sub elements and attributes of the request
	//
	static const std::string RemoteIdentityTag	= "RemoteIdentity";
	static const std::string PhoneNumberTag	= "PhoneNumber";
	static const std::string MacTag	= "MAC";
	static const std::string DeviceTokenTag = "DeviceToken";
	static const std::string SessionIdTag = "SessionID";
	static const std::string CallIdTag	= "LastKnownCallID";
	static const std::string VersionTag	= "Version";
	static const std::string ProductNameTag	= "ProductName";
	static const std::string SendErrorsTag	= "SendErrors";
	static const std::string TimeStampTag	= "TimeStamp";
	static const std::string DataTag	= "Data";
	static const std::string NetworkTypeTag	= "NetworkType";
	static const std::string NetworkDataTag	= "NetworkData";
	static const std::string InterfaceModeTag	= "InterfaceMode";
	static const std::string CoreUserIdTag	= "CoreUserID";
	static const std::string CoreGroupIdTag	= "CoreGroupID";

	IXML_Element *remoteLoggerElement;
	retVal |= AddSubElementToRequest(RemoteIdentityTag.c_str (), nullptr, &remoteLoggerElement);

	retVal |= AddSubElementToThisElement(remoteLoggerElement, TimeStampTag.c_str (), timeStamp.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, PhoneNumberTag.c_str (), (m_strLoggedInPhoneNumber.empty()) ? "Unknown" : m_strLoggedInPhoneNumber.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, MacTag.c_str (), m_MacAddress.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, DeviceTokenTag.c_str (), m_deviceToken.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, SessionIdTag.c_str (), m_sessionId.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, CallIdTag.c_str (), m_vrsCallId.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, VersionTag.c_str (), m_version.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, ProductNameTag.c_str (), m_productName.c_str ());
	retVal |= AddSubElementToThisElement(remoteLoggerElement, SendErrorsTag.c_str (), std::to_string (m_nSendErrorCount).c_str ());
	
	if (!m_networkType.empty ())
	{
		retVal |= AddSubElementToThisElement(remoteLoggerElement, NetworkTypeTag.c_str (), m_networkType.c_str ());
	}
	
	if (!m_networkData.empty ())
	{
		retVal |= AddSubElementToThisElement(remoteLoggerElement, NetworkDataTag.c_str (), m_networkData.c_str ());
	}
	
	if (!m_interfaceMode.empty ())
	{
		retVal |= AddSubElementToThisElement(remoteLoggerElement, InterfaceModeTag.c_str (), m_interfaceMode.c_str ());
	}
	
	if (!m_coreUserId.empty ())
	{
		retVal |= AddSubElementToThisElement(remoteLoggerElement, CoreUserIdTag.c_str (), m_coreUserId.c_str ());
	}
	
	if (!m_coreGroupId.empty ())
	{
		retVal |= AddSubElementToThisElement(remoteLoggerElement, CoreGroupIdTag.c_str (), m_coreGroupId.c_str ());
	}

	//Add spaces so Splunk can parse the data
	std::stringstream strLogData;
	strLogData << " " << logData << " ";
	retVal |= AddSubElementToRequest(DataTag.c_str (), strLogData.str().c_str(), &remoteLoggerElement);

	//retVal |= AddSubElementToThisElement(remoteLoggerIdentityElement, RLRQ_TAG_LogData, pszLogData);

	return retVal;
}

void CstiRemoteLoggerRequest::macAddressSet (const std::string& mac)
{
	m_MacAddress = mac;
}

void CstiRemoteLoggerRequest::deviceTokenSet (const std::string& deviceToken)
{
	m_deviceToken = deviceToken;
}

void CstiRemoteLoggerRequest::sessionIdSet (const std::string& sessionId)
{
	m_sessionId = sessionId;
}

void CstiRemoteLoggerRequest::vrsCallIdSet (const std::string& callId)
{
	m_vrsCallId = callId;
}

void CstiRemoteLoggerRequest::versionSet (const std::string& version)
{
	m_version = version;
}

void CstiRemoteLoggerRequest::productNameSet (const std::string& productName)
{
	m_productName = productName;
}

void CstiRemoteLoggerRequest::loggedInPhoneNumberSet (const std::string& phoneNumber)
{
	m_strLoggedInPhoneNumber = phoneNumber;
}

void CstiRemoteLoggerRequest::errorCountSet (const int errorCount)
{
	m_nSendErrorCount = errorCount;
}

void CstiRemoteLoggerRequest::errorCountIncrease ()
{
	m_nSendErrorCount++;
}

void CstiRemoteLoggerRequest::coreUserIdSet (const std::string& userId)
{
	m_coreUserId = userId;
}

void CstiRemoteLoggerRequest::coreGroupIdSet (const std::string& groupId)
{
	m_coreGroupId = groupId;
}

void CstiRemoteLoggerRequest::interfaceModeSet (const std::string& interfaceMode)
{
	m_interfaceMode = interfaceMode;
}

void CstiRemoteLoggerRequest::networkTypeSet (const std::string& networkType)
{
	m_networkType = networkType;
}

void CstiRemoteLoggerRequest::networkDataSet (const std::string& networkData)
{
	m_networkData = networkData;
}


const char *CstiRemoteLoggerRequest::RequestTypeGet()
{
	return RLRQ_RemoteLoggerRequest;
}
