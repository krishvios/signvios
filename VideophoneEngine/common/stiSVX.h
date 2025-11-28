/*!
 * \file stiSVX.h
 * \brief SVX standard definitions header
 *
 *  Sorenson Communications Inc. Confidential. -- Do not distribute
 *  Copyright 2000 - 2011 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STISVX_H
#define STISVX_H

/*
 * Includes
 */
#include "stiDefs.h"
#include "stiMem.h"
#include "stiTrace.h"
#include "stiAssert.h"
#include "stiError.h"
#include <string>
#include <memory>
#include "stiMessages.h"

/*
 * Constants
 */
#define szstiSORENSON_VRS_NAME "Sorenson VRS"

#define RELAY_LANGUAGE_ENGLISH "English"
#define RELAY_LANGUAGE_SPANISH "Spanish"

#define nMAX_MAC_ADDRESS_NO_COLONS_STRING_LENGTH	13 // Maximum length of a MAC Address string without colons.
#define nMAX_MAC_ADDRESS_WITH_COLONS_STRING_LENGTH	18 // Maximum length of a MAC Address string with colons.

#define stiIPV4_ADDRESS_LENGTH 15 // Maximum length of an IPV4 address.
#define stiIPV6_ADDRESS_LENGTH 45 // Maximum length of an IPV6 address.
#define stiIP_ADDRESS_LENGTH stiIPV6_ADDRESS_LENGTH
#define stiIPV6_LINK_LOCAL_PREFIX "fe80::" // IPV6 Prefix for a link local address. 

// The following MPI constants are the minimum picture interval in units
// of 1/29.97 seconds
#define un8stiSQCIF_MPI (uint8_t) 0  // Disable SQCIF until it has been tested further
#define un8stiQCIF_MPI (uint8_t) 1
#define un8stiSIF_MPI (uint8_t) 1
#define un8stiCIF_MPI (uint8_t) 1

// Macro block dimensions for the standard video sizes.
#define unstiSQCIF_MB_ROWS				(unsigned int) 6
#define unstiSQCIF_MB_COLS				(unsigned int) 8
#define unstiQCIF_MB_ROWS				(unsigned int) 9
#define unstiQCIF_MB_COLS				(unsigned int) 11
#define unstiSIF_MB_ROWS				(unsigned int) 15
#define unstiSIF_MB_COLS				(unsigned int) 22
#define unstiCIF_MB_ROWS				(unsigned int) 18
#define unstiCIF_MB_COLS				(unsigned int) 22

// Pixel dimensions for the standard video sizes.
#define unstiSQCIF_ROWS             (unsigned int) 96
#define unstiSQCIF_COLS             (unsigned int) 128
#define unstiQCIF_ROWS              (unsigned int) 144
#define unstiQCIF_COLS              (unsigned int) 176
#define unstiSIF_ROWS               (unsigned int) 240
#define unstiSIF_COLS               (unsigned int) 352
#define unstiCIF_ROWS               (unsigned int) 288
#define unstiCIF_COLS               (unsigned int) 352

#define unstiVGA_ROWS               (unsigned int) 480
#define unstiVGA_COLS               (unsigned int) 640
#define unsti4SIF_ROWS              (unsigned int) 480
#define unsti4SIF_COLS              (unsigned int) 704
#define unsti4CIF_ROWS              (unsigned int) 576
#define unsti4CIF_COLS              (unsigned int) 704
#define unstiNTSC_ROWS              (unsigned int) 480
#define unstiNTSC_COLS              (unsigned int) 720
#define unstiWVGA_ROWS              (unsigned int) 480
#define unstiWVGA_COLS              (unsigned int) 800
#define unstiSVGA_ROWS              (unsigned int) 600
#define unstiSVGA_COLS              (unsigned int) 800
#define unstiHD_ROWS                (unsigned int) 720
#define unstiHD_COLS                (unsigned int) 960
#define unstiXGA_ROWS               (unsigned int) 768
#define unstiXGA_COLS               (unsigned int) 1024
#define unstiWHD_ROWS               (unsigned int) 720
#define unstiWHD_COLS               (unsigned int) 1280
#define unstiFHD_ROWS               (unsigned int) 1080
#define unstiFHD_COLS               (unsigned int) 1440


#define un8stiMAX_DOMAIN_NAME_LENGTH	(uint8_t)  48	//!< Maximum DNS Domain Name Length.
#define un8stiUSER_ID_LENGTH            (uint8_t)  32	//!< Max length of User ID.
#define un8stiMAX_URL_LENGTH            (uint8_t)  128	//!<8 bit max length of a URL.
#define un16stiMAX_URL_LENGTH           (uint16_t) 256	//!<16 bit max length of a URL.
#define un8stiNAME_LENGTH               (uint8_t)  64	// Max length of name
#define un8stiCANONICAL_PHONE_LENGTH    (uint8_t)  50	// Max length of phone#
#define un8stiEMAIL_ADDRESS_LENGTH      (uint8_t)  64	// Max length of an
														// E-mail address
#define un8stiDIAL_STRING_LENGTH		(uint8_t) 128	// Max length of a Dial
														// string
#define un8stiDIRECTORY_SERVER_NAME_LENGTH (uint8_t) 128	// Max length of the
															// directory server
#define un8stiSIP_PROXY_ADDRESS_LENGTH	(uint8_t) 128	// Max length of the
														// SIP Proxy server
														// address.

#define nMINIMUM_NET_SPEED 				64000			// The minimum
														// connection speed upon
														// which we want to work

#define AUTO_SPEED_MIN_START_SPEED_MINIMUM	256000		// The range in kbps permitted
#define AUTO_SPEED_MIN_START_SPEED_MAXIMUM	1536000		// for the minimum starting data
														// rate for autospeed

// The following check is here to ensure that the value of nMINIMUM_NET_SPEED is
// not changed during developement.  The error will flag the problem during
// compilation if it is changed.
#if (64000 > nMINIMUM_NET_SPEED)
	#error - The minimum allowable speed is 64000
#endif

// Level ranges for audio volume
#define un8stiMIN_VOLUME_LEVEL			(uint8_t) 0
#define un8stiMAX_VOLUME_LEVEL			(uint8_t) 31

// Level ranges for video brightness, contrast, hue and saturation
#define un32stiMIN_VIDEO_LEVEL 			(uint32_t) 0
#define un32stiMAX_VIDEO_LEVEL			(uint32_t) 255

// Level ranges for the video record maximum quantizer
#define un32stiMIN_MAX_QUANTIZER		(uint32_t) 1
#define un32stiMAX_MAX_QUANTIZER		(uint32_t) 31

// Level ranges for the video record maximum frames refeshes
#define un16MIN_REFRESHES_LEVEL			(uint16_t) 1
#define un16MAX_REFRESHES_LEVEL			(uint16_t) 255

// Maximum and minimum values for the port base and range
#define un16MAX_PORT_NUMBER				(uint16_t) 65535
#define un16MIN_PORT_RANGE				(uint16_t) 6
#define un16MIN_PORT_BASE				(uint16_t) 2049
#define un16MAX_PORT_BASE				(uint16_t) (un16MAX_PORT_NUMBER - \
													 un16MIN_PORT_RANGE)

// Connection status update interval default in seconds.
#define	un32stiCONNECTION_STATUS_UPDATE_INTERVAL_DEFAULT	(uint32_t) 1


/* The following defines are duplicated by the AppLoader in its
 * stiSystemDefaults file.  If their values change, these changes need to be
 * reflected in the AppLoader's stiSystemDefaults file.
 */
#define	stiPRODUCT_VERSION_LENGTH	24
#define un8stiUPDATE_STRING_LENGTH		(uint8_t) 65	// Maximum update config
														// file or server name
														// length

#define un8stiUPDATE_PRODUCT_LENGTH		(uint8_t) 32	// Maximum length for
														// identifying the
														// product to download.

// Phone number length
#define PHONE_NUMBER_LENGTH		11
#define EXTENSION_NUMBER_LENGTH 6 // When changing this value, make sure to change szEXTENSION_NUMBER_LENGTH TO MATCH
#define szEXTENSION_NUMBER_LENGTH "6" // When changing this value, make sure to change EXTENSION_NUMBER_LENGTH TO MATCH
#define HEARING_PHONE_NUMBER_LENGTH (PHONE_NUMBER_LENGTH + EXTENSION_NUMBER_LENGTH)   // Account for extensions

static const char SUICIDE_LIFELINE[] = "988";

const int webrtcSSV{10};

/* The following constants are used for Camera Control
 *
 */
const int nMAX_VIDEO_SOURCES_ALLOWED = 16;
const int nMAX_VIDEO_SOURCE_DESC_LEN = 16;
const int nPTZ_PAN_CAP = 0x01;  // The ability to send/receive PAN commands
const int nPTZ_TILT_CAP = 0x02; // The ability to send/receive TILT commands
const int nPTZ_ZOOM_CAP = 0x04; // The ability to send/receive ZOOM commands
const int nPTZ_CAPS_MASK = 0x07; // Any of the pan, tilt or zoom
const int nPTZ_COMMANDS_CAP = 0x80; // The ability to send commands

#if APPLICATION == APP_MEDIASERVER
	#define MAX_CALLS 100 // Media Server can have many calls
#else
	#define MAX_CALLS 3 // 1 connected, 1 on hold, and 1 registrar
#endif

#define MAX_HOLDSERVER_CALLS 100 // Holdserver

#define INVALID_DYNAMIC_PAYLOAD_TYPE (-1) // Payload Type initial value indicating dynamic payload has not been set.
#define OLD_RTP_KEEPALIVE_PAYLOAD_ID 66 // Old KeepAlive Payload that is not compatible with rtcp-mux see section 4 of RFC 5761.
#define RTP_KEEPALIVE_PAYLOAD_ID 63 // KeepAlive Payload

/*
 * Typedefs
 */
class CstiCall;
class CstiSipCall;
class CstiProtocolCall;
class CstiSipCallLeg;

using CstiCallSP = std::shared_ptr<CstiCall>;
using CstiCallWP = std::weak_ptr<CstiCall>;
using CstiSipCallSP = std::shared_ptr<CstiSipCall>;
using CstiSipCallWP = std::weak_ptr<CstiSipCall>;
using CstiProtocolCallSP = std::shared_ptr<CstiProtocolCall>;
using CstiProtocolCallWP = std::weak_ptr<CstiProtocolCall>;
using CstiSipCallLegSP = std::shared_ptr<CstiSipCallLeg>;
using CstiSipCallLegWP = std::weak_ptr<CstiSipCallLeg>;


/*! \brief This enumeration defines the possible states of a button */
typedef enum EstiButtonState
{
	estiBUTTON_NOT_PRESSED, //!< The button is not currently pressed.
	estiBUTTON_PRESSED,     //!< The button is currently being pressed.
} EstiButtonState;

/*! \brief This enumeration defines the possible locations for camera control messages
 * to be sent.
 */
typedef enum EstiTerminal
{
	estiTERMINAL_NONE = -1,        /*!< None */
	estiTERMINAL_LOCAL = 0,        /*!< Local Terminal. */
	estiTERMINAL_REMOTE_1 = 1,     /*!< Remote connection 1.  If not in a
									* multi-point call, this simply indicates
									* the remote terminal to which we are connected.
									*/
} EstiTerminal;


/*! \brief This enumeration defines bit masks for camera control.  These can be
 * OR'd together to do combination moves.
 *
 * - Bit 7 = PAN
 * - Bit 6 = Left (0) or Right (1)
 * - Bit 5 = TILT
 * - Bit 4 = Down (0) or Up (1)
 * - Bit 3 = ZOOM
 * - Bit 2 = Out (0) or In (1)
 * - Bit 1 = FOCUS
 * - Bit 0 = Out (1) or In (1)
 */
typedef enum EstiCameraControlBits
{
	estiPAN_LEFT    = 0x80, /*!< Pan left. */
	estiPAN_RIGHT   = 0xc0, /*!< Pan right. */
	estiTILT_UP     = 0x30, /*!< Tilt up. */
	estiTILT_DOWN   = 0x20, /*!< Tilt down. */
	estiZOOM_IN     = 0x0c, /*!< Zoom in. */
	estiZOOM_OUT    = 0x08, /*!< Zoom out. */
	estiFOCUS_IN    = 0x03, /*!< Focus in. */
	estiFOCUS_OUT   = 0x02  /*!< Focus out. */
} EstiCameraControlBits;


/*! \brief The following macros are to help with divisions in code.  Because of
 * restrictions found in the arm processor, divisions are emulated by software.
 * The following macros are divisions by powers of 2 using shifts.
 */
#define stiDIVIDE_BY_TWO(x)			((x) >> 1) /*!< Divide by two */
#define stiDIVIDE_BY_FOUR(x)		((x) >> 2) /*!< Divide by four */
#define stiDIVIDE_BY_EIGHT(x)		((x) >> 3) /*!< Divide by eight */
#define stiDIVIDE_BY_SIXTEEN(x)		((x) >> 4) /*!< Divide by sixteen */

/*! \brief The following macros are for byte swap and only work for 32 bits
* unsigned value
*/
#define stiWORD_BYTE_SWAP(x) ((uint16_t) (((x) >> 8) | ((x) << 8)))
#define stiDWORD_BYTE_SWAP(x) ((uint32_t) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24)))

/*! \brief The EstiMediaType enumeration defines the different data types that can
 * be used by either endpoint during a call.
 */
typedef enum EstiMediaType
{
	estiMEDIA_TYPE_UNKNOWN,
	estiMEDIA_TYPE_VIDEO,
	estiMEDIA_TYPE_AUDIO,
	estiMEDIA_TYPE_TEXT,
} EstiMediaType;

/*! \brief This enumeration defines the various colorspaces that you can encode with.
*/
typedef enum EstiColorSpace
{
	estiCOLOR_SPACE_I420,
	estiCOLOR_SPACE_NV12,
	estiCOLOR_SPACE_422YpCbCr8,
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
	estiCOLOR_SPACE_CM_SAMPLE_BUFFER, // Signal to the hardware encoder that we passed a CMSampleBuffer
#endif
} EstiColorSpace;

/*! \brief The EstiAudioCodec enumeration defines the different codecs that can
 * be used by either endpoint during a call.
 * NOTE: These values are used when storing a preferrerd codec in the property table so
 * changing the value of a given codec name may produce undesired results.
 */
typedef enum EstiAudioCodec
{
	estiAUDIO_NONE = 0,			/*!< Codec not determined yet (the default).
								 * Used in the context of the preferred
								 * codec, this signifies allowing the
								 * engine to select the codec.
								 */
	estiAUDIO_G711_ALAW = 1,			/*!< G.711 ALAW audio codec. */
	estiAUDIO_G711_MULAW = 2,			/*!< G.711 MULAW audio codec. */

	estiAUDIO_G711_ALAW_R = 3,			/*!< Robust G.711 ALAW audio codec. */
	estiAUDIO_G711_MULAW_R = 4,			/*!< Robust G.711 MULAW audio codec. */
	estiAUDIO_G722 = 5,					/*!< G.722 audio codec. */
	estiAUDIO_DTMF = 6,						/*!< DTMF telephone events */
	estiAUDIO_RAW = 7,					/*!< Raw audio codec */
	estiAUDIO_AAC = 8,					/*!< AAC audio codec */
} EstiAudioCodec;

/*! \brief The EstiTextCodec enumeration defines the different codecs that can
 * be used by either endpoint during a call.
 * NOTE: These values are used when storing a preferrerd codec in the property table so
 * changing the value of a given codec name may produce undesired results.
 */
typedef enum EstiTextCodec
{
	estiTEXT_NONE = 0,			/*!< Codec not determined yet (the default).
					 * Used in the context of the preferred
					 * codec, this signifies allowing the
					 * engine to select the codec.
					 */
	estiTEXT_T140 = 1,			/*!< T140 real time text. */
	estiTEXT_T140_RED = 2,		/*!< T140 real time text with redundancy. */
} EstiTextCodec;

/*! \brief This enumeration is used to keep track of the protocol being used. */
typedef enum EstiProtocol
{
	estiPROT_UNKNOWN = 0, /*!< The protocol of the current call is unknown. */
	estiSIP = 2,	/*!< The current call is using the SIP protocol. */
	estiSIPS = 3,	/*!< The current call is using the SIPS protocol. */
} EstiProtocol;

typedef enum EstiVcoType
{
	estiVCO_NONE = 0,
	estiVCO_ONE_LINE = 1,
	estiVCO_TWO_LINE = 2
} EstiVcoType;

/*! \brief This enumeration is used to keep track of the results of a call. */
typedef enum EstiCallResult
{
		estiRESULT_UNKNOWN = 0,                     /*!< No calls made yet - default. */
		estiRESULT_CALL_SUCCESSFUL = 1,             /*!< Call was successfully made. */
		estiRESULT_LOCAL_SYSTEM_REJECTED = 2,       /*!< The local system rejected the call */
		estiRESULT_LOCAL_SYSTEM_BUSY = 3,           /*!< The local system is busy. */
		estiRESULT_NOT_FOUND_IN_DIRECTORY = 4,      /*!< Unable to make call because
											 * the remote system was not
											 * listed in the directory.
											 */
		estiRESULT_DIRECTORY_FIND_FAILED = 5,       /*!< Unable to make call because
											 * the directory service failed
											 * during the entry find process.
											 */
		estiRESULT_REMOTE_SYSTEM_REJECTED = 6,      /*!< The remote system rejected the call */
		estiRESULT_REMOTE_SYSTEM_BUSY = 7,          /*!< The remote system is busy. */
		estiRESULT_REMOTE_SYSTEM_UNREACHABLE = 8,   /*!< The remote system is not
											 * responding.
											 */
		estiRESULT_REMOTE_SYSTEM_UNREGISTERED = 9,  /*!< The remote system is not
											 * registered with the registration
											 * service.
											 */
		estiRESULT_REMOTE_SYSTEM_BLOCKED = 10,       /*!< The remote system has been blocked
											 * from receiving calls.
											 */
		estiRESULT_DIALING_SELF = 11,                /*!< An attempt to dial one's self
											 * has been made.
											 */
		estiRESULT_LOST_CONNECTION = 12,             /*!< Lost connection to remote system. */
		estiRESULT_NO_ASSOCIATED_PHONE = 14,			/*!< No Current or Associated phoneID for current
											 * user record.
											 */
		estiRESULT_NO_P2P_EXTENSIONS = 15,			/*!< Extensions are not supported for point-to-point calls */
		estiRESULT_REMOTE_SYSTEM_OUT_OF_NETWORK = 16, /*!< The remote system is out of network
											 * and InNetworkMode is true.
											 */

		estiRESULT_VRS_CALL_NOT_ALLOWED = 18,	/*!< Hearing users are not allowed to place VRS calls */
		estiRESULT_TRANSFER_FAILED = 19, /*!<Received an error transferring most likely Directory Resolve related*/
		
		estiRESULT_SECURITY_INADEQUATE = 20, /*!< The local system rejected the call because of inadequate security available */
		estiRESULT_ANONYMOUS_CALL_NOT_ALLOWED = 21,	/*!< Callers with caller ID blocked are not allowed to call users with block anonymous callers enabled */
		estiRESULT_LOCAL_HANGUP_BEFORE_ANSWER = 22, /*!< The caller hung up before the remote system was able to answer */
		estiRESULT_LOCAL_HANGUP_BEFORE_DIRECTORY_RESOLVE = 23, /*!< The caller hung up before the directory resolve had completed */
		estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE = 24, /*!< Could not send a Direct SignMail to the number.*/
		estiRESULT_ANONYMOUS_DIRECT_SIGNMAIL_NOT_ALLOWED = 25, /*!< Could not send a Direct SignMail to the number.*/
		estiRESULT_HANGUP_AND_LEAVE_MESSAGE = 26,   /*!< Don't wait, just hang up and leave a SignMail. */
		estiRESULT_REMOTE_SYSTEM_TEMPORARILY_UNAVAILABLE = 27,
		estiRESULT_ENCRYPTION_REQUIRED = 28

} EstiCallResult;

/*! \brief This enumeration defines the call dialing methods. */
typedef enum EstiDialMethod
{
	/*! User supplied dial string. */
	estiDM_BY_DIAL_STRING = 0,

	/*! Phone number supplied, used for look up in a Directory Service like LDAP.
	*/
	estiDM_BY_DS_PHONE_NUMBER = 1,

	/*! Phone number supplied, used for dialing via a Video Relay Service.	*/
	estiDM_BY_VRS_PHONE_NUMBER = 3,

	/*! Phone number supplied, used for dialing via a Video Relay Service using Voice
	Carry Over.
	*/
	estiDM_BY_VRS_WITH_VCO = 4,

	/*! Unknown dial method.  The number may be a hearing number or a deaf number. */
	estiDM_UNKNOWN = 8,
	
//	estiDM_BY_VIDEO_SERVER_NUMBER = 9, /*!< Number supplied is used in the called party field when calling a video server */ -- This dial type has been deprecated.

	/*! Call is placed through another VRS provider */
	estiDM_BY_OTHER_VRS_PROVIDER = 10,

	/*! Unknown dial method. The number may be a hearing number or a deaf number.
	If the number is determined to be a hearing number then use VCO.
	*/
	estiDM_UNKNOWN_WITH_VCO = 11,

	/*! VRS call disconnected before dialing the customer. IVR, HoldServer, Hand-off, Interpreter.
	*/
	estiDM_BY_VRS_DISCONNECTED = 12,
} EstiDialMethod;


/*! \brief This enumeration defines the message types. */
typedef enum EstiMessageType
{
	estiMT_NONE = 0,					/*!< No message type specified. */
	estiMT_SIGN_MAIL = 1,				/*!< Sign mail message type. */
	estiMT_MESSAGE_FROM_SORENSON = 2,	/*!< Message from Sorenson. */
	estiMT_MESSAGE_FROM_OTHER = 3,		/*!< Message from another entity. */
	estiMT_NEWS = 4,					/*!< News item. */
	estiMT_FROM_TECH_SUPPORT = 5,		/*!< Message from Sorenson Tech support. */
	estiMT_P2P_SIGNMAIL = 6,			/*!< Sign mail message type for point-to-point.  */
	estiMT_THIRD_PARTY_VIDEO_MAIL = 7,	/*!< UNUSED */
	estiMT_SORENSON_PRODUCT_TIPS = 8,	/*!< UNUSED */
	estiMT_DIRECT_SIGNMAIL = 9,			/*!< Message sent through Direct SignMail. */
	estiMT_INTERACTIVE_CARE = 10		/*!< Message sent by an Interactive Care Client. */
} EstiMessageType;


/*! \brief This enumeration is used to keep track of the direction of the call
 * and the channels that are negotiated.
 */
typedef enum EstiDirection
{
	estiUNKNOWN = 0,    /*!< Unknown direction ... not yet set. */
	estiINCOMING = 1,	/*!< Call initiated from the remote end. */
	estiOUTGOING = 2,	/*!< Call initiated locally. */
} EstiDirection;


#if 0
/*! \brief This enumeration defines the handset status. */
typedef enum EstiHandsetStatus
{
	estiON_HOOK,	/*!< The handset is on the hook (hung up). */
	estiOFF_HOOK	/*!< The handset is off the hook (in use). */
} EstiHandsetStatus;
#endif


/*!\brief The following enumeration is for events that are common amongst most, if
 * not all, tasks.
 */
enum EstiEvents
{
	estiEVENT_HEARTBEAT = 0,
	/*!< Heart beat message, check if task still alive. */

	estiEVENT_REPORT_ERROR = 1,
	/*!< Report Error message. */

	estiEVENT_SHUTDOWN = 2,
	/*!< Shutdown message, pass to child tasks and stop operations. */

	estiEVENT_NOP = 3,
	/*!< This event is used to kick a thread out of a wait state and
	typically has no handler (it should be handled in the task method).
	*/

	estiEVENT_CALLBACK = 4,

	estiEVENT_TIMER = 5,

	estiEVENT_NEXT,
	/*!< Classes implementing event handlers should use
	this as the value of the first event in the class
	event enumeration.
	*/

};

/*! \brief This enumeration is used to define public IP address assignment methods. */
typedef enum EstiPublicIPAssignMethod
{
	estiIP_ASSIGN_AUTO = 0,		/*!< The system will auto detect the public IP address. */
	estiIP_ASSIGN_MANUAL = 1,	/*!< A manually entered public IP address will be required. */
	estiIP_ASSIGN_SYSTEM = 2	/*!< The system's own IP address will also be
								* considered the public address.
								*/
} EstiPublicIPAssignMethod;

#if 0
/*! \brief The following enumeration is used to tell the state of registration with
 *	a Proxy Server.
 */
typedef enum EstiSipProxyRegState
{
	estiSIP_PROXY_REG_INVALID,      /*!< Registration information was invalid. */
	estiSIP_PROXY_REG_IDLE,         /*!< Initial state for proxy registrations.
									 * It remains in this state until registration
									 * has been attempted.
									 */
	estiSIP_PROXY_REG_TERMINATED,   /*!< Registration has been terminated. */
	estiSIP_PROXY_REG_REGISTERING,  /*!< In the process of registring. */
	estiSIP_PROXY_REG_REDIRECTED,   /*!< Registration has been redirected to a
									 * different proxy server.
									 */
	estiSIP_PROXY_REG_UNAUTHENTICATED, /*!< Authentication is required to register.*/
	estiSIP_PROXY_REG_REGISTERED,   /*!< Registration has been successful. */
	estiSIP_PROXY_REG_FAILED,       /*!< Registration has failed. */
	estiSIP_PROXY_REG_MSG_SEND_FAILURE, /*!< Failure to send registration
										 * message to proxy server.
										 */
} EstiSipProxyRegState;
#endif


/*! \brief This enumeration defines the video capture sizes supported by the
 *	hardware.
 */
typedef enum EstiPictureSize
{
	estiSQCIF,	/*!< Sub-Quarter CIF - 128 x 96 pixels. */
	estiQCIF,	/*!< Quarter CIF - 176 x 144 pixels. */
	estiSIF,    /*!< SIF - 352 x 240 pixels. */
	estiCIF,	/*!< CIF - 352 x 288 pixels. */
	estiCUSTOM,	/*!< Custom picture size. */
	estiFULL_SCREEN /*!< Full Screen mode. */
} EstiPictureSize;


/*! \brief This enumeration is used to define the possible Call States that are
 *  possible.
 */
typedef enum EsmiCallState
{
	/* ***NOTE! If you add a state to this list, DON'T FORGET to add a case
	 * statement to the debug code in CstiCall::BuildStateString!
	 * Call states
	 */
		esmiCS_UNKNOWN        = 0x00000000, // It isn't known.  Used only for reporting to the app
											// when they request a call state but pass an invalid
											// call handle.
		esmiCS_IDLE           = 0x00000001, // The object has been created but not connected
		esmiCS_CONNECTING     = 0x00000002, // The call is being negotiated.
		esmiCS_CONNECTED      = 0x00000004, // The call is currently connected.
		esmiCS_HOLD_LCL       = 0x00000008, // The call is currently on "hold" locally.
		esmiCS_HOLD_RMT       = 0x00000010, // The call is currently on "hold" remotely.
		esmiCS_HOLD_BOTH      = 0x00000020, // The call is currently on "hold" both remotely and locally.
		esmiCS_DISCONNECTING  = 0x00000040, // The call is being disconnected.
		esmiCS_DISCONNECTED   = 0x00000080, // The call has been disconnected.
		esmiCS_CRITICAL_ERROR = 0x00000100, // An unrecoverable error has occured.
		esmiCS_INIT_TRANSFER  = 0x00000200, // We are in the process of initiating a transfer.
		esmiCS_TRANSFERRING   = 0x00000400, // We are in the process of completing a transfer.
} EsmiCallState;

/*! \brief This enumeration is used to keep track of the possible sub-states the
 * conference engine can get into.
 */
typedef enum EstiSubState
{
	/* NOTE:  If you add an substate to this list, DON'T FORGET to add a
	 * case statement to the debug code in CstiConferenceManager::BuildStateString! and CstiCall::BuildStateString
	 */
	estiSUBSTATE_NONE                      = 0x00000000, /*!< Default substate */

	/* Call Substates */
	/* Connecting substates */
	estiSUBSTATE_CALLING                   = 0x00000001, /*!< The local endpoint has begun
														   * a call.  Valid only in the
														   * "Connecting" state.
														   */
	estiSUBSTATE_ANSWERING                 = 0x00000002, /*!< The local endpoint is receiving
														   * an incoming call.  Valid only in
														   * the "Connecting" state.
														   */
	estiSUBSTATE_RESOLVE_NAME              = 0x00000004, /*!< The dial string passed in is
														   * being resolved to a valid dialable
														   * address.  Valid only in the
														   * "Connecting" state.
														   */
	estiSUBSTATE_WAITING_FOR_REMOTE_RESP   = 0x00000008, /*!< The remote endpoint is ringing
														   * and the local endpoint is waiting
														   * for a response.  Valid only in the
														   * "Connecting" state.
														   */
	estiSUBSTATE_WAITING_FOR_USER_RESP     = 0x00000010, /*!< The local endpoint is ringing
														   * and waiting for a response from
														   * the local user.  Valid only in
														   * the "Connecting" state.
														   */

	/* Connected substates */
	estiSUBSTATE_ESTABLISHING              = 0x00000020, /*!< The call has been initially
														   * connected and the channels and
														   * capabilities are being established.
														   * Valid only in the "Connected" state.
														   */
	estiSUBSTATE_CONFERENCING              = 0x00000040, /*!< The call has now been fully
														   * established and is conferencing.
														   * Valid only in the "Connected" state.
														   */
	/* Holding/HoldResume substates
	 * In addition to these substates, the substates for the sub-systems listed above
	 * for MEDIA are also valid in the Holding/HoldResume states.
	 */
	estiSUBSTATE_NEGOTIATING_LCL_HOLD       = 0x00000080, /*!< Negotiating a call hold.  Initiated locally. */
	estiSUBSTATE_NEGOTIATING_RMT_HOLD       = 0x00000100, /*!< Negotiating a call hold.  Initiated remotely. */
	estiSUBSTATE_NEGOTIATING_LCL_RESUME     = 0x00000200, /*!< Negotiating the resume of a call hold.  Initiated locally. */
	estiSUBSTATE_NEGOTIATING_RMT_RESUME     = 0x00000400, /*!< Negotiating the resume of a call hold.  Initiated remotely. */
	estiSUBSTATE_HELD                       = 0x00000800, //!< On hold.

	/* Disconnecting substates */
	estiSUBSTATE_UNKNOWN                    = 0x00001000, /*!< The call is disconnecting
														   * for an unknown reason.  Valid only
														   * in the "Disconnecting" state.
														   */
	estiSUBSTATE_BUSY                       = 0x00002000, /*!< The call is disconnecting
														   * due to the remote endpoint being
														   * busy.  Valid only in the
														   * "Disconnecting" state.
														   */
	estiSUBSTATE_REJECT                     = 0x00004000, /*!< The call is disconnecting
														   * due to the remote endpoint
														   * rejecting the call.  Valid
														   * "only in the Disconnecting"
														   * state.
														   */
	estiSUBSTATE_UNREACHABLE                = 0x00008000, /*!< The call is disconnecting
														   * due to the remote endpoint being
														   * unreachable (e.g. not on-line).
														   * Valid only in the "Disconnecting"
														   * state.
														   */
	estiSUBSTATE_CREATE_VRS_CALL			= 0x00010000, /*!< The call is disconnecting because
														   * the dialed number was not found in
														   * either the ENUM or iTRS databases.
														   * A VRS Call will be created.
														   * Valid only in the "Disconnecting"
														   * state.
														   */
	estiSUBSTATE_LOCAL_HANGUP               = 0x00020000, /*!< The call is disconnecting
														   * due to the local endpoint hanging
														   * up.  Valid only in the
														   * "Disconnecting" state.
														   */
	estiSUBSTATE_REMOTE_HANGUP              = 0x00040000, /*!< The call is disconnecting
														   * due to the remote endpoint
														   * hanging up.  Valid only in the
														   * "Disconnecting" state.
														   */
	estiSUBSTATE_SHUTTING_DOWN              = 0x00080000, /*!< The call is disconnecting
														   * due to the local endpoint
														   * shutting down.  Valid only in
														   * the "Disconnecting" state.
														   */
	estiSUBSTATE_ERROR_OCCURRED             = 0x00100000, /*!< The call is disconnecting
														   * due to an error in the local
														   * system's protocol stack.
														   * Valid only in the
														   * "Disconnecting" state.
														   */
	estiSUBSTATE_LEAVE_MESSAGE              = 0x00200000, /*!< The remote endpoint did not
														   * a call before max number of rings was
														   * reached. Tells the app it can leave
														   * a video message. Valid only in the
														   * "Disconnected" state.
														   */
	estiSUBSTATE_MESSAGE_COMPLETE           = 0x00400000, /*!< The message recording has been compelted.
														   * This state is set so that a message does
														   * not get rerecorded. Valid only in the
														   * "Disconnected" state.
														   */
	estiSUBSTATE_TRANSPORT_CONNECTED		= 0x00800000, /*!< The H.245 channel has been established and
														   * is now ready for communications over it.
														   */

	/* ***NOTE! If you add an substate to this list, DON'T FORGET to add a
	 * case statement to the debug code in
	 * CstiConferenceManager::StateStringBuild!
	 */
} EstiSubState;


/*! \brief The EstiVideoCodec enumeration defines the different codecs that can
 * be used by either endpoint during a call.
 * NOTE: These values are used when storing a preferrerd codec in the property table so
 * changing the value of a given codec name may produce undesired results.
 */
typedef enum EstiVideoCodec
{
	estiVIDEO_NONE = 0, /*!< Video codec not determined
						 * (default value).
						 */
//    estiVIDEO_H261 = 1, /*!< H.261 video codec. */
	estiVIDEO_H263 = 2, /*!< H.263 video codec. */
	estiVIDEO_H264 = 3, /*!< H.264 video codec. */
//    estiVIDEO_SRV1_UNUSED = 4, /*!< Sorenson Robust Video v1 codec (H.263). */
//    estiVIDEO_SRV2_UNUSED = 5, /*!< Sorenson Robust Video v2 codec (H.264). */
//    estiVIDEO_SRV3 = 6, /*!< Sorenson Robust Video v3 codec (H.264 w/proprietary bitstream). */
//	estiVIDEO_CAMERA_CONTROL = 7,
	estiVIDEO_H265 = 8, /*!< H.265 video codec. */
	estiVIDEO_RTX = 9 /*! < Retransmission codec. */
} EstiVideoCodec;

/*! \brief This new enumeration of the video codecs will replace the one above in the near future.
 * At this point in time, it is only used to allow the application layer to tell the conference
 * manager which codecs to offer to a non-sorenson device.
 * The EstiVideoCodec2 enumeration defines the different codecs that can
 * be used by either endpoint during a call.
 * NOTE: These values are used when storing a preferrerd codec in the property table so
 * changing the value of a given codec name may produce undesired results.
 */
typedef enum EstiVideoCodec2
{
	estiVIDEOCODEC_NONE = 0, /*!< Video codec not determined
						 * (default value).
						 */
//    estiVIDEOCODEC_H261 = (1 << 0), /*!< H.261 video codec. */
	estiVIDEOCODEC_H263 = (1 << 1), /*!< H.263 video codec. */
	estiVIDEOCODEC_H264 = (1 << 2), /*!< H.264 video codec. */
	estiVIDEOCODEC_H265 = (1 << 3), /*!< h.265 video codec. */
//    estiVIDEOCODEC_SRV3 = (1 << 3), /*!< Sorenson Robust Video v3 codec (H.264 w/proprietary bitstream). */
	estiVIDEOCODEC_ALL = 0xffff /*!<Used to signify support for all codecs the given endpoint supports.  */
} EstiVideoCodec2;

/*! \brief The EstiPacketizationScheme enumeration defines the different packetization methods that
 *  can be used for the codecs.
 */
typedef enum EstiPacketizationScheme
{
	estiPktUnknown,
	estiH263_Microsoft, /*!< Initial industry standard method */
	estiH263_RFC_2190, /*!< h.263 ratified method */
	estiH263_RFC_2429, /*!< h.263+ or 1998 method */
	estiH264_SINGLE_NAL, /*!< required for H.264, default method */
	estiH264_NON_INTERLEAVED, /*!< Support for NAL, STAP-A, and FU-A types */
	estiH264_INTERLEAVED, /*!< Support for STAP-B, MTAP16, MTAP24, FU-A and FU-B types */
	estiH265_NON_INTERLEAVED, /*!< the transmission order of NAL units is the same as their decoding order */
	estiPktNone,
} EstiPacketizationScheme;


/*!
 * \brief The RtcpFeedbackType enum defines the different RTCP feedback mechanisms we can use
 */
enum RtcpFeedbackType
{
	UNKNOWN,
	TMMBR,   // Used to notify remote endpoint of new max rate
	FIR,     // Used for keyframe requests not due to packet loss
	PLI,      // Used for Keyframe requests due to packet loss
	NACKRTX	// Used to signal lost packets and retransmit missing data
};


/*! \brief This enumeration is used as a mask to express which capture sizes are
 * capable of being requested of the remote system.
 */
typedef enum EstiVideoPlaybackCtrlMask
{
	estiVP_NONE		= 0x0000,   /*!< None. */
	estiVP_SQCIF	= 0x0001,   /*!< Sub QCif capable */
	estiVP_QCIF		= 0x0002,   /*!< QCif capable */
	estiVP_CIF		= 0x0004,   /*!< Cif capable */
	estiVP_CIF4		= 0x0008,   /*!< 4 x Cif capable */
	estiVP_CIF16	= 0x0010,   /*!< 16 x Cif capable */
	estiVP_CUSTOM	= 0x1000,   /*!< Custom picture size capable */
} EstiVideoPlaybackControlMask;

/*! \brief This enumeration defines the video refresh intervals.  Parts of the
 *	video picture doesn't get updated because nothing changes (nothing moves or
 *	changes in that part of the picture).  The refresh interval forces those
 *	parts of the video picture to refresh (even if nothing changes) every
 *	interval.
 */
typedef enum EstiVideoRefreshRate
{
	estiNEVER_REFRESH,				/*!< Never force refreshes */
	estiLOW_RATE,					/*!< Refresh every 511 frames */
	estiMEDIUM_RATE,				/*!< Refresh every 255 frames */
	estiHIGH_RATE,					/*!< Refresh every 127 frames */
} EstiVideoRefreshRate;

/*! \brief This enumeration is used to communicate DTMF signals received or to
 * be sent between endpoints.
 */
typedef enum EstiDTMFDigit
{
	estiDTMF_DIGIT_0,   /*!< Telephone key "1". */
	estiDTMF_DIGIT_1,   /*!< Telephone key "2". */
	estiDTMF_DIGIT_2,   /*!< Telephone key "3". */
	estiDTMF_DIGIT_3,   /*!< Telephone key "4". */
	estiDTMF_DIGIT_4,   /*!< Telephone key "5". */
	estiDTMF_DIGIT_5,   /*!< Telephone key "6". */
	estiDTMF_DIGIT_6,   /*!< Telephone key "7". */
	estiDTMF_DIGIT_7,   /*!< Telephone key "8". */
	estiDTMF_DIGIT_8,   /*!< Telephone key "9". */
	estiDTMF_DIGIT_9,   /*!< Telephone key "0". */
	estiDTMF_DIGIT_S,   /*!< Telephone key "*". */
	estiDTMF_DIGIT_P    /*!< Telephone key "#". */
} EstiDTMFDigit;

/*! \brief This enumeration is used to communicate to the device which tone
 * to play.
 */
typedef enum EstiToneMode
{
	estiTONE_NONE,					/*!< Stop all tones. */
	estiTONE_RING,					/*!< Start the ringing tone. */
	estiTONE_BUSY,					/*!< Start the busy signal tone. */
	estiTONE_FAST_BUSY				/*!< Start fast busy signal tone. */
} EstiToneMode;

/*! \brief This enumeration is used to define the hold mode for true call waiting
 */
typedef enum EstiHoldMode
{
	estiHOLD_NONE = 0,
	estiHOLD_NEAREND,				/*!< Near-end hold. */
	estiHOLD_FAREND					/*!< Far-end hold. */
} EstiHoldMode;

typedef struct SstiUserPhoneNumbers
{
	char szPreferredPhoneNumber[PHONE_NUMBER_LENGTH + 1];	// Holds Preferred phone number
	char szSorensonPhoneNumber[PHONE_NUMBER_LENGTH + 1];	// Holds Sorenson phone number
	char szTollFreePhoneNumber[PHONE_NUMBER_LENGTH + 1];	// Holds Toll-free phone number
	char szLocalPhoneNumber[PHONE_NUMBER_LENGTH + 1];	// Holds local phone number
	char szHearingPhoneNumber[HEARING_PHONE_NUMBER_LENGTH + 1];	// Holds the hearing callers number
	char szRingGroupLocalPhoneNumber[PHONE_NUMBER_LENGTH + 1];	// Holds the ring group local phone number
	char szRingGroupTollFreePhoneNumber[PHONE_NUMBER_LENGTH + 1];	// Holds the ring group local phone number
} SstiUserPhoneNumbers;

typedef struct SstiGvcParticipant
{
	std::string Id;
	std::string PhoneNumber;
	std::string Type;  // The known type to the MCU (generally will be "ad_hoc")
} SstiGvcParticipant;



enum EstiPropertyType
{
	estiPTypeUser,
	estiPTypePhone
};


enum ESSLFlag
{
	eSSL_ON_WITH_FAILOVER = 0,
	eSSL_OFF = 1,
	eSSL_ON_WITHOUT_FAILOVER = 2
};

typedef enum EsmdEchoMode
{
	esmdAUDIO_ECHO_OFF,					// echo cancellation off
	esmdAUDIO_ECHO_ON,					// echo cancellation on
} EsmdEchoMode;

//
// Generic call back function pointer type
//
typedef stiHResult (stiSTDCALL *PstiAppGenericCallback) (
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam);

//
// Generic call back function pointer type
//
typedef stiHResult (*PstiObjectCallback) (
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t CallbackFromId);

//
// Structure used to pass the various ports and port ranges to use in the protocol stacks
//
struct SstiConferencePortsSettings
{
	EstiBool bUseSecondSIPListenPort;
	EstiBool bUseAltSIPTlsListenPort;
	int nSecondSIPListenPort;
	int nAltSIPTlsListenPort;
//	int nUPnPPortsStart;
//	int nUPnPPortsEnd;
};


struct SstiConferencePortsStatus
{
	EstiBool bUseAltSIPListenPort;
	int nAltSIPListenPort;
	EstiBool bUseAltSIPTlsListenPort;
	int nAltSIPTlsListenPort;
};


enum
{
	estiDEVICE_TYPE_SVRS_HOLD_SERVER = 1,
	estiDEVICE_TYPE_SVRS_VIDEO_SERVER = 2,
	estiDEVICE_TYPE_SVRS_VIDEOPHONE = 3,
	estiDEVICE_TYPE_SVRS_DEVICE = 4,			///< A videphone, hold server or video server
	estiDEVICE_TYPE_SVRS_VP200 = 5
};


enum EstiSecureCallMode
{
	estiSECURE_CALL_NOT_PREFERRED = 0,
	estiSECURE_CALL_PREFERRED = 1,
	estiSECURE_CALL_REQUIRED = 2
};

enum class DeviceLocationType
{
	NotSet = -1,
	Update = 0,
	SharedWorkspace = 1,
	PrivateWorkspace = 2,
	SharedLivingSpace = 3,
	PrivateLivingSpace = 4,
	WorkIssuedMobileDevice = 5,
	OtherSharedSpace = 6,
	OtherPrivateSpace = 7,
	Prison = 8,
	Restricted = 9,
};

//
// NAT Traversal constants (for ICE and H.460).
//
enum EstiSIPNATTraversal
{
	estiSIPNATDisabled = 0,
	estiSIPNATEnabledMediaRelayDisabled = 1,
	estiSIPNATEnabledMediaRelayAllowed = 2,
	estiSIPNATEnabledUseTunneling = 3
};

struct SstiConferenceAddresses
{
	SstiConferenceAddresses () = default;
	
	//
	// Equal operator
	//
	bool operator == (
		const SstiConferenceAddresses &Other)
	{
		return (SIPPublicAddress == Other.SIPPublicAddress
		 && nSIPPublicPort == Other.nSIPPublicPort
		 && SIPPrivateAddress == Other.SIPPrivateAddress
		 && nSIPPrivatePort == Other.nSIPPrivatePort);
	}
	
	//
	// Not equal operator
	//
	bool operator != (
		const SstiConferenceAddresses &Other)
	{
		return (SIPPublicAddress != Other.SIPPublicAddress
		 || nSIPPublicPort != Other.nSIPPublicPort
		 || SIPPrivateAddress != Other.SIPPrivateAddress
		 || nSIPPrivatePort != Other.nSIPPrivatePort);
	}
	
	std::string SIPPublicAddress;
	int nSIPPublicPort = 0;
	std::string SIPPrivateAddress;
	int nSIPPrivatePort = 0;
};


enum EstiChannelState
{
	estiCHANNEL_STATE_IDLE,
	estiCHANNEL_STATE_ACTIVE,
	estiCHANNEL_STATE_NEGOTIATING,
};


// The profile supported by the video codec.  The hex value of the H.264 profiles is what is used to signal in the SDP
enum EstiVideoProfile
{
	estiPROFILE_NONE	= 0,
	estiH263_ZERO		= 1,
	estiH264_BASELINE	= 0x42,  // 66
	estiH264_MAIN		= 0x4d,  // 77
	estiH264_EXTENDED	= 0x58,  // 88
	estiH264_HIGH		= 0x64,  // 100
	estiH265_PROFILE_MAIN		= 1, //Default 8 bit depth
	estiH265_PROFILE_MAIN10		= 2  //10 bit depth (Improves video quality at no cost w/ higher bitrates)
/*
 The following are not implemented in the conference engine.
	estiH264_HIGH_10	= 0x6e,  // 110
	estiH264_HIGH_4_2_2	= 0x7a,  // 122
	estiH264_HIGH_4_4_4	= 0xf4,  // 244
 */
};

struct SstiProfileAndConstraint
{
	SstiProfileAndConstraint () = default;

	EstiVideoProfile eProfile{estiPROFILE_NONE};
	uint8_t un8Constraints{0};
};

enum EstiH264Level
{
	estiH264_LEVEL_1 = 10,
/*	estiH264_LEVEL_1_B = ??, Not implemented in the conference engine. */
	estiH264_LEVEL_1_1 = 11,
	estiH264_LEVEL_1_2 = 12,
	estiH264_LEVEL_1_3 = 13,
	estiH264_LEVEL_2 = 20,
	estiH264_LEVEL_2_1 = 21,
	estiH264_LEVEL_2_2 = 22,
	estiH264_LEVEL_3 = 30,
	estiH264_LEVEL_3_1 = 31,
	estiH264_LEVEL_3_2 = 32,
	estiH264_LEVEL_4 = 40,
	estiH264_LEVEL_4_1 = 41,
	estiH264_LEVEL_4_2 = 42,
	estiH264_LEVEL_5 = 50,
	estiH264_LEVEL_5_1 = 51,
};

//These numbers are the level * 30.  Because we want it as a readable integer we are using the level * 10 * 3
// From the ietf payload rtp h265 07 document:
// If no tier-flag is present, a value of 0 MUST be inferred and if no level - id is present, a value of 93 (i.e.level 3.1) MUST be inferred.
// The value of level-id MUST be in the range of 0 to 255, inclusive.

enum EstiH265Level
{
	estiH265_LEVEL_1 = 10 * 3,
	estiH265_LEVEL_2 = 20 * 3, //352 x 288 @ 30 fps
	estiH265_LEVEL_2_1 = 21 * 3, //640 x 360 @ 30 fps
	estiH265_LEVEL_3 = 30 * 3, //960 x 540 @ 30 fps
	estiH265_LEVEL_3_1 = 31 * 3, //1280 x 720 @ 33.7 fps
	estiH265_LEVEL_4 = 40 * 3, //1920 x 1080 @ 32 fps
	estiH265_LEVEL_4_1 = 41 * 3, //1920 x 1080 @ 64 fps
	estiH265_LEVEL_5 = 50 * 3, //4096 x 2160 @ 30 fps
	estiH265_LEVEL_5_1 = 51 * 3,
	estiH265_LEVEL_5_2 = 52 * 3,
	estiH265_LEVEL_6 = 60 * 3,
	estiH265_LEVEL_6_1 = 61 * 3,
	estiH265_LEVEL_6_2 = 62 * 3,
	estiH265_LEVEL_8_5 = 85 * 3 //Final Level 
};

enum EstiH265Tier
{
	estiH265_TIER_MAIN = 0, //Default Tier for H265
	//estiH265_TIER_HIGH = 1, //Generally not used for streaming video
	
};


enum EstiAutoSpeedMode
{
	estiAUTO_SPEED_MODE_LEGACY = 0,
	estiAUTO_SPEED_MODE_LIMITED = 1,
	estiAUTO_SPEED_MODE_AUTO = 2
};

struct SstiRecordedMessageInfo
{
	std::string toPhoneNumber;
	std::string messageId;
	uint32_t un32MessageSize{0};
	uint32_t un32MessageSeconds{0};
	uint32_t un32MessageBitrate{0};
};

enum eSignMailGreetingType
{
	eGREETING_NONE,
	eGREETING_DEFAULT,
	eGREETING_PERSONAL,
	eGREETING_PERSONAL_TEXT,
	eGREETING_TEXT_ONLY
};

enum eAnnounceH2DPreference
{
	eANNOUNCE_H2D_RELAY = 0,
	eANNOUNCE_H2D_NONE,
	eANNOUNCE_H2D_CUSTOM
};

enum eAnnounceH2DTextSelection
{
	eH2D_OPTION_DEFAULT = 0,
	eH2D_OPTION_EXTENDED
};

enum eAnnounceD2HPreference
{
	eANNOUNCE_D2H_RELAY = 0,
	eANNOUNCE_D2H_NONE,
	eANNOUNCE_D2H_CUSTOM
};

enum eProfileImagePrivacyLevel
{
	ePUBLIC = 0,
	ePHONEBOOK_ONLY
};

struct SstiPersonalGreetingPreferences
{
	std::string szGreetingText;
	EstiBool bGreetingEnabled;
	eSignMailGreetingType eGreetingType;
	eSignMailGreetingType ePersonalType;
};

//! \brief Interface mode of the videophone.
typedef enum EstiInterfaceMode
{
	// **IMPORTANT!!** DO NOT change the values of these enumerations!! If more
	// are needed, they must be added at the end of the list! If one value is no
	// longer used, just put on a comment on the line indicating that it is
	// unused.
	estiSTANDARD_MODE = 0,		//!< Can make all types of calls (e.g. IP, VRS, VCO).
	estiPUBLIC_MODE = 1,		/*!< Can make all types of calls but is not allowed to
								* access settings.
								*/

	estiKIOSK_MODE = 2,			/*!< Restricted to VRS calls, not allowed to access
								* settings.
								*/

	estiINTERPRETER_MODE = 3,	/*!< Same ability as "Standard Mode" + logged in as an
								* interpreter, which changes what is sent as
								* "Display Name" and how missed calls are handled.
								* On the VP200, some additional UI changes have been
								* made to limit the user (e.g. settings).
								*/

	estiTECH_SUPPORT_MODE = 4,	/*!< Same ability as "Standard Mode" except the "Display
								* Name" and return address are changed to obscure who
								* they are.
								*/

	estiVRI_MODE = 5,			/*!< No outbound calling ability, */

	estiABUSIVE_CALLER_MODE = 6, /*!< No outbound calling ability and no VRS inbound calls */

	estiPORTED_MODE = 7,		/*!< Ported phone number, so no associated SVRS account, no communication */

	estiHEARING_MODE = 8,		/*~< Hearing phone number*/
	// ^^^ Add new items above this line!
	estiINTERFACE_MODE_ZZZZ_NO_MORE

} EstiInterfaceMode;


typedef enum EstiEmergencyAddressProvisionStatus
{
	// **IMPORTANT!!** DO NOT change the values of these enumerations!! If more
	// are needed, they must be added at the end of the list! If one value is no
	// longer used, just put on a comment on the line indicating that it is
	// unused.
	estiPROVISION_STATUS_NOT_SUBMITTED,
	estiPROVISION_STATUS_SUBMITTED,
	estiPROVISION_STATUS_VPUSER_VERIFY,	// need to verify an update made on back end
	estiPROVISION_STATUS_UNKNOWN,

	// ^^^ Add new items above this line!
	estiPROVISION_STATUS_ZZZZ_NO_MORE,
} EstiEmergencyAddressProvisionStatus;


//
// This structure is used to contain URI information returned from Core.
//
struct SstiURIInfo
{
	enum EstiNetwork
	{
		estiUnknown = 0,
		estiSorenson = (1 << 0),
		estiITRS = (1 << 1)
	};
	
	EstiNetwork eNetwork{estiUnknown};
	std::string URI;
};

#define MAX_STATS_PACKET_POSITIONS	10


enum EstiDialSource
{
	estiDS_UNKNOWN = 0,
	estiDS_CONTACT = 1,
	estiDS_CALL_HISTORY = 2,
	estiDS_SIGNMAIL = 3,
	estiDS_FAVORITE = 4,
	estiDS_AD_HOC = 5, // This is meant to be used when the user is entering digits directly into the dial field using a dial pad, keyboard or remote control
	estiDS_VRCL = 6,
	estiDS_SVRS_BUTTON = 7,
	estiDS_RECENT_CALLS = 8,
	//estiDS_TRANSFERRED = 9,  We cannot log transferred as it will over write the original dial source on out going calls
	estiDS_INTERNET_SEARCH = 10, // Internet Search
	estiDS_WEB_DIAL = 11,
	estiDS_PUSH_NOTIFICATION_MISSED_CALL = 12,
	estiDS_PUSH_NOTIFICATION_SIGNMAIL_CALL = 13,
	estiDS_LDAP = 14,
	estiDS_FAST_SEARCH = 15,  //This is specific to mac and pc it searches your contacts and recent calls with the dial pad
	estiDS_MYPHONE = 16, //This is when performing Add Call to a MyPhone Group number (intercom)
	estiDS_UI_BUTTON = 17, // This is making a call from any UI Button like a Call CIR Dialog or Techsupport in settings.
	estiDS_SVRS_ENGLISH_CONTACT = 18,
	estiDS_SVRS_SPANISH_CONTACT = 19,
	estiDS_DEVICE_CONTACT = 20, // This is when a call is placed from the Device Contacts UI (iOS)
	estiDS_DIRECT_SIGNMAIL = 21, // The user has placed a call from a direct SignMail message item
	estiDS_DIRECT_SIGNMAIL_FAILURE = 22,	// When a direct SignMail fails to send, the user is presented with a call button
											// to call them instead.  This DialSource indicates they clicked that button.
	estiDS_CALL_HISTORY_DISCONNECTED = 23,  // This is when a call in call history that has been disconnected before an interpreter sees the call is called back.
	estiDS_RECENT_CALLS_DISCONNECTED = 24,  // This is when a call in the recent call list that has been disconnected before an interpreter sees the call is called back.
	estiDS_NATIVE_DIALER = 25,  // This is when a call is placed from outside the application and is intercepted and processed by our app.
	estiDS_BLOCKLIST = 26,
	estiDS_CALL_WITH_ENCRYPTION = 27,
	estiDS_DIRECTORY = 28,
	estiDS_SPANISH_AD_HOC = 29,
	estiDS_HELP_UI = 30,
	estiDS_LOGIN_UI = 31,
	estiDS_ERROR_UI = 32,
	estiDS_CALL_CUSTSERVICE_UI = 33
};

enum EstiSharedTextSource
{
	estiSTS_UNKNOWN = 0,
	estiSTS_KEYBOARD = 1,
	estiSTS_SAVED_TEXT = 2,
};

/*!
 * \struct SstiVideoCapabilities
 *
 * \brief Video Capabilities flag
 */
struct SstiVideoCapabilities
{
	SstiVideoCapabilities () = default;

	SstiVideoCapabilities (const SstiVideoCapabilities &other) = default;
	SstiVideoCapabilities (SstiVideoCapabilities &&other) = default;
	SstiVideoCapabilities &operator= (const SstiVideoCapabilities &other) = default;
	SstiVideoCapabilities &operator= (SstiVideoCapabilities &&other) = default;

	virtual ~SstiVideoCapabilities() = default;

	std::list<SstiProfileAndConstraint> Profiles;  //! A list of profiles supported by the decoder along with their constraints.
};

/*!
 * \struct SstiH264Capabilities
 *
 * \brief
 */
struct SstiH264Capabilities : SstiVideoCapabilities
{
	SstiH264Capabilities () = default;

	SstiH264Capabilities (const SstiH264Capabilities &other) = default;
	SstiH264Capabilities (SstiH264Capabilities &&other) = default;
	SstiH264Capabilities &operator= (const SstiH264Capabilities &other) = default;
	SstiH264Capabilities &operator= (SstiH264Capabilities &&other) = default;

	~SstiH264Capabilities () override = default;

	EstiH264Level eLevel{estiH264_LEVEL_1_1};  //! The supported level.
	uint32_t	unCustomMaxMBPS{0}; //! 0 for default or custom maximum macroblocs/second upgrade (in units of 500)
	uint32_t 	unCustomMaxFS{0};	//! 0 for default or custom maximum frame size upgrade (in units of 256-macroblocks)
/*
 *  The following are not implemented in the conference engine.
	int nCustomMaxDPB;
	int nCustomMaxBRandCPB;
	int nMaxStaticMBPS;
	int nMaxRcmdNalUnitSize;
	int nMaxNalUnitSize;
	int nSampleAspectRatiosSupported;
	int nAdditionalModesSupported;
 */
};

/*!
* \struct SstiH265Capabilities
*
* \brief
*/
struct SstiH265Capabilities : SstiVideoCapabilities
{
	SstiH265Capabilities() = default;

	SstiH265Capabilities (const SstiH265Capabilities &other) = default;
	SstiH265Capabilities (SstiH265Capabilities &&other) = default;
	SstiH265Capabilities &operator= (const SstiH265Capabilities &other) = default;
	SstiH265Capabilities &operator= (SstiH265Capabilities &&other) = default;

	~SstiH265Capabilities () override = default;

	EstiH265Level eLevel{estiH265_LEVEL_3_1};  //! The supported level.
	EstiH265Tier eTier{estiH265_TIER_MAIN};
};


/*!\brief Media transport types
 *
 */
enum EstiMediaTransportType
{
	estiMediaTransportUnknown = 0,
	estiMediaTransportHost = 1,
	estiMediaTransportReflexive = 2,
	estiMediaTransportRelayed = 3,
	estiMediaTransportPeerReflexive = 4
};

/*!\brief Hearing call statuses
 *
 */
enum EstiHearingCallStatus
{
	estiHearingCallDisconnected = 0,
	estiHearingCallConnecting = 1,
	estiHearingCallConnected = 2
};


/*!\brief Reflects the ready state of the engine for conferencing
 *
 */
enum EstiConferencingReadyState
{
	estiConferencingStateNotReady = 0,
	estiConferencingStateReady = 1
};

/*!brief ICE support status
 *
 */
enum class IceSupport
{
	Unknown = -1,
	Supported = 0,
	NotSupported = 1,
	Mismatch = 2,
	ReportedMismatch = 3,
	TrickleIceMismatch = 4,
	Error = 5,
	Unspecified = 6,
};


/*!\brief Result of starting location service on device
 *
 */
enum class GeolocationAvailable
{
	NotDetermined = 0,				// Geolocation was never requested during the call
	Available = 1,					// Geolocation was requested and is available
	Unavailable = 2,				// Geolocation was requested but the device doesn't support Google Play Services (Android only)
	DisabledOnDevice = 3,			// Geolocation was requested but location services are disabled on the device
	ApplicationNotAuthorized = 4	// Geolocation was requested but location services are disallowed for the ntouch application
};

enum class ProductType
{
	Unknown,
	iOS,
	Android,
	iOSDHV,
	AndroidDHV,
	iOSFBC,
	iOSVRI,
	PC,
	Mac,
	MediaServer,
	MediaBridge,
	HoldServer,
	LoadTestTool,
	VP2,
	VP3,
	VP4,
};


/*!\brief Indicates when to signal a codec or other feature.
 *
 */
enum class SignalingSupport
{
	Disabled = 0,
	SorensonOnly = 1,
	Everyone = 2
};


enum class UserLoginCheckResult
{
	UserNameOrPasswordInvalid = 0,
	NotLoggedIn = 1,
	LoggedInOnCurrentDevice = 2,
	LoggedInOnDifferentDevice = 3,
};


enum class NetworkType
{
	None = 0,
	Wired = 1,
	WiFi = 2,
	Cellular = 3,
};

namespace vpe
{
	/*!
	 * \brief encryption state enum
	 *
	 * This is intended for the UI layer to use to show
	 * the encrypted state of the call.
	 */
	enum class EncryptionState
	{
		NONE = 0,
		AES_128 = 1,
		AES_256 = 2,
	};

	enum class HashFuncEnum
	{
		Undefined,
		Sha1,
		Sha224,
		Sha256,
		Sha384,
		Sha512,
		Md5,
		Md2,
	};

	enum class KeyExchangeMethod
	{
		None = 0,
		DTLS = 1,
		SDES_AES_CM_128_HMAC_SHA1_80 = 2,
		SDES_AES_256_CM_HMAC_SHA1_80 = 3,
	};
}

/*!\brief Standard port numbers
 *
 */

static constexpr int STANDARD_HTTP_PORT = 80;
static constexpr int STANDARD_HTTPS_PORT = 443;
static constexpr int STANDARD_LDAP_PORT = 389;
static constexpr int STANDARD_SIP_PORT = 5060;
static constexpr int STANDARD_SIP_TLS_PORT = 5061;


/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */

#endif /* STISVX_H */
/* end file stiSVX.h  */
