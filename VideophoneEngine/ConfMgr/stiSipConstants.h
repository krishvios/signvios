////////////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  stiSipConstants.h
//  Implementation of the Class stiSipConstants
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __sip_contants_h_
#define __sip_contants_h_

static const char g_szSVRSAllowIncomingCallPrivateHeader[] = "P-SVRS-AIC"; // Allow Incoming call
static const char g_szSVRSConvertSIPToH323PrivateHeader[] = "P-SVRS-SIP-TO-H323"; // Convert call from SIP to H323
static const char g_szSVRSSInfoPrivateHeader[] = "P-SVRS-SInfo";
static const char g_szSDPSVRSSinfoPrivateHeader[] = "X-SVRS-SINFO";
static const char g_szSDPSVRSMailPrivateHeader[] = "X-SVRS-VIDEOMAIL";
static const char g_szSDPSVRSGroupChatPrivateHeader[] = "X-SVRS-GC";
static const char g_szHoldServer[] = "Sorenson VRS Hold Server";
static const char g_szSVRS_DEVICE[] = "Sorenson";
static const char g_svrsDisplaySize[] = "X-SVRS-DISPLAY-SIZE";

// Method types
#define INVITE_METHOD "INVITE"
#define INFO_METHOD "INFO"
#define MESSAGE_METHOD "MESSAGE"
#define OPTIONS_METHOD "OPTIONS"
#define UPDATE_METHOD "UPDATE"
#define MAX_METHOD_LEN (sizeof (MESSAGE_METHOD) + 1)

// Mime types
#define MEDIA_CONTROL_MIME "application/media_control+xml"
#define SINFO_MIME "application/sinfo"
#define SDP_MIME "application/sdp"
#define TEXT_MIME "text/plain"
#define CONTACT_MIME "text/contact"
#define NUDGE_MIME "application/im-poke+xml"
#define SORENSON_CUSTOM_MIME "application/sorenson"
#define HEARING_STATUS_MIME "text/hearing_status"
#define NEW_CALL_READY_MIME "text/new_call_ready"
#define SPAWN_CALL_MIME "text/hearing_call_spawn"
#define DISPLAY_SIZE_MIME "text/svrs_display_size"
#define DHVI_MIME "text/dhvi_message"
#define KEEP_ALIVE_MIME "application/keep_alive"
#define MAX_MIME_LEN (sizeof (MEDIA_CONTROL_MIME) + 1)

// INFO message package types
#define CONTACT_PACKAGE "SVRS_CONTACT"
#define HEARING_STATUS_PACKAGE "SVRS_HEARING_STATUS"
#define NEW_CALL_READY_PACKAGE "SVRS_NEW_CALL_READY"
#define HEARING_CALL_SPAWN_PACKAGE "SVRS_HEARING_CALL_SPAWN"
#define DISPLAY_SIZE_PACKAGE "SVRS_DISPLAY_SIZE"
#define S_INFO_PACKAGE "SVRS_SINFO"
#define DHVI_PACKAGE "SVRS_DHVI"
#define KEEP_ALIVE_PACKAGE "SVRS_KEEPALIVE"

// Custom headers
#define CONTENT_DISPOSITION "Content-Disposition"
#define INFO_PACKAGE "Info-Package"
#define RECV_INFO "Recv-Info"

// Dial Plans
#define DIALPLAN_URI_HEADER "SVRS-"
#define DEAF_DIALPLAN_URI_HEADER "sip:D-"
#define HEARING_DIALPLAN_URI_HEADER "sip:H-"

// Prefix
#define SIP_PREFIX "sip:"

// Common messages
#define NUDGE_MESSAGE "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<poke xmlns=\"urn:ietf:params:xml:ns:im-poke\" />"
#define KEYFRAME_REQUEST_MESSAGE "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<media_control>\n\t<vc_primitive>\n\t\t<to_encoder>\n\t\t\t<picture_fast_update></picture_fast_update>\n\t\t</to_encoder>\n\t</vc_primitive>\n</media_control>"

// Sip responses
enum SipResponse
{
	SIPRESP_NONE = 0,
	SIPRESP_TRYING = 100,
	SIPRESP_RINGING = 180,
	SIPRESP_PROGRESSING = 183,
	SIPRESP_SUCCESS = 200,
	SIPRESP_MOVED_PERMANENTLY = 301,
	SIPRESP_MOVED_TEMPORARILY = 302,
	SIPRESP_USE_ALTERNATE_SERVICE = 380,
	SIPRESP_BAD_REQUEST = 400,
	SIPRESP_AUTHENTICATION_REQD = 401,
	SIPRESP_FORBIDDEN = 403,
	SIPRESP_NOT_FOUND = 404,
	SIPRESP_METHOD_NOT_ALLOWED = 405,
	SIPRESP_NOT_ACCEPTABLE = 406,
	SIPRESP_AUTHENTICATION_REQD_PROXY = 407,
	SIPRESP_TIMEOUT = 408,
	SIPRESP_NO_ANONYMOUS = 433,
	SIPRESP_TEMPORARILY_UNAVAILABLE = 480,
	SIPRESP_LOOP_DETECTED = 482,
	SIPRESP_BUSY_HERE = 486,
	SIPRESP_NOT_ACCEPTABLE_HERE = 488,
	SIPRESP_REINVITE_INSIDE_REINVITE = 491,
	SIPRESP_INTERNAL_ERROR = 500,
	SIPRESP_NOT_IMPLEMENTED = 501,
	SIPRESP_SERVICE_UNAVAILABLE = 503,
	SIPRESP_SERVER_TIMEOUT = 504,
	SIPRESP_BUSY_EVERYWHERE = 600,
	SIPRESP_DECLINE = 603,
};

// Other
#define SIP_VERSION_SEPARATOR "/"

#endif // __sip_contants_h_
