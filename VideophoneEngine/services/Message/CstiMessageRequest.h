/*!
 * \file CstiMessageRequest.h
 * \brief Definition of the CstiMessageRequest class.
 *
 * \date Wed Oct 18, 2006
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
 
#ifndef CSTIMESSAGEREQUEST_H
#define CSTIMESSAGEREQUEST_H


#include "CstiMessageList.h"
#include "stiSVX.h"
#include "CstiVPServiceRequest.h"
#include "CstiItemId.h"

//
// Constants
//

//
// Forward Declarations
//


/**
*/
class CstiMessageRequest : public CstiVPServiceRequest
{
public:

	CstiMessageRequest() = default;
	~CstiMessageRequest() override = default;

	CstiMessageRequest (const CstiMessageRequest &) = delete;
	CstiMessageRequest (CstiMessageRequest &&) = delete;
	CstiMessageRequest &operator= (const CstiMessageRequest &) = delete;
	CstiMessageRequest &operator= (CstiMessageRequest &&) = delete;

////////////////////////////////////////////////////////////////////////////////
// The following APIs create each of the supported message requests.
////////////////////////////////////////////////////////////////////////////////

	/**
	* \brief Requests the Message information for the message identified by parameters
	* \param pszItemId The id of the file
	* \param pszName The name of the person who left the message (security double-check)
	* \param pszDialString The phone number of the person who left the message (security double-check)
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageInfoGet(
		const char *pszItemId,
		const char *pszName,
		const char *pszDialString);
	
	/**
	* \brief Sends the Request indicating that this message has been viewed
	* \param pItemId A CstiItemId that contains the Id of the file
	* \param pszName The Name of person who left the message (security double-check)
	* \param pszDialString The phone number of person who left the message (security double-check)
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageViewed(
		const CstiItemId &pItemId,
		const char *pszName,
		const char *pszDialString);
	
	/**
	* \brief Requests the List of Messages
	* \param baseIndex The index offset in the list of all messages where to start
	* \param count The number of messages to retrieve
	* \param eSortDir The sort direction (default eASCENDING)
	* \param ttAfterTime Only retrieve entries after this time
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageListGet(
		unsigned int baseIndex = 0,
		unsigned int count = 0,
		CstiMessageList::SortDirection eSortDir = CstiMessageList::SortDirection::ASCENDING,
		time_t ttAfterTime = 0);
	
	/**
	* \brief Requests the number of unviewed Messages
	* \param ttAfterTime Indicates the time from when the count should start
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int UnviewedMessageCountGet(
		time_t ttAfterTime);

	/**
	* \brief Sends the request information to create a new message
	* \param pszNumberDialed The number for which to leave a message.
	* \param eDialType The dial type.
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageCreate(
		const char *pszNumberDialed,
		EstiDialMethod eDialType);

	/**
	* \brief Sends the request information to delete a message
	* \param pItemId A CstiItemId that contains the Id of the file
	* \param pszName The Name of person who left the message (security double-check)
	* \param pszDialString The phone number of person who left the message (security double-check)
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageDelete(
		const CstiItemId &pItemId,
		const char *pszName,
		const char *pszDialString);

	/**
	* \brief Sends the request to delete all messages
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageDeleteAll();

	/**
	* \brief Requests the Message key for the message identified by parameters
	* \param pszItemId The Id of the file
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessageDownloadURLGet(
		const CstiItemId &pItemId);

	/**
	 * \brief Requests the Message download Url list
	 * \param pszItemId The Id of the message
	 * \return Error code if fails, otherwise SUCCESS (0)
	 */
	int MessageDownloadURLListGet(
		const CstiItemId &pItemId);

	/**
	* \brief Requests the Message key for the message identified by parameters
	* \param pItemId A CstiItemId that contains the Id of the file
	* \param pszName The Name of person who left the message (security double-check)
	* \param pszPhoneNumber The phone number of person who left the message (security double-check)
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int RetrieveMessageKey(
		const CstiItemId &pItemId,
		const char *pszName,
		const char *pszPhoneNumber);

	/**
	* \brief Sends the Request indicating the stopped point in the 
	*  		 message.
	* \param pItemId A CstiItemId that contains the Id of the file
	* \param pszViewId The id of the session.
	* \param nPausePoint The paused point in seconds.
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int MessagePausePointSave(
		const CstiItemId &itemId,
		const std::string &viewId,
		int pausePoint);

	/**
	* \brief Sends the request information to delete an uploaded 
	*  		 SignMail message
	* \param pszItemId The Id of the file
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int SignMailDelete(
		const char *pszItemId);

	/**
	* \brief Sends the request information to send a message
	* \param toPhoneNum The number of the phone being called.
	* \param messageId The Id of the message.
	* \param nMessageSize The size of the message in bytes.
	* \param nMessageLength The length of the message in seconds.
	* \param nBitrate The final bitrate of the message. 
	* \param signMailText Text to be displayed in the SignMail. 
	* \param startSeconds start time for when the text should be 
	*   	 displayed.
	* \param endSeconds end time for when the text should be 
	*   	 removed.
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int SignMailSend(
		const std::string &toPhoneNum,
		const std::string &messageId,
		int nMessageSize,
		int nMessageLength,
		int nBitrate,
		const std::string &signMailText,
		int startSeconds,
		int stopSeconds);

	/**
	* \brief Sends the Request for the upload URL to be used when 
	*  		 recording a message.
	* \param pszToPhoneNumber The number of the receiving phone.
	* \param pszFromPhoneNumber The number of the sending phone (TerpNet only)
	* \param pszFromName The name of the sender (TerpNet only)
	* \param pszInterpreterId The Id of the interpreter (TerpNet only)
	* \param pszCallId The Id of the call (TerpNet only)
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int SignMailUploadURLGet(
		const char *pszToPhoneNumber,
		EstiMessageType messageType,
		const char *pszFromPhoneNumber = nullptr,
	 	EstiBool bBlockCallerId = estiFALSE,
		const char *pszFromName = nullptr,
		const char *pszInterpreterId = nullptr,
		const char *pszCallId = nullptr);

	int GreetingInfoGet();

	/**
	* \brief Sends the Request for the upload URL to be used when 
	*   	 uploading SignMailGreeting.
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int GreetingUploadURLGet();
	int GreetingEnabledStateSet(
		const char *pszGreetingKey,
		EstiBool bEnabled);



////////////////////////////////////////////////////////////////////////////////
// Protected APIs and variables internal to creating message requests
////////////////////////////////////////////////////////////////////////////////

private:

	const char *RequestTypeGet() override;
};


#endif // CSTIMESSAGEREQUEST_H
