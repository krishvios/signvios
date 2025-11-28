/*!
 * \file stiMessages.h
 * \brief See stiMessages.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIMESSAGES_H
#define STIMESSAGES_H

#include "stiDefs.h"
#include <string>

/*!
 * \ingroup VideophoneEngineMessage
 * \brief The EstiCmMessage enumeration defines the messages sent from the CCI
 * to the application.  The messages deal with important state transitions,
 * critical error messages and the feature updated message.
 */
typedef enum EstiCmMessage
{
	/* Conference manager */
	estiMSG_CONFERENCE_MANAGER_STARTUP_COMPLETE = 0,
	/*!<
	 * \brief The conference manager is now ready to make and recieve calls
	 *
	 * \MsgParam{0}
	 */

	estiMSG_CONFERENCE_MANAGER_CRITICAL_ERROR,
	/*!<
	 * \brief System encountered critical error.
	 *
	 * \MsgParam{0}
	 */

	/* Call Starting */
	estiMSG_DIALING,
	/*!<
	 * \brief Attempting to connect with remote system.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	 */
	
	estiMSG_PRE_INCOMING_CALL,
	/*!<
	* Remote system attempting to connect, before ICE negotiation
	*
	* \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_INCOMING_CALL,
	/*!<
	 * Remote system attempting to connect.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_OUTGOING_CALL,
	/*!<
	 * An outgoing call is being placed.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	 */

	estiMSG_ACTIVE_CALL,
	/*!<
	 * There is or isnt an active call.
	 *
	 * \MsgParam{TRUE or FALSE}
	*/
	
	/* Call progressing */
	estiMSG_RINGING,
	/*!<
	 * Remote system has been notified and is ringing.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_REMOTE_RING_COUNT,
	/*!<
	 * Message indicating that the Remote system has rung.
	 *
	 * \MsgParam{ring count}
	*/

	estiMSG_LOCAL_RING_COUNT,
	/*!<
	 * Message indicating that the Local system has rung.
	 *
	 * \MsgParam{ring count}
	*/

	estiMSG_LEAVE_MESSAGE,
	/*!<
	 * Remote system has reached max number of rings with no answer, message can be left.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_SIGNMAIL_AVAILABLE,
		/*!<
		 * Notify VRCL server that SignMail is avalable.
		 *
		 * \MsgParam{A pointer to an IstiCall object.}
		*/

	estiMSG_SIGNMAIL_RECORDING_STARTED,
	/*!<
	 * Notify VRCL server that SignMail Recording has started.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_HEARING_VIDEO_CAPABLE,
	/*!<
	 * Notify VRCL server whether the call is hearing video capable.
	 *
	 * \MsgParam{TRUE or FALSE}
	*/

	estiMSG_HEARING_VIDEO_CONNECTED,
	/*!<
	 * Notify VRCL server that hearing video is connected.
	*/

	estiMSG_HEARING_VIDEO_DISCONNECTED,
	/*!<
	 * Notify VRCL server that the hearing video has been disconneted.
	 *
	 * \MsgParam{indication who terminated the hearing video (deaf/hearing}
	*/

	estiMSG_HEARING_VIDEO_CONNECTION_FAILED,
	/*!<
	 * Notify VRCL server the hearing video connection failed.
	*/

	estiMSG_SIGNMAIL_HANGUP,
	/*!<
	 * Notify UI to hang up the call.
	*/

	estiMSG_SIGNMAIL_RECORD_FINISHED,
	/*!<
	 * Notify VRCL Server that the SignMail has finished recording.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_MAILBOX_FULL,
	/*!<
	 * Remote system's mailbox is full so a message cannot be left.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_RECORD_ERROR,
	/*!<
	 * An error occured while recording a message.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_RECORD_MESSAGE,
	/*!<
	 * Begin recording message.
	 *
	 * \MsgParam{0}
	*/

	estiMSG_RECORD_MESSAGE_SEND_FAILED,
	/*!<
	 * Recorded message failed to send.
	 *
	 * \MsgParam{0}
	*/

	estiMSG_RECORD_MESSAGE_SEND_SUCCESS,
	/*!<
	 * Recorded message send succeeded.
	 *
	 * \MsgParam{0}
	*/

	estiMSG_VRS_ANNOUNCE_CLEAR,
	/*!<
	 * Notify the UI to clear the VRS Announce dialog.
	*/

	estiMSG_VRS_ANNOUNCE_SET,
	/*!<
	 * Notify the UI to display the VRS Announce text.
	*/

	estiMSG_GREETING_INFO_REQUEST_FAILED,
	/*!<
	 * Recorded greeting failed to upload.
	 *
	 * \MsgParam{0}
	*/

	estiMSG_GREETING_INFO,
	/*!<
	 * Recorded greeting Info from core.
	 *
	 * \MsgParam{A pointer to a CstiMessageResponse.}
	*/

    estiMSG_ANSWERING_CALL,
	/*!<
	 * Responding to remote system.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_ESTABLISHING_CONFERENCE,
	/*!<
	 * Starting call.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_HELD_CALL_LOCAL,
	/*!<
	 * The call is now on hold. (initiated locally)
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_HELD_CALL_REMOTE,
	/*!<
	 * The call is now on hold. (initiated remotely)
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_RESUMED_CALL_LOCAL,
	/*!<
	 * The call placed on hold locally is again active.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_RESUMED_CALL_REMOTE,
	/*!<
	 * The call placed on hold remotely is again active.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_RESOLVING_NAME,
	/*!<
	 * Resolving remote system IP address through directory service.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_DIRECTORY_RESOLVE_RESULT,
	/*!<
	 * Inform the application that a directory resolve had returned
	 *
	 * \MsgParam{A pointer to an SstiCallResolution object.}
	*/

	/* Call Results */
	estiMSG_CONFERENCING,
	/*!<
	 * Channel negotiations have been started.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_CONFERENCE_ENCRYPTION_STATE_CHANGED,
	/*!<
	 * Advertises the encryption state has changed and should be
	 * checked
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_CONFERENCING_WITH_MCU,

	/* Call terminating */
	estiMSG_DISCONNECTING,
	/*!<
	 * Hanging up call.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_TRANSFERRING,
	/*!<
	 * Transferring call.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_TRANSFER_FAILED,
	/*!<
	 * Failure to Transfer Call.
	 *
	 * \MsgParam{A pointer to an CstiCall object.}
	 */

	estiMSG_DISCONNECTED,
	/*!<
	 * The call is now complete and the call object
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	/* VRS Signmail event messages */
	estiVRS_SIGNMAIL_CONFIRMATION_DONE,
	/*!<
	 * The Signmail VRS Confirmation message sent OK.
	 *
	 * \MsgParam{0}
	*/

	estiMSG_CALL_INFORMATION_CHANGED,
	/*!<
	 * Indicates the information for a given call has changed.
	 *
	 * \MsgParam{A pointer to an IstiCall object.}
	*/

	estiMSG_PRINTSCREEN,

	/* Callback messages */
	estiMSG_CB_ERROR_REPORT,
	estiMSG_CB_PUBLIC_IP_RESOLVED,
	estiMSG_CB_CURRENT_TIME_SET,
	estiMSG_CB_SERVICE_SHUTDOWN,
	estiMSG_CB_VIDEO_PLAYBACK_MUTED,
	estiMSG_CB_VIDEO_PLAYBACK_UNMUTED,
	estiMSG_CB_DTMF_RECVD,
	estiMSG_CB_SIP_REGISTRATION_CONFIRMED,


	/* Core Service Messages */
	estiMSG_DNS_ERROR, /*!< The Core Service or State notify service Name cannot be resolved */
	estiMSG_CORE_SERVICE_AUTHENTICATED,	/*!< User was successfully authenticated. */
	estiMSG_CORE_SERVICE_NOT_AUTHENTICATED, /*!< Invalid user credentials */
	estiMSG_CORE_SERVICE_CONNECTED,		/*!< Connected to the Core Service. */
	estiMSG_CORE_SERVICE_CONNECTING,	/*!< Connecting (or reconnecting) to
										* the Core Service. */
	estiMSG_CORE_SERVICE_RECONNECTED,	/*!< Re-connected to State Notify Service
										* after having been unavailable.
										*/
	estiMSG_CORE_RESPONSE,				/*!< Passes a Core Response to the app.
										* A pointer to a CstiCoreResponse
										* object is passed in the event
										* parameter, and MUST BE FREED with
										* 'delete'. The application is
										* responsible for deleting the
										* object when it is no
										* longer needed.
										*/
	estiMSG_CORE_REQUEST_REMOVED,		/*!< Indiates that a core request has
										* been removed.  A pointer to a SstiRequestRemoved
										* structure is passed in the event parameter
										* and MUST BE FREED with 'delete'.  The application
										* is responsible for deleting the object when
										* it is no longer needed.
										*/
	estiMSG_CORE_SERVICE_SSL_FAILOVER,	/*!< Indicates failover from SSL to port 80 */

	estiMSG_CORE_ASYNC_REQUEST_ID,		/*!< Contains the request id of a core request that
										 * was asked to be sent asynchronously.
										 */

	estiMSG_CORE_ASYNC_REQUEST_FAILED,	/*!< The async request failed. */

	/* State Notify Messages */
	estiMSG_STATE_NOTIFY_SERVICE_CONNECTED,		/*!< Connected to the State
												* Notify Service.
												*/
	estiMSG_STATE_NOTIFY_SERVICE_CONNECTING,	/*!< Connecting to the State
												* Notify Service.
												*/
	estiMSG_STATE_NOTIFY_SERVICE_RECONNECTED,	/*!< Re-connected to State
												* Notify Service after having
												* been unavailable.
												*/

	estiMSG_STATE_NOTIFY_RESPONSE,		/*!< Passes a State Notify Response to
										* the app. A pointer to a
										* CstiStateNotifyResponse object is
										* passed in the event parameter, and
										* MUST BE FREED with 'delete'. The
										* application is responsible for
										* deleting the object when it is no
										* longer needed.
										*/
	estiMSG_STATE_NOTIFY_SSL_FAILOVER,	/*!< Indicates failover from SSL to port 80 */

	/* Message Service Messages */
	estiMSG_MESSAGE_SERVICE_CONNECTED,			/*!< Connected to the Message
												* Service.
												*/
	estiMSG_MESSAGE_SERVICE_CONNECTING,			/*!< Connecting to the Message
												* Service.
												*/
	estiMSG_MESSAGE_SERVICE_RECONNECTED,		/*!< Re-connected to Message
												* Service after having
												* been unavailable.
												*/

	estiMSG_MESSAGE_RESPONSE,			/*!< Passes a Message Response to
										* the app. A pointer to a
										* CstiMessageResponse object is
										* passed in the event parameter, and
										* MUST BE FREED with 'delete'. The
										* application is responsible for
										* deleting the object when it is no
										* longer needed.
										*/
	estiMSG_MESSAGE_REQUEST_REMOVED,	/*!< Indicates that a message request has
										* been removed.  A pointer to a SstiRequestRemoved
										* structure is passed in the event parameter
										* and MUST BE FREED with 'delete'.  The application
										* is responsible for deleting the object when
										* it is no longer needed.
										*/
	estiMSG_MESSAGE_SERVICE_SSL_FAILOVER,	/*!< Indicates failover from SSL to port 80 */

	/* Conference Service Messages */
	estiMSG_CONFERENCE_SERVICE_CONNECTED,			/*!< Connected to the Message
												* Service.
												*/
	estiMSG_CONFERENCE_SERVICE_CONNECTING,			/*!< Connecting to the Message
												* Service.
												*/
	estiMSG_CONFERENCE_SERVICE_RECONNECTED,		/*!< Re-connected to Message
												* Service after having
												* been unavailable.
												*/

	estiMSG_CONFERENCE_RESPONSE,			/*!< Passes a Message Response to
										* the app. A pointer to a
										* CstiMessageResponse object is
										* passed in the event parameter, and
										* MUST BE FREED with 'delete'. The
										* application is responsible for
										* deleting the object when it is no
										* longer needed.
										*/
	estiMSG_CONFERENCE_REQUEST_REMOVED,	/*!< Indiates that a message request has
										* been removed.  A pointer to a SstiRequestRemoved
										* structure is passed in the event parameter
										* and MUST BE FREED with 'delete'.  The application
										* is responsible for deleting the object when
										* it is no longer needed.
										*/
	estiMSG_CONFERENCE_SERVICE_SSL_FAILOVER,	/*!< Indicates failover from SSL to port 80 */

	estiMSG_FLASH_LIGHT_RING,        /* Notify the application to flash the light ring */

	estiMSG_FILE_PLAY_STATE,		/*!< Notify the application that the file play state has changed */

	estiMSG_DISABLE_PLAYER_CONTROLS,	/*!< Notify the application that the video messages cannot skip disable the controls */

	estiMSG_REQUEST_GUID,			/*!< Notify the application to get new GUID from the Message Server for replaying */

	estiMSG_REQUEST_UPLOAD_GUID,   	/*!< Notify the application to get a upload GUID from the Message Server for storing a message */

    estiMSG_UPLOAD_GUID_REQUEST_FAILED, /*!< Notify the application that the request for an upload GUID failed */ 

	estiMSG_REQUEST_GREETING_UPLOAD_GUID, /*!< Notify CCI to get a upload GUID from the Message Server for storing a personal greeting */

	estiMSG_REBOOT_DEVICE,			/*!< Notify the application a reboot is requested */

	/* Remote Logger Services Messages*/
	estiMSG_REMOTELOGGER_SERVICE_SSL_FAILOVER,	/*!< Indicates failover from SSL to port 80 */

	// The following are for events from the IstiNetwork object.
	estiMSG_NETWORK_STATE_CHANGE, 		/*!< Notify the application that a network state
										 *   change has occurred.
										 */

	estiMSG_NETWORK_SETTINGS_CHANGED,	// the active connection has changed
	estiMSG_NETWORK_WIRED_CONNECTED,	// someone plugged in the cable
	estiMSG_NETWORK_WIRED_DISCONNECTED,	// someone unplugged the cable

	estiMSG_CONFERENCE_PORTS_CHANGED,

	estiMSG_USBRCU_CONNECTION_STATUS_CHANGED,
	estiMSG_USBRCU_RESET,
	estiMSG_MFDDRVD_STATUS_CHANGED,
	estiMSG_NETWORK_WIRELESS_CONNECTION_SUCCESS,
	estiMSG_NETWORK_WIRELESS_CONNECTION_FAILURE,

	estiMSG_IMAGE_LOADED,
	estiMSG_IMAGE_LOAD_FAILED,
	estiMSG_IMAGE_ENCODED,

	estiMSG_INTERFACE_MODE_CHANGED,

	estiMSG_MAX_SPEEDS_CHANGED,

	estiMSG_AUTO_SPEED_SETTINGS_CHANGED,

	estiMSG_PUBLIC_IP_CHANGED,

	// Call History list
	estiMSG_CALL_LIST_CHANGED,
	estiMSG_CALL_LIST_ENABLED,
	estiMSG_CALL_LIST_DISABLED,
	estiMSG_CALL_LIST_ERROR,

	estiMSG_CALL_LIST_ITEM_ADDED,
	estiMSG_CALL_LIST_ITEM_DELETED,
	estiMSG_CALL_LIST_ITEM_ADD_ERROR,
	estiMSG_CALL_LIST_ITEM_DELETE_ERROR,
	estiMSG_CALL_LIST_ITEM_EDIT_ERROR,

	estiMSG_MISSED_CALL_COUNT,

	// Block list
	estiMSG_BLOCK_LIST_ITEM_ADDED, //! This message is sent once core has confirmed that the item has been added.
	estiMSG_BLOCK_LIST_ITEM_EDITED, //! This message is sent once core has confirmed that the item has been edited.
	estiMSG_BLOCK_LIST_ITEM_DELETED, //! This message is sent once core has confirmed that the item has been deleted.
	estiMSG_BLOCK_LIST_CHANGED, //! This message is sent if the block list manager has reloaded the list.  This will occur if core indicates the block list has changed.

	estiMSG_BLOCK_LIST_ENABLED, //! This message is sent if the block list has been enabled.
	estiMSG_BLOCK_LIST_DISABLED, //! This message is sent if the block list has been disabled.

	// Contact list
	estiMSG_CONTACT_LIST_ITEM_ADDED, //! This message is sent once core has confirmed that the item has been added.
	estiMSG_CONTACT_LIST_ITEM_EDITED, //! This message is sent once core has confirmed that the item has been edited.
	estiMSG_CONTACT_LIST_ITEM_DELETED, //! This message is sent once core has confirmed that the item has been deleted.
	estiMSG_CONTACT_LIST_CHANGED, //! This message is sent if the block list manager has reloaded the list.  This will occur if core indicates the block list has changed.
	estiMSG_CONTACT_LIST_DELETED, //! This message is sent if the contact list has been deleted.
	estiMSG_CONTACT_LIST_IMPORT_COMPLETE, //! This message is sent when the contact import process is complete

	// Favorites
	estiMSG_FAVORITE_ITEM_ADDED,
	estiMSG_FAVORITE_ITEM_EDITED,
	estiMSG_FAVORITE_ITEM_REMOVED,
	estiMSG_FAVORITE_LIST_CHANGED,
	estiMSG_FAVORITE_LIST_SET,
	
	// LDAP Directory contact list
	estiMSG_LDAP_DIRECTORY_CONTACT_LIST_CHANGED, //! This message is sent if the LDAP Directory manager has reloaded the list.
    estiMSG_LDAP_DIRECTORY_VALID_CREDENTIALS,
    estiMSG_LDAP_DIRECTORY_INVALID_CREDENTIALS,
	estiMSG_LDAP_DIRECTORY_SERVER_UNAVAILABLE,
	estiMSG_LDAP_DIRECTORY_ERROR,

	
	// Message List
	estiMSG_MESSAGE_CATEGORY_CHANGED, //! This message is sent to the UI when a category's content changes.
	estiMSG_MESSAGE_ITEM_CHANGED,    //! This messages is sent to the UI when a message is changed.
	estiMSG_MESSAGE_DELETE_FAIL, //! This messages is sent to the UI when a message fails to be deleted.


	estiMSG_CB_USER_INFO_UPDATED,  //! Some remote user information has been updated.  Alert the app (and/or VRCL Server)

	// VRCL Connection Messages
	estiMSG_VRCL_CONNECTION_ESTABLISHED,
	estiMSG_VRCL_CONNECTION_CLOSED,
	estiMSG_VRCL_LOCAL_ALTERNATE_NAME_SET,

	// Ring Group
	estiMSG_RING_GROUP_INFO_CHANGED,	//! Informs the UI that the Ring Group info has been updated
	estiMSG_RING_GROUP_REMOVED,			//! Informs the UI that this endpoint was removed from the ring group

	estiMSG_RING_GROUP_MODE_CHANGED,	//! Informs the UI that Ring Group mode changed

	estiMSG_RING_GROUP_CREATED,     //! A myPhone group has successfully been created.
	estiMSG_RING_GROUP_INVITE_ACCEPTED, //! A myPhone invitation has successfully been accepted.
	estiMSG_RING_GROUP_INVITE_REJECTED, //! A myPhone invitation has successfully been rejected.
	estiMSG_RING_GROUP_ITEM_ADDED, //! This message is sent once core has confirmed that the item has been added.
	estiMSG_RING_GROUP_ITEM_EDITED, //! This message is sent once core has confirmed that the item has been edited.
	estiMSG_RING_GROUP_ITEM_DELETED, //! This message is sent once core has confirmed that the item has been deleted.
	estiMSG_RING_GROUP_VALIDATED, //! The user has successfully validated credentials for the ring group.
	estiMSG_RING_GROUP_PASSWORD_CHANGED, //! The user has successfully changed the ring group password.

	estiMSG_RING_GROUP_CREATE_ERROR,     //! The myPhone group could not be created for some reason.
	estiMSG_RING_GROUP_INVITE_ACCEPT_ERROR, //! The invitation could not be accepted for some reason.
	estiMSG_RING_GROUP_INVITE_REJECT_ERROR, //! The invitation could not be rejected for some reason.
	estiMSG_RING_GROUP_ITEM_ADD_ERROR, //! This message is sent when receiving error on attempt to add an item to a ring group.
	estiMSG_RING_GROUP_ITEM_EDIT_ERROR, //! This message is sent when receiving error on attempt to edit an item in a ring group.
	estiMSG_RING_GROUP_ITEM_DELETE_ERROR, //! This message is sent when receiving error on attempt to delete an item to a ring group.
	estiMSG_RING_GROUP_VALIDATE_ERROR, //! The user entered incorrect myPhone credentials
	estiMSG_RING_GROUP_PASSWORD_CHANGE_ERROR, //! The new password was rejected

	estiMSG_RING_GROUP_WIZARD,      //! CIR has requested the videophone to display the myPhone wizard.
	estiMSG_RING_GROUP_CIR_CREATED, //! CIR has created a ring group for us.
	estiMSG_RING_GROUP_INVITATION,  //! An invitation has been received to join a ring group.

	estiMSG_FAILED_TO_ESTABLISH_TUNNEL,
	estiMSG_TUNNEL_TERMINATED,
	estiMSG_REMOVE_TEXT,
	// Remote Text Sharing
	estiMSG_REMOTE_TEXT_RECEIVED,
		/*!<
		 * Notify application and VRCL server of a received buffer of text..
		 *
		 * \MsgParam{A pointer to an SstiTextMsg object.}
		*/

	// Local Text Message Sent (using VRCL)
	estiMSG_TEXT_MESSAGE_SENT,
		/*!<
		 * Notify Application that VRCL sent a Text Message to remote endpoint
		 *
		 * \MsgParam{A pointer to a Unicode string with the text sent.}
		 */

	// Terp net mode set (using VRCL)
	estiMSG_TERP_NET_MODE_SET,
		/*!<
		 * Notify Application that VRCL set the terp net mode (for an indicator on terp vp)
		 *
		 * \MsgParam{A pointer to a Unicode string with the text indicating the mode.}
		 */

	estiMSG_TIME_SET,				//! This message is sent the first time the time is set on the nVP
		/*!<
		 * Notify application that a frame of self video has been captured as an
		 * image in memory.
		 * \MsgParam{A pointer to a SstiCapturedVideoImage that the receiver must cleanup along with the pun8Image field.}
		 */

	estiMSG_RV_TCP_CONNECTION_STATE_CHANGED,	//! This message is sent any time a change occurs in Radvision's TCP connection state

	estiMSG_CONFERENCE_SERVICE_INSUFFICIENT_RESOURCES,	//! Insufficient resources on the MCU to complete the request.
	estiMSG_CONFERENCE_SERVICE_ERROR,	//! Generic error.  Something went wrong with the Conference Service request.

	estiMSG_CONTACT_IMAGES_ENABLED_CHANGED,
	
	estiMSG_HEARING_STATUS_CHANGED, //! Notifies the app that there has been a change to the hearing status.
	
	estiMSG_HEARING_STATUS_SENT, //! Informs the VRCL server that the hearing status was succesfully sent.
	
	estiMSG_NEW_CALL_READY_SENT, //! Informs the VRCL server that the new call ready status was succesfully sent.

	estiMSG_BLOCK_CALLER_ID_CHANGED, //! Notifies the app that there has been a change to the caller id blocked status.
	
	estiMSG_CONFERENCE_ROOM_ADD_ALLOWED_CHANGED,	//! Notifies the UI when the conference room stats have changed.

	estiMSG_CONFERENCING_READY_STATE, //! notifies the application layer when the engine is ready for conferencing
    
	estiMSG_BLOCK_ANONYMOUS_CALLERS_CHANGED, //! Notifies the app that there has been a change to the Block Anonymous Callers status.
	
	estiMSG_NAT_TRAVERSAL_SIP_CHANGED, //! Notifies the app that there has been a change to the NAT Traversal SIP setting.
	
	estiMSG_RINGS_BEFORE_GREETING_CHANGED, //! Notifies the app that there has been a change to the Rings Before Greeting conference parameter.
	
	estiMSG_PLEASE_WAIT, //! Notifies the app that the call has still not connected and to inform the user we are still attempting to connect.
	
	estiMSG_SIGNMAIL_UPLOAD_URL_GET_FAILED, // Notifies the UI that SignMailUploadURLGet returned an error

	// Last (invalid) event
	estiMSG_NEXT						// Not a valid event - used only to set the next private message.

} EstiCmMessage;


stiHResult stiMessageCleanup (
	int32_t n32Message,
	size_t Param);
	
	
std::string stiMessageStringGet (
	int32_t n32Message);

	
#endif // STIMESSAGES_H
