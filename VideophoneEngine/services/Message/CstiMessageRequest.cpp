/*!
 * \file CstiMessgeRequest.cpp
 * \brief Implementation of the CstiMessageRequest class.
 *
 * Constructs XML MessageRequest files to send to the Message Service.
 *
 * \date Wed Oct 18, 2006
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include "ixml.h"
#include "CstiMessageRequest.h"
#include "stiCoreRequestDefines.h"     // for the TAG_... defines, etc.
#include "CstiMessageResponse.h"
#include <cstdlib>
#include <ctime>
#include <sstream>

#ifdef stiLINUX
using namespace std;
#endif

//
// Constant strings representing the Request tag names
//
const char MRQ_MessageRequest[] = "MessageRequest";
const char MRQ_MessageCreate[] = "MessageCreate";
const char MRQ_MessageDelete[] = "MessageDelete";
const char MRQ_MessageDeleteAll[] = "MessageDeleteAll";
const char MRQ_MessageInfoGet[] = "MessageInfoGet";
const char MRQ_MessageListGet[] = "MessageListGet";
const char MRQ_MessageListItemEdit[] = "MessageListItemEdit";
const char MRQ_SignMailSend[] = "SignMailSend";
const char MRQ_SignMailDelete[] = "SignMailDelete";
const char MRQ_MessageViewed[] = "MessageViewed";
const char MRQ_ItemViewed[] = "ItemViewed";
const char MRQ_NewMessageCountGet[] = "NewMessageCountGet";
const char MRQ_RetrieveMessageKey[] = "RetrieveMessageKey";
const char MRQ_MessageDownloadURLGet[] = "MessageDownloadURLGet";
const char MRQ_MessageDownloadURLListGet[] = "MessageDownloadURLListGet";
const char MRQ_SignMailUploadURLGet[] = "SignMailUploadURLGet";
const char MRQ_GreetingInfoGet[] = "GreetingInfoGet";
const char MRQ_GreetingUploadURLGet[] = "GreetingUploadURLGet";
const char MRQ_GreetingEnabledStateSet[] = "GreetingEnabledStateSet";
//
// Constant strings representing the sub elements and attributes of the request
//
const char MRQ_TAG_Name[] = "Name";
const char MRQ_TAG_PhoneNumber[] = "PhoneNumber";
const char MRQ_TAG_MessageIdentity[] = "MessageIdentity";
const char MRQ_TAG_MessageList[] = "MessageList";
const char MRQ_TAG_ViewId[] = "ViewId";
const char MRQ_TAG_PlaybackTime[] = "PlaybackTime";
const char MRQ_TAG_ToPhoneNumber[] = "ToPhoneNumber";
const char MRQ_TAG_FromPhoneNumber[] = "FromPhoneNumber";
const char MRQ_TAG_BlockCallerId[] = "BlockCallerId";
const char MRQ_TAG_FromName[] = "FromName";
const char MRQ_TAG_InterpreterId[] = "InterpreterId";
const char MRQ_TAG_MessageTypeId[] = "MessageTypeId";
const char MRQ_TAG_CallId[] = "CallId";
const char MRQ_TAG_MessageId[] = "MessageId";
const char MRQ_TAG_MessageSize[] = "MessageSize";
const char MRQ_TAG_MessageSeconds[] = "MessageSeconds";
const char MRQ_TAG_Bitrate[] = "Bitrate";
const char MRQ_TAG_SignMailText[] = "Text";
const char MRQ_TAG_StartSeconds[] = "TextStartSeconds";
const char MRQ_TAG_StopSeconds[] = "TextStopSeconds";
const char MRQ_TAG_GreetingKey[] = "GreetingKey";


////////////////////////////////////////////////////////////////////////////////
//
// MessageRequest functions
//  NOTE: see doxygen comments in the CstiMessageRequest.h file
//

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageInfoGet
// <MessageRequest Id="0001" ClientToken="">
//   <MessageInfoGet>
//     <MessageIdentity>
//       <ItemId>1234</ItemId>
//       <Name>John Doe</Name>
//       <DialString>15555551212</DialString>
//     </MessageIdentity>
//   </MessageInfoGet>
// </MessageRequest>
//
int CstiMessageRequest::MessageInfoGet(
	const char *pszItemId,
	const char *pszName,
	const char *pszDialString)
{
	IXML_Element *messageIdentityElement;

	auto hResult = AddRequest(MRQ_MessageInfoGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_INFO_GET_RESULT);
	
	hResult = AddSubElementToRequest(MRQ_TAG_MessageIdentity, nullptr, &messageIdentityElement);
	stiTESTRESULT ();
	
	hResult = AddSubElementToThisElement(messageIdentityElement, TAG_ItemId, pszItemId);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(messageIdentityElement, MRQ_TAG_Name, pszName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(messageIdentityElement, TAG_DialString, pszDialString);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageViewed
// <MessageRequest Id="0001" ClientToken="">
//   <MessageViewed>
//     <MessageIdentity>
//       <ItemId>1234</ItemId>
//       <Name>John Doe</Name>
//       <DialString>15555551212</DialString>
//     </MessageIdentity>
//   </MessageViewed>
// </MessageRequest>
//
int CstiMessageRequest::MessageViewed(
	const CstiItemId &pItemId,
	const char *pszName,
	const char *pszDialString)
{
	auto hResult = AddRequest(MRQ_MessageViewed);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_VIEWED_RESULT);
	
	IXML_Element *messageIdentityElement;
	hResult = AddSubElementToRequest(MRQ_TAG_MessageIdentity, nullptr, &messageIdentityElement);
	stiTESTRESULT ();

	{
		std::string itemId;
		pItemId.ItemIdGet(&itemId);
		hResult = AddSubElementToThisElement(messageIdentityElement, TAG_ItemId, itemId.c_str());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToThisElement(messageIdentityElement, MRQ_TAG_Name, pszName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(messageIdentityElement, TAG_DialString, pszDialString);
	stiTESTRESULT ();
	
STI_BAIL:

	return requestResultGet (hResult);
}


////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageListGet
// <MessageRequest Id="0001" ClientToken="">
//   <MessageListGet>
//     <MessageList BaseIndex="0" Count="10" SortDir="DESC">
//     </MessageList>
//   </MessageListGet>
// </MessageRequest>
//
int CstiMessageRequest::MessageListGet(
	unsigned int baseIndex,
	unsigned int count,
	CstiMessageList::SortDirection eSortDir,
	time_t ttAfterTime)
{
	int retVal = IXML_SUCCESS;
	const char * szSortDir = nullptr;

	auto hResult = AddRequest(MRQ_MessageListGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_LIST_GET_RESULT);

	// Convert the baseIndex, count and SortDirection to strings
	//
	szSortDir = ( CstiMessageList::SortDirection::ASCENDING == eSortDir ? VAL_ASC : VAL_DESC);
	char szIndex[12];
	sprintf(szIndex, "%u", baseIndex);
	char szCount[12];
	sprintf(szCount, "%u", count);

	IXML_Element *messageListElement;
	hResult = AddSubElementToRequest(
		MRQ_TAG_MessageList,
		nullptr,
		&messageListElement,
		ATT_BaseIndex,
		szIndex,
		ATT_Count,
		szCount);
	stiTESTRESULT ();
	
	hResult = addTimeAttributeToElement (messageListElement, ttAfterTime);
	stiTESTRESULT ();
	
	// Add remaining attributes:  sortField, sortDir
	retVal = ixmlElement_setAttribute(
		messageListElement,
		(char*)ATT_SortDir,
		(char*)szSortDir);
	stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
	
STI_BAIL:

	return requestResultGet (hResult);
}


////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for UnviewedMessageCountGet
// <MessageRequest Id="0001" ClientToken="">
//   <NewMessageCountGet />
//      <Time>2009-05-20 00:00:00</Time>
// </MessageRequest>
//
// Note: the name of the function is different than the name of the request.
// The name of the function is more accurate since the request only considers
// unviewed messages.
//
int CstiMessageRequest::UnviewedMessageCountGet(
	time_t ttAfterTime)
{

	auto hResult = AddRequest(MRQ_NewMessageCountGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eNEW_MESSAGE_COUNT_GET_RESULT);
	
	if (ttAfterTime != 0)
	{
		char szTime[32];

		GetFormattedTime (&ttAfterTime, szTime, sizeof (szTime));
		
		hResult = AddSubElementToRequest(TAG_Time, szTime);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}


////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageCreate
// <MessageRequest Id="0001" ClientToken="">
//   <MessageCreate>
//     <PhoneNumber>15555551212</PhoneNumber>
//     <DialType>1</DialType>
//   </MessageCreate>
// </MessageRequest>
//
int CstiMessageRequest::MessageCreate(
	const char *pszNumberDialed,
	EstiDialMethod eDialType)
{
	auto hResult = AddRequest(MRQ_MessageCreate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_CREATE_RESULT);
	
	hResult = AddSubElementToRequest(
		MRQ_TAG_PhoneNumber,
		pszNumberDialed);
	stiTESTRESULT ();
	
	char szDialType[8];
	sprintf(szDialType, "%d", (int)eDialType);
	
	hResult = AddSubElementToRequest(
		TAG_DialType,
		szDialType);
	stiTESTRESULT ();
	
STI_BAIL:

	return requestResultGet (hResult);
}


////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageDelete
// <MessageRequest Id="0001" ClientToken="">
//   <MessageDelete>
//     <MessageIdentity>
//       <ItemId>1234</ItemId>
//       <Name>John Doe</Name>
//       <DialString>15555551212</DialString>
//     </MessageIdentity>
//   </MessageDelete>
// </MessageRequest>
//
int CstiMessageRequest::MessageDelete(
	const CstiItemId &pItemId,
	const char *pszName,
	const char *pszDialString)
{
	auto hResult = AddRequest(MRQ_MessageDelete);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_DELETE_RESULT);
	
	IXML_Element *messageIdentityElement;
	hResult = AddSubElementToRequest(MRQ_TAG_MessageIdentity, nullptr, &messageIdentityElement);
	stiTESTRESULT ();
	
	{
		std::string itemId;
		pItemId.ItemIdGet(&itemId);
		hResult = AddSubElementToThisElement(messageIdentityElement, TAG_ItemId, itemId.c_str());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToThisElement(messageIdentityElement, MRQ_TAG_Name, pszName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(messageIdentityElement, TAG_DialString, pszDialString);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageDeleteAll
// <MessageRequest Id="0001" ClientToken="">
//   <MessageDeleteAll />
// </MessageRequest>
//
int CstiMessageRequest::MessageDeleteAll()
{
	auto hResult = AddRequest(MRQ_MessageDeleteAll);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_DELETE_ALL_RESULT);
	
STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for SignMailDelete
// <MessageRequest Id="0001" ClientToken="">
//   <SignMailDelete>
//     <MessageIdentity>
//       <ItemId>1234</ItemId>
//     </MessageIdentity>
//   </MessageDelete>
// </MessageRequest>
//
int CstiMessageRequest::SignMailDelete(
	const char *pszItemId)
{
	auto hResult = AddRequest(MRQ_SignMailDelete);
	stiTESTRESULT ();

//	ExpectedResultAdd (CstiMessageResponse::);
	
	hResult = AddSubElementToRequest(TAG_ItemId, pszItemId, nullptr);
	stiTESTRESULT ();
	
STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for SignMailSend
// <MessageRequest Id="0001" ClientToken="">
//   <SignMailSend>
//   	<MessageId>1234</MessageId>
//   	<MessageSize>1200000</MessageSize>
//   	<MessageSeconds>45</MessageSeconds>
// 		<Bitrate>512000</Bitrate>
// 		<Text>Text to be displayed</Text>
// 		<TextStartSeconds>5</TextStartSeconds>
// 		<TextEndSeconds>10</TextEndSeconds>
//   </SignMailSend>
// </MessageRequest>
//
int CstiMessageRequest::SignMailSend(
	const std::string &toPhoneNum,
	const std::string &messageId,		 
	int nMessageSize, 
	int nMessageLength,		 
	int nBitrate,
	const std::string &signMailText,
	int startSeconds,
	int stopSeconds)
{
	auto hResult = AddRequest(MRQ_SignMailSend);
	stiTESTRESULT ();

//	ExpectedResultAdd (CstiMessageResponse::);
	
	hResult = AddSubElementToRequest(MRQ_TAG_ToPhoneNumber, toPhoneNum.c_str());
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(MRQ_TAG_MessageId, messageId.c_str());
	stiTESTRESULT ();

	char szMessageData[64];
	sprintf(szMessageData, "%d", nMessageSize);
	hResult = AddSubElementToRequest(MRQ_TAG_MessageSize, szMessageData);
	stiTESTRESULT ();
	 
	sprintf(szMessageData, "%d", nMessageLength);
	hResult = AddSubElementToRequest(MRQ_TAG_MessageSeconds, szMessageData);
	stiTESTRESULT ();

	sprintf(szMessageData, "%d", nBitrate);
	hResult = AddSubElementToRequest(MRQ_TAG_Bitrate, szMessageData);
	stiTESTRESULT ();

	if (!signMailText.empty())
	{
		hResult = AddSubElementToRequest(MRQ_TAG_SignMailText, signMailText.c_str());
		stiTESTRESULT ();

		if (startSeconds > -1)
		{
			hResult = AddSubElementToRequest(MRQ_TAG_StartSeconds, std::to_string(startSeconds).c_str());
			stiTESTRESULT ();
		}

		if (stopSeconds > -1)
		{
			hResult = AddSubElementToRequest(MRQ_TAG_StopSeconds, std::to_string(stopSeconds).c_str());
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageViewed
// <MessageRequest Id="0001" ClientToken="">
//   <MessageListItemEdit>
//     <MessageViewed>
//       <ViewId>1</ViewId>
//       <ItemId>1234</ItemId>
//       <PlaybackTime>5</PlaybackTime>
//     </MessageViewed>
//   </MessageListItemEdit>
// </MessageRequest>
//
int CstiMessageRequest::MessagePausePointSave(
	const CstiItemId &itemId,
	const std::string &viewId,
	const int pausePoint)
{
	auto hResult = AddRequest(MRQ_MessageListItemEdit);
	stiTESTRESULT ();

//	ExpectedResultAdd (CstiMessageResponse::);
	
	IXML_Element *messageViewedElement;
	hResult = AddSubElementToRequest(MRQ_ItemViewed, nullptr, &messageViewedElement);
	stiTESTRESULT ();
	
	{
		std::string idValue;
		itemId.ItemIdGet(&idValue);
		hResult = AddSubElementToThisElement(messageViewedElement, TAG_ItemId, idValue.c_str());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToThisElement(messageViewedElement, MRQ_TAG_ViewId, viewId.c_str());
	stiTESTRESULT ();

	char szPausePoint[11];
	snprintf(szPausePoint, sizeof (szPausePoint), "%d", pausePoint);
	hResult = AddSubElementToThisElement(messageViewedElement, MRQ_TAG_PlaybackTime, szPausePoint);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}
	
////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageDownloadURLGet
// <MessageRequest Id="0001" ClientToken="">
//   <MessageDownloadURLGet>
//     <ItemId>1234</ItemId>
//   </MessageDownloadURLGet>
// </MessageRequest>
//
int CstiMessageRequest::MessageDownloadURLGet(
	const CstiItemId &pItemId)
{
	auto hResult = AddRequest(MRQ_MessageDownloadURLGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_GET_RESULT);

	{
		std::string itemId;
		pItemId.ItemIdGet(&itemId);
		hResult = AddSubElementToRequest(TAG_ItemId, itemId.c_str());
		stiTESTRESULT ();
	}
	
STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageDownloadURLListGet
// <MessageRequest Id="0001" ClientToken="">
//   <MessageDownloadURLListGet>
//     <ItemId>1234</ItemId>
//   </MessageDownloadURLListGet>
// </MessageRequest>
//
int CstiMessageRequest::MessageDownloadURLListGet(
	const CstiItemId &pItemId)
{
	auto hResult = AddRequest(MRQ_MessageDownloadURLListGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_LIST_GET_RESULT);
	
	{
		std::string itemId;
		pItemId.ItemIdGet(&itemId);
		hResult = AddSubElementToRequest(TAG_ItemId, itemId.c_str());
		stiTESTRESULT ();
	}
	
STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for RetrieveMessageKey
// <MessageRequest Id="0001" ClientToken="">
//   <RetrieveMessageKey>
//     <MessageIdentity>
//       <ItemId>1234</ItemId>
//       <Name>John Doe</Name>
//       <DialString>15555551212</DialString>
//     </MessageIdentity>
//   </RetrieveMessageKey>
// </MessageRequest>
//
int CstiMessageRequest::RetrieveMessageKey(
	const CstiItemId &pItemId,
	const char *pszName,
	const char *pszDialString)
{
	auto hResult = AddRequest(MRQ_RetrieveMessageKey);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiMessageResponse::eRETRIEVE_MESSAGE_KEY_RESULT);
	
	IXML_Element *messageIdentityElement;
	hResult = AddSubElementToRequest(MRQ_TAG_MessageIdentity, nullptr, &messageIdentityElement);
	stiTESTRESULT ();
	
	{
		std::string itemId;
		pItemId.ItemIdGet(&itemId);
		hResult = AddSubElementToThisElement(messageIdentityElement, TAG_ItemId, itemId.c_str());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToThisElement(messageIdentityElement, MRQ_TAG_Name, pszName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(messageIdentityElement, TAG_DialString, pszDialString);
	stiTESTRESULT ();
	
STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for MessageUploadKeyGet
// <MessageRequest Id="0001" ClientToken="">
//   <SignMailUploadURLGet>
//     <ToPhoneNumber></ToPhoneNumber>
//     <FromPhoneNumber></FromPhoneNumber> (TerpNet)
//     <FromName></FromName> (TerpNet)
//     <InterpreterId></InterpreterId> (TerpNet)
//     <CallId></CallId> (TerpNet)
//     <BlockCallerId>True</BlockCallerId>
//   </SignMailUploadURLGet>
// </MessageRequest>
//
int CstiMessageRequest::SignMailUploadURLGet(
	const char *pszToPhoneNumber,
	EstiMessageType messageType,
	const char *pszFromPhoneNumber,
	EstiBool bBlockCallerId,
	const char *pszFromName,
	const char *pszInterpreterId,
	const char *pszCallId)
{
	auto hResult = AddRequest(MRQ_SignMailUploadURLGet);
	stiTESTRESULT ();

//	ExpectedResultAdd (CstiMessageResponse::);
	
	hResult = AddSubElementToRequest(MRQ_TAG_ToPhoneNumber, pszToPhoneNumber);
	stiTESTRESULT ();

	if (pszFromPhoneNumber)
	{
		hResult = AddSubElementToRequest(MRQ_TAG_FromPhoneNumber, pszFromPhoneNumber);
		stiTESTRESULT ();
	}

	if (pszFromName)
	{
		hResult = AddSubElementToRequest(MRQ_TAG_FromName, pszFromName);
		stiTESTRESULT ();
	}

	if (pszInterpreterId)
	{
		hResult = AddSubElementToRequest(MRQ_TAG_InterpreterId, pszInterpreterId);
		stiTESTRESULT ();
	}

	if (pszCallId)
	{
		hResult = AddSubElementToRequest(MRQ_TAG_CallId, pszCallId);
		stiTESTRESULT ();
	}

	if (bBlockCallerId == estiTRUE)
	{
		hResult = AddSubElementToRequest(
			MRQ_TAG_BlockCallerId,
			bBlockCallerId?"true":"false");
		stiTESTRESULT ();
	}

	{
		std::stringstream messageTypeIdStream;
		messageTypeIdStream << messageType;
		hResult = AddSubElementToRequest(MRQ_TAG_MessageTypeId, messageTypeIdStream.str().c_str());
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for GreetingInfoGet
//<MessageRequest Id="0001" ClientToken="">
//  <GreetingInfoGet />
//</MessageRequest>
int CstiMessageRequest::GreetingInfoGet()
{
	auto hResult = AddRequest(MRQ_GreetingInfoGet);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for GreetingUploadURLGet
//<MessageRequest Id="0001" ClientToken="">
//  <GreetingUploadURLGet />
//</MessageRequest>
int CstiMessageRequest::GreetingUploadURLGet()
{
	auto hResult = AddRequest(MRQ_GreetingUploadURLGet);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

////////////////////////////////////////////////////////////////////////////////
// Example XML for MessageRequest for GreetingEnabledStateSet
//<MessageRequest Id="0001" ClientToken="">
//  <GreetingEnabledStateSet>
//    <GreetingKey>00000000-0000-0000-0000-000000000000</GreetingKey>
//    <Enabled>true</Enabled>
//  </GreetingEnabledStateSet>
//</MessageRequest>
int CstiMessageRequest::GreetingEnabledStateSet(
	const char *pszGreetingKey,
	EstiBool bEnabled)
{
	auto hResult = AddRequest(MRQ_GreetingEnabledStateSet);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(MRQ_TAG_GreetingKey, pszGreetingKey);
	stiTESTRESULT ();
	
	hResult = AddSubElementToRequest(
									 "Enabled",
									 bEnabled?"true":"false");
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


const char *CstiMessageRequest::RequestTypeGet()
{
	return MRQ_MessageRequest;
}
