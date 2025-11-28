/*!
*  \file cmPropertyNameDef.cpp
*  \brief The Property Name definitions
*
*  This file defines the property name used by conference manager and data tasks
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#include "cmPropertyNameDef.h"

const char NetHostname[] = "NetDNSHostName";

const char IPv4IpAddress[] = "NetIPAddress";
const char IPv4SubnetMask[] = "NetSubnetMask";
const char IPv4GatewayIp[] = "NetGateway";
const char IPv4Dns1[] = "DNSNameServerAddr1";
const char IPv4Dns2[] = "DNSNameServerAddr2";
const char IPv4Dhcp[] = "NetUseDHCP";

const char IPv6Enabled[] = "IPv6Enabled";
const char IPv6IpAddress[] = "IPv6IPAddress";
const char IPv6Prefix[] = "IPv6Prefix";
const char IPv6GatewayIp[] = "IPv6Gateway";
const char IPv6Dns1[] = "IPv6DNSNameServerAddr1";
const char IPv6Dns2[] = "IPv6DNSNameServerAddr2";
const char IPv6Method[] = "IPv6Method";

//const char NetIpAddressObsolete[] = "uiNetIpAddress";
//const char NetSubnetMaskObsolete[] = "uiNetSubnetMask";
//const char NetGatewayIpObsolete[] = "uiNetGatewayIp";
//const char NetHostnameObsolete[] = "uiNetHostname";
//const char NetDns1Obsolete[] = "uiNetDns1";
//const char NetDns2Obsolete[] = "uiNetDns2";
//const char NetDhcpObsolete[] = "uiNetDhcp";

// For Video Playback
//const char cmVP_SNAPSHOT[] = "cmVideoPlaybackSnapShot";
//const char cmVP_MUTE_DETECT_DURATION[] = "cmVideoMuteDetectDuration";

// For Video Record
//const char cmVR_MAXQUANTIZATION[] = "cmVideoRecordMaxQuantization";
//const char cmVR_MAXREFRESHES[] = "cmVideoRecordMaxRefreshes";
//const char cmVR_REFRESHESRATE[] = "cmVideoRecordRefreshesRate";
//const char cmVR_FREEZECAPABLE[] = "cmVideoRecordFreezeCapable";
//const char cmVR_FREEZEPICMODE[] = "cmVideoRecordFreezePictureMode";
const char AUTO_KEYFRAME_SEND[] = "AutoKeyframeSend";

//const char cmVR_BRIGHTNESS_MAX[] = "cmVideoRecordBrightnessMax";
//const char cmVR_BRIGHTNESS_DEFAULT[] = "cmVideoRecordBrightnessDefault";
//const char cmVR_SATURATION_MAX[] = "cmVideoRecordSaturationMax";
//const char cmVR_SATURATION_DEFAULT[] = "cmVideoRecordSaturationDefault";
//const char cmVR_TINT_MAX[] = "cmVideoRecordTintMax";
//const char cmVR_TINT_DEFAULT[] = "cmVideoRecordTintDefault";

// For Audio Playback
//const char cmAP_MUTE_DETECT_DURATION[] = "cmAudioMuteDetectDuration";

// For Audio Record
//const char cmAR_VOLUME[] = "cmAudioRecordVolume";
//const char cmAR_ECHOMODE[] = "cmAudioRecordEchoMode";

const char BLOCK_LIST_ENABLED[] = "BlockListEnabled";
//const char PROTOCOL_REGISTRATION[] = "ProtocolRegistration";
//const char PROTOCOL_DIAL[] = "ProtocolDial";
//const char PROTOCOL_VRS[] = "ProtocolVRS";
//const char IN_NETWORK_MODE[] = "InNetworkMode";
//const char cmAUTO_BANDWIDTH_DETECTION[] = "cmAutoBandwidthDetection";
//const char cmAUTO_DETECT_IP_STATUS[] = "cmAutoDetectIpStatus";
//const char cmAUTO_VIDEO_PRIVACY[] = "cmCallAutoVideoPrivacy";
//const char cmHANDSET_STATUS[] = "cmHandsetStatus";
//const char cmHANDS_FREE_SWITCH[] = "cmHandsFreeSwitch";
const char cmIR_REPEAT_FREQUENCY[] = "cmIrRepeatFrequency";
//const char cmLOCAL_CONNECTION_TYPE[] = "cmLocalConnectionType";
//const char cmLOCAL_HARDWARE_ID[] = "cmCallLocalHardwareId";
//const char cmLOCAL_HARDWARE_VERSION_ID[] = "cmCallLocalHardwareVersionId";
const char cmLOCAL_INTERFACE_MODE[] = "cmLocalInterfaceMode";
const char cmLOW_FRM_RATE_THRSHLD_H263[] = "cmCallLowFrmRateThrshldH263";
const char cmLOW_FRM_RATE_THRSHLD_H264[] = "cmCallLowFrmRateThrshldH264";
const char cmMAX_RECV_SPEED[] = "cmCallMaxRecvSpeed";
const char cmMAX_SEND_SPEED[] = "cmCallMaxSendSpeed";
const char AUTO_SPEED_ENABLED[] = "AutoSpeedEnabled";
const char AUTO_SPEED_MODE[] = "AutoSpeedMode";
const char AUTO_SPEED_MIN_START_SPEED[] = "AutoSpeedMinStartSpeed";
const char cmSIGNMAIL_ENABLED[] = "cmSignMailEnabled";
const char cmPREFERRED_AUDIO_CODEC[] = "cmCallPreferredAudioCodec";
const char cmPREFERRED_VIDEO_CODEC[] = "cmCallPreferredVideoCodec";
const char ALLOWED_VIDEO_ENCODERS[] = "AllowedVideoEncoders";
const char cmPUBLIC_IP_ADDRESS_AUTO[] = "cmPublicIpAddressAuto";
//const char cmPUBLIC_IP_ADDRESS_MANUAL_OBSOLETE[] = "cmPublicIpAddressManual";
//const char cmPUBLIC_IP_ASSIGN_METHOD_OBSOLETE[] = "cmPublicIpAssignMethod";
const char cmPUBLIC_IP_ADDRESS_MANUAL[] = "PublicIPAddressManual";
const char cmPUBLIC_IP_ASSIGN_METHOD[] = "PublicIPAssignMethod";
//const char cmRAS_STATE[] = "cmRasState";
const char cmRINGS_BEFORE_GREETING[] = "RingsBeforeGreeting";
//const char cmSIP_PROXY_REG_STATE[] = "cmSipProxyRegState";
//const char cmSINFO_MECHANISM[] = "cmSInfoMechanism";
//const char cmSIP_TRANSPORT[] = "cmSipTransport";
//const char cmSSL_EXPIRATION_DATE[] = "SSLExpirationDate";
const char cmSSL_CA_EXPIRATION_DATE[] = "SSLCAExpirationDate";
const char cmUDP_TRACE_ADDRESS[] = "cmUdpTraceAddress";
const char cmUDP_TRACE_ENABLED[] = "cmUdpTraceEnabled";
const char cmUDP_TRACE_PORT[] = "cmUdpTracePort";
const char cmVRCL_PORT[] = "cmVrclPort";
const char cmVRS_SERVER[] = "cmVrsServer";
const char cmVRS_ALT_SERVER[] = "cmVrsAltServer";
const char SPANISH_VRS_SERVER[] = "SpanishVrsServer";
const char SPANISH_VRS_ALT_SERVER[] = "SpanishVrsAltServer";
const char VRS_FAILOVER_SERVER[] = "VrsFailOverServer";
const char VRS_FAILOVER_TIMEOUT[] = "VrsFailOverTimeout";
//const char cmVIDEO_SERVER[] = "cmVideoServer";
const char cmHEARTBEAT_DELAY[] = "cmHeartbeatDelay";
const char SERVICE_OUTAGE_TIMER_INTERVAL[] = "ServiceOutageTimerInterval";
const char cmCAMERA_STEP_SIZE[] = "cmCameraStepSize";

// For Camera Control
//const char cmCAMERA_ALLOW_RMT_TO_CTRL[] = "cmCameraAllowRemoteToControl";

const char cmVIDEO_SOURCE[] = "cmVideoSource";
const char cmVIDEO_SOURCE1_ZOOM[] = "cmVideoSource1Zoom";
const char cmVIDEO_SOURCE1_HORZ_ZOOM[] = "cmVideoSource1HorzZoom";	// Deprecated
const char cmVIDEO_SOURCE1_VERT_ZOOM[] = "cmVideoSource1VertZoom";	// Deprecated
const char cmVIDEO_SOURCE1_HORZ_POS[] = "cmVideoSource1HorzPos"; // Deprecated
const char cmVIDEO_SOURCE1_VERT_POS[] = "cmVideoSource1VertPos"; // Deprecated
const char cmVIDEO_SOURCE1_HORZ_CTR[] = "cmVideoSource1HorzCtr";
const char cmVIDEO_SOURCE1_VERT_CTR[] = "cmVideoSource1VertCtr";

//For remote logging
const char REMOTE_TRACE_LOGGING[] = "RemoteTraceLogging";
const char REMOTE_ASSERT_LOGGING[] = "RemoteAssertLogging";
const char REMOTE_CALL_STATS_LOGGING[] = "RemoteCallStatsLogging";
extern const char REMOTE_LOG_EVENT_SEND_LOGGING[] = "RemoteLogEventSendLogging";
const char REMOTE_FLOW_CONTROL_LOGGING[] = "RemoteFlowControlLogging";

// defines for Core Service items
const char cmCORE_SERVICE_URL1[] = "cmCoreServiceUrl1";
const char cmCORE_SERVICE_ALT_URL1[] = "cmCoreServiceAltUrl1";
//const char cmCORE_SERVICE_URL2[] = "cmCoreServiceUrl2"; // Deprecated
const char cmSTATE_NOTIFY_SERVICE_URL1[] = "cmStateNotifyServiceUrl1";
const char cmSTATE_NOTIFY_SERVICE_ALT_URL1[] = "cmStateNotifyServiceAltUrl1";
//const char cmSTATE_NOTIFY_SERVICE_URL2[] = "cmStateNotifyServiceUrl2"; // deprecated
const char cmMESSAGE_SERVICE_URL1[] = "cmMessageServiceUrl1";
const char cmMESSAGE_SERVICE_ALT_URL1[] = "cmMessageServiceAltUrl1";
//const char cmMESSAGE_SERVICE_URL2[] = "cmMessageServiceUrl2"; // deprecated
const char CONFERENCE_SERVICE_URL1[] = "ConferenceServiceUrl1";
const char CONFERENCE_SERVICE_ALT_URL1[] = "ConferenceServiceAltUrl1";
const char REMOTELOGGER_SERVICE_URL1[] = "RemoteLoggerServiceUrl1";
const char REMOTELOGGER_SERVICE_ALT_URL1[] = "RemoteLoggerServiceAltUrl1";
const char SERVICE_OUTAGE_SERVICE_URL1[] = "ServiceOutageServiceUrl1";
const char CORE_AUTH_TOKEN[] = "CoreAuthToken";

//const char cmDUMMY_PROPERTY[] = "cmDummyProperty";
//const char szstiCORE_SERVICE1_DEFAULT[] = "http://core1.sorenson.com/VPServices/CoreRequest.aspx";
//const char szstiSTATE_NOTIFY_SERVICE1_DEFAULT[] = "http://statenotify1.sorenson.com/VPServices/StateNotifyRequest.aspx";
//const char szstiMESSAGE_SERVICE1_DEFAULT[] = "http://message1.sorenson.com/VPServices/MessageRequest.aspx";

//const char cmSERVICE_MASK[] = "cmServiceMask";
//const char cmSERVICE_LEVEL[] = "cmServiceLevel";
const char cmCHECK_FOR_AUTH_NUMBER[] = "cmCheckForAuthorizedNumber";

const char CoreServiceSSLFailOver[] = "CoreServiceSSLFailOver";
const char StateNotifySSLFailOver[] = "StateNotifySSLFailOver";
const char MessageServiceSSLFailOver[] = "MessageServiceSSLFailOver";
const char ConferenceServiceSSLFailOver[] = "ConferenceServiceSSLFailOver";
const char RemoteLoggerServiceSSLFailOver[] = "RemoteLoggerServiceSSLFailOver";
const char AltSipListenPortEnabled[] = "SecondStaticSipListenPortEnabled";
const char AltSipTlsListenPortEnabled[] = "AltSipTlsListenPortEnabled";
const char AltSipListenPort[] = "SecondStaticSipListenPort";
const char AltSipTlsListenPort[] = "AltSipTlsListenPort";
const char CurrentSipListenPort[] = "CurrentSipListenPort";
const char CurrentMediaPortBase[] = "CurrentChannelPortBase";
const char CurrentMediaPortRange[] = "CurrentChannelPortRange";
//const char cmCHANNEL_PORT_BASE[] = "cmChannelPortBase";
//const char cmCHANNEL_PORT_RANGE[] = "cmChannelPortRange";
//const char RouterFriendlyName[] = "RouterFriendlyName";
//const char RouterMfgName[] = "RouterMfgName";
//const char RouterModelName[] = "RouterModelName";
//const char RouterModelNumber[] = "RouterModelNumber";
//const char RouterModelDesc[] = "RouterModelDesc";
//const char RouterUPnPEnabled[] = "RouterUPnPEnabled";
//const char RouterUPnPStartPort[] = "RouterUPnPStartPort";
//const char RouterUPnPEndPort[] = "RouterUPnPEndPort";
//const char RouterValidationResult[] = "RouterValidationResult";
//const char UPnPMode[] = "UPnPMode";
//const char UPnPControlPortRange[] = "UPnPControlPortRange";
//const char UPnPMediaPortRange[] = "UPnPMediaPortRange";
const char LOCAL_FROM_NAME[] = "uiFromName";
const char LOCAL_FROM_DIAL_TYPE[] = "uiFromDialType";
const char LOCAL_FROM_DIAL_STRING[] = "uiFromDialString";
const char DefUserName[] = "uiDefUserName";
const char cmFIRMWARE_VERSION[] = "cmFirmwareVersion";
//const char DOS_ATTACK_DETECT_WINDOW[] = "DOSAttackDetectWindow";
//const char DOS_ATTACK_MAX_CALLS_TRIGGER[] = "DOSAttackMaxCallsTrigger";
//const char DOS_ATTACK_LOCKOUT_DURATION[] = "DOSAttackLockoutDuration";
const char BOOT_COUNT[] = "BootCount";
const char BOOT_TIME[] = "BootTime";
const char NAT_TRAVERSAL_SIP[] = "NATTraversalSIP";

const char TUNNEL_SERVER[] = "TunnelServer";
const char TUNNELING_ENABLED[] = "TunnelingEnabled";
const char TURN_SERVER[] = "TURNServer";
const char TURN_SERVER_ALT[] = "TURNServerAlt";
const char TURN_SERVER_USERNAME[] = "TURNServerUsername";
const char TURN_SERVER_PASSWORD[] = "TURNServerPassword";
//const char STUN_SERVER[] = "STUNServer";
//const char STUN_SERVER_ALT[] = "STUNServerAlt";
//const char SIP_PROXY[] = "SIPProxy";
//const char SIP_REGISTRAR[] = "SIPRegistrar";
//const char SIP_TRANSPORT[] = "SIPTransport";
const char SIP_NAT_TRANSPORT[] = "SIPNatTransport";
const char SECURE_CALL_MODE[] = "SecureCallMode";
const char RING_GROUP_CAPABLE[] = "RingGroupCapable";
const char RING_GROUP_ENABLED[] = "RingGroupEnabled";
const char RING_GROUP_DISPLAY_MODE[] = "RingGroupDisplayMode";
const char VIDEO_CHANNELS_CAPABLE[] = "VideoChannelsCapable";
const char SIP_PRIVATE_DOMAIN_OVERRIDE[] = "SipPrivateDomainOverride";
const char SIP_PRIVATE_DOMAIN_ALT_OVERRIDE[] = "SipPrivateDomainAltOverride";
const char SIP_PUBLIC_DOMAIN_OVERRIDE[] = "SipPublicDomainOverride";
const char SIP_PUBLIC_DOMAIN_ALT_OVERRIDE[] = "SipPublicDomainAltOverride";
const char SIP_AGENT_DOMAIN_OVERRIDE[] = "SipAgentDomainOverride";
const char SIP_AGENT_DOMAIN_ALT_OVERRIDE[] = "SipAgentDomainAltOverride";
#ifdef stiINTEROP_EVENT
const char SIP_USER_OVERRIDE[] = "InteropProxyUser";
const char SIP_PASSWORD_OVERRIDE[] = "InteropProxyPwd";
#endif
//const char H323_PUBLIC_DOMAIN_OVERRIDE[] = "H323PublicDomainOverride";
//const char H323_PUBLIC_DOMAIN_ALT_OVERRIDE[] = "H323PublicDomainAltOverride";
//const char USE_SIP_FOR_URI_DIALING[] = "UseSIPForURIDialing";

//const char SEND_VIDEO_ONLY_WHEN_VIDEO_RECEIVED[] = "SendVideoOnlyWhenVideoReceived";

const char CALL_STATUS[] = "CallStatus";
const char INSTANCE_GUID[] = "InstanceGUID";

//const char ONLY_USE_SORENSON_URIS[] = "OnlyUseSorensonURIs";

const char PERSONAL_GREETING_ENABLED[] = "PersonalGreetingEnabled";
const char DIRECT_SIGNMAIL_ENABLED[] = "DirectSignMailEnabled";
const char GREETING_TEXT[] = "GreetingText";
const char GREETING_PREFERENCE[] = "GreetingPreference";
const char PERSONAL_GREETING_TYPE[] = "PersonalGreetingType";
const char CONTACT_IMAGES_ENABLED[] = "ContactImagesEnabled";
const char EXPORT_CONTACTS_ENABLED[] = "ExportContactsEnabled";
const char PULSE_ENABLED[] = "PulseEnabled";
const char PHONE_USER_ID[] = "PhoneUserID";
const char GROUP_USER_ID[] = "GroupUserID";

const char LAST_MISSED_TIME[] = "uiLastMissedTime";

const char DTMF_ENABLED[] = "DTMFEnabled";
const char REAL_TIME_TEXT_ENABLED[] = "RealTimeTextEnabled";
const char WEB_DIAL_ENABLED[] = "WebDialEnabled";
const char ENABLE_CALL_BRIDGE[] = "CallBridgeEnabled";
const char FAVORITES_ENABLED[] = "ContactFavoriteEnabled";
const char BLOCK_CALLER_ID_ENABLED[] = "BlockCallerIdEnabled";

const char MAC_ADDRESS[] = "MacAddress";

const char HTTP_PROXY_ADDRESS[] = "HTTPProxyAddress"; // This should be in the form Address:port

const char BLOCK_CALLER_ID[] = "BlockCallerId";
const char BLOCK_ANONYMOUS_CALLERS[] = "BlockAnonymousCalls";

const char TECH_SUPPORT_URI[] = "TechSupportURI";
const char CUSTOMER_INFORMATION_URI[] = "CustomerInformationURI";

const char GROUP_VIDEO_CHAT_ENABLED[] = "GroupVideoChatEnabled";

const char LDAP_ENABLED[] = "LDAPDirectoryEnabled";
const char LDAP_SERVER[] = "LDAPServer"; // (string) - LDAP host
const char LDAP_SERVER_PORT[] = "LDAPServerPort"; // (int) - default 389
const char LDAP_SERVER_USE_SSL[] = "LDAPServerUseSSL"; // (int) - 0 or 1
const char LDAP_SERVER_REQUIRES_AUTHENTICATION[] = "LDAPServerRequiresAuthentication";
const char LDAP_DOMAIN_BASE[] = "LDAPDomainBase"; // (string) - The point in the LDAP tree where groups and users are searched from. (ou=People,dc=github,dc=com)
const char LDAP_TELEPHONE_NUMBER_FIELD[] = "LDAPTelephoneNumberField"; // (string) - Name of attribute to be used, default = ‘telephoneNumber’. (Mapped to work number)
const char LDAP_HOME_NUMBER_FIELD[] = "LDAPHomeNumberField"; // (string) -  Name of attribute to be used, default = ‘homePhone’
const char LDAP_MOBILE_NUMBER_FIELD[] = "LDAPMobileNumberField"; // (string) -  Name of attribute to be used, default = ‘mobile’
const char LDAP_DIRECTORY_DISPLAY_NAME[] = "LDAPDirectoryDisplayName"; // - Name to use on Directory tab
const char LDAP_SCOPE[] = "LDAPScope"; // (int) 0 - base,1 - 1 level ,2 - subtree

const char NEW_CALL_ENABLED[] = "NewCallEnabled"; // (int) - Determines if in a call we can spawn a new relay call with interpreter.

extern const char LAST_SIGNMAIL_UPDATE_TIME[] = "LastMessageViewTime"; // Last update time for SignMail
extern const char LAST_VIDEO_UPDATE_TIME[] = "LastChannelMessageViewTime"; // Last update time for VideoCenter videos.

#ifdef stiINTERNET_SEARCH
const char INTERNET_SEARCH_ENABLED[] = "InternetSearchEnabled";  // (int) - Feature Enable for Internet Search.
const char INTERNET_SEARCH_ALLOWED[] = "InternetSearchAllowed"; // (int) - User has allowed the use of Internet Search.
#endif

#if APPLICATION != APP_NTOUCH_VP2 && APPLICATION != APP_NTOUCH_VP3 && APPLICATION != APP_NTOUCH_VP4
//
// These properties are also defined at the UI layer and are not accessed by the engine.
// These should be removed from this file.
//
const char DISPLAY_CONTACT_IMAGES[] = "DisplayContactImages";
const char PROFILE_IMAGE_PRIVACY_LEVEL[] = "ProfileImagePrivacyLevel";
const char LAST_CIR_CALL_TIME[] = "LastCIRCallTime";
const char ANNOUNCE_ENABLED[] = "AnnounceEnabled";
const char ANNOUNCE_H2D_PREFERENCE[] = "AnnouncePreferenceH2D";
const char ANNOUNCE_H2D_TEXT[] = "AnnounceTextH2D";
const char ANNOUNCE_D2H_PREFERENCE[] = "AnnouncePreferenceD2H";
const char ANNOUNCE_D2H_TEXT[] = "AnnounceTextD2H";
const char ANNOUNCE_TEXT_H2D_SELECTION[] = "AnnounceTextH2DSelection";
#endif

const char CALL_TRANSFER_ENABLED[] = "CallTransferEnabled";
const char AGREEMENTS_ENABLED[] = "AgreementsEnabled";

//const char UPDATE_URL_OVERRIDE[] = "UpdateUrlOverride";
const char UPDATE_URL_OVERRIDE[] = "uiUpdateServer";  // NOTE: match the nVP tag name for use with VPControl

const char CLIENT_REREGISTER_MAX_LOOKUP_TIME[] = "RestartProxyMaxLookupTime";
const char CLIENT_REREGISTER_MIN_LOOKUP_TIME[] = "RestartProxyMinLookupTime";

const char UNREGISTERED_SIP_RESTART_TIME[] = "TimeUnregisteredBeforeRestart";

const char FIR_FEEDBACK_ENABLED[] = "FIRKeyframeRequestsEnabled";
const char PLI_FEEDBACK_ENABLED[] = "PLIFeedbackEnabled";
const char AFB_FEEDBACK_ENABLED[] = "AFBFeedbackEnabled";
const char TMMBR_FEEDBACK_ENABLED[] = "TMMBRFeedbackEnabled";
const char HEVC_SIGNMAIL_PLAYBACK_ENABLED[] = "HEVCSignMailPlaybackEnabled";
const char MANAGED_DEVICE[] = "ManagedDevice";

const char REMOTE_FLASHER_STATUS[] = "RemoteFlasherStatus";

const char NACK_RTX_FEEDBACK_ENABLED[] = "NACKRTXFeedbackEnabled";
const char VRS_FOCUSED_ROUTING[] = "VrsFocusedRouting";
const char DHV_API_URL[] = "DhvApiUrl";
const char DHV_API_KEY[] = "DhvApiKey";
const char DEAF_HEARING_VIDEO_ENABLED[] = "DeafHearingVideoEnabled";

const char ENDPOINT_MONITOR_SERVER[] = "EndpointMonitorServer";
const char ENDPOINT_MONITOR_ENABLED[] = "EndpointMonitorEnabled";
const char DEVICE_LOCATION_TYPE[] = "DeviceLocationType";

const char UI_LANGUAGE[] = "uiLanguage";
