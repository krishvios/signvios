/*!
*  \file cmPropertyNameDef.h
*  \brief The Property Name definitions
*
*  This file defines the property name used by conference manager and data tasks
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/
#ifndef STIPROPERTYNAMEDEF_H
#define STIPROPERTYNAMEDEF_H

extern const char NetHostname[];             //!< Host name.

extern const char IPv4IpAddress[];            //!< IP address.
extern const char IPv4SubnetMask[];           //!< SUbnet mask
extern const char IPv4GatewayIp[];            //!< Gateway address.
extern const char IPv4Dns1[];                 //!< Primary DNS
extern const char IPv4Dns2[];                 //!< Secondary DNS
extern const char IPv4Dhcp[];                 //!< DHCP enabled or disabled.

extern const char IPv6Enabled[];
extern const char IPv6IpAddress[];            //!< IP address.
extern const char IPv6Prefix[];               //!< Network prefix
extern const char IPv6GatewayIp[];            //!< Gateway address.
extern const char IPv6Dns1[];                 //!< Primary DNS
extern const char IPv6Dns2[];                 //!< Secondary DNS
extern const char IPv6Method[];               //!< Manual or Automatic.


// extern const char NetIpAddressObsolete[];
// extern const char NetSubnetMaskObsolete[];
// extern const char NetGatewayIpObsolete[];
// extern const char NetHostnameObsolete[];
// extern const char NetDns1Obsolete[];
// extern const char NetDns2Obsolete[];
// extern const char NetDhcpObsolete[];

// For Video Playback
//extern const char cmVP_SNAPSHOT[];
//extern const char cmVP_MUTE_DETECT_DURATION[];

// For Video Record
/*extern const char cmVR_MAXQUANTIZATION[];
extern const char cmVR_MAXREFRESHES[];
extern const char cmVR_REFRESHESRATE[];*/
//extern const char cmVR_FREEZECAPABLE[];
/*extern const char cmVR_FREEZEPICMODE[];*/
extern const char AUTO_KEYFRAME_SEND[]; // In milliseconds. (0=disabled)

//extern const char cmVR_BRIGHTNESS_MAX[];
//extern const char cmVR_BRIGHTNESS_DEFAULT[];
//extern const char cmVR_SATURATION_MAX[];
//extern const char cmVR_SATURATION_DEFAULT[];
//extern const char cmVR_TINT_MAX[];
//extern const char cmVR_TINT_DEFAULT[];
#
// For Audio Playback
//extern const char cmAP_MUTE_DETECT_DURATION[];

// For Audio Record
//extern const char cmAR_VOLUME[];
//extern const char cmAR_ECHOMODE[];

extern const char BLOCK_LIST_ENABLED[];
//extern const char PROTOCOL_REGISTRATION[];
//extern const char PROTOCOL_DIAL[];
//extern const char PROTOCOL_VRS[];;
//extern const char IN_NETWORK_MODE[];
//extern const char cmAUTO_BANDWIDTH_DETECTION[];
//extern const char cmAUTO_DETECT_IP_STATUS[];
//extern const char cmAUTO_VIDEO_PRIVACY[];
//extern const char cmHANDSET_STATUS[];
//extern const char cmHANDS_FREE_SWITCH[];
extern const char cmIR_REPEAT_FREQUENCY[];
/*extern const char cmLOCAL_CONNECTION_TYPE[];
extern const char cmLOCAL_HARDWARE_ID[];
extern const char cmLOCAL_HARDWARE_VERSION_ID[];*/
extern const char cmLOCAL_INTERFACE_MODE[];
/*extern const char cmLOCAL_FROM_NAME[];
extern const char cmLOCAL_FROM_DIAL_TYPE[];
extern const char cmLOCAL_FROM_DIAL_STRING[];*/
//extern const char cmLOCAL_NAME[]; No longer used
//extern const char cmLOCAL_USER_ID[];  No longer used
extern const char cmLOW_FRM_RATE_THRSHLD_H263[];
extern const char cmLOW_FRM_RATE_THRSHLD_H264[];
extern const char cmMAX_RECV_SPEED[];
extern const char cmMAX_SEND_SPEED[];
extern const char AUTO_SPEED_ENABLED[];
extern const char AUTO_SPEED_MODE[];
extern const char AUTO_SPEED_MIN_START_SPEED[];
extern const char cmSIGNMAIL_ENABLED[];
extern const char cmPREFERRED_AUDIO_CODEC[];
extern const char cmPREFERRED_VIDEO_CODEC[];
extern const char ALLOWED_VIDEO_ENCODERS[];
extern const char cmPUBLIC_IP_ADDRESS_AUTO[];
/*extern const char cmPUBLIC_IP_ADDRESS_MANUAL_OBSOLETE[];
extern const char cmPUBLIC_IP_ASSIGN_METHOD_OBSOLETE[];*/
extern const char cmPUBLIC_IP_ADDRESS_MANUAL[];
extern const char cmPUBLIC_IP_ASSIGN_METHOD[];
/*extern const char cmRAS_STATE[];*/
extern const char cmRINGS_BEFORE_GREETING[];
//extern const char cmSIP_PROXY_REG_STATE[];
//extern const char cmSINFO_MECHANISM[];
//extern const char cmSIP_TRANSPORT[];
//extern const char cmSSL_EXPIRATION_DATE[];
extern const char cmSSL_CA_EXPIRATION_DATE[];
extern const char cmUDP_TRACE_ADDRESS[];
extern const char cmUDP_TRACE_ENABLED[];
extern const char cmUDP_TRACE_PORT[];
extern const char cmVRCL_PORT[];
extern const char cmVRS_SERVER[];
extern const char cmVRS_ALT_SERVER[];
extern const char SPANISH_VRS_SERVER[];
extern const char SPANISH_VRS_ALT_SERVER[];
extern const char VRS_FAILOVER_SERVER[];
extern const char VRS_FAILOVER_TIMEOUT[];
//extern const char cmVIDEO_SERVER[];
extern const char cmHEARTBEAT_DELAY[];
extern const char SERVICE_OUTAGE_TIMER_INTERVAL[];
extern const char cmCAMERA_STEP_SIZE[];

// For Camera Control
//extern const char cmCAMERA_ALLOW_RMT_TO_CTRL[];

extern const char cmVIDEO_SOURCE[];
extern const char cmVIDEO_SOURCE1_ZOOM[];
extern const char cmVIDEO_SOURCE1_HORZ_ZOOM[]; // Deprecated
extern const char cmVIDEO_SOURCE1_VERT_ZOOM[]; // Deprecated
extern const char cmVIDEO_SOURCE1_HORZ_POS[]; // Deprecated
extern const char cmVIDEO_SOURCE1_VERT_POS[]; // Deprecated
extern const char cmVIDEO_SOURCE1_HORZ_CTR[];
extern const char cmVIDEO_SOURCE1_VERT_CTR[];

//For remote logging
extern const char REMOTE_TRACE_LOGGING[];
extern const char REMOTE_ASSERT_LOGGING[];
extern const char REMOTE_CALL_STATS_LOGGING[];
extern const char REMOTE_LOG_EVENT_SEND_LOGGING[];
extern const char REMOTE_FLOW_CONTROL_LOGGING[];

// defines for Core Service items
extern const char cmCORE_SERVICE_URL1[];
extern const char cmCORE_SERVICE_ALT_URL1[];
//extern const char cmCORE_SERVICE_URL2[];	 // Deprecated
extern const char cmSTATE_NOTIFY_SERVICE_URL1[];
extern const char cmSTATE_NOTIFY_SERVICE_ALT_URL1[];
//extern const char cmSTATE_NOTIFY_SERVICE_URL2[];	 // Deprecated
extern const char cmMESSAGE_SERVICE_URL1[];
extern const char cmMESSAGE_SERVICE_ALT_URL1[];
//extern const char cmMESSAGE_SERVICE_URL2[];	 // Deprecated
extern const char CONFERENCE_SERVICE_URL1[];
extern const char CONFERENCE_SERVICE_ALT_URL1[];
extern const char REMOTELOGGER_SERVICE_URL1[];
extern const char REMOTELOGGER_SERVICE_ALT_URL1[];
extern const char SERVICE_OUTAGE_SERVICE_URL1[];
extern const char CORE_AUTH_TOKEN[];

// extern const char cmDUMMY_PROPERTY[];
// extern const char szstiCORE_SERVICE1_DEFAULT[];
// extern const char szstiSTATE_NOTIFY_SERVICE1_DEFAULT[];
// extern const char szstiMESSAGE_SERVICE1_DEFAULT[];

/*extern const char cmSERVICE_MASK[];
extern const char cmSERVICE_LEVEL[];*/
extern const char cmCHECK_FOR_AUTH_NUMBER[];

extern const char CoreServiceSSLFailOver[];
extern const char StateNotifySSLFailOver[];
extern const char MessageServiceSSLFailOver[];
extern const char ConferenceServiceSSLFailOver[];
extern const char RemoteLoggerServiceSSLFailOver[];

extern const char AltSipListenPortEnabled[];
extern const char AltSipTlsListenPortEnabled[];
extern const char AltSipListenPort[];
extern const char AltSipTlsListenPort[];
extern const char CurrentSipListenPort[];
extern const char CurrentMediaPortBase[];
extern const char CurrentMediaPortRange[];
//extern const char cmCHANNEL_PORT_BASE[];
//extern const char cmCHANNEL_PORT_RANGE[];
//extern const char RouterFriendlyName[];
//extern const char RouterMfgName[];
//extern const char RouterModelName[];
//extern const char RouterModelNumber[];
//extern const char RouterModelDesc[];
//extern const char RouterUPnPEnabled[];
//extern const char RouterUPnPStartPort[];
//extern const char RouterUPnPEndPort[];
//extern const char RouterValidationResult[];
//extern const char UPnPMode[];
//extern const char UPnPControlPortRange[];
//extern const char UPnPMediaPortRange[];
extern const char LOCAL_FROM_NAME[]; 			// We have the ui tag so we can tell the
extern const char LOCAL_FROM_DIAL_TYPE[];		// the difference between these and those
extern const char LOCAL_FROM_DIAL_STRING[];	// on the CM side.
extern const char DefUserName[];
extern const char cmFIRMWARE_VERSION[];
//extern const char DOS_ATTACK_DETECT_WINDOW[];
//extern const char DOS_ATTACK_MAX_CALLS_TRIGGER[];
//extern const char DOS_ATTACK_LOCKOUT_DURATION[];
extern const char BOOT_COUNT[];
extern const char BOOT_TIME[];
extern const char NAT_TRAVERSAL_SIP[];
extern const char TUNNEL_SERVER[];
extern const char TUNNELING_ENABLED[];
extern const char TURN_SERVER[];
extern const char TURN_SERVER_ALT[];
extern const char TURN_SERVER_USERNAME[];
extern const char TURN_SERVER_PASSWORD[];
//extern const char STUN_SERVER[];
//extern const char STUN_SERVER_ALT[];
//extern const char SIP_PROXY[];
//extern const char SIP_REGISTRAR[];
//extern const char SIP_TRANSPORT[];
extern const char SIP_NAT_TRANSPORT[];
extern const char SECURE_CALL_MODE[];
extern const char RING_GROUP_CAPABLE[];
extern const char RING_GROUP_ENABLED[];
extern const char RING_GROUP_DISPLAY_MODE[];
extern const char VIDEO_CHANNELS_CAPABLE[];
//extern const char SEND_VIDEO_ONLY_WHEN_VIDEO_RECEIVED[];
extern const char SIP_PRIVATE_DOMAIN_OVERRIDE[];
extern const char SIP_PRIVATE_DOMAIN_ALT_OVERRIDE[];
extern const char SIP_PUBLIC_DOMAIN_OVERRIDE[];
extern const char SIP_PUBLIC_DOMAIN_ALT_OVERRIDE[];
extern const char SIP_AGENT_DOMAIN_OVERRIDE[];
extern const char SIP_AGENT_DOMAIN_ALT_OVERRIDE[];
#ifdef stiINTEROP_EVENT
extern const char SIP_USER_OVERRIDE[];
extern const char SIP_PASSWORD_OVERRIDE[];
#endif
//extern const char H323_PUBLIC_DOMAIN_OVERRIDE[];
//extern const char H323_PUBLIC_DOMAIN_ALT_OVERRIDE[];
//extern const char USE_SIP_FOR_URI_DIALING[];

extern const char CALL_STATUS[];
extern const char INSTANCE_GUID[];
//extern const char ONLY_USE_SORENSON_URIS[];

extern const char PERSONAL_GREETING_ENABLED[];
extern const char DIRECT_SIGNMAIL_ENABLED[];
extern const char GREETING_TEXT[];
extern const char GREETING_PREFERENCE[];
extern const char PERSONAL_GREETING_TYPE[];
extern const char CONTACT_IMAGES_ENABLED[];
extern const char EXPORT_CONTACTS_ENABLED[];
extern const char PULSE_ENABLED[];
extern const char PHONE_USER_ID[];
extern const char GROUP_USER_ID[];

extern const char DTMF_ENABLED[];
extern const char REAL_TIME_TEXT_ENABLED[];
extern const char WEB_DIAL_ENABLED[];
extern const char ENABLE_CALL_BRIDGE[];
extern const char FAVORITES_ENABLED[];
extern const char BLOCK_CALLER_ID_ENABLED[];

extern const char LAST_MISSED_TIME[];

extern const char MAC_ADDRESS[];

extern const char HTTP_PROXY_ADDRESS[];

extern const char BLOCK_CALLER_ID[];
extern const char BLOCK_ANONYMOUS_CALLERS[];

extern const char TECH_SUPPORT_URI[];
extern const char CUSTOMER_INFORMATION_URI[];

extern const char GROUP_VIDEO_CHAT_ENABLED[];

extern const char LDAP_ENABLED[];
extern const char LDAP_SERVER[];
extern const char LDAP_SERVER_PORT[];
extern const char LDAP_SERVER_USE_SSL[];
extern const char LDAP_SERVER_REQUIRES_AUTHENTICATION[];
extern const char LDAP_DOMAIN_BASE[];
extern const char LDAP_TELEPHONE_NUMBER_FIELD[];
extern const char LDAP_HOME_NUMBER_FIELD[];
extern const char LDAP_MOBILE_NUMBER_FIELD[];
extern const char LDAP_DIRECTORY_DISPLAY_NAME[];
extern const char LDAP_SCOPE[];

extern const char NEW_CALL_ENABLED[];

extern const char LAST_SIGNMAIL_UPDATE_TIME[];
extern const char LAST_VIDEO_UPDATE_TIME[];

#ifdef stiINTERNET_SEARCH
extern const char INTERNET_SEARCH_ENABLED[];
extern const char INTERNET_SEARCH_ALLOWED[];
#endif

#if APPLICATION != APP_NTOUCH_VP2 && APPLICATION != APP_NTOUCH_VP3 && APPLICATION != APP_NTOUCH_VP4
//
// These properties are also defined at the UI layer and are not accessed by the engine.
// These should be removed from this file.
//
extern const char DISPLAY_CONTACT_IMAGES[];
extern const char PROFILE_IMAGE_PRIVACY_LEVEL[];
extern const char LAST_CIR_CALL_TIME[];
extern const char ANNOUNCE_ENABLED[];
extern const char ANNOUNCE_H2D_PREFERENCE[];
extern const char ANNOUNCE_H2D_TEXT[];
extern const char ANNOUNCE_D2H_PREFERENCE[];
extern const char ANNOUNCE_D2H_TEXT[];
extern const char ANNOUNCE_TEXT_H2D_SELECTION[];
#endif

extern const char CALL_TRANSFER_ENABLED[];
extern const char AGREEMENTS_ENABLED[];

extern const char UPDATE_URL_OVERRIDE[];

extern const char CLIENT_REREGISTER_MAX_LOOKUP_TIME[];	// Maximum time, in seconds, to wait before restarting a complete DNS proxy lookup and registration.
extern const char CLIENT_REREGISTER_MIN_LOOKUP_TIME[];	// Minimum time, in seconds, to wait before restarting a complete DNS proxy lookup and registration.

extern const char UNREGISTERED_SIP_RESTART_TIME[]; // Number of minutes after registration expires to restart stack if endpoint fails to Re-register. 0 means disabled.

extern const char FIR_FEEDBACK_ENABLED[];   // Indicates if we should send keyframe requests via FIR RTCP feedback. Setting 0 to disable and 1 to enable.
extern const char PLI_FEEDBACK_ENABLED[];
extern const char AFB_FEEDBACK_ENABLED[];
extern const char TMMBR_FEEDBACK_ENABLED[]; // Indicates if we should send rate control information via TMMBR RTCP feedback. Setting 0 to disable and 1 to enable.
extern const char HEVC_SIGNMAIL_PLAYBACK_ENABLED[]; //Enable or Disable the HEVC SignMail Playback
extern const char MANAGED_DEVICE[]; // iPad or Mac controlled by MDM server
extern const char NACK_RTX_FEEDBACK_ENABLED[]; //Indicates if we should signal and support NACK and RTX. Setting 0 is disabled, 1 enabled Sorenson Only, 2 enabled all.

#endif

extern const char REMOTE_FLASHER_STATUS[];

extern const char VRS_FOCUSED_ROUTING[];
extern const char DHV_API_URL[];
extern const char DHV_API_KEY[];
extern const char DEAF_HEARING_VIDEO_ENABLED[];

extern const char ENDPOINT_MONITOR_SERVER[];
extern const char ENDPOINT_MONITOR_ENABLED[];
extern const char DEVICE_LOCATION_TYPE[];

extern const char UI_LANGUAGE[];
