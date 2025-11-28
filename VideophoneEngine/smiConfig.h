/*!
 * \file smiConfig.h
 * \brief Defines options for a particular build configuration
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2003 - 2011 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#ifndef _SMICONFIG_H_
#define _SMICONFIG_H_

#define APP_VP100			0
#define APP_NTOUCH_PC		4
#define APP_NTOUCH_MAC		5
#define APP_HOLDSERVER		6
#define APP_NTOUCH_IOS		7
#define APP_NTOUCH_ANDROID	8
#define APP_NTOUCH_VP2		9
#define APP_NTOUCH_VP3		10
#define APP_MEDIASERVER     11
#define APP_REGISTRAR_LOAD_TEST_TOOL 12
#define APP_DHV_PC			13
#define APP_DHV_IOS		14
#define APP_DHV_ANDROID	15
#define APP_NTOUCH_VP4		16

//PASS This in as a compiler define for every file
//#define APPLICATION APP_NTOUCH_PC

#ifndef APPLICATION
//#error "APPLICATION not defined"
#endif

#define DEV_X86             0
#define DEV_NTOUCH_PC       4
#define DEV_NTOUCH_MAC      5
#define DEV_NTOUCH_ANDROID	6
#define DEV_NTOUCH_VP2		7
#define DEV_NTOUCH_IOS		8
#define DEV_NTOUCH_VP3		9
#define DEV_DHV_PC			10
#define DEV_DHV_IOS		11
#define DEV_DHV_ANDROID	12
#define DEV_NTOUCH_VP4		13

#define SUBDEV_NONE		0
#define SUBDEV_MPU		1
#define SUBDEV_RCU		2

//
// Basic defines
//
#ifndef stiOS
#define stiOS          //!< Use the OS Abstraction Layer
#endif //stiOS

#ifndef WIN32
#define __LINUX__      //!< Required to enable Linux build options in the RadVision SIP toolkit
#define _REDHAT        //!< Required to enable RedHat build options in the RadVision SIP toolkit
#endif //WIN32
// Define RV mode if it hasn't already been defined
#if !defined(RV_RELEASE) && !defined(RV_DEBUG)
#define RV_RELEASE     //!< Required for release build with the Radvision stack
#endif

#ifdef WIN32
#define stiSTDCALL __stdcall
#else
#define stiSTDCALL
#endif

//
// Memory check tools
//
//#define stiMEM_CHECK_TOOLS		//!< Turn on memory check tools
//#define stiMEM_CHECK_LINKED_LIST
//#define stiMUTEX_DEBUGGING
//#define stiMUTEX_DEBUGGING_BACKTRACE

//
// Feature defines
//

//
// VRS options
//

//
// Core Services options
//

//
// Uncomment to add videomail support
//
#define stiVIDEOMAIL             //!< Enable videomail
//#define stiP2P_VIDEOMAIL         //!< Enable Point-to-point videomail

// Disable lip synchronization
//#define stiNO_LIP_SYNC         //!< Disable lip synchronization

// UI defines
//#define smiUI_TESTS              //!< Enable test code in UI.
#define smiEXIT_SHIFT_Q                 //!< Enable exit on shift-q key.

// Enable Premium Services support
// #define smiPREMIUM_SERVICES
// #define stiPREMIUM_SERVICES

// Disable Lost Connection Detection
//#define stiDISABLE_LOST_CONNECTION

// Enable 911 support in the videophone
#define sti911_SUPPORT_ENABLED

// Set the default SSL mode
#define stiSSL_DEFAULT_MODE eSSL_ON_WITH_FAILOVER

//
// Protocol dial and registration defaults
//
#define stiNAT_TRAVERSAL_SIP_DEFAULT estiSIPNATDisabled

#define stiSIP_TRANSPORT_DEFAULT estiTransportTCP
#define stiSIP_NAT_TRANSPORT_DEFAULT estiTransportTCP
#define stiIPV6_ENABLED_DEFAULT 0
#define stiSECURE_CALL_MODE_DEFAULT estiSECURE_CALL_NOT_PREFERRED

#define stiTUNNEL_SERVER_DNS_NAME_DEFAULT "tu.ci.svrs.net"
#define stiTURN_SERVER_USER_NAME_DEFAULT "ntouch"
#define stiTURN_SERVER_PASSWORD_DEFAULT "Td8dx5sy"

#define stiSIP_AGENT_DOMAIN_OVERRIDE_DEFAULT "agent.na.sip.svrs.net"
#define stiSIP_AGENT_DOMAIN_ALT_OVERRIDE_DEFAULT "agent.na.sip.svrs.net"

#define stiSIP_UNRESOLVED_DOMAIN "unresolved.sorenson.com"

#define stiENDPOINT_MONITOR_SERVER_DEFAULT "https://ems.svrs.cc/ws"

// Socket receive buffer size
#define stiSOCK_RECV_BUFF_SIZE 8192

// Number of Out of Order packet buffers
#define stiOUTOFORDER_VP_BUFFERS 512

#define stiPLAYER_CONTROLS_ENABLED_DEFAULT	0
#define stiSIGNMAIL_ENABLED_DEFAULT	0
#define stiBLOCK_LIST_ENABLED_DEFAULT	false
#define stiCONTACT_PHOTOS_ENABLED_DEFAULT	1
#define stiPSMG_ENABLED_DEFAULT	1
#define stiDTMF_ENABLED_DEFAULT	1
#define stiREAL_TIME_TEXT_ENABLED_DEFAULT	1
#define stiCALL_TRANSFER_ENABLED_DEFAULT	1
#define stiAGREEMENTS_ENABLED_DEFAULT	1
#define stiFAVORITES_ENABLED_DEFAULT	1
#define stiBLOCK_CALLER_ID_ENABLED_DEFAULT 1
#define stiLANGUAGE_FEATURES_PREFERENCE_DEFAULT "EN"
#define stiNEW_CALL_ENABLED_DEFAULT 1
#define stiINTERNET_SEARCH_ENABLED_DEFAULT 1
#define stiDIRECT_SIGNMAIL_ENABLED_DEFAULT 1
#define stiRINGS_BEFORE_GREETING_DEFAULT 5

#define stiMAX_RECV_SPEED_DEFAULT	256000
#define stiMAX_SEND_SPEED_DEFAULT	256000

#define stiMAX_AUTO_RECV_SPEED_DEFAULT	1536000
#define	stiMAX_AUTO_SEND_SPEED_DEFAULT	1536000

#define stiAUTO_SPEED_ENABLED_DEFAULT	1
#define stiAUTO_SPEED_MODE_DEFAULT	estiAUTO_SPEED_MODE_AUTO
#define stiAUTO_SPEED_MIN_START_SPEED_DEFAULT	256000

//
// Maximum SDP message size we intend to receive.
// If MAX_SDP_MESSAGE_SIZE is changed then RV_SDP_MSG_BUFFER_SIZE (defined in rvsdpconfig.h) should also be updated to match.
//
#define MAX_SDP_MESSAGE_SIZE (1024 * 16)

//
// Maximum size of SIP message we can receive.
//
#define MAX_SIP_MESSAGE_SIZE (MAX_SDP_MESSAGE_SIZE + 1024)

#define stiVCO_PREFERRED_TYPE_DEFAULT estiVCO_NONE

#define stiVIDEO_CHANNELS_CAPABLE_DEFAULT	0
#define stiRING_GROUP_CAPABLE_DEFAULT	0
#define stiRING_GROUP_ENABLED_DEFAULT	true
#define stiRING_GROUP_MODE_DEFAULT	IstiRingGroupManager::eRING_GROUP_DISPLAY_MODE_READ_ONLY

#define stiENABLE_SIPS_DIALING 1
#define stiENABLE_ICE
#define stiENABLE_AES_SECURITY 1

#define ADD_PLUS_TO_PHONENUMBER

// Use "0.0.0.0" for IP address
#define stiUSE_ZEROS_FOR_IP

#define stiVRS_SERVER_DEFAULT "hold.sorensonvrs.com"
#define stiVRS_ALT_SERVER_DEFAULT "hold.sorenson2.biz"
#define SPANISH_VRS_SERVER_DEFAULT "spanish.svrs.tv"
#define SPANISH_VRS_ALT_SERVER_DEFAULT "spanish.svrs.tv"
#define stiVRS_FAILOVER_TIMEOUT_DEFAULT 10
#define stiPLEASE_WAIT_TIMEOUT_DEFAULT 5

#define stiTECH_SUPPORT_URI_DEFAULT "sip:18012879403@tech.svrs.tv"
#define stiCUSTOMER_INFORMATION_URI_DEFAULT "sip:18667566729@cir.svrs.tv"

#define stiREMOTE_LOG_EVENT_SEND_DEFAULT	0
#define stiREMOTE_LOG_CALL_STATS_DEFAULT	0
#define stiREMOTE_LOG_TRACE_DEFAULT		0
#define stiREMOTE_LOG_ASSERT_DEFAULT	1

//
// Core, StateNotify, and Message server defaults
//
#define CORE_SERVICE_DEFAULT_A 0
#define CORE_SERVICE_DEFAULT_B 2
#define CORE_SERVICE_DEFAULT_C 4
#define CORE_SERVICE_DEFAULT_PRODUCTION 8

#define CORE_SERVICE_DEFAULT CORE_SERVICE_DEFAULT_PRODUCTION

#define stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT 0
#define stiLDAP_ENABLED_DEFAULT false

#define CLIENT_REREGISTER_MAX_LOOKUP_TIME_DEFAULT (24 * 60 * 60) // Max time to wait, in seconds, before complete DNS and re-registration
#define CLIENT_REREGISTER_MIN_LOOKUP_TIME_DEFAULT (20 * 60 * 60) // Max time to wait, in seconds, before complete DNS and re-registration

#define stiSTACK_RESTART_DELTA_DEFAULT (10) // Time to wait to restart stack after registration expires if needed.

#define stiFIR_FEEDBACK_ENABLED_DEFAULT 1   // Default value for allowing keyframe requests via FIR RTCP feedback (demand for keyframe)
#define stiPLI_FEEDBACK_ENABLED_DEFAULT 1   // Default value for allowing keyframe requests via PLI RTCP feedback (packet loss indicator)
#define stiAFB_FEEDBACK_ENABLED_DEFAULT 0   // Default value for allowing AFB messages (application feedback)
#define stiTMMBR_FEEDBACK_ENABLED_DEFAULT 2 // Default value for allowing media rate control via TMMBR RTCP feedback
#define stiNACKRTX_FEEDBACK_ENABLED_DEFAULT 2	// Default value for allowing NACK RTCP feedback and RTX of missing data

#define stiHEVC_SIGNMAIL_PLAYBACK_ENABLED_DEFAULT	1 //Default value for HEVC SignMailPlayback

// For boost::ptree XML parsing
#define BOOST_PROPERTY_TREE_RAPIDXML_STATIC_POOL_SIZE 0

#if BOOST_VERSION < 105900	// After 1.59 we should no longer need this.
	#define BOOST_SPIRIT_THREADSAFE
#endif

// For Interops ...
//#define stiENABLE_CALL_HOLD_TOGGLE
//#define stiINTEROP_EVENT

#define stiDHV_API_KEY_DEFAULT "5nQz9RXfLc7W5dVe3XfPI5JuBgqQv0xK29X4lm8A"

#define stiDEAF_HEARING_VIDEO_ENABLED_DEFAULT 1
#define stiDHV_CONNECTING_TIMEOUT_DEFAULT 40  // Seconds to wait before disconnecting a DHV call in the connecting state.

#define MINIMUM_PACKET_SEND_DELAY_USEC	0

#define DEVICE_LOCATION_TYPE_DEFAULT DeviceLocationType::NotSet

/////////////////////////////////////////////////////////////////////////////////
//
// Defines for the ntouch VP2 and ntouch VP3
//
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4

	#define stiLINUX
	#define SM_LITTLE_ENDIAN

	#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
	#define stiDEBUG                //!< Turn on ASSERT for debugging purposes

	// Enable Contact Photos
	#define stiCONTACT_PHOTOS

	// Enable Core 2.0-style contact lists
	#define stiCONTACT_LIST_MULTI_PHONE         // Multiple phone numbers per contact

	// Enable Message Manager
	#define stiMESSAGE_MANAGER

	// Enable Tunnel Manager
	#define stiTUNNEL_MANAGER

	#define stiTUNNEL_MAX_SEND_RECV_BIT_RATE 1536000

	#ifdef stiUSE_ZEROS_FOR_IP
	#undef stiUSE_ZEROS_FOR_IP
	#endif // stiUSE_ZEROS_FOR_IP

	// Enable Image Manager
	#define stiIMAGE_MANAGER

	// Enable Call History Manager
	#define stiCALL_HISTORY_MANAGER
	#define stiCALL_HISTORY_COMPRESSED
	#undef stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT
	#define stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT 1
	#define stiUSE_MONOTONIC_CLOCK

	// Enable Favorites
    #define stiFAVORITES

	#define OPENSSL_NO_KRB5
	#undef stiSSL_DEFAULT_MODE
	#define stiSSL_DEFAULT_MODE eSSL_ON_WITHOUT_FAILOVER

#if APPLICATION == APP_NTOUCH_VP2
	#define TSC_DDT
	#define PERSISTENT_DATA	"/data/ntouchvp2"
	#define STATIC_DATA "/usr/share/ntouchvp2"
	#define PRODUCT_TYPE ProductType::VP2
#elif  APPLICATION == APP_NTOUCH_VP3
	#define PERSISTENT_DATA	"/var/ntouchvp3"
	#define STATIC_DATA "/usr/share/lumina"
	#define PRODUCT_TYPE ProductType::VP3
#else
	#define PERSISTENT_DATA	"/var/ntouchvp4"
	#define STATIC_DATA "/usr/share/lumina"
	#define PRODUCT_TYPE ProductType::VP4
#endif

	#define stiWIRELESS

//		#define stiSIGNMAIL_AUDIO_ENABLED

	#define stiLDAP_CONTACT_LIST
	#define stiALLOW_SHARE_CONTACT 1

	// Enable version 2 of the RingGroupManager implementation.
	#define stiRING_GROUP_MANAGER_v2

	#define stiDISABLE_PM_VERSION_CHECK

	#define stiSIGNMAIL_ENABLED

	#undef stiNAT_TRAVERSAL_SIP_DEFAULT
	#define stiNAT_TRAVERSAL_SIP_DEFAULT estiSIPNATEnabledMediaRelayAllowed
	
	//
	// TURN server address default
	//
	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "vpstun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "vpstun.sorenson2.biz"
	
	//
	// Set the SIP transport to TCP
	//
	#undef stiSIP_TRANSPORT_DEFAULT
	#define stiSIP_TRANSPORT_DEFAULT estiTransportTCP

#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	#undef MINIMUM_PACKET_SEND_DELAY_USEC
	#define MINIMUM_PACKET_SEND_DELAY_USEC	50
#endif

	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE (128 * 1024) // 128kB

	#undef stiSOCK_SEND_BUFF_SIZE
	#define stiSOCK_SEND_BUFF_SIZE	(128 * 1024) // 128kB
	
	#undef stiPLAYER_CONTROLS_ENABLED_DEFAULT
	#define stiPLAYER_CONTROLS_ENABLED_DEFAULT	1
	
	#undef stiSIGNMAIL_ENABLED_DEFAULT
	#define stiSIGNMAIL_ENABLED_DEFAULT	1
	
	#undef stiBLOCK_LIST_ENABLED_DEFAULT
	#define stiBLOCK_LIST_ENABLED_DEFAULT	true

	//#define RCU_REPORT_FRAME_RATES
	//#define RCU_REPORT_DROPPED_FRAMES

//    #define stiVIDEO_CRC_CHECK

	// Define method for capturing system/memory stats
	#define stiSTATS_MEMORY_METHOD_2

#if APPLICATION == APP_NTOUCH_VP2
	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	1024000

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	1024000
#else
	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	1536000

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	1536000
#endif

	#undef stiMAX_AUTO_RECV_SPEED_DEFAULT
	#define stiMAX_AUTO_RECV_SPEED_DEFAULT	(5 * 1024 * 1024) // 5Mbps

	#undef stiMAX_AUTO_SEND_SPEED_DEFAULT
	#define	stiMAX_AUTO_SEND_SPEED_DEFAULT	(5 * 1024 * 1024) // 5Mbps

	#undef stiAUTO_SPEED_MODE_DEFAULT
	#define stiAUTO_SPEED_MODE_DEFAULT	estiAUTO_SPEED_MODE_LIMITED
	
	#define stiMUTEX_DEBUGGING_BACKTRACE

    #define stiLOW_VISION
	
	#undef stiVCO_PREFERRED_TYPE_DEFAULT
	#define stiVCO_PREFERRED_TYPE_DEFAULT estiVCO_TWO_LINE

	#undef stiVIDEO_CHANNELS_CAPABLE_DEFAULT
	#define stiVIDEO_CHANNELS_CAPABLE_DEFAULT	1

	#undef stiRING_GROUP_CAPABLE_DEFAULT
	#define stiRING_GROUP_CAPABLE_DEFAULT	1

	#define stiENABLE_TLS
	
	#undef stiSIP_NAT_TRANSPORT_DEFAULT
	#define stiSIP_NAT_TRANSPORT_DEFAULT estiTransportTLS
	
	#undef stiREMOTE_LOG_EVENT_SEND_DEFAULT
	#define stiREMOTE_LOG_EVENT_SEND_DEFAULT	1
	#undef stiREMOTE_LOG_CALL_STATS_DEFAULT
	#define stiREMOTE_LOG_CALL_STATS_DEFAULT	1
	#undef stiHEVC_SIGNMAIL_PLAYBACK_ENABLED_DEFAULT
	#define stiHEVC_SIGNMAIL_PLAYBACK_ENABLED_DEFAULT 0
	
	#undef stiDHV_API_KEY_DEFAULT
	#define stiDHV_API_KEY_DEFAULT "A0V7OOQxwn3rHZR7fsSij6xeEdR4Di5I6vgH6s7i"
	#define LDAP_DEPRECATED 1

#endif // APP_NTOUCH_VP2, APP_NTOUCH_VP3, APP_NTOUCH_VP4

/////////////////////////////////////////////////////////////////////////////////
//
// Defines for the ntouch PC
//
#if APPLICATION == APP_NTOUCH_PC

	#define X264_LINK_STATIC

	//Enable Internet Search
	#define	stiINTERNET_SEARCH

	#undef stiENABLE_H265_DECODE
	#define stiENABLE_H265_DECODE
	#undef stiENABLE_H265_ENCODE
	#define stiENABLE_H265_ENCODE

	#define SM_LITTLE_ENDIAN

	#define stiTUNNEL_MANAGER

	#define stiENABLE_TLS

	#define stiENABLE_ICE
	
	#define DEVICE DEV_NTOUCH_PC

	#define stiFAVORITES

	#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
#ifdef _DEBUG
	#define stiDEBUG                //!< Turn on ASSERT for debugging purposes
#endif
	#define SSL_CERT_DIR ".\\"

	#undef stiNAT_TRAVERSAL_SIP_DEFAULT
	#define stiNAT_TRAVERSAL_SIP_DEFAULT estiSIPNATEnabledMediaRelayAllowed

	#undef stiSIP_NAT_TRANSPORT_DEFAULT
	#define stiSIP_NAT_TRANSPORT_DEFAULT estiTransportTLS
	
	//
	// TURN server address default
	//
	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "mstun.sorenson.com"
	//ovveride <TURNServer type="string">65.37.249.212<\TURNServer>
	
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "mstun.sorenson2.biz"

	//
	// Enable Contact Photos
	//
	#define stiCONTACT_PHOTOS

	#define stiSIGNMAIL_ENABLED

	// Enable Core 2.0-style contact lists
	#define stiCONTACT_LIST_MULTI_PHONE

	#define stiCALL_HISTORY_MANAGER

	//Removes the <Time> tag when saving CallListItem objects to core.
	//This makes it so core will set the timestamp to the current time on the server.
	#define stiFORCE_CORE_TIMESTAMP
	
	#undef stiSSL_DEFAULT_MODE
	#define stiSSL_DEFAULT_MODE eSSL_ON_WITHOUT_FAILOVER
	
	#undef stiSTATS_MEMORY_METHOD_1
	#undef stiMUTEX_DEBUGGING
	//#define stiMUTEX_DEBUGGING

	#define PERSISTENT_DATA	"."

	#define stiDISABLE_UPDATE
	#define stiDISABLE_SDK_NETWORK_INTERFACE
	#define stiENABLE_EXTERNAL_TRANSPORT
	#define stiENABLE_H264

	#undef  stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	1024000
//768000
	
	#undef  stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	1024000
//1536000



	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE	262144 // 1024kB 65536
	// Socket send buffer size
	// Socket size will not be modified from OS default if this is not defined
	#undef stiSOCK_SEND_BUFF_SIZE
	#define stiSOCK_SEND_BUFF_SIZE	262144 // 1024kB  65536
	//#define stiDISABLE_STUN
	#define stiENCRYPT_XML_FILES
	#define stiDISABLE_NECC
#ifdef _DEBUG
	#define stiDEBUG_OUTPUT
	//#define stiDEBUG_OUTPUT_NET
	#define stiDEBUG_OUTPUT_CONF_MGR
	#define stiDEBUG_OUTPUT_CONF_MGR_EXT
	//#define stiDEBUG_OUTPUT_PLATFORM
	//#define stiDEBUG_OUTPUT_RTP
	//#define stiDEBUG_OUTPUT_RV
	//
	// stiDISABLE_RV_LOG - Also needs #undef RV_NOLOG
	//                                #define RV_NOLOG 1
	// in Radvision/Common/Configuration Files/rvusrconfig.h
	//
	//#define stiDISABLE_RV_LOG 
#endif
	#undef stiVCO_PREFERRED_TYPE_DEFAULT
	#define stiVCO_PREFERRED_TYPE_DEFAULT estiVCO_TWO_LINE
	#define stiMESSAGE_MANAGER
	#define stiDISABLE_PORTED_MODE
	#undef stiDISABLE_CAMERA_CONTROL

	#undef stiSIGNMAIL_ENABLED_DEFAULT
	#define stiSIGNMAIL_ENABLED_DEFAULT	1

	#undef stiBLOCK_LIST_ENABLED_DEFAULT
	#define stiBLOCK_LIST_ENABLED_DEFAULT	true

	#undef stiVIDEO_CHANNELS_CAPABLE_DEFAULT
	#define stiVIDEO_CHANNELS_CAPABLE_DEFAULT	1

	#undef stiRING_GROUP_CAPABLE_DEFAULT
	#define stiRING_GROUP_CAPABLE_DEFAULT	1

	#undef stiREMOTE_LOG_EVENT_SEND_DEFAULT
	#define stiREMOTE_LOG_EVENT_SEND_DEFAULT 1

	#undef stiREMOTE_LOG_CALL_STATS_DEFAULT
	#define stiREMOTE_LOG_CALL_STATS_DEFAULT 1

	#undef stiALLOW_SHARE_CONTACT
	#define stiALLOW_SHARE_CONTACT

	#define stiDISABLE_CAMERA_CONTROL

	#undef BOOST_PROPERTY_TREE_RAPIDXML_STATIC_POOL_SIZE

	#undef stiDHV_API_KEY_DEFAULT
	#define stiDHV_API_KEY_DEFAULT "9Vw8QhdKj79dGwGM77tqY9spQRGtsqEHaUVW4ukx"

	#undef stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT
	#define stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT 1
#endif // APP_NTOUCH_PC

#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_ANDROID
#define stiMVRS_APP
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Defines for MVRS
//
#ifdef stiMVRS_APP

	#ifdef stiMUTEX_DEBUGGING
	#undef stiMUTEX_DEBUGGING
	#endif

	#define stiLINUX
	#define SM_LITTLE_ENDIAN

	#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
	// stiDEBUG defined in project settings for iOS
	// #define stiDEBUG             //!< Turn on ASSERT for debugging purposes

	#undef stiSSL_DEFAULT_MODE
	#define stiSSL_DEFAULT_MODE eSSL_ON_WITHOUT_FAILOVER

	#undef stiNAT_TRAVERSAL_SIP_DEFAULT
	#define stiNAT_TRAVERSAL_SIP_DEFAULT estiSIPNATEnabledMediaRelayAllowed

	#undef stiSIP_TRANSPORT_DEFAULT
	#define stiSIP_TRANSPORT_DEFAULT estiTransportTCP

	#define stiSIGNMAIL_ENABLED

	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE 256000

	// Socket send buffer size
	#undef stiSOCK_SEND_BUFF_SIZE
	#define stiSOCK_SEND_BUFF_SIZE	256000

	#undef MINIMUM_PERCENTAGE_OF_ACTUAL_VS_TARGET_BITRATE_TO_FLOW_UP
	#define MINIMUM_PERCENTAGE_OF_ACTUAL_VS_TARGET_BITRATE_TO_FLOW_UP 0.5

	#undef stiPLAYER_CONTROLS_ENABLED_DEFAULT
	#define stiPLAYER_CONTROLS_ENABLED_DEFAULT	1

	#undef stiSIGNMAIL_ENABLED_DEFAULT
	#define stiSIGNMAIL_ENABLED_DEFAULT	1

	// Multiple phone numbers per contact
	#define stiCONTACT_LIST_MULTI_PHONE

	// Enable Message Manager
	#define stiMESSAGE_MANAGER

	// Enable Favorites
	#define stiFAVORITES

	//Enable Internet Search
	#define	stiINTERNET_SEARCH

	// Enable Call History manager
	#define stiCALL_HISTORY_MANAGER

	// Enable Image Manager
	//#define stiIMAGE_MANAGER

	//Removes the <Time> tag when saving CallListItem objects to core.
	//This makes it so core will set the timestamp to the current time on the server.
	#define stiFORCE_CORE_TIMESTAMP

	#define stiDISABLE_UPDATE
	//	#define stiREFERENCE_DEBUGGING
	#define stiDISABLE_SDK_NETWORK_INTERFACE
	#define stiDISABLE_PORTED_MODE
	#define stiDISABLE_CAMERA_CONTROL
	#undef stiVCO_PREFERRED_TYPE_DEFAULT
	#define stiVCO_PREFERRED_TYPE_DEFAULT estiVCO_ONE_LINE
	#define stiENABLE_SIP_VRS_CALL_FAILOVER

	#define PERSISTENT_DATA	"/data/ntouch"

	#undef stiVIDEO_CHANNELS_CAPABLE_DEFAULT
	#define stiVIDEO_CHANNELS_CAPABLE_DEFAULT	1
	
	#undef stiRING_GROUP_CAPABLE_DEFAULT
	#define stiRING_GROUP_CAPABLE_DEFAULT	1

	#undef stiBLOCK_LIST_ENABLED_DEFAULT
	#define stiBLOCK_LIST_ENABLED_DEFAULT	true

	#undef stiHAS_LIBGD // allows text sharing on video

	#define stiENABLE_TLS

	#undef stiSIP_NAT_TRANSPORT_DEFAULT
	#define stiSIP_NAT_TRANSPORT_DEFAULT estiTransportTLS

	// stiDISABLE_RV_LOG - Also needs #undef RV_NOLOG
	//                                #define RV_NOLOG 1
	// in Radvision/Common/Configuration Files/rvusrconfig.h
	//
	#define stiDISABLE_RV_LOG
	#undef RV_NOLOG
	#define RV_NOLOG 1

	#define CORE_SERVICE_DEFAULT CORE_SERVICE_DEFAULT_PRODUCTION
#ifdef stiDEBUG
	#undef CORE_SERVICE_DEFAULT
	#define CORE_SERVICE_DEFAULT CORE_SERVICE_DEFAULT_C
// For debug builds allow for Radvision logging
	#undef RV_NOLOG
	#undef stiDISABLE_RV_LOG
#endif

	#define stiTUNNEL_MANAGER
	#define TSC_DDT

	#define stiCONTACT_PHOTOS

	#define stiENCRYPT_XML_FILES

	#undef stiREMOTE_LOG_EVENT_SEND_DEFAULT
	#define stiREMOTE_LOG_EVENT_SEND_DEFAULT 1

	#undef stiREMOTE_LOG_CALL_STATS_DEFAULT
	#define stiREMOTE_LOG_CALL_STATS_DEFAULT 1

	#define stiALLOW_SHARE_CONTACT

	#undef stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT
	#define stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT 1
#endif // stiMVRS_APP

////////////////////////////////////////////////////////////////////////////////
//
// Defines for the ntouch iOS
//
#if APPLICATION == APP_NTOUCH_IOS

	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "istun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "istun.sorenson2.biz"
	
	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	512000

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	512000

    // Disabled to fix issues with P3 calls. (Bug #14925)
	//#define stiENABLE_H46017

	//#define stiENABLE_LIGHT_CONTROL
	//#define stiENABLE_VP_REMOTE

	#undef stiHAS_LIBGD // disable libgd on iOS
	#define stiLDAP_CONTACT_LIST

	#define DEVICE DEV_NTOUCH_IOS

	#define stiCALL_HISTORY_COMPRESSED

	#define stiPULSE_ENABLED_DEFAULT 0

	#undef stiDHV_API_KEY_DEFAULT
	#define stiDHV_API_KEY_DEFAULT "JuClN74CHi2CTYTAAV8wo2BezMbvS7483byGxwoR"

	#define stiDISABLE_DNS_CACHE_STORE
	#define stiUSE_GOOGLE_DNS

#endif

////////////////////////////////////////////////////////////////////////////////
//
// Defines for the ntouch Mac
//
#if APPLICATION == APP_NTOUCH_MAC
	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "mstun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "mstun.sorenson2.biz"
	
	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	1024000

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	1024000

	#undef stiMAX_AUTO_RECV_SPEED_DEFAULT
	#define stiMAX_AUTO_RECV_SPEED_DEFAULT	6144000
	#undef stiMAX_AUTO_SEND_SPEED_DEFAULT
	#define	stiMAX_AUTO_SEND_SPEED_DEFAULT	6144000

	#undef stiLDAP_CONTACT_LIST
	#define stiLDAP_CONTACT_LIST

	#define DEVICE DEV_NTOUCH_MAC

	#undef stiENABLE_H265_DECODE
	#define stiENABLE_H265_DECODE
	#undef stiENABLE_H265_ENCODE
	#define stiENABLE_H265_ENCODE
	#define stiPULSE_ENABLED_DEFAULT 0
	#define stiUSE_GOOGLE_DNS
	
	#undef stiDHV_API_KEY_DEFAULT
	#define stiDHV_API_KEY_DEFAULT "xrkgjVUJBp3KKlnKEug6n41JGRVgMzLx7wMeaQxm"
	
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Defines for the ntouch Android
//
#if APPLICATION == APP_NTOUCH_ANDROID

    #undef PERSISTENT_DATA
	#define PERSISTENT_DATA	"/data/data/com.sorenson.mvrs.android/files"

	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "astun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "astun.sorenson2.biz"
	
	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	256000

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	256000

	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE 256000

    // Disabled to fix issues with P3 calls. (Bug #14925)
	//#define stiENABLE_H46017

	#define stiLDAP_CONTACT_LIST

	#define DEVICE DEV_NTOUCH_ANDROID

	//Some of our users are getting IPV6 DNS which cause the app to be unable
	//to register.  If we use Google DNS we don't have this issue.
	#define stiUSE_GOOGLE_DNS
	#define X264_LINK_STATIC
	#define LDAP_DEPRECATED 1

//	#define stiENABLE_H265_ENCODE
//	#undef stiENABLE_H265_DECODE
//	#define stiENABLE_H265_DECODE

	#undef stiDHV_API_KEY_DEFAULT
	#define stiDHV_API_KEY_DEFAULT "Sm4PSkMypL3n6ri7hX0gK1AgVi1wYUcx3ARWQsOd"

#endif


#if APPLICATION == APP_DHV_ANDROID || APPLICATION == APP_DHV_IOS
#define stiMVRS_APP
#undef stiENABLE_ICE // We're always unregistered so we have to disable ICE
#endif


#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC

#define NO_AUDIO_TIMESTAMP_SUPPORT

#ifdef stiDEBUG
	#undef stiTECH_SUPPORT_URI_DEFAULT
	#define stiTECH_SUPPORT_URI_DEFAULT "sip:18012879403@vrs-eng.qa.sip.svrs.net"
	#undef stiCUSTOMER_INFORMATION_URI_DEFAULT
	#define stiCUSTOMER_INFORMATION_URI_DEFAULT "sip:18667566729@vrs-eng.qa.sip.svrs.net"
	#undef stiTUNNEL_SERVER_DNS_NAME_DEFAULT
	#define stiTUNNEL_SERVER_DNS_NAME_DEFAULT "tu.ci-qa.svrs.net"
	#undef stiSSL_DEFAULT_MODE
	#define stiSSL_DEFAULT_MODE eSSL_OFF
	#undef stiVRS_SERVER_DEFAULT
	#define stiVRS_SERVER_DEFAULT "vrs-eng.qa.sip.svrs.net"
	#undef stiVRS_ALT_SERVER_DEFAULT
	#define stiVRS_ALT_SERVER_DEFAULT "vrs-eng.qa.sip.svrs.net"
#endif
#endif


////////////////////////////////////////////////////////////////////////////////
//
// Defines for the media server
//
////////////////////////////////////////////////////////////////////////////////
#if APPLICATION == APP_MEDIASERVER

#undef stiALLOW_SHARE_CONTACT

#ifndef stiMEDIASERVER
#define stiMEDIASERVER
#endif //stiMEDIASERVER

#ifndef stiENABLE_MULTIPLE_VIDEO_INPUT
#define stiENABLE_MULTIPLE_VIDEO_INPUT
#endif //stiENABLE_MULTIPLE_VIDEO_INPUT

#ifndef stiENABLE_MULTIPLE_VIDEO_OUTPUT
#define stiENABLE_MULTIPLE_VIDEO_OUTPUT
#endif //stiENABLE_MULTIPLE_VIDEO_OUTPUT

#ifndef stiENABLE_MULTIPLE_AUDIO_OUTPUT
#define stiENABLE_MULTIPLE_AUDIO_OUTPUT
#endif //stiENABLE_MULTIPLE_AUDIO_OUTPUT

#ifndef stiDISABLE_CAMERA_CONTROL
#define stiDISABLE_CAMERA_CONTROL
#endif //stiDISABLE_CAMERA_CONTROL

#ifdef stiVRS // stiVRS features not needed for the Media Server
#undef stiVRS
#endif // stiVRS 

#define SM_LITTLE_ENDIAN

//#define stiTUNNEL_MANAGER

//#define stiENABLE_TLS
#define stiENABLE_ICE

//
// TURN server address default
//
#undef stiTURN_SERVER_DNS_NAME_DEFAULT
#define stiTURN_SERVER_DNS_NAME_DEFAULT "mstun.sorenson.com"
#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "mstun.sorenson2.biz"

#ifndef stiENABLE_H264
#define stiENABLE_H264
#endif // stiENABLE_H264

#define DEVICE DEV_X86

#ifndef PROTOTYPES  // flag for md5 prototype macros
#define PROTOTYPES 1
#endif

//
// Tools define
//
#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
#define stiDEBUG                //!< Turn on ASSERT for debugging purposes
//#define DEBUG_SIP_LOG_TO_FILE

//#ifdef stiMUTEX_DEBUGGING
//#undef stiMUTEX_DEBUGGING
//#endif

//
// Feature defines
//
//
// Video Format Make sure the correct value is defined
//
#define stiFORMAT_NTSC
// #define stiFORMAT_PAL

// Disable Lost Connection Detection
#define stiDISABLE_LOST_CONNECTION

#ifndef stiDISABLE_PROPERTY_MANAGER
#define stiDISABLE_PROPERTY_MANAGER
#endif

#define X264_LINK_STATIC

#endif //APP_MEDIASERVER

#if APPLICATION == APP_DHV_PC

#undef stiALLOW_SHARE_CONTACT

#ifndef stiDISABLE_CAMERA_CONTROL
#define stiDISABLE_CAMERA_CONTROL
#endif //stiDISABLE_CAMERA_CONTROL

#ifdef stiVRS // stiVRS features not needed for the Media Server
#undef stiVRS
#endif // stiVRS

#define stiENABLE_TLS
#define stiENABLE_ICE

//
// TURN server address default
//
#undef stiTURN_SERVER_DNS_NAME_DEFAULT
#define stiTURN_SERVER_DNS_NAME_DEFAULT "mstun.sorenson.com"
#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "mstun.sorenson2.biz"

#ifndef stiENABLE_H264
#define stiENABLE_H264
#endif // stiENABLE_H264

#define DEVICE DEV_X86

//
// Tools define
//
#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
#define stiDEBUG                //!< Turn on ASSERT for debugging purposes


// Disable Lost Connection Detection
#define stiDISABLE_LOST_CONNECTION

#ifndef stiDISABLE_PROPERTY_MANAGER
#define stiDISABLE_PROPERTY_MANAGER
#endif

#define X264_LINK_STATIC

#define DEVICE DEV_DHV_PC

#endif //APP_MEDIASERVER

//////////////////////////
// DHV iOS
//
#if APPLICATION == APP_DHV_IOS

	#undef stiALLOW_SHARE_CONTACT

	#ifndef stiDISABLE_CAMERA_CONTROL
	#define stiDISABLE_CAMERA_CONTROL
	#endif //stiDISABLE_CAMERA_CONTROL

	#ifdef stiVRS
	#undef stiVRS
	#endif // stiVRS

	#define stiENABLE_TLS
//	#define stiENABLE_ICE

	//
	// TURN server address default
	//
	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "vpstun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "mstun.sorenson2.biz"

	#ifndef stiENABLE_H264
	#define stiENABLE_H264
	#endif // stiENABLE_H264

	#ifndef stiDISABLE_PROPERTY_MANAGER
	#define stiDISABLE_PROPERTY_MANAGER
	#endif

	#define stiLINUX

	#define DEVICE DEV_DHV_IOS

	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE 256000

	// Socket send buffer size
	#undef stiSOCK_SEND_BUFF_SIZE
	#define stiSOCK_SEND_BUFF_SIZE	256000

	//#define stiTUNNEL_MANAGER

	#undef CORE_SERVICE_DEFAULT
	#ifdef stiDEBUG
		#define stiREMOTELOGGER_SERVICE_DEFAULT1 "https://remotelogger-uat.sorensonqa.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		#define stiREMOTELOGGER_SERVICE_DEFAULT2 "http://logger2.qa.sorenson2.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		// For debug builds allow for Radvision logging
		#undef  RV_NOLOG
	#else
		#define stiREMOTELOGGER_SERVICE_DEFAULT1 	"https://remotelogger.sorensonprod.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		#define stiREMOTELOGGER_SERVICE_DEFAULT2 	"http://logger2.sorenson2.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		#define RV_NOLOG // Turn off Radvision logging.
	#endif
#endif //APP_DHV_IOS


////////////////////////////////////////////////////////////////////////////////
//
// Defines for DHV Android
//
#if APPLICATION == APP_DHV_ANDROID

	#define stiLINUX
	#define SM_LITTLE_ENDIAN

	#undef stiALLOW_SHARE_CONTACT
	#undef stiSIGNMAIL_ENABLED
	#undef stiMESSAGE_MANAGER
	#undef stiFAVORITES
	#undef stiINTERNET_SEARCH
	#undef stiCALL_HISTORY_MANAGER
	#undef stiFORCE_CORE_TIMESTAMP
	#undef stiCONTACT_PHOTOS

	#ifdef stiVRS
	#undef stiVRS
	#endif // stiVRS

	#ifndef stiDISABLE_PROPERTY_MANAGER
	#define stiDISABLE_PROPERTY_MANAGER
	#endif

	#ifdef stiMUTEX_DEBUGGING
	#undef stiMUTEX_DEBUGGING
	#endif

	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "vpstun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "mstun.sorenson2.biz"

	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	256000

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	256000

	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE 256000

	//Some of our users are getting IPV6 DNS which cause the app to be unable
	//to register.  If we use Google DNS we don't have this issue.
	#define stiUSE_GOOGLE_DNS
	#define X264_LINK_STATIC

	//
	// Tools define
	//
	#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
	#if !defined(stiRelease) || stiRelease == 0
		#define stiDEBUG            //!< Turn on ASSERT for debugging purposes
	#endif

	#define stiENABLE_TLS
//	#define stiENABLE_ICE

	#define DEVICE DEV_DHV_ANDROID

	#undef CORE_SERVICE_DEFAULT
	#ifdef stiDEBUG
		#define stiREMOTELOGGER_SERVICE_DEFAULT1 "http://logger2.qa.svrs.net/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		#define stiREMOTELOGGER_SERVICE_DEFAULT2 "http://logger2.qa.sorenson2.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		// For debug builds allow for Radvision logging
		#undef  RV_NOLOG
    #else
		#define stiREMOTELOGGER_SERVICE_DEFAULT1 	"http://logger2.sorenson.com/RemoteLoggerServices/RemoteLoggerRequest.aspx"
		#define stiREMOTELOGGER_SERVICE_DEFAULT2 	"http://logger2.sorenson2.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#endif

#endif // APP_DHV_ANDROID


////////////////////////////////////////////////////////////////////////////////
//
// Defines for the hold server
//
#if APPLICATION == APP_HOLDSERVER

#undef stiALLOW_SHARE_CONTACT

#ifndef stiHOLDSERVER
#define stiHOLDSERVER
#endif //stiHOLDserver

#ifndef HOLDSERVER_DLL
#define HOLDSERVER_DLL
#endif // HOLDSERVER_DLL

#ifndef stiENABLE_H264
#define stiENABLE_H264
#endif // stiENABLE_H264

#ifndef stiENABLE_MULTIPLE_VIDEO_INPUT
#define stiENABLE_MULTIPLE_VIDEO_INPUT
#endif // stiENABLE_MULTIPLE_VIDEO_INPUT

//#define DEVICE DEV_X86
//#define APPLICATION APP_HOLDSERVER

#ifndef PROTOTYPES  // flag for md5 prototype macros
#define PROTOTYPES 1

#endif

//
// Tools define
//
#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
#define stiDEBUG                //!< Turn on ASSERT for debugging purposes
#define stiREFERENCE_DEBUGGING     //!< Turn on Call Reference debugging
//#define DEBUG_SIP_LOG_TO_FILE

//#ifdef stiMUTEX_DEBUGGING
//#undef stiMUTEX_DEBUGGING
//#endif

//
// Feature defines
//
//
// Video Format Make sure the correct value is defined
//
#define stiFORMAT_NTSC
// #define stiFORMAT_PAL

// Enable 911 support in the videophone
#define sti911_SUPPORT_ENABLED

// Disable Lost Connection Detection
#define stiDISABLE_LOST_CONNECTION

#ifndef stiDISABLE_PROPERTY_MANAGER
#define stiDISABLE_PROPERTY_MANAGER
#endif

#ifndef stiDISABLE_CAMERA_CONTROL
#define stiDISABLE_CAMERA_CONTROL
#endif

#endif // APP_HOLDSERVER


/////////////////////////////////////////////////////////////////////////////////
//
// Defines for the Registrar Load Test Tool
//
#if APPLICATION == APP_REGISTRAR_LOAD_TEST_TOOL

	#define stiLINUX
	#define SM_LITTLE_ENDIAN

	#define stiDEBUGGING_TOOLS      //!< Turn on debugging tools features
	#define stiDEBUG                //!< Turn on ASSERT for debugging purposes

	// Enable Contact Photos
//	#define stiCONTACT_PHOTOS

	// Enable Core 2.0-style contact lists
	#define stiCONTACT_LIST_MULTI_PHONE         // Multiple phone numbers per contact

	// Enable Message Manager
	#define stiMESSAGE_MANAGER

	// Enable Tunnel Manager
//	#define stiTUNNEL_MANAGER
//	#define TSC_DDT

	#define stiTUNNEL_MAX_SEND_RECV_BIT_RATE 1536000

	#ifdef stiUSE_ZEROS_FOR_IP
	#undef stiUSE_ZEROS_FOR_IP
	#endif // stiUSE_ZEROS_FOR_IP

	// Enable Favorites
    #define stiFAVORITES

	#define OPENSSL_NO_KRB5
	#undef stiSSL_DEFAULT_MODE
	#define stiSSL_DEFAULT_MODE eSSL_ON_WITHOUT_FAILOVER

	#define PERSISTENT_DATA	"/data/ntouchvp2"
	#define STATIC_DATA "/usr/share/ntouchvp2"

	#define stiDISABLE_PM_VERSION_CHECK

	#define stiSIGNMAIL_ENABLED

	#ifdef stiENABLE_ICE
	#undef stiENABLE_ICE
	#endif // stiENABLE_ICE

	#undef stiNAT_TRAVERSAL_SIP_DEFAULT
	#define stiNAT_TRAVERSAL_SIP_DEFAULT estiSIPNATEnabledMediaRelayAllowed

	//
	// TURN server address default
	//
	#undef stiTURN_SERVER_DNS_NAME_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_DEFAULT "vpstun.sorenson.com"
	#undef stiTURN_SERVER_DNS_NAME_ALT_DEFAULT
	#define stiTURN_SERVER_DNS_NAME_ALT_DEFAULT "vpstun.sorenson2.biz"

	//
	// Set the SIP transport to TCP
	//
	#undef stiSIP_TRANSPORT_DEFAULT
	#define stiSIP_TRANSPORT_DEFAULT estiTransportTCP

	// Socket receive buffer size
	#undef stiSOCK_RECV_BUFF_SIZE
	#define stiSOCK_RECV_BUFF_SIZE (128 * 1024) // 128kB

	#undef stiPLAYER_CONTROLS_ENABLED_DEFAULT
	#define stiPLAYER_CONTROLS_ENABLED_DEFAULT	1

	#undef stiSIGNMAIL_ENABLED_DEFAULT
	#define stiSIGNMAIL_ENABLED_DEFAULT	1

	#undef stiBLOCK_LIST_ENABLED_DEFAULT
	#define stiBLOCK_LIST_ENABLED_DEFAULT	true

	// Define method for capturing system/memory stats
	#define stiSTATS_MEMORY_METHOD_2

	#undef stiMAX_RECV_SPEED_DEFAULT
	#define stiMAX_RECV_SPEED_DEFAULT	1024 * 1024 // 1 Mbps

	#undef stiMAX_SEND_SPEED_DEFAULT
	#define stiMAX_SEND_SPEED_DEFAULT	1024 * 1024 // 1 Mbps

	#undef stiMAX_AUTO_RECV_SPEED_DEFAULT
	#define stiMAX_AUTO_RECV_SPEED_DEFAULT	5 * 1024 * 1024 // 5Mbps

	#undef stiMAX_AUTO_SEND_SPEED_DEFAULT
	#define	stiMAX_AUTO_SEND_SPEED_DEFAULT	5 * 1024 * 1024  // 5Mbps

	#undef stiAUTO_SPEED_MODE_DEFAULT
	#define stiAUTO_SPEED_MODE_DEFAULT	estiAUTO_SPEED_MODE_LIMITED

	#define stiMUTEX_DEBUGGING_BACKTRACE

    #define stiLOW_VISION

	#undef stiVCO_PREFERRED_TYPE_DEFAULT
	#define stiVCO_PREFERRED_TYPE_DEFAULT estiVCO_TWO_LINE

	#undef stiVIDEO_CHANNELS_CAPABLE_DEFAULT
	#define stiVIDEO_CHANNELS_CAPABLE_DEFAULT	1

	#undef stiRING_GROUP_CAPABLE_DEFAULT
	#define stiRING_GROUP_CAPABLE_DEFAULT	1

	#define stiENABLE_TLS

	#undef stiSIP_NAT_TRANSPORT_DEFAULT
	#define stiSIP_NAT_TRANSPORT_DEFAULT estiTransportTLS

	#undef CORE_SERVICE_DEFAULT
	#define CORE_SERVICE_DEFAULT CORE_SERVICE_DEFAULT_C

	#undef stiREMOTE_LOG_EVENT_SEND_DEFAULT
	#define stiREMOTE_LOG_EVENT_SEND_DEFAULT	1
	#undef stiREMOTE_LOG_CALL_STATS_DEFAULT
	#define stiREMOTE_LOG_CALL_STATS_DEFAULT	1

	#define stiDISABLE_SDK_NETWORK_INTERFACE
	#define stiDISABLE_CAMERA_CONTROL

	// disable sending re-invites when no media is exchanged
	// (since we're not doing media yet)
	#define stiDISABLE_LOST_CONNECTION

#ifndef stiDISABLE_PROPERTY_MANAGER
	#define stiDISABLE_PROPERTY_MANAGER
#endif

#endif // APP_REGISTRAR_LOAD_TEST_TOOL


#if CORE_SERVICE_DEFAULT == CORE_SERVICE_DEFAULT_B
	#define stiCORE_SERVICE_DEFAULT1 		"http://core1-QA.sorensonqa.biz/VPServices/CoreRequest.aspx"
	#define stiCORE_SERVICE_DEFAULT2 		"http://core2-QA.sorensonqa.biz/VPServices/CoreRequest.aspx"
	#define stiSTATE_NOTIFY_DEFAULT1 		"http://statenotify1-QA.sorensonqa.biz/VPServices/StateNotifyRequest.aspx"
	#define stiSTATE_NOTIFY_DEFAULT2 		"http://statenotify2-QA.sorensonqa.biz/VPServices/StateNotifyRequest.aspx"
	#define stiMESSAGE_SERVICE_DEFAULT1 	"http://message1-QA.sorensonqa.biz/VPServices/MessageRequest.aspx"
	#define stiMESSAGE_SERVICE_DEFAULT2 	"http://message2-QA.sorensonqa.biz/VPServices/MessageRequest.aspx"
	#define stiCONFERENCE_SERVICE_DEFAULT1 	"http://conference1-QA.sorensonqa.biz/VPServices/ConferenceRequest.aspx"
	#define stiCONFERENCE_SERVICE_DEFAULT2 	"http://conference2-QA.sorensonqa.biz/VPServices/ConferenceRequest.aspx"
	#define stiREMOTELOGGER_SERVICE_DEFAULT1 "http://remotelogger-qa.sorensonqa.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#define stiREMOTELOGGER_SERVICE_DEFAULT2 "http://remotelogger-qa.sorensonqa.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#define stiVRS_FAILOVER_SERVER_DEFAULT "vrs-eng.qa.sip.svrs.net"
	#define stiSERVICE_OUTAGE_SERVER_DEFAULT "http://qa.svrsstatus.com/endpointstatus.xml"
	#define stiDHV_API_URL_DEFAULT "https://ens-dhvi.aws-svrs-qa.com"


#elif CORE_SERVICE_DEFAULT == CORE_SERVICE_DEFAULT_C
	#define stiCORE_SERVICE_DEFAULT1 		"http://core1-UAT.sorensonqa.biz/VPServices/CoreRequest.aspx"
	#define stiCORE_SERVICE_DEFAULT2 		"http://core2-UAT.sorensonqa.biz/VPServices/CoreRequest.aspx"
	#define stiSTATE_NOTIFY_DEFAULT1 		"http://statenotify1-UAT.sorensonqa.biz/VPServices/StateNotifyRequest.aspx"
	#define stiSTATE_NOTIFY_DEFAULT2 		"http://statenotify2-UAT.sorensonqa.biz/VPServices/StateNotifyRequest.aspx"
	#define stiMESSAGE_SERVICE_DEFAULT1 	"http://message1-UAT.sorensonqa.biz/VPServices/MessageRequest.aspx"
	#define stiMESSAGE_SERVICE_DEFAULT2 	"http://message2-UAT.sorensonqa.biz/VPServices/MessageRequest.aspx"
	#define stiCONFERENCE_SERVICE_DEFAULT1 	"http://conference1-UAT.sorensonqa.biz/VPServices/ConferenceRequest.aspx"
	#define stiCONFERENCE_SERVICE_DEFAULT2 	"http://conference2-UAT.sorensonqa.biz/VPServices/ConferenceRequest.aspx"
	#define stiREMOTELOGGER_SERVICE_DEFAULT1 "http://remotelogger-uat.sorensonqa.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#define stiREMOTELOGGER_SERVICE_DEFAULT2 "http://remotelogger-uat.sorensonqa.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#define stiVRS_FAILOVER_SERVER_DEFAULT "vrs-eng.qa.sip.svrs.net"
	#define stiSERVICE_OUTAGE_SERVER_DEFAULT "http://qa.svrsstatus.com/endpointstatus.xml"
	#define stiDHV_API_URL_DEFAULT "https://ens-dhvi.aws-svrs-qa.com"

#elif CORE_SERVICE_DEFAULT == CORE_SERVICE_DEFAULT_PRODUCTION
	#define stiCORE_SERVICE_DEFAULT1 		"http://core1.sorensonprod.biz/VPServices/CoreRequest.aspx"
	#define stiCORE_SERVICE_DEFAULT2 		"http://core2.sorensonprod.biz/VPServices/CoreRequest.aspx"
	#define stiSTATE_NOTIFY_DEFAULT1 		"http://statenotify1.sorensonprod.biz/VPServices/StateNotifyRequest.aspx"
	#define stiSTATE_NOTIFY_DEFAULT2 		"http://statenotify2.sorensonprod.biz/VPServices/StateNotifyRequest.aspx"
	#define stiMESSAGE_SERVICE_DEFAULT1 	"http://message1.sorensonprod.biz/VPServices/MessageRequest.aspx"
	#define stiMESSAGE_SERVICE_DEFAULT2 	"http://message2.sorensonprod.biz/VPServices/MessageRequest.aspx"
	#define stiCONFERENCE_SERVICE_DEFAULT1 	"http://conference1.sorensonprod.biz/VPServices/ConferenceRequest.aspx"
	#define stiCONFERENCE_SERVICE_DEFAULT2 	"http://conference2.sorensonprod.biz/VPServices/ConferenceRequest.aspx"
	#define stiREMOTELOGGER_SERVICE_DEFAULT1 	"http://logger2.sorenson.com/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#define stiREMOTELOGGER_SERVICE_DEFAULT2 	"http://logger2.sorenson2.biz/RemoteLoggerServices/RemoteLoggerRequest.aspx"
	#define stiVRS_FAILOVER_SERVER_DEFAULT "hold.svrs.net"
	#define stiSERVICE_OUTAGE_SERVER_DEFAULT "http://www.svrsstatus.com/endpointstatus.xml"
	#define stiDHV_API_URL_DEFAULT "https://ens-dhvi.sorensonaws.com"
#endif


/*
#if DEVICE == DEV_NTOUCH_VP2
  //Temporary until network code works
  #define stiDISABLE_SDK_NETWORK_INTERFACE
#endif
*/

#endif // _SMICONFIG_H_





