/*!
 * \file CstiCCI.cpp
 * \brief The Conference Communication Interface or CCI is an API class
 *
 * It provides functions for an application to interface with the
 * conferencing engine and other services
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2000 – 2020 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//
#include "CstiCCI.h"
#include <cctype>
#include <cstring>
#include "stiSVX.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "CstiWatchDog.h"
#include "stiTaskInfo.h"

#include "CstiConferenceManager.h"
#include "PropertyManager.h"
#include "CstiRoutingAddress.h"
#include "CstiVRCLServer.h"
#include "stiVRCLCommon.h"

#include "CstiHTTPTask.h"
#include "CstiCoreService.h"
#include "CstiStateNotifyService.h"
#include "CstiStateNotifyResponse.h"
#include "CstiMessageService.h"
#include "CstiConferenceService.h"

#include "CstiCall.h"
#include "CstiSipCallLeg.h"

#ifdef stiLDAP_CONTACT_LIST
#include "LDAPDirectoryContactListItem.h"
#endif

#include "ContactListItem.h"
#include "CstiBlockListMgrResponse.h"

#ifdef stiCALL_HISTORY_MANAGER
#include "CstiCallHistoryMgrResponse.h"
#endif

#include "IstiVideoInput.h"
#include "IstiAudioInput.h"
#include "IstiPlatform.h"
#include <set>
#include <sstream>
#include <array>
#include "IstiAudibleRinger.h"
#include "stiConfDefs.h"
#include "stiGUID.h"
#include <algorithm>
#include "stiRemoteLogEvent.h"
#include "CstiHostNameResolver.h"
#include "stiMessages.h"
#include "IstiContactManager.h"
#include "stiSipTools.h"
#include "EndpointMonitor.h"
#include <boost/lexical_cast.hpp>

using namespace WillowPM;

//
// Constants
//
#define VIDEO_SERVER_URL_DEFAULT	"video.sorensonvrs.com"
#define MAX_PHONE_NUMBER_WITH_EXT_LENGTH		16
#define TEXT_ONLY_GREETING_WAIT_TIME (30 * 1000) //30 seconds in milliseconds
#define UPLOAD_GUID_REQUEST_TIME (CstiHostNameResolver::nRESOLVER_TIMEOUT + 1000) // Resolver timeout + 1 second in milliseconds
#ifdef stiTHREAD_STATS
#define THREAD_STATS_TIME	(5 * 1000)
#endif

#define stiSVRS_ESPANOL_TOLL_FREE_NUMBER "18669877528"
#define stiSVRS_ESPANOL_LOCAL_NUMBER "18012879628"


//
// The max length of a boot time string.
//
#define BOOT_TIME_LENGTH	64

static const unsigned int unLOST_CONNECTION_DELAY = 30;  // Wait this number of seconds before sending a lost connection msg

static const int nGVC_STATUS_UPDATE_DELAY = 10000;  // Number of milliseconds
static const int nGVC_CREATE_DELAY = 30000; // 30 seconds

static const int DHVI_CREATE_DELAY = 10000; // 10 seconds

#define ALT_SIP_LISTEN_PORT_ENABLED_DEFAULT	1
#define ALT_SIP_LISTEN_PORT_DEFAULT	50060
#define ALT_SIP_TLS_LISTEN_PORT_ENABLED_DEFAULT 0
#define ALT_SIP_TLS_LISTEN_PORT_DEFAULT 50061

static const std::string g_callSVRSDomain = "call.svrs.tv";

//
// This structure and the following array are used by an algorithm that determines if the videophone
// should retrieve a phone setting from Enterprise Services or if it should send it to
// Enterpise Services.  Basically, if the videophone does not have the setting in its persistent
// property table it will ask core for the setting.  If core does not have the setting
// then the videophone will push the value it has (its default value) to core.
//

//
// Typedefs
//

enum EstiValidUriTypes
{
	eURI_SIP =  (1 << 1),
	eURI_SIPS = (1 << 2),
};

//
// Forward Declarations
//
static void PropertiesInitialize ();

//
// Globals
//

//
// Function Declarations
//

//:-----------------------------------------------------------------------------
//: Class Initialization Functions
//:-----------------------------------------------------------------------------

///
///\brief Creates the videophone engine
///
///\return A pointer to a CCI object with the IstiVideophoneEngine interface
///
IstiVideophoneEngine *videophoneEngineCreate (
	ProductType productType, 				///< The phone type (i.e. VP-100, VP-200, ntouch)
	const std::string &version,				 	///< The version number
	bool verifyAddress,					///< Indicates that E911 addresses must be validated
	bool reportDialMethodDetermination,	///< Inform the application when a dial method is determined
	const std::string &dynamicDataFolder,		///< The folder, under which, dynamic, persistent data will be written (e.g. Property Manager, etc)
	const std::string &staticDataFolder)		///< The folder, under which, static, persistent data will be read (e.g. certificates).
{
	return videophoneEngineCreate (productType, version, verifyAddress, reportDialMethodDetermination, dynamicDataFolder, staticDataFolder, nullptr, 0);
}


///
///\brief Creates the videophone engine
///
///\return A pointer to a CCI object with the IstiVideophoneEngine interface
///
IstiVideophoneEngine *videophoneEngineCreate (
	ProductType productType, 				///< The product type (i.e. VP-100, VP-200, ntouch)
	const std::string &version,				 	///< The version number
	bool verifyAddress,					///< Indicates that E911 addresses must be validated
	bool reportDialMethodDetermination,	///< Inform the application when a dial method is determined
	const std::string &dynamicDataFolder,		///< The folder, under which, dynamic, persistent data will be written (e.g. Property Manager, etc)
	const std::string &staticDataFolder,		///< The folder, under which, static, persistent data will be read (e.g. certificates).
	PstiAppGenericCallback callback,		///< Function called when the engine sends messages to the application
	size_t callbackParam)					///< The callback parameter used when the callback is called.
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IstiPlatform *platform = nullptr;
	CstiCCI * conference = nullptr;
	EstiInterfaceMode localInterfaceMode = estiSTANDARD_MODE;
	
	// Store the dynamic and static data folders with the OS layer.
	stiOSDynamicDataFolderSet (dynamicDataFolder);

	stiOSStaticDataFolderSet (staticDataFolder);

	DebugTools::instanceGet()->fileLoad();

	//
	// Initialize the watchdog timers.
	//
	stiOSWdInitialize ();
	CstiWatchDog::Instance().Start();

	// Start the shared event queue
	CstiEventQueue::sharedEventQueueGet ().StartEventLoop ();

	// Initialize the ErrorLogging system
	stiErrorLogSystemInit (version);

	PropertyManager::getInstance ()->init(version);

	PropertiesInitialize ();

	// If we are in Ported mode, disable the use of the PropertySender.
	PropertyManager::getInstance ()->propertyGet (cmLOCAL_INTERFACE_MODE, &localInterfaceMode);

#ifdef stiDISABLE_PORTED_MODE
	if (estiPORTED_MODE == localInterfaceMode)
	{
		localInterfaceMode = estiSTANDARD_MODE;
	}
#endif

	if (estiPORTED_MODE == localInterfaceMode)
	{
		PropertyManager::getInstance()->PropertySenderEnable (estiFALSE);
	}

	// Update the version number
	PropertyManager::getInstance()->propertySet (cmFIRMWARE_VERSION, version);

	//Initialize Certificates for use with HTTPS communication
	pocoSSLCertificatesInitialize();

	platform = IstiPlatform::InstanceGet ();

	hResult = platform->Initialize ();
	stiTESTRESULT ();

	// Initialize the os layer
	stiOSInit ();

	conference = new CstiCCI (localInterfaceMode,
								reportDialMethodDetermination);

	hResult = conference->Initialize (productType, version, verifyAddress, callback, callbackParam);
	stiTESTRESULT ();

	// NOTE: Log this after CstiCCI is initialized since the remote logging is not set up until then
#if APPLICATION == APP_NTOUCH_VP2
	// HACK/TODO: this call needs to be moved into the common bluetooth api once it's finished
	hResult = platform->BluetoothPairedDevicesLog ();
	stiTESTRESULT ();
#endif

	//
	// Start the task
	//
	conference->StartEventLoop ();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (conference)
		{
			stiTrace ("CreateVideophoneEngine() poConference failed to initialize\n");
			delete conference;
			conference = nullptr;
		}
		else
		{
			stiTrace ("CreateVideophoneEngine() pPlatform failed to initialize\n");
		}
	}

	return conference;
}


void DestroyVideophoneEngine (
	IstiVideophoneEngine *pEngine)
{
	//
	// Ensure that all threads are shutdown before destroying the engine.
	//

	// Dependency: must clean up this watchdog before it's referenced
	// timers
	CstiWatchDog::Instance ().Stop ();

	pEngine->Shutdown ();

	auto pCCI = (CstiCCI *)pEngine;

	delete pCCI;
	pCCI = nullptr;

	IstiPlatform::InstanceGet ()->Uninitialize ();

	PropertyManager::getInstance ()->uninit ();

	stiErrorLogSystemShutdown ();

	// Dependency: can't clean up the old watchdog unless all the
	// timers (which reference some watchdog state) are cleaned up
	stiOSWdUninitialize ();

	// Stop the shared event queue
	CstiEventQueue::sharedEventQueueGet ().StopEventLoop ();
}


/*!\brief Default Constructor
 *
 *  \return none.
 */
CstiCCI::CstiCCI (
	EstiInterfaceMode eLocalInterfaceMode,	///< Initial mode the object is created with
	bool bReportDialMethodDetermination)	///< Inform the application when a dial method is determined
:
	CstiEventQueue (stiCCI_TASK_NAME),
	m_eLocalInterfaceMode (eLocalInterfaceMode),
	m_textOnlyGreetingTimer (TEXT_ONLY_GREETING_WAIT_TIME, this),
	m_uploadGUIDRequestTimer (UPLOAD_GUID_REQUEST_TIME, this),
#ifdef stiTHREAD_STATS
	m_threadStatsTimer (THREAD_STATS_TIME, this),
#endif
	m_groupVideoChatRoomStatsTimer (nGVC_STATUS_UPDATE_DELAY, this),
	m_groupVideoChatRoomCreateTimer (nGVC_CREATE_DELAY, this),
	m_dhviRoomCreateTimer (DHVI_CREATE_DELAY, this),
	m_bReportDialMethodDetermination (bReportDialMethodDetermination)
{
	stiLOG_ENTRY_NAME (CstiCCI::CstiCCI);
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	m_EngineCreationTime = time (nullptr);
	m_bNeedsClockSync = true;
#endif

	m_RelayLanguageList.emplace_back (1, RELAY_LANGUAGE_ENGLISH);
	m_RelayLanguageList.emplace_back (2, RELAY_LANGUAGE_SPANISH);

	propertyChangedMethodsInitialize ();

	// Obtain the instance of the Property Manager System
	m_poPM = PropertyManager::getInstance ();

	// Initialize the last number dialed.
	m_lastDialed.clear ();

	vpe::DtlsContext::init();

#ifdef stiTHREAD_STATS
	m_signalConnections.push_back (startedSignal.Connect (
		[this]{
			m_threadStatsTimer.start ();
		}));
#endif

	m_signalConnections.push_back (m_textOnlyGreetingTimer.timeoutSignal.Connect (
		[this]{
			if (m_spLeaveMessageCall && m_spLeaveMessageCall->SubstateValidate(estiSUBSTATE_LEAVE_MESSAGE))
			{
				SstiMessageInfo stMessageInfo;
				auto &call = m_spLeaveMessageCall;
				auto hResult = call->MessageInfoGet(&stMessageInfo);
				// If the mailbox is full notify the UI.
				if (!stiIS_ERROR (hResult) && stMessageInfo.bMailBoxFull == estiTRUE)
				{
					hResult = RecordMessageMailBoxFull (call);
					stiASSERT(false);
				}
				else
				{
					// Start the countdown video.
					m_pFilePlay->LoadCountdownVideo();
				}
			}
		}));

	m_signalConnections.push_back (m_uploadGUIDRequestTimer.timeoutSignal.Connect (
		[this]{
			// Cancel SignMail upload GUID request
			MessageRequestCancel (m_unVideoMessageGUIDRequestId);
			m_unVideoMessageGUIDRequestId = 0;
			AppNotify (estiMSG_UPLOAD_GUID_REQUEST_FAILED, 0);
			stiASSERTMSG(false, "SignMail upload GUID request failed - timeout reached.");
		}));

#ifdef stiTHREAD_STATS
	m_signalConnections.push_back (m_threadStatsTimer.timeoutSignal.Connect (
		[this]{
			stiOSLinuxTaskTimeReport ();
			m_threadStatsTimer.restart();
		}));
#endif

	m_signalConnections.push_back (m_groupVideoChatRoomStatsTimer.timeoutSignal.Connect (
			std::bind(&CstiCCI::EventConferenceRoomStatsTimeout, this)));

	m_signalConnections.push_back (m_groupVideoChatRoomCreateTimer.timeoutSignal.Connect (
			std::bind(&CstiCCI::EventConferenceRoomCreateTimeout, this)));
	
	m_signalConnections.push_back (m_dhviRoomCreateTimer.timeoutSignal.Connect (
			std::bind(&CstiCCI::EventDhviRoomCreateTimeout, this)));
	}


/*! \brief Gets the system's public IP address
*
* NOTE:  It is the responsibility of the calling party to free memory allocated
* by this function (e.g. delete [] szIPAddress; ).
*
* \retval A pointer to a string allocated by this function containing the
* system's public IP address in the format "xxx.xxx.xxx.xxx" where "xxx" is
* a number between 0 and 255.
*/
stiHResult CstiCCI::PublicIPAddressGet (
	std::string *pPublicIPAddress,
	EstiIpAddressType eIpAddressType)
{
	stiLOG_ENTRY_NAME (PublicIPAddressGet);

	std::string ipAddress;
	EstiPublicIPAssignMethod eIPAssignMethod = estiIP_ASSIGN_AUTO;
	PropertyManager* pPM = PropertyManager::getInstance ();
	stiHResult hResult = stiRESULT_SUCCESS;

	if (eIpAddressType == estiTYPE_IPV6)
	{
		if (m_bIPv6Enabled)
		{
			hResult = stiGetLocalIp (pPublicIPAddress, eIpAddressType);
			stiTESTRESULT ();
		}
	}
	else
	{
		// Get the current public IP assignment method.
		pPM->propertyGet (cmPUBLIC_IP_ASSIGN_METHOD, &eIPAssignMethod, PropertyManager::Persistent);

		int nRet = 0;

		switch (eIPAssignMethod)
		{
			case estiIP_ASSIGN_MANUAL:
			{
				nRet = pPM->propertyGet (cmPUBLIC_IP_ADDRESS_MANUAL, &ipAddress);
				if (PM_RESULT_SUCCESS == nRet)
				{
					pPublicIPAddress->assign (ipAddress);
				} // end if
				else if (PM_RESULT_NOT_FOUND == nRet)
				{
					// The return from propertyGet signifies that the property simply wasn't in the system.
					// Set hResult appropriately.
					stiTESTCOND (estiFALSE, stiRESULT_PM_PROPERTY_NOT_FOUND);
				}

				break;
			}

			case estiIP_ASSIGN_SYSTEM:
			{
				// Use the system's IP address.
				hResult = stiGetLocalIp (pPublicIPAddress, eIpAddressType);
				stiTESTRESULT ();

				break;
			}

			case estiIP_ASSIGN_AUTO:
			{
				nRet = pPM->propertyGet (cmPUBLIC_IP_ADDRESS_AUTO, &ipAddress, PropertyManager::Persistent);
				if (PM_RESULT_SUCCESS == nRet)
				{
					pPublicIPAddress->assign (ipAddress);
				} // end if
				else if (PM_RESULT_NOT_FOUND == nRet)
				{
					// The return from propertyGet signifies that the property simply wasn't in the system.
					// Set hResult appropriately.
					stiTESTCOND_NOLOG (estiFALSE, stiRESULT_PM_PROPERTY_NOT_FOUND);
				}

				break;
			}
		}

		// Is the address we've obtained a valid IP address?
		stiTESTCOND (IPAddressValidate (pPublicIPAddress->c_str ()), stiRESULT_ERROR);
	}

STI_BAIL:

	return (hResult);
} // end PublicIPAddressGet


static void ServiceContactsGet (
	const char *pszServiceUrlName1,
	const char *pszServiceUrlName2,
	std::string *pServiceContacts)
{
	std::string url;

	PropertyManager *pPM = PropertyManager::getInstance ();
	pPM->propertyGet (pszServiceUrlName1, &url);
	pServiceContacts[0] = url;

	pPM->propertyGet (pszServiceUrlName2, &url);
	pServiceContacts[1] = url;
}


static void ServiceContactsSet (
	CstiVPService *pService,
	const char *pszServiceUrlName1,
	const char *pszServiceUrlName2)
{
	std::string ServiceContacts[MAX_SERVICE_CONTACTS];

	ServiceContactsGet (
		pszServiceUrlName1,
		pszServiceUrlName2,
		ServiceContacts);

	pService->ServiceContactsSet (ServiceContacts);
}


static void PropertiesInitialize ()
{
	PropertyManager *poPM = PropertyManager::getInstance();

	// Placing the value vpe::ServiceOutageClient::DefaultTimerInterval results in a link error.
	// Creating a local variable to work around this issue.
	auto serviceOutageDefaultTimerInterval = vpe::ServiceOutageClient::DefaultTimerInterval;

	const std::vector<std::pair<std::string, int>> intProperties =
	{
		{AltSipListenPortEnabled, ALT_SIP_LISTEN_PORT_ENABLED_DEFAULT},
		{AltSipListenPort, ALT_SIP_LISTEN_PORT_DEFAULT},
		{AltSipTlsListenPortEnabled, ALT_SIP_TLS_LISTEN_PORT_ENABLED_DEFAULT},
		{AltSipTlsListenPort, ALT_SIP_TLS_LISTEN_PORT_DEFAULT},
		{CoreServiceSSLFailOver, stiSSL_DEFAULT_MODE},
		{StateNotifySSLFailOver, stiSSL_DEFAULT_MODE},
		{MessageServiceSSLFailOver, stiSSL_DEFAULT_MODE},
		{ConferenceServiceSSLFailOver, stiSSL_DEFAULT_MODE},
		{RemoteLoggerServiceSSLFailOver, stiSSL_DEFAULT_MODE},
		{cmPUBLIC_IP_ASSIGN_METHOD, estiIP_ASSIGN_AUTO},
		{NAT_TRAVERSAL_SIP, stiNAT_TRAVERSAL_SIP_DEFAULT},
		{SIP_NAT_TRANSPORT, stiSIP_NAT_TRANSPORT_DEFAULT},
		{IPv6Enabled, stiIPV6_ENABLED_DEFAULT},
		{AUTO_SPEED_MODE, stiAUTO_SPEED_MODE_DEFAULT},
		{cmMAX_RECV_SPEED, stiMAX_RECV_SPEED_DEFAULT},
		{cmMAX_SEND_SPEED, stiMAX_SEND_SPEED_DEFAULT},
		{SERVICE_OUTAGE_TIMER_INTERVAL, serviceOutageDefaultTimerInterval},
		{HEVC_SIGNMAIL_PLAYBACK_ENABLED, stiHEVC_SIGNMAIL_PLAYBACK_ENABLED_DEFAULT},
		{RING_GROUP_DISPLAY_MODE, stiRING_GROUP_MODE_DEFAULT},
		{RING_GROUP_ENABLED, stiRING_GROUP_ENABLED_DEFAULT},
		{FAVORITES_ENABLED, stiFAVORITES_ENABLED_DEFAULT},
		{BLOCK_CALLER_ID_ENABLED, stiBLOCK_CALLER_ID_ENABLED_DEFAULT},
		{BLOCK_CALLER_ID, 0},
		{BLOCK_ANONYMOUS_CALLERS, 0},
		{GROUP_VIDEO_CHAT_ENABLED, stiGROUP_VIDEO_CHAT_ENABLED_DEFAULT},
		{LDAP_ENABLED, stiLDAP_ENABLED_DEFAULT},
		{LDAP_SERVER_PORT, STANDARD_LDAP_PORT},
		{LDAP_SERVER_USE_SSL, 0},
		{LDAP_SERVER_REQUIRES_AUTHENTICATION, 0},
		{LDAP_SCOPE, 0},
		{NEW_CALL_ENABLED, stiNEW_CALL_ENABLED_DEFAULT},
#ifdef stiINTERNET_SEARCH
		{INTERNET_SEARCH_ENABLED, stiINTERNET_SEARCH_ENABLED_DEFAULT},
		{INTERNET_SEARCH_ALLOWED, 0},
#endif
		{CONTACT_IMAGES_ENABLED, stiCONTACT_PHOTOS_ENABLED_DEFAULT},
		{DTMF_ENABLED, stiDTMF_ENABLED_DEFAULT},
		{REAL_TIME_TEXT_ENABLED, stiREAL_TIME_TEXT_ENABLED_DEFAULT},
		{PERSONAL_GREETING_ENABLED, stiPSMG_ENABLED_DEFAULT},
		{CALL_TRANSFER_ENABLED, stiCALL_TRANSFER_ENABLED_DEFAULT},
		{AGREEMENTS_ENABLED, stiAGREEMENTS_ENABLED_DEFAULT},
		{AUTO_SPEED_ENABLED, stiAUTO_SPEED_ENABLED_DEFAULT},
		{GREETING_PREFERENCE, eGREETING_DEFAULT},
		{DIRECT_SIGNMAIL_ENABLED, stiDIRECT_SIGNMAIL_ENABLED_DEFAULT},
		{cmRINGS_BEFORE_GREETING, stiRINGS_BEFORE_GREETING_DEFAULT},
		{cmLOCAL_INTERFACE_MODE, estiSTANDARD_MODE},
		{cmHEARTBEAT_DELAY, nDEFAULT_HEARTBEAT_DELAY},
		{REMOTE_TRACE_LOGGING, stiREMOTE_LOG_TRACE_DEFAULT},
		{REMOTE_ASSERT_LOGGING, stiREMOTE_LOG_ASSERT_DEFAULT},
		{REMOTE_CALL_STATS_LOGGING, stiREMOTE_LOG_CALL_STATS_DEFAULT},
		{REMOTE_LOG_EVENT_SEND_LOGGING, stiREMOTE_LOG_EVENT_SEND_DEFAULT},
		{BLOCK_LIST_ENABLED, stiBLOCK_LIST_ENABLED_DEFAULT},
		{AUTO_KEYFRAME_SEND, 5000},
		{cmLOW_FRM_RATE_THRSHLD_H264, 128000},
		{cmLOW_FRM_RATE_THRSHLD_H263, 180000},
		{CLIENT_REREGISTER_MAX_LOOKUP_TIME, CLIENT_REREGISTER_MAX_LOOKUP_TIME_DEFAULT},
		{CLIENT_REREGISTER_MIN_LOOKUP_TIME, CLIENT_REREGISTER_MIN_LOOKUP_TIME_DEFAULT},
		{cmCHECK_FOR_AUTH_NUMBER, 1},
		// We will want to enable this default setting here if we ever have all interfaces synched.
//		{SECURE_CALL_MODE, stiSECURE_CALL_MODE_DEFAULT},
		{AUTO_SPEED_MIN_START_SPEED, stiAUTO_SPEED_MIN_START_SPEED_DEFAULT},
		{cmSIGNMAIL_ENABLED, stiSIGNMAIL_ENABLED_DEFAULT},
		{cmPREFERRED_AUDIO_CODEC, estiAUDIO_NONE},
		{cmPREFERRED_VIDEO_CODEC, estiVIDEO_NONE},
		{ALLOWED_VIDEO_ENCODERS, estiVIDEOCODEC_ALL},
		{FIR_FEEDBACK_ENABLED, stiFIR_FEEDBACK_ENABLED_DEFAULT},
		{PLI_FEEDBACK_ENABLED, stiPLI_FEEDBACK_ENABLED_DEFAULT},
		{AFB_FEEDBACK_ENABLED, stiAFB_FEEDBACK_ENABLED_DEFAULT},
		{TMMBR_FEEDBACK_ENABLED, stiTMMBR_FEEDBACK_ENABLED_DEFAULT},
		{NACK_RTX_FEEDBACK_ENABLED, stiNACKRTX_FEEDBACK_ENABLED_DEFAULT},
		{VRS_FAILOVER_TIMEOUT, stiVRS_FAILOVER_TIMEOUT_DEFAULT},
		{UNREGISTERED_SIP_RESTART_TIME, stiSTACK_RESTART_DELTA_DEFAULT},
		{cmUDP_TRACE_ENABLED, estiFALSE},
		{LOCAL_FROM_DIAL_TYPE, estiDM_UNKNOWN},
		{PERSONAL_GREETING_TYPE, eGREETING_NONE},
		{HEVC_SIGNMAIL_PLAYBACK_ENABLED, stiHEVC_SIGNMAIL_PLAYBACK_ENABLED_DEFAULT},
		{DEAF_HEARING_VIDEO_ENABLED, stiDEAF_HEARING_VIDEO_ENABLED_DEFAULT},
		{ENABLE_CALL_BRIDGE, 1},
		{DEVICE_LOCATION_TYPE, static_cast<int>(DEVICE_LOCATION_TYPE_DEFAULT)}
	};

	for (auto &property : intProperties)
	{
		poPM->propertyDefaultSet (property.first, property.second);
	}

	const std::vector<std::pair<std::string, std::string>> stringProperties =
	{
		{cmCORE_SERVICE_URL1, stiCORE_SERVICE_DEFAULT1},
		{cmCORE_SERVICE_ALT_URL1, stiCORE_SERVICE_DEFAULT2},
		{cmSTATE_NOTIFY_SERVICE_URL1, stiSTATE_NOTIFY_DEFAULT1},
		{cmSTATE_NOTIFY_SERVICE_ALT_URL1, stiSTATE_NOTIFY_DEFAULT2},
		{cmMESSAGE_SERVICE_URL1, stiMESSAGE_SERVICE_DEFAULT1},
		{cmMESSAGE_SERVICE_ALT_URL1, stiMESSAGE_SERVICE_DEFAULT2},
		{CONFERENCE_SERVICE_URL1, stiCONFERENCE_SERVICE_DEFAULT1},
		{CONFERENCE_SERVICE_ALT_URL1, stiCONFERENCE_SERVICE_DEFAULT2},
		{REMOTELOGGER_SERVICE_URL1, stiREMOTELOGGER_SERVICE_DEFAULT1},
		{REMOTELOGGER_SERVICE_ALT_URL1, stiREMOTELOGGER_SERVICE_DEFAULT2},
		{SERVICE_OUTAGE_SERVICE_URL1, stiSERVICE_OUTAGE_SERVER_DEFAULT},
#ifdef stiLDAP_CONTACT_LIST
		{LDAP_TELEPHONE_NUMBER_FIELD, LDAP_TELEPHONE_NUMBER_FIELD_DEFAULT},
		{LDAP_HOME_NUMBER_FIELD, LDAP_HOME_NUMBER_FIELD_DEFAULT},
		{LDAP_MOBILE_NUMBER_FIELD, LDAP_MOBILE_NUMBER_FIELD_DEFAULT},
		{LDAP_DIRECTORY_DISPLAY_NAME, LDAP_DIRECTORY_DISPLAY_NAME_DEFAULT},
#endif
		{TUNNEL_SERVER, stiTUNNEL_SERVER_DNS_NAME_DEFAULT},

		// When core starts providing these values in the RegistrationInfoGet core response
		// then these defaults need to be removed.
		{SIP_AGENT_DOMAIN_OVERRIDE, stiSIP_AGENT_DOMAIN_OVERRIDE_DEFAULT},
		{SIP_AGENT_DOMAIN_ALT_OVERRIDE, stiSIP_AGENT_DOMAIN_ALT_OVERRIDE_DEFAULT},

		{TURN_SERVER, stiTURN_SERVER_DNS_NAME_DEFAULT},
		{TURN_SERVER_ALT, stiTURN_SERVER_DNS_NAME_ALT_DEFAULT},
		{TURN_SERVER_USERNAME, stiTURN_SERVER_USER_NAME_DEFAULT},
		{TURN_SERVER_PASSWORD, stiTURN_SERVER_PASSWORD_DEFAULT},
		{cmVRS_SERVER, stiVRS_SERVER_DEFAULT},
		{cmVRS_ALT_SERVER, stiVRS_ALT_SERVER_DEFAULT},
		{SPANISH_VRS_SERVER, SPANISH_VRS_SERVER_DEFAULT},
		{SPANISH_VRS_ALT_SERVER, SPANISH_VRS_ALT_SERVER_DEFAULT},
		{CUSTOMER_INFORMATION_URI, stiCUSTOMER_INFORMATION_URI_DEFAULT},
		{DefUserName, "Unknown"},
		{VRS_FAILOVER_SERVER, stiVRS_FAILOVER_SERVER_DEFAULT},
		
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_ANDROID
		{ "TelecomCarrier", "Unknown" },
#endif
		{DHV_API_URL, stiDHV_API_URL_DEFAULT},
		{DHV_API_KEY, stiDHV_API_KEY_DEFAULT},
		{ENDPOINT_MONITOR_SERVER, "https://ems.svrs.cc/ws"}
	};

	for (auto &property : stringProperties)
	{
		poPM->propertyDefaultSet (property.first, property.second);
	}

	//
	// Get, increment and set the boot count property.  This value will be sent to
	// core upon authentication.
	//
	int nCount = 0;

	poPM->propertyGet (BOOT_COUNT, &nCount);

	++nCount;

	poPM->propertySet (BOOT_COUNT, nCount, PropertyManager::Persistent);
}


static void LoadPortSettingsProperties (
	SstiConferencePortsSettings *pPorts)
{
	// Set the ports that we will use in the conference engine to be those stored in persistent data.
	PropertyManager *pPropertyManager = PropertyManager::getInstance();

	pPropertyManager->propertyGet (AltSipListenPortEnabled, &pPorts->bUseSecondSIPListenPort, PropertyManager::Persistent);
	pPropertyManager->propertyGet (AltSipListenPort, &pPorts->nSecondSIPListenPort, PropertyManager::Persistent);

	pPropertyManager->propertyGet (AltSipTlsListenPortEnabled, &pPorts->bUseAltSIPTlsListenPort, PropertyManager::Persistent);
	pPropertyManager->propertyGet (AltSipTlsListenPort, &pPorts->nAltSIPTlsListenPort, PropertyManager::Persistent);
}


static void SavePortSettingsProperties (
	const SstiConferencePortsSettings *pPorts)
{
	PropertyManager *pPropertyManager = PropertyManager::getInstance();

	pPropertyManager->propertySet (AltSipListenPortEnabled, pPorts->bUseSecondSIPListenPort ? 1 : 0,
									  PropertyManager::Persistent);
	pPropertyManager->propertySet (AltSipListenPort, pPorts->nSecondSIPListenPort,
									  PropertyManager::Persistent);
}


static void CurrentPortsPropertiesSet (
	const SstiConferencePortsStatus *pPorts)
{
	PropertyManager *poPM = PropertyManager::getInstance();

	poPM->propertySet (CurrentSipListenPort, pPorts->bUseAltSIPListenPort
								 ? pPorts->nAltSIPListenPort : nDEFAULT_SIP_LISTEN_PORT, PropertyManager::Temporary);
}


stiHResult CstiCCI::initializeHTTP ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string httpProxyAddressAndPort;

	//
	// Get the http proxy address and port from the property table.
	//
	m_poPM->propertyGet (HTTP_PROXY_ADDRESS, &httpProxyAddressAndPort);

	if (!httpProxyAddressAndPort.empty ())
	{
		std::string httpProxyServer;
		uint16_t httpProxyPort = 0;

		AddressAndPortParse (httpProxyAddressAndPort.c_str (), &httpProxyServer, &httpProxyPort);

		CstiHTTPService::ProxyAddressSet (httpProxyServer, httpProxyPort);
	}

	//
	// Initialize the HTTP services
	//
	m_pHTTPTask = new CstiHTTPTask (ThreadCallbackHTTP, (size_t)this);
	stiTESTCOND (m_pHTTPTask != nullptr, stiRESULT_ERROR);


	hResult = m_pHTTPTask->Initialize ();
	stiTESTRESULT ();

	// Now one for just the Logger
	m_pHTTPLoggerTask = new CstiHTTPTask (ThreadCallbackHTTP, (size_t)this);
	stiTESTCOND (m_pHTTPLoggerTask != nullptr, stiRESULT_ERROR);

	hResult = m_pHTTPLoggerTask->Initialize ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::initializeCoreService (
	ProductType productType,
	const std::string &uniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string ServiceContacts[MAX_SERVICE_CONTACTS];

	//
	// Initialize the core service subsystem
	//
	ServiceContactsGet (
		cmCORE_SERVICE_URL1,
		cmCORE_SERVICE_ALT_URL1,
		ServiceContacts);

	m_pCoreService = new CstiCoreService (m_pHTTPTask, ThreadCallbackCoreService, (size_t)this);

	m_pCoreService->offlineCore.authenticate = [this](const std::string& phoneNumber, const std::string& pin)
	{
		return m_pRegistrationManager->CredentialsCheck (*phoneNumber.c_str (), *pin.c_str ());
	};

	m_pCoreService->offlineCore.registrationInfoGet = [this](SstiRegistrationInfo& info)
	{
		info = m_pRegistrationManager->SipRegistrationInfoGet ();
	};

	m_pCoreService->offlineCore.userPhoneNumbersGet = [this](SstiUserPhoneNumbers *phoneNumbers)
	{
		m_pRegistrationManager->PhoneNumbersGet (phoneNumbers);
	};

	m_pCoreService->offlineCore.propertyStringGet = [this](const std::string &propertyKey)
	{
		std::string value;
		m_poPM->propertyGet (propertyKey, &value);
		return value;
	};

	m_pCoreService->offlineCore.propertyIntegerGet = [this](const std::string& propertyKey)
	{
		int value = 0;
		m_poPM->propertyGet (propertyKey, &value);
		return value;
	};

	m_pCoreService->offlineCore.remoteLoggerPhoneNumberSet = [this](const std::string& phoneNumber)
	{
		m_pRemoteLoggerService->LoggedInPhoneNumberSet (phoneNumber.c_str ());
	};

	m_pCoreService->offlineCore.remoteLog = [](const std::string& logMessage)
	{
		stiRemoteLogEventSend (logMessage.c_str());
	};

	m_pCoreService->offlineCore.userAccountInfoGet = [this](CstiUserAccountInfo& userAccountInfo)
	{
		if (m_userAccountManager->cacheAvailable ())
		{
			userAccountInfo = m_userAccountManager->userAccountInfoGet ();
			return true;
		}
		return false;
	};

	m_pCoreService->offlineCore.emergencyAddressGet = [this](vpe::Address& address)
	{
		if (m_userAccountManager->cacheAvailable ())
		{
			address = m_userAccountManager->emergencyAddressGet ();
			return true;
		}
		return false;
	};

	m_pCoreService->offlineCore.emergencyAddressStatusGet = [this](EstiEmergencyAddressProvisionStatus& status)
	{
		if (m_userAccountManager->cacheAvailable ())
		{
			status = m_userAccountManager->addressStatusGet ();
			return true;
		}
		return false;
	};

	m_pCoreService->offlineCore.myPhoneGroupListGet = [this](std::vector<CstiCoreResponse::SRingGroupInfo>& myPhoneGroupList)
	{
		myPhoneGroupList.clear ();
		for (int i = 0; i < m_pRingGroupManager->ItemCountGet(); i++)
		{
			CstiCoreResponse::SRingGroupInfo item{};
			IstiRingGroupManager::ERingGroupMemberState state{};
			m_pRingGroupManager->ItemGetByIndex (i, &item.LocalPhoneNumber, &item.TollFreePhoneNumber, &item.Description, &state);
			item.eState = static_cast<CstiCoreResponse::ERingGroupMemberState>(state);
			myPhoneGroupList.push_back (item);
		}
	};

	m_signalConnections.push_back (m_pCoreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		PostEvent (std::bind (&CstiCCI::clientAuthenticateHandle, this, result));
	}));

	m_signalConnections.push_back (m_pCoreService->userAccountInfoReceivedSignal.Connect ([this](const ServiceResponseSP<CstiUserAccountInfo>& userAccountInfoResult) {
		PostEvent (std::bind (&CstiCCI::userAccountInfoGetHandle, this, userAccountInfoResult));
	}));

	m_signalConnections.push_back (m_pCoreService->emergencyAddressReceivedSignal.Connect ([this](const ServiceResponseSP<vpe::Address>& emergencyAddress) {
		PostEvent ([this, emergencyAddress] {
			emergencyAddressReceivedSignal.Emit (emergencyAddress);
		});
	}));

	m_signalConnections.push_back (m_pCoreService->emergencyAddressStatusReceivedSignal.Connect ([this](const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>& addressStatus) {
		PostEvent ([this, addressStatus] {
			emergencyAddressStatusReceivedSignal.Emit (addressStatus);
		});
	}));

	hResult = m_pCoreService->Initialize (ServiceContacts, uniqueID, productType);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::initializeStateNotifyService (
	const std::string &uniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string ServiceContacts[MAX_SERVICE_CONTACTS];

	//
	// Initialize the state notify service subsystem
	//
	ServiceContactsGet (
		cmSTATE_NOTIFY_SERVICE_URL1,
		cmSTATE_NOTIFY_SERVICE_ALT_URL1,
		ServiceContacts);

	m_pStateNotifyService = new CstiStateNotifyService (m_pCoreService, m_pHTTPTask,
														ThreadCallbackStateNotify, (size_t)this);
	hResult = m_pStateNotifyService->Initialize (ServiceContacts, uniqueID);
	stiTESTRESULT ();

	{
		int nInterval {};

		m_poPM->propertyGet(cmHEARTBEAT_DELAY, &nInterval);

		m_pStateNotifyService->HeartbeatDelaySet (nInterval);
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::initializeMessageService (
	const std::string &uniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string ServiceContacts[MAX_SERVICE_CONTACTS];

	//
	// Initialize the message service subsystem
	//
	ServiceContactsGet (
		cmMESSAGE_SERVICE_URL1,
		cmMESSAGE_SERVICE_ALT_URL1,
		ServiceContacts);

	m_pMessageService = new CstiMessageService (m_pCoreService, m_pHTTPTask, ThreadCallbackMessageService,
												(size_t)this);
	hResult = m_pMessageService->Initialize (ServiceContacts, uniqueID);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::initializeConferenceService (
	const std::string &uniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string ServiceContacts[MAX_SERVICE_CONTACTS];

	//
	// Initialize the conference service subsystem
	//
	ServiceContactsGet (
		CONFERENCE_SERVICE_URL1,
		CONFERENCE_SERVICE_ALT_URL1,
		ServiceContacts);

	m_pConferenceService = new CstiConferenceService (m_pCoreService, m_pHTTPTask, ThreadCallbackConferenceService,
												(size_t)this);
	hResult = m_pConferenceService->Initialize (ServiceContacts, uniqueID);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


void initializeEndpointMonitor ()
{
	std::string endpointMonitorServer;

	PropertyManager::getInstance()->propertyGet (ENDPOINT_MONITOR_SERVER, &endpointMonitorServer);

	vpe::EndpointMonitor::instanceGet()->urlSet(endpointMonitorServer);

	// Ensure endpoint monitoring is off upon initialization and send setting to core
	PropertyManager::getInstance()->propertySet (ENDPOINT_MONITOR_ENABLED, 0, PropertyManager::Persistent);
	PropertyManager::getInstance()->PropertySend (ENDPOINT_MONITOR_ENABLED, estiPTypePhone);
}


stiHResult CstiCCI::initializeCloudServices (
	ProductType productType,
	const std::string &uniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = initializeCoreService (productType, uniqueID);
	stiTESTRESULT ();

	hResult = initializeStateNotifyService (uniqueID);
	stiTESTRESULT ();

	hResult = initializeMessageService (uniqueID);
	stiTESTRESULT ();

	hResult = initializeConferenceService (uniqueID);
	stiTESTRESULT ();

	//
	// Inform core service of the other services.
	//
	hResult = m_pCoreService->StateNotifyServiceSet (m_pStateNotifyService);
	stiTESTRESULT ();

	hResult = m_pCoreService->MessageServiceSet (m_pMessageService);
	stiTESTRESULT ();

	hResult = m_pCoreService->ConferenceServiceSet (m_pConferenceService);
	stiTESTRESULT ();

	//
	// Get the SSL flags
	//
	{
		ESSLFlag coreSSLFlag = stiSSL_DEFAULT_MODE;
		ESSLFlag stateNotifySSLFlag = stiSSL_DEFAULT_MODE;
		ESSLFlag messageSSLFlag = stiSSL_DEFAULT_MODE;
		ESSLFlag conferenceSSLFlag = stiSSL_DEFAULT_MODE;

		PropertyManager::getInstance()->propertyGet (CoreServiceSSLFailOver, &coreSSLFlag);
		PropertyManager::getInstance()->propertyGet (StateNotifySSLFailOver, &stateNotifySSLFlag);
		PropertyManager::getInstance()->propertyGet (MessageServiceSSLFailOver, &messageSSLFlag);
		PropertyManager::getInstance()->propertyGet (ConferenceServiceSSLFailOver, &conferenceSSLFlag);

		m_pCoreService->SSLFlagSet(coreSSLFlag);
		m_pStateNotifyService->SSLFlagSet(stateNotifySSLFlag);
		m_pMessageService->SSLFlagSet(messageSSLFlag);
		m_pConferenceService->SSLFlagSet (conferenceSSLFlag);
	}

	//
	// Send the current SSL fail over values
	//
	m_poPM->PropertySend(CoreServiceSSLFailOver, estiPTypePhone);
	m_poPM->PropertySend(StateNotifySSLFailOver, estiPTypePhone);
	m_poPM->PropertySend(MessageServiceSSLFailOver, estiPTypePhone);
	m_poPM->PropertySend(ConferenceServiceSSLFailOver, estiPTypePhone);

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::initializeLoggingService (
	ProductType productType,
	const std::string &version,
	const std::string &uniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string ServiceContacts[MAX_SERVICE_CONTACTS];

	//
	// Initialize the remote logging service subsystem
	//
	ServiceContactsGet (
		REMOTELOGGER_SERVICE_URL1,
		REMOTELOGGER_SERVICE_ALT_URL1,
		ServiceContacts);

	m_pRemoteLoggerService = new CstiRemoteLoggerService (m_pHTTPLoggerTask, ThreadCallbackRemoteLoggerService,
														  (size_t)this);

	//Get the current trace logging value or zero (off)
	int nRemoteLoggingTraceVal;
	nRemoteLoggingTraceVal = stiREMOTE_LOG_TRACE_DEFAULT;
	m_poPM->propertyGet (REMOTE_TRACE_LOGGING, &nRemoteLoggingTraceVal);

	//Get the current assert logging value or zero (off)
	int nRemoteLoggingAssertVal;
	nRemoteLoggingAssertVal = stiREMOTE_LOG_ASSERT_DEFAULT;
	m_poPM->propertyGet (REMOTE_ASSERT_LOGGING, &nRemoteLoggingAssertVal);

	//Get the current call stats logging value or zero (off)
	m_poPM->propertyGet (REMOTE_CALL_STATS_LOGGING, &m_bRemoteLoggingCallStatsEnabled);

	//Get the current event logging value or zero (off)
	int nRemoteLoggingEventVal;
	nRemoteLoggingEventVal = stiREMOTE_LOG_EVENT_SEND_DEFAULT;
	m_poPM->propertyGet (REMOTE_LOG_EVENT_SEND_LOGGING, &nRemoteLoggingEventVal);

	hResult = m_pRemoteLoggerService->Initialize (ServiceContacts,
				uniqueID,
				version,
				productType,
				nRemoteLoggingTraceVal,
				nRemoteLoggingAssertVal,
				nRemoteLoggingEventVal);
	stiTESTRESULT ();

	stiTraceRemoteLoggingSet((size_t*) m_pRemoteLoggerService, CstiRemoteLoggerService::RemoteTraceSend);
	stiAssertRemoteLoggingSet((size_t*) m_pRemoteLoggerService, CstiRemoteLoggerService::RemoteAssertSend);
	stiErrorLogRemoteLoggingSet((size_t *)m_pRemoteLoggerService, CstiRemoteLoggerService::RemoteAssertSend);
	stiRemoteLogEventRemoteLoggingSet((size_t *)m_pRemoteLoggerService, CstiRemoteLoggerService::RemoteLogEventSend);

	// Inform core service of remote logging service
	hResult = m_pCoreService->RemoteLoggerServiceSet (m_pRemoteLoggerService);
	stiTESTRESULT ();

	{
		ESSLFlag remoteLoggerSSLFlag = stiSSL_DEFAULT_MODE;
		PropertyManager::getInstance()->propertyGet (RemoteLoggerServiceSSLFailOver, &remoteLoggerSSLFlag);
		m_pRemoteLoggerService->SSLFlagSet(remoteLoggerSSLFlag);
		m_poPM->PropertySend(RemoteLoggerServiceSSLFailOver, estiPTypePhone);
	}

STI_BAIL:

	return hResult;
}


static void ConferencePortsCopy (
	const SstiConferencePortsSettings *pSettings,
	SstiConferencePorts *pPorts)
{
	if (pSettings->bUseSecondSIPListenPort)
	{
		pPorts->nSIPListenPort = pSettings->nSecondSIPListenPort;
	}
	else
	{
		pPorts->nSIPListenPort = nDEFAULT_SIP_LISTEN_PORT;
	}

	if (pSettings->bUseAltSIPTlsListenPort)
	{
		pPorts->nSIPTLSListenPort = pSettings->nAltSIPTlsListenPort;
	}
	else
	{
		pPorts->nSIPTLSListenPort = nDEFAULT_TLS_SIP_LISTEN_PORT;
	}
}


stiHResult CstiCCI::initializeConferenceManager (
	ProductType productType,
	const std::string &version,
	bool verifyAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	PropertyManager *pPM = PropertyManager::getInstance ();
	std::string publicDomainOverride;
	std::string publicDomainAltOverride;
	std::string agentDomainOverride;
	std::string agentDomainAltOverride;
	int nResult {0};
	std::string ipAddress;
	bool checkForAuthPhoneNumber {true};
	std::string instanceGUID;
	int nFIRRequestsEnabled {stiFIR_FEEDBACK_ENABLED_DEFAULT};
	int pliFeedbackEnabled {stiPLI_FEEDBACK_ENABLED_DEFAULT};
	int afbFeedbackEnabled {stiAFB_FEEDBACK_ENABLED_DEFAULT};
	int tmmbrFeedbackEnabled {stiTMMBR_FEEDBACK_ENABLED_DEFAULT};
	int nackRtxFeedbackEnabled {stiNACKRTX_FEEDBACK_ENABLED_DEFAULT};
	SstiConferencePorts stConferencePorts;
	std::string vrsFocusedRouting;

	LoadPortSettingsProperties (&m_stConferencePortsSettings);

	ConferencePortsCopy (&m_stConferencePortsSettings, &stConferencePorts);

	m_stConferencePortsStatus.bUseAltSIPListenPort = m_stConferencePortsSettings.bUseSecondSIPListenPort;
	m_stConferencePortsStatus.nAltSIPListenPort = m_stConferencePortsSettings.nSecondSIPListenPort;
	m_stConferencePortsStatus.bUseAltSIPTlsListenPort = m_stConferencePortsSettings.bUseAltSIPTlsListenPort;
	m_stConferencePortsStatus.nAltSIPTlsListenPort = m_stConferencePortsSettings.nAltSIPTlsListenPort;

	CurrentPortsPropertiesSet (&m_stConferencePortsStatus);

	stConferenceParams.bIPv6Enabled = m_bIPv6Enabled;

	//Get the current flow control logging value or zero (off)
	m_poPM->propertyGet (REMOTE_FLOW_CONTROL_LOGGING, &stConferenceParams.bRemoteFlowControlLogging);

	//
	// Indicate which tunnel server to use.
	//
	{
		std::string tunnelServer;
		m_poPM->propertyGet (TUNNEL_SERVER, &tunnelServer);
		stConferenceParams.TunnelServer.AddressSet(tunnelServer.c_str ());
	}

	//
	// Initialize the conference manager.
	//
	m_pConferenceManager = new CstiConferenceManager ();
	conferenceManagerSignalsConnect ();

	stConferenceParams.stMediaDirection.bInboundAudio = true;
	stConferenceParams.stMediaDirection.bOutboundAudio = true;
	stConferenceParams.stMediaDirection.bInboundVideo = true;
	stConferenceParams.stMediaDirection.bOutboundVideo = true;
	stConferenceParams.bVerifyAddress = verifyAddress;

	pPM->propertyGet (AUTO_KEYFRAME_SEND, &stConferenceParams.nAutoKeyframeSend);

	pPM->propertyGet (cmLOW_FRM_RATE_THRSHLD_H264, &stConferenceParams.unLowFrameRateThreshold264);
	pPM->propertyGet (cmLOW_FRM_RATE_THRSHLD_H263, &stConferenceParams.unLowFrameRateThreshold263);

	{
		int nAutoSpeedEnabled = stiAUTO_SPEED_ENABLED_DEFAULT;
		EstiAutoSpeedMode eAutoSpeedMode = stiAUTO_SPEED_MODE_DEFAULT;
		pPM->propertyGet (AUTO_SPEED_ENABLED, &nAutoSpeedEnabled);
		pPM->propertyGet (AUTO_SPEED_MODE, &eAutoSpeedMode);

		if (!nAutoSpeedEnabled)
		{
			stConferenceParams.eAutoSpeedSetting = estiAUTO_SPEED_MODE_LEGACY;
		}
		else
		{
			stConferenceParams.eAutoSpeedSetting = eAutoSpeedMode;
		}

		if (stConferenceParams.eAutoSpeedSetting == estiAUTO_SPEED_MODE_AUTO)
		{	//If Auto Speed is Auto, do not read from the property table
			//These member variables default to stiMAX_AUTO_RECV_SPEED_DEFAULT and stiMAX_AUTO_SEND_SPEED_DEFAULT
			//The UI should call MaxAutoSpeedsSet() to set max speeds in Auto
			stConferenceParams.nMaxSendSpeed = m_un32MaxAutoSendSpeed;
			stConferenceParams.nMaxRecvSpeed = m_un32MaxAutoRecvSpeed;
		}
		else
		{	//Otherwise read max send and receive speeds from the property table
			pPM->propertyGet (cmMAX_SEND_SPEED, &stConferenceParams.nMaxSendSpeed);
			pPM->propertyGet (cmMAX_RECV_SPEED, &stConferenceParams.nMaxRecvSpeed);
		}

		pPM->propertyGet (AUTO_SPEED_MIN_START_SPEED, &stConferenceParams.un32AutoSpeedMinStartSpeed);
		if ((int32_t)(stConferenceParams.un32AutoSpeedMinStartSpeed) < AUTO_SPEED_MIN_START_SPEED_MINIMUM ||
			(int32_t)(stConferenceParams.un32AutoSpeedMinStartSpeed) > AUTO_SPEED_MIN_START_SPEED_MAXIMUM)
		{
			stConferenceParams.un32AutoSpeedMinStartSpeed = AUTO_SPEED_MIN_START_SPEED_MINIMUM;
		}
	}

	pPM->propertyGet (cmRINGS_BEFORE_GREETING, &stConferenceParams.nRingsBeforeGreeting);

	{
		int nSignMailEnabled = stiSIGNMAIL_ENABLED_DEFAULT;
		pPM->propertyGet (cmSIGNMAIL_ENABLED, &nSignMailEnabled);
		m_bSignMailEnabled = nSignMailEnabled == 1;
	}

	{
		int nRealTimeTextEnabled = stiREAL_TIME_TEXT_ENABLED_DEFAULT;
		pPM->propertyGet (REAL_TIME_TEXT_ENABLED, &nRealTimeTextEnabled);
		bool bRealTimeTextEnabled = nRealTimeTextEnabled == 1;
		stConferenceParams.stMediaDirection.bOutboundText = bRealTimeTextEnabled;
		stConferenceParams.stMediaDirection.bInboundText = bRealTimeTextEnabled;
	}

	{
		int nDTMFEnabled = stiDTMF_ENABLED_DEFAULT;
		pPM->propertyGet (DTMF_ENABLED, &nDTMFEnabled);
		stConferenceParams.bDTMFEnabled = nDTMFEnabled == 1;
	}

	if (m_eLocalInterfaceMode == estiINTERPRETER_MODE || m_eLocalInterfaceMode == estiVRI_MODE)
	{
		// In interpreter mode, the default is to disallow G.722
		stConferenceParams.AllowedAudioCodecs.push_back ("PCMA");
		stConferenceParams.AllowedAudioCodecs.push_back ("PCMU");
	}

	{
		int nBlockCallerID = 0;
		if (estiSTANDARD_MODE == m_eLocalInterfaceMode
			|| estiHEARING_MODE == m_eLocalInterfaceMode
			|| estiPUBLIC_MODE == m_eLocalInterfaceMode)
		{
			pPM->propertyGet (BLOCK_CALLER_ID_ENABLED, &nBlockCallerID);
			if (nBlockCallerID)
			{
				pPM->propertyGet (BLOCK_CALLER_ID, &nBlockCallerID);
			}
		}
		stConferenceParams.bBlockCallerID = nBlockCallerID;
	}

	{
		int nBlockAnonymousCallers = 0;
		if (estiSTANDARD_MODE == m_eLocalInterfaceMode
			|| estiHEARING_MODE == m_eLocalInterfaceMode)
		{
			pPM->propertyGet (BLOCK_ANONYMOUS_CALLERS, &nBlockAnonymousCallers);
		}
		stConferenceParams.bBlockAnonymousCallers = nBlockAnonymousCallers;
	}

	pPM->propertyGet (cmPREFERRED_AUDIO_CODEC, &stConferenceParams.ePreferredAudioCodec);
	pPM->propertyGet (cmPREFERRED_VIDEO_CODEC, &stConferenceParams.ePreferredVideoCodec);
	pPM->propertyGet (ALLOWED_VIDEO_ENCODERS, &stConferenceParams.eAllowedVideoEncoders);

	stiASSERTMSG(stConferenceParams.eAllowedVideoEncoders == estiVIDEOCODEC_ALL, "Allowed Encoders is being set to %d video encoding may not work", stConferenceParams.eAllowedVideoEncoders);

	// Get New Call Enabled setting
	pPM->propertyGet(NEW_CALL_ENABLED, &m_bNewCallEnabled);

	//
	// Get and set the SIP NAT Traversal setting
	//
	if (m_eLocalInterfaceMode != estiPORTED_MODE)
	{
		pPM->propertyGet (NAT_TRAVERSAL_SIP,
						 (int*)&stConferenceParams.eNatTraversalSIP);
	}
	else
	{
		stConferenceParams.eNatTraversalSIP = estiSIPNATDisabled;
	}

	//
	// Set the SIP Transport settings.
	//
	stConferenceParams.eSIPDefaultTransport = stiSIP_TRANSPORT_DEFAULT;
	
	// Due to concerns over encryption impacting performace for customers we are temporarly setting it to not be on by default.
	stConferenceParams.eSecureCallMode = stiSECURE_CALL_MODE_DEFAULT;
	
	// We still want it set to preferred for call centers.
	if (estiINTERPRETER_MODE == m_eLocalInterfaceMode
		|| estiVRI_MODE == m_eLocalInterfaceMode
		|| estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode)
	{
		stConferenceParams.eSecureCallMode = estiSECURE_CALL_PREFERRED;
	}

	// If we have a property set we want to honor that.
	pPM->propertyGet (SECURE_CALL_MODE,
						(int *)&stConferenceParams.eSecureCallMode);

	pPM->propertyGet (SIP_NAT_TRANSPORT,
					(int *)&stConferenceParams.nAllowedProxyTransports);

	//
	// Set the public interface to the proxy.
	//

	// Use the overrides during initialization. If the override properties are not set then
	// the values will get replaced when core responds with the registration information.

	// Note: We do not want the properties SIP_PUBLIC_DOMAIN_OVERRIDE or
	// SIP_PUBLIC_DOMAIN_ALT_OVERRIDE to have default values other than empty strings.
	// If these have non-empty default values, the registration information from
	// core will be ignored. In the case of the primary domain, the registration
	// information from core is ignored indefinitely. For the alternate domain, the
	// registration information from core is ignored on the first boot after core
	// changes the registration information. 

	m_poPM->propertyGet (SIP_PUBLIC_DOMAIN_OVERRIDE, &publicDomainOverride, PropertyManager::Persistent);
	stConferenceParams.SIPRegistrationInfo.PublicDomain = publicDomainOverride;

	m_poPM->propertyGet (SIP_PUBLIC_DOMAIN_ALT_OVERRIDE, &publicDomainAltOverride, PropertyManager::Persistent);
	stConferenceParams.SIPRegistrationInfo.PublicDomainAlt = publicDomainAltOverride;

	//
	// Set the agent interface to the proxy.
	//
	if (estiINTERPRETER_MODE == m_eLocalInterfaceMode
		|| estiVRI_MODE == m_eLocalInterfaceMode
		|| estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode)
	{
		m_poPM->propertyGet (SIP_AGENT_DOMAIN_OVERRIDE, &agentDomainOverride, PropertyManager::Persistent);

		if (!agentDomainOverride.empty ())
		{
			stiDEBUG_TOOL (g_stiCCIDebug,
					stiTrace ("Overriding SIP Agent domain with: %s\n", agentDomainOverride.c_str ());
			);

			stConferenceParams.SIPRegistrationInfo.AgentDomain = agentDomainOverride;
		}

		m_poPM->propertyGet (SIP_AGENT_DOMAIN_ALT_OVERRIDE, &agentDomainAltOverride, PropertyManager::Persistent);

		if (!agentDomainAltOverride.empty ())
		{
			stiDEBUG_TOOL (g_stiCCIDebug,
					stiTrace ("Overriding SIP Agent domain alternate with: %s\n", agentDomainAltOverride.c_str ());
			);

			stConferenceParams.SIPRegistrationInfo.AgentDomainAlt = agentDomainAltOverride;
		}
	}
	else
	{
		stConferenceParams.SIPRegistrationInfo.AgentDomain.clear ();
		stConferenceParams.SIPRegistrationInfo.AgentDomainAlt.clear ();
	}

	{
		//
		// Retrieve and set the number of seconds to restart the proxy lookup.
		//
		int nRestartProxyMaxLookupTime = CLIENT_REREGISTER_MAX_LOOKUP_TIME_DEFAULT;
		int nRestartProxyMinLookupTime = CLIENT_REREGISTER_MIN_LOOKUP_TIME_DEFAULT;
		m_poPM->propertyGet (CLIENT_REREGISTER_MAX_LOOKUP_TIME, &nRestartProxyMaxLookupTime);
		m_poPM->propertyGet (CLIENT_REREGISTER_MIN_LOOKUP_TIME, &nRestartProxyMinLookupTime);

		stConferenceParams.SIPRegistrationInfo.nRestartProxyMaxLookupTime = nRestartProxyMaxLookupTime;
		stConferenceParams.SIPRegistrationInfo.nRestartProxyMinLookupTime = nRestartProxyMinLookupTime;
	}

	//
	// Indicate which TURN server to use.
	//
	{
		std::string turnServer;
		std::string turnServerAlt;
		std::string turnServerUsername;
		std::string turnServerPassword;

		m_poPM->propertyGet (TURN_SERVER, &turnServer);
		m_poPM->propertyGet (TURN_SERVER_ALT, &turnServerAlt);
		m_poPM->propertyGet (TURN_SERVER_USERNAME, &turnServerUsername);
		m_poPM->propertyGet (TURN_SERVER_PASSWORD, &turnServerPassword);

		stConferenceParams.TurnServer.AddressSet (turnServer.c_str ());
		stConferenceParams.TurnServer.UsernameSet (turnServerUsername.c_str ());
		stConferenceParams.TurnServer.PasswordSet (turnServerPassword.c_str ());

		if (!turnServerAlt.empty ())
		{
			stConferenceParams.TurnServerAlt.AddressSet (turnServerAlt.c_str ());
			stConferenceParams.TurnServerAlt.UsernameSet (turnServerUsername.c_str ());
			stConferenceParams.TurnServerAlt.PasswordSet (turnServerPassword.c_str ());
		}
	}

	//
	// Get the public IP information.
	//
	m_poPM->propertyGet (cmPUBLIC_IP_ASSIGN_METHOD, &stConferenceParams.eIPAssignMethod);

	hResult = PublicIPAddressGet (&stConferenceParams.PublicIPv4, estiTYPE_IPV4);
	if (stiRESULT_CODE (hResult) == stiRESULT_PM_PROPERTY_NOT_FOUND)
	{
		// For initialization, if the property wasn't found, don't return error, just set the PublicIPv4 to an empty string.
		// When the public IP address is returned (AUTO) or set (Manual), the system needs to again call
		// m_pConferenceManager->ConferenceParamsSet with the updated value.
		hResult = stiRESULT_SUCCESS;
		stConferenceParams.PublicIPv4.clear ();
	}

#ifdef IPV6_ENABLED
	hResult = PublicIPAddressGet (&stConferenceParams.PublicIPv6, estiTYPE_IPV6);
	if (stiRESULT_CODE (hResult) == stiRESULT_PM_PROPERTY_NOT_FOUND)
	{
		// For initialization, if the property wasn't found, don't return error, just set the PublicIPv6 to an empty string.
		// When the public IP address is returned (AUTO) or set (Manual), the system needs to again call
		// m_pConferenceManager->ConferenceParamsSet with the updated value.
		hResult = stiRESULT_SUCCESS;
		stConferenceParams.PublicIPv6.clear ();
	}
#endif // IPV6_ENABLED

	//
	// Get the auto detected public IP address
	//
	nResult = pPM->propertyGet (cmPUBLIC_IP_ADDRESS_AUTO, &ipAddress, PropertyManager::Persistent);
	if (PM_RESULT_SUCCESS == nResult)
	{
		stConferenceParams.AutoDetectedPublicIp.assign (ipAddress);
		m_serviceOutageClient.publicIpAddressSet (ipAddress);
	} // end if
	else
	{
		stConferenceParams.AutoDetectedPublicIp.clear ();
	}

	//
	// Inform conference manager of current VCO settings.
	//
	stConferenceParams.ePreferredVCOType = m_ePreferredVCOType;

	// If preferred type is off, then set the other values to disabled
	if (estiVCO_NONE == m_ePreferredVCOType)
	{
		stConferenceParams.vcoNumber.clear ();
	}
	else
	{
		stConferenceParams.vcoNumber = m_vcoNumber;
	}

	stConferenceParams.unLostConnectionDelay = unLOST_CONNECTION_DELAY;

	pPM->propertyGet (cmCHECK_FOR_AUTH_NUMBER, &checkForAuthPhoneNumber, WillowPM::PropertyManager::Persistent);
	stConferenceParams.bCheckForAuthorizedNumber = checkForAuthPhoneNumber;

	//
	// Retrieve the endpoint instance.  If one is not present then create one.
	//
	pPM->propertyGet (INSTANCE_GUID, &instanceGUID);

	if (instanceGUID.empty () || !stricmp ("00000000-0000-1000-8000-000000000000", instanceGUID.c_str ()))
	{
		if (productType == ProductType::VP2 || productType == ProductType::VP3 || productType == ProductType::VP4 || 
			productType == ProductType::Mac || productType == ProductType::Android)
		{
			//
			// For the ntouch VP and VP-200, since the clock is not set at this point, we are choosing
			// to use the variant of the uuid described in RFC 5626, section 4.1.
			//
			instanceGUID = stiGUIDGenerateWithoutTime ();
			if (instanceGUID.empty () || !stricmp ("00000000-0000-1000-8000-000000000000", instanceGUID.c_str ()))
			{
				instanceGUID = stiGUIDGenerate ();
			}
		}
		else
		{
			instanceGUID = stiGUIDGenerate ();
		}

		if (!instanceGUID.empty ())
		{
			stConferenceParams.SipInstanceGUID = instanceGUID;

			pPM->propertySet (INSTANCE_GUID, instanceGUID, PropertyManager::Persistent);
		}
	}
	else
	{
		stConferenceParams.SipInstanceGUID = instanceGUID;
	}

	m_poPM->propertyGet (VRS_FAILOVER_TIMEOUT,
				(int*)&stConferenceParams.vrsFailoverTimeout);

	pPM->propertyGet(FIR_FEEDBACK_ENABLED, &nFIRRequestsEnabled);
	stConferenceParams.rtcpFeedbackFirEnabled = (nFIRRequestsEnabled == 1);

	pPM->propertyGet(PLI_FEEDBACK_ENABLED, &pliFeedbackEnabled);
	stConferenceParams.rtcpFeedbackPliEnabled = (pliFeedbackEnabled == 1);

	pPM->propertyGet(AFB_FEEDBACK_ENABLED, &afbFeedbackEnabled);
	stConferenceParams.rtcpFeedbackAfbEnabled = (afbFeedbackEnabled == 1);

	pPM->propertyGet(TMMBR_FEEDBACK_ENABLED, &tmmbrFeedbackEnabled);
	stConferenceParams.rtcpFeedbackTmmbrEnabled = static_cast<SignalingSupport>(tmmbrFeedbackEnabled);

	pPM->propertyGet(NACK_RTX_FEEDBACK_ENABLED, &nackRtxFeedbackEnabled);
	stConferenceParams.rtcpFeedbackNackRtxSupport = static_cast<SignalingSupport>(nackRtxFeedbackEnabled);

	hResult = m_pConferenceManager->Initialize (
		m_pBlockListManager,
		productType,
		version,
		&stConferenceParams,
		&stConferencePorts);
	stiTESTRESULT ();
	
	pPM->propertyGet (VRS_FOCUSED_ROUTING, &vrsFocusedRouting);
	m_pConferenceManager->vrsFocusedRoutingSet (vrsFocusedRouting);
	dhvApiUrlChanged ();
	dhvApiKeyChanged ();
	dhvEnabledChanged ();
	m_pConferenceManager->AutoRejectSet (false);

	//ConferencePortsSettingsApply ();
	ConferencePortsSettingsChanged ();
	deviceLocationTypeChanged ();
	
STI_BAIL:

	return hResult;
}


std::string uniqueIdGet ()
{
	std::string uniqueID;

#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	//
	// Fetch the stored Unique ID and set it into the OS subsystem.
	// If the Unique ID is not present in the property table then generate one and store it both
	// in the property table and the OS subsystem.
	//
	int result = -1;

#if DEVICE == DEV_X86 // Only allow X86 to set mac address by property table
	result = PropertyManager::getInstance()->propertyGet (MAC_ADDRESS, &uniqueID, WillowPM::PropertyManager::Persistent);
#endif

	if (result != 0 || uniqueID.empty ())
	{
		stiOSGenerateUniqueID (&uniqueID);

		PropertyManager::getInstance ()->propertySet (MAC_ADDRESS, uniqueID, WillowPM::PropertyManager::Persistent);
	}

	stiOSSetUniqueID (uniqueID.c_str ());
#endif

	stiOSGetUniqueID (&uniqueID);

	return uniqueID;
}


///
///\brief Initializes CCI by creating and initializing the objects it uses.
///
stiHResult CstiCCI::Initialize (
	ProductType productType,
	const std::string &version,
	bool verifyAddress,
	PstiAppGenericCallback callback,
	size_t callbackParam)
{
	stiLOG_ENTRY_NAME (CstiCCI::Initialize);

	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::Initialize\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	bool blockListEnabled = stiBLOCK_LIST_ENABLED_DEFAULT;
	PropertyManager *pPM =  PropertyManager::getInstance ();
	IstiImageManager *pImageManager = nullptr;
	std::string serviceOutageServiceUrl = stiSERVICE_OUTAGE_SERVER_DEFAULT;
	auto serviceOutageTimerInterval = vpe::ServiceOutageClient::DefaultTimerInterval;
	std::string uniqueID;

	//
	// Set the application calback if it is not NULL.
	//
	if (callback)
	{
		AppNotifyCallBackSet (callback, callbackParam);
	}

	//
	// Get the IPv6Enabled property and send the value to the conference manager and
	// the host name resolver.
	//
	pPM->propertyGet (IPv6Enabled, &m_bIPv6Enabled);

	hResult = CstiHostNameResolver::getInstance ()->IPv6Enable(m_bIPv6Enabled);
	stiTESTRESULT ();

	m_version = version;

	uniqueID = uniqueIdGet ();

	hResult = initializeHTTP ();
	stiTESTRESULT ();

	hResult = initializeCloudServices (productType, uniqueID);
	stiTESTRESULT ();

	PropertyManager::getInstance ()->CoreServiceSet (m_pCoreService);

	hResult = initializeLoggingService (productType, version, uniqueID);
	stiTESTRESULT ();

	m_poPM->propertyGet (SERVICE_OUTAGE_SERVICE_URL1, &serviceOutageServiceUrl);
	m_poPM->propertyGet (SERVICE_OUTAGE_TIMER_INTERVAL, &serviceOutageTimerInterval);

	m_serviceOutageClient.urlSet (serviceOutageServiceUrl);
	m_serviceOutageClient.timerIntervalSet (serviceOutageTimerInterval);
	m_serviceOutageClient.initialize ();

#ifdef stiIMAGE_MANAGER
	{
		//
		// Create, initialize and start up the ImageManager
		//
		m_pImageManager = new CstiImageManager (m_pCoreService, ThreadCallback, (size_t)this);
		stiTESTCOND (m_pImageManager, stiRESULT_ERROR);

		int nContactImagesEnabled = stiCONTACT_PHOTOS_ENABLED_DEFAULT;
		PropertyManager::getInstance()->propertyGet(CONTACT_IMAGES_ENABLED, &nContactImagesEnabled);

		hResult = m_pImageManager->Initialize ((EstiBool)nContactImagesEnabled);
		stiTESTRESULT();

		// Locally stored avatars ought to be accessible whether or not we ever
		// get a network connection, so start up the image manager task now instead of
		// waiting for the ServicesStartup() function.
		hResult = m_pImageManager->Startup();
		if (stiRESULT_CODE(hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT();
		}
	}
#endif

	//
	// Create and initialize the BlockListManager
	//
	if (m_eLocalInterfaceMode == estiPORTED_MODE
	 || m_eLocalInterfaceMode == estiPUBLIC_MODE
	 || m_eLocalInterfaceMode == estiINTERPRETER_MODE
	 || m_eLocalInterfaceMode == estiVRI_MODE
	 || m_eLocalInterfaceMode == estiTECH_SUPPORT_MODE)
	{
		//
		// The user is not in a mode that supports the block list so do not
		// enable it.
		//
		blockListEnabled = false;
	}
	else
	{
		pPM->propertyGet (BLOCK_LIST_ENABLED, &blockListEnabled);
	}

	m_pBlockListManager = new CstiBlockListManager (blockListEnabled, m_pCoreService,
													ThreadCallback, (size_t)this);
	stiTESTCOND (m_pBlockListManager, stiRESULT_ERROR);

	hResult = m_pBlockListManager->Initialize ();
	stiTESTRESULT ();

	m_pCoreService->offlineCore.blockListGet = [this](CstiCallList *blockList)
	{
		auto hResult = stiRESULT_SUCCESS;
		auto index = 0;
		while (hResult == stiRESULT_SUCCESS)
		{
			std::string id, number, description;
			hResult = m_pBlockListManager->ItemGetByIndex (index++, &id, &number, &description);
			if (hResult == stiRESULT_SUCCESS)
			{
				auto item = std::make_shared<CstiCallListItem> ();
				item->ItemIdSet (id.c_str());
				item->NameSet (description.c_str ());
				item->DialStringSet (number.c_str ());
				blockList->ItemAdd (item);
			}
		}
	};

	//
	// Create and initialize the Ring Group Manager.
	//
	{
		bool ringGroupEnabled = stiRING_GROUP_ENABLED_DEFAULT;
		m_poPM->propertyGet (RING_GROUP_ENABLED, &ringGroupEnabled);
		auto nMode = (int)IstiRingGroupManager::eRING_GROUP_DISABLED;

		if (ringGroupEnabled)
		{
			m_poPM->propertyGet (RING_GROUP_DISPLAY_MODE, &nMode);
		}

		m_pRingGroupManager = new CstiRingGroupManager (
				(IstiRingGroupManager::ERingGroupDisplayMode)nMode,
				m_pCoreService,
				ThreadCallbackRingGroup,
				(size_t)this);
		stiTESTCOND (m_pRingGroupManager, stiRESULT_ERROR);
	}
	
	m_pRingGroupManager->Initialize ();

	//
	// Create and initialize the ContactManager
	//
#ifdef stiIMAGE_MANAGER
	pImageManager = m_pImageManager;
#endif
	m_pContactManager = new CstiContactManager (m_pCoreService, ThreadCallback, (size_t)this, pImageManager);
	stiTESTCOND (m_pContactManager, stiRESULT_ERROR);

	m_pContactManager->Initialize ();

	m_pCoreService->offlineCore.contactListGet = [this](CstiContactList *contactList)
	{
		auto count = m_pContactManager->getNumContacts();
		for (auto i = 0; i < count; i++)
		{
			auto contact = m_pContactManager->getContact (i);
			auto listItem = contact->GetCstiContactListItem ();
			contactList->ItemAdd (listItem);
		}
	};
	m_pCoreService->offlineCore.favoriteListGet = [this](SstiFavoriteList *favoriteList)
	{
		auto count = m_pContactManager->FavoritesCountGet ();
		favoriteList->maxCount = m_pContactManager->FavoritesMaxCountGet ();
		for (auto i = 0; i < count; i++)
		{
			auto favorite = m_pContactManager->FavoriteByIndexGet (i);
			favoriteList->favorites.push_back (*favorite);
		}
	};
	
#ifdef stiLDAP_CONTACT_LIST
	{
		std::string ldapDirectoryDisplayName;
		std::string ldapServer;
		std::string ldapDomainBase;
		std::string ldapTelephoneNumberField;
		std::string ldapHomeNumberField;
		std::string ldapMobileNumberField;
		int nLDAPServerPort = STANDARD_LDAP_PORT;
		int nLDAPServerUseSSL = 0;
		int nLDAPScope = 0;
		auto eLDAPServerRequiresAuthentication = IstiLDAPDirectoryContactManager::eLDAPAuthenticateMethod_NONE;
		bool ldapEnabled = false;

		m_poPM->propertyGet (LDAP_DIRECTORY_DISPLAY_NAME, &ldapDirectoryDisplayName);
		m_poPM->propertyGet (LDAP_SERVER, &ldapServer);
		m_poPM->propertyGet (LDAP_DOMAIN_BASE, &ldapDomainBase);
		m_poPM->propertyGet (LDAP_TELEPHONE_NUMBER_FIELD, &ldapTelephoneNumberField);
		m_poPM->propertyGet (LDAP_HOME_NUMBER_FIELD, &ldapHomeNumberField);
		m_poPM->propertyGet (LDAP_MOBILE_NUMBER_FIELD, &ldapMobileNumberField);
		m_poPM->propertyGet (LDAP_SERVER_PORT, &nLDAPServerPort);
		m_poPM->propertyGet (LDAP_SERVER_USE_SSL, &nLDAPServerUseSSL);
		m_poPM->propertyGet (LDAP_SCOPE, &nLDAPScope);
		m_poPM->propertyGet (LDAP_SERVER_REQUIRES_AUTHENTICATION, &eLDAPServerRequiresAuthentication);
		m_poPM->propertyGet (LDAP_ENABLED, &ldapEnabled);

		m_pLDAPDirectoryContactManager = new CstiLDAPDirectoryContactManager (ldapEnabled, m_pCoreService, ThreadCallback, (size_t)this);
		m_pLDAPDirectoryContactManager->Initialize();

		m_pLDAPDirectoryContactManager->LDAPDirectoryNameSet(ldapDirectoryDisplayName);
		m_pLDAPDirectoryContactManager->LDAPHostSet(ldapServer);
		m_pLDAPDirectoryContactManager->LDAPBaseEntrySet(ldapDomainBase);
		m_pLDAPDirectoryContactManager->LDAPTelephoneNumberFieldSet(ldapTelephoneNumberField);
		m_pLDAPDirectoryContactManager->LDAPHomeNumberFieldSet(ldapHomeNumberField);
		m_pLDAPDirectoryContactManager->LDAPMobileNumberFieldSet(ldapMobileNumberField);
		m_pLDAPDirectoryContactManager->LDAPServerPortSet(nLDAPServerPort);
		m_pLDAPDirectoryContactManager->LDAPServerUseSSLSet(nLDAPServerUseSSL);
		m_pLDAPDirectoryContactManager->LDAPScopeSet(nLDAPScope);
		m_pLDAPDirectoryContactManager->LDAPServerRequiresAuthenticationSet(eLDAPServerRequiresAuthentication);
	}
#endif // stiLDAP_CONTACT_LIST

#ifdef stiCALL_HISTORY_MANAGER
	m_pCallHistoryManager = new CstiCallHistoryManager (true, m_pCoreService, m_pContactManager, ThreadCallback, (size_t)this);
	stiTESTCOND (m_pCallHistoryManager, stiRESULT_ERROR);

	m_pCallHistoryManager->Initialize ();

	m_pCoreService->offlineCore.callListGet = [this](CstiCallList *callList, CstiCallList::EType callListType)
	{
		auto count = m_pCallHistoryManager->ListCountGet (callListType);
		for (auto i = 0U; i < count; i++)
		{
			CstiCallHistoryItemSharedPtr item;
			m_pCallHistoryManager->ItemGetByIndex (callListType, i, &item);
			if (item)
			{
				callList->ItemAdd (item->CoreItemCreate (false));
			}
		}
	};

#endif // stiCALL_HISTORY_MANAGER

	m_pRegistrationManager = new CstiRegistrationManager ();
	stiTESTCOND (m_pRegistrationManager, stiRESULT_ERROR);

	hResult = m_pRegistrationManager->Initialize ();
	stiTESTRESULT ();

	m_userAccountManager = sci::make_unique<UserAccountManager>(m_pCoreService);

	hResult = initializeConferenceManager (productType, version, verifyAddress);
	stiTESTRESULT ();

	{
		SstiConferenceParams stConferenceParams;

		//
		// Get the FilePlay pointer.
		//
		m_pFilePlay = IstiMessageViewer::InstanceGet();

		m_signalConnections.push_back (m_pFilePlay->requestGUIDSignalGet().Connect (
			[this](){
				PostEvent([this]{videoMessageGUIDRequest();});
			}));

		m_signalConnections.push_back (m_pFilePlay->requestUploadGUIDSignalGet().Connect (
			[this](){
				PostEvent([this]{eventMessageUploadGUIDRequest();});
			}));

		m_signalConnections.push_back (m_pFilePlay->disablePlayerControlsSignalGet().Connect (
			[this](){
				PostEvent([this]{videoMessageGUIDRequest();});
			}));

		m_signalConnections.push_back (m_pFilePlay->deleteRecordedMessageSignalGet().Connect (
			[this](SstiRecordedMessageInfo &messageInfo){
				PostEvent([this, messageInfo]{eventRecordedMessageDelete(messageInfo);});
			}));
		
		m_signalConnections.push_back (m_pFilePlay->sendRecordedMessageSignalGet().Connect (
			[this](SstiRecordedMessageInfo &messageInfo){
				PostEvent([this, messageInfo]{eventRecordedMessageSend(messageInfo);});
			}));

		m_signalConnections.push_back (m_pFilePlay->signMailGreetingSkippedSignalGet().Connect (
			[this](){
				PostEvent([this]{eventSignMailGreetingSkipped();});
			}));

		m_signalConnections.push_back (m_pFilePlay->stateSetSignalGet().Connect (
			[this](IstiMessageViewer::EState state){
				PostEvent([this, state]{eventFilePlayerStateSet(state);});
			}));

		m_signalConnections.push_back (m_pFilePlay->requestGreetingUploadGUIDSignalGet().Connect (
			[this](){
				PostEvent([this]{GreetingUpload();});
			}));

		hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		stiTESTRESULT ();

		// Set the upload and download speeds on the fileplayer.
		m_pFilePlay->MaxSpeedSet(stConferenceParams.nMaxRecvSpeed,
								 stConferenceParams.nMaxSendSpeed);
	}

	//
	// Create and initialize the VRCL server
	//
	m_pVRCLServer = new CstiVRCLServer (m_pConferenceManager);
	stiTESTCOND (m_pVRCLServer, stiRESULT_ERROR);

	vrclSignalsConnect ();

	m_pVRCLServer->Initialize (version);
	
	//
	// Set the local names in the engine.
	//
	LocalNamesSet ();

	{
		std::string fromDialString;
		EstiDialMethod method = estiDM_UNKNOWN;

		m_poPM->propertyGet (LOCAL_FROM_DIAL_TYPE, &method);

		m_poPM->propertyGet (LOCAL_FROM_DIAL_STRING, &fromDialString);

		// Need to set the local names according to the interface mode we are using.
		if (!fromDialString.empty () && method != estiDM_UNKNOWN)
		{
			// We are using UserInterfaceGroups, set things according to the UserGroup stuff.
			LocalReturnCallInfoSet (method, fromDialString);
		}
		else
		{
			// We are using UserInterfaceGroups, set things according to the UserGroup stuff.
			LocalReturnCallInfoSet (estiDM_UNKNOWN, "");
		}
	}

#ifdef stiMESSAGE_MANAGER
	// Create the Message Manager and reqeust the message list.
	EstiBool bMessageManagerEnabled;
	bMessageManagerEnabled = estiTRUE;

	if (m_eLocalInterfaceMode == estiPORTED_MODE ||
		m_eLocalInterfaceMode == estiPUBLIC_MODE ||
		m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
		m_eLocalInterfaceMode == estiVRI_MODE ||
		m_eLocalInterfaceMode == estiTECH_SUPPORT_MODE)
	{
		//
		// The user is not in a mode that supports the message manager so enable it.
		//
		bMessageManagerEnabled = estiFALSE;
	}

	m_pMessageManager = new CstiMessageManager (bMessageManagerEnabled, m_pMessageService,
												ThreadCallback, (size_t)this);
	stiTESTCOND (m_pMessageManager, stiRESULT_ERROR);

	// Connect the signals to store the update time in the property table.
	m_signalConnections.push_back (m_pMessageManager->lastSignMailUpdateSignal.Connect (
		[](time_t updateTime){

			// If updateTime is 0 don't store it, the value is probably a result of a new account 
			// or a factory reset and storing it will override the correct value. 
			if (updateTime > 0)
			{
				// Save the new update time and send it to Core.
				PropertyManager::getInstance()->propertySet(LAST_SIGNMAIL_UPDATE_TIME, updateTime, PropertyManager::Persistent);
				PropertyManager::getInstance()->PropertySend(LAST_SIGNMAIL_UPDATE_TIME, estiPTypeUser);
			}
		}));

	m_signalConnections.push_back (m_pMessageManager->lastMessageUpdateSignal.Connect (
		[](time_t updateTime){
			PropertyManager::getInstance()->propertySet(LAST_VIDEO_UPDATE_TIME, updateTime, PropertyManager::Persistent);
			PropertyManager::getInstance()->PropertySend(LAST_VIDEO_UPDATE_TIME, estiPTypeUser);
		}));


	// Initialize the message list.
	m_pMessageManager->Initialize ();

	signMailUpdateTimeChanged();
	videoUpdateTimeChanged();
#endif

	//
	// Make sure everything is setup according to interface mode.
	//
	LocalInterfaceModeUpdate (m_eLocalInterfaceMode);
	m_pRemoteLoggerService->InterfaceModeSet (m_eLocalInterfaceMode);

	// RINGGROUP: Specify if this firmware release supports Multiring
	m_poPM->propertySet(RING_GROUP_CAPABLE, stiRING_GROUP_CAPABLE_DEFAULT, PropertyManager::Temporary);
	m_poPM->PropertySend(RING_GROUP_CAPABLE, estiPTypePhone);

	// VIDEO CHANNELS: Specify that this firmware release supports Video Channels
	m_poPM->propertySet(VIDEO_CHANNELS_CAPABLE, stiVIDEO_CHANNELS_CAPABLE_DEFAULT, PropertyManager::Temporary);
	m_poPM->PropertySend(VIDEO_CHANNELS_CAPABLE, estiPTypePhone);

#ifndef stiDISABLE_SDK_NETWORK_INTERFACE

	m_signalConnections.push_back (IstiNetwork::InstanceGet ()->networkSettingsChangedSignalGet().Connect (
		[this]() {
			PostEvent (std::bind(&CstiCCI::eventNetworkSettingsChanged, this));
	}));

	m_signalConnections.push_back (IstiNetwork::InstanceGet ()->networkStateChangedSignalGet ().Connect (
		[this](IstiNetwork::EstiState eState) {
			PostEvent (std::bind(&CstiCCI::eventNetworkStateChange, this, eState));
	}));

	// NOTE: Signals are connected. Now start up the network
	IstiNetwork::InstanceGet ()->Startup();
#endif // stiDISABLE_SDK_NETWORK_INTERFACE
	
	m_pRemoteLoggerService->NetworkTypeAndDataSet (
		IstiNetwork::InstanceGet ()->networkTypeGet (),
		IstiNetwork::InstanceGet ()->networkDataGet ());

	m_eHearingStatus = estiHearingCallDisconnected;
	
	m_poPM->propertyGet (UNREGISTERED_SIP_RESTART_TIME, &m_nStackRestartDelta);
	
	VRSFailoverDomainResolve ();

STI_BAIL:

	return hResult;
}


//!
// \brief Destructor for CCI
//
CstiCCI::~CstiCCI ()
{
	Shutdown ();

	if (m_pVRCLServer)
	{
		delete m_pVRCLServer;
		m_pVRCLServer = nullptr;
	}

	if (m_pConferenceManager)
	{
		delete m_pConferenceManager;
		m_pConferenceManager = nullptr;
	}

	if (m_pStateNotifyService)
	{
		if (m_pCoreService)
		{
			m_pCoreService->StateNotifyServiceSet (nullptr);
		}

		delete m_pStateNotifyService;
		m_pStateNotifyService = nullptr;
	}

	if (m_pMessageService)
	{
		if (m_pCoreService)
		{
			m_pCoreService->MessageServiceSet (nullptr);
		}

		delete m_pMessageService;
		m_pMessageService = nullptr;
	}

	if (m_pConferenceService)
	{
		if (m_pCoreService)
		{
			m_pCoreService->ConferenceServiceSet (nullptr);
		}

		delete m_pConferenceService;
		m_pConferenceService = nullptr;
	}

	if (m_pRemoteLoggerService)
	{
		stiTraceRemoteLoggingSet(nullptr, nullptr);
		stiAssertRemoteLoggingSet(nullptr, nullptr);
		stiErrorLogRemoteLoggingSet(nullptr, nullptr);
		stiRemoteLogEventRemoteLoggingSet(nullptr, nullptr);

		delete m_pRemoteLoggerService;
		m_pRemoteLoggerService = nullptr;
	}

	if (m_pCoreService)
	{
		delete m_pCoreService;
		m_pCoreService = nullptr;
	}

	if (m_pHTTPTask)
	{
		delete m_pHTTPTask;
		m_pHTTPTask = nullptr;
	}

	if (m_pHTTPLoggerTask)
	{
		delete m_pHTTPLoggerTask;
		m_pHTTPLoggerTask = nullptr;
	}

#ifdef stiIMAGE_MANAGER
	if (m_pImageManager)
	{
		delete m_pImageManager;
		m_pImageManager = nullptr;
	}
#endif

	if (m_pBlockListManager)
	{
		delete m_pBlockListManager;
		m_pBlockListManager = nullptr;
	}

	if (m_pRingGroupManager)
	{
		delete m_pRingGroupManager;
		m_pRingGroupManager = nullptr;
	}

	if (m_pContactManager)
	{
		delete m_pContactManager;
		m_pContactManager = nullptr;
	}
	
#ifdef stiLDAP_CONTACT_LIST
	if (m_pLDAPDirectoryContactManager) {
		delete m_pLDAPDirectoryContactManager;
		m_pLDAPDirectoryContactManager = nullptr;
	}
#endif
	
#ifdef stiMESSAGE_MANAGER
	if (m_pMessageManager)
	{
		delete m_pMessageManager;
		m_pMessageManager = nullptr;
	}
#endif

#ifdef stiCALL_HISTORY_MANAGER
	if (m_pCallHistoryManager)
	{
		delete m_pCallHistoryManager;
		m_pCallHistoryManager = nullptr;
	}
#endif

	if (m_pRegistrationManager)
	{
		// TODO: this fails to clean up
		//delete m_pRegistrationManager;
		//m_pRegistrationManager = nullptr;
	}
}


static stiHResult directSignMailLog (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiDialMethod dialMethod = estiDM_UNKNOWN;
	std::string dialString;

	hResult = call->DialStringGet (&dialMethod, &dialString);
	stiTESTRESULT ();

	stiRemoteLogEventSend ("EventType=DirectSignMail DialString=\"%s\" Result=%i Duration=%u DialSource=%i",
		dialString.c_str(), call->ResultGet(), call->SignMailDurationGet(), call->DialSourceGet());

STI_BAIL:

	return hResult;
}


/*!\brief Stop the ringing
 *
 */
static void IncomingRingStop ()
{
	IstiAudibleRinger::InstanceGet ()->Stop ();
	IstiPlatform::InstanceGet ()->ringStop ();
}


/*!
 * \brief Connect to signals of other components that we are interested in
 */
void CstiCCI::conferenceManagerSignalsConnect ()
{
	//
	// Conference Manager Signals
	//
	m_signalConnections.push_back (m_pConferenceManager->startupCompletedSignal.Connect (
		[this]{
			PostEvent ([this]{ AppNotify(estiMSG_CONFERENCE_MANAGER_STARTUP_COMPLETE, 0); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->remoteRingCountSignal.Connect (
		[this](int ringCount){
			PostEvent (std::bind(&CstiCCI::EventRemoteRingCountReceived, this, ringCount));
		}));

	m_signalConnections.push_back (m_pConferenceManager->localRingCountSignal.Connect (
		[this](uint32_t ringCount){
			PostEvent (std::bind(&CstiCCI::EventLocalRingCountReceived, this, ringCount));
		}));

	m_signalConnections.push_back (m_pConferenceManager->sipMessageConfAddressChangedSignal.Connect (
		[this](const SstiConferenceAddresses &addresses){
			PostEvent (std::bind(&CstiCCI::EventSipMessageConfAddressChanged, this, addresses));
		}));

	m_signalConnections.push_back (m_pConferenceManager->sipRegistrationConfirmedSignal.Connect (
		[this](std::string proxyIpAddress){
			PostEvent (std::bind(&CstiCCI::EventSipRegistrationConfirmed, this, proxyIpAddress));
		}));

	m_signalConnections.push_back (m_pConferenceManager->publicIpResolvedSignal.Connect (
		[this](std::string ipAddress){
			PostEvent (std::bind(&CstiCCI::EventPublicIpResolved, this, ipAddress));
		}));

	m_signalConnections.push_back (m_pConferenceManager->currentTimeSetSignal.Connect (
		[this](time_t currentTime){
			PostEvent (std::bind(&CstiCCI::EventCurrentTimeSet, this, currentTime));
		}));

	m_signalConnections.push_back (m_pConferenceManager->directoryResolveSignal.Connect (
		[this](CstiCallSP call){
			PostEvent (std::bind(&CstiCCI::EventDirectoryResolve, this, call));
		}));

	m_signalConnections.push_back (m_pConferenceManager->callTransferringSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call] {
				AppNotify (estiMSG_TRANSFERRING, (size_t)call.get());
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->hearingCallSpawnSignal.Connect (
		[this](const SstiSharedContact &contact){
			PostEvent ([this, contact]{
				if (estiINTERPRETER_MODE == m_eLocalInterfaceMode)
				{   // Send contact to TerpNet to spawn a new VRS call
					m_pVRCLServer->HearingCallSpawn (contact);
				}
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->hearingStatusChangedSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ AppNotify (estiMSG_HEARING_STATUS_CHANGED, (size_t)call.get ()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->hearingStatusSentSignal.Connect (
		[this]{
			PostEvent ([this]{
				AppNotify (estiMSG_HEARING_STATUS_SENT, 0);
				m_pVRCLServer->VRCLNotify (estiMSG_HEARING_STATUS_SENT);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->newCallReadySentSignal.Connect (
		[this]{
			PostEvent ([this]{
				AppNotify (estiMSG_NEW_CALL_READY_SENT, 0);
				m_pVRCLServer->VRCLNotify (estiMSG_NEW_CALL_READY_SENT);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->flashLightRingSignal.Connect (
		[this]{
			PostEvent ([this]{
				AppNotify (estiMSG_FLASH_LIGHT_RING, 0);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->conferencingWithMcuSignal.Connect (
		[this](CstiCallSP call){
			PostEvent (std::bind(&CstiCCI::EventConferencingWithMcu, this, call));
		}));

	m_signalConnections.push_back (m_pConferenceManager->heartbeatRequestSignal.Connect (
		[this]{
			PostEvent ([this]{ m_pStateNotifyService->HeartbeatRequestAsync(0); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->rvTcpConnectionStateChangedSignal.Connect (
		[this](RvSipTransportConnectionState state){
			PostEvent ([this, state]{ AppNotify (estiMSG_RV_TCP_CONNECTION_STATE_CHANGED, (size_t)state); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->conferencePortsSetSignal.Connect (
		[this](const SstiConferencePorts &ports){
			PostEvent (std::bind(&CstiCCI::EventConferencePortsSet, this, ports));
		}));

	m_signalConnections.push_back (m_pConferenceManager->removeTextSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				AppNotify (estiMSG_REMOVE_TEXT, (size_t)call.get());
				m_pVRCLServer->VRCLNotify (estiMSG_REMOVE_TEXT);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->missedCallSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ MissedCallHandle (call); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->callBridgeStateChangedSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ m_pVRCLServer->BridgeStateChange (call); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->preAnsweringCallSignal.Connect (
		[this](CstiCallSP /*call*/){
			// If viewing video message, close it so call doesn't comptete for memory
			PostEvent ([this]{m_pFilePlay->Close (); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->callTransferFailedSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ AppNotify (estiMSG_TRANSFER_FAILED, (size_t)call.get()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->callInformationChangedSignal.Connect (
	    [this](CstiCallSP call){
			PostEvent ([this, call]{
#ifdef stiCALL_HISTORY_MANAGER
				vrsCallInfoUpdate (call);
#endif
				AppNotify (estiMSG_CALL_INFORMATION_CHANGED, (size_t)call.get()); 
			});
	    }));

#ifdef stiCALL_HISTORY_MANAGER
	m_signalConnections.push_back (m_pCallHistoryManager->callHistoryAdded.Connect (
		[this](CstiCallHistoryItemSharedPtr callHistoryItem) {
			PostEvent ([this, callHistoryItem] {
				auto call = m_pConferenceManager->callObjectGetByCallIndex (callHistoryItem->callIndexGet ());
				if (call != nullptr)
				{
					vrsCallInfoUpdate (call);
				}
			});
		}));
#endif

	m_signalConnections.push_back (m_pConferenceManager->preHangupCallSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				if (call->AppDataGet())
				{   // Cancel the request associated with this call.
					m_pCoreService->RequestCancelAsync ((uint32_t)call->AppDataGet ());
				}
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->videoPlaybackPrivacyModeChangedSignal.Connect (
		[this](CstiCallSP call, bool muted){
			auto msg = (muted ?
				estiMSG_CB_VIDEO_PLAYBACK_MUTED :
				estiMSG_CB_VIDEO_PLAYBACK_UNMUTED);
			PostEvent ([this, call, msg]{ AppNotify (msg, (size_t)call.get()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->remoteTextReceivedSignal.Connect (
		[this](SstiTextMsg *textMsg){
			PostEvent ([this, textMsg]{ AppNotify (estiMSG_REMOTE_TEXT_RECEIVED, (size_t)textMsg); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->callStateChangedSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ m_pVRCLServer->StateChangeNotify (call); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->dialingSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				AppNotify (estiMSG_DIALING, (size_t)call.get());  // Dialing remote system
				m_pVRCLServer->VRCLNotify (estiMSG_DIALING, call);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->answeringCallSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				IncomingRingStop ();
				AppNotify (estiMSG_ANSWERING_CALL, (size_t)call.get());
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->resolvingNameSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				// Resolving remote system IP through directory services
				AppNotify (estiMSG_RESOLVING_NAME, (size_t)call.get());
				m_pVRCLServer->VRCLNotify (estiMSG_RESOLVING_NAME, call);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->incomingCallSignal.Connect (
		[this](CstiCallSP call){
			PostEvent (std::bind(&CstiCCI::EventIncomingCall, this, call));
		}));
	
	m_signalConnections.push_back (m_pConferenceManager->preIncomingCallSignal.Connect (
		[this](CstiCallSP call){
			AppNotify (estiMSG_PRE_INCOMING_CALL, (size_t)call.get());
		}));

	m_signalConnections.push_back (m_pConferenceManager->ringingSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				AppNotify (estiMSG_RINGING, (size_t)call.get());  // Remote system has been notified and is ringing
			});
		}));
	
	m_signalConnections.push_back (m_pConferenceManager->activeCallsSignal.Connect (
		[this](bool activeCall){
			PostEvent ([this, activeCall]{
				AppNotify (estiMSG_ACTIVE_CALL, (size_t)activeCall);  // Active call state changed
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->establishingConferenceSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ AppNotify (estiMSG_ESTABLISHING_CONFERENCE, (size_t)call.get()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->conferencingSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
#ifdef stiCALL_HISTORY_MANAGER
				AddCallToCallHistory (call);
#endif
				// We may reach the conferencing state on either a regular call or call transfer
				// If it was from a call transfer, log the transfer information to core
				TransferLog (call);
				AppNotify (estiMSG_CONFERENCING, (size_t)call.get());
				m_pVRCLServer->VRCLNotify (estiMSG_CONFERENCING, call);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->conferenceEncryptionStateChangedSignal.Connect (
	    [this](CstiCallSP call){
			PostEvent ([this, call]{
				AppNotify (estiMSG_CONFERENCE_ENCRYPTION_STATE_CHANGED, (size_t)call.get());
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->resumedCallLocalSignal.Connect (
	    [this](CstiCallSP call){
			PostEvent ([this, call]{
				AppNotify (estiMSG_RESUMED_CALL_LOCAL, (size_t)call.get());
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->resumedCallRemoteSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ AppNotify (estiMSG_RESUMED_CALL_REMOTE, (size_t)call.get()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->disconnectingSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				IncomingRingStop ();
				if (call)
				{
					auto connected = m_pVRCLServer->IsConnectedCheck ();
					call->ResultSentToVRCLSet (connected);
				}

				// Reset phone numbers back to this user's registered phone numbers
				m_pConferenceManager->UserPhoneNumbersSet (&m_sUserPhoneNumbers);
				AppNotify(estiMSG_DISCONNECTING, (size_t)call.get());
			});
		}));


	m_signalConnections.push_back (m_pConferenceManager->leaveMessageSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				if (call->ResultGet() == estiRESULT_HANGUP_AND_LEAVE_MESSAGE)
				{
					call->LeaveMessageSet (estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD);
					SignMailStart (call);
				}
				else
				{
					std::string actualVideophoneNumber;
					call->VideoMailServerNumberGet (&actualVideophoneNumber);
					if (actualVideophoneNumber.empty())
					{
						SignMailStart (call);
					}
					else
					{   // We got a number from the server that needs to be lookup up before leaving signmail
						CstiCallSP callRef = call;
						DirectoryResolveSubmit (actualVideophoneNumber, &callRef, nullptr, nullptr, false);
					}
				}
			});
		}));


	m_signalConnections.push_back (m_pConferenceManager->createVrsCallSignal.Connect ([this](CstiCallSP call) {
		PostEvent ([this, call] {
			// The same CstiCall will be used to create a new call to VRS.
			// Reset the call state to idle.
			call->StateSet (esmiCS_CONNECTING);
			VRSCallCreate (call);
		});
	}));
	
	m_signalConnections.push_back (m_pConferenceManager->logCallSignal.Connect (
		[this](CstiCallSP call, CstiSipCallLegSP callLeg) {
			PostEvent ([this, call, callLeg]{
				if (!callLeg->callLoggedGet () &&
					(m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
					call->RemoteInterfaceModeGet() == estiINTERPRETER_MODE ||
					m_eLocalInterfaceMode == estiVRI_MODE ||
					call->RemoteInterfaceModeGet() == estiVRI_MODE ||
					m_bRemoteLoggingCallStatsEnabled))
				{
					callLeg->callLoggedSet (true);
					CallStatusSend (call, callLeg);
				}
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->disconnectedSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				IncomingRingStop ();

				// In case a transfer was initiated or received call the
				// transfer logging method to log the information to core.
				TransferLog (call);

				//Get the remote interface mode
				EstiInterfaceMode remoteInterfaceMode = call->RemoteInterfaceModeGet();

				// Log direct SignMails as an event
				if (call->directSignMailGet ())
				{
					directSignMailLog (call);
				}
				// If no call leg was created send stats to remote logger.
				else if (!call->callLegCreatedGet () &&
						 (m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
						  remoteInterfaceMode == estiINTERPRETER_MODE ||
						  m_eLocalInterfaceMode == estiVRI_MODE ||
						  remoteInterfaceMode == estiVRI_MODE ||
						  m_bRemoteLoggingCallStatsEnabled))
				{
					CallStatusSend (call, nullptr);
				}

#ifdef stiCALL_HISTORY_MANAGER
				AddCallToCallHistory(call);
#endif
				AppNotify (estiMSG_DISCONNECTED, (size_t)call.get());
				m_pVRCLServer->VRCLNotify (estiMSG_DISCONNECTED, call);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->heldCallLocalSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{
				AppNotify (estiMSG_HELD_CALL_LOCAL, (size_t)call.get());
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->heldCallRemoteSignal.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ AppNotify (estiMSG_HELD_CALL_REMOTE, (size_t)call.get()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->criticalError.Connect (
		[this](CstiCallSP call){
			PostEvent ([this, call]{ AppNotify (estiMSG_CONFERENCE_MANAGER_CRITICAL_ERROR, (size_t)call.get()); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->errorReportSignal.Connect (
		[this](const SstiErrorLog *errorLog){
			PostEvent ([this, errorLog]{
				AppNotify (estiMSG_CB_ERROR_REPORT, (size_t)errorLog);
				m_pVRCLServer->VRCLNotify (estiMSG_CB_ERROR_REPORT, (size_t)errorLog);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->tunnelTerminatedSignal.Connect (
		[this]{
			PostEvent ([this]{ AppNotify (estiMSG_TUNNEL_TERMINATED, 0); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->failedToEstablishTunnelSignal.Connect (
		[this]{
			PostEvent ([this]{ AppNotify (estiMSG_FAILED_TO_ESTABLISH_TUNNEL, 0); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->vrsServerDomainResolveSignal.Connect (
		[this]{
			PostEvent ([this]{ VRSFailoverDomainResolve(); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->pleaseWaitSignal.Connect (
		[this]{
			PostEvent ([this]{ AppNotify (estiMSG_PLEASE_WAIT, 0); });
		}));

	m_signalConnections.push_back (m_pConferenceManager->signMailSkipToRecordSignal.Connect (
		[this]{
			PostEvent ([this]{ 
				std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);
				// If we are in terp mode and have a spLeaveMessageCall obect we are viewing the greeting 
				// but we want to just skip to recording so just skip the greeting.  
				if (m_spLeaveMessageCall &&
					(m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
					 m_eLocalInterfaceMode == estiVRI_MODE))
				{
					m_spLeaveMessageCall->LeaveMessageSet (estiSM_LEAVE_MESSAGE_SKIP_GREETING);
					m_pFilePlay->SkipSignMailGreeting();
				}
			});
		}));
	
	m_signalConnections.push_back (m_pConferenceManager->remoteGeolocationChangedSignal.Connect (
		[this](const std::string& geolocation){
			m_pVRCLServer->remoteGeolocationChanged (geolocation);
		}));

	m_signalConnections.push_back (m_pConferenceManager->contactReceivedSignal.Connect (
		[this](const SstiSharedContact &contact)
	{
		PostEvent ([this, contact] {
			contactReceivedSignal.Emit (contact);
		});
	}));

	m_signalConnections.push_back (m_pConferenceManager->dhviCapableSignal.Connect (
		[this](bool dhviCapable){
			PostEvent([this, dhviCapable]{m_pVRCLServer->VRCLNotify(estiMSG_HEARING_VIDEO_CAPABLE, (size_t)dhviCapable);});
		}));

	m_signalConnections.push_back (m_pConferenceManager->dhviConnectingSignal.Connect (
		[this](const std::string &conferenceURI, const std::string &initiator, const std::string &profileId){
			PostEvent([this, conferenceURI, initiator, profileId]
			{
				m_pVRCLServer->HearingVideoConnecting(conferenceURI,
													  initiator,
													  profileId);
			});
		}));

	m_signalConnections.push_back (m_pConferenceManager->dhviConnectedSignal.Connect (
		[this]{
			PostEvent([this]{m_pVRCLServer->VRCLNotify(estiMSG_HEARING_VIDEO_CONNECTED);});
		}));

	m_signalConnections.push_back (m_pConferenceManager->dhviDisconnectedSignal.Connect (
		[this](bool disconnectedByHearing){
			PostEvent([this, disconnectedByHearing]{m_pVRCLServer->VRCLNotify(estiMSG_HEARING_VIDEO_DISCONNECTED, (size_t)disconnectedByHearing);});
		}));

	m_signalConnections.push_back (m_pConferenceManager->dhviConnectionFailedSignal.Connect (
		[this]{
			PostEvent([this]{m_pVRCLServer->VRCLNotify(estiMSG_HEARING_VIDEO_CONNECTION_FAILED);});
		}));
	
	m_signalConnections.push_back (m_pConferenceManager->dhviHearingNumberReceived.Connect (
		[this](CstiCallSP call, const std::string hearingNumber){
			PostEvent([this, call, hearingNumber] {
				if(m_dhvClient.dhvEnabledGet())
				{
					call->dhvHearingNumberSet (hearingNumber);
					m_dhvClient.dhvCapableGet (call, call->dhvHearingNumberGet ());
				}
			});
		}));
	m_signalConnections.push_back (m_pConferenceManager->dhviStartCallReceived.Connect (
		[this](CstiCallSP call, const std::string &toNumber, const std::string &fromNumber, const std::string &displayName, const std::string &mcuAddress){
			PostEvent([this, call, toNumber, fromNumber, displayName, mcuAddress]{
				if(m_dhvClient.dhvEnabledGet())
				{
					m_dhvClient.dhvStartCall(call, toNumber, fromNumber, displayName, mcuAddress);
				}
			});
		}));

}

/*!\brief Sets the app notify call back function
 *
 *  This function can be called any time.
 *
 *  \return none.
 */
void CstiCCI::AppNotifyCallBackSet (
	PstiAppGenericCallback fpCallBackFunction,  ///< Application's message queue ID.
	size_t CallbackParam) 					///< The callback parameter
{
	if (nullptr == fpCallBackFunction)
	{
		stiASSERT (estiFALSE);
	}
	else
	{
		m_fpAppNotifyCB = fpCallBackFunction;
		m_CallbackParam = CallbackParam;
	}

} // end CstiCCI::PropertyNotificationQueueSet


/*!\brief Registers a callback to notify the UI of service outage changes.
 *
 *  \return none.
 */
void CstiCCI::ServiceOutageMessageConnect (std::function <void(std::shared_ptr<vpe::ServiceOutageMessage>)> callback)
{
	m_signalConnections.push_back (m_serviceOutageClient.outageMessageReceived.Connect([this, callback](std::shared_ptr<vpe::ServiceOutageMessage> message)
	{
		// Post this to CCI's event queue
		PostEvent ([message, callback]()
		{
			callback (message);
		});
	}));

	// Fire the timer immediately so we know the initial service outage message state.
	ServiceOutageForceCheck ();
}

/*!
 * \brief Force ServiceOutageClient to check for Outage
 */
void CstiCCI::ServiceOutageForceCheck()
{
	m_serviceOutageClient.forceCheckForOutage();
}

///\brief Sets the VRCL port
///
stiHResult CstiCCI::VRCLPortSet (
	uint16_t un16Port)
{
	return m_pVRCLServer->PortSet (un16Port);
}


/*!\brief Starts CCI
 *
 *
 *  \retval stiHResult
 */
stiHResult CstiCCI::Startup ()
{
	stiLOG_ENTRY_NAME (CstiCCI::Startup);

	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::Startup\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);
#ifndef stiDISABLE_SDK_NETWORK_INTERFACE
	if (IstiNetwork::InstanceGet ()->StateGet () != IstiNetwork::estiIDLE)
	{
		m_bStartEngine = true;
	}
	else
#endif
	// For builds not using the SDK Network object, we want to fall into the else.
	{
		initializeEndpointMonitor ();

		//
		// Start all Service threads
		//
		hResult = ServicesStartup ();

		if (stiRESULT_TASK_ALREADY_STARTED == stiRESULT_CODE(hResult))
		{
			stiDEBUG_TOOL (g_stiCCIDebug,
				stiTrace ("CstiCCI::Startup stiRESULT_TASK_ALREADY_STARTED\n");
			);

			hResult = stiRESULT_SUCCESS;
		}
		else
		{
			stiTESTRESULT ();
		}

		//
		// Start ConferenceManager
		//
		hResult = ConferenceManagerStartup ();

		if (stiRESULT_CONFERENCE_ENGINE_ALREADY_STARTED == hResult)
		{
			hResult = stiRESULT_SUCCESS;
		}
		else
		{
			stiTESTRESULT ();
		}

		m_bStartEngine = false;
	}

STI_BAIL:

	return hResult;
}


/*!\brief Shutdown CCI
 *
 *
 *  \retval stiHResult the most likely cause of failure or stiRESULT_SUCCESS
 */
stiHResult CstiCCI::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiCCI::Shutdown);

	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::Shutdown\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	stiHResult hCurrent;

	PropertyManager::getInstance ()->saveToPersistentStorage ();

	//
	// Begin shutdown of the ConferenceManager
	//
	hCurrent = ConferenceManagerShutdown ();
	if (hCurrent != stiRESULT_SUCCESS)
	{
		hResult = hCurrent;
	}

	//
	// Begin shutdown of all Service threads
	//
	hCurrent = ServicesShutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	//
	// Begin shutdown of the network thread
	//
	hCurrent = NetworkShutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	StopEventLoop ();

	m_fpAppNotifyCB = nullptr;

	return hResult;
}


/*!\brief Starts the conference manager and its threads
 *
 *
 *  \retval stiHResult
 */
stiHResult CstiCCI::ConferenceManagerStartup ()
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::ConferenceManagerStartup\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Start the ConferenceManager thread
	//
	hResult = m_pConferenceManager->Startup ();

	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

/*!\brief Shut down the conference manager and its threads
 *
 *
 *  \retval stiHResult
 */
stiHResult CstiCCI::ConferenceManagerShutdown ()
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::ConferenceManagerShutdown\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pConferenceManager->Shutdown ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


#define EXPIRATION_DATE_MAX_LEN 24
void CstiCCI::SendSSLExpirationDates ()
{
	auto expirationData = m_pHTTPTask->SecurityExpirationDateGet (CertificateType::TRUSTEDROOT_DIGICERTCA);
	if (!expirationData.empty())
	{
		PropertyManager *poPM = PropertyManager::getInstance ();
		poPM->propertySet (cmSSL_CA_EXPIRATION_DATE, expirationData, PropertyManager::Temporary);
		poPM->PropertySend (cmSSL_CA_EXPIRATION_DATE, estiPTypePhone);
	}
	else
	{
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("ERR: CA expiration date unknown!\n");
		);
	}
}


/*!
* \brief Notifies the core server of the currently used ports for listen, control and media.
*
*/
static void SendCurrentPortsToCore ()
{
	PropertyManager::getInstance ()->PropertySend(CurrentSipListenPort, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(CurrentMediaPortBase, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(CurrentMediaPortRange, estiPTypePhone);
}


/*!
* \brief Notifies the core server of the static ports for listen, control and media.
*
*/
void SendPortSettingsToCore ()
{
	PropertyManager::getInstance ()->PropertySend(AltSipListenPortEnabled, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(AltSipListenPort, estiPTypePhone);
}


/*!\brief Starts the Network thread
 *
 *
 *  \return stiHResult
 *  \retval stiRESULT_SUCCESS if started properly
 */
stiHResult CstiCCI::NetworkStartup ()
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::NetworkStartup\n");
	);
	stiHResult hResult;

	hResult = m_pVRCLServer->Startup ();
	stiTESTRESULT ();


STI_BAIL:

	return hResult;
}


/*!\brief Shut down the Network thread
 *
 *  \return stiHResult
 *
 *  \retval stiRESULT_SUCCESS if thread properly shuts down
 */
stiHResult CstiCCI::NetworkShutdown ()
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::NetworkShutdown\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_pVRCLServer->Shutdown ();

	return hResult;
}


/*!\brief Starts the Service threads
 *
 *
 *  \return stiHResult
 *  \retval stiRESULT_SUCCESS if all services started properly
 */
stiHResult CstiCCI::ServicesStartup ()
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::ServicesStartup\n");
	);

	stiHResult hResult = stiRESULT_TASK_ALREADY_STARTED;

	if (!m_bServicesStarted)
	{
		//
		// If we are in ported mode then start some of the services in a paused state.
		//
		bool bPause = false;

		if (estiPORTED_MODE == m_eLocalInterfaceMode)
		{
			bPause = true;
		}

		//
		// Make sure the HTTP tasks are starting before starting the other services
		//
		hResult = m_pHTTPTask->Startup ();

		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		hResult = m_pHTTPLoggerTask->Startup ();
		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		SendSSLExpirationDates ();

		//
		// Start core services.
		//
		hResult = m_pCoreService->Startup (bPause);

		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		PropertyManager::getInstance ()->CoreServiceSet (m_pCoreService);

#if 0
		if (estiPORTED_MODE != m_eLocalInterfaceMode)
		{
			hResult = m_pCoreService->Connect ();
			stiTESTRESULT ();
		}
#endif

		//
		// Always start up state notify
		//
		hResult = m_pStateNotifyService->Startup (false);

		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		//
		// Start up the message service
		//
		hResult = m_pMessageService->Startup (bPause);

		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		//
		// Start up the conference service
		//
		hResult = m_pConferenceService->Startup (bPause);

		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		//
		// Start up the remote logger service
		//
		hResult = m_pRemoteLoggerService->Startup (bPause);

		if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		//
		// Indicate that the services have been started.
		//
		m_bServicesStarted = true;
	}

STI_BAIL:

	return hResult;
}


/*!\brief Shut down all the Service threads that we can
 *
 *  \return stiHResult
 *
 *  \retval stiRESULT_SUCCESS if all threads properly shut down
 */
stiHResult CstiCCI::ServicesShutdown ()
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::ServicesShutdown\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	stiHResult hCurrent;

	hCurrent = m_pConferenceService->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	hCurrent = m_pMessageService->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	hCurrent = m_pRemoteLoggerService->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	hCurrent = m_pStateNotifyService->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}
	
	PropertyManager::getInstance ()->SendProperties ();
	PropertyManager::getInstance ()->PropertySendWait (5000);
	
#ifndef __APPLE__ // this causes PropertySender to stop working if user steps back in wizard
	PropertyManager::getInstance ()->CoreServiceSet (nullptr); // pm must stop using core
#endif
	hCurrent = m_pCoreService->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	m_serviceOutageClient.shutdown ();

	//
	// Now shutdown the HTTP Task
	//
	hCurrent = m_pHTTPTask->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	hCurrent = m_pHTTPLoggerTask->Shutdown ();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

#ifdef stiIMAGE_MANAGER
	hResult = m_pImageManager->Shutdown();
	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}
#endif

	//
	// Wait for everything to shut itsself down
	//
	m_pHTTPTask->WaitForShutdown ();
	m_pHTTPLoggerTask->WaitForShutdown ();
#ifdef stiIMAGE_MANAGER
	m_pImageManager->WaitForShutdown();
#endif
	m_bServicesStarted = false;

	return hResult;
}

//:-----------------------------------------------------------------------------
//: Application Functions -- Calling
//:-----------------------------------------------------------------------------


/*!\brief Formats a server dialing (TA: string)
 *
 *  Formats a TA: string with the provided server property string, default server,
 *  and dial string.
 *  NOTE: This function assumes the buffer the formatted string is placed is long enough.
 *
 * \param pszHostName			The domain name of the VRS server
 * \param pszAltHostName		An alternate domain name of the VRS server
 * \param pszDialString			The dial string that is being dialed
 * \param pszServerDialString	A pointer to a buffer to which to write the results
 *
 *  \retval stiRESULT_SUCCESS if the string was successfully formatted
 *      otherwise an error code is returned.
 */
stiHResult FormatServerDialString(
	const std::string &hostName,
	const std::string &altHostName,
	const std::string &dialString,
	std::string *serverDialString)
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("FormatServerDialString\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::string serverIP;

	// Look up the ip address of the VRS Server
	hResult = stiDNSGetHostByName (hostName.c_str (), 			// The name to be resolved
									altHostName.c_str (), 	// An alternate domain name to be resolved
								   &serverIP);		// The results
	stiTESTRESULT();

	SipUriCreate(dialString, serverIP, serverDialString);

STI_BAIL:

	return hResult;
}


/*!\brief Formats a server dialing (TA: string)
 *
 *  Formats a TA: string with the provided server property string, default server,
 *  and dial string.
 *  NOTE: This function assumes the buffer the formatted string is placed is long enough.
 *
 * \param pszServerPropertyName	The name of the property that contains the server address
 * \param pszAltServerPropertyName	The name of the property that contains the alternate server address
 * \param pszDefaultServer		The name of the default server in case the property is not defined
 * \param pszDefaultAltServer	The name of the default alternate server in case the property is not defined
 * \param pszDialString			The dial string that is being dialed
 * \param pszServerDialString	A pointer to a buffer to which to write the results
 *
 * \retval stiRESULT_SUCCESS if the string was successfully formatted
 *      otherwise an error code is returned.
 */
stiHResult FormatServerDialString(
	const char *pszServerPropertyName,
	const char *pszAltServerPropertyName,
	const std::string &pszDialString,
	std::string *serverDialString)
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("FormatServerDialString\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::string hostName;
	std::string altHostName;

	PropertyManager::getInstance()->propertyGet (pszServerPropertyName, &hostName);
	stiTESTCOND(!hostName.empty (), stiRESULT_PM_FAILURE);

	PropertyManager::getInstance()->propertyGet (pszAltServerPropertyName, &altHostName);
	stiTESTCOND(!altHostName.empty (), stiRESULT_PM_FAILURE);

	hResult = FormatServerDialString (hostName, altHostName, pszDialString, serverDialString);

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::FormatVRSDialString(
	const std::string &dialString,				//!< The dial string that is being dialed
	const OptString &preferredLanguage,
	std::string *serverDialString)				//!< A pointer to a buffer to which to write the results
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	std::string hostName;

	stiDEBUG_TOOL (g_stiDialDebug,
		vpe::trace ("FormatVRSDialString:\n\tDial String: ", dialString, "\n");
	);

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	// If we aren't using a proxy then resolve the name now otherwise we will let our proxy do the work for us.
	if (stConferenceParams.eNatTraversalSIP == estiSIPNATDisabled)
	{
		if (preferredLanguage == RELAY_LANGUAGE_SPANISH)
		{
			hResult = FormatServerDialString(SPANISH_VRS_SERVER,
											 SPANISH_VRS_ALT_SERVER,
											 dialString,
											 serverDialString);
			stiTESTRESULT();
		}
		else
		{
			hResult = FormatServerDialString(cmVRS_SERVER,
											 cmVRS_ALT_SERVER,
											 dialString,
											 serverDialString);
			stiTESTRESULT();
		}
	}
	else
	{
		std::string URI;
		
		if (preferredLanguage == RELAY_LANGUAGE_SPANISH)
		{
			PropertyManager::getInstance()->propertyGet (SPANISH_VRS_SERVER, &hostName);
			stiTESTCOND(!hostName.empty (), stiRESULT_PM_FAILURE);
		}
		else
		{
			PropertyManager::getInstance()->propertyGet (cmVRS_SERVER, &hostName);
			stiTESTCOND(!hostName.empty (), stiRESULT_PM_FAILURE);
		}
		
		SipUriCreate (dialString, hostName, serverDialString);
	}

STI_BAIL:
	
	return hResult;
}


/*!\brief Submits a directory resolve request to core services.
 *
 *  \retval stiRESULT_SUCCESS if the request and call object were successfully created.
 */
stiHResult CstiCCI::DirectoryResolveSubmit (
	const std::string &dialString, //!< The dial string to resolve
	CstiCallSP *ppoCall, //!< The created call object
	OptString fromPhoneNumber, //!< The hearing phone number
	const OptString &fromNameOverride,
	bool enableEncryption,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("CstiCCI::DirectoryResolveSubmit\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;
	CstiCoreRequest *poCoreRequest = nullptr;
	std::string LocalReturnDialString;
	EstiBool bBlockCallerId = estiFALSE;

	//
	// If there is a hearing caller's phone number,
	//  send it to the engine to use as the calling phone number
	//
	if (fromPhoneNumber)
	{
		// If the hearing phone number is NOT "Unknown", then add the hearing number
		// in the conference manager.  Otherwise, keep it blank.
		if (fromPhoneNumber != g_szUNKNOWN)
		{
			SstiUserPhoneNumbers tmpUserPhoneNumbers;
			memcpy (&tmpUserPhoneNumbers, &m_sUserPhoneNumbers, sizeof (m_sUserPhoneNumbers));

			strncpy(
				tmpUserPhoneNumbers.szHearingPhoneNumber,
				fromPhoneNumber.value ().c_str (),
				sizeof (tmpUserPhoneNumbers.szHearingPhoneNumber) - 1);
			tmpUserPhoneNumbers.szHearingPhoneNumber[sizeof (tmpUserPhoneNumbers.szHearingPhoneNumber) - 1] = 0;

			m_pConferenceManager->UserPhoneNumbersSet (&tmpUserPhoneNumbers);
		}
	}
	else if (!m_LocalReturnDialString.empty ())
	{
		//
		// If there is no hearing number but there is a callback number
		// then use that number
		//
		fromPhoneNumber = m_LocalReturnDialString;
	}

	// Store the request ID with the call object and notify
	// the Conference Manager that a lookup is being
	// performed.
	if (*ppoCall)
	{
		call = *ppoCall;
	}
	else
	{
		call = m_pConferenceManager->outgoingCallConstruct (dialString, estiSUBSTATE_RESOLVE_NAME, enableEncryption);
		call->fromNameOverrideSet (fromNameOverride);
	}

	stiTESTCOND(call, stiRESULT_ERROR);

	call->OriginalDialMethodSet(estiDM_BY_DS_PHONE_NUMBER);
	call->DialMethodSet(estiDM_BY_DS_PHONE_NUMBER);
	call->additionalHeadersSet(std::move(additionalHeaders));

	if (m_eLocalInterfaceMode == estiINTERPRETER_MODE && (!fromPhoneNumber || fromPhoneNumber == g_szUNKNOWN))
	{
		call->LocalCallerIdBlockedSet (true);
	}

	poCoreRequest = new CstiCoreRequest ();

	poCoreRequest->RemoveUponCommunicationError() = estiTRUE;
	stiTESTCOND(poCoreRequest != nullptr, stiRESULT_ERROR);

	if (call->LocalCallerIdBlockedGet ()
	 && m_pCoreService->APIMajorVersionGet () >= 8) // Only supported on Core 8.0 Release 2 and newer
	{
		// Don't block anonymous hearing calls (CR-10)
		if (m_eLocalInterfaceMode != estiINTERPRETER_MODE)
		{
			bBlockCallerId = estiTRUE;
		}
	}

	// Set the dial string to be looked up in the
	// request object
	poCoreRequest->publicIPGet ();
	poCoreRequest->directoryResolve (dialString.c_str(), estiDM_BY_DS_PHONE_NUMBER, fromPhoneNumber.value_or(std::string()).c_str (), bBlockCallerId);
	poCoreRequest->retryOfflineSet (true);

	hResult = m_pCoreService->RequestSendEx (poCoreRequest, &m_unDirectoryResolveRequestId);

	stiTESTRESULT ();

	//
	// Set this to NULL so it is not freed in the clean up step.
	//
	poCoreRequest = nullptr;

	call->AppDataSet ((size_t)m_unDirectoryResolveRequestId);

	m_pDirectoryResolveCall = call;

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (poCoreRequest)
		{
			if (m_unDirectoryResolveRequestId != 0)
			{
				m_pCoreService->RequestCancel (m_unDirectoryResolveRequestId);
			}

			delete poCoreRequest;
			poCoreRequest = nullptr;
		}

		m_unDirectoryResolveRequestId = 0;

		if (call)
		{
			call->HangUp ();
			call = nullptr;
		}
	}
	else
	{
		*ppoCall = call;
	}

	return hResult;
}


/*!\brief Ensures that we are not dialing ourselves and then submits
 *  the directory resolve request.
 *
 *  \retval stiRESULT_SUCCESS if the request and call object were successfully created.
 */
stiHResult CstiCCI::DialByDSPhoneNumber(
	const std::string &dialString,			//!< The dial string to dial
	CstiCallSP *ppoCall,	//!< The created call object
	const OptString &fromPhoneNumber,		//!< The hearing phone number
	const OptString &fromNameOverride,	  // Overrides what name is shown to the remote endpoint
	bool enableEncryption,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;

	if (*ppoCall)
	{
		call = *ppoCall;
	}

	// Are we trying to look ourselves up in the directory?
	stiTESTCOND(dialString != m_sUserPhoneNumbers.szSorensonPhoneNumber, stiRESULT_UNABLE_TO_DIAL_SELF);
	stiTESTCOND(dialString != m_sUserPhoneNumbers.szTollFreePhoneNumber, stiRESULT_UNABLE_TO_DIAL_SELF);
	stiTESTCOND(dialString != m_sUserPhoneNumbers.szLocalPhoneNumber, stiRESULT_UNABLE_TO_DIAL_SELF);

	m_lastDialed = dialString;

	hResult = DirectoryResolveSubmit(dialString, &call, fromPhoneNumber, fromNameOverride, enableEncryption, additionalHeaders);
	stiTESTRESULT();

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (call && !*ppoCall) //If we are transferring we will let DialCommon handle the error
		{
			call->HangUp ();
			call = nullptr;
		}
	}
	else
	{
		*ppoCall = call;
	}

	return hResult;
}


/*!\brief Determines the dialing method and ensures (as far as poissible)
 *   that we are not dialing ourselves.
 *
 * Possible "By Dial String" Dial Methods:
 * 1) IP
 * 2) DNS Name
 * 3) SIP URI
 * 4) Phone Number (use CoreServices to look up)
 * 5) Some TBD string to look up in Core.  See Note.
 *
 * NOTE: Originally, CoreServices was planned to be augmented to resolve
 * other possibilities such as Email address, Instant Messenger name, etc.
 * For now, however, we will send all unknown strings to Core to be resolved.
 *
 * LIMITATIONS: There is currently no way to distinguish between a sip user@location
 * and an email address.  This is probably OK for now.
 *
 *  \param dialString The dial string to resolve.
 *  \param ppoCall The call object that is allocated and returned.
 *
 *  \retval stiRESULT_SUCCESS if the request and call object were successfully created.
 */
stiHResult CstiCCI::DialByDialString (
	const std::string &dialString,
	CstiCallSP *ppoCall,
	const OptString &fromPhoneNumber,		//!< The hearing phone number
	const OptString &fromNameOverride,
	const OptString &callListName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;
	CstiRoutingAddress oDialString (dialString);

	if (*ppoCall)
	{
		call = *ppoCall;
	}

	// Is the address an IP address? (Did it map to one?)
	if (oDialString.IsValid ())
	{
		// List all known ip addresses we are known by
		std::string LocalIp;
		std::string PublicIp;
		std::vector<std::string> ipAddresses
		{
			"127.0.0.1",
			"0.0.0.0",
			"::1",
		};

		hResult = PublicIPAddressGet (PublicIPAssignMethodGet (), &PublicIp, estiTYPE_IPV4);
		stiTESTRESULT();

		ipAddresses.push_back(PublicIp);

		hResult = stiGetLocalIp (&LocalIp, estiTYPE_IPV4);
		stiTESTRESULT();

		ipAddresses.push_back(LocalIp);

		// List all known ports we are listening on
		std::vector<int> ports
		{
			nDEFAULT_SIP_LISTEN_PORT,
			m_nCurrentSIPListenPort,
		};

		// Are we dialing our own ip address?
		if (oDialString.SameAs (ipAddresses, ports))
		{
			stiTESTCOND (false, stiRESULT_UNABLE_TO_DIAL_SELF);
		}
		else
		{
			//
			// If there is a hearing caller's phone number,
			//  send it to the engine to use as the calling phone number
			//
			// If the hearing phone number is NOT "Unknown", then add the hearing number
			// in the conference manager.  Otherwise, keep it blank.
			//
			if (fromPhoneNumber && fromPhoneNumber != g_szUNKNOWN)
			{
				SstiUserPhoneNumbers tmpUserPhoneNumbers;
				memcpy (&tmpUserPhoneNumbers, &m_sUserPhoneNumbers, sizeof (m_sUserPhoneNumbers));

				strncpy(
					tmpUserPhoneNumbers.szHearingPhoneNumber,
					fromPhoneNumber.value().c_str (),
					sizeof (tmpUserPhoneNumbers.szHearingPhoneNumber) - 1);
				tmpUserPhoneNumbers.szHearingPhoneNumber[sizeof (tmpUserPhoneNumbers.szHearingPhoneNumber) - 1] = 0;

				m_pConferenceManager->UserPhoneNumbersSet (&tmpUserPhoneNumbers);
			}

			call = m_pConferenceManager->CallDial (
						estiDM_BY_DIAL_STRING,
						oDialString,
						dialString,
						fromNameOverride,
						callListName);
			stiTESTCOND(call, stiRESULT_ERROR);
		}
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "Invalid dial string: %s\n", dialString.c_str());
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (call && !*ppoCall) //If we are transferring we will let DialCommon handle the error
		{
			call->HangUp ();
			call = nullptr;
		}
	}
	else
	{
		*ppoCall = call;
	}

	return hResult;
}

/*!\brief Checks if a string represents a valid dial string
 *
 */
bool CstiCCI::DialStringIsValid(
	const std::string &phoneNumber)
{
	return CstiPhoneNumberValidator::InstanceGet()->DialStringIsValid(phoneNumber.c_str ());
}


/*!\brief Checks if a string represents a valid phone number
 *
 */
bool CstiCCI::PhoneNumberIsValid(
	const std::string &phoneNumber)
{
	return CstiPhoneNumberValidator::InstanceGet ()->PhoneNumberIsValid (phoneNumber.c_str ());
}


/*!\brief Adds a '1' and an area code if required
 *
 * If we decide the dial string is a VRS number, then we do nothing and
 * return stiRESULT_INVALID_PARAMETER.  Otherwise, we see if the number
 * is missing a '1' or an area code and add them.
 */
stiHResult CstiCCI::PhoneNumberReformat(
	const std::string &oldPhoneNumber,
	std::string *newPhoneNumber)
{
	return CstiPhoneNumberValidator::InstanceGet ()->PhoneNumberReformat (oldPhoneNumber, *newPhoneNumber);
}

/*!\brief Starts a call to a remote endpoint.
 *
 *  This function should be called only while in the WAITING state and will
 *  fail if the state is not WAITING.
 *
 * \param eDialMethod 			The dial method
 * \param pszDialString 		Dial string contains the formatted or non-formatted string to be called.
 * \param pszCallListName 		Name of callee from a call list (can be set to NULL)
 * \param pszRelayLanguage		The language to use for this call if it is a VRS call
 * \param ppCallID				A pointer that will point to the call object upon return
 *
 *  \retval Success or failure in a stiHResult
 */
stiHResult CstiCCI::CallDial (
	const EstiDialMethod eDialMethod,
	const std::string &dialString,
	const OptString &callListName,
	const OptString &relayLanguage,
	EstiDialSource eDialSource,
	IstiCall **ppCallID)
{
	return CallDial (eDialMethod, dialString, callListName, nullptr, nullptr, relayLanguage, false, eDialSource, false, ppCallID);
}

stiHResult CstiCCI::CallDial (
	EstiDialMethod dialMethod,
	const std::string& dialString,
	const OptString& callListName,
	const OptString& relayLanguage,
	EstiDialSource dialSource,
	bool enableEncryption,
	IstiCall** ppCall)
{
	return CallDial (dialMethod, dialString, callListName, nullptr, nullptr, relayLanguage, false, dialSource, enableEncryption, ppCall);
}


static void callContactInfoSet (
	CstiCallSP &call,
	const ContactListItem &contact,
	const CstiItemId &phoneNumberId)
{
	auto name = contact.NameGet();

	if (!name.empty ())
	{
		call->RemoteContactNameSet (name.c_str ());
	}
	else
	{
		call->RemoteContactNameSet (contact.CompanyNameGet ().c_str ());
	}

	call->IsInContactsSet (estiInContactsTrue);
	call->ContactIdSet(contact.ItemIdGet());
	call->ContactPhoneNumberIdSet(phoneNumberId);
}


/*!\brief Starts a call to a remote endpoint.
 *
 *  This function should be called only while in the WAITING state and will
 *  fail if the state is not WAITING.
 *
 * \param eDialMethod 			The dial method
 * \param dialString 		Dial string contains the formatted or non-formatted string to be called.
 * \param pszCallListName 		Name of callee from a call list (can be set to NULL)
 * \param fromPhoneNumber	Phone number calling from if different (can be set to NULL)
 * \param fromNameOverride      Overrides the name that is shown to the remote endpoint (caller id)
 * \param pszRelayLanguage		The language to use for this call if it is a VRS call
 * \param ppCall				A pointer that will point to the call object upon return
 * \param additionalHeaders		Optional. Extra SIP headers to be added to INVITEs associated with this call
 *
 * \return Success or failure in a stiHResult
 */
stiHResult CstiCCI::CallDial (
	const EstiDialMethod eDialMethod,
	std::string dialString,
	const OptString &callListName,
	const OptString &fromPhoneNumber,
	const OptString &fromNameOverride,
	const OptString &relayLanguage,
	bool bAlwaysAllow,
	EstiDialSource eDialSource,
	bool enableEncryption,
	IstiCall **ppCall,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	stiLOG_ENTRY_NAME (CstiCCI::CallDial);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiCallSP call = nullptr;

	LeftTrim (&dialString);

	hResult = DialCommon (eDialMethod, dialString, callListName,
			fromPhoneNumber, fromNameOverride, relayLanguage, bAlwaysAllow, enableEncryption, &call, additionalHeaders);
	stiTESTRESULT();

	if (call)
	{
		call->DialSourceSet (eDialSource);

		// Check if the phone number is in the contact list
		if (estiPORTED_MODE != m_eLocalInterfaceMode &&
			estiINTERPRETER_MODE != m_eLocalInterfaceMode &&
			estiVRI_MODE != m_eLocalInterfaceMode &&
			estiPUBLIC_MODE != m_eLocalInterfaceMode &&
			estiTECH_SUPPORT_MODE != m_eLocalInterfaceMode)
		{
			auto contact = findContact(dialString);

			if (contact)
			{
				CstiContactListItem::EPhoneNumberType eType = CstiContactListItem::ePHONE_MAX;
				contact->PhoneNumberTypeGet(dialString, &eType);

				callContactInfoSet (call, contact, contact->PhoneNumberIdGet(eType));
			}
			else
			{
				call->IsInContactsSet (estiInContactsFalse);
			}
		}
		else
		{
			call->IsInContactsSet(estiInContactsFalse);
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (call)
		{
			call->HangUp ();
			call = nullptr;
		}
	}
	else
	{
		if (nullptr == ppCall)
		{
			call = nullptr;
		}
		else
		{
			*ppCall  = call.get();
		}
	}

	return hResult;
} // end CstiCCI::CallDial


/*!\brief Starts a call to a remote endpoint.
 *
 *  This function should be called only while in the WAITING state and will
 *  fail if the state is not WAITING.
 *
 *  \retval An opaque call handle or NULL on error
 */
stiHResult CstiCCI::CallDial (
	const CstiItemId &ContactId,
	const CstiItemId &PhoneNumberId,
	EstiDialSource eDialSource,
	bool enableEncryption,
	IstiCall **ppCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string lang;
	bool bVCO = false;
	std::string DialString;
	EstiDialMethod eMethod = estiDM_UNKNOWN;
	CstiCallSP call = nullptr;

	// First find the contact
	auto contact = findContact(ContactId);

	stiTESTCOND(contact, stiRESULT_NOT_A_PHONE_NUMBER);

	// Next make sure we can find the phone number
	contact->DialStringGetById(PhoneNumberId, &DialString);
	stiTESTCOND(!DialString.empty(), stiRESULT_NOT_A_PHONE_NUMBER);

	contact->LanguageGet(&lang);
	contact->VCOGet(&bVCO);
	if (bVCO)
	{
		eMethod = estiDM_UNKNOWN_WITH_VCO;
	}

	// Dial
	hResult = DialCommon (eMethod, DialString, nullptr, nullptr, nullptr, lang, false, enableEncryption, &call);
	stiTESTRESULT();

	if (call)
	{
		call->DialSourceSet (eDialSource);

		if (contact)
		{
			callContactInfoSet (call, contact, PhoneNumberId);
		}
		else
		{
			call->IsInContactsSet (estiInContactsFalse);
		}
	}

STI_BAIL:

	*ppCall = call.get();

	return hResult;
}


/*!\brief Begins the process for sending a direct SignMail
*
* \param dialString 		Dial string contains the formatted or non-formatted string to be called.
* \param dialSource 		Value indicating where in the UI this SignMail originated from
* \param ppCall				A pointer that will point to the call object upon return
*
* \return Success or failure in a stiHResult
*/
stiHResult CstiCCI::SignMailSend (
	std::string dialString,
	EstiDialSource dialSource,
	IstiCall **ppCall)
{
	stiLOG_ENTRY_NAME (CstiCCI::SignMailSend);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;
	std::string newPhoneNumber;
	bool bIsVRS = false;
	int directSignMailEnabled = 0;

	hResult = CstiPhoneNumberValidator::InstanceGet ()->
		PhoneNumberReformat (dialString, newPhoneNumber, &bIsVRS);
	stiTESTRESULT ();
	stiTESTCOND_NOLOG (!bIsVRS, stiRESULT_ERROR);

	stiTESTCOND_NOLOG (!isOffline (), stiRESULT_ERROR);

	//
	// Are we trying to look ourselves up in the directory?
	//
	stiTESTCOND(newPhoneNumber != m_sUserPhoneNumbers.szSorensonPhoneNumber, stiRESULT_UNABLE_TO_DIAL_SELF);
	stiTESTCOND(newPhoneNumber != m_sUserPhoneNumbers.szTollFreePhoneNumber, stiRESULT_UNABLE_TO_DIAL_SELF);
	stiTESTCOND(newPhoneNumber != m_sUserPhoneNumbers.szLocalPhoneNumber, stiRESULT_UNABLE_TO_DIAL_SELF);

	dialString = newPhoneNumber;

	// Check to see if the Direct SignMail feature has been enabled.
	m_poPM->propertyGet(DIRECT_SIGNMAIL_ENABLED,
		&directSignMailEnabled,
		PropertyManager::Persistent);

	stiTESTCOND(directSignMailEnabled == 1, stiRESULT_ERROR);

	//
	// Create a call object without actually placing the call
	//
	call = m_pConferenceManager->outgoingCallConstruct (dialString, estiSUBSTATE_RESOLVE_NAME, false);
	stiTESTCOND (call, stiRESULT_ERROR);

	call->directSignMailSet (true);
	call->DialSourceSet (dialSource);

	hResult = DirectoryResolveSubmit (dialString, &call, nullptr, nullptr, false);

	stiTESTRESULT ();
	*ppCall = call.get();

STI_BAIL:
	if (stiIS_ERROR (hResult))
	{
		if (call)
		{
			call->SignMailInitiatedSet (false);
			call->ResultSet (estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE);
			call->HangUp ();
		}
	}
	return hResult;
}


/*!\brief Begins the process for sending a direct SignMail
*
* \param contactId 				Integer ID of the contact set as a char array
* \param phoneNumberPublicId	GUID of the contact's phone number (PublicId) set as a char array
* \param dialSource 			Value indicating where in the UI this SignMail originated from
* \param ppCall					A pointer that will point to the call object upon return
*
* \return Success or failure in a stiHResult
*/
stiHResult CstiCCI::SignMailSend (
	const CstiItemId &contactId,
	const CstiItemId &phoneNumberPublicId,
	EstiDialSource dialSource,
	IstiCall **ppCall)
{
	stiLOG_ENTRY_NAME (CstiCCI::SignMailSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::string dialString;
	CstiCallSP call = nullptr;
	int directSignMailEnabled = 0;

	auto contact = findContact(contactId);

	stiTESTCOND (contact, stiRESULT_NOT_A_PHONE_NUMBER);
	stiTESTCOND_NOLOG (!isOffline (), stiRESULT_ERROR);

	m_pContactManager->Lock ();

	// Check to see if the Direct SignMail feature has been enabled.
	m_poPM->propertyGet (DIRECT_SIGNMAIL_ENABLED,
		&directSignMailEnabled,
		PropertyManager::Persistent);

	stiTESTCOND (directSignMailEnabled == 1, stiRESULT_ERROR);

	// Next make sure we can find the phone number
	contact->DialStringGetById (phoneNumberPublicId, &dialString);
	stiTESTCOND (!dialString.empty (), stiRESULT_NOT_A_PHONE_NUMBER);

	//
	// Are we trying to look ourselves up in the directory?
	//
	stiTESTCOND (strcmp (dialString.c_str (), m_sUserPhoneNumbers.szSorensonPhoneNumber) != 0, stiRESULT_UNABLE_TO_DIAL_SELF);
	stiTESTCOND (strcmp (dialString.c_str (), m_sUserPhoneNumbers.szTollFreePhoneNumber) != 0, stiRESULT_UNABLE_TO_DIAL_SELF);
	stiTESTCOND (strcmp (dialString.c_str (), m_sUserPhoneNumbers.szLocalPhoneNumber) != 0, stiRESULT_UNABLE_TO_DIAL_SELF);

	//
	// Create a call object without actually placing the call
	//
	call = m_pConferenceManager->outgoingCallConstruct (dialString, estiSUBSTATE_RESOLVE_NAME, false);
	stiTESTCOND (call, stiRESULT_ERROR);

	call->directSignMailSet (true);
	call->DialSourceSet (dialSource);

	hResult = DirectoryResolveSubmit (dialString, &call, nullptr, nullptr, false);
	stiTESTRESULT ();

	*ppCall = call.get();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (call)
		{
			call->SignMailInitiatedSet (false);
			call->ResultSet (estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE);
			call->HangUp ();
		}
	}
	return hResult;
}


/*!\brief Checks to see if the dial string matches one of the SVRS hold server domains
 *
 */
static bool dialStringIsSVRSDomain (
	const std::string &dialString)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	bool result = false;

	if (dialString == g_callSVRSDomain)
	{
		result = true;
	}
	else
	{
		static const std::vector<std::string> svrsHoldServerDomains = {
			cmVRS_SERVER,
			cmVRS_ALT_SERVER,
			SPANISH_VRS_SERVER,
			SPANISH_VRS_ALT_SERVER};

		for (auto &domain: svrsHoldServerDomains)
		{
			std::string hostName;

			PropertyManager::getInstance()->propertyGet (domain, &hostName);
			stiTESTCOND(!hostName.empty (), stiRESULT_PM_FAILURE);

			if (dialString == hostName)
			{
				result = true;
				break;
			}
		}
	}

STI_BAIL:

	return result;
}


/*!\brief Common functionality shared between two different dial methods
 *
 *  \retval An opaque call handle or NULL on error
 */
stiHResult CstiCCI::DialCommon (
	EstiDialMethod eDialMethod,		///< The dial method
	const std::string &dialStringOriginal, /*!< Dial string contains the formatted or non formatted string to be called. */
	OptString callListName,    /*!< Name of callee from a call list (can be set to NULL) */
	const OptString &fromPhoneNumber, /*!< Phone number calling from if different (can be set to NULL) */
	const OptString &fromNameOverride, // Override the name that gets shown to the remote endpoint
	OptString relayLanguage,
	bool bAlwaysAllow,
	bool enableEncryption,
	CstiCallSP *ppCall,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	std::lock_guard<std::recursive_mutex> lk(m_CallDialMutex);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::string dialString = dialStringOriginal;
	std::string fullDialString;
	std::string newPhoneNumber;
	CstiCallSP call = *ppCall;
	std::string RingGroupMemberNumber;
	IstiRingGroupManager::ERingGroupMemberState eState;
	bool spawningCall = false;

	stiDEBUG_TOOL (g_stiDialDebug,
		vpe::trace ("CstiCCI::DialCommon:\n"
				  "\tDial Method: ", eDialMethod, "\n"
				  "\tDial String: ", dialString, "\n"
				  "\tCall List Name: ", callListName, "\n"
				  "\tFrom Phone Number: ", fromPhoneNumber, "\n"
				  "\tFrom Name Override: ", fromNameOverride, "\n"
				  "\tRelay Language: ", relayLanguage, "\n");
	);

	if (NewRelayCallAllowed ())
	{
		auto connectedCall = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTED);
		if (connectedCall)
		{
			call = connectedCall;
			call->SpawnCallNumberSet (dialString);
			spawningCall = true;
		}
	}

	//
	// Only allow if outgoing calls aren't disallowed
	// except when the call is to CS, TS, or an emergency call
	//
	stiTESTCOND_NOLOG(!m_disallowOutgoingCalls
		|| dialString == "911"
		|| CstiPhoneNumberValidator::InstanceGet ()->PhoneNumberIsSupport (dialString), stiRESULT_ERROR);
	
	//
	// Only allow a NULL or empty dial string when the
	// dial type is explicitly VRS related
	//
	stiTESTCOND_NOLOG (eDialMethod == estiDM_BY_VRS_PHONE_NUMBER
	 || eDialMethod == estiDM_BY_VRS_WITH_VCO
	 || !dialString.empty(), stiRESULT_ERROR);

	//
	// Do not allow VRS calls by domain name
	// when in hearing interface mode or when in "must call CIR" mode.
	//
	stiTESTCOND_NOLOG ((m_eLocalInterfaceMode != estiHEARING_MODE && !m_mustCallCIR) || !dialStringIsSVRSDomain (dialString),
			stiRESULT_SVRS_CALLS_NOT_ALLOWED);

	//
	// 611 and 18013868500 calls are routed to CIR
	// Set dial method to prevent DirectoryResolve
	//
	if (isCustomerServiceNumber (dialString))
	{
		eDialMethod = estiDM_BY_DIAL_STRING;
	}
	
	//
	// If VCO is disabled and the dial type suggests VCO
	// then change the dial type to a non-VCO type.
	//
	if (!m_bUseVCO)
	{
		if (estiDM_BY_VRS_WITH_VCO == eDialMethod)
		{
			eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
		}
		else if (estiDM_UNKNOWN_WITH_VCO == eDialMethod)
		{
			eDialMethod = estiDM_UNKNOWN;
		}
	}

	// Initialize the last number dialed.
	m_lastDialed.clear ();

	stiTESTCOND (m_pConferenceManager->eventLoopIsRunning(), stiRESULT_ERROR); // No longer checking number of calls when attempting to place or transfer a call.
//			&& (0 == m_pConferenceManager->CallObjectsCountGet (esmiCS_CONNECTING |  esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH)), stiRESULT_ERROR);

	stiTESTCOND(!m_bUpdating, stiRESULT_ERROR);

	//
	// If the type is "unknown" we will try to figure out
	// what it should be changed to by analyzing the dial string.
	//
	if (eDialMethod == estiDM_UNKNOWN
	 || eDialMethod == estiDM_UNKNOWN_WITH_VCO)
	{
		stiDEBUG_TOOL (g_stiCCIDebug,
			vpe::trace ("Checking to see if dial string ", dialString, " is a VRS string.\n");
		);

		bool bIsVRS = false;
		hResult = CstiPhoneNumberValidator::InstanceGet()->
						PhoneNumberReformat(dialString, newPhoneNumber, &bIsVRS);
		//
		// If the result indicates that the dial string is not a phone number
		// then simply set the dial method to "dial string".
		//
		if (stiRESULT_CODE (hResult) == stiRESULT_NOT_A_PHONE_NUMBER)
		{
			//
			// Check to see if the dial string is a ring group member description
			//
			if (m_pRingGroupManager->ItemGetByDescription(dialString.c_str(), &RingGroupMemberNumber, &eState))
			{
				eDialMethod = estiDM_BY_DS_PHONE_NUMBER;

				hResult = stiRESULT_SUCCESS;
			}
			else
			{
				if (CstiPhoneNumberValidator::InstanceGet ()->
							DialStringIsValid (dialString.c_str()))
				{
					eDialMethod = estiDM_BY_DIAL_STRING;

					hResult = stiRESULT_SUCCESS;
				}
				else
				{
					stiTHROW (stiRESULT_NOT_A_PHONE_NUMBER);
				}
			}
		}
		else
		{
			//
			// If it is any other type of error then fail.
			//
			stiTESTRESULT ();

			//
			// If the dial string turns out to be a "VRS" dial string
			// then set the dial method appropriately.
			//
			if (bIsVRS)
			{
				stiDEBUG_TOOL (g_stiDialDebug,
					vpe::trace (dialString, " is a VRS dial string.\n");
				);

				if (eDialMethod == estiDM_UNKNOWN)
				{
					eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
				}
				else
				{
					eDialMethod = estiDM_BY_VRS_WITH_VCO;
				}

			}
			else
			{
				if (m_pRingGroupManager->ItemGetByDescription(dialString.c_str(), &RingGroupMemberNumber, &eState))
				{
					eDialMethod = estiDM_BY_DS_PHONE_NUMBER;
				}
			}

			dialString = newPhoneNumber;
			if (call &&
				(call->StateGet() == esmiCS_INIT_TRANSFER ||
				 call->StateGet() == esmiCS_TRANSFERRING))
			{
				call->TransferDialStringSet(newPhoneNumber);
			}
		}

		if (eDialMethod != estiDM_BY_DIAL_STRING)
		{
			//
			// If we are in interpreter mode then this number is only allowed
			// to be a deaf number.  If it turns out to be a hearing number
			// then the call will terminate in a number not found.
			//
			if (m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
				m_eLocalInterfaceMode == estiVRI_MODE)
			{
				eDialMethod = estiDM_BY_DS_PHONE_NUMBER;
			}
		}
	}
	
	//
	// For calls to SVRS ESPANOL set the language to Spanish so that the call is routed to a Spanish Interpreter.
	//
	if (dialString == stiSVRS_ESPANOL_TOLL_FREE_NUMBER
	 || dialString == stiSVRS_ESPANOL_LOCAL_NUMBER)
	{
		relayLanguage = RELAY_LANGUAGE_SPANISH;
	}

	m_bDeterminingDialMethod = false;
	m_bPossibleVCOCall = false;
	m_Extension.clear ();

	switch (eDialMethod)
	{
		case estiDM_UNKNOWN:
		case estiDM_UNKNOWN_WITH_VCO:

			m_bDeterminingDialMethod = true;

			if (eDialMethod == estiDM_UNKNOWN_WITH_VCO)
			{
				m_bPossibleVCOCall = true;
			}

			//
			// Take only the first 11 digits and save the extension
			// If the directory resolve comes back with routing information
			// then we will error out because extensions are not allowed with
			// point-to-point calls.
			//
			if (dialString.length() > PHONE_NUMBER_LENGTH)
			{
				std::string dialStringNoExt = dialString.substr (0, PHONE_NUMBER_LENGTH);
				m_Extension = &dialString[PHONE_NUMBER_LENGTH];

				hResult = DialByDSPhoneNumber (dialStringNoExt, &call, fromPhoneNumber, fromNameOverride, enableEncryption);
				stiTESTRESULT();
			}
			else
			{
				hResult = DialByDSPhoneNumber (dialString, &call, fromPhoneNumber, fromNameOverride, enableEncryption);
				stiTESTRESULT();
			}

			if (call->StateGet () != esmiCS_INIT_TRANSFER
				&& call->StateGet () != esmiCS_TRANSFERRING
				&& !NewRelayCallAllowed ())
			{
				call->OriginalDialStringSet (eDialMethod, dialString);
				call->RemoteCallListNameSet (callListName.value_or(""));
				RelayLanguageSet (call.get (), relayLanguage);
			}
			
			// If we are set to use VCO for calls set LocalIsVCOActive to true in case we are calling an IVVR bug #21133
			if (eDialMethod == estiDM_UNKNOWN_WITH_VCO)
			{
				call->LocalIsVcoActiveSet(estiTRUE);
			}

			break;

		case estiDM_BY_VRS_PHONE_NUMBER:
		{
			//
			// VRS calls are not allowed when in interpreter mode.
			//
			stiTESTCOND (m_eLocalInterfaceMode != estiINTERPRETER_MODE, stiRESULT_ERROR);
			stiTESTCOND (m_eLocalInterfaceMode != estiVRI_MODE, stiRESULT_ERROR);

			//
			// VRS calls are not allowed when in hearing mode.
			//
			stiTESTCOND (m_eLocalInterfaceMode != estiHEARING_MODE, stiRESULT_SVRS_CALLS_NOT_ALLOWED);
			stiTESTCOND (!m_mustCallCIR, stiRESULT_SVRS_CALLS_NOT_ALLOWED);

			//
			// Public mode is not allowed to be transferred to VRS numbers.
			//
			if (call)
			{
				bool bIsTransferrer = call->StateGet () == esmiCS_INIT_TRANSFER;
				bool bIsTransferree = call->TransferredGet () == estiTRUE;
				stiTESTCONDMSG (!(estiPUBLIC_MODE == m_eLocalInterfaceMode && bIsTransferree), stiRESULT_ERROR, "This public mode endpoint cannot be transferred to VRS number: %s", dialString.c_str());
				stiTESTCONDMSG (!(estiPUBLIC_MODE == call->RemoteInterfaceModeGet () && bIsTransferrer), stiRESULT_ERROR, "Cannot transfer a public mode endpoint to a VRS number: %s", dialString.c_str());
			}

			// Format the dial string to use Sorenson's VRS server.
			hResult = FormatVRSDialString (dialString, relayLanguage, &fullDialString);
			stiTESTRESULT ();

			if (call &&
				(call->StateGet () == esmiCS_INIT_TRANSFER ||
				 call->StateGet () == esmiCS_TRANSFERRING))
			{
				CstiRoutingAddress RoutingAddress (fullDialString);
				call->RoutingAddressSet (RoutingAddress);
				hResult = call->ContinueDial();
			}
			else
			{
				// Check to see if we should spawn a new relay call.
				if (NewRelayCallAllowed ())
				{
					SpawnNewCall (dialString);
				}
				else
				{
					// If we have a call in the connected state we want to place it on hold before making this call.
					ConnectedCallHold ();

					CstiRoutingAddress RoutingAddress (fullDialString);

					hResult = VrsCallCreate (eDialMethod, RoutingAddress, dialString, callListName, relayLanguage, &call);
					stiTESTRESULT ();
				}
				
			}
			
			call->SpawnCallNumberSet("");
			call->VRSFailOverServerSet (m_vrsFailoverIP);

			break;
		}

		case estiDM_BY_VRS_WITH_VCO:
		{
			//
			// VRS calls are not allowed when in interpreter mode.
			//
			stiTESTCOND (m_eLocalInterfaceMode != estiINTERPRETER_MODE, stiRESULT_ERROR);
			stiTESTCOND (m_eLocalInterfaceMode != estiVRI_MODE, stiRESULT_ERROR);

			//
			// VRS calls are not allowed when in hearing mode.
			//
			stiTESTCOND (m_eLocalInterfaceMode != estiHEARING_MODE, stiRESULT_SVRS_CALLS_NOT_ALLOWED);

			hResult = FormatVRSDialString(dialString, relayLanguage, &fullDialString);
			stiTESTRESULT();

			if (call &&
				(call->StateGet () == esmiCS_INIT_TRANSFER ||
				 call->StateGet () == esmiCS_TRANSFERRING))
			{
				CstiRoutingAddress RoutingAddress (fullDialString);
				call->RoutingAddressSet (RoutingAddress);
				hResult = call->ContinueDial();
			}
			else
			{
				// Check to see if we should spawn a new relay call.
				if (NewRelayCallAllowed ())
				{
					SpawnNewCall (dialString);
				}
				else
				{
					// If we have a call in the connected state we want to place it on hold before making this call.
					ConnectedCallHold ();

					CstiRoutingAddress RoutingAddress (fullDialString);

					hResult = VrsCallCreate (eDialMethod, RoutingAddress, dialString, callListName, relayLanguage, &call);
					stiTESTRESULT ();
				}
			}
			
			call->SpawnCallNumberSet("");
			call->VRSFailOverServerSet (m_vrsFailoverIP);
			
			break;
		}

		case estiDM_BY_DS_PHONE_NUMBER:

			if (RingGroupMemberNumber.empty ())
			{
				hResult = DialByDSPhoneNumber (dialString, &call, fromPhoneNumber, fromNameOverride, enableEncryption, additionalHeaders);
				stiTESTRESULT();
			}
			else
			{
				hResult = DialByDSPhoneNumber (RingGroupMemberNumber, &call, fromPhoneNumber, fromNameOverride, enableEncryption, additionalHeaders);
				stiTESTRESULT();
				if (call->StateValidate (esmiCS_INIT_TRANSFER | esmiCS_TRANSFERRING))
				{
					//Ensures TransferDialString is a number and not a description.
					call->TransferDialStringSet (RingGroupMemberNumber);
				}
			}

			if (call->StateGet () != esmiCS_INIT_TRANSFER
				&& call->StateGet () != esmiCS_TRANSFERRING
				&& !NewRelayCallAllowed ())
			{
				call->AlwaysAllowSet((bAlwaysAllow ? estiTRUE : estiFALSE));
				call->OriginalDialStringSet (eDialMethod, dialString);
				call->RemoteCallListNameSet (callListName.value_or (""));
				RelayLanguageSet (call.get (), relayLanguage);
			}

			break;

		case estiDM_BY_DIAL_STRING:
			
			if (isCustomerServiceNumber (dialString))
			{
				std::string customerInformationURI;

				PropertyManager::getInstance()->propertyGet (
						CUSTOMER_INFORMATION_URI,
						&customerInformationURI);

				CstiRoutingAddress RoutingAddress (customerInformationURI);

				if (call && call->StateValidate (esmiCS_INIT_TRANSFER | esmiCS_TRANSFERRING))
				{
					call->RoutingAddressSet (RoutingAddress);
					hResult = call->ContinueDial();
				}
				else
				{
					ConnectedCallHold ();
					call = m_pConferenceManager->CallDial (
								estiDM_BY_DIAL_STRING,
								RoutingAddress,
								dialString,
								fromNameOverride,
								callListName);

					stiTESTCOND(call, stiRESULT_ERROR);
				}
			}
			else
			{
				ConnectedCallHold ();
				hResult = DialByDialString (dialString, &call, fromPhoneNumber, fromNameOverride, callListName);
				stiTESTRESULT();
			}
			
			call->SpawnCallNumberSet("");
			
			// If dial method is set to estiDM_BY_DIAL_STRING
			// we need to have the remote endpoint handle logging its own missed call.
			call->RemoteAddMissedCallSet (true);
			
			break;

		default:

			stiTESTCOND (estiFALSE, stiRESULT_UNSUPPORTED_DIAL_TYPE);

			break;
	} // end switch

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (call)
		{
			if (call->StateGet () == esmiCS_INIT_TRANSFER ||
				call->StateGet () == esmiCS_TRANSFERRING)
			{
				// If there are any issues preventing us from transferring the call
				// we want should return to a CONNECTED state and notify the App if it
				// was initiating the transfer or hang up the call if we were performing
				// the transfer.

				if (call->StateGet () == esmiCS_TRANSFERRING)
				{
					call->StateSet (esmiCS_CONNECTED);
					call->ResultSet(estiRESULT_TRANSFER_FAILED);
					call->HangUp();
				}
				else
				{
					call->StateSet (esmiCS_CONNECTED);
					AppNotify (estiMSG_TRANSFER_FAILED, (size_t)call.get());
				}
			}
			else if (!spawningCall)
			{
				call->HangUp ();
				call = nullptr;
			}
		}
	}

	// Only return the call object created to the application if we are sure we are not going to spawn a new relay call.
	if (!NewRelayCallAllowed())
	{
		*ppCall = call;
	}

	return hResult;
}


/*!\brief Determine the number of call objects there are in the passed-in state(s)
 *
 *  \retval the number of call(s) in the given state(s)
 */
unsigned int CstiCCI::CallObjectsCountGet (uint32_t un32StateMask)
{
	stiLOG_ENTRY_NAME (CstiCCI::CallObjectsCountGet)

	return (m_pConferenceManager->CallObjectsCountGet (un32StateMask));
} // end CstiCCI::CallObjectsCountGet


/*!
 * \brief Causes the call object to no longer be anchored in call storage
 */
stiHResult CstiCCI::CallObjectRemove (
	IstiCall *call)
{
	stiLOG_ENTRY_NAME (CstiCCI::CallObjectRemove)
	stiHResult hResult = stiRESULT_SUCCESS;

	if (call)
	{
		std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

		// Is this call the leave message call?
		if (m_spLeaveMessageCall.get() == call)
		{
			// If we have already requested an ID and not received it then cancel
			// the request before we request it again.  Also cancel the GUID request timer.
			if (m_unVideoMessageGUIDRequestId != 0)
			{
				MessageRequestCancel (m_unVideoMessageGUIDRequestId);
				m_unVideoMessageGUIDRequestId = 0;
				m_uploadGUIDRequestTimer.stop ();
			}

			// If we had a text only greeting then we nned to make sure that the greeting timer is canceled.
			if (m_spLeaveMessageCall->MessageGreetingTypeGet() == eGREETING_TEXT_ONLY)
			{
				m_textOnlyGreetingTimer.stop ();
			}

			// Release this copy of the call object so it gets dereferenced
			m_spLeaveMessageCall = nullptr;
			m_pFilePlay->signMailCallSet(nullptr);

			// If the player is in a playing state then we are probably playing the greeting and we 
			// probably hung up the call from VRCL So shut it down now, so we don't end up in a recording state!
			IstiMessageViewer::EState ePlayState = m_pFilePlay->StateGet();
			if (ePlayState & (IstiMessageViewer::estiOPENING | IstiMessageViewer::estiPLAYING | IstiMessageViewer::estiPAUSED))
			{
				m_pFilePlay->Close();
			}
			// If the player is in a recording state then we did not get it shut down. So shut it
			// down now. We should not be recording without a call object!
			else if (ePlayState & (IstiMessageViewer::estiRECORD_CONFIGURE | IstiMessageViewer::estiWAITING_TO_RECORD |
								IstiMessageViewer::estiRECORDING))
			{
				EstiBool RecordingWithoutCallObject = estiFALSE;
				stiASSERT(RecordingWithoutCallObject);

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
				m_pFilePlay->RecordStop();
				m_pFilePlay->DeleteRecordedMessage();
#endif
			}
		}

		hResult = m_pConferenceManager->CallObjectRemove ((CstiCall*)call);
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}


/*! \brief Gets the number of call objects in the store.
 *
 *  \retval The number of call objects in the store.
 */
unsigned int CstiCCI::CallObjectsCountGet ()
{
	stiLOG_ENTRY_NAME (CstiCCI::CallObjectsCountGet);

	return (m_pConferenceManager->CallObjectsCountGet ());
} // end CstiCCI::CallObjectsCountGet


/*! \brief Removes any call objects that are left in callStorage
 *
 */
void CstiCCI::allCallObjectsRemove ()
{
	m_pConferenceManager->allCallObjectsRemove();
}

//:-----------------------------------------------------------------------------
//: Application Functions -- Configuration
//:-----------------------------------------------------------------------------


/*! \brief Gets the current auto-reject setting (on/off).
*
*  It is recommended that this function not be called until the conference
*  engine has finished initializing.  This function may be called at any time,
*  but there is no guarantee that the setting is initialized or valid until the
*  conference engine has reached the WAITING state.
*
*  \retval estiON If auto-reject is turned on.
*  \retval estiOFF If it is turned off.
*/
EstiSwitch CstiCCI::AutoRejectGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::AutoRejectGet);

	EstiSwitch eReject = estiOFF;

	if (m_pConferenceManager->AutoRejectGet ())
	{
		eReject = estiON;
	}

	return eReject;
} // end CstiCCI::AutoRejectGet


/*!\brief Sets the auto-reject switch (on/off)
 *
 *  \retval estiOK If in an initialized state and the auto-reject set message was
 *	  sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::AutoRejectSet (
	EstiSwitch eReject)  //!< Auto-reject switch - estiON or estiOFF.
{
	stiLOG_ENTRY_NAME (CstiCCI::AutoRejectSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pConferenceManager->AutoRejectSet (eReject == estiON);

	return (hResult);
} // end CstiCCI::AutoRejectSet


/*!
 * \brief Gets the maximum number of calls
 *
 * \return The max number of calls
 */
int CstiCCI::MaxCallsGet () const
{
	return static_cast<int>(m_pConferenceManager->MaxCallsGet());
}

/*!
 * \brief Sets the maximum number of calls
 *
 * The interface mode may override the actual number of max calls.
 * For exmaple, interpreter mode only allows one call.
 *
 * \param unNumMaxCalls The number of max calls
 *
 * \return Success or failure
 */
stiHResult CstiCCI::MaxCallsSet (
	int numMaxCalls)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (numMaxCalls > 0, stiRESULT_ERROR);

	m_numMaxCalls = numMaxCalls;

	MaxCallsUpdate ();

STI_BAIL:

	return hResult;
}


/*!
 * \brief Updates the conference manager with the maximum number of calls.
 *
 */
void CstiCCI::MaxCallsUpdate ()
{
	if (estiINTERPRETER_MODE == m_eLocalInterfaceMode ||
		estiKIOSK_MODE == m_eLocalInterfaceMode ||
		estiPUBLIC_MODE == m_eLocalInterfaceMode ||
		estiPORTED_MODE == m_eLocalInterfaceMode ||
		estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode ||
		estiVRI_MODE == m_eLocalInterfaceMode)
	{
		m_pConferenceManager->MaxCallsSet (1);
	}
	else
	{
		m_pConferenceManager->MaxCallsSet (m_numMaxCalls);
	}
}


///\brief Sets the conference port settings
///
stiHResult CstiCCI::ConferencePortsSettingsSet (
	SstiConferencePortsSettings *pstConferencePorts)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_stConferencePortsSettings = *pstConferencePorts;

	SavePortSettingsProperties (&m_stConferencePortsSettings);
	SendPortSettingsToCore ();

	hResult = ConferencePortsSettingsApply ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


///\brief Apply the conference port settings
///
stiHResult CstiCCI::ConferencePortsSettingsApply ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferencePorts stConferencePorts;

	ConferencePortsCopy (&m_stConferencePortsSettings, &stConferencePorts);

	hResult = ConferencePortsSet (&stConferencePorts);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


///\brief Gets the conference ports settings
///
stiHResult CstiCCI::ConferencePortsSettingsGet (
	SstiConferencePortsSettings *pstConferencePorts) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pstConferencePorts = m_stConferencePortsSettings;

	return hResult;
}


///\brief Get thes status of the conference ports
///
stiHResult CstiCCI::ConferencePortsStatusGet (
	SstiConferencePortsStatus *pstConferencePorts) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pstConferencePorts = m_stConferencePortsStatus;

	return hResult;
}


/*!\brief Sets the firewall ports (base and range) used for data channels, control ports and second listen ports.
 *
 *  Calling this function will cause the conference engine to shutdown its
 *  and restart the stack to reconfigure with the new open ports in
 *  the firewall.
 *
 *  \retval estiOK If able to post the event
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::ConferencePortsSet (
	SstiConferencePorts *pstConferencePorts)
{
	stiLOG_ENTRY_NAME (CstiCCI::ConferencePortsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);

	// update the stack with the change
	hResult = m_pConferenceManager->ConferencePortsSet (pstConferencePorts);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
} // end CstiCCI::ConferencePortsSet


/*!\brief Sets the local interface mode
 *
 *  \retval estiOK On success.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::LocalInterfaceModeSet (
	EstiInterfaceMode eMode)    /*!< Interface mode to change to.  Valid values are:
								 * - estiSTANDARD_MODE - Can make all types of calls.
								 * - estiPUBLIC_MODE - Can make all types of calls,
								 *   not allowed to access settings.
								 * - estiKIOSK_MODE - Restricted to VRS calls,
								 *   not allowed to access settings.
								 * - estiINTERPRETER_MODE - Can make all types
								 *   of calls, changes what is sent as "Display
								 *   Name" and how missed calls are handled.
								 *   Additionally, some UI abilities are limited (e.g.
								 *   Can't access "Settings").
								 * - estiVRI_MODE - Mostly similar to Interpreter mode
								 *   except can make a second outgoing call and join them.
								 * - estiTECH_SUPPORT_MODE - Can make all types
								 *   of calls, changes what is sent as "Display
								 *   Name" and how missed calls are handled.
								 * - estiPORTED_MODE - The videophone has been ported
								 *   away from use of Core Services to an alternate
								 *   VRS provider.
								 */
{
	stiLOG_ENTRY_NAME (CstiCCI::LocalInterfaceModeSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (eMode != m_eLocalInterfaceMode)
	{
		hResult = LocalInterfaceModeUpdate (eMode);
		AppNotify (estiMSG_INTERFACE_MODE_CHANGED, 0);
	}
	else
	{
		stiTHROW_NOLOG (stiRESULT_ALREADY_SET);
	}

STI_BAIL:

	return hResult;
} // end CstiCCI::LocalInterfaceModeSet


/*!\brief Performs all the needed functionality when the interface mode changes.
 *
 *  \retval true Indicates that the mode actually changed.
 *  \retval false Indicates that no mode change took place
 */
stiHResult CstiCCI::LocalInterfaceModeUpdate (
	EstiInterfaceMode eMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	PropertyManager *pPM = PropertyManager::getInstance ();

#ifdef stiDISABLE_PORTED_MODE
	if (eMode == estiPORTED_MODE)
	{
		m_pCoreService->EmptyQueue ();
		// for mobile device, logout but don't disable communication
		CoreLogout (NULL, &m_unPortingLogoutRequestId);
		// Empty the queues of any requests.
		m_pMessageService->EmptyQueue ();
		m_pConferenceService->EmptyQueue ();
		m_pRemoteLoggerService->EmptyQueue ();

		//
		// Clear out the phone numbers the engine knows about.
		//
		SstiUserPhoneNumbers sUserPhoneNumbers;

		memset (&sUserPhoneNumbers, 0, sizeof (sUserPhoneNumbers));
		UserPhoneNumbersSet (&sUserPhoneNumbers);

		eMode = estiSTANDARD_MODE;
	}
#endif

	if (eMode == estiPORTED_MODE)
	{
		//
		// We are switching to ported mode.
		//
		m_pCoreService->EmptyQueue ();

		CoreLogout (nullptr, &m_unPortingLogoutRequestId);

		if (!m_pMessageService->PauseGet ())
		{
			m_pMessageService->PauseSet (true);
		}

		if (!m_pConferenceService->PauseGet ())
		{
			m_pConferenceService->PauseSet (true);
		}

		if (!m_pRemoteLoggerService->PauseGet ())
		{
			m_pRemoteLoggerService->PauseSet (true);
		}

		//
		// Empty the queues of any requests.
		//
		m_pMessageService->EmptyQueue ();
		m_pConferenceService->EmptyQueue ();
		m_pRemoteLoggerService->EmptyQueue ();

		// Now the following boolean to false so that if/when Core Services are restored, we will
		// correctly inquire of Core certain phone settings.
		m_bRequestedPhoneSettings = false;

		//
		// Clear out the phone numbers the engine knows about.
		//
		SstiUserPhoneNumbers sUserPhoneNumbers;

		memset (&sUserPhoneNumbers, 0, sizeof (sUserPhoneNumbers));
		UserPhoneNumbersSet (&sUserPhoneNumbers);

		// Remove the user name
		pPM->removeProperty (DefUserName);
		pPM->removeProperty (LOCAL_FROM_NAME);
		// Reset the local names into the engine.
		LocalNamesSet ();

		// Disable the sip proxy
		{
			SstiConferenceParams stConferenceParams;
			hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
			stiTESTRESULT ();
			stConferenceParams.eNatTraversalSIP = estiSIPNATDisabled;
			hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
			stiTESTRESULT ();
		}
	}
	else if (m_eLocalInterfaceMode == estiPORTED_MODE)
	{
		//
		// We are switching away from ported mode.
		//
		m_pCoreService->PauseSet (false);
		m_pMessageService->PauseSet (false);
		m_pConferenceService->PauseSet (false);
		m_pRemoteLoggerService->PauseSet (false);
		//
		// Should we perform a service contacts get and a public IP get?
		//

		SstiConferenceParams stConferenceParams;
		hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		stiTESTRESULT ();

		pPM->propertyGet (NAT_TRAVERSAL_SIP,
						 (int*)&stConferenceParams.eNatTraversalSIP);
		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();
	}

	// Store the local interface mode being used at this time.
	pPM->propertySet (
		cmLOCAL_INTERFACE_MODE,
		eMode,
		PropertyManager::Persistent);

	m_eLocalInterfaceMode = eMode;
	m_pRemoteLoggerService->InterfaceModeSet (m_eLocalInterfaceMode);

	//
	// Set agent domain override conference parameters if in interpreter or tech support modes
	// Clear parameters otherwise
	//
	{
		std::string agentDomainOverride;
		std::string agentDomainAltOverride;

		SstiConferenceParams stConferenceParams;
		hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		stiTESTRESULT ();

		if (estiINTERPRETER_MODE == m_eLocalInterfaceMode
			|| estiVRI_MODE == m_eLocalInterfaceMode
			|| estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode)
		{
			m_poPM->propertyGet (SIP_AGENT_DOMAIN_OVERRIDE, &agentDomainOverride, PropertyManager::Persistent);

			if (!agentDomainOverride.empty ())
			{
				stiDEBUG_TOOL (g_stiCCIDebug,
						stiTrace ("LocalInterfaceModeUpdate overriding SIP Agent domain with: %s\n", agentDomainOverride.c_str ());
				);

				stConferenceParams.SIPRegistrationInfo.AgentDomain = agentDomainOverride;
			}

			m_poPM->propertyGet (SIP_AGENT_DOMAIN_ALT_OVERRIDE, &agentDomainAltOverride, PropertyManager::Persistent);

			if (!agentDomainAltOverride.empty ())
			{
				stiDEBUG_TOOL (g_stiCCIDebug,
						stiTrace ("LocalInterfaceModeUpdate overriding SIP Agent domain alternate with: %s\n", agentDomainAltOverride.c_str ());
				);

				stConferenceParams.SIPRegistrationInfo.AgentDomainAlt = agentDomainAltOverride;
			}

			m_poPM->propertyDefaultSet (SECURE_CALL_MODE, estiSECURE_CALL_PREFERRED);
		}
		else
		{
			stConferenceParams.SIPRegistrationInfo.AgentDomain.clear ();
			stConferenceParams.SIPRegistrationInfo.AgentDomainAlt.clear ();
			m_poPM->propertyDefaultSet (SECURE_CALL_MODE, stiSECURE_CALL_MODE_DEFAULT);
		}

		hResult = m_pConferenceManager->ConferenceParamsSet(&stConferenceParams);
		stiTESTRESULT ();
	}

	//
	// Enable Remote Logging for Interpreter mode by setting the RemoteLoggerValue
	// to 1 (on).
	//
	if (m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
		m_eLocalInterfaceMode == estiVRI_MODE)
	{
		//Enable assert and start remote logging
		hResult = m_pRemoteLoggerService->RemoteLoggerAssertEnable(1);
		stiTESTRESULT ();

		hResult = m_pRemoteLoggerService->RemoteLoggerEventEnable(1);
		stiTESTRESULT ();

	}
	else
	{
		//Not in Interpreter mode so get the current remote logging value or zero (off)
		int nRemoteLoggingAssertVal;
		nRemoteLoggingAssertVal = stiREMOTE_LOG_ASSERT_DEFAULT;
		m_poPM->propertyGet (REMOTE_ASSERT_LOGGING, &nRemoteLoggingAssertVal);

		hResult = m_pRemoteLoggerService->RemoteLoggerAssertEnable(nRemoteLoggingAssertVal);
		stiTESTRESULT ();

		//Get the current event logging value or zero (off)
		int nRemoteLoggingEventVal = stiREMOTE_LOG_EVENT_SEND_DEFAULT;
		m_poPM->propertyGet (REMOTE_LOG_EVENT_SEND_LOGGING, &nRemoteLoggingEventVal);

		hResult = m_pRemoteLoggerService->RemoteLoggerEventEnable(nRemoteLoggingEventVal);
		stiTESTRESULT ();

		//Get the current trace logging value or zero (off)
		int nRemoteLoggingTraceVal;
		nRemoteLoggingTraceVal = stiREMOTE_LOG_TRACE_DEFAULT;
		m_poPM->propertyGet (REMOTE_TRACE_LOGGING, &nRemoteLoggingTraceVal);

		hResult = m_pRemoteLoggerService->RemoteLoggerTraceEnable(nRemoteLoggingTraceVal);
		stiTESTRESULT ();
	}

	//
	// Enable the block list manager and message manager if interface mode supports it
	// and the property allows for it.
	//
	{
		bool blockListEnabled = stiBLOCK_LIST_ENABLED_DEFAULT;
		EstiBool bMessageManagerEnabled = estiTRUE;

		if (m_eLocalInterfaceMode == estiPORTED_MODE
			|| m_eLocalInterfaceMode == estiPUBLIC_MODE
			|| m_eLocalInterfaceMode == estiINTERPRETER_MODE
			|| m_eLocalInterfaceMode == estiVRI_MODE
			|| m_eLocalInterfaceMode == estiTECH_SUPPORT_MODE)
		{
			blockListEnabled = false;
			bMessageManagerEnabled = estiFALSE;
		}
		else
		{
			m_poPM->propertyGet (BLOCK_LIST_ENABLED, &blockListEnabled);
		}

		m_pBlockListManager->EnabledSet (blockListEnabled);

#ifdef stiMESSAGE_MANAGER
		m_pMessageManager->EnabledSet(bMessageManagerEnabled);
#endif
	}

	if (m_eLocalInterfaceMode == estiINTERPRETER_MODE ||
		m_eLocalInterfaceMode == estiVRI_MODE ||
		m_eLocalInterfaceMode == estiPORTED_MODE ||
		m_eLocalInterfaceMode == estiTECH_SUPPORT_MODE)
	{
		m_serviceOutageClient.enabledSet (false);
	}
	else
	{
		m_serviceOutageClient.enabledSet (true);
	}

	//
	// Set the allowed audio codec list based on interface mode.
	//
	{
		SstiConferenceParams stConferenceParams;
		hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		stiTESTRESULT ();

		stConferenceParams.AllowedAudioCodecs.clear ();

		if (m_eLocalInterfaceMode == estiINTERPRETER_MODE || m_eLocalInterfaceMode == estiVRI_MODE)
		{
			// In interpreter mode, the default is to disallow G.722
			stConferenceParams.AllowedAudioCodecs.push_back ("PCMA");
			stConferenceParams.AllowedAudioCodecs.push_back ("PCMU");
		}

		hResult = m_pConferenceManager->ConferenceParamsSet(&stConferenceParams);
		stiTESTRESULT ();
	}

	//
	// Update the maximum number of calls.
	//
	MaxCallsUpdate ();

	m_pConferenceManager->LocalInterfaceModeSet (eMode);

	//
	// Make sure the return call information is set correctly.
	//
	LocalReturnCallInfoUpdate ();

	//
	// Make sure the display and alternate names are set correctly.
	//
	LocalNamesUpdate ();
	
	//
	// Update the block caller id feature.
	//
	BlockCallerIDChanged ();
	BlockAnonymousCallersChanged ();

	SecureCallModeChanged ();

STI_BAIL:

	return hResult;
}


/*!\brief Sets the default provider agreement status
 *
 *  \return nothing
 */
void CstiCCI::DefaultProviderAgreementSet (bool bAgreed)
{
	m_pConferenceManager->DefaultProviderAgreementSet (bAgreed);
}


stiHResult CstiCCI::LocalNameSet (
	const std::string &localName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	PropertyManager* poPM = PropertyManager::getInstance();

	poPM->propertySet (DefUserName, localName, PropertyManager::Persistent);

	LocalNamesSet ();

	return hResult;
}


stiHResult CstiCCI::LocalNameGet (
	std::string *localName) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	PropertyManager* poPM = PropertyManager::getInstance();
	std::string defaultUserName;

	poPM->propertyGet (DefUserName, &defaultUserName);

	*localName = defaultUserName;

	return hResult;
}


/*!
* \brief Set the local names into the engine
*
*/
stiHResult CstiCCI::LocalNamesSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string fromName;
	std::string localName;
	PropertyManager* poPM = PropertyManager::getInstance();

	poPM->propertyGet (DefUserName, &localName);

	int nRet = poPM->propertyGet (LOCAL_FROM_NAME, &fromName);

	//
	// If the property was found and it was not an empty string
	// then use it as the display name.  Otherwise, use the local
	// name as the display name and no alternate name.
	//
	if (PM_RESULT_SUCCESS == nRet && !fromName.empty ())
	{
		// We have data from a UserInterfaceGroup, set things according to the UserGroup stuff.
		hResult = LocalNamesSet (fromName, localName);
	}
	else
	{
		hResult = LocalNamesSet (localName, "");
	}

	return hResult;
}


/*!\brief Sets the local names of user strings
 *
 *  This function should be called by the application layer.  It is
 *  used to set the Display Name (sent in the DisplayName field of the
 *  Q.931 Setup message and the Alternate Name (sent in the SInfo).
 *  In some cases, these will be the same but in other cases, such as
 *  when in Interpreter mode, the display name will be something like
 *  Sorenson VRS and the Alternate name will end up being set to the
 *  Interpreter's ID.
 *
 *  \retval stiHResult Contains the result code.
 */
stiHResult CstiCCI::LocalNamesSet (
	const std::string &displayName,
	const std::string &alternateName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_DisplayName = displayName;
	m_AlternateName = alternateName;

	//
	// If the display name and the alternate name are the
	// same then make the alternate name an empty string.
	//
	if (m_DisplayName == m_AlternateName)
	{
		m_AlternateName = "";
	}

	LocalNamesUpdate ();

	return hResult;
} // end CstiCCI::LocalNamesSet


///!\brief Sets the local name and display name for the conference manager
///
stiHResult CstiCCI::LocalNamesUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pConferenceManager->LocalDisplayNameSet (m_DisplayName);
	stiTESTRESULT ();

	hResult = LocalAlternateNameSet (m_AlternateName, g_nPRIORITY_UI);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/*!\brief Sets the local alternate name of user string
 *
 *  This function should be called only while in the WAITING state and will
 *  fail if the state is not WAITING.
 *
 *  \retval estiOK If in the waiting state and the local name set message was
 *      sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::LocalAlternateNameSet (
	const std::string &alternateName,  //!< Local (user) name string.
	int nPriority)
{
	stiLOG_ENTRY_NAME (CstiCCI::LocalAlternateNameSet);

	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND(nPriority <= m_nLocalNamePriority, stiRESULT_PRIORITY_TOO_LOW);

	// Update the configuration data object.
	hResult = m_pConferenceManager->localAlternateNameSet (alternateName);

	stiTESTRESULT();

	m_nLocalNamePriority = nPriority;

STI_BAIL:

	return (hResult);
} // end CstiCCI::LocalAlternateNameSet


/*!\brief Sets the return info for callback purposes
 *
 *  This function is used to set the information needed by the remote system
 *  to call back to the desired location (not always the originating system).
 *  This data will be passed in SInfo to the remote system.
 *
 */
stiHResult CstiCCI::LocalReturnCallInfoSet (
	EstiDialMethod eMethod, // The dial method to use for the return call
	std::string dialString)	// The dial string to dial on the return call
{
	stiLOG_ENTRY_NAME (CstiCCI::LocalReturnCallInfoSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_eLocalReturnDialMethod = eMethod;
	m_LocalReturnDialString = dialString;

	//
	// Make sure the return call information is set
	//
	hResult = LocalReturnCallInfoUpdate ();

	stiTESTRESULT();

STI_BAIL:

	return (hResult);
}


/*!\brief Updates the return call information
 *
 *  This function ensures the correct data is passed to the conference
 *  engine for callback purposes.  It takes into account the current
 *  callback information set and the interface mode.
 *
 */
stiHResult CstiCCI::LocalReturnCallInfoUpdate ()
{
	stiLOG_ENTRY_NAME (CstiCCI::LocalReturnCallInfoUpdate);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pConferenceManager->LocalReturnCallInfoSet (m_eLocalReturnDialMethod,
														   m_LocalReturnDialString);

	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}


/*!\brief Sets the user phone numbers
 *
 *  This function should be called only while in the WAITING state and will
 *  fail if the state is not WAITING.
 *
 *  \retval estiOK If in the waiting state and the phone number set
 *      message was sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::UserPhoneNumbersSet (
	const SstiUserPhoneNumbers *psUserPhoneNumbers)       //!< Phone numbers structure
{
	stiLOG_ENTRY_NAME (CstiCCI::UserPhoneNumbersSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	std::stringstream PhoneNumberList;
	std::string EventLog;

	// Update the configuration data object.
	hResult = m_pConferenceManager->UserPhoneNumbersSet (psUserPhoneNumbers);
	stiTESTRESULT ();

	//
	// If the user is going from a myPhone Group user to a non-myPhone Group user
	// *and* they are not explicitly using NAT Traversal
	// then we need to have the engine get the Public IP from core instead of from
	// SMX.
	//
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	if (stConferenceParams.eNatTraversalSIP == estiSIPNATDisabled
	 && (m_sUserPhoneNumbers.szRingGroupLocalPhoneNumber[0] != '\0' || m_sUserPhoneNumbers.szRingGroupTollFreePhoneNumber[0] != '\0')
	 && psUserPhoneNumbers->szRingGroupLocalPhoneNumber[0] == '\0' && psUserPhoneNumbers->szRingGroupTollFreePhoneNumber[0] == '\0')
	{
		//
		// Clear state notify's knowledge of the previosu Public IP
		// and tell it to do a heartbeat to start the process of
		// detecting the public IP.
		//
		m_pStateNotifyService->PublicIPClear ();
		StateNotifyHeartbeatRequest ();
	}

	//
	// Store the phone numbers and retrieve the area code.
	// [IS-25179] The area code must come from one of the local numbers,
	//            so we can't use the preferred number.
	//
	m_sUserPhoneNumbers = *psUserPhoneNumbers;

	if (m_sUserPhoneNumbers.szRingGroupLocalPhoneNumber[0] != '\0')
	{
		CstiPhoneNumberValidator::InstanceGet ()->LocalAreaCodeSet (&m_sUserPhoneNumbers.szRingGroupLocalPhoneNumber[1]);
	}
	else
	{
		CstiPhoneNumberValidator::InstanceGet ()->LocalAreaCodeSet (&m_sUserPhoneNumbers.szLocalPhoneNumber[1]);
	}

	// Set the LoggedInPhoneNumber in the VRCL
	m_pVRCLServer->LoggedInPhoneNumberSet(m_sUserPhoneNumbers.szPreferredPhoneNumber);

	// Set the LoggedInPhoneNumber in the RemoteLoggerService
	m_pRemoteLoggerService->LoggedInPhoneNumberSet(m_sUserPhoneNumbers.szPreferredPhoneNumber);
	// Set the PhoneNumber list to be backed up with the Registration Manager
	m_pRegistrationManager->PhoneNumbersSet (&m_sUserPhoneNumbers);
	
	// Log the numbers returned from Core that we will use
	PhoneNumberList << "EventType=UserPhoneNumbersSet";
	PhoneNumberList << " PreferredPhoneNumber=" << m_sUserPhoneNumbers.szPreferredPhoneNumber;
	PhoneNumberList << " SorensonPhoneNumber=" << m_sUserPhoneNumbers.szSorensonPhoneNumber;
	PhoneNumberList << " TollFreePhoneNumber=" << m_sUserPhoneNumbers.szTollFreePhoneNumber;
	PhoneNumberList << " LocalPhoneNumber=" << m_sUserPhoneNumbers.szLocalPhoneNumber;
	PhoneNumberList << " HearingPhoneNumber=" << m_sUserPhoneNumbers.szHearingPhoneNumber;
	PhoneNumberList << " RingGroupLocalPhoneNumber=" << m_sUserPhoneNumbers.szRingGroupLocalPhoneNumber;
	PhoneNumberList << " RingGroupTollFreePhoneNumber=" << m_sUserPhoneNumbers.szRingGroupTollFreePhoneNumber;
	
	EventLog = PhoneNumberList.str();
	stiRemoteLogEventSend(&EventLog);

STI_BAIL:

	return hResult;
} // end CstiCCI::UserPhoneNumbersSet


/*!\brief Gets the current number of rings before the signmail greeting begins to play
 *		  and returns the maximum number of rings allowed.
 *
 *  \retval stiRESULT_SUCCESS
 */
stiHResult CstiCCI::RingsBeforeGreetingGet(
	uint32_t *pun32CurrentMaxRings,
	uint32_t *pun32MaxRings)
	const
{
	stiLOG_ENTRY_NAME (CstiCCI::MaxRingsBeforeGreetingGet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	*pun32CurrentMaxRings = stConferenceParams.nRingsBeforeGreeting;
	*pun32MaxRings = nMAX_RINGS_BEFORE_GREETING;

STI_BAIL:

	return (hResult);
}

/*!\brief Sets the number of rings before the signmail greeting begins to play.
 *
 *  \retval stiRESULT_SUCCESS If in the number of rings message was sent successfully.
 *  \retval stiERROR Otherwise.
 */
stiHResult CstiCCI::RingsBeforeGreetingSet(
	uint32_t un32RingsBeforeGreeting)
{
	stiLOG_ENTRY_NAME (CstiCCI::MaxRingsBeforeGreetingSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	stConferenceParams.nRingsBeforeGreeting = un32RingsBeforeGreeting;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

	m_poPM->propertySet (cmRINGS_BEFORE_GREETING, un32RingsBeforeGreeting, PropertyManager::Persistent);

	PropertyManager::getInstance ()->PropertySend(cmRINGS_BEFORE_GREETING, estiPTypeUser);

STI_BAIL:

	return (hResult);
}


/*!\brief Sets the maximum download (or receive) speed
 *
 *  \retval estiOK If in the waiting state and the maximum receive speed
 *      message was sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::MaxSpeedsSet (
	uint32_t un32RecvSpeed, ///< Download network connection speed setting
	uint32_t un32SendSpeed) ///< Upload network connection speed setting
{
	stiLOG_ENTRY_NAME (CstiCCI::MaxSpeedsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_poPM->propertySet (cmMAX_RECV_SPEED, un32RecvSpeed, PropertyManager::Persistent);
	m_poPM->propertySet (cmMAX_SEND_SPEED, un32SendSpeed, PropertyManager::Persistent);

	m_poPM->PropertySend(cmMAX_RECV_SPEED, estiPTypePhone);
	m_poPM->PropertySend(cmMAX_SEND_SPEED, estiPTypePhone);

	hResult = MaxSpeedsUpdate ();
	stiTESTRESULT();

STI_BAIL:

	return (hResult);
} // end CstiCCI::MaxRecvSpeedSet


stiHResult CstiCCI::MaxSpeedsUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	if (stConferenceParams.eAutoSpeedSetting == estiAUTO_SPEED_MODE_AUTO)
	{	//If Auto Speed is auto, set speed conference params to member variables
		//These values are set by UI call to MaxAutoSpeedsSet()
		stConferenceParams.nMaxRecvSpeed = m_un32MaxAutoRecvSpeed;
		stConferenceParams.nMaxSendSpeed = m_un32MaxAutoSendSpeed;
	}
	else
	{	//Otherwise, set speed conference params to values in property table
		m_poPM->propertyGet (cmMAX_SEND_SPEED, &stConferenceParams.nMaxSendSpeed);
		m_poPM->propertyGet (cmMAX_RECV_SPEED, &stConferenceParams.nMaxRecvSpeed);
	}

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

	// Set the upload and download speed on the fileplayer.
	m_pFilePlay->MaxSpeedSet(stConferenceParams.nMaxRecvSpeed, stConferenceParams.nMaxSendSpeed);

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::AutoSpeedSettingsUpdate (uint32_t un32AutoSpeedEnabled, EstiAutoSpeedMode eAutoSpeedMode, EstiAutoSpeedMode *eAutoSpeedSetting)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	if (!un32AutoSpeedEnabled)
	{
		stConferenceParams.eAutoSpeedSetting = estiAUTO_SPEED_MODE_LEGACY;
	}
	else
	{
		stConferenceParams.eAutoSpeedSetting = eAutoSpeedMode;
	}
	*eAutoSpeedSetting = stConferenceParams.eAutoSpeedSetting;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::AutoSpeedMinStartSpeedUpdate (uint32_t un32AutoSpeedMinStartSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	stConferenceParams.un32AutoSpeedMinStartSpeed = un32AutoSpeedMinStartSpeed;
	
	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}


/*! \brief Gets the current Auto Speed Setting based on AutoSpeedEnable
 *	 and AutoSpeedMode.
 *
 *  \return Current AutoSpeedSetting
 */
EstiAutoSpeedMode CstiCCI::AutoSpeedSettingGet ()
const
{
	stiLOG_ENTRY_NAME (CstiCCI::AutoSpeedSettingGet);

	SstiConferenceParams stConferenceParams;

	m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);

	return stConferenceParams.eAutoSpeedSetting;

} // end CstiCCI::AutoSpeedSettingGet


/*!\brief Sets the member variables for max auto receive and send speeds
 *
 *  \retval estiOK If in the waiting state and the maximum receive speed
 *      message was sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::MaxAutoSpeedsSet (
	uint32_t un32MaxAutoRecvSpeed, ///< Max download speed in auto mode
	uint32_t un32MaxAutoSendSpeed) ///< Max upload speed in auto mode
{
	stiLOG_ENTRY_NAME (CstiCCI::MaxAutoSpeedsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_un32MaxAutoRecvSpeed = un32MaxAutoRecvSpeed;
	m_un32MaxAutoSendSpeed = un32MaxAutoSendSpeed;

	hResult = MaxSpeedsUpdate ();
	stiTESTRESULT();

STI_BAIL:

	return (hResult);
} // end CstiCCI::MaxAutoSpeedsSet


/*! \brief Gets the current Maximum Receive speed.
 *
*  \return Current maximum receive speed.
 */
stiHResult CstiCCI::MaxSpeedsGet (
	uint32_t *pun32RecvSpeed, ///< Download network connection speed setting
	uint32_t *pun32SendSpeed) ///< Upload network connection speed setting
	const
{
	stiLOG_ENTRY_NAME (CstiCCI::MaxSpeedsGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	int32_t n32RecvSpeed = 0;
	int32_t n32SendSpeed = 0;

	m_poPM->propertyGet (cmMAX_SEND_SPEED, &n32SendSpeed);
	m_poPM->propertyGet (cmMAX_RECV_SPEED, &n32RecvSpeed);

	*pun32RecvSpeed = n32RecvSpeed;
	*pun32SendSpeed = n32SendSpeed;

	return hResult;
} // end CstiCCI::MaxSpeedsGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::UriListSend
//
// Description: Informs Core Services (or a VRS Provider Interface) that the
// phone is online.
//
// Abstract:
//
// Returns:
//	Success or failure
//
stiHResult CstiCCI::UriListSend ()
{
	stiLOG_ENTRY_NAME (CstiCCI::UriListSend);

	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *poCoreRequest = nullptr;

	if (estiPORTED_MODE != m_eLocalInterfaceMode)
	{
		//
		// Setup the URIs
		//
		std::string PublicSIPUri;
		std::string PrivateSIPUri;

		///
		/// Use the conferencing address sent from the SIP manager in the SIP URI.
		///
		if (m_SIPConferenceAddresses.SIPPublicAddress.length () > 0)
		{
			PublicSIPUri = "sip:\\1@";

			PublicSIPUri += m_SIPConferenceAddresses.SIPPublicAddress;

			if (m_SIPConferenceAddresses.nSIPPublicPort != nDEFAULT_SIP_LISTEN_PORT)
			{
				char szPort[7];

				snprintf (szPort, sizeof(szPort), ":%d", m_SIPConferenceAddresses.nSIPPublicPort);

				PublicSIPUri += szPort;
			}
		}

		if (m_SIPConferenceAddresses.SIPPrivateAddress.length () > 0)
		{
				PrivateSIPUri = "sip:\\1@";
				
				auto address = m_SIPConferenceAddresses.SIPPrivateAddress;
				
				// Fixes bug# 27667. If we ever get an IPv6 address we need to convert it to IPv4 before sending to Core.
				if (IPv6AddressValidateEx (address.c_str ()))
				{
					hResult = stiMapIPv6AddressToIPv4 (m_SIPConferenceAddresses.SIPPrivateAddress, &address);
					stiTESTRESULTMSG ("UriListSend failed to map to IPv4 address attempted to map %s",
									 m_SIPConferenceAddresses.SIPPrivateAddress.c_str ());
					
					stiTESTCONDMSG (IPv4AddressValidate (address.c_str ()), stiRESULT_ERROR,
								   "UriListSend failed to obtain valid IPv4 address using %s",
								   m_SIPConferenceAddresses.SIPPrivateAddress.c_str ());
				}

				PrivateSIPUri += address;

				if (m_SIPConferenceAddresses.nSIPPrivatePort != nDEFAULT_SIP_LISTEN_PORT)
				{
					char szPort[7];

					snprintf (szPort, sizeof(szPort), ":%d", m_SIPConferenceAddresses.nSIPPrivatePort);

					PrivateSIPUri += szPort;
				}
		}

		//
		// Create a core request object to send the URIs but only
		// if at least one of the URIs is not empty.
		//
		if (!PublicSIPUri.empty () || !PrivateSIPUri.empty ())
		{
			poCoreRequest = new CstiCoreRequest;
			stiTESTCOND(poCoreRequest != nullptr, stiRESULT_ERROR);

			std::string URIs[2];

			poCoreRequest->uriListSet (PublicSIPUri.c_str (), PrivateSIPUri.c_str (), nullptr, nullptr, nullptr, nullptr);
			URIs[0] = PublicSIPUri;

			stiDEBUG_TOOL (g_stiUPnPDebug,
				stiTrace ("<CstiCCI::UriListSend>  Telling core our URI%s %s%s%s\n", URIs[1].length () == 0 ? " is" : "s are", URIs[0].c_str (), URIs[1].length () == 0 ? "" : " and ", URIs[1].c_str ());
			);

			hResult = m_pCoreService->RequestSendAsync (poCoreRequest, CoreRequestAsyncCallback, (size_t)this);
			stiTESTRESULT ();

			poCoreRequest = nullptr;
		}
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (poCoreRequest)
		{
			delete poCoreRequest;
			poCoreRequest = nullptr;
		}
	}

	return (hResult);
} // end CstiCCI::UriListSend


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::PhoneOnlineSend
//
// Description: Informs Core Services (or a VRS Provider Interface) that the
// phone is online.
//
// Abstract:
//
// Returns:
//	Success or failure
//
stiHResult CstiCCI::PhoneOnlineSend ()
{
	stiLOG_ENTRY_NAME (CstiCCI::PhoneOnlineSend);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *poCoreRequest = nullptr;

	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);

	if (estiPORTED_MODE != m_eLocalInterfaceMode)
	{
		int nResult;

		// Get a constant character pointer to point to
		// the IP address to use in the directory entry.
		std::string EntryIPAddress;
		hResult = PublicIPAddressGet (&EntryIPAddress, estiTYPE_IPV4);
		stiTESTRESULT ();

		//
		// If the address is different from the last address we sent then send the new one.
		//
		if (m_LastPublicIpSent != EntryIPAddress)
		{
			// Create a core request object
			poCoreRequest = new CstiCoreRequest;
			stiTESTCOND(poCoreRequest != nullptr, stiRESULT_ERROR);

			// We are use the CoreServices system, finish preparing the request and send it.
				nResult = poCoreRequest->phoneOnline (EntryIPAddress.c_str ());
				stiTESTCOND(nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

				stiDEBUG_TOOL (g_stiUPnPDebug,
					stiTrace ("<CstiCCI::PhoneOnlineSend> Telling core our public IP is %s\n", EntryIPAddress.c_str ());
			);

			hResult = m_pCoreService->RequestSendAsync (poCoreRequest, CoreRequestAsyncCallback, (size_t)this);
			stiTESTRESULT ();

			poCoreRequest = nullptr;

			m_LastPublicIpSent.assign (EntryIPAddress);
		}
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (poCoreRequest)
		{
			delete poCoreRequest;
			poCoreRequest = nullptr;
		}
	}

	return (hResult);
} // end CstiCCI::PhoneOnlineSend


/*!\brief Mutes/Unmutes the audible ringer.
 *
 *  This function can be called while in the waiting state.
 *
 *  \retval estiOK If the function was successful.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::AudibleRingerMute (
	bool bMute)  //!< estiTRUE or estiFALSE.
{
	stiLOG_ENTRY_NAME (CstiCCI::AudibleRingerMute);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	m_bAudibleRingerMuted = bMute;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
} // end CstiCCI::AudibleRingerMute


/*! \brief Gets the system's public IP address for directory listings.
 *
 *  It is recommended that this function not be called until the conference
 *  engine has finished initializing.  This function may be called at any time,
 *  but there is no guarantee that the setting is initialized or valid until the
 *  conference engine has reached the WAITING state.
 *
 *  NOTE:  The application is responsible for freeing the memory
 *  allocated to return this string.  Use the command "delete [] szAddr;"
 *
 * \param eAssignMethod		The assignment method for which the IP address is desired.
 * \param pAddress 		The buffer that will contain the IP address
 * \param eIpAddressType	Whether to fetch ipv4 or ipv6 address
 *
 * \return Success or failure in an stiHResult
 */
stiHResult CstiCCI::PublicIPAddressGet (
	EstiPublicIPAssignMethod eAssignMethod,
	std::string *pAddress,
	EstiIpAddressType eIpAddressType) const
{
	stiLOG_ENTRY_NAME (CstiCCI::PublicIPAddressGet);
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string propAddress;

	switch (eAssignMethod)
	{
		case estiIP_ASSIGN_AUTO:
			m_poPM->propertyGet (cmPUBLIC_IP_ADDRESS_AUTO, &propAddress);
			pAddress->assign (propAddress);

			break;

		case estiIP_ASSIGN_MANUAL:

			m_poPM->propertyGet (cmPUBLIC_IP_ADDRESS_MANUAL, &propAddress);
			pAddress->assign (propAddress);

			break;

		case estiIP_ASSIGN_SYSTEM:

			/* If the get fails, load the return value with the default value
			 * sent in.
			 */
			hResult = stiGetLocalIp (pAddress, eIpAddressType);
			stiTESTRESULT();

			break;

		default:
			break;
	} // end switch

STI_BAIL:

	return hResult;
} // end CstiCCI::PublicIPAddressGet


/*!\brief Sets the public IP assignment method for the system.
 *
 *  This method is used to set the public IP assignment method.  When selecting
 *  the method stiIP_ASSIGN_MANUAL, it also requires the IP address to be used
 *  as the public IP address.
 *
 *  This function should be called only while in the WAITING state and will
 *  fail if the state is not WAITING.
 *
 *  \retval estiOK If in the waiting state and the function was successful.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::PublicIPAddressInfoSet (
	EstiPublicIPAssignMethod eIPAssignMethod,    /*!< The public IP address
													 * assignment method -
													 * estiIP_ASSIGN_AUTO,
													 * estiIP_ASSIGN_MANUAL,
													 * estiIP_ASSIGN_SYSTEM
													 */
	const std::string &ipAddress)                 /*!< System's public IP
													 * address - use
													 * "xxx.xxx.xxx.xxx" format
													 * where "xxx" is a number
													 * between 0 and 255.
													 * Default = NULL.
													 */
{
	stiLOG_ENTRY_NAME (CstiCCI::PublicIPAddressInfoSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	stiHResult hNewResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	std::string CurrentIP;
	std::string NewIP;

	//
	// Make sure the assignment method is correct
	//
	stiTESTCOND (eIPAssignMethod == estiIP_ASSIGN_MANUAL || eIPAssignMethod == estiIP_ASSIGN_AUTO || eIPAssignMethod == estiIP_ASSIGN_SYSTEM, stiRESULT_ERROR);

	//
	// Make sure we are in the right state before proceeding.
	//
	stiTESTCOND (m_pConferenceManager->eventLoopIsRunning()
			&& (0 == m_pConferenceManager->CallObjectsCountGet (esmiCS_CONNECTING |  esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH)), stiRESULT_ERROR);

	hResult = PublicIPAddressGet (&CurrentIP, estiTYPE_IPV4);

	// If the assign method is manual, we need an IP address too.
	if (estiIP_ASSIGN_MANUAL == eIPAssignMethod)
	{
		if (IPAddressValidate (ipAddress.c_str ()))
		{
			// We have a valid IP address.  Store it in the PropertyManager.
			m_poPM->propertySet (cmPUBLIC_IP_ADDRESS_MANUAL, ipAddress, PropertyManager::Persistent);
		} // end if
		else
		{
			// This is an error.  When manual is selected, we need an address.
			stiTHROW (stiRESULT_ERROR);
		} // end else
	} // end if
	else
	{
		//
		// Save the value but don't bother validating it because it is not used in the current configuration.
		//
		m_poPM->propertySet (cmPUBLIC_IP_ADDRESS_MANUAL, ipAddress, PropertyManager::Persistent);
	}

	// Yes.  Set the public IP assignment method in the configuration
	// object.
	m_poPM->propertySet (cmPUBLIC_IP_ASSIGN_METHOD, eIPAssignMethod, PropertyManager::Persistent);

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	m_poPM->propertyGet (cmPUBLIC_IP_ASSIGN_METHOD, &stConferenceParams.eIPAssignMethod);

	hResult = PublicIPAddressGet (&stConferenceParams.PublicIPv4, estiTYPE_IPV4);
#ifdef IPV6_ENABLED
	hResult = PublicIPAddressGet (&stConferenceParams.PublicIPv6, estiTYPE_IPV6);
#endif // IPV6_ENABLED
	m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);

	m_poPM->PropertySend(cmPUBLIC_IP_ASSIGN_METHOD, estiPTypePhone);
	m_poPM->PropertySend(cmPUBLIC_IP_ADDRESS_MANUAL, estiPTypePhone);

	// Get the Public IP address again and see if it has changed.
	hNewResult = PublicIPAddressGet (&NewIP, estiTYPE_IPV4);

	if (!stiIS_ERROR (hResult)
		&& !stiIS_ERROR (hNewResult)
		&& CurrentIP != NewIP)
	{
		PhoneOnlineSend ();
	} // end if

STI_BAIL:

	return (hResult);
} // end CstiCCI::PublicIPAddressInfoSet


/*!\brief Processes any potential changes made in the public IP address
 *
 * This method determines if the conference manager needs to be updated
 * because of changes made to the public ip settings or the auto detected public IP.
 * It also calls PhoneOnlineSend to determine if core needs to be updated with
 * changes made to the public ip.
 */
stiHResult CstiCCI::PublicIpChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);

	if (!stiIS_ERROR (hResult))
	{
		EstiPublicIPAssignMethod ipAssignMethod = estiIP_ASSIGN_AUTO;
		std::string CurrentPublicIPv4Address;
#ifdef IPV6_ENABLED
		std::string CurrentPublicIPv6Address;
#endif
		std::string autoDetectedIPAddress;

		//
		// Get the public IP address and method
		//
		m_poPM->propertyGet (cmPUBLIC_IP_ASSIGN_METHOD, &ipAssignMethod);

		PublicIPAddressGet (&CurrentPublicIPv4Address, estiTYPE_IPV4);
#ifdef IPV6_ENABLED
		PublicIPAddressGet (&CurrentPublicIPv6Address, estiTYPE_IPV6);
#endif

		m_serviceOutageClient.publicIpAddressSet (CurrentPublicIPv4Address);

		//
		// Get the auto detected public IP address
		//
		m_poPM->propertyGet(cmPUBLIC_IP_ADDRESS_AUTO, &autoDetectedIPAddress);

		//
		// If the assignment method, the public IP used by the assignment method or if the auto
		// detected public IP has changed then setup the conference parameters structure
		// and send it to the conference manager.
		//
		if (stConferenceParams.eIPAssignMethod != ipAssignMethod
			|| stConferenceParams.PublicIPv4 != CurrentPublicIPv4Address
#ifdef IPV6_ENABLED
		 || stConferenceParams.PublicIPv6 != CurrentPublicIPv6Address
#endif
		 || stConferenceParams.AutoDetectedPublicIp != autoDetectedIPAddress)
		{
			stConferenceParams.eIPAssignMethod = ipAssignMethod;

			stConferenceParams.PublicIPv4.assign(CurrentPublicIPv4Address);
#ifdef IPV6_ENABLED
			stConferenceParams.PublicIPv6.assign (CurrentPublicIPv6Address);
#endif
			stConferenceParams.AutoDetectedPublicIp = autoDetectedIPAddress;

			m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		}
	}

	// Notify that the phone is online
	PhoneOnlineSend ();

	AppNotify (estiMSG_PUBLIC_IP_CHANGED, 0);

	return hResult;
}


/*!\brief Notifies the Conference Engine of a public IP change.
 *
 *  This function handles the result of an auto detect IP change.
 *  It should only be called if the public ip address has actually
 *  changed and the system is set to use automatic IP detection.
 *  This function should only be called by CoreServices and not the
 *  application.
 *
 *  \retval estiOK If the function successfully executes.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::PublicIPResolved (
	const std::string &publicIp) //!< Public IP address
{
	EstiBool bChangeAddress = estiFALSE;

	// Were we passed a Public IP address, if not, simply drop to the end.
	// This would be the case if either cmPUBLIC_IP_ASSIGN_METHOD or
	// cmPUBLIC_IP_ADDRESS_MANUAL had been responses from CoreServices.
	if (!publicIp.empty())
	{
		// Get the current Public IP
		std::string tmpEntryIPAddress;

		m_poPM->propertyGet(
			cmPUBLIC_IP_ADDRESS_AUTO,
			&tmpEntryIPAddress);

		// Has it changed?
		if (tmpEntryIPAddress != publicIp)
		{
			// Yes! Save the detected ip address
			m_poPM->propertySet(cmPUBLIC_IP_ADDRESS_AUTO,
				publicIp,
				PropertyManager::Persistent);

				bChangeAddress = estiTRUE;
		} // end if
	}

	// Have we sent the phone online yet?
	// - OR -
	// Has our public IP address changed?
	if (m_LastPublicIpSent.empty () || bChangeAddress)
	{
		PublicIpChanged ();
	} // end if

	return stiRESULT_SUCCESS;
} // end


/*!\brief Causes an application printscreen.
 *
 *  Causes the app do a printscreen.  This is generated by VRCL.
 *
 */
void CstiCCI::PrintScreen ()
{
	AppNotify (estiMSG_PRINTSCREEN, 0);
}


/*!\brief Sets whether to use VCO by default
 *
 *  This function sets a preference for the use of VCO with VRS calls.
 *
 *  \retval estiOK If in an initialized state and the Use VCO set message is
 *      successfully sent.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::VCOUseSet (
	bool bVRSUseVCO) //!< Should we use VCO by default?
{
	stiLOG_ENTRY_NAME (CstiCCI::szVRSUseVCO);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_bUseVCO = bVRSUseVCO;

	return (hResult);
} // end CstiCCI::VCOUseSet


/*!\brief Sets VCO Type to use by default
 *
 *  This function sets a preference for the type of VCO with VRS calls.
 *
 *  \retval estiOK If in an initialized state and the VCO Type set message is
 *      successfully sent.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::VCOTypeSet (
	EstiVcoType eVCOType) //!< Should we use VCO by default?
{
	stiLOG_ENTRY_NAME (CstiCCI::VCOTypeSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	// If the type has changed, then update it
	if (eVCOType != stConferenceParams.ePreferredVCOType)
	{
		// If preferred type is off, then set the other values to disabled
		if (estiVCO_NONE == eVCOType)
		{
			stConferenceParams.vcoNumber.clear ();
		}
		else
		{
			stConferenceParams.vcoNumber = m_vcoNumber;
		}

		stConferenceParams.ePreferredVCOType = eVCOType;

		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();

		m_ePreferredVCOType = eVCOType;
	}

STI_BAIL:

	return (hResult);
} // end CstiCCI::VCOTypeSet

/*!
 * \brief Sets audio enabled (on/off)
 *
 * \param eEnabled Audio Enabled - estiTRUE or estiFALSE.
 *
 * \return Success or failure result
 */
stiHResult CstiCCI::AudioEnabledSet (
		bool eEnabled)
{
	stiLOG_ENTRY_NAME (CstiCCI::AudioEnabledSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	stConferenceParams.stMediaDirection.bOutboundAudio = eEnabled;
	stConferenceParams.stMediaDirection.bInboundAudio = eEnabled;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);

} // end CstiCCI::AudioEnabledSet

#ifdef stiTUNNEL_MANAGER

bool CstiCCI::TunnelingEnabledGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::TunnelingEnabledGet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	return stConferenceParams.eNatTraversalSIP == estiSIPNATEnabledUseTunneling;

STI_BAIL:

	return false;

}

stiHResult CstiCCI::TunnelingEnabledSet (
	bool eEnabled)
{
	stiLOG_ENTRY_NAME (CstiCCI::TunnelingEnabledSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	if (eEnabled)
	{
		stiRemoteLogEventSend ("UserEvent=UserEnabledTunneling");
		stConferenceParams.eNatTraversalSIP = estiSIPNATEnabledUseTunneling;
	}
	else
	{
		stiRemoteLogEventSend ("UserEvent=UserDisabledTunneling");
		stConferenceParams.eNatTraversalSIP = estiSIPNATEnabledMediaRelayAllowed;
	}

	m_poPM->propertySet(NAT_TRAVERSAL_SIP, stConferenceParams.eNatTraversalSIP, PropertyManager::Persistent);
	m_poPM->saveToPersistentStorage();
	m_poPM->PropertySend(NAT_TRAVERSAL_SIP, estiPTypePhone);
	
	// If we're on an IPv6-only network, tunneling won't work and we need to
	// temporarily disable it until we change networks and IPv6EnabledChanged is
	// called.
	if (m_bIPv6Enabled && eEnabled)
	{
		stConferenceParams.eNatTraversalSIP = estiSIPNATEnabledMediaRelayAllowed;
	}

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);

}
#endif //stiTUNNEL_MANAGER

/*!\brief Sets the callback number for Voice Carry-Over
 *
 *  This function should be called while in the WAITING state and will fail if
 *  the state not WAITING.
 *
 *  This function sets a preference for the use of VCO with VRS calls.
 *
 *  \retval estiOK If in an initialized state and the VCO Callback Number Set
 *      message is successfully sent.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::VCOCallbackNumberSet (
	const std::string &vrsVCOCallbackNumber)    //!< Name or IP of VRS Server.
{
	stiLOG_ENTRY_NAME (CstiCCI::VRSVCOCallbackNumberSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_vcoNumber = vrsVCOCallbackNumber;

	if (estiVCO_NONE != m_ePreferredVCOType)
	{
		SstiConferenceParams stConferenceParams;

		hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		stiTESTRESULT ();

		stConferenceParams.vcoNumber = m_vcoNumber;

		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
} // end CstiCCI::VRSVCOCallbackNumberSet


//:-----------------------------------------------------------------------------
//: Methods for recording a SignMail message
//:-----------------------------------------------------------------------------

/*!\brief  Initializes the call to send a SignMail message.
 *
 * 	This function is called by DirectoryResolveResult() and initializes the call
 * 	to send a SignMail message.
 *
 *  \retval estiOK When successful.
 *  \retval otherwise estiERROR.
 */

stiHResult CstiCCI::CallInfoForSignMailSet(
	const CstiDirectoryResolveResult *poDirectoryResolveResult,
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiMessageInfo stMessageInfo;

	if (!call ||
		!poDirectoryResolveResult)
	{
		stiTHROW (stiRESULT_ERROR);
	}

	hResult = call->MessageInfoGet (&stMessageInfo);
	stiTESTRESULT ();

	stMessageInfo.bP2PMessageSupported = poDirectoryResolveResult->P2PMessageSupportedGet ();
	stMessageInfo.nMaxNumbOfRings = poDirectoryResolveResult->MaxNumberOfRingsGet ();
	stMessageInfo.nMaxRecordSeconds = poDirectoryResolveResult->MaxRecordSecondsGet ();
	stMessageInfo.nRemoteDownloadSpeed = poDirectoryResolveResult->RemoteDownloadSpeedGet() > 0 ? poDirectoryResolveResult->RemoteDownloadSpeedGet() : nDEFAULT_SIGNMAIL_DOWNLOAD_SPEED;

	//
	// If the P2P signmail is not supported then don't load the greeting information
	// into the message info structure (it appears that cloud services can return
	// greeting information even though the remote user has signmail disabled).
	//
	stMessageInfo.GreetingURL.clear ();
	stMessageInfo.GreetingText.clear ();
	stMessageInfo.bCountdownEmbedded = estiFALSE;

	if (stMessageInfo.bP2PMessageSupported)
	{
		const char* pszGreetingUrl = poDirectoryResolveResult->GreetingURLGet ();
		if (pszGreetingUrl)
		{
			stMessageInfo.GreetingURL = pszGreetingUrl;
		}

		if (poDirectoryResolveResult->GreetingTextGet())
		{
			stMessageInfo.GreetingText = poDirectoryResolveResult->GreetingTextGet ();
		}

		stMessageInfo.eGreetingType = poDirectoryResolveResult->GreetingPreferenceGet();
		stMessageInfo.bCountdownEmbedded = poDirectoryResolveResult->CountdownEmbeddedGet();
	}

	// Set the response message info on the call object.
	call->MessageInfoSet (&stMessageInfo);

	// Check to make sure that Messaging is supported, we have a valid
	// URL (not the empty GUID) and that the call is not blocked.
	if (m_bSignMailEnabled &&
		poDirectoryResolveResult->P2PMessageSupportedGet() &&
		!poDirectoryResolveResult->BlockedGet() &&
		!poDirectoryResolveResult->AnonymousBlockedGet() &&
		(LocalInterfaceModeGet() != estiINTERPRETER_MODE ||
		 (LocalInterfaceModeGet() == estiINTERPRETER_MODE && m_pVRCLServer->SignMailEnabledGet ())) &&
		LocalInterfaceModeGet() != estiPORTED_MODE)
	{
		if (stMessageInfo.eGreetingType == eGREETING_TEXT_ONLY ||
			poDirectoryResolveResult->GreetingURLGet())
		{
			// Set up the call object and let the CCI know that we need to play the countdown.
			call->LeaveMessageSet(estiSM_LEAVE_MESSAGE_NORMAL);
		}
		else
		{
			call->LeaveMessageSet(estiSM_LEAVE_MESSAGE_SKIP_GREETING);
		}

		// Indicate that we haven't loaded the greeting yet.
		m_bSignMailGreetingLoaded = false;

		// Notify the VRCL Client that the remote system supports Signmail
		AppNotify (estiMSG_SIGNMAIL_AVAILABLE, (size_t)call.get());
		m_pVRCLServer->VRCLNotify (estiMSG_SIGNMAIL_AVAILABLE);
	}
	else
	{
		call->LeaveMessageSet (estiSM_NONE);
	}

STI_BAIL:
	return (hResult);
}

/*!\brief
 *
 *
 *  \retval estiOK When successful.
 *  \retval otherwise estiERROR.
 */
stiHResult CstiCCI::RecordMessageDelete (
	const SstiRecordedMessageInfo &recordedMsgInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = nullptr;

	stiTESTCOND(!recordedMsgInfo.messageId.empty (), stiRESULT_INVALID_PARAMETER);

	pMessageRequest = new CstiMessageRequest;
	if (pMessageRequest)
	{
		pMessageRequest->SignMailDelete (recordedMsgInfo.messageId.c_str ());
		pMessageRequest->RemoveUponCommunicationError () = estiTRUE;
		hResult = MessageRequestSend (pMessageRequest, nullptr);

		pMessageRequest = nullptr;
	}

STI_BAIL:

	if (pMessageRequest)
	{
		delete pMessageRequest;
		pMessageRequest = nullptr;
	}

	return (hResult);
}


/*!\brief Notifies app that the mailbox is full and closes the call.
 *
 *
 *  \retval estiOK When successful.
 *  \retval otherwise estiERROR.
 */
stiHResult CstiCCI::RecordMessageMailBoxFull (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// The mailbox is full so hangup the call.
	call->HangUp();

	// If privacy was turned off make sure we turn it back on.
	if (m_vidPrivacyEnabled)
	{
		m_vidPrivacyEnabled = false;

		// If video privacy was enabled, enable it.
		IstiVideoInput::InstanceGet ()->PrivacySet (estiTRUE);
	}

	// Notify the application of the mailbox being full. VCRL will be notified thru the
	// hangup of the call.
	AppNotify(estiMSG_MAILBOX_FULL, 0);

	return hResult;
}

/*!\brief
 *
 *
 *  \retval estiOK When successful.
 *  \retval otherwise estiERROR.
 */
stiHResult CstiCCI::RecordMessageSend (
	const SstiRecordedMessageInfo &recordedMsgInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = nullptr;

	stiTESTCOND(!recordedMsgInfo.messageId.empty (), stiRESULT_INVALID_PARAMETER);

	pMessageRequest = new CstiMessageRequest;
	if (pMessageRequest)
	{
		stiDEBUG_TOOL (g_stiUiEventDebug,
			stiTrace ("Sent save P2P message request\n");
		);

		pMessageRequest->SignMailSend (recordedMsgInfo.toPhoneNumber,
									   recordedMsgInfo.messageId,
									   recordedMsgInfo.un32MessageSize,
									   recordedMsgInfo.un32MessageSeconds,
									   recordedMsgInfo.un32MessageBitrate,
									   m_signMailText,
									   m_signMailTextStartSeconds,
									   m_signMailTextEndSeconds);

		pMessageRequest->RemoveUponCommunicationError () = estiTRUE;
		MessageRequestSend (pMessageRequest, &m_unSignMailSendRequestId);

		pMessageRequest = nullptr;
	}

	//
	// If we have a call object then store the SignMail duration in the
	// call object for purposes of logging it with the other call stats.
	//
	if (m_spLeaveMessageCall)
	{
		m_spLeaveMessageCall->SignMailDurationSet(recordedMsgInfo.un32MessageSeconds);
	}

STI_BAIL:

	if (pMessageRequest)
	{
		delete pMessageRequest;
		pMessageRequest = nullptr;
	}

	return (hResult);
}


#ifdef stiIMAGE_MANAGER
///\brief Returns a pointer to the Image Manager
///
IstiImageManager *CstiCCI::ImageManagerGet ()
{
	return m_pImageManager;
}
#endif


///\brief Returns a pointer to the block list manager.
///
///\returns If the feature is enabled then a pointer to the block list manager is returned.
///Otherwise, NULL is returned.
IstiBlockListManager *CstiCCI::BlockListManagerGet ()
{
	return m_pBlockListManager;
}


///\brief Returns a pointer to the ring group manager.
///
IstiRingGroupManager *CstiCCI::RingGroupManagerGet ()
{
	return m_pRingGroupManager;
}


#ifdef stiMESSAGE_MANAGER
///\brief Returns a pointer to the message manager.
///
///\returns If the feature is enabled then a pointer to the block list manager is returned.
///Otherwise, NULL is returned.
IstiMessageManager *CstiCCI::MessageManagerGet ()
{
	return m_pMessageManager;
}
#endif


///\brief Returns a pointer to the contact manager.
///
///\returns A pointer to the contact manager is returned.
IstiContactManager *CstiCCI::ContactManagerGet ()
{
	return m_pContactManager;
}


#ifdef stiLDAP_CONTACT_LIST
///\brief Returns a pointer to the LDAP Directory contact manager.
///
///\returns A pointer to the LDAP Directory contact manager is returned.
IstiLDAPDirectoryContactManager *CstiCCI::LDAPDirectoryContactManagerGet ()
{
	return m_pLDAPDirectoryContactManager;
}
#endif

#ifdef stiCALL_HISTORY_MANAGER
///\brief Returns a pointer to the call history manager.
///
///\returns A pointer to the call history manager is returned.
IstiCallHistoryManager *CstiCCI::CallHistoryManagerGet ()
{
	return m_pCallHistoryManager;
}
#endif

//!\brief Finds a call by the request id
//
// \retval A pointer to the call object or NULL if not found.
//
CstiCallSP CstiCCI::FindCallByRequestId(
	unsigned int unRequestId) //!< The request id associated to the call object to find
{
	return m_pConferenceManager->CallObjectGetByAppData ((size_t)unRequestId);
}


static stiHResult InsertDialedStringInUriString (
	std::string *pString,
	CstiCallSP call,
	const char *pszNewNumber)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int nLoc = pString->find ("\\1");
	if (0 < nLoc)
	{
		// Construct a string with the new number (if it exists).
		std::string oDialString ((pszNewNumber && *pszNewNumber) ? pszNewNumber : "");

		// If we didn't load the string with the new number, get the original dial string.
		if (oDialString.empty ())
		{
			if (call->StateGet () != esmiCS_INIT_TRANSFER
				&& call->StateGet () != esmiCS_TRANSFERRING)
			{
				call->RemoteDialStringGet (&oDialString);
			}
			else
			{
				call->TransferDialStringGet (&oDialString);
			}
		}
		pString->replace (nLoc, 2, oDialString);
	}

	return hResult;
}


/*!\brief Find a URI with the passed in type.
 *
 *  This function will look through the passed in list of URI strings for one
 *  that is of the type requested.  If found, it will return "true" and if the
 *  string parameter is not NULL, will load it with the found string.
 *
 *  \retval true if a URI of type un32UriType was found.
 *  \retval false otherwise.
 */
static bool FindUri (
	const std::list <SstiURIInfo> *pList,
	uint32_t un32UriType,
	uint32_t un32Network,
	SstiURIInfo *poURIInfo)
{
	bool bRet = false;
	const char szSIP_PROTOCOL[] = "sip:";
	const char szSIPS_PROTOCOL[] = "sips:";

	std::list <SstiURIInfo>::const_iterator it;

	for (it = pList->begin (); it != pList->end (); it++)
	{
		if (un32Network & (*it).eNetwork)
		{
			if (un32UriType & eURI_SIP)
			{
				if (0 == strncasecmp (szSIP_PROTOCOL, (*it).URI.c_str (), strlen (szSIP_PROTOCOL)))
				{
					if (poURIInfo)
					{
						*poURIInfo = *it;
					}

					bRet = true;
					break;
				}
			}

			// We will look for a SIPS URI if we didn't find a satisfying URI already (and thus hit the break command).
			if (un32UriType & eURI_SIPS)
			{
				if (0 == strncasecmp (szSIPS_PROTOCOL, (*it).URI.c_str (), strlen (szSIPS_PROTOCOL)))
				{
					if (poURIInfo)
					{
						*poURIInfo = *it;
					}

					bRet = true;
					break;
				}
			}
		}
	}

	return bRet;
}


//:-----------------------------------------------------------------------------
//: Application Functions -- Directory Services
//:-----------------------------------------------------------------------------

//:-----------------------------------------------------------------------------
//: Directory Services Callback Functions
//:-----------------------------------------------------------------------------

/*!\brief Returns the results from a directory service entry find request.
 *
 *  This function handles the return of the results from an entry find request
 *  to the directory services.  The directory services controller only does
 *  entry find requests for user ID's (which are unique in the directory
 *  server).  This guarantees that only a single entry will be returned to the
 *  CCI.
 *
 *  NOTE:  The application is responsible for freeing the memory
 *  allocated to the entry return list.
 *
 *  If the request ID returned matches the request ID the CCI is waiting for
 *  then use the results.  The IP in the entry is passed to the conference
 *  manager to continue the call dialing.
 *
 *  \retval estiOK If the directory service entry find results were returned
 *      and processed successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::DirectoryResolveReturn (
	const CstiCoreResponse *poResponse)    //!< The response from CoreServices
{
	stiLOG_ENTRY_NAME (CstiCCI::DirectoryResolveReturn);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiCallResult eCallResult = estiRESULT_DIRECTORY_FIND_FAILED;
	SstiCallResolution *pCallResolution;
	std::string SpawnCallNumber;

	//
	// Find the call matching this request id.
	//
	auto call = FindCallByRequestId (poResponse->RequestIDGet());

	stiTESTCOND(call, stiRESULT_ERROR);

	// No longer looking for results; set the request id to zero.
	call->AppDataSet ((size_t)0);
	// We can perform a Directory Resolve in a disconnected state to allow for SignMail with Sorenson Bridge.
	stiTESTCOND((esmiCS_CONNECTING == call->StateGet ()
				&& estiSUBSTATE_RESOLVE_NAME == call->SubstateGet ())
				|| call->StateValidate (esmiCS_INIT_TRANSFER | esmiCS_TRANSFERRING |
										esmiCS_CONNECTED | esmiCS_DISCONNECTED),
				stiRESULT_INVALID_CALL_OBJECT_STATE);
	
	call->SpawnCallNumberGet(&SpawnCallNumber);
	call->SpawnCallNumberSet("");
	
	// This is the request we are waiting for.  Was the entry find
	// request successful?
	if (estiOK == poResponse->ResponseResultGet ())
	{
		if (!SpawnCallNumber.empty ())
		{
			// If we have a SpawnCallNumber we did not create a new call object for this directory resolve.
			// We need to create a new call object before proceeding.
			call->SpawnCallNumberSet("");
			std::string newPhoneNumber;
			CstiPhoneNumberValidator::InstanceGet ()->
				PhoneNumberReformat(SpawnCallNumber, newPhoneNumber);

			call = m_pConferenceManager->outgoingCallConstruct (newPhoneNumber, estiSUBSTATE_RESOLVE_NAME, false);
		}
		const CstiDirectoryResolveResult *poDirectoryResolveResult = poResponse->DirectoryResolveResultGet ();
		
		// Check to see if a SignMail should be sent.
		CallInfoForSignMailSet(poDirectoryResolveResult, call);
		
		std::string VideoMailNumber;
		call->VideoMailServerNumberGet (&VideoMailNumber);
		// If we have a VideoMailServerNumber then this directory resolve was meant to only get SignMail information.
		if (!VideoMailNumber.empty())
		{
			// We are going to leave a message for a Sorenson endpoint behind an IVVR.
			// In order to also update their missed call history we need to set m_lastDialed
			// to be the number returned by the Media Server and call MissedCallHandle.
			m_lastDialed = VideoMailNumber;
			MissedCallHandle(call);
			
			if(call->LeaveMessageGet () != estiSM_NONE)
			{
				SignMailStart (call);
			}
			else
			{
				call->SignMailInitiatedSet (false);
				call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
				call->HangUp ();
			}
		}
		// If we have an DirectSignMailNumber then this directory resolve was meant to only get SignMail information.
		else if (call->directSignMailGet())
		{
			if (call->LeaveMessageGet() != estiSM_NONE)
			{
				// We should never play a greeting when sending a direct SignMail.  To do this we get the message info and
				// set the greeting type to none and the URL to the empty string.
				SstiMessageInfo messageInfo;
				call->MessageInfoGet (&messageInfo);
				messageInfo.GreetingURL = "";
				messageInfo.eGreetingType = eGREETING_NONE;
				call->MessageInfoSet (&messageInfo);

				call->SignMailInitiatedSet(true);

				// Go to the disconnected:leaveMessage state.
				call->NextStateSet (esmiCS_DISCONNECTED, estiSUBSTATE_LEAVE_MESSAGE);
			}
			else
			{
				const CstiDirectoryResolveResult *poDirectoryResolveResult = poResponse->DirectoryResolveResultGet ();

				call->SignMailInitiatedSet (false);

				if (poDirectoryResolveResult->AnonymousBlockedGet ())
				{
					call->ResultSet (estiRESULT_ANONYMOUS_DIRECT_SIGNMAIL_NOT_ALLOWED);
				}
				else
				{
					call->ResultSet (estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE);
				}

				call->HangUp ();
			}
		}
		// As long as the call object isn't in a disconnected state, handle the response. Fixes bug #24469.
		else if (!call->StateValidate (esmiCS_DISCONNECTED))
		{
			// If we have a connected call and are not transferring
			// and we are placing a new call then place the connected call on hold.
			ConnectedCallHold ();
			
			if (m_bDeterminingDialMethod
			 && !m_Extension.empty ())
			{
				//
				// This case occurs when the user provided a phonenumber with an extension
				// using the estiUNKNOWN dial type and it turned out to be a point-to-point call.
				// Extensions are not supported with point-to-point calls.
				//
				eCallResult = estiRESULT_NO_P2P_EXTENSIONS;

				stiTHROW (stiRESULT_ERROR);
			}

			SstiConferenceParams stConferenceParams;

			hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
			stiTESTRESULT ();

			call->OriginalDialMethodSet(estiDM_BY_DS_PHONE_NUMBER);
			call->DialMethodSet (estiDM_BY_DS_PHONE_NUMBER);

			// Figure out the dial string to dial
			CstiRoutingAddress *pDialString = nullptr; ///< This will hold the resolved dial string to dial

			//
			// For interpreter mode, only consider URIs from iTRS.  Otherwise, consider
			// all URIs.
			//
			uint32_t un32Network = SstiURIInfo::estiSorenson | SstiURIInfo::estiITRS;

			// todo: is this needed now that interpreter endpoints always call through the agent realm?
			// TODO: need to revisit this now that interpreters are registering with the agent realm
			if (m_eLocalInterfaceMode == estiINTERPRETER_MODE)
			{
				un32Network = SstiURIInfo::estiITRS;
			}

			std::list <SstiURIInfo> sUriList;
			poDirectoryResolveResult->UriListGet (&sUriList);

			if (!sUriList.empty ()) // If the list is not empty...
			{
				SstiURIInfo oURIInfo;
				// Find the first (highest priority) URI we can accept
				if (FindUri (&sUriList, eURI_SIP, un32Network, &oURIInfo))
				{
					call->UriSet (oURIInfo.URI);

					InsertDialedStringInUriString (&oURIInfo.URI, call, poDirectoryResolveResult->NewNumberGet ());

					pDialString = new CstiRoutingAddress (oURIInfo.URI);
				}
			}

			// Set the callBlocked info.
			call->CallBlockedSet (poDirectoryResolveResult->BlockedGet ());

			// Did we get an Address?
			if (!pDialString)
			{ // No.

				// If the URI list is empty or there are known URIs (sip, sips) for any network
				if (sUriList.empty()
				 || FindUri (&sUriList, eURI_SIP | eURI_SIPS, SstiURIInfo::estiSorenson | SstiURIInfo::estiITRS, nullptr))
				{
					// If the call isn't being blocked
					if (!poDirectoryResolveResult->BlockedGet () && !poDirectoryResolveResult->AnonymousBlockedGet())
					{
						MissedCallHandle (call);
					}

					if (poDirectoryResolveResult->AnonymousBlockedGet ())
					{
						eCallResult = estiRESULT_ANONYMOUS_CALL_NOT_ALLOWED;
					}
					else
					{
						eCallResult = estiRESULT_REMOTE_SYSTEM_UNREACHABLE;
					}

					// We want to throw the error but we don't want to log it. The call result will
					// be logged as part of the call stats.
					stiTESTCOND_NOLOG (false, stiRESULT_ERROR);
				}
				else if (poDirectoryResolveResult->BlockedGet ())
				{
					eCallResult = estiRESULT_REMOTE_SYSTEM_UNREACHABLE;

					stiTHROW (stiRESULT_ERROR);
				}
				else if (poDirectoryResolveResult->AnonymousBlockedGet ())
				{
					eCallResult = estiRESULT_ANONYMOUS_CALL_NOT_ALLOWED;

					stiTHROW (stiRESULT_ERROR);
				}
				else if (m_eLocalInterfaceMode == estiHEARING_MODE)
				{
					eCallResult = estiRESULT_VRS_CALL_NOT_ALLOWED;

					stiTHROW (stiRESULT_ERROR);
				}
				// There are URIs in the list but they are not URIs used in relay (sip, sips).
				// Treat it as a VRS call and send the original dial string.
				// Note:  An example of this might be an AIM address for a VRS to IPRelay call.
				else
				{
					hResult = VRSCallCreate (call);
					stiTESTRESULT ();
				}
			} // end if
			else
			{ // Yes.
				call->RoutingAddressSet (*pDialString);

				// CalledNameSet is necessary so that the called name can be sent to the remote
				// device. (The remote phone uses value to determine if we are logging a missed
				// call for them or whether they need to do it themselves.)
				call->CalledNameSet (poDirectoryResolveResult->NameGet ());

				if (nullptr != poDirectoryResolveResult->FromNameGet ()
				&& 0 < strlen (poDirectoryResolveResult->FromNameGet ()))
				{
					const char *pFromName = poDirectoryResolveResult->FromNameGet ();
					std::string fromName;
					if (pFromName)
					{
						fromName = pFromName;
					}
					call->LocalDisplayNameSet (fromName);

					//
					// If we are not in tech support or interpreter mode
					// and the remote end point has a contact entry for us
					// then override our alternate name just for this call.
					//
					// BUGBUG: Should alternate name be used if we are not in
					// tech support or interpreter mode?
					//
	//					if (estiTECH_SUPPORT_MODE != m_eLocalInterfaceMode
	//					 && estiINTERPRETER_MODE != m_eLocalInterfaceMode)
	//					{
	//						call->localAlternateNameSet (poDirectoryResolveResult->FromNameGet ());
	//					}
				}

				int nCallResolution = SstiCallResolution::eUNKNOWN;

				if (poDirectoryResolveResult->NewNumberGet()
					&& *poDirectoryResolveResult->NewNumberGet())
				{
					call->NewDialStringSet (poDirectoryResolveResult->NewNumberGet ());
					nCallResolution |= SstiCallResolution::eREDIRECTED_NUMBER;
				}

				if (m_bDeterminingDialMethod && m_bReportDialMethodDetermination)
				{
					nCallResolution |= SstiCallResolution::eDIAL_METHOD_DETERMINED;
				}

				//
				// Determine if this is dialing a member of its own ring group
				//
				std::string DialString;
				call->RemoteDialStringGet(&DialString);

				if (esmiCS_TRANSFERRING == call->StateGet ())
				{
					call->TransferDialStringGet (&DialString);
				}

				call->DialedOwnRingGroupSet(
					IsDialStringInRingGroup(DialString.c_str ()));

				//
				// If there is a reason for the UI to take an action before
				// continuing the dialing process then send a message to the UI.
				// Otherwise, continue to the dialing process.
				//
				if (nCallResolution != SstiCallResolution::eUNKNOWN)
				{
					pCallResolution = new SstiCallResolution;

					pCallResolution->pCall = call.get();
					pCallResolution->nReason = nCallResolution;

					// Alert the application
					AppNotify (estiMSG_DIRECTORY_RESOLVE_RESULT, (size_t)pCallResolution);
				}
				else
				{
					call->ContinueDial ();
				}

				delete pDialString;
				pDialString = nullptr;
			}
		}
	} // end if
	else
	{
		if (call->directSignMailGet())
		{
			call->SignMailInitiatedSet (false);
			call->ResultSet (estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE);
			call->HangUp ();
		}
		else
		// Check to see if we should spawn a new relay call.
		if (NewRelayCallAllowed ())
		{
			SpawnNewCall (SpawnCallNumber);
		}
		else
		{
			// If we have a call in the connected state we want to place it on hold before making this call.
			ConnectedCallHold ();
			
			if (!SpawnCallNumber.empty ())
			{
				// If we have a SpawnCallNumber we did not create a new call object for this directory resolve.
				// We need to create a new call object before proceeding.
				call = m_pConferenceManager->outgoingCallConstruct (SpawnCallNumber, estiSUBSTATE_RESOLVE_NAME, false);
			}
			
			// No.	Was it because it wasn't in the database or was it
			// an error?
			// Was it missing from the database?
			if (CstiCoreResponse::ePHONE_NUMBER_NOT_FOUND == poResponse->ErrorGet ())
			{
				//
				// If this is currently an unknown dial type then, since the number
				// wasn't found in core, this must be a VRS call.  Set it up to be so.
				//
				if (m_bDeterminingDialMethod)
				{
					if (m_mustCallCIR)
					{
						eCallResult = estiRESULT_VRS_CALL_NOT_ALLOWED;
						stiTHROW (stiRESULT_ERROR);
					}

					// This is a VRS call, so in general Hearing mode should not allow it.
					//  However, if a hearing mode phone transfers a customer mode phone to VRS,
					//  that is permitted.  The code below (which is a bit confusing) checks
					//  for and permits a hearing mode vp to transfer another vp to VRS,
					//  but disallows itself from being transferred to VRS.
					if (m_eLocalInterfaceMode == estiHEARING_MODE				// I'm hearing mode and ...
						&& (call->StateGet () == esmiCS_TRANSFERRING  		// I'm being transfered to VRS (not allowed), or
							|| call->StateGet () != esmiCS_INIT_TRANSFER))	// I placed a VRS call, I'm not transferring (also not allowed)
					{
						eCallResult = estiRESULT_VRS_CALL_NOT_ALLOWED;			// User is in hearing mode and did not initiate a transfer

						stiTHROW (stiRESULT_ERROR);
					}

					if (m_eLocalInterfaceMode == estiPUBLIC_MODE
						&& call->StateGet () == esmiCS_TRANSFERRING)
					{
						// Don't allow a transfer to VRS when in Public mode
						//  NOTE: this is a work-around that may be fixed better in the future
						eCallResult = estiRESULT_VRS_CALL_NOT_ALLOWED;
						stiTHROW (stiRESULT_ERROR);
					}

					hResult = VRSCallCreate (call);
					stiTESTRESULT ();
				}
				else
				{
					eCallResult = estiRESULT_NOT_FOUND_IN_DIRECTORY;

					stiTHROW (stiRESULT_ERROR);
				}
			} // end if
			else
			{
				//
				// If this was an unknown dial type then send a message to the UI
				// to ask the user if they would like to place a VRS call.
				//
				if (m_bDeterminingDialMethod)
				{
					pCallResolution = new SstiCallResolution;
					stiTESTCOND (pCallResolution, stiRESULT_ERROR);

					pCallResolution->pCall = call.get();
					pCallResolution->nReason = SstiCallResolution::eDIRECTORY_RESOLUTION_FAILED;

					AppNotify (estiMSG_DIRECTORY_RESOLVE_RESULT, (size_t)pCallResolution);
					
					stiASSERTMSG (false, "DirectoryResolve Error = %d", poResponse->ErrorGet ());
				}
				else
				{
					stiTHROW (stiRESULT_ERROR);
				}
			}
		}
	} // end else

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (call)
		{
			if (call->StateGet () == esmiCS_INIT_TRANSFER
				|| call->StateGet () == esmiCS_TRANSFERRING)
			{
				// If there are any issues preventing us from transferring the call
				// we should return to a CONNECTED state and notify the App if it
				// was initiating the transfer or hang up the call if we were performing
				// the transfer.
				if (call->StateGet () == esmiCS_TRANSFERRING)
				{
					call->StateSet (esmiCS_CONNECTED);
					call->ResultSet(estiRESULT_TRANSFER_FAILED);
					call->HangUp();
				}
				else
				{
					call->StateSet (esmiCS_CONNECTED);
					AppNotify (estiMSG_TRANSFER_FAILED, (size_t)call.get());
				}
			}
			else
			{
				// The entry find failed.  Set the call result to
				// directory find failed.
				call->ResultSet (eCallResult);

				// Hangup the call.
				call->HangUp ();
			}
		}
		else
		{
			m_pVRCLServer->callDialError ();
		}
	}

	return (hResult);
} // end CstiCCI::DirectoryResolveReturn


/*!\brief Turns this call into a VRS call, passing in the original dial string
 *
 *  \retval stiHResult
 */
stiHResult CstiCCI::VRSCallCreate (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	EstiDialMethod eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
	std::string fullDialString;
	std::string OriginalDialString;

	if (call->StateGet () != esmiCS_INIT_TRANSFER
		&& call->StateGet () != esmiCS_TRANSFERRING)
	{
		call->OriginalDialStringGet (&OriginalDialString);
	}
	else
	{
		call->TransferDialStringGet (&OriginalDialString);
	}

	OptString preferredLanguage;
	call->LocalPreferredLanguageGet (&preferredLanguage);
	hResult = FormatVRSDialString(OriginalDialString, preferredLanguage, &fullDialString);
	
	call->VRSFailOverServerSet (m_vrsFailoverIP);
	
	stiTESTRESULT();

	{
		//
		// If this was a VCO call then
		// set the dial type appropriately.
		//
		if (m_bPossibleVCOCall)
		{
			eDialMethod = estiDM_BY_VRS_WITH_VCO;
		}

		if (call->StateGet () != esmiCS_INIT_TRANSFER
			&& call->StateGet () != esmiCS_TRANSFERRING)
		{
			//
			// Setup all the appropriate dial strings
			//
			call->OriginalDialMethodSet(eDialMethod);
			call->DialMethodSet (eDialMethod);
			call->RemoteDialStringSet (fullDialString);
		}

		CstiRoutingAddress RoutingAddress (fullDialString);
		call->RoutingAddressSet (RoutingAddress);
		if (m_bReportDialMethodDetermination)
		{
			auto pCallResolution = new SstiCallResolution;
			stiTESTCOND (pCallResolution, stiRESULT_ERROR);

			pCallResolution->pCall = call.get();
			pCallResolution->nReason = SstiCallResolution::eDIAL_METHOD_DETERMINED;
			
			if (call->vrsPromptRequiredGet ())
			{
				pCallResolution->nReason |= SstiCallResolution::eDIRECTORY_RESOLUTION_FAILED;
			}

			AppNotify (estiMSG_DIRECTORY_RESOLVE_RESULT, (size_t)pCallResolution);
		}
		else
		{
			hResult = call->ContinueDial ();
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Creates or places a VRS call
 *
 *  \retval stiHResult
 */
stiHResult CstiCCI::VrsCallCreate (
	EstiDialMethod eDialMethod,
	const CstiRoutingAddress &RoutingAddress,
	const std::string &dialString,
	const OptString &callListName,
	const OptString &relayLanguage,
	CstiCallSP *ppoCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;

	if (m_bReportDialMethodDetermination)
	{
		//
		// Create a call object without actually placing the call
		//
		call = m_pConferenceManager->outgoingCallConstruct (dialString, estiSUBSTATE_RESOLVE_NAME, false);
		stiTESTCOND(call, stiRESULT_ERROR);

		//
		// Now add to the call all the pertinent information to place the call
		// when the DialContinue method is called.
		//
		call->RoutingAddressSet (RoutingAddress);

		call->DialMethodSet (eDialMethod);
		call->OriginalDialMethodSet(eDialMethod);
		call->OriginalDialStringSet (eDialMethod, dialString);
		call->RemoteCallListNameSet (callListName.value_or (""));
		RelayLanguageSet (call.get (), relayLanguage);

		//
		// Inform the application layer that the dial method was determined.
		//
		auto pCallResolution = new SstiCallResolution;

		pCallResolution->pCall = call.get();
		pCallResolution->nReason = SstiCallResolution::eDIAL_METHOD_DETERMINED;

		// Alert the application
		AppNotify (estiMSG_DIRECTORY_RESOLVE_RESULT, (size_t)pCallResolution);
	}
	else
	{
		// Tell the conference manager to place the call.
		call = m_pConferenceManager->CallDial (
					eDialMethod,
					RoutingAddress,
					dialString,
					nullptr,
					callListName);
		stiTESTCOND(call, stiRESULT_ERROR);
	}

	*ppoCall = call;

STI_BAIL:

	return hResult;
}



void CstiCCI::vrsCallInfoUpdate (CstiCallSP call)
{
	if (!call->vrsCallIdGet ().empty () && !call->vrsAgentIdGet ().empty ())
	{
		auto callListType = call->DirectionGet () == estiINCOMING ? CstiCallList::eANSWERED : CstiCallList::eDIALED;

		auto callHistoryItem = m_pCallHistoryManager->itemGetByCallIndex (call->CallIndexGet (), callListType);
		if (callHistoryItem != nullptr)
		{
			if (callHistoryItem->vrsCallIdGet ().empty ())
			{
				callHistoryItem->vrsCallIdSet (call->vrsCallIdGet ());
			}

			// VRS call ID and at least one agent history item are required.  Also, only report it if there's a new agent ID.
			// The VSR call ID must match. If it has changed, then Relay will have created a new call history item. Relay creates
			// call history when a new call is spawned so it is up to Relay to create agent history for that call history item.
			if (callHistoryItem->vrsCallIdGet () == call->vrsCallIdGet () &&
				(callHistoryItem->m_agentHistory.empty () || callHistoryItem->m_agentHistory.front ().m_agentId != call->vrsAgentIdGet ()))
			{
				callHistoryItem->m_agentHistory.emplace_front (call->vrsAgentIdGet ());
				auto coreRequest = new CstiCoreRequest ();
				coreRequest->vrsAgentHistoryAdd (*callHistoryItem->CoreItemCreate (false), call->DirectionGet ());
				m_pCallHistoryManager->CoreRequestSend (coreRequest, nullptr, callListType);
			}
		}
	}
}


//!\brief Terminates the call associated to a removed request
//
// This function finds the call associated to the removed
// request and terminates the call that is in process.
//
stiHResult CstiCCI::DirectoryResolveRemoved(
	CstiVPServiceResponse *response)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCallResolution *pCallResolution = nullptr;


	auto call = FindCallByRequestId(response->RequestIDGet());
	stiTESTCOND(call, stiRESULT_ERROR);

	//
	// If this was an unknown dial type then send a message to the UI
	// to ask the user if they would like to place a VRS call.
	//
	if (m_bDeterminingDialMethod)
	{
		pCallResolution = new SstiCallResolution;
		stiTESTCOND (pCallResolution, stiRESULT_ERROR);

		pCallResolution->pCall = call.get();
		pCallResolution->nReason = SstiCallResolution::eDIRECTORY_RESOLUTION_FAILED;

		AppNotify (estiMSG_DIRECTORY_RESOLVE_RESULT, (size_t)pCallResolution);
	}
	else
	{
		// No longer looking for results; set the requset id to zero.
		call->AppDataSet (0);

		call->ResultSet (estiRESULT_DIRECTORY_FIND_FAILED);

		// Hangup the call.
		call->HangUp ();
	}

STI_BAIL:

	return (hResult);
}

//:-----------------------------------------------------------------------------
//: Conference Engine Functions
//:-----------------------------------------------------------------------------


//
// Callback function for core to notify CCI
// of the request id for the asynchronously sent request.
//
stiHResult CstiCCI::CoreRequestAsyncCallback (
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t CallbackFromId)
{
	auto pThis = (CstiCCI *)CallbackParam;

	if (n32Message == estiMSG_CORE_ASYNC_REQUEST_ID)
	{
		std::lock_guard<std::recursive_mutex> lock(pThis->m_LockMutex);

		pThis->m_PendingCoreRequests.insert (MessageParam);
	}

	return stiRESULT_SUCCESS;
}


/*!\brief Start the phone ringing
 *
 */
void CstiCCI::IncomingRingStart ()
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::RingStart);

	//
	// Don't ring the audible ring if it is muted or if we are already in a call.
	//
	if (!m_bAudibleRingerMuted
	 && 0 == m_pConferenceManager->CallObjectsCountGet (esmiCS_CONNECTED
														 | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT
														 | esmiCS_HOLD_BOTH))
	{
		IstiAudibleRinger::InstanceGet ()->Start ();
	} // end if

	IstiPlatform::InstanceGet ()->ringStart ();
} // end CstiProtocolManager::RingStart


/*!\brief This methood loads the video greeting.
 *
 * @return Sucess or failure code
 */
stiHResult CstiCCI::LoadVideoGreeting(
	CstiCallSP call,
	const EstiBool bPause)
{
	char szServerIP[un8stiMAX_URL_LENGTH + 1];
	char szServerPort[20];
	char szFileUrl[un16stiMAX_URL_LENGTH + 1];
	char szServerName[un8stiMAX_URL_LENGTH + 1];

	stiHResult hResult = stiRESULT_SUCCESS;

	if (!m_bSignMailGreetingLoaded)
	{
		auto messageGreetingUrl = call->MessageGreetingURLGet();

		if (!messageGreetingUrl.empty () &&
			eURL_OK == stiURLParse (messageGreetingUrl.c_str (),
									szServerIP, sizeof (szServerIP),
									szServerPort, sizeof (szServerPort),
									szFileUrl, sizeof (szFileUrl),
									szServerName, sizeof (szServerName)))
		{
			// Make sure the Greeting starts at the begining.
			m_pFilePlay->SetStartPosition(0);

			hResult = m_pFilePlay->LoadVideoMessage (szServerName,
										szFileUrl,
										"",
										0,
										"",
										bPause);
			stiTESTRESULT ();

			m_bSignMailGreetingLoaded = true;
		}
		else
		{
			stiASSERT(estiTRUE);
			m_pFilePlay->LogFileInitializeError (IstiMessageViewer::estiERROR_PARSING_GUID);

			stiTHROW (stiRESULT_ERROR);
		}
	}
	else
	{
		// Unpause the video.
		m_pFilePlay->Play();
	}

STI_BAIL:
	return hResult;
}

/*!\brief Starts the record message process
 *
 */
stiHResult CstiCCI::RecordMessageStart (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

	//
	// If m_pLeaveMessage is not NULL then something went wrong.
	//
	if (!m_spLeaveMessageCall)
	{
		//
		// Make sure the pointer to the call object is valid.
		//
		stiTESTCOND (call, stiRESULT_ERROR);
		stiTESTCOND (call->SubstateValidate(estiSUBSTATE_LEAVE_MESSAGE), stiRESULT_ERROR);

		// Save off the shared pointer call object
		m_spLeaveMessageCall = call;

		// Clear the SignMail Text variables.
		m_signMailText.clear();
		m_signMailTextStartSeconds = -1;
		m_signMailTextEndSeconds = -1;

		//
		// Ensure that any previous information is cleared out Bug #22529.
		// Calling SignMailInfoClear here instead of RecordP2PMessageInfoClear
		// to prevent changing state causing bug #23988.
		// 
		m_pFilePlay->SignMailInfoClear ();

		m_pFilePlay->signMailCallSet(std::static_pointer_cast<IstiCall>(m_spLeaveMessageCall));

		// Set the recordtype so that we can record on a greeting error.
		// *** This code needs to be removed on the rework of CCI and FilePlayer***
		m_pFilePlay->RecordTypeSet(IstiMessageViewer::estiRECORDING_UPLOAD_URL);
		

		if (m_spLeaveMessageCall->LeaveMessageGet() == estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD)
		{
			SstiMessageInfo stMessageInfo;

			// If we are a Text only greeting then we need to cleanup the timer.
			if (m_spLeaveMessageCall->MessageGreetingTypeGet() == eGREETING_TEXT_ONLY)
			{
				m_textOnlyGreetingTimer.stop ();
			}

			hResult = call->MessageInfoGet(&stMessageInfo);
			if (!stiIS_ERROR (hResult) && stMessageInfo.bMailBoxFull == estiFALSE)
			{
				RequestMessageUploadGUID();
				m_pFilePlay->LoadCountdownVideo();
			}
			else
			{
				RecordMessageMailBoxFull(call);
			}
		}
		else if (m_spLeaveMessageCall->LeaveMessageGet() == estiSM_LEAVE_MESSAGE_SKIP_GREETING ||
				 m_spLeaveMessageCall->MessageGreetingURLGet().empty ())
		{
			RequestMessageUploadGUID();
		}
		else if (m_spLeaveMessageCall->MessageGreetingTypeGet() == eGREETING_TEXT_ONLY)
		{
			// We don't have a video to show so start the timer to run for 30 seconds.
			m_textOnlyGreetingTimer.restart ();
			RequestMessageUploadGUID();
		}
		else
		{
			hResult = LoadVideoGreeting(m_spLeaveMessageCall);
			stiTESTRESULT ();
			RequestMessageUploadGUID();
		}
	}
	else
	{
		//
		// Somehow we entered this method with a non-NULL m_spLeaveMessageCall pointer.
		//
		stiASSERT (estiFALSE);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (m_spLeaveMessageCall)
		{
			// Release the call object if we found one.
			m_spLeaveMessageCall = nullptr;
			m_pFilePlay->signMailCallSet(nullptr);
		}
	}

	return hResult;
}

stiHResult CstiCCI::SignMailStart (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = RecordMessageStart (call);
	
	if (!stiIS_ERROR (hResult))
	{
		hResult = AppNotify(estiMSG_LEAVE_MESSAGE, (size_t)call.get());
		m_pVRCLServer->VRCLNotify (estiMSG_LEAVE_MESSAGE, call);
	}
	else
	{
		//
		// Something went wrong... change the call to a non signmail state and drop the call with
		// a "system unreachable" result.
		//
		call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
		call->HangUp();
	}
	
	return hResult;
}


/*!\brief Translates an ICE result into a human readable string.
 *
 * \param eICENominationsResult The ICE result to translate.
 */
const char *ICENominationsResultStringGet (
	EstiICENominationsResult eICENominationsResult)
{
	const char *pszICENominationsResult = "Unknown";

	switch (eICENominationsResult)
	{
		case eICENominationsUnknown:

			pszICENominationsResult = "Unknown";

			break;

		case eRemoteNotICE:

			pszICENominationsResult = "NotICE";
			break;

		case eICENominationsSucceeded:

			pszICENominationsResult = "Succeeded";
			break;

		case eICENominationsFailed:

			pszICENominationsResult = "Failed";
			break;
	}

	return pszICENominationsResult;
}


/*!\brief Translates an ICE support values into a human readable string.
 *
 * \param iceSupport The ICE support value to translate.
 */
static std::string RemoteIceSupportStringGet (
	IceSupport iceSupport)
{
	switch (iceSupport)
	{
		case IceSupport::Unknown:

			return "Unknown";

		case IceSupport::Supported:

			return "Supported";

		case IceSupport::NotSupported:

			return "NotSupported";

		case IceSupport::Mismatch:

			return "Mismatch";

		case IceSupport::ReportedMismatch:

			return "ReportedMismatch";

		case IceSupport::TrickleIceMismatch:

			return "TrickleIceMismatch";

		case IceSupport::Error:

			return "Error";

		case IceSupport::Unspecified:

			return "Unspecified";
	}

	return "Unknown";
}


/*!\brief Translates a media transport type into a human readable string.
 *
 * \param eTransportType The transport type to translate.
 */
const char *MediaTransportStringGet (
	EstiMediaTransportType eTransportType)
{
	const char *pszTransportType = "Unknown";

	switch (eTransportType)
	{
		case estiMediaTransportUnknown:
			pszTransportType = "Unknown";
			break;
		case estiMediaTransportHost:
			pszTransportType = "Host";
			break;
		case estiMediaTransportReflexive:
			pszTransportType = "Reflexive";
			break;
		case estiMediaTransportRelayed:
			pszTransportType = "Relayed";
			break;
		case estiMediaTransportPeerReflexive:
			pszTransportType = "PeerReflexive";
			break;
	}

	return pszTransportType;
}


/*!\brief Collects and sends call statistics to core
 *
 * This method collects various stats about the call and
 * sends them to core in a property.  Short, two letter codes
 * are used to indicate the collected stat to save space in the
 * string:
 *
 * CR: Call Result
 * DS: Dialstring
 * CP: Conferencing Protocol
 * CD: Call Duration
 * DI: Call Direction
 *
 * Other status codes that are not implemented yet are defined in a seperate document.
 *
 * \param call - The call to collect the stats from.
 */
void CstiCCI::CallStatusSend (
	CstiCallSP call,
	CstiSipCallLegSP callLeg)
{
	std::stringstream CallStatus;
	std::string DialString;
	const uint32_t highPacketCount = 1000000000;

	if (call)
	{
		CallStatus << "\nCall/Video Stats:\n";

		SstiCallStatistics stCallStatistics;
		call->StatisticsGet(&stCallStatistics);

		//
		// Get and Set the call ID
		//
		std::string callId;
		call->CallIdGet(&callId);
		CallStatus << "\tCallID = \"" << callId << "\"\n";

		if (call->DirectionGet () == estiINCOMING)
		{
			CallStatus << "\tInitialInviteAddress = " << call->InitialInviteAddressGet () << "\n";
		}
		else
		{
			CallStatus << "\tUri = \"" << call->UriGet () << "\"\n";
		}
		
		// Log the address we are sending to.
		CallStatus << "\tSIPDestAddr = " << call->remoteDestGet () << "\n";

		//
		// Set the call result
		//
		CallStatus << "\tCallResult = " << call->ResultGet () << "\n";
		
		if (call->ForcedVRSFailoverGet ())
		{
			CallStatus << "\tVRSFailOver = 1\n";
		}

		//
		// Get the dial string.
		// The remote dial string is the original dial string for an
		// outbound call and the remote dial string for an inbound call.
		//
		call->RemoteDialStringGet (&DialString);

		CallStatus << "\tDialString = " << DialString << "\n";

		//
		// Is the call with a contact
		//
		IstiContactSharedPtr contact;
		CallStatus << "\tIsContact = " << (m_pContactManager->getContactByPhoneNumber (DialString, &contact) ? "1" : "0") << "\n";

		//
		// Set the dial method
		//
		CallStatus << "\tDialMethod = " << call->DialMethodGet () << "\n";

		//
		// Set the connecting duration
		//
		CallStatus << "\tConnectingDuration = " << call->ConnectingDurationGet ().count () / 1000.0 << "\n";

		CallStatus << "\tCallDuration = " << stCallStatistics.dCallDuration << "\n";

		//
		// Set call direction
		//
		CallStatus << "\tCallDirection = " << call->DirectionGet () << "\n";

		//
		// Dial source
		//
		CallStatus << "\tDialSource = " << call->DialSourceGet () << "\n";

		//
		// Language
		//
		OptString preferredLanguage;
		call->LocalPreferredLanguageGet (&preferredLanguage);
		CallStatus << "\tSVRSLanguage = " << preferredLanguage << "\n";
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4 || \
		defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_ANDROID

		std::string uiLanguage;
		m_poPM->getPropertyString (UI_LANGUAGE, &uiLanguage, "en-us", PropertyManager::Persistent);
		CallStatus << "\tUILanguage = " << uiLanguage << "\n";
#endif
		CallStatus << "\tRemoteCallerIdBlocked = " << call->RemoteCallerIdBlockedGet () << "\n";
		CallStatus << "\tLocalCallerIdBlocked = " << call->LocalCallerIdBlockedGet () << "\n";

		//
		// Is the remote endpoint a Sorenson endpoint?
		// If the call object ends with a success, local reject, local busy or lost connection
		// we should have enough information to determine if the remote endpoint is a Sorenson or
		// a non-Sorenson endpoint.  If the call result is any other value then we likely don't have
		// the information so we will declare that the remote endpoint type is unknown.
		//
		enum
		{
			estiRemoteEPUnknown = 0,
			estiRemoteEPSorenson = 1,
			estiRemoteEPNonSorenson = 2
		};

		if (call->ConnectedWithMcuGet (estiMCU_GVC))
		{
			// Log that we were connected with a Group Video Chat room.
			std::string userID;
			const ICallInfo *pCallInfo = nullptr;
			pCallInfo = call->LocalCallInfoGet ();
			pCallInfo->UserIDGet (&userID);

			std::string ConferenceId;
			call->ConferencePublicIdGet (&ConferenceId);
			CallStatus << "\tGVCConferenceId = " << ConferenceId << "\n";
			CallStatus << "\tCoreId = " << userID << "\n";
		}

		if (callLeg != nullptr)
		{
			std::string RemoteProductName;
			callLeg->RemoteProductNameGet (&RemoteProductName);
			CallStatus << "\tRemoteProductName = \"" << RemoteProductName << "\"\n";
			
			std::string RemoteProductVersion;
			callLeg->RemoteProductVersionGet (&RemoteProductVersion);
			CallStatus << "\tRemoteProductVersion = " << RemoteProductVersion << "\n";
			
			if (!callLeg->vrsFocusedRoutingSentGet().empty())
			{
				CallStatus << "\tVrsFocusedRoutingSent = " << callLeg->vrsFocusedRoutingSentGet() << "\n";
			}
			
			if (!callLeg->vrsFocusedRoutingReceivedGet().empty())
			{
				CallStatus << "\tVrsFocusedRoutingReceived = " << callLeg->vrsFocusedRoutingReceivedGet() << "\n";
			}
		}

#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC
		// Device Model
		std::string deviceModel;
		m_poPM->propertyGet("DeviceModel", &deviceModel);
		CallStatus << "\tDeviceModel = \"" << deviceModel << "\"\n";

		// Managed Device
		int nManagedDevice = 0;
		m_poPM->propertyGet (MANAGED_DEVICE, &nManagedDevice);
		if (nManagedDevice == 1)
		{
			CallStatus << "\tManagedDevice = 1\n";
		}

		// System Version
		std::string systemVersion;
		m_poPM->propertyGet("SystemVersion", &systemVersion);
		CallStatus << "\tSystemVersion = " << systemVersion << "\n";
#endif

		CallStatus << "\tOfflineMode = " << isOffline () << "\n";
		
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_ANDROID
		// Telecom Carrier
		std::string telecomCarrier;
		m_poPM->propertyGet("TelecomCarrier", &telecomCarrier);
		CallStatus << "\tTelecomCarrier = " << telecomCarrier << "\n";
#endif
		
		// Public IP
		SstiConferenceParams stConferenceParams;
		stiHResult hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		if (!stiIS_ERROR (hResult))
		{
			CallStatus << "\tPublicIP = " << stConferenceParams.PublicIPv4 << "\n";
			CallStatus << "\tNatTraversalSIP = " << stConferenceParams.eNatTraversalSIP << "\n";
		}
		
		CallStatus << "\tTunneled = " << stCallStatistics.bTunneled << "\n";

		EstiProtocol eProtocol = call->ProtocolGet ();

		CallStatus << "\tProtocol = " << (estiSIP == eProtocol ? "SIP" : "Unknown") << "\n";

		if (call->ProtocolModifiedGet ())
		{
			CallStatus << "\tForcedSIP = 1\n";
		}

		//
		// Log the SignMail duration if SignMail was initiated.
		// If the message was not sent then the duration will be zero.
		//
		if (call->SignMailInitiatedGet())
		{
			CallStatus << "\tSignMailDuration = " << call->SignMailDurationGet() << "\n";
		}
		
		if (call->unregisteredCallGet ())
		{
			CallStatus << "\tUnregisteredCall = 1\n";
		}

		CallStatus << "\tVcoPreference = " << stConferenceParams.ePreferredVCOType << "\n";

		CallStatus << "\tVcoActive = " << call->LocalIsVcoActiveGet () << "\n";
		
		//
		// Encryption reporting
		//
		CallStatus << "\tSecureCallMode = " << SecureCallModeGet () << "\n";
		//
		// Platform specific information
		//
		IstiPlatform::InstanceGet ()->callStatusAdd (CallStatus);
		
		if (callLeg && callLeg->ConferencedGet ())
		{
			// Add preferred display size stats to call stats.
			std::string displaySizeStats;
			call->PreferredDisplaySizeStatsGet(&displaySizeStats);
			CallStatus << displaySizeStats;
			//
			// Received Video Stats (playback)
			//
			CallStatus << "\nReceived Video Stats:\n";
			CallStatus << "\tVideoPacketQueueEmptyErrors = " << stCallStatistics.videoPacketQueueEmptyErrors << "\n";
			CallStatus << "\tVideoPacketsRead = " << stCallStatistics.videoPacketsRead << "\n";
			CallStatus << "\tVideoKeepAlivePackets = " << stCallStatistics.videoKeepAlivePackets << "\n";
			CallStatus << "\tVideoPayloadTypeErrors = " << stCallStatistics.videoUnknownPayloadTypeErrors << "\n";
			CallStatus << "\tVideoPacketsDiscardedReadMuted = " << stCallStatistics.videoPacketsDiscardedReadMuted << "\n";
			CallStatus << "\tVideoPacketsDiscardedPlaybackMuted = " << stCallStatistics.videoPacketsDiscardedPlaybackMuted << "\n";
			CallStatus << "\tVideoPacketsDiscardedEmpty = " << stCallStatistics.videoPacketsDiscardedEmpty << "\n";
			CallStatus << "\tVideoPacketsUsingPreviousSSRC = " << stCallStatistics.videoPacketsUsingPreviousSSRC << "\n";
			CallStatus << "\tTotalPacketsReceived = " << stCallStatistics.Playback.totalPacketsReceived << "\n";
			CallStatus << "\tReceivedPacketLoss = " <<  stCallStatistics.fReceivedPacketsLostPercent << "\n";
			CallStatus << "\tActualVideoKbpsRecv = " <<  stCallStatistics.Playback.nActualVideoDataRate << "\n";
			CallStatus << "\tActualFPSRecv = " << stCallStatistics.Playback.nActualFrameRate << "\n";
			CallStatus << "\tVideoFrameSizeRecv = \"" << stCallStatistics.Playback.nVideoWidth << " x " << stCallStatistics.Playback.nVideoHeight << "\"\n";
			CallStatus << "\tVideoCodecRecv = " << VideoCodecToString (stCallStatistics.Playback.eVideoCodec) << "\n";
			CallStatus << "\tVideoTimeMutedByRemote = " << stCallStatistics.videoTimeMutedByRemote.count () / 1000.0 << "\n";
			CallStatus << "\tTotalFramesAssembledAndSentToDecoder = " <<  stCallStatistics.un32FrameCount << "\n";
			
			CallStatus << "\tVideoFramesSentToPlatform = " << stCallStatistics.videoFramesSentToPlatform << "\n";
			CallStatus << "\tVideoPartialKeyFramesSentToPlatform = " << stCallStatistics.videoPartialKeyFramesSentToPlatform << "\n";
			CallStatus << "\tVideoPartialNonKeyFramesSentToPlatform = " << stCallStatistics.videoPartialNonKeyFramesSentToPlatform << "\n";
			CallStatus << "\tVideoWholeKeyFramesSentToPlatform = " << stCallStatistics.videoWholeKeyFramesSentToPlatform << "\n";
			CallStatus << "\tVideoWholeNonKeyFramesSentToPlatform = " << stCallStatistics.videoWholeNonKeyFramesSentToPlatform << "\n";
			
			CallStatus << "\tLostPackets = "  << stCallStatistics.Playback.totalPacketsLost << "\n";
			CallStatus << "\tHighestPacketLossSeen = " << stCallStatistics.highestPacketLossSeen << "\n";
			CallStatus << "\tNumberOfTimesPacketLossReported = " << stCallStatistics.countOfCallsToPacketLossCountAdd << "\n";
			CallStatus << "\tDiscardedFromBurst = " << stCallStatistics.burstPacketsDropped << "\n";
			CallStatus << "\tVideoPlaybackFrameGetErrors = " << stCallStatistics.un32VideoPlaybackFrameGetErrors << "\n";
			CallStatus << "\tFirNegotiated = " << call->FirNegotiatedGet () << "\n";
			CallStatus << "\tPliNegotiated = " << call->PliNegotiatedGet () << "\n";
			CallStatus << "\tKeyframesRequested = " << stCallStatistics.un32KeyframesRequestedVP << "\n";
			CallStatus << "\tKeyframesReceived = " << stCallStatistics.un32KeyframesReceived <<	"\n";
			CallStatus << "\tKeyframeWaitTimeMax = " << stCallStatistics.un32KeyframeMaxWaitVP << "\n";
			CallStatus << "\tKeyframeWaitTimeMin = " << stCallStatistics.un32KeyframeMinWaitVP << "\n";
			CallStatus << "\tKeyframeWaitTimeAvg = " << stCallStatistics.un32KeyframeAvgWaitVP << "\n";
			CallStatus << "\tKeyframeWaitTimeTotal = " << stCallStatistics.fKeyframeTotalWaitVP << "\n";
			CallStatus << "\tErrorConcealmentEvents = " << stCallStatistics.un32ErrorConcealmentEvents << "\n";
			CallStatus << "\tI-FramesDiscardedAsIncomplete = " << stCallStatistics.un32IFramesDiscardedIncomplete << "\n";
			CallStatus << "\tP-FramesDiscardedAsIncomplete = " << stCallStatistics.un32PFramesDiscardedIncomplete << "\n";
			CallStatus << "\tGapsInFrameNumbers = " << stCallStatistics.un32GapsInFrameNumber << "\n";
			
			//
			// NACK and RTX Stats
			//
			CallStatus << "\nNACK RTX Stats:\n";
			CallStatus << "\tNackRtxVideoNegotiated = " << call->NackRtxNegotiatedGet () << "\n";
			CallStatus << "\tActualVPPacketsLost = " << stCallStatistics.actualVPPacketLoss << "\n";
			CallStatus << "\tNackVideoRequestsSent = " << stCallStatistics.Playback.totalNackRequestsSent << "\n";
			CallStatus << "\tNackVideoRequestsReceived = " << stCallStatistics.Record.totalNackRequestsReceived << "\n";
			CallStatus << "\tRtxVideoRequestsIgnored = " << stCallStatistics.rtxPacketsIgnored << "\n";
			CallStatus << "\tRtxVideoPacketsReceived = " << stCallStatistics.Playback.totalRtxPacketsReceived << "\n";
			CallStatus << "\tRtxVideoPacketsSent = " << stCallStatistics.Record.totalRtxPacketsSent << "\n";
			CallStatus << "\tRtxVideoPacketsNotFound = " << stCallStatistics.rtxPacketsNotFound << "\n";
			CallStatus << "\tVideoPlaybackDelay = " << stCallStatistics.avgPlaybackDelay << "\n";
			CallStatus << "\tActualRtxVideoKbpsSent = " << stCallStatistics.rtxKbpsSent << "\n";
			CallStatus << "\tNumberOfDuplicatePackets = " << stCallStatistics.duplicatePacketsReceived << "\n";
			CallStatus << "\tNackDueToDelay = " << stCallStatistics.rtxFromNoData << "\n";
			CallStatus << "\tAvgRTT = " << stCallStatistics.avgRtcpRTT << "\n";
			CallStatus << "\tAvgJitter = " << stCallStatistics.avgRtcpJitter << "\n";
			
			if (stCallStatistics.actualVPPacketLoss > highPacketCount ||
				stCallStatistics.burstPacketsDropped > highPacketCount)
			{
				CallStatus << "\tPossibleStatsError = 1\n";
			}
			
			//
			// Sent Video Stats (record)
			//
			CallStatus << "\nSent Video Stats:\n";
			CallStatus << "\tVideoPacketsSent = " << stCallStatistics.videoPacketsSent << "\n";
			CallStatus << "\tVideoPacketSendErrors = " << stCallStatistics.videoPacketSendErrors << "\n";
			CallStatus << "\tTargetVideoKbps = "  <<  stCallStatistics.Record.nTargetVideoDataRate << "\n";
			CallStatus << "\tActualVideoKbpsSent = " << stCallStatistics.Record.nActualVideoDataRate << "\n";
			CallStatus << "\tTargetFPS = " << stCallStatistics.Record.nTargetFrameRate << "\n";
			CallStatus << "\tActualFPSSent = " << stCallStatistics.Record.nActualFrameRate << "\n";
			CallStatus << "\tVideoFrameSizeSent = \"" << stCallStatistics.Record.nVideoWidth << " x " << stCallStatistics.Record.nVideoHeight << "\"\n";
			CallStatus << "\tVideoCodecSent = " << VideoCodecToString (stCallStatistics.Record.eVideoCodec) << "\n";
			CallStatus << "\tVRKeyframesRequested = " << stCallStatistics.un32KeyframesRequestedVR << "\n";
			CallStatus << "\tVRKeyframeWaitTimeMax = " << stCallStatistics.un32KeyframeMaxWaitVR << "\n";
			CallStatus << "\tVRKeyframeWaitTimeMin = " << stCallStatistics.un32KeyframeMinWaitVR << "\n";
			CallStatus << "\tVRKeyframeWaitTimeAvg = " << stCallStatistics.un32KeyframeAvgWaitVR << "\n";
			CallStatus << "\tKeyframesSent = " << stCallStatistics.un32KeyframesSent << "\n";
			CallStatus << "\tFramesLostFromRCU = " << stCallStatistics.un32FramesLostFromRcu << "\n";
			CallStatus << "\tPartialFramesSent = " << stCallStatistics.countOfPartialFramesSent << "\n";
			CallStatus << "\tEncryption = " << static_cast<int> (callLeg->encryptionStateVideoGet ()) << "\n";
			CallStatus << "\tKeyExchangeMethod = " << static_cast<int> (callLeg->keyExchangeMethodVideoGet ()) << "\n";
			CallStatus << "\tEncryptionChanges = " << callLeg->encryptionChangesVideoToJsonArrayGet () << "\n";

			//
			// Sent Audio Stats
			//
			CallStatus << "\nSent Audio Stats:\n";
			CallStatus << "\tActualAudioCodecSent = " << AudioCodecToString (stCallStatistics.Record.audioCodec) << "\n";
			CallStatus << "\tAudioKbpsSent = " << stCallStatistics.Record.nActualAudioDataRate << "\n";
			CallStatus << "\tAudioPacketsSent = " << stCallStatistics.Record.numberAudioPackets << "\n";
			CallStatus << "\tEncryption = " << static_cast<int> (callLeg->encryptionStateAudioGet ()) << "\n";
			CallStatus << "\tKeyExchangeMethod = " << static_cast<int> (callLeg->keyExchangeMethodAudioGet ()) << "\n";
			CallStatus << "\tEncryptionChanges = " << callLeg->encryptionChangesAudioToJsonArrayGet () << "\n";

			//
			// Received Audio Stats
			//
			CallStatus << "\nReceived Audio Stats:\n";
			CallStatus << "\tAudioCodecReceived = " << AudioCodecToString (stCallStatistics.Playback.audioCodec) << "\n";
			CallStatus << "\tActualAudioKbpsReceived = " << stCallStatistics.Playback.nActualAudioDataRate << "\n";
			CallStatus << "\tAudioPacketsReceived = " << stCallStatistics.Playback.numberAudioPackets << "\n";
			CallStatus << "\tAudioPacketsLost = " << stCallStatistics.Playback.audioPacketsLost << "\n";
			
			//
			// Share text char count stats (sent and received)
			//
			CallStatus << "\nShare Text Stats:\n";
			CallStatus << "\tShareTextCharsSent = " << stCallStatistics.un32ShareTextCharsSent << "\n";
			CallStatus << "\tShareTextCharsReceived = " << stCallStatistics.un32ShareTextCharsReceived << "\n";
			CallStatus << "\tShareTextCharsSentFromSavedText = " << stCallStatistics.un32ShareTextCharsSentFromSavedText << "\n";
			CallStatus << "\tShareTextCharsSentFromKeyboard = " <<	stCallStatistics.un32ShareTextCharsSentFromKeyboard << "\n";
			CallStatus << "\tEncryption = " << static_cast<int> (callLeg->encryptionStateTextGet ()) << "\n";
			CallStatus << "\tKeyExchangeMethod = " << static_cast<int> (callLeg->keyExchangeMethodTextGet ()) << "\n";
			CallStatus << "\tEncryptionChanges = " << callLeg->encryptionChangesTextToJsonArrayGet () << "\n";

			//
			// Flow Control Stats
			//
			CallStatus << "\nFlow Control Information:\n";
			CallStatus << "\tAutoSpeedMode = " << AutoSpeedSettingGet() << "\n";
			CallStatus << "\tMinimumTargetSendSpeed = " << stCallStatistics.un32MinSendRate << "\n";
			CallStatus << "\tMaximumTargetSendSpeedWithAcceptableLoss = " << stCallStatistics.un32MaxSendRateWithAcceptableLoss << "\n";
			CallStatus << "\tMaximumTargetSendSpeed = " << stCallStatistics.un32MaxSendRate << "\n";
			CallStatus << "\tAverageTargetSendSpeedKbps = " << stCallStatistics.un32AveSendRate << "\n";
			CallStatus << "\tTmmbrNegotiated = " << call->TmmbrNegotiatedGet () << "\n";
			
			//
			// Log the ICE results
			//
			CallStatus << "\tICENominationsResult = " << ICENominationsResultStringGet (callLeg->iceNominationResultGet ()) << "\n";

			CallStatus << "\tRemoteIceSupport = " << RemoteIceSupportStringGet (callLeg->m_remoteIceSupport) << "\n";
		}

		//
		// Log the media routing information
		//
		CallStatus << "\nMedia Routing Information:\n";

		CallStatus << "\tRemoteVideoAddr = " << call->RemoteVideoAddrGet () << "\n";
		CallStatus << "\tVideoRemotePort = " << call->VideoRemotePortGet () << "\n";
		CallStatus << "\tAudioRemotePort = " << call->AudioRemotePortGet () << "\n";
		CallStatus << "\tTextRemotePort = " << call->TextRemotePortGet () << "\n";
		CallStatus << "\tLocalVideoAddr = " << call->LocalVideoAddrGet () << "\n";
		CallStatus << "\tVideoPlaybackPort = " << call->VideoPlaybackPortGet () << "\n";
		CallStatus << "\tAudioPlaybackPort = " << call->AudioPlaybackPortGet () << "\n";
		CallStatus << "\tTextPlaybackPort = " << call->TextPlaybackPortGet () << "\n";



		//
		// Log the media tranport types used during the call
		//
		EstiMediaTransportType eRtpTransportAudio;
		EstiMediaTransportType eRtcpTransportAudio;
		EstiMediaTransportType eRtpTransportText;
		EstiMediaTransportType eRtcpTransportText;
		EstiMediaTransportType eRtpTransportVideo;
		EstiMediaTransportType eRtcpTransportVideo;

		call->MediaTransportTypesGet (
										&eRtpTransportAudio, &eRtcpTransportAudio,
										&eRtpTransportText, &eRtcpTransportText,
										&eRtpTransportVideo, &eRtcpTransportVideo);

		CallStatus << "\tRtpTransportAudio = " << MediaTransportStringGet(eRtpTransportAudio) << "\n";
		CallStatus << "\tRtcpTransportAudio = " << MediaTransportStringGet(eRtcpTransportAudio) << "\n";
		CallStatus << "\tRtpTransportText = " << MediaTransportStringGet(eRtpTransportText) << "\n";
		CallStatus << "\tRtcpTransportText = " << MediaTransportStringGet(eRtcpTransportText) << "\n";
		CallStatus << "\tRtpTransportVideo = " << MediaTransportStringGet(eRtpTransportVideo) << "\n";
		CallStatus << "\tRtcpTransportVideo = " << MediaTransportStringGet(eRtcpTransportVideo) << "\n";

		//
		// Log geolocation information
		//
		if (call->geolocationRequestedGet ())
		{
			CallStatus << "GeolocationAvailable = " << (int)call->geolocationAvailableGet () << "\n";
		}
	}

	m_pRemoteLoggerService->RemoteCallStatsSend(CallStatus.str().c_str ());
}


/*!\brief This method removes the specified list of properties from the property table.
 *
 * @param pSettings The list of properties to remove
 * @param nNumberOfSettings The number of properties in the list
 */
void CstiCCI::PropertiesRemove (
	const std::vector<InitializationSetting> &settings)
{
	std::set<PstiPropertyUpdateMethod, classcomp> Methods;

	for (auto &setting : settings)
	{
		// When a property is removed we must check to see if it has changed value (i.e.
		// the current value is different from the default value. If it has changed
		// then call the associated property changed method
		auto result = m_poPM->removeProperty (setting.name);

		if (result == PM_RESULT_VALUE_CHANGED)
		{
			for (auto &method: m_propertyChangedMethods)
			{
				if (setting.name == method.pszPropertyName)
				{
					Methods.insert (method.pfnMethod);
				}
			}
		}
	}

	CallPropertyUpdateMethods(&Methods);
}

/*!\brief This method checks to see if a property exists and if it doesn't sends a request to core to see if it has it.
 *
 * @param pSettings The list of properties to check
 * @param nNumberOfSettings The number of properties in the list
 * @param nSettingsType The type of property (phone or user)
 * @param pbRequestSent A pointer to a boolean so the caller knows whether or not the request was sent
 * @return Sucess or failure code
 */
stiHResult CstiCCI::PropertiesCheck (
	const std::vector<InitializationSetting> &settings,
	int nSettingsType,
	bool *pbRequestSent)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto  poCoreRequest = new CstiCoreRequest ();
	int nResult;
	bool bSendRequest = false;

	for (auto &setting : settings)
	{
		//
		// Check to see if the property is in the property table.
		// If it is not there then request it from core.
		//
		std::string value;

		nResult = PropertyManager::getInstance()->propertyGet (setting.name,
															   &value,
															   PropertyManager::Persistent);

		if (PM_RESULT_NOT_FOUND == nResult)
		{
			if (nSettingsType == estiPTypePhone)
			{
				poCoreRequest->phoneSettingsGet (setting.name.c_str ());
			}
			else
			{
				poCoreRequest->userSettingsGet (setting.name.c_str ());
			}

			bSendRequest = true;
		}
	}

	if (bSendRequest)
	{
		CoreRequestSend (poCoreRequest, nullptr);

		if (pbRequestSent)
		{
			*pbRequestSent = true;
		}
	}
	else
	{
		// No! Free the memory allocated
		delete poCoreRequest;
		poCoreRequest = nullptr;
	} // end else

	return hResult;
}


std::string CstiCCI::MessageStringGet (
	int32_t n32Message)
{
	return stiMessageStringGet (n32Message);
}


/*!\brief Log transfer in core services.
 *
 * \param call - The call object that contains the transfer information to be logged.
 */
stiHResult CstiCCI::TransferLog (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiCoreRequest *poCoreRequest = nullptr;

	if (call->TransferLogTypeGet () != estiTRANSFER_LOG_TYPE_NONE)
	{
		//
		// Log transfer in Core
		//
		std::string FromDialString;
		std::string ToDialString;
		std::string OriginalCallID;
		std::string TransferCallID;

		call->TransferFromDialStringGet (&FromDialString);
		call->TransferToDialStringGet (&ToDialString);

		//
		// If this is the transferer then the original call ID is the
		// call ID of the current call.  The transferer does not have access
		// to the transfered call ID so that is left empty.
		// If this is the transferee the call IDs are retrieved normally.
		//
		if (call->TransferLogTypeGet () == estiTRANSFER_LOG_TYPE_TRANSFERER)
		{
			call->CallIdGet (&OriginalCallID);
		}
		else
		{
			call->CallIdGet (&TransferCallID);
			call->OriginalCallIDGet (&OriginalCallID);
		}

		poCoreRequest = new CstiCoreRequest ();

		poCoreRequest->RemoveUponCommunicationError () = estiTRUE;

		poCoreRequest->logCallTransfer (FromDialString.c_str (), ToDialString.c_str (),
										OriginalCallID.c_str (), TransferCallID.c_str (),
										call->TransferLogTypeGet () == estiTRANSFER_LOG_TYPE_TRANSFERER);

		hResult = m_pCoreService->RequestSendEx (poCoreRequest, nullptr);
		stiTESTRESULT ();

		poCoreRequest = nullptr;
	}

STI_BAIL:

	if (poCoreRequest)
	{
		delete poCoreRequest;
		poCoreRequest = nullptr;
	}

	//
	// Don't log this transfer again...
	//
	call->TransferLogTypeSet (estiTRANSFER_LOG_TYPE_NONE);

	return hResult;
}


void CstiCCI::ConferencingReadyStateSet (
	EstiConferencingReadyState eConferencingReadyState)
{
	if (m_eConferencingReadyState != eConferencingReadyState)
	{
		// This block executes anytime the state changes.
		m_eConferencingReadyState = eConferencingReadyState;
	}
	// This always executes.
	AppNotify (estiMSG_CONFERENCING_READY_STATE, (size_t)m_eConferencingReadyState);
}

EstiConferencingReadyState CstiCCI::ConferencingReadyStateGet ()
{
	return m_eConferencingReadyState;
}


/*!\brief Send a notification message to the application
 *
 *  \retval estiOK If the message was sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::AppNotify (
	int32_t n32Message,  	//!< Notification message ID.
	size_t MessageParam)	//!< Parameter to send.
{
	stiLOG_ENTRY_NAME (CstiCCI::AppNotify);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiCCIDebug,
		stiTrace ("AppNotify Message: %s (%d), 0x%x\n", MessageStringGet(n32Message).c_str (), n32Message, MessageParam);
	);

	//
	// Assert if this message is in the private message range.
	//
	stiASSERT (n32Message < estiMSG_NEXT);

	// Check for special cases.
	switch (n32Message)
	{
		case estiMSG_INTERFACE_MODE_CHANGED:
#ifdef stiMESSAGE_MANAGER
			m_pMessageManager->Refresh();
#endif
			break;

		default:
			break;
	} // end switch (n32Message)

	// If we have a valid message queue ID, post the message
	// If we don't send it to the app, we need to make sure that the poResponse
	// object has been freed.

	// Assign to a local, because another thread can set the member variable to
	// NULL between null check and invocation.
	PstiAppGenericCallback appNotifyCallback = m_fpAppNotifyCB;
	if (appNotifyCallback)
	{
		appNotifyCallback(n32Message, MessageParam, m_CallbackParam);
	}
	else
	{
		MessageCleanup(n32Message, MessageParam);
	}

	return (hResult);
} // end CstiCCI::AppNotify


//:-----------------------------------------------------------------------------
//: Utility and Helper Functions
//:-----------------------------------------------------------------------------


//:-----------------------------------------------------------------------------
//: Core Services Functions
//:-----------------------------------------------------------------------------


/*!\brief Calls Login() to log in a user.
 *
 *  This function should be called when Core Services has been connected.
 *
 * \param pszPhoneNumber The user's phone number.
 * \param pszPin         The user's pin.
 * \param pContext
 * \param punRequestId   The ID of the request.
 * \param pszLoginAs
 *
 * \return Success or failure result
 */
stiHResult CstiCCI::CoreLogin (
	const std::string &phoneNumber,
	const std::string &pin,
	const VPServiceRequestContextSharedPtr &context,
	unsigned int *punRequestId,
	const OptString &loginAs)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiCCI::CoreLogin);

	//
	// If the the videophone has been ported away from Sorenson, we need to
	// avoid communications with the Sorenson back end.
	//
	if (estiPORTED_MODE != m_eLocalInterfaceMode ||
		m_portingBack)
	{
		hResult = Login (phoneNumber.c_str (),
						 pin.c_str (),
						 context,
						 punRequestId,
						 loginAs.value_or(std::string ()).c_str ());
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
} // end CstiCCI::CoreLogin


/*!\brief Calls the CoreService to log in a user
 *
 *  This function should be called when Core Services has been connected.
 *
 * \param pszPhoneNumber The user's phone number.
 * \param pszPin         The user's pin.
 * \param pContext
 * \param punRequestId   The ID of the request.
 * \param pszLoginAs
 *
 * \return Success or failure result
 */
stiHResult CstiCCI::Login (
	const char *pszPhoneNumber,
	const char *pszPin,
	const VPServiceRequestContextSharedPtr &context,
	unsigned int *punRequestId,
	const char *pszLoginAs)
{

	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiCCI::Login);

	// Record the login start time so we can log time to register
	TimeGet (&m_stCoreLoginStartTime);
	m_unCoreLoginCount++;

	//Check for a registration match so we can start registering before we login.
	if (m_pRegistrationManager->CredentialsCheck ((*pszPhoneNumber), (*pszPin)))
	{
		m_pRegistrationManager->PhoneNumbersGet (&m_sUserPhoneNumbers);
		m_pConferenceManager->UserPhoneNumbersSet (&m_sUserPhoneNumbers);
		RegistrationConferenceParamsSet ();
	}
	else
	{
		// Since we are not going to use the cached registration information clear it.
		// This will ensure we register after the RegistrationInfoGetResult is received.
		// Fixes bug #25309.
		m_pRegistrationManager->ClearRegistrations ();
	}

	stiRemoteLogEventSend ("EventType=CoreLogin Reason=LoginRequest User=%s", pszPhoneNumber);

	hResult = m_pCoreService->Login (
				pszPhoneNumber,
				pszPin,
				context,
				punRequestId,
				pszLoginAs);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::CoreLogout
//
// Description: Log out of the core system.
//
// Abstract:
//  This function should be called when Core Services has been logged in.
//
// Returns:
//  estiOK if in an initialized state and the Core Service logout message is
//  successfully sent, estiERROR otherwise.
//
stiHResult CstiCCI::CoreLogout (
	const VPServiceRequestContextSharedPtr &context,
	unsigned int *punRequestId)  // ID of request
{
	stiLOG_ENTRY_NAME (CstiCCI::CoreLogout);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_pRegistrationManager->ClearRegistrations ();
	//
	// If the videophone has been ported away from Sorenson, we must
	// avoid communications with the Sorenson back end.
	//
	if (estiPORTED_MODE != m_eLocalInterfaceMode)
	{
		stiRemoteLogEventSend ("EventType=CoreLogin Reason=LogoutRequest");

		memset (&m_stCoreLoginStartTime, 0, sizeof (m_stCoreLoginStartTime));
		hResult = m_pCoreService->Logout (context, punRequestId);
	}

	return (hResult);
} // end CstiCCI::CoreLogout


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::CoreAuthenticationPinUpdate
//
// Description: Update the Password in CoreService.
//
void CstiCCI::CoreAuthenticationPinUpdate (
	const std::string &pin)              //!< The user's pin.
{
	stiLOG_ENTRY_NAME (CstiCCI::CoreAuthenticationPinUpdate);

	m_pCoreService->UserPinSet (pin.c_str ());

} // end CstiCCI::CoreAuthenticationPinUpdate


/*!\brief Calls CoreLogin() to login a ported user.
 *
 *  This function should be called when reconnecting a ported user.
 *
 * \param phoneNumber The user's phone number.
 * \param password         The user's pin.
 *
 * \return Success or failure result
 */
stiHResult CstiCCI::portBackLogin(
	const std::string &phoneNumber,
	const std::string &password)
{
	stiLOG_ENTRY_NAME (CstiCCI::portBackLogin);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure CoreLogin() and CoreRequestSend() know we are porting back.
	m_portingBack = true;

	// Restart coreServices to process CoreLogin
	m_pCoreService->PauseSet (false);
	hResult = CoreLogin(phoneNumber,
						password,
						nullptr, nullptr, nullptr);

	if (stiIS_ERROR(hResult))
	{
		m_portingBack = false;
	}

	return hResult;
}


/*!\brief Sets if we should be using the proxy keep alive code or not.
 *
 */
void CstiCCI::useProxyKeepAliveSet (bool useProxyKeepAlive)
{
	if (m_pConferenceManager)
	{
		m_pConferenceManager->useProxyKeepAliveSet (useProxyKeepAlive);
	}
}

void CstiCCI::externalCallSet(VRCLCallSP& call)
{
	if (m_pVRCLServer)
	{
		m_pVRCLServer->externalCallSet(call);
	}
}

stiHResult CstiCCI::externalCallStatusSet(std::string& callStatus)
{
	if (m_pVRCLServer)
	{
		return m_pVRCLServer->externalConferenceStateChanged(callStatus);
	}
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

stiHResult CstiCCI::externalCallParticipantCountSet(int participantCount)
{
	if (m_pVRCLServer)
	{
		return m_pVRCLServer->externalParticipantCountChanged(participantCount);
	}
	return stiMAKE_ERROR (stiRESULT_ERROR);
}

ISignal<const SstiSharedContact&>& CstiCCI::contactReceivedSignalGet ()
{
	return contactReceivedSignal;
}

ISignal<const ExternalConferenceData&>& CstiCCI::externalConferenceSignalGet()
{
	return externalConferenceSignal;
}

ISignal<bool>& CstiCCI::externalCameraUseSignalGet()
{
	return externalCameraUseSignal;
}


/*!\brief Signal for when ClientAuthenticateResult core response is received.
 *
 */
ISignal<const ServiceResponseSP<ClientAuthenticateResult>&>& CstiCCI::clientAuthenticateSignalGet ()
{
	return clientAuthenticateSignal;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::CoreRequestSend
//
// Description: Send a Core Request
//
// Abstract:
//  This function should be called when Core Services has been logged in.  If
//  not logged in, the CoreService.RequestSend will initiate a re-login
//  sequence.
//
//  NOTE: The poCoreRequest parameter MUST have been created with the 'new'
//  operator. This object WILL NO LONGER belong to the calling code, and will
//  be freed by this function.
//
// Returns:
//  estiOK if in an initialized state and the Core Service logout message is
//  successfully sent, estiERROR otherwise.
//
// Note: This method has been deprecated.  Use CoreRequestSendEx instead.
//
stiHResult CstiCCI::CoreRequestSend (
	CstiCoreRequest * poCoreRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiCCI::CoreRequestSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Only send the request when not in ported mode.
	// If in ported mode, allow the request to be deleted and
	// return success.
	//
	if (estiPORTED_MODE != m_eLocalInterfaceMode ||
		m_portingBack)
	{
		hResult = m_pCoreService->RequestSendEx (
			poCoreRequest,
			punRequestId);
		stiTESTRESULT ();

		poCoreRequest = nullptr;
	}
	else
	{
		if (punRequestId)
		{
			*punRequestId = 0;
		}
	}

STI_BAIL:

	if (poCoreRequest)
	{
		delete poCoreRequest;
		poCoreRequest = nullptr;
	}

	return hResult;
} // end CstiCCI::CoreRequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::CoreRequestSendEx
//
// Description: Send a Core Request
//
// Abstract:
//  This function should be called when Core Services has been logged in.  If
//  not logged in, the CoreService.RequestSend will initiate a re-login
//  sequence.
//
//  NOTE: The poCoreRequest parameter MUST have been created with the 'new'
//  operator.
//
// Returns:
//  estiOK if in an initialized state and the Core Service logout message is
//  successfully sent, estiERROR otherwise.
//
stiHResult CstiCCI::CoreRequestSendEx (
	CstiCoreRequest * poCoreRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiCCI::CoreRequestSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (estiPORTED_MODE != m_eLocalInterfaceMode ||
		m_portingBack)
	{
		hResult = m_pCoreService->RequestSendEx (
			poCoreRequest,
			punRequestId);
	}
	else
	{
		// When in ported mode, simply free the request and return success.
		if (nullptr != poCoreRequest)
		{
			delete poCoreRequest;
			poCoreRequest = nullptr;
		}
	}

	return hResult;
} // end CstiCCI::CoreRequestSend


/*! \brief Requests a heartbeat message be sent to the core system.
 *
//  This function should be called when logged in to Core Services.
 *
 *  \return estiOK If in an initialized state and the Core Service heartbeat
 *      message is successfully sent.
 *  \retval estiERROR Otherwise.
 */
void CstiCCI::StateNotifyHeartbeatRequest ()
{
	stiLOG_ENTRY_NAME (CstiCCI::StateNotifyHeartbeatRequest);
	m_pStateNotifyService->HeartbeatRequestAsync (0);
} // end CstiCCI::StateNotifyHeartbeatRequest


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::ConferenceRequestSendEx
//
// Description: Send a Conference Request
//
// Abstract:
//  Sends a conference request.  Will automatically reauthenticate if somehow
//  core service was logged out.
//
//  NOTE: The poConferenceRequest parameter MUST have been created with the 'new'
//  operator. This object WILL NO LONGER belong to the calling code, and will
//  be freed by this function.
//
// Returns:
//  estiOK if in an initialized state and the Message Service logout message is
//  successfully sent, estiERROR otherwise.
//
stiHResult CstiCCI::ConferenceRequestSendEx (
	CstiConferenceRequest * poConferenceRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiCCI::ConferenceRequestSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pConferenceService->RequestSendEx (
		poConferenceRequest,
		punRequestId);

	return (hResult);
} // end CstiCCI::ConferenceRequestSendEx


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::MessageRequestSend
//
// Description: Send a Message Request
//
// Abstract:
//  Sends a message request.  Will automatically reauthenticate if somehow
//  core service was logged out.
//
//  NOTE: The poMessageRequest parameter MUST have been created with the 'new'
//  operator. This object WILL NO LONGER belong to the calling code, and will
//  be freed by this function.
//
// Returns:
//  estiOK if in an initialized state and the Message Service logout message is
//  successfully sent, estiERROR otherwise.
//
stiHResult CstiCCI::MessageRequestSend (
	CstiMessageRequest * poMessageRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiCCI::MessageRequestSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pMessageService->RequestSendEx (
		poMessageRequest,
		punRequestId);
	stiTESTRESULT ();

	poMessageRequest = nullptr;

STI_BAIL:

	if (poMessageRequest)
	{
		delete poMessageRequest;
		poMessageRequest = nullptr;
	}

	return (hResult);
} // end CstiCCI::MessageRequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::MessageRequestSendEx
//
// Description: Send a Message Request
//
// Abstract:
//  Sends a message request.  Will automatically reauthenticate if somehow
//  core service was logged out.
//
//  NOTE: The poMessageRequest parameter MUST have been created with the 'new'
//  operator.
//
// Returns:
//  Failure or success in an stiHResult
//
stiHResult CstiCCI::MessageRequestSendEx (
	CstiMessageRequest * poMessageRequest,
	unsigned int *punRequestId)
{
	stiLOG_ENTRY_NAME (CstiCCI::MessageRequestSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pMessageService->RequestSendEx (
		poMessageRequest,
		punRequestId);

	return hResult;
} // end CstiCCI::MessageRequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::MessageRequestCancel
//
// Description: Cancel a Message Request
//
// Abstract:
//  Cancels a previously submitted message request.
//
// Returns:
//  estiOK if the cancel request succeeds, estiERROR otherwise.
//
void CstiCCI::MessageRequestCancel (
	unsigned int unRequestId)
{
	stiLOG_ENTRY_NAME (CstiCCI::MessageRequestCancel);
	m_pMessageService->RequestCancelAsync (unRequestId);
} // end CstiCCI::MessageRequestSend


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RemoteLogEventSend
//
// Description: Send an event message to the Remote Logger service
//
// Abstract:
//  Sends a Remote Logger event message.
//
//
// Returns:
//  estiOK if in an initialized state and the Remote Logger logout message is
//  successfully sent, estiERROR otherwise.
//
stiHResult CstiCCI::RemoteLogEventSend (
	const std::string &buff)
{
	stiLOG_ENTRY_NAME (CstiCCI::RemoteLogEventSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiRemoteLogEventSend (buff.c_str ());

	return (hResult);
}// end CstiCCI::LogEventSend


////////////////////////////////////////////////////////////////////////////////
//; CurrentTimeGet
//
// Description: Gets the current UTC (GMT) time
//
// Abstract:
//  This function should be called after Core Service has connected.
//
// Returns:
//  time_t - the current time, or 0 (Zero) on error
//
static time_t CurrentTimeGet ()
{
	stiLOG_ENTRY_NAME (CstiCCI::CurrentTimeGet);

	time_t ttReturn = 0;

	//
	// Get the time.
	//
	time(&ttReturn);

	return (ttReturn);
} // end CstiCCI::CurrentTimeGet


/*!\brief Sets the current time
 *
 *  This function should be called while in any initialized state and will
 *  fail if the state is INVALID, INITIALIZING or SHUTDOWN.
 *
 * \param currentTime  The current time.
 */
void CstiCCI::CurrentTimeSet (
	time_t currentTime)
{
	stiLOG_ENTRY_NAME (CstiCCI::CurrentTimeSet);

	std::lock_guard<std::recursive_mutex> lock(m_LockMutex);

	// Check to see if there are any current calls
	if (0 == CallObjectsCountGet(
		esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH)
	 && m_pFilePlay->StateGet () != IstiMessageViewer::estiPLAYING)
	{
		time_t BootTime = 0;
		
		if (!m_bTimeSet)
		{
#if DEVICE == DEV_NTOUCH
			//
			// How long has it been since the epoch?
			// Subtract it from the current time and that
			// will be our boot time.
			//
			BootTime = currentTime - time (nullptr);
#elif APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
			BootTime = currentTime - (time (nullptr) - m_EngineCreationTime);
#else
			BootTime = currentTime;
#endif
		}
		
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
		stiDEBUG_TOOL (g_stiCCIDebug,
					   char buf[100];
					   timespec currentTimespec {};
					   currentTimespec.tv_sec = currentTime;
					   currentTimespec.tv_nsec = 0;
					   std::strftime(buf, sizeof (buf), "%D %T", std::gmtime(&currentTimespec.tv_sec));
					   stiTrace ("CstiCCI::CurrentTimeSet: setting current time to: %s.%09ld UTC\n", buf, currentTimespec.tv_nsec);
		);
		
		IstiPlatform::InstanceGet ()->currentTimeSet (currentTime);
#endif

		if (!m_bTimeSet)
		{
			int nCount = 0;
			char szTimeBuffer[BOOT_TIME_LENGTH + 1];
			tm utcBootTime{};

			//
			// Send up the boot time with the boot count value so we can associate a boot count
			// property with the boot time.
			//

			PropertyManager::getInstance ()->propertyGet (BOOT_COUNT, &nCount);

			gmtime_r (&BootTime, &utcBootTime);
			strftime (szTimeBuffer, sizeof (szTimeBuffer) - 1, "%Y%m%d %H:%M:%S", &utcBootTime);

			const char *pszRestartReason = "Unknown";

			IstiPlatform::EstiRestartReason eRestartReason = IstiPlatform::estiRESTART_REASON_UNKNOWN;
			IstiPlatform::InstanceGet ()->RestartReasonGet (&eRestartReason);

			switch (eRestartReason)
			{
				case IstiPlatform::estiRESTART_REASON_UNKNOWN:

					pszRestartReason = "Unknown";
					break;

				case IstiPlatform::estiRESTART_REASON_MEDIA_LOSS:

					pszRestartReason = "MediaLoss";
					break;

				case IstiPlatform::estiRESTART_REASON_VRCL_COMMAND:

					pszRestartReason = "VRCLCommand";
					break;

				case IstiPlatform::estiRESTART_REASON_UPDATE:

					pszRestartReason = "Update";
					break;

				case IstiPlatform::estiRESTART_REASON_CRASH:

					pszRestartReason = "Crash";
					break;

				case IstiPlatform::estiRESTART_REASON_ACCOUNT_TRANSFER:

					pszRestartReason = "AccountTransfer";
					break;

				case IstiPlatform::estiRESTART_REASON_DNS_CHANGE:

					pszRestartReason = "DNSChange";
					break;

				case IstiPlatform::estiRESTART_REASON_STATE_NOTIFY_EVENT:

					pszRestartReason = "StateNotifyEvent";
					break;

				case IstiPlatform::estiRESTART_REASON_TERMINATED:

					pszRestartReason = "Terminated";
					break;

				case IstiPlatform::estiRESTART_REASON_UPDATE_FAILED:

					pszRestartReason = "UpdateFailed";
					break;

				case IstiPlatform::estiRESTART_REASON_OUT_OF_MEMORY:

					pszRestartReason = "OutOfMemory";
					break;

				case IstiPlatform::estiRESTART_REASON_UPDATE_TIMEOUT:

					pszRestartReason = "UpdateTimeout";
					break;

				case IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION:
					pszRestartReason = "UnhandledException";
					break;

				case IstiPlatform::estiRESTART_REASON_POWER_ON_RESET:
					pszRestartReason = "PowerOnReset";
					break;

				case IstiPlatform::estiRESTART_REASON_WATCHDOG_RESET:
					pszRestartReason = "WatchdogReset";
					break;

				case IstiPlatform::estiRESTART_REASON_OVER_TEMPERATURE_RESET:
					pszRestartReason = "OverTemperatureReset";
					break;

				case IstiPlatform::estiRESTART_REASON_SOFTWARE_RESET:
					pszRestartReason = "SoftwareReset";
					break;

				case IstiPlatform::estiRESTART_REASON_LOW_POWER_EXIT:
					pszRestartReason = "LowPowerExit";
					break;

				case IstiPlatform::estiRESTART_REASON_UI_BUTTON:
					pszRestartReason = "UIButton";
					break;

				case IstiPlatform::estiRESTART_REASON_DAILY:
					pszRestartReason = "DailyReset";
					break;
			}

			std::stringstream ss;
			ss << nCount << ": " << szTimeBuffer << " " << pszRestartReason;
			std::string bootTimePropertyValue = ss.str();

			PropertyManager::getInstance ()->propertySet (BOOT_TIME, bootTimePropertyValue);
			PropertyManager::getInstance ()->PropertySend (BOOT_TIME, estiPTypePhone);

			std::stringstream strLogEvent;
			strLogEvent << "BootCount = " << nCount << " BootTime = \"" << szTimeBuffer << "\" RestartReason = " << pszRestartReason;
			stiRemoteLogEventSend (strLogEvent.str().c_str());

			stiDEBUG_TOOL (g_stiCCIDebug,
						vpe::trace ("CstiCCI::CurrentTimeSet: BootCount = ", nCount, " BootTime = ", szTimeBuffer, " RestartReason = ", pszRestartReason, "\n");
			);
			
			AppNotify(estiMSG_TIME_SET, 0);
			m_bTimeSet = true;
			m_bNeedsClockSync = false;
		}
		
	} // end if

} // end CstiCCI::CurrentTimeSet


/*!\brief Instructs the CCI to add a missed call to the Missed Call List
 *
 *  \retval estiOK On success.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::MissedCallHandle (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiCCI::MissedCallHandle);

	if (call)
	{
		unsigned int unRequestId = 0;
		EstiBool bBlockCallerId = estiFALSE;

		if (call->LocalCallerIdBlockedGet () &&
			m_pCoreService->APIMajorVersionGet () >= 8) // Only supported on Core 8.0 Release 2 and newer
		{
			bBlockCallerId = estiTRUE;
		}

		if (estiKIOSK_MODE !=  call->RemoteInterfaceModeGet ()
		 && estiPORTED_MODE != m_eLocalInterfaceMode)
		{
			// Yes! Were we answering this call?
			if (estiINCOMING == call->DirectionGet ())
			{
				EstiBool bSameProduct = estiFALSE;
				EstiBool bHavePhone = estiFALSE;

				// Yes! Are they both Sorenson Videophones?  It could be a VP100
				// (identified as "Sorenson Videophone") or a VP200 (identified
				// as "Sorenson Videophone V2").

				// Only set bSameProduct to true if we are actually talking to two Sorenson endpoints
				// and we do not want to force this endpoint to log a missed call.
				if (call->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_VIDEOPHONE)
					 && !call->AddMissedCallGet ())
				{
					// Yes!
					bSameProduct = estiTRUE;

					// Is there a phone number?
					auto poCallInfo = (CstiCallInfo *)call->RemoteCallInfoGet ();
					const SstiUserPhoneNumbers *psUserPhoneNumbers = poCallInfo->UserPhoneNumbersGet ();
					if (0 != strlen (psUserPhoneNumbers->szPreferredPhoneNumber))
					{
						// Yes!
						bHavePhone = estiTRUE;
					} // end if
				} // end if

				// Was the caller NOT this same product?
				if (!bSameProduct)
				{
					// Yes! Build a call list item with the user's info
					auto oCallListItem = std::make_shared<CstiCallListItem> ();

					// Were we called by a VRS interpreter?
					if (estiINTERPRETER_MODE == call->RemoteInterfaceModeGet ())
					{
						// Yes!  Grab the display name, and add a blank call to the
						// missed call list.
						oCallListItem->DialMethodSet (estiDM_BY_VRS_PHONE_NUMBER);
						oCallListItem->DialStringSet ("");

						std::string RemoteDisplayName;
						call->RemoteDisplayNameGet (&RemoteDisplayName);

						oCallListItem->NameSet (RemoteDisplayName.c_str ());
					} // end if
					else
					{
						// No!  Were we sent a phone number?
						if (bHavePhone)
						{
							// Yes! Grab a copy of the phone number for processing.
							char szPhone [un8stiCANONICAL_PHONE_LENGTH + 1] = "";
							auto poCallInfo = (CstiCallInfo *)call->RemoteCallInfoGet ();
							const SstiUserPhoneNumbers *psUserPhoneNumbers = poCallInfo->UserPhoneNumbersGet ();
							const char *pszPhoneNumber = psUserPhoneNumbers->szPreferredPhoneNumber;

							// Remove non-digits from the phone number
							int nIndex = 0;
							for (size_t i = 0; i < strlen (pszPhoneNumber); i++)
							{
								if (isdigit (pszPhoneNumber[i]))
								{
									szPhone[nIndex] = pszPhoneNumber[i];
									nIndex++;
								} // end if
							} // end for
							szPhone[nIndex] = '\0';

							// Save the call by phone number.
							oCallListItem->DialMethodSet (estiDM_BY_DS_PHONE_NUMBER);
							oCallListItem->DialStringSet (szPhone);
						} // end if
						else
						{
							// No! Save the call by IP address.
							oCallListItem->DialMethodSet (estiDM_BY_DIAL_STRING);

							//
							// First check to see if there is a remote dial string.  This could
							// be a phone number or a URI.  If one does not exist then
							// get the ip address.
							//
							std::string remoteDialString;

							call->RemoteDialStringGet (&remoteDialString);

							if (remoteDialString.empty ())
							{
								remoteDialString = call->RemoteIpAddressGet ();
							}

							oCallListItem->DialStringSet (remoteDialString.c_str ());
						} // end else

						std::string RemoteName;
						call->RemoteNameGet (&RemoteName);

						oCallListItem->NameSet (RemoteName.c_str ());
					} // end else

					oCallListItem->CallTimeSet (CurrentTimeGet ());

					// check for and Set the Sip call id
					std::string callId;
					call->CallIdGet(&callId);
					if (!callId.empty())
					{
						oCallListItem->CallPublicIdSet (callId.c_str());
					}

					//
					// Set the intended dial string
					//
					std::string IntendedDialString;
					call->IntendedDialStringGet(&IntendedDialString);
					if (!IntendedDialString.empty())
					{
						oCallListItem->IntendedDialStringSet(IntendedDialString.c_str());
					}

					// Add this call to our own missed list

					// Create a core request object
					auto  poRequest = new CstiCoreRequest;
					stiTESTCOND (poRequest, stiRESULT_ERROR);

					// Set the missed call info into the request
					poRequest->callListItemAdd (
									*oCallListItem,
									CstiCallList::eMISSED);

					// Send the request
					hResult = CoreRequestSend (poRequest, &unRequestId);
					stiTESTRESULT ();

					m_PendingCoreRequests.insert (unRequestId);

					// Yes! Notify the app that the list has changed, so that it can
					// be requested again.
					auto poResponse = new CstiStateNotifyResponse (
								nullptr,
								CstiStateNotifyResponse::eCALL_LIST_CHANGED,
								estiOK,
								CstiStateNotifyResponse::eNO_ERROR,
								nullptr);
					stiTESTCOND (poResponse, stiRESULT_ERROR);

					// Could we create a response?
					// Yes! Send it to the app.
					poResponse->ResponseValueSet (CstiCallList::eMISSED);
#ifndef stiCALL_HISTORY_MANAGER
					hResult = AppNotify (estiMSG_STATE_NOTIFY_RESPONSE, (size_t)poResponse);
#else
					if (m_pCallHistoryManager == nullptr || !m_pCallHistoryManager->StateNotifyEventHandle (poResponse))
					{
						MessageCleanup (estiMSG_STATE_NOTIFY_RESPONSE, reinterpret_cast<size_t>(poResponse));
					}
#endif // stiCALL_HISTORY_MANAGER
				} // end if
				else
				{
					// The caller was the same product. The other end will
					// add the call, so just request an updated list in 10
					// seconds
					m_pStateNotifyService->HeartbeatRequestAsync (
													10); // 10 seconds
				} // end else
			} // end if
			else
			{
				// No! We placed the call. We need to add it to the callee's missed
				// call list.

				// check for and get the Sip call publicId (for use in the MissedCallAdd request)
				std::string publicId;
				call->CallIdGet(&publicId);

				// Are we using Interpreter mode?
				if (estiINTERPRETER_MODE == m_eLocalInterfaceMode)
				{
					// Yes! We are using interpreter mode
					// Is there a last number dialed saved?
					if (!m_lastDialed.empty ())
					{
						// Yes! Add the missed call

						// Create a core request object
						auto  poRequest = new CstiCoreRequest;
						stiTESTCOND (poRequest, stiRESULT_ERROR);

						EstiDialMethod eDialMethod = estiDM_UNKNOWN;
						std::string returnDialString;
						std::string dialString;
						std::string displayName;

						//
						// If there is a hearing number then use it
						// otherwise use the UIG return dial string.
						// If that is not available then use a VRS dial type
						// and an empty dial string.
						//
						auto poCallInfo = (CstiCallInfo *)call->LocalCallInfoGet ();
						const SstiUserPhoneNumbers *psUserPhoneNumbers = poCallInfo->UserPhoneNumbersGet ();
						if (psUserPhoneNumbers->szHearingPhoneNumber[0])
						{
							eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
							dialString = psUserPhoneNumbers->szHearingPhoneNumber;
							displayName = call->LocalDisplayNameGet ();
						}
						else
						{
							eDialMethod = call->LocalReturnCallDialMethodGet ();
							call->LocalReturnCallDialStringGet (&returnDialString);

							if (eDialMethod == estiDM_UNKNOWN)
							{
								eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
							}

							dialString = returnDialString;
							displayName = ""; // No Caller ID
						}

						{
							// If there is a fromName override value, always use it
							auto fromNameOverride = call->fromNameOverrideGet();
							if (fromNameOverride)
							{
								displayName = fromNameOverride.value();
							}
						}

						// Set the missed call info into the request
						poRequest->missedCallAdd (
							m_lastDialed.c_str (),
							eDialMethod, // Default to VRS.
							dialString.c_str (),     // Hearing Phone number
							displayName.c_str(), // Name
							CurrentTimeGet (),
							(publicId.empty() ? "" : publicId.c_str()),
							bBlockCallerId);  // CallPublicId

						// Send the request
						hResult = CoreRequestSend (poRequest, &unRequestId);
						stiTESTRESULT ();

						m_PendingCoreRequests.insert (unRequestId);
					} // end if
				} // end if
				else
				{
					// No! We are using normal mode
					// Is there a last number dialed saved?
					if (!m_lastDialed.empty () &&

						// Is the dialtype a directory service by phone number?
						estiDM_BY_DS_PHONE_NUMBER == call->DialMethodGet ())
					{
						// Yes! Add the missed call

						// Create a core request object
						auto  poRequest = new CstiCoreRequest;
						stiTESTCOND (poRequest, stiRESULT_ERROR);

						std::string returnDialString;

						EstiDialMethod eMethod = call->LocalReturnCallDialMethodGet ();
						call->LocalReturnCallDialStringGet (&returnDialString);

						if (eMethod == estiDM_UNKNOWN)
						{
							eMethod = estiDM_BY_DS_PHONE_NUMBER;
							returnDialString = m_sUserPhoneNumbers.szPreferredPhoneNumber;
						}

						auto displayName = call->LocalDisplayNameGet ();

						// if the display name was empty, get the alternate name
						if (displayName.empty())
						{
							displayName = call->localAlternateNameGet ();
						}

						{
							// If there is a fromName override value, always use it
							auto fromNameOverride = call->fromNameOverrideGet();
							if (fromNameOverride)
							{
								displayName = fromNameOverride.value ();
							}
						}

						//
						// Check to see if dialed a member of the ring group
						//
						if (IsDialStringInRingGroup(m_lastDialed.c_str()))
						{
							std::string Description;
							IstiRingGroupManager::ERingGroupMemberState eState;

							// Get the description of this phone to use for the return dial string
							if (m_pRingGroupManager->ItemGetByNumber (
								m_sUserPhoneNumbers.szLocalPhoneNumber,
								&Description,
								&eState))
							{
								returnDialString = Description;
							}
						}

						// Set the missed call info into the request
						poRequest->missedCallAdd (
							m_lastDialed.c_str (),
							eMethod,
							returnDialString.c_str (),
							displayName.c_str(),  // Name
							CurrentTimeGet (),
							(publicId.empty() ? "" : publicId.c_str()),
							bBlockCallerId);  // CallPublicId

						// Send the request
						hResult = CoreRequestSend (poRequest, &unRequestId);
						stiTESTRESULT ();

						m_PendingCoreRequests.insert (unRequestId);
					} // end if
				} // end else
			} // end else
		} // end if
	} // end if

STI_BAIL:

	return (hResult);
} // end CstiCCI::MissedCallHandle

#ifndef stiDISABLE_SDK_NETWORK_INTERFACE
///!
///\brief Update the property values that represent the network settings.
///
static stiHResult UpdateNetworkProperties (
	const SstiNetworkSettings *pSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PropertyManager *pPm = PropertyManager::getInstance ();

	pPm->propertySet (NetHostname, pSettings->NetHostname, PropertyManager::Persistent);

	pPm->propertySet (IPv4Dhcp, pSettings->bIPv4UseDHCP ? 1 : 0, PropertyManager::Persistent);
	pPm->propertySet (IPv4IpAddress, pSettings->IPv4IP, PropertyManager::Persistent);
	pPm->propertySet (IPv4SubnetMask, pSettings->IPv4SubnetMask, PropertyManager::Persistent);
	pPm->propertySet (IPv4GatewayIp, pSettings->IPv4NetGatewayIP, PropertyManager::Persistent);
	pPm->propertySet (IPv4Dns1, pSettings->IPv4DNS1, PropertyManager::Persistent);
	pPm->propertySet (IPv4Dns2, pSettings->IPv4DNS2, PropertyManager::Persistent);

	pPm->propertySet (IPv6Method, pSettings->eIPv6Method ? 1 : 0, PropertyManager::Persistent);
	pPm->propertySet (IPv6IpAddress, pSettings->IPv6IP, PropertyManager::Persistent);
	pPm->propertySet (IPv6Prefix, pSettings->nIPv6Prefix, PropertyManager::Persistent);
	pPm->propertySet (IPv6GatewayIp, pSettings->IPv6GatewayIP, PropertyManager::Persistent);
	pPm->propertySet (IPv6Dns1, pSettings->IPv6DNS1, PropertyManager::Persistent);
	pPm->propertySet (IPv6Dns2, pSettings->IPv6DNS2, PropertyManager::Persistent);

	return hResult;
}
#endif


#ifndef stiDISABLE_SDK_NETWORK_INTERFACE
/*!
 * \brief Sends the network settings to core server.
 *
 */
static stiHResult SendNetworkSettings()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PropertyManager::getInstance ()->PropertySend(NetHostname, estiPTypePhone);

	PropertyManager::getInstance ()->PropertySend(IPv4Dhcp, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv4IpAddress, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv4GatewayIp, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv4SubnetMask, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv4Dns1, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv4Dns2, estiPTypePhone);

	PropertyManager::getInstance ()->PropertySend(IPv6Method, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv6IpAddress, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv6GatewayIp, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv6Prefix, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv6Dns1, estiPTypePhone);
	PropertyManager::getInstance ()->PropertySend(IPv6Dns2, estiPTypePhone);

	return hResult;
}
#endif

stiHResult CstiCCI::UpdatePublicIP (
	const SstiNetworkSettings *pSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifndef USE_PROXY_FOR_PUBLIC_IP
	CstiCoreRequest *poCoreRequest = nullptr;

	//
	// Compare the IP address that has been the public ip.  If it is
	// different than the private IP consider sending the ip address
	// to core or requesting the current public IP as seen by core.
	//
	if (m_LastPublicIpSent != pSettings->IPv4IP)
	{
		PropertyManager *pPm = PropertyManager::getInstance();
		int nResult = REQUEST_SUCCESS;
		EstiPublicIPAssignMethod publicIpMethod = estiIP_ASSIGN_AUTO;
		pPm->propertyGet (cmPUBLIC_IP_ASSIGN_METHOD, &publicIpMethod);

		if (publicIpMethod == estiIP_ASSIGN_SYSTEM)
		{
			//
			// If public ip mode is setup to use the
			// private ip of the device then just send up
			// a new phone online request.
			//
			PhoneOnlineSend();
		}
		else if (publicIpMethod == estiIP_ASSIGN_AUTO)
		{
			//
			// If the public ip mode is set to auto detect then
			// just send a request to get the ip address as seen by core.
			//
			poCoreRequest = new CstiCoreRequest();

			// Did it allocate?
			if (poCoreRequest != nullptr)
			{
				// Not using private ip, send a publicIPGet request
				nResult = poCoreRequest->publicIPGet();
				stiTESTCOND (nResult == REQUEST_SUCCESS, stiRESULT_ERROR);

				// Yes! Send the request.
				CoreRequestSend(poCoreRequest, nullptr);
			}
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (poCoreRequest)
		{
			delete poCoreRequest;
			poCoreRequest = nullptr;
		}
	}
#endif // USE_PROXY_FOR_PUBLIC_IP

	return hResult;
}


/*!
 * \brief Helper method for connecting/handling VRCL server signals
 *
 *        TODO:  The old callback mechanism ran all of these handlers in
 *               the VRCLServer thread... need to verify if any of these
 *               handlers need to lock the execMutex for CCI's thread, or
 *               use PostEvent
 */
void CstiCCI::vrclSignalsConnect ()
{
	m_signalConnections.push_back (m_pVRCLServer->rebootDeviceSignal.Connect (
		[this]{
			AppNotify (estiMSG_REBOOT_DEVICE,
				(size_t)IstiPlatform::estiRESTART_REASON_VRCL_COMMAND);
		}));

	m_signalConnections.push_back (m_pVRCLServer->diagnosticsGetSignal.Connect (
			[this]{ DiagnosticsSend(); }));

	m_signalConnections.push_back (m_pVRCLServer->callDialSignal.Connect (
		[this](const SstiCallDialInfo &dialInfo){
			IstiCall *pCall = nullptr;

			auto hResult = CallDial (
				dialInfo.eDialMethod,
				dialInfo.DestNumber,
				nullptr,
				dialInfo.HearingNumber.empty() ? nullptr : dialInfo.HearingNumber.c_str(),
				dialInfo.hearingName,
				dialInfo.RelayLanguage,
				dialInfo.bAlwaysAllow,
				estiDS_VRCL,
				false,
				&pCall,
				dialInfo.additionalHeaders);

			if (!stiIS_ERROR (hResult))
			{
				auto call = (CstiCall*)pCall;
				// Store the additional information for leaving signmail (if needed)
				SstiMessageInfo messageInfo;
				hResult = call->MessageInfoGet (&messageInfo);
				if (!stiIS_ERROR (hResult))
				{
					messageInfo.CallId = dialInfo.CallId;
					messageInfo.TerpId = dialInfo.TerpId;
					messageInfo.hearingName = dialInfo.hearingName;
					// Set the response message info on the call object.
					call->MessageInfoSet (&messageInfo);
				}
			}
			else
			{
				m_pVRCLServer->callDialError ();
			}
		}));

	m_signalConnections.push_back (m_pVRCLServer->localNameSetSignal.Connect (
		[this](std::string name){
			auto hResult = LocalAlternateNameSet (name, g_nPRIORITY_VRCL);
			if (!stiIS_ERROR (hResult))
			{
				AppNotify (estiMSG_VRCL_LOCAL_ALTERNATE_NAME_SET, 0);
			}
		}));

	m_signalConnections.push_back (m_pVRCLServer->connectionClosedSignal.Connect (
		[this]{
			m_nLocalNamePriority = g_nPRIORITY_UI;
			LocalNamesUpdate();
			AppNotify(estiMSG_VRCL_CONNECTION_CLOSED, 0);
		}));

	m_signalConnections.push_back (m_pVRCLServer->propertiesSetSignal.Connect (
		[this](const CstiVRCLServer::PropertyList &propertyList){
			PropertiesSet (propertyList);
		}));

	m_signalConnections.push_back (m_pVRCLServer->signMailHangupSignal.Connect (
			[this]{ AppNotify(estiMSG_SIGNMAIL_HANGUP, 0); }));

	m_signalConnections.push_back (m_pVRCLServer->vrsCallIdSetSignal.Connect (
		[this](std::string callId){
			// Set VRSCallId so that it is passed to the remote endpoint. 
			m_pConferenceManager->vrsCallIdSet (callId);
			m_pRemoteLoggerService->VRSCallIdSet(callId.c_str());
		}));

	m_signalConnections.push_back (m_pVRCLServer->hearingStatusChangedSignal.Connect (
		[this](EstiHearingCallStatus hearingStatus){
			// We should be getting this from TerpNet and there should only ever be one connected call to an interpreter.
			// So lets get that call and send a message to the other phone.
			auto call = m_pConferenceManager->CallObjectGet (
					esmiCS_CONNECTED | esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT | esmiCS_HOLD_LCL);

			// Set local copy of Hearing Status for interpreter mode UI
			HearingStatusSet(hearingStatus);
			size_t callParam = 0;
			if (call)
			{
				call->HearingStatusSend(hearingStatus);
				callParam = (size_t)call.get();
			}

			// Tell the interpreter UI about the new status too.
			AppNotify(estiMSG_HEARING_STATUS_CHANGED, callParam);
		}));

	m_signalConnections.push_back (m_pVRCLServer->relayNewCallReadySignal.Connect (
		[this](bool ready){
			// We should be getting this from TerpNet and there should only ever be one connected call to an interpreter.
			// So lets get that call and send a message to the other phone.
			auto call = m_pConferenceManager->CallObjectGet (
					esmiCS_CONNECTED | esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT | esmiCS_HOLD_LCL);
			if (call)
			{
				call->NewCallReadyStatusSend(ready);
			}
		}));

	m_signalConnections.push_back (m_pVRCLServer->connectionEstablishedSignal.Connect (
			[this]{ AppNotify (estiMSG_VRCL_CONNECTION_ESTABLISHED, 0); }));

	m_signalConnections.push_back (m_pVRCLServer->textMessageSentSignal.Connect (
			[this](uint16_t *text){ AppNotify (estiMSG_TEXT_MESSAGE_SENT, (size_t)text); }));

	m_signalConnections.push_back (m_pVRCLServer->terpNetModeSetSignal.Connect (
			[this](const std::string &text)
			{
				auto copy = new char[text.size () + 1];
				strcpy (copy, text.c_str ());

				AppNotify (estiMSG_TERP_NET_MODE_SET, (size_t)copy);
			}));

//	m_signalConnections.push_back (m_pVRCLServer->updateCheckNeededSignal.Connect (
//			[this]{ AppNotify (estiMSG_UPDATE_CHECK_NEEDED, 0); }));

	m_signalConnections.push_back (m_pVRCLServer->vrsAnnounceClearSignal.Connect (
			[this]{ AppNotify (estiMSG_VRS_ANNOUNCE_CLEAR, 0); }));

	m_signalConnections.push_back (m_pVRCLServer->vrsAnnounceSetSignal.Connect (
			[this](const std::string &text)
			{
				auto copy = new char[text.size () + 1];
				strcpy (copy, text.c_str ());

				AppNotify (estiMSG_VRS_ANNOUNCE_SET, (size_t)copy);
			}));

	m_signalConnections.push_back (m_pVRCLServer->textInSignMailSetSignal.Connect (
			[this](const std::string &signMailText, int startSeconds, int endSeconds)
			{
				PostEvent([this, signMailText, startSeconds, endSeconds]
				{
					m_signMailText = signMailText; 
					m_signMailTextStartSeconds = startSeconds;
					m_signMailTextEndSeconds = endSeconds;  
				});
			}));

	m_signalConnections.push_back (m_pVRCLServer->hearingVideoCapabilitySignal.Connect (
		[this](const std::string &hearingNumber){
			// We should be getting this from TerpNet and there should only ever be one connected call to an interpreter.
			// So lets get that call and send a message to the other phone.
			auto call = m_pConferenceManager->CallObjectGet (
					esmiCS_CONNECTED | esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT | esmiCS_HOLD_LCL);
			if (call)
			{
				call->dhviCapabilityCheck (hearingNumber);
			}
			else
			{
				stiASSERTMSG (false, "DHV Capability Check failed to find valid call to check number %s", hearingNumber.c_str ());
			}
		}));

	m_signalConnections.push_back(m_pVRCLServer->externalConferenceSignal.Connect(
		[this](const ExternalConferenceData& data) {
			externalConferenceSignal.Emit(data);
		}));

	m_signalConnections.push_back(m_pVRCLServer->externalCameraUseSignal.Connect(
		[this](bool inUse) {
			externalCameraUseSignal.Emit(inUse);
		}));
}


/*!\brief Event handler to request an upload GUID
 *
 */
void CstiCCI::eventMessageUploadGUIDRequest()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);
	if (m_spLeaveMessageCall)
	{
		SstiMessageInfo stMessageInfo;
		hResult = m_spLeaveMessageCall->MessageInfoGet(&stMessageInfo);

		// If the mailbox is full don't request the Upload GUID.
		if (!stiIS_ERROR (hResult) && stMessageInfo.bMailBoxFull == estiFALSE)
		{
			hResult = RequestMessageUploadGUID();
		}
	}
}


/*!\brief Event handler to delete a recorded message.
 *
 */
void CstiCCI::eventRecordedMessageDelete(
	const SstiRecordedMessageInfo &messageInfo)
{
	// The message was asked to be deleted so don't need the call object any longer.
	RecordMessageDelete(messageInfo);

	std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);
	if (m_spLeaveMessageCall)
	{
		m_spLeaveMessageCall = nullptr;
		m_pFilePlay->signMailCallSet(nullptr);
	}
}


/*!\brief Event handler to send a recorded message.
 *
 */
void CstiCCI::eventRecordedMessageSend(
	const SstiRecordedMessageInfo &messageInfo)
{
	// The message was asked to be sent so don't need the call object any longer.
	RecordMessageSend(messageInfo);

	std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);
	if (m_spLeaveMessageCall)
	{
		m_spLeaveMessageCall = nullptr;
		m_pFilePlay->signMailCallSet(nullptr);
	}
}


/*!\brief Event handler handles recording of a SignMail when greeting is skipped.
 *
 */
void CstiCCI::eventSignMailGreetingSkipped()
{
	std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

	// If we have a text only greeting then start the countdown video.
	if (m_spLeaveMessageCall &&
		m_spLeaveMessageCall->SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE))
	{
		SstiMessageInfo stMessageInfo;
		auto call = m_spLeaveMessageCall;
		call->MessageInfoGet(&stMessageInfo);

		if (m_spLeaveMessageCall->MessageGreetingTypeGet() == eGREETING_TEXT_ONLY)
		{
			// If the mailbox is full notify the UI.
			if (stMessageInfo.bMailBoxFull == estiTRUE)
			{
				RecordMessageMailBoxFull(call);
			}
			else
			{
				// Start the countdown video.
				m_pFilePlay->LoadCountdownVideo();
			}

			// Stop the TextOnlyGreeting timer
			m_textOnlyGreetingTimer.stop ();
		}
		else if (m_spLeaveMessageCall->MessageGreetingTypeGet() == eGREETING_DEFAULT && 
				 stMessageInfo.bCountdownEmbedded == estiTRUE)
		{
			// Start the countdown video.
			m_pFilePlay->LoadCountdownVideo();
		}
	}
}

/*!\brief Event handler handles fileplayer state changes.
 *
 */
void CstiCCI::eventFilePlayerStateSet(
	IstiMessageViewer::EState state)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	switch (state)
	{
		case IstiMessageViewer::estiOPENING:
		{
			//
			// Pause update
			//
//			hResult = IstiUpdate::InstanceGet()->Pause();
			break;
		}

		case IstiMessageViewer::estiPLAYING:
		{
			//
			// If we are playing and we have a call object in the "leave message" state
			// then assume we are playing a greeting.  Use this opportunity to request
			// an upload guid.
			//
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			if (m_spLeaveMessageCall)
			{
				if (m_spLeaveMessageCall->SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE))
				{
					SstiMessageInfo stMessageInfo;
					hResult = m_spLeaveMessageCall->MessageInfoGet(&stMessageInfo);

					// If the mailbox is not full then request the Upload GUID.
					if (!stiIS_ERROR (hResult) && stMessageInfo.bMailBoxFull == estiFALSE)
					{
						// We can't just pass m_vidPrivacyEnabled into PrivacyGet() because we could be testing the
						// privacy setting after it has already been cleared and it would be reset it to false when it is 
						// already true.  So we use videoPrivacy to check it.
						auto videoPrivacy = estiFALSE;
						IstiVideoInput::InstanceGet()->PrivacyGet(&videoPrivacy);
						if (videoPrivacy)
						{
							m_vidPrivacyEnabled = true;

							// If video privacy is enabled, disable it.
							IstiVideoInput::InstanceGet ()->PrivacySet (estiFALSE);
						}
					}
				}
				else
				{
					stiASSERT (estiFALSE);
				}
			}
#ifdef stiMESSAGE_MANAGER
			else if (m_pFilePlay->GetItemId().IsValid ())
			{
				//If we are playing a message then we need to update it's viewed status.
				CstiMessageInfo messageInfo;
				messageInfo.ItemIdSet(m_pFilePlay->GetItemId());

				m_pMessageManager->Lock();

				m_pMessageManager->MessageInfoGet(&messageInfo);

				if (messageInfo.ViewedGet() == estiFALSE)
				{
					messageInfo.ViewedSet(estiTRUE);
					m_pMessageManager->MessageInfoSet(&messageInfo);
				}

				m_pMessageManager->Unlock();
			}
#endif
			break;
		}

		case IstiMessageViewer::estiPAUSED:
		{
#ifdef stiMESSAGE_MANAGER
			//
			// If we are playing and we have a call object in the "leave message" state
			// then assume we are playing a greeting.  So we don't need to store a pause point.
			//
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			if (!m_spLeaveMessageCall && m_pFilePlay->GetItemId ().IsValid ())
			{
				UpdatePausePoint();
			}
#endif
			break;
		}

		case IstiMessageViewer::estiVIDEO_END:
		{
			//
			// We have reached the end of the video.
			// If we have a call object that is in a "leave message" state
			// then we can presume that the video that just ended is the
			// greeting.  Close the video and wait for the "closed" state to start recording.
			//
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			if (m_spLeaveMessageCall ||
				m_pFilePlay->RecordTypeGet() != IstiMessageViewer::estiRECORDING_IDLE)
			{
				// We should now start to record so close the Movie file.
				m_pFilePlay->CloseMovie();
			}
			else if (m_pFilePlay->GetItemId ().IsValid ())
			{
				UpdatePausePoint();
			}

			break;
		}

		case IstiMessageViewer::estiCLOSED:
		{
			//
			// Resume update, if one is in progress
			//
//			hResult = IstiUpdate::InstanceGet()->Resume();

			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			if (m_spLeaveMessageCall)
			{
				auto call = m_spLeaveMessageCall;

				if (call->SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE))
				{
					SstiMessageInfo stMessageInfo;
					hResult = call->MessageInfoGet(&stMessageInfo);

					// If the mailbox is full notify the UI.
					if (!stiIS_ERROR (hResult) && stMessageInfo.bMailBoxFull == estiTRUE)
					{
						hResult = RecordMessageMailBoxFull(call);
					}
				}
				else
				{
					// If we are not in the LEAVE_MESSAGE substate then we need to
					// stop recording and clean up the message.
					m_pFilePlay->RecordStop();
					m_pFilePlay->DeleteRecordedMessage();
				}
			}
			// If we don't have a LeaveMessageCall object and the fileplay is in RECORDING_UPLOAD_URL
			// record state then the call has been disconnected and we need to stop the record, cleanup
			// the message and reset the fileplayer recording type.
			else if (m_pFilePlay->RecordTypeGet() == IstiMessageViewer::estiRECORDING_UPLOAD_URL)
			{
				m_pFilePlay->RecordStop();
				m_pFilePlay->DeleteRecordedMessage();
			}

			break;
		}

		case IstiMessageViewer::estiERROR:
		{
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			// If this is a record error then send the UI the record error message.
			// Otherwise just send error message to the app.
			if (m_spLeaveMessageCall)
			{ 
				if (m_pFilePlay->ErrorGet() == IstiMessageViewer::estiERROR_RECORD_CONFIG_INCOMPLETE)
				{
					// If we get here FilePlayer has not been given an upload GUID, so it has generated an
					// error.  If the mailbox if full FilePlayer will not get a GUID. Because FilePlayer cannot 
					// check the mailbox status, we need to do it here. 

					SstiMessageInfo stMessageInfo;
					hResult = m_spLeaveMessageCall->MessageInfoGet(&stMessageInfo);

					// If the mailbox is not full then notify the app of the record error. If the mailbox is
					// full, the mailbox full event was sent when the eMESSAGE_UPLOAD_URL_GET_RESULT was
					// processed and we don't want to send this record error. 
					if (stiIS_ERROR (hResult) || 
						stMessageInfo.bMailBoxFull == estiFALSE)
					{
						AppNotify(estiMSG_RECORD_ERROR, 0);
					}
				}
			}
			// Check to see if we had an error opening the greeting and that the call is not in a
			// connecting or connected state.
			else if ((m_pConferenceManager->CallObjectGet (esmiCS_CONNECTING) ||
					  m_pConferenceManager->CallObjectGet (esmiCS_CONNECTED)) &&
					 m_pFilePlay->ErrorGet() == IstiMessageViewer::estiERROR_OPENING)
			{
				// The greeting failed to open so close the file and wait for the ring count
				// to expire or the remote phone to reject the call.  Once that happens then
				// the error can be handled by the UI.
				m_pFilePlay->CloseMovie();
			}
			break;
		}

		case IstiMessageViewer::estiREQUESTING_GUID:
		{
			// Why is this needed here?
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			if (m_spLeaveMessageCall
			 && m_spLeaveMessageCall->SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE))
			{
				AppNotify(estiMSG_LEAVE_MESSAGE, (size_t)m_spLeaveMessageCall.get());
				m_pVRCLServer->VRCLNotify (estiMSG_LEAVE_MESSAGE, m_spLeaveMessageCall);
			}

			break;
		}

		case IstiMessageViewer::estiRECORDING:
		{
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);

			if (m_spLeaveMessageCall &&
				m_spLeaveMessageCall->SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE))
			{
				// Inform the VRCL client that we are starting the recording now.
				AppNotify (estiMSG_SIGNMAIL_RECORDING_STARTED, (size_t)m_spLeaveMessageCall.get());
				m_pVRCLServer->VRCLNotify (estiMSG_SIGNMAIL_RECORDING_STARTED);
			}

			break;
		}

		case IstiMessageViewer::estiRECORD_FINISHED:
		{
			std::lock_guard<std::recursive_mutex> lk (m_LeaveMessageMutex);
			if (m_spLeaveMessageCall)
			{
				if (LocalInterfaceModeGet() == estiINTERPRETER_MODE &&
					 m_pVRCLServer->SignMailEnabledGet ())
				{
					AppNotify (estiMSG_SIGNMAIL_RECORD_FINISHED, (size_t)m_spLeaveMessageCall.get());
				}
			}

			break;
		}

		case IstiMessageViewer::estiRECORD_CLOSED:
		{
			// If video privacy was on turn it back on.
			if (m_vidPrivacyEnabled)
			{
				m_vidPrivacyEnabled = false;

				// If video privacy was enabled, enable it.
				IstiVideoInput::InstanceGet ()->PrivacySet (estiTRUE);
			}

			break;
		}

		default:
			break;
	}

}

/*!\brief Handles network settings changed events
 *
 */
void CstiCCI::eventNetworkSettingsChanged ()
{
#ifndef stiDISABLE_SDK_NETWORK_INTERFACE
	SstiNetworkSettings Settings;

	IstiNetwork::InstanceGet ()->SettingsGet (&Settings);

	UpdateNetworkProperties (&Settings);

	///\todo: do we need to check if core is up?

	SendNetworkSettings ();

	///\todo: Handle public IP stuff
	UpdatePublicIP (&Settings);
#endif

	m_pConferenceManager->NetworkChangedNotify();
}


/*!\brief Handles network state change events
 *
 */
void CstiCCI::eventNetworkStateChange (
	IstiNetwork::EstiState eState)
{
	m_pRemoteLoggerService->NetworkTypeAndDataSet (
		IstiNetwork::InstanceGet ()->networkTypeGet (),
		IstiNetwork::InstanceGet ()->networkDataGet ());

	if (eState == IstiNetwork::estiIDLE)
	{
		//
		// In case the local IP has just changed (e.g. just obtained in the case of DHCP),
		// make sure to update the local IP address in the conference params structure.
		//
		SstiConferenceParams stConferenceParams;

		auto hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);

		if (!stiIS_ERROR (hResult))
		{
			PublicIPAddressGet (&stConferenceParams.PublicIPv4, estiTYPE_IPV4);
#ifdef IPV6_ENABLED
			PublicIPAddressGet (&stConferenceParams.PublicIPv6, estiTYPE_IPV6);
#endif
			m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		}

		//
		// If the engine was waiting for the network to go to
		// idle then start the engine now.
		//
		if (m_bStartEngine)
		{
			Startup ();
		}
	}
}


/*!
 * \brief Event handler for when groupVideoChatRoomStatTimer fires
 */
void CstiCCI::EventConferenceRoomStatsTimeout ()
{
	// For all calls that are connected (or held), send a request to the
	// ConferenceService for each that has a conference GUID. This code takes
	// advantage of the fact that there should be at most two calls to be
	// concerned with at this time; one in the connected | hold_rmt state and
	// the other in a hold_lcl or hold_both state.
	auto call = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTED | esmiCS_HOLD_RMT);
	if (call)
	{
		ConferenceRoomStatusGet (call);
	}
	call = m_pConferenceManager->CallObjectGet (esmiCS_HOLD_LCL | esmiCS_HOLD_BOTH);
	if (call)
	{
		ConferenceRoomStatusGet (call);
	}
}


/*!
 * \brief Event handler for when the groupVideoChatRoomCreateTimer fires
 */
void CstiCCI::EventConferenceRoomCreateTimeout ()
{
	// Cancel the request
	m_pConferenceService->RequestCancel (m_unConferenceRoomCreateRequestId);

	// Find the call objects that had been waiting and clear the request ID from their object
	std::string userID;

	const ICallInfo *pCallInfo = nullptr;
	auto call = FindCallByRequestId (m_unConferenceRoomCreateRequestId);
	if (call)
	{
		call->AppDataSet ((size_t)0); // Clear the appdata data member
		pCallInfo = call->LocalCallInfoGet ();
		pCallInfo->UserIDGet (&userID);

		call = FindCallByRequestId (m_unConferenceRoomCreateRequestId);
		if (call)
		{
			call->AppDataSet ((size_t)0); // Clear the appdata data member
		}
	}
	m_unConferenceRoomCreateRequestId = 0;

	// Log to Splunk that the creation of the Conference Room failed due to not receiving a response
	// from the Conference Server
	stiRemoteLogEventSend ("EventType=GVC Reason=\"ConferenceService-NoResponse\" CoreId=%s",
			userID.c_str ());

	// Inform the UI of the failure
	AppNotify (estiMSG_CONFERENCE_SERVICE_ERROR, 0);
}


/*!
 * \brief Event handler for when the DHVIRoomCreateTimer fires
 */
void CstiCCI::EventDhviRoomCreateTimeout ()
{
	// Cancel the request
	m_pConferenceService->RequestCancel (m_dhviCreateRequestId);

	// Find the call objects that had been waiting and clear the request ID from their object
	std::string userID;

	const ICallInfo *callInfo = nullptr;
	auto call = FindCallByRequestId (m_dhviCreateRequestId);
	if (call)
	{
		call->AppDataSet ((size_t)0); // Clear the appdata data member
		callInfo = call->LocalCallInfoGet ();
		callInfo->UserIDGet (&userID);
		call->dhviStateSet(IstiCall::DhviState::Failed); // We timed out getting a response from Core so this is a failed request.
	}
	m_dhviCreateRequestId = 0;

	// Log to Splunk that the creation of the Conference Room failed due to not receiving a response
	// from the Conference Server
	stiRemoteLogEventSend ("EventType=DHVI Reason=ConferenceService-NoResponse CoreId=%s",
			userID.c_str ());
}


/*!\brief Callback function from a variety of objects
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackRingGroup(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	// Extract data and call AppNotify
	hResult = pThis->AppNotify (n32Message, MessageParam);

	stiTESTRESULT ();

STI_BAIL:
	return hResult;
}


/*!\brief Process the user account information
 *
 */
void CstiCCI::userAccountInfoProcess (
	const CstiUserAccountInfo *userAccountInfo)
{
	if (userAccountInfo)
	{
		if (!userAccountInfo->m_CLFavoritesQuota.empty())
		{
			ContactManagerGet()->FavoritesMaxSet(std::stoi(userAccountInfo->m_CLFavoritesQuota));
		}

		userIdsSet (userAccountInfo->m_userId, userAccountInfo->m_groupUserId);

		m_mustCallCIR = userAccountInfo->m_MustCallCIR == "True";
	}
}


/*!\brief Callback function from a core service
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackCoreService(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventCoreServiceMessage, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief Core Service message handler
 *
 * \return Success or failure as a stiHResult value
 */
void CstiCCI::EventCoreServiceMessage (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bSendToApp = true;

	switch (n32Message)
	{
		case estiMSG_CORE_SERVICE_CONNECTED:
		{
			if (!m_bRequestedPhoneSettings)
			{
				PropertiesCheck (m_PhoneSettings, estiPTypePhone,
								 &m_bRequestedPhoneSettings);
			}

			break;
		}

		case estiMSG_CORE_SERVICE_SSL_FAILOVER:

			m_poPM->propertySet(CoreServiceSSLFailOver, eSSL_OFF);
			m_poPM->PropertySend(CoreServiceSSLFailOver, estiPTypePhone);

			break;

		case estiMSG_CB_PUBLIC_IP_RESOLVED:
		{
			SstiConferenceParams stConferenceParams;

			auto  poResponse = reinterpret_cast<CstiCoreResponse *>(MessageParam);
			if (nullptr != poResponse)
			{
				// Only listen to what core says when we have no proxy to tell us better.
				m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
				auto publicIp = poResponse->publicIPGet();

				m_pConferenceManager->corePublicIpSet (publicIp);

				if (stConferenceParams.eNatTraversalSIP == estiSIPNATDisabled)
				{
					EventPublicIpResolved (publicIp);
				}
			}

			bSendToApp = false;
			break;
		}

		case estiMSG_CB_CURRENT_TIME_SET:
		{
			auto  poResponse = reinterpret_cast<CstiCoreResponse *>(MessageParam);
			if (nullptr != poResponse)
			{
				auto  ttTime = poResponse->timeGet();
				EventCurrentTimeSet (ttTime);
			}
			bSendToApp = false;
			break;
		}

		case estiMSG_CORE_RESPONSE:
		{
			auto  poResponse = (CstiCoreResponse*)MessageParam;

			if (nullptr != poResponse)
			{
				if (poResponse->RequestIDGet () == m_unDirectoryResolveRequestId)
				{
					//
					// Try to lock the mutex.  If we are waiting for it to unlock
					// then we must be in CallDial still.
					//
					std::lock_guard<std::recursive_mutex> lock(m_CallDialMutex);

					// DirectoryResolveReturn frees the response object.
					DirectoryResolveReturn (poResponse);

					bSendToApp = estiFALSE;

					m_pDirectoryResolveCall = nullptr;

					m_unDirectoryResolveRequestId = 0;

				}
				else if (poResponse->RequestIDGet () == m_unPortingLogoutRequestId)
				{
					//
					// We successfully logged out.  Now disable core messages and
					// empty the queue.
					//
					if (!m_pCoreService->PauseGet ())
					{
						m_pCoreService->PauseSet (true);
					}
					m_pCoreService->EmptyQueue ();
				}
				else if (poResponse->RequestIDGet () == m_unUserInterfaceGroupRequestId && poResponse->ResponseGet () == CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT)
				{
					if (poResponse->ResponseResultGet() == estiOK)
					{
						// Get the LocalInterfaceMode from the response
						auto tmp = poResponse->ResponseStringGet(0);
						if (!tmp.empty ())
						{
							LocalInterfaceModeSet ((EstiInterfaceMode)atoi (tmp.c_str ()));
						}

						// Get the LocalFromName from the response
						tmp = poResponse->ResponseStringGet(1);
						if (!tmp.empty ())
						{
							PropertyManager::getInstance()->propertySet (LOCAL_FROM_NAME, tmp, PropertyManager::Persistent);
						}
						else
						{
							// remove the value from the property table
							PropertyManager::getInstance()->removeProperty (LOCAL_FROM_NAME);
						}

						LocalNamesSet ();

						// Get the LocalFromDialString from the response
						auto fromDialString = poResponse->ResponseStringGet(2);
						if (!fromDialString.empty ())
						{
							PropertyManager::getInstance()->propertySet  (LOCAL_FROM_DIAL_STRING,
									fromDialString, PropertyManager::Persistent);
						}
						else
						{
							// remove the value from the property table
							PropertyManager::getInstance()->removeProperty (LOCAL_FROM_DIAL_STRING);
						}

						// Get the LocalFromDialType from the response
						auto fromDialType = poResponse->ResponseStringGet(3);
						if (!fromDialType.empty ()
						&& -1 != atoi (fromDialType.c_str ()))
						{
							PropertyManager::getInstance()->propertySet (LOCAL_FROM_DIAL_TYPE, atoi(fromDialType.c_str ()), PropertyManager::Persistent);
						}
						else
						{
							// remove the value from the property table
							PropertyManager::getInstance()->removeProperty (LOCAL_FROM_DIAL_TYPE);
						}

						if (!fromDialString.empty ()
						&& !fromDialType.empty ())
						{
							LocalReturnCallInfoSet (
								(EstiDialMethod)atoi(fromDialType.c_str ()), // The dial method to use for the return call
								fromDialString);	// The dial string to dial on the return call
						}
						else
						{
							LocalReturnCallInfoSet (estiDM_UNKNOWN, "");
						}
					}

					m_unUserInterfaceGroupRequestId = 0;

					// Check to see if the AutoSpeed properties are in local storage or not.
					EstiAutoSpeedMode eAutoSpeedMode {stiAUTO_SPEED_MODE_DEFAULT};

					if (PropertyManager::getInstance()->propertyGet (AUTO_SPEED_MODE, &eAutoSpeedMode, PropertyManager::Persistent) == XM_RESULT_NOT_FOUND)
					{
						// Since it isn't found in our local property table, we need to set it and send it to core.
						m_poPM->propertySet (AUTO_SPEED_MODE, eAutoSpeedMode, PropertyManager::Persistent);
						m_poPM->PropertySend(AUTO_SPEED_MODE, estiPTypePhone);
					}

					// Clear the porting back status.
					m_portingBack = false;

					bSendToApp = estiFALSE;
				}

				else if (poResponse->RequestIDGet () == m_unRegistrationInfoRequestId && poResponse->ResponseGet () == CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT)
				{
					if (poResponse->ResponseResultGet() == estiOK)
					{
						//We got a registration now update the manager with it.
						if (m_pRegistrationManager->RegistrationsUpdate (poResponse))
						{
							m_pRegistrationManager->PhoneNumbersSet (&m_sUserPhoneNumbers);
							RegistrationConferenceParamsSet ();
						}
					}

					m_unRegistrationInfoRequestId = 0;

					bSendToApp = estiFALSE;
				}
				
				else if (poResponse->RequestIDGet () == m_unRingGroupInfoRequestId && poResponse->ResponseGet () == CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT)
				{
					if (m_pRingGroupManager)
					{
						m_pRingGroupManager->CoreResponseHandle (poResponse);
					}
					m_unRingGroupInfoRequestId = 0;
					
					bSendToApp = estiFALSE;
				}
				else if (
					(m_pBlockListManager && m_pBlockListManager->CoreResponseHandle (poResponse))
				 || (m_pRingGroupManager && m_pRingGroupManager->CoreResponseHandle (poResponse))
				 || (m_pContactManager && m_pContactManager->CoreResponseHandle (poResponse))
				 || m_userAccountManager->coreResponseHandle(poResponse)
#ifdef stiLDAP_CONTACT_LIST
				 || (m_pLDAPDirectoryContactManager && m_pLDAPDirectoryContactManager->CoreResponseHandle (poResponse))
#endif
#ifdef stiIMAGE_MANAGER
				 || (m_pImageManager && m_pImageManager->CoreResponseHandle (poResponse))
#endif
#ifdef stiMESSAGE_MANAGER
				 || (m_pMessageManager && m_pMessageManager->CoreResponseHandle (poResponse))
#endif
#ifdef stiCALL_HISTORY_MANAGER
				 || (m_pCallHistoryManager && m_pCallHistoryManager->CoreResponseHandle (poResponse))
#endif
				 )
				{
					// This message was handled by the BlockListManager
					bSendToApp = estiFALSE;
					MessageParam = 0;
				}
				else
				{
					switch (poResponse->ResponseGet ())
					{
						case CstiCoreResponse::eSERVICE_CONTACTS_GET_RESULT:
						{
							//
							// Store the service contacts in persistent storage
							//
							std::string ServiceContacts[MAX_SERVICE_CONTACTS];

							poResponse->CoreServiceContactsGet (ServiceContacts);

							if (!ServiceContacts[0].empty ())
							{
								m_poPM->propertySet (cmCORE_SERVICE_URL1, ServiceContacts[0], PropertyManager::Persistent);
							}

							if (!ServiceContacts[1].empty ())
							{
								m_poPM->propertySet (cmCORE_SERVICE_ALT_URL1, ServiceContacts[1], PropertyManager::Persistent);
							}

							poResponse->NotifyServiceContactsGet (ServiceContacts);

							if (!ServiceContacts[0].empty ())
							{
								m_poPM->propertySet (cmSTATE_NOTIFY_SERVICE_URL1, ServiceContacts[0], PropertyManager::Persistent);
							}

							if (!ServiceContacts[1].empty ())
							{
								m_poPM->propertySet (cmSTATE_NOTIFY_SERVICE_ALT_URL1, ServiceContacts[1], PropertyManager::Persistent);
							}

							poResponse->MessageServiceContactsGet (ServiceContacts);

							if (!ServiceContacts[0].empty ())
							{
								m_poPM->propertySet (cmMESSAGE_SERVICE_URL1, ServiceContacts[0], PropertyManager::Persistent);
							}

							if (!ServiceContacts[1].empty ())
							{
								m_poPM->propertySet (cmMESSAGE_SERVICE_ALT_URL1, ServiceContacts[1], PropertyManager::Persistent);
							}

							poResponse->RemoteLoggerServiceContactsGet (ServiceContacts);

							if (!ServiceContacts[0].empty ())
							{
								m_poPM->propertySet (REMOTELOGGER_SERVICE_URL1, ServiceContacts[0], PropertyManager::Persistent);
							}

							if (!ServiceContacts[1].empty ())
							{
								m_poPM->propertySet (REMOTELOGGER_SERVICE_ALT_URL1, ServiceContacts[1], PropertyManager::Persistent);
							}


							poResponse->ConferenceServiceContactsGet (ServiceContacts);

							if (!ServiceContacts[0].empty ())
							{
								m_poPM->propertySet (CONFERENCE_SERVICE_URL1, ServiceContacts[0], PropertyManager::Persistent);
							}

							if (!ServiceContacts[1].empty ())
							{
								m_poPM->propertySet (CONFERENCE_SERVICE_ALT_URL1, ServiceContacts[1], PropertyManager::Persistent);
							}

							bSendToApp = estiFALSE;

							break;
						}

						case CstiCoreResponse::eUSER_DEFAULTS_RESULT:
						{
							SettingsGetProcess (poResponse);

							auto settingList = poResponse->SettingListGet ();

							for (auto &setting: settingList)
							{
								PropertyManager::getInstance ()->PropertySend (setting.Name, estiPTypeUser);
							}

							break;
						}

						case CstiCoreResponse::ePHONE_SETTINGS_GET_RESULT:
						case CstiCoreResponse::eUSER_SETTINGS_GET_RESULT:

							SettingsGetProcess (poResponse);

							break;

						case CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT:
						case CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT:
						{
							userAccountInfoProcess (poResponse->UserAccountInfoGet());

							// The password has (or may have) changed, therefore we need to re-request credentials
							auto  poCoreRequest = new CstiCoreRequest ();
							poCoreRequest->registrationInfoGet ();
							CoreRequestSend (poCoreRequest, &m_unRegistrationInfoRequestId);

							break;
						}
						case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
						{
							m_poPM->removeProperty (CORE_AUTH_TOKEN);

							m_pConferenceManager->LoggedInSet(false);
							ConferencingReadyStateSet (estiConferencingStateNotReady);

							stiRemoteLogEventSend ("EventType=CoreLogin Reason=LogoutResponse");

							break;
						}

						default:

							//
							// Do nothing
							//

							break;
					}

					if (poResponse)
					{
						//
						// Tell the property manager of the response.
						//
						PropertyManager::getInstance()->ResponseCheck (poResponse);

						//
						// If this is a core response to a request that the engine issued then
						// delete the response and don't tell the UI.
						//
						std::set <unsigned int>::iterator i;

						i = m_PendingCoreRequests.find (poResponse->RequestIDGet ());

						if (i != m_PendingCoreRequests.end ())
						{
							m_PendingCoreRequests.erase (i);

							bSendToApp = estiFALSE;
						}
					}
				}
			}

			break;
		}

		case estiMSG_CORE_REQUEST_REMOVED:
		{
			auto removedResponse = reinterpret_cast<CstiVPServiceResponse *>(MessageParam);

			if (m_pBlockListManager->RemovedUponCommunicationErrorHandle (removedResponse)
			 || m_pRingGroupManager->RemovedUponCommunicationErrorHandle (removedResponse)
			 || m_pContactManager->RemovedUponCommunicationErrorHandle (removedResponse)
#ifdef stiCALL_HISTORY_MANAGER
			 || m_pCallHistoryManager->RemovedUponCommunicationErrorHandle (removedResponse)
#endif
			 )
			{
				bSendToApp = estiFALSE;
			}
			else
			{
				//
				// Try to lock the mutex.  If we are waiting for it to unlock
				// then we must be in CallDial still.
				//
				std::lock_guard<std::recursive_mutex> lock(m_CallDialMutex);

				if (removedResponse != nullptr)
				{
					if (removedResponse->RequestIDGet() == m_unDirectoryResolveRequestId)
					{
						DirectoryResolveRemoved (removedResponse);

						bSendToApp = estiFALSE;

						m_pDirectoryResolveCall = nullptr;

						m_unDirectoryResolveRequestId = 0;
					}
				}

			}

			break;
		}

		// All remaining messages, send on to AppNotify
		default:
		{
			break;
		}
	}

	if (bSendToApp)
	{
		// Extract data and call AppNotify
		hResult = AppNotify(n32Message, MessageParam);
		stiTESTRESULT ();
	}
	else
	{
		MessageCleanup (n32Message, MessageParam);
	}

STI_BAIL:
	return;
}


/*!\brief Callback function from state notify
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackStateNotify(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventStateNotifyMessage, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief State Notify message handler
 *
 * \return Success or failure as a stiHResult value
 */
void CstiCCI::EventStateNotifyMessage (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bSendToApp = true;

	switch (n32Message)
	{
		case estiMSG_STATE_NOTIFY_SSL_FAILOVER:

			m_poPM->propertySet(StateNotifySSLFailOver, eSSL_OFF);
			m_poPM->PropertySend(StateNotifySSLFailOver, estiPTypePhone);

			break;

		case estiMSG_CB_CURRENT_TIME_SET:
		{
			auto  poResponse = reinterpret_cast<CstiStateNotifyResponse *>(MessageParam);
			if (nullptr != poResponse)
			{
				auto  ttTime = poResponse->timeGet();
				EventCurrentTimeSet (ttTime);
			}
			bSendToApp = false;
			break;
		}

		case estiMSG_STATE_NOTIFY_RESPONSE:
		{
			auto  poResponse = reinterpret_cast<CstiStateNotifyResponse *>(MessageParam);

			// If we are in ported mode trap all notify events and look for the Login event.
			if (estiPORTED_MODE == m_eLocalInterfaceMode)
			{
				// Only send the eLOGIN to the UI delete all other events.
				if (poResponse->ResponseGet() != CstiStateNotifyResponse::eLOGIN)
				{
					bSendToApp = false;
				}
				break;
			}

			m_userAccountManager->stateNotifyEventHandle (poResponse);

			if (
				(m_pBlockListManager && m_pBlockListManager->StateNotifyEventHandle (poResponse))
			 || (m_pRingGroupManager && m_pRingGroupManager->StateNotifyEventHandle (poResponse))
			 || (m_pContactManager && m_pContactManager->StateNotifyEventHandle (poResponse))
#ifdef stiMESSAGE_MANAGER
			 || (m_pMessageManager && m_pMessageManager->StateNotifyEventHandle (poResponse))
#endif
#ifdef stiCALL_HISTORY_MANAGER
			 || (m_pCallHistoryManager && m_pCallHistoryManager->StateNotifyEventHandle (poResponse))
#endif
			 )
			{
				bSendToApp = estiFALSE;
				MessageParam = 0; // response deleted by one of the above managers.
			}
			else
			{
				switch (poResponse->ResponseGet ())
				{
					case CstiStateNotifyResponse::eDIAGNOSTICS_GET:
						DiagnosticsSend ();

						bSendToApp = false;

						break;

					case CstiStateNotifyResponse::ePUBLIC_IP_SETTINGS_CHANGED:
					{
						// Tell the Core Service to re-request the public IP
						auto  poCoreRequest = new CstiCoreRequest;

						if (poCoreRequest != nullptr)
						{
							poCoreRequest->phoneSettingsGet (cmPUBLIC_IP_ASSIGN_METHOD);
							poCoreRequest->phoneSettingsGet (cmPUBLIC_IP_ADDRESS_MANUAL);

							stiHResult hResult = m_pCoreService->RequestSendAsync (poCoreRequest,
															CoreRequestAsyncCallback, (size_t)this);

							if (stiIS_ERROR (hResult))
							{
								delete poCoreRequest;
								poCoreRequest = nullptr;
							}
						} // end if

						break;
					}

					case CstiStateNotifyResponse::eUSER_INTERFACE_GROUP_CHANGED:  //rgh
					{
						auto  poCoreRequest = new CstiCoreRequest ();
						poCoreRequest->userInterfaceGroupGet ();
						CoreRequestSend (poCoreRequest, &m_unUserInterfaceGroupRequestId);

						bSendToApp = estiFALSE;

						break;
					}

					case CstiStateNotifyResponse::eREGISTRATION_INFO_CHANGED:
					{
						// The registration info has changed, therefore we need to re-request credentials
						auto  poCoreRequest = new CstiCoreRequest ();
						poCoreRequest->registrationInfoGet ();
						CoreRequestSend (poCoreRequest, &m_unRegistrationInfoRequestId);

						bSendToApp = estiFALSE;

						break;
					}

					case CstiStateNotifyResponse::eCLIENT_REREGISTER:
					{
						m_pConferenceManager->ClientReregister();

						bSendToApp = estiFALSE;

						break;
					}

					case CstiStateNotifyResponse::eLOGGED_OUT:
					{
						m_pConferenceManager->LoggedInSet(false);
						ConferencingReadyStateSet (estiConferencingStateNotReady);

						stiRemoteLogEventSend ("EventType=CoreLogin Reason=StateNotifyLoggedOutEvent");

						break;
					}
						
					case CstiStateNotifyResponse::eHEARTBEAT_RESULT:
					{
						// Checks that endpoints that should be registerd still are.
						m_pConferenceManager->RegistrationStatusCheck (m_nStackRestartDelta);

						break;
					}

					default:

						break;
				}
			}

			break;
		}

		// All remaining messages, send on to AppNotify
		default:
		{
			break;
		}
	}

	if (bSendToApp)
	{
		// Extract data and call AppNotify
		hResult = AppNotify(n32Message, MessageParam);
	}
	else
	{
		MessageCleanup (n32Message, MessageParam);
	}

	stiTESTRESULT ();

STI_BAIL:
	return;
}


/*!\brief Callback function from the message service
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackMessageService(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventMessageServiceMessage, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief Message Service message handler
 *
 * \return Success or failure as a stiHResult value
 */
void CstiCCI::EventMessageServiceMessage (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bSendToApp = true;

	switch (n32Message)
	{
		case estiMSG_MESSAGE_SERVICE_SSL_FAILOVER:

			m_poPM->propertySet(MessageServiceSSLFailOver, eSSL_OFF);
			m_poPM->PropertySend(MessageServiceSSLFailOver, estiPTypePhone);

			break;

		case estiMSG_MESSAGE_RESPONSE:
		{
			auto  poResponse = (CstiMessageResponse*)MessageParam;
#ifdef stiMESSAGE_MANAGER
			if (m_pMessageManager && estiTRUE == m_pMessageManager->MessageResponseHandle(poResponse))
			{
				bSendToApp = estiFALSE;
				MessageParam = 0; // response deleted by message manager.
			}
			else if (poResponse->RequestIDGet () == m_unVideoMessageGUIDRequestId)
#else
			if (poResponse->RequestIDGet () == m_unVideoMessageGUIDRequestId)
#endif
			{
				std::string fileGUID;
				bool downloadResponse = true;

				uint32_t recvSpeed = 0;
				uint32_t sendSpeed = 0;
				stiHResult hResult2 = MaxSpeedsGet(&recvSpeed, &sendSpeed);

				if (stiIS_ERROR(hResult2))
				{
					stiASSERTMSG(estiFALSE, "Failed to obtain send and receive speeds");
				}

				bSendToApp = estiFALSE;
				switch(poResponse->ResponseGet())
				{
					case CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_GET_RESULT:
					{
						if(poResponse->ResponseResultGet() == estiOK)
						{
							fileGUID = poResponse->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_GUID);
						}
						else
						{
							m_pFilePlay->LogFileInitializeError(IstiMessageViewer::estiERROR_REQUESTING_GUID);
						}
						break;
					}
					case CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_LIST_GET_RESULT:
					{
						if(poResponse->ResponseResultGet() == estiOK)
						{
							auto messageList = *poResponse->MessageDownloadUrlListGet();
							if (!messageList.empty())
							{
								int fileGuidHeight = 0;

								for (auto &it: messageList)
								{
									// If HEVC is supported it will be the first GUID in the list.  Use the first entry in
									// the list.  Then search the list for a better video.  We can't use bitrate because
									// all other encodings types will have a higher bitrate.  Compare the height of the 
									// GUID we have to the next one.  If it is larger then assume it is a better resolution
									// and use that GUID. 
									if (fileGUID.empty() ||
										 ((uint32_t)it.m_nMaxBitRate <= recvSpeed &&
										 it.m_nHeight > fileGuidHeight))
									{
										fileGUID = it.m_DownloadURL; 
										fileGuidHeight = it.m_nHeight;
									}
								}
							}
							else
							{
								stiASSERTMSG(estiFALSE, "MessageDownloadUrlListGet returned an empty list");
							}
						}
						else
						{
							m_pFilePlay->LogFileInitializeError(IstiMessageViewer::estiERROR_REQUESTING_GUID);
						}
						break;
					}

					case CstiMessageResponse::eMESSAGE_UPLOAD_URL_GET_RESULT:
					{
						m_uploadGUIDRequestTimer.stop ();
						downloadResponse = false;

						if (poResponse->ResponseResultGet() == estiOK)
						{
							const std::string mailBoxFull = poResponse->ResponseStringGet (CstiMessageResponse::eRESPONSE_STRING_SERVER);

							std::lock_guard<std::recursive_mutex> lock(m_LeaveMessageMutex);

							if (m_spLeaveMessageCall)
							{
								if (m_spLeaveMessageCall->SubstateValidate(estiSUBSTATE_LEAVE_MESSAGE) ||
									m_spLeaveMessageCall->SubstateValidate(estiSUBSTATE_MESSAGE_COMPLETE))
								{
									// Get the message info from the call object.
									SstiMessageInfo stMessageInfo;
									hResult = m_spLeaveMessageCall->MessageInfoGet (&stMessageInfo);
									if (!stiIS_ERROR (hResult))
									{
										// Check to see if the mailbox is full.
										if (mailBoxFull == "False")
										{
											fileGUID = poResponse->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_GUID);
										}
										else
										{
											stMessageInfo.bMailBoxFull = estiTRUE;
											m_spLeaveMessageCall->MessageInfoSet (&stMessageInfo);

											if (m_spLeaveMessageCall->LeaveMessageGet() == estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD ||
												m_spLeaveMessageCall->MessageGreetingTypeGet() == eGREETING_NONE ||
												m_pFilePlay->ErrorGet() == IstiMessageViewer::estiERROR_RECORD_CONFIG_INCOMPLETE)
											{
												RecordMessageMailBoxFull(m_spLeaveMessageCall);
											}

											if (m_spLeaveMessageCall->LeaveMessageGet() == estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD &&
												estiINTERPRETER_MODE != m_eLocalInterfaceMode &&
												estiVRI_MODE != m_eLocalInterfaceMode)
											{
												// The countdown was told to start so stop it from playing.
												m_pFilePlay->Close();
											}

										}
									}
								}
								else
								{
									stiASSERT (estiFALSE);
								}
							}
							else
							{
								stiASSERT (estiFALSE);
							}
						}
						break;
					}

					default:
					{
						AppNotify (estiMSG_SIGNMAIL_UPLOAD_URL_GET_FAILED, 0);
						stiASSERTMSG(false, "SignMail upload URL get request failed. Error: 0x%x", poResponse->ErrorGet());
					}
				}

				if (!fileGUID.empty())
				{
					char serverIP[un8stiMAX_URL_LENGTH + 1];
					char serverPort[20];
					char fileUrl[un16stiMAX_URL_LENGTH + 1];
					char serverName[un8stiMAX_URL_LENGTH + 1];

					EURLParseResult eURLResult = stiURLParse (fileGUID.c_str(),
															  serverIP, sizeof(serverIP),
															  serverPort, sizeof(serverPort),
															  fileUrl, sizeof(fileUrl),
															  serverName, sizeof(serverName));
					if (eURLResult == eURL_OK)
					{
						if (downloadResponse)
						{
							m_unVideoMessageGUIDRequestId = 0;
							uint64_t fileSize = 0;

							stiDEBUG_TOOL(g_stiUiEventDebug, 
										  stiTrace("Have video message GUID\n"););

							auto viewId = poResponse->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_RETRIEVAL_METHOD);

							fileSize = poResponse->ResponseValueGet();
							auto clientToken = poResponse->clientTokenGet();

							EstiBool bPause = estiFALSE;

							// If we have a call being negotiated, pause the video message
							if (0 < CallObjectsCountGet ())
							{
								bPause = estiTRUE;
							}
							m_pFilePlay->LoadVideoMessage (serverName,
														   fileUrl,
														   viewId,
														   fileSize,
														   clientToken,
														   bPause);
						} 
						else
						{
							int remoteDownloadSpeed = 0;
							int recordTime = 0;
							std::string dialString;

							// We now have the GUID so pass call info to the fileplayer
							// so that it can record a message.
							recordTime = m_spLeaveMessageCall->MessageRecordTimeGet();
							remoteDownloadSpeed = m_spLeaveMessageCall->RemoteDownloadSpeedGet();
							hResult = MaxSpeedsGet(&recvSpeed, &sendSpeed);
							m_spLeaveMessageCall->RemoteDialStringGet(&dialString);
							m_pFilePlay->RecordP2PMessageInfoSet(recordTime,
																 remoteDownloadSpeed,
																 sendSpeed,
																 serverName,
																 fileUrl,
																 dialString);

							// If we are skipping the greeting and countdown then start recording.
							if (m_spLeaveMessageCall->LeaveMessageGet() != estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD)
							{
								SstiMessageInfo stMessageInfo;
								m_spLeaveMessageCall->MessageInfoGet(&stMessageInfo);

								if (!stMessageInfo.bCountdownEmbedded || m_spLeaveMessageCall->directSignMailGet())
								{
									m_pFilePlay->LoadCountdownVideo(m_spLeaveMessageCall->MessageGreetingTypeGet() != eGREETING_NONE ? estiTRUE : estiFALSE);
								}
							}
						}
					}
					else if (downloadResponse)
					{
						m_pFilePlay->LogFileInitializeError (IstiMessageViewer::estiERROR_PARSING_GUID);
					}
				}
			}
			else if (poResponse->RequestIDGet () == m_unSignMailSendRequestId)
			{
				bSendToApp = estiFALSE;
				if (poResponse->ResponseResultGet() == estiERROR)
				{
					AppNotify(estiMSG_RECORD_MESSAGE_SEND_FAILED, 0);
					stiASSERTMSG(false, "SignMail send failed.");
				}
				else
				{
					AppNotify(estiMSG_RECORD_MESSAGE_SEND_SUCCESS, 0);
				}
			}
			else if (poResponse->RequestIDGet () == m_unGreetingInfoRequestId)
			{
				bSendToApp = estiFALSE;
				if (poResponse->ResponseResultGet () == estiERROR)
				{
					AppNotify (estiMSG_GREETING_INFO_REQUEST_FAILED, 0);
				}
				else if (poResponse->ResponseGet() == CstiMessageResponse::eGREETING_UPLOAD_URL_GET_RESULT)
				{
					if (poResponse->ResponseResultGet() == estiOK)
					{
						auto uploadGUID = poResponse->ResponseStringGet (CstiMessageResponse::eRESPONSE_STRING_GUID);

						char serverIP[un8stiMAX_URL_LENGTH + 1];
						char serverPort[20];
						char uploadUrl[un16stiMAX_URL_LENGTH + 1];
						char serverName[un8stiMAX_URL_LENGTH + 1];

						if (eURL_OK == stiURLParse (uploadGUID.c_str (),
							serverIP, sizeof (serverIP),
							serverPort, sizeof (serverPort),
							uploadUrl, sizeof (uploadUrl),
							serverName, sizeof (serverName)))
						{
							m_pFilePlay->GreetingUploadInfoSet(uploadUrl, serverName);
						}
					}
				}
				else if (poResponse->ResponseGet() == CstiMessageResponse::eGREETING_INFO_GET_RESULT)
				{
					// Create a new vector that can be passed to the UI. The one in the message response
					// will be deleted.
					auto pGreetingInfoList =
						new std::vector <CstiMessageResponse::SGreetingInfo> (*(poResponse->GreetingInfoListGet()));

					// Pass the greeting info to the app.
					AppNotify(estiMSG_GREETING_INFO,
							  reinterpret_cast<size_t>(pGreetingInfoList));
				}
			}
			break;
		}

		case estiMSG_MESSAGE_REQUEST_REMOVED:
		{
			auto removedResponse = reinterpret_cast<CstiVPServiceResponse *>(MessageParam);
			if (removedResponse != nullptr)
			{
				if (removedResponse->RequestIDGet() == m_unVideoMessageGUIDRequestId)
				{
					m_pFilePlay->LogFileInitializeError(IstiMessageViewer::estiERROR_REQUESTING_GUID);
					bSendToApp = estiFALSE;
				}
				else if (removedResponse->RequestIDGet() == m_unSignMailSendRequestId)
				{
					bSendToApp = estiFALSE;
					AppNotify(estiMSG_RECORD_MESSAGE_SEND_FAILED, 0);
				}
				else if (removedResponse->RequestIDGet() == m_unGreetingInfoRequestId)
				{
					bSendToApp = estiFALSE;
					AppNotify(estiMSG_GREETING_INFO_REQUEST_FAILED, 0);
				}
			}

			break;
		}

		// All remaining messages, send on to AppNotify
		default:
		{
			break;
		}
	}

	if (bSendToApp)
	{
		AppNotify(n32Message, MessageParam);
	}
	else
	{
		MessageCleanup (n32Message, MessageParam);
	}
}


/*!\brief Callback function from the conference service
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackConferenceService(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventConferenceServiceMessage, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief Conference Service message handler
 *
 * \return Success or failure as a stiHResult value
 */
void CstiCCI::EventConferenceServiceMessage (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG (hResult);
	bool bSendToApp = true;

	switch (n32Message)
	{
		case estiMSG_CONFERENCE_SERVICE_SSL_FAILOVER:

			m_poPM->propertySet(ConferenceServiceSSLFailOver, eSSL_OFF);
			m_poPM->PropertySend(ConferenceServiceSSLFailOver, estiPTypePhone);

			break;

		case estiMSG_CONFERENCE_RESPONSE:
		{
			bSendToApp = false;

			auto  poResponse = (CstiConferenceResponse*)MessageParam;
			if (poResponse->ResponseGet () == CstiConferenceResponse::eCONFERENCE_ROOM_CREATE_RESULT)
			{
				const ICallInfo *pCallInfo = nullptr;
				std::string userID;

				if (m_unConferenceRoomCreateRequestId == poResponse->RequestIDGet())
				{
					// Cancel the timer
					m_groupVideoChatRoomCreateTimer.stop ();

					if (poResponse->ResponseResultGet () == estiOK)
					{
						std::string DialString = poResponse->ResponseStringGet (1);
						std::string ConferenceId = poResponse->ResponseStringGet (0);

						//
						// Find the calls matching this request id.
						//
						auto call1 = FindCallByRequestId (poResponse->RequestIDGet());
						stiTESTCOND (call1, stiRESULT_ERROR);
						call1->AppDataSet ((size_t)0); // Clear the appdata data member

						auto call2 = FindCallByRequestId (poResponse->RequestIDGet());
						if (!call2)
						{
							call1 = nullptr;
							stiTHROW (stiRESULT_ERROR);
						}

						call2->AppDataSet ((size_t)0); // Clear the appdata data member

						pCallInfo = call1->LocalCallInfoGet ();
						pCallInfo->UserIDGet (&userID);

						// Log to Splunk the successful creation of the Conference Room
						stiRemoteLogEventSend ("EventType=GVC Reason=\"RoomCreated\" GUID=%s CoreId=%s",
								ConferenceId.c_str (),
								userID.c_str ());

						m_pConferenceManager->ConferenceJoin (call1, call2, DialString);
					}
					else
					{
						//
						// Find the calls matching this request id.
						//
						auto call1 = FindCallByRequestId (poResponse->RequestIDGet());
						if (call1)
						{
							call1->AppDataSet ((size_t)0); // Clear the appdata data member
						}

						auto call2 = FindCallByRequestId (poResponse->RequestIDGet());
						if (call2)
						{
							call2->AppDataSet ((size_t)0); // Clear the appdata data member
						}

						if (call1)
						{
							pCallInfo = call1->LocalCallInfoGet();
							pCallInfo->UserIDGet(&userID);
						}
						else if (call2)
						{
							pCallInfo = call2->LocalCallInfoGet();
							pCallInfo->UserIDGet(&userID);
						}

						CstiConferenceResponse::EResponseError responseError = poResponse->ErrorGet ();

						switch (responseError)
						{
							case CstiConferenceResponse::eRESPONSE_INSUFFICIENT_RESOURCES:
								AppNotify (estiMSG_CONFERENCE_SERVICE_INSUFFICIENT_RESOURCES, 0);

								// Log to Splunk that the creation of the Conference Room failed due to resources
								stiRemoteLogEventSend ("EventType=GVC Reason=\"CreateFailed-Resources\" CoreId=%s",
										userID.c_str ());
								break;

							default:
								AppNotify(estiMSG_CONFERENCE_SERVICE_ERROR, 0);

								// Log to Splunk that the creation of the Conference Room failed
								stiRemoteLogEventSend("EventType=GVC Reason=\"CreateFailed\" Error=%d CoreId=%s",
									(int)responseError,
									userID.c_str ());
								break;
						}
					}

					// Reset this request ID now
					m_unConferenceRoomCreateRequestId = 0;
				}
				else if (m_dhviCreateRequestId == poResponse->RequestIDGet())
				{
					m_dhviRoomCreateTimer.stop ();
					auto call = FindCallByRequestId (poResponse->RequestIDGet());
					
					if (call)
					{
						if (call->dhviStateGet()== IstiCall::DhviState::Connecting)
						{
							if (poResponse->ResponseResultGet () == estiOK)
							{
								std::string DialString = poResponse->ResponseStringGet (1);
								std::string ConferenceId = poResponse->ResponseStringGet (0);
								call->backgroundGroupCallCreate (DialString, ConferenceId);
							}
							else
							{
								stiRemoteLogEventSend ("EventType=DHVI Reason=\"Room Create Failed\" Error=\"%d\"", poResponse->ErrorGet ());
								call->dhviStateSet(IstiCall::DhviState::Failed);
							}
						}
						call->AppDataSet ((size_t)0); // Clear the appdata data member
					}
					
					m_dhviCreateRequestId = 0;
				}
			}

			else if (poResponse->ResponseGet () == CstiConferenceResponse::eCONFERENCE_ROOM_STATUS_GET_RESULT)
			{
				if (poResponse->ResponseResultGet () == estiOK)
				{
					auto requestId = poResponse->RequestIDGet ();
					auto call = FindCallByRequestId (requestId);

					if (call)
					{
						if (call->StateGet () != esmiCS_DISCONNECTED)
						{
							bool bLackOfResources = false;
							bool bPreviousAddAllowed = call->ConferenceRoomParticipantAddAllowed (&bLackOfResources);

							call->ConferenceRoomStatsSet (
								poResponse->AddAllowedGet (),
								poResponse->ActiveParticipantsGet (),
								poResponse->AllowedParticipantsGet (),
								poResponse->ParticipantListGet ());

							if (call->ConferenceRoomParticipantAddAllowed (&bLackOfResources) != bPreviousAddAllowed)
							{
								AppNotify (estiMSG_CONFERENCE_ROOM_ADD_ALLOWED_CHANGED, (size_t)call.get());
							}

							// If there is only one person in the conference room (it must be me :D ),
							// drop the call but only if there had been more than one at some point.

							if (1 >= poResponse->ActiveParticipantsGet () && 1 < call->ConferenceRoomPeakParticipantsGet ())
							{
								call->HangUp ();
							}
						}
					}
				}

				// Start a timer again
				m_groupVideoChatRoomStatsTimer.restart ();
			}

			break;
		}

		// All remaining messages, send on to AppNotify
		default:
		{
			break;
		}
	}

STI_BAIL:

	if (bSendToApp)
	{
		hResult = AppNotify(n32Message, MessageParam);
	}
	else
	{
		MessageCleanup(n32Message, MessageParam);
	}
}


/*!\brief Callback function from the remote logger service
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackRemoteLoggerService(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventRemoteLoggerServiceMessage, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief Remote Logger Service message handler
 * \Turn on\off remote logging
 */
void CstiCCI::EventRemoteLoggerServiceMessage (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	switch (n32Message)
	{
		case estiMSG_REMOTELOGGER_SERVICE_SSL_FAILOVER:

			m_poPM->propertySet(RemoteLoggerServiceSSLFailOver, eSSL_OFF);
			m_poPM->PropertySend(RemoteLoggerServiceSSLFailOver, estiPTypePhone);

			break;

		default:

			break;
	}

	// Extract data and call AppNotify
	hResult = AppNotify(n32Message, MessageParam);
	stiTESTRESULT ();

STI_BAIL:
	return;
}


/*!\brief Callback function from the http object
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallbackHTTP(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventHTTPMessage, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief HTTP message handler
 */
void CstiCCI::EventHTTPMessage (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Extract data and call AppNotify
	hResult = AppNotify (n32Message, MessageParam);
	stiTESTRESULT ();

STI_BAIL:
	return;
}


/*!\brief Callback function from a variety of objects
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiCCI::ThreadCallback(
	int32_t n32Message, 	///< holds the type of message
	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam,	///< points to the instantiated CCI object
	size_t CallbackFromId)
{
	// retrieve pointer to CCI object from pParam
	auto pThis = (CstiCCI *)CallbackParam;

	pThis->PostEvent (
		std::bind(&CstiCCI::EventThreadCallback, pThis, n32Message, MessageParam));

	return stiRESULT_SUCCESS;
}


/*!\brief Generic event handler for thread callback messages
 *
 */
void CstiCCI::EventThreadCallback (
	int32_t n32Message,
	size_t MessageParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	switch (n32Message)
	{
		case estiMSG_CB_PUBLIC_IP_RESOLVED:
		{
			auto pIpAddress = (std::string *)MessageParam;
			if (pIpAddress)
			{
				EventPublicIpResolved (*pIpAddress);
				delete pIpAddress;
				pIpAddress = nullptr;
			}
			break;
		}

		// All remaining messages, send on to AppNotify
		default:
		{
			// Extract data and call AppNotify
			hResult = AppNotify(n32Message, MessageParam);
			break;
		}
	}

	stiTESTRESULT ();

STI_BAIL:
	return;
}


/*!
 * \brief Event handler for when the remote ring count changes
 * \param ringCount
 */
void CstiCCI::EventRemoteRingCountReceived (int ringCount)
{
	auto hResult = stiRESULT_SUCCESS;
	int32_t message = estiMSG_REMOTE_RING_COUNT;

	// If SignMail is not enabled, just notify the application of the ringcount
	if (!m_bSignMailEnabled)
	{
		AppNotify (message, (size_t)ringCount);
		return;
	}

	// Otherwise, if we've reached our max ring count, hangup the call.
	// This will cause conference manager to signal that we can leave a message.
	auto call = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTING);
	if (call)
	{
		if (call->SubstateValidate(estiSUBSTATE_WAITING_FOR_REMOTE_RESP))
		{
			SstiMessageInfo stMessageInfo;
			hResult = call->MessageInfoGet (&stMessageInfo);
			if (!stiIS_ERROR (hResult))
			{
				if (ringCount == 2 &&
					!call->MessageGreetingURLGet().empty () &&
					call->MessageGreetingTypeGet() != eGREETING_TEXT_ONLY)
				{
					hResult = LoadVideoGreeting(call, estiTRUE);
				}

				if (ringCount > stMessageInfo.nMaxNumbOfRings)
				{
					call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
					call->HangUp();
				}
			}
		}
	}

	AppNotify(message, (size_t)ringCount);
}


/*!
 * \brief Event handler for when the local ring count changes
 * \param ringCount
 */
void CstiCCI::EventLocalRingCountReceived (uint32_t ringCount)
{
	auto call = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTING);
	if (call)
	{
		if (call->SubstateValidate(estiSUBSTATE_WAITING_FOR_USER_RESP))
		{
			uint32_t currentRingCount = 0;
			uint32_t maxRingCount = 0;
			RingsBeforeGreetingGet(&currentRingCount, &maxRingCount);

			// If the number of rings that have occured is greater than the phones
			// ring count setting we will reject the call.
			if (ringCount > currentRingCount)
			{
				call->Reject(estiRESULT_LOCAL_SYSTEM_BUSY);
			}
		}
	}

	// TODO:  Why do we ifdef this?
	// Why don't applications that don't care about it just ignore it?
#if APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_MAC
	AppNotify(estiMSG_LOCAL_RING_COUNT, (size_t)ringCount);
#endif
}


/*!
 * \brief Event handler for when conference manager notifies us the conference
 *        addresses have changed
 * \param addresses
 */
void CstiCCI::EventSipMessageConfAddressChanged (
	SstiConferenceAddresses addresses)
{
	if (m_SIPConferenceAddresses != addresses)
	{
		m_SIPConferenceAddresses.SIPPrivateAddress = addresses.SIPPrivateAddress;
		m_SIPConferenceAddresses.SIPPublicAddress = addresses.SIPPublicAddress;
		m_SIPConferenceAddresses.nSIPPrivatePort = addresses.nSIPPrivatePort;
		m_SIPConferenceAddresses.nSIPPublicPort = addresses.nSIPPublicPort;

		UriListSend();
	}
}


/*!
 * \brief Event handler for when conference manager notifies us sip registration
 *        was confirmed
 * \param proxyIpAddress
 */
void CstiCCI::EventSipRegistrationConfirmed (
	std::string proxyIpAddress)
{
	ConferencingReadyStateSet (estiConferencingStateReady);

	auto pProxyIpAddress = new std::string (proxyIpAddress);
	AppNotify(estiMSG_CB_SIP_REGISTRATION_CONFIRMED, (size_t)pProxyIpAddress);

	// If we have a login start time log the amount of time between login and register
	if (m_stCoreLoginStartTime.tv_sec || m_stCoreLoginStartTime.tv_nsec)
	{
		timespec timevalNow{};
		TimeGet (&timevalNow);

		std::stringstream LoginTimeStream;
		LoginTimeStream << "EventType=LoginTime LoginCount=" << m_unCoreLoginCount;
		LoginTimeStream << " TimeToRegister=" << TimeDifferenceInSeconds (m_stCoreLoginStartTime, timevalNow);
		stiRemoteLogEventSend (LoginTimeStream.str().c_str());
		memset (&m_stCoreLoginStartTime, 0, sizeof (m_stCoreLoginStartTime));
	}
}


/*!
 * \brief Event handler for when the public IP address is resolved
 *        This can come from conference manager, core, or tunnel manager
 */
void CstiCCI::EventPublicIpResolved (std::string ipAddress)
{
	PublicIPResolved(ipAddress);
}


/*!
 * \brief Event handler for when the current time has been set
 *        This can come from conference manager, core, or state notify
 * \param currentTime
 */
void CstiCCI::EventCurrentTimeSet (
	time_t currentTime)
{
	CurrentTimeSet (currentTime);
}



/*!
 * \brief Event handler for when conference manager signals for directory resolve
 *        TODO:  Consider renaming this method since it is only used for call
 *               transfers, and would be incorrect to use as a generic
 *               directory resolve handler.
 * \param call
 */
void CstiCCI::EventDirectoryResolve(
	CstiCallSP call)
{
	std::string dialString;
	call->TransferDialStringGet (&dialString);
	auto fromNameOverride = call->fromNameOverrideGet ();
	auto hResult = DialCommon (estiDM_UNKNOWN, dialString, nullptr, nullptr, fromNameOverride, nullptr, false, false, &call);
	stiTESTRESULT ();

STI_BAIL:
	return;
}


/*!
 * \brief Event handler for when conference manager signals we are conferencing
 *        with the MCU
 * \param call
 */
void CstiCCI::EventConferencingWithMcu (
	CstiCallSP call)
{
	// Do we have a valid call object and is it connected with a GVC MCU?
	if (call && call->ConnectedWithMcuGet (estiMCU_GVC))
	{
		// Start a timer to retrieve the conference room status from the ConferenceService
		m_groupVideoChatRoomStatsTimer.restart ();

		// Get the room status right away
		ConferenceRoomStatusGet(call);

		stiDEBUG_TOOL (g_stiGVC_Debug,
			std::string Guid;
			call->ConferencePublicIdGet (&Guid);
			stiTrace ("<CstiCCI::EventConferenceWithMcu> Received conferencingWithMcuSignal.\n\t"
					"Connected with GVC MCU.  ConferenceID = [%s]\n", Guid.c_str ());
		);
	}

	AppNotify (estiMSG_CONFERENCING_WITH_MCU, (size_t)call.get());
}


/*!
 * \brief Event handler for when conference manager signals the ports
 *        have been set
 * \param success
 * \param params
 */
void CstiCCI::EventConferencePortsSet (const SstiConferencePorts &ports)
{
	m_stConferencePortsStatus.bUseAltSIPListenPort = (ports.nSIPListenPort != nDEFAULT_SIP_LISTEN_PORT) ? estiTRUE : estiFALSE;
	m_stConferencePortsStatus.nAltSIPListenPort = ports.nSIPListenPort;

	CurrentPortsPropertiesSet (&m_stConferencePortsStatus);
	SendCurrentPortsToCore ();

	AppNotify (estiMSG_CONFERENCE_PORTS_CHANGED, 0);
}


/*!
 * \brief Event handler for when a remote system is attempting to connect
 * \param call
 */
void CstiCCI::EventIncomingCall (
	CstiCallSP call)
{
	std::string dialString;
	call->RemoteDialStringGet(&dialString);

	std::string description;
	IstiRingGroupManager::ERingGroupMemberState ringGroupState;

	if (m_pRingGroupManager->ItemGetByNumber (dialString.c_str (), &description, &ringGroupState))
	{
		call->RemoteDialStringSet (description);
	}

	if (estiPORTED_MODE != m_eLocalInterfaceMode &&
		estiINTERPRETER_MODE != m_eLocalInterfaceMode &&
		estiVRI_MODE != m_eLocalInterfaceMode &&
		estiPUBLIC_MODE != m_eLocalInterfaceMode &&
		estiTECH_SUPPORT_MODE != m_eLocalInterfaceMode)
	{
		// Check if remote caller name is in our contact list
		if (call)
		{
			std::string strDialString;
			call->RemoteDialStringGet(&strDialString);

			auto contact = findContact(strDialString);

			if (contact)
			{
				CstiContactListItem::EPhoneNumberType eType = CstiContactListItem::ePHONE_MAX;
				contact->PhoneNumberTypeGet(strDialString, &eType);

				callContactInfoSet (call, contact, contact->PhoneNumberIdGet(eType));

				// set the VcoActive state in call object
				bool bVco;
				contact->VCOGet(&bVco);
				call->LocalIsVcoActiveSet(bVco ? estiTRUE : estiFALSE);
			}
		}
	}

	IncomingRingStart ();
	AppNotify (estiMSG_INCOMING_CALL, (size_t)call.get());
	m_pVRCLServer->VRCLNotify (estiMSG_INCOMING_CALL, call);
}


stiHResult CstiCCI::PublicIPSettingsChanged ()
{
	PublicIpChanged ();

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::CheckForAuthorizedNumberChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	int nCheckForAuthPhoneNumber = 1;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	PropertyManager::getInstance ()->propertyGet (cmCHECK_FOR_AUTH_NUMBER, &nCheckForAuthPhoneNumber, PropertyManager::Persistent);
	stConferenceParams.bCheckForAuthorizedNumber = (nCheckForAuthPhoneNumber == 1);

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::HeartbeatDelayChanged ()
{
	int nDelay{nDEFAULT_HEARTBEAT_DELAY};

	PropertyManager::getInstance ()->propertyGet (cmHEARTBEAT_DELAY, &nDelay);

	m_pStateNotifyService->HeartbeatDelaySet (nDelay);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::CoreServiceUrlChanged ()
{
	ServiceContactsSet (m_pCoreService, cmCORE_SERVICE_URL1, cmCORE_SERVICE_ALT_URL1);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::StateNotifyServiceUrlChanged ()
{
	ServiceContactsSet (m_pStateNotifyService, cmSTATE_NOTIFY_SERVICE_URL1, cmSTATE_NOTIFY_SERVICE_ALT_URL1);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::MessageServiceUrlChanged ()
{
	ServiceContactsSet (m_pMessageService, cmMESSAGE_SERVICE_URL1, cmMESSAGE_SERVICE_ALT_URL1);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::ConferenceServiceUrlChanged ()
{
	ServiceContactsSet (m_pConferenceService, CONFERENCE_SERVICE_URL1, CONFERENCE_SERVICE_ALT_URL1);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::RemoteLoggerServiceUrlChanged ()
{
	ServiceContactsSet (m_pRemoteLoggerService, REMOTELOGGER_SERVICE_URL1, REMOTELOGGER_SERVICE_ALT_URL1);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::CoreServiceSSLFailOverChanged ()
{
	int nSSLFlag = stiSSL_DEFAULT_MODE;

	m_poPM->propertyGet(CoreServiceSSLFailOver, &nSSLFlag);
	m_pCoreService->SSLFlagSet((ESSLFlag)nSSLFlag);
	m_poPM->PropertySend(CoreServiceSSLFailOver, estiPTypePhone);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::StateNotifySSLFailOverChanged ()
{
	int nSSLFlag = stiSSL_DEFAULT_MODE;

	m_poPM->propertyGet(StateNotifySSLFailOver, &nSSLFlag);
	m_pStateNotifyService->SSLFlagSet((ESSLFlag)nSSLFlag);
	m_poPM->PropertySend(StateNotifySSLFailOver, estiPTypePhone);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::MessageServiceSSLFailOverChanged ()
{
	int nSSLFlag = stiSSL_DEFAULT_MODE;

	m_poPM->propertyGet(MessageServiceSSLFailOver, &nSSLFlag);
	m_pMessageService->SSLFlagSet((ESSLFlag)nSSLFlag);
	m_poPM->PropertySend(MessageServiceSSLFailOver, estiPTypePhone);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::ConferenceServiceSSLFailOverChanged ()
{
	int nSSLFlag = stiSSL_DEFAULT_MODE;

	m_poPM->propertyGet(ConferenceServiceSSLFailOver, &nSSLFlag);
	m_pConferenceService->SSLFlagSet((ESSLFlag)nSSLFlag);
	m_poPM->PropertySend(ConferenceServiceSSLFailOver, estiPTypePhone);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::RemoteLoggerServiceSSLFailOverChanged ()
{
	int nSSLFlag = stiSSL_DEFAULT_MODE;

	m_poPM->propertyGet(RemoteLoggerServiceSSLFailOver, &nSSLFlag);
	m_pRemoteLoggerService->SSLFlagSet((ESSLFlag)nSSLFlag);
	m_poPM->PropertySend(RemoteLoggerServiceSSLFailOver, estiPTypePhone);

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::ConferencePortsSettingsChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	LoadPortSettingsProperties (&m_stConferencePortsSettings);

	ConferencePortsSettingsApply();

	return hResult;
}


stiHResult CstiCCI::NATTraversalSIPChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	if (m_eLocalInterfaceMode != estiPORTED_MODE)
	{
		PropertyManager *pPM = PropertyManager::getInstance();
		pPM->propertyGet (NAT_TRAVERSAL_SIP,
						 (int*)&stConferenceParams.eNatTraversalSIP);
	}
	else
	{
		stConferenceParams.eNatTraversalSIP = estiSIPNATDisabled;
	}

	// If NAT traversal became disabled, we will need to fetch our public ip from core
	if (stConferenceParams.eNatTraversalSIP == estiSIPNATDisabled)
	{
		//
		// Clear state notify's knowledge of the previosu Public IP
		// and tell it to do a heartbeat to start the process of
		// detecting the public IP.
		//
		m_pStateNotifyService->PublicIPClear ();
		StateNotifyHeartbeatRequest ();

		ConferencingReadyStateSet (estiConferencingStateReady);
	}
	else
	{
		ConferencingReadyStateSet (estiConferencingStateNotReady);
	}

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();
	
	AppNotify (estiMSG_NAT_TRAVERSAL_SIP_CHANGED, stConferenceParams.eNatTraversalSIP);

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::SIPNatTransportChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	m_poPM->propertyGet (SIP_NAT_TRANSPORT,
					(int*)&stConferenceParams.nAllowedProxyTransports);

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::BlockCallerIDEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	int nBlockCallerIDEnabled = 0;
	int nBlockCallerID = 0;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	if (estiSTANDARD_MODE == m_eLocalInterfaceMode
		|| estiHEARING_MODE == m_eLocalInterfaceMode
		|| estiPUBLIC_MODE == m_eLocalInterfaceMode)
	{
		m_poPM->propertyGet (BLOCK_CALLER_ID_ENABLED, &nBlockCallerIDEnabled);

		if (nBlockCallerIDEnabled)
		{
			m_poPM->propertyGet (BLOCK_CALLER_ID, &nBlockCallerID);
		}
	}

	if (stConferenceParams.bBlockCallerID != (bool)nBlockCallerID) {
		stConferenceParams.bBlockCallerID = nBlockCallerID;
		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();
	}

	// AppNotify is called every time for the case that BlockCallerIDEnabled
	// was changed and BlockCallerID was not
	AppNotify (estiMSG_BLOCK_CALLER_ID_CHANGED, nBlockCallerID);

STI_BAIL:
	
	return hResult;
}


stiHResult CstiCCI::BlockCallerIDChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	SstiConferenceParams stConferenceParams;
	int nBlockCallerID = 0;
	
	if (estiSTANDARD_MODE == m_eLocalInterfaceMode
		|| estiHEARING_MODE == m_eLocalInterfaceMode 
		|| estiPUBLIC_MODE == m_eLocalInterfaceMode)
	{
		m_poPM->propertyGet (BLOCK_CALLER_ID_ENABLED, &nBlockCallerID);
		if (nBlockCallerID)
		{
			m_poPM->propertyGet (BLOCK_CALLER_ID,
								&nBlockCallerID);
		}
	}
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();
			
	if (stConferenceParams.bBlockCallerID != (bool)nBlockCallerID)
	{
		stConferenceParams.bBlockCallerID = nBlockCallerID;
		
		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();
	
		AppNotify (estiMSG_BLOCK_CALLER_ID_CHANGED, nBlockCallerID);
	}
	
STI_BAIL:
	
	return hResult;
}


stiHResult CstiCCI::BlockAnonymousCallersChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	SstiConferenceParams stConferenceParams;
	int nBlockAnonymousCallers = 0;

	if (estiSTANDARD_MODE == m_eLocalInterfaceMode 
		|| estiHEARING_MODE == m_eLocalInterfaceMode)
	{
		m_poPM->propertyGet (BLOCK_ANONYMOUS_CALLERS,
							&nBlockAnonymousCallers);
	}
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();
	
	if (stConferenceParams.bBlockAnonymousCallers != (bool)nBlockAnonymousCallers)
	{
		stConferenceParams.bBlockAnonymousCallers = nBlockAnonymousCallers;

		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();

		AppNotify (estiMSG_BLOCK_ANONYMOUS_CALLERS_CHANGED, nBlockAnonymousCallers);
	}

STI_BAIL:
	
	return hResult;
}

stiHResult CstiCCI::LDAPChanged ()
{
#ifdef stiLDAP_CONTACT_LIST
	std::string directoryDisplayName;
	std::string ldapServer;
	std::string ldapDomainBase;
	std::string ldapTelephoneNumberField;
	std::string ldapHomeNumberField;
	std::string ldapMobileNumberField;
	int nLDAPServerPort = STANDARD_LDAP_PORT;
	int nLDAPServerUseSSL = 0;
	int nLDAPScope = 0;
	auto eLDAPServerRequiresAuthentication = IstiLDAPDirectoryContactManager::eLDAPAuthenticateMethod_NONE;
	bool ldapEnabled = stiLDAP_ENABLED_DEFAULT;
	
	m_poPM->propertyGet (LDAP_ENABLED, &ldapEnabled);
	
	m_pLDAPDirectoryContactManager->EnabledSet (ldapEnabled);
	
	m_poPM->propertyGet (LDAP_DIRECTORY_DISPLAY_NAME, &directoryDisplayName);
	m_poPM->propertyGet (LDAP_SERVER, &ldapServer);
	m_poPM->propertyGet (LDAP_DOMAIN_BASE, &ldapDomainBase);
	m_poPM->propertyGet (LDAP_TELEPHONE_NUMBER_FIELD, &ldapTelephoneNumberField);
	m_poPM->propertyGet (LDAP_HOME_NUMBER_FIELD, &ldapHomeNumberField);
	m_poPM->propertyGet (LDAP_MOBILE_NUMBER_FIELD, &ldapMobileNumberField);
	m_poPM->propertyGet (LDAP_SERVER_PORT, &nLDAPServerPort);
	m_poPM->propertyGet (LDAP_SERVER_USE_SSL, &nLDAPServerUseSSL);
	m_poPM->propertyGet (LDAP_SCOPE, &nLDAPScope);
	m_poPM->propertyGet (LDAP_SERVER_REQUIRES_AUTHENTICATION, &eLDAPServerRequiresAuthentication);
	
	m_pLDAPDirectoryContactManager->LDAPDirectoryNameSet(directoryDisplayName);
	m_pLDAPDirectoryContactManager->LDAPHostSet(ldapServer);
	m_pLDAPDirectoryContactManager->LDAPBaseEntrySet(ldapDomainBase);
	m_pLDAPDirectoryContactManager->LDAPTelephoneNumberFieldSet(ldapTelephoneNumberField);
	m_pLDAPDirectoryContactManager->LDAPHomeNumberFieldSet(ldapHomeNumberField);
	m_pLDAPDirectoryContactManager->LDAPMobileNumberFieldSet(ldapMobileNumberField);
	m_pLDAPDirectoryContactManager->LDAPServerPortSet(nLDAPServerPort);
	m_pLDAPDirectoryContactManager->LDAPServerUseSSLSet(nLDAPServerUseSSL);
	m_pLDAPDirectoryContactManager->LDAPScopeSet(nLDAPScope);
	m_pLDAPDirectoryContactManager->LDAPServerRequiresAuthenticationSet(eLDAPServerRequiresAuthentication);
#endif

	return stiRESULT_SUCCESS;
}


stiHResult CstiCCI::IPv6EnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;

	int nIPv6Enabled = m_bIPv6Enabled;
	m_poPM->propertyGet (IPv6Enabled,
						&nIPv6Enabled);
	m_bIPv6Enabled = nIPv6Enabled;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	stConferenceParams.bIPv6Enabled = m_bIPv6Enabled;
	
	// If we are disabling IPv6 clear our public IPv6 address otherwise try to add it.
	if (!m_bIPv6Enabled)
	{
		stConferenceParams.PublicIPv6.clear ();

		// Restore NAT setting from property table in case we had tunneling enabled
		m_poPM->propertyGet (NAT_TRAVERSAL_SIP,
								(int*)&stConferenceParams.eNatTraversalSIP);
	}
	else
	{
		std::string PublicIP;
		stiHResult hResult2 = PublicIPAddressGet(&PublicIP, estiTYPE_IPV6);

		if (!stiIS_ERROR (hResult2))
		{
			stConferenceParams.PublicIPv6.assign(PublicIP);
		}

		// Disable tunneling if IPv6 is enabled, we don't support IPv6 tunneling.
		if (stConferenceParams.eNatTraversalSIP == estiSIPNATEnabledUseTunneling)
		{
			stConferenceParams.eNatTraversalSIP = estiSIPNATEnabledMediaRelayAllowed;
		}
	}

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

	hResult = CstiHostNameResolver::getInstance ()->IPv6Enable(m_bIPv6Enabled);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiCCI::SecureCallModeChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	m_poPM->propertyGet (SECURE_CALL_MODE, (int*)&stConferenceParams.eSecureCallMode);

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

	secureCallModeChangedSignal.Emit (stConferenceParams.eSecureCallMode);

STI_BAIL:

	return hResult;
}


EstiSecureCallMode CstiCCI::SecureCallModeGet ()
{
	EstiSecureCallMode eSecureCallMode{stiSECURE_CALL_MODE_DEFAULT};

	m_poPM->propertyGet (SECURE_CALL_MODE,
					(int*)&eSecureCallMode);

	return eSecureCallMode;
}


void CstiCCI::SecureCallModeSet (EstiSecureCallMode eSecureCallMode)
{
	m_poPM->propertySet (SECURE_CALL_MODE, eSecureCallMode, PropertyManager::Persistent);
	PropertyManager::getInstance()->PropertySend(SECURE_CALL_MODE, estiPTypeUser);
	SecureCallModeChanged ();
}


DeviceLocationType CstiCCI::deviceLocationTypeGet ()
{
	auto deviceLocationType = DEVICE_LOCATION_TYPE_DEFAULT;
	m_poPM->propertyGet (DEVICE_LOCATION_TYPE, &deviceLocationType);
	return deviceLocationType;
}


void CstiCCI::deviceLocationTypeSet (DeviceLocationType deviceLocationType)
{
	m_poPM->propertySet (DEVICE_LOCATION_TYPE, deviceLocationType, PropertyManager::Persistent);
	m_poPM->PropertySend (DEVICE_LOCATION_TYPE, estiPTypePhone);
	deviceLocationTypeChanged ();
}


stiHResult CstiCCI::MaxSpeedsChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	MaxSpeedsUpdate ();

	AppNotify(estiMSG_MAX_SPEEDS_CHANGED, 0);

	return hResult;
}


stiHResult CstiCCI::AutoSpeedSettingsChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nAutoSpeedEnabled = stiAUTO_SPEED_ENABLED_DEFAULT;
	EstiAutoSpeedMode eAutoSpeedMode;
	EstiAutoSpeedMode eAutoSpeedSetting;

	m_poPM->propertyGet (AUTO_SPEED_ENABLED, &nAutoSpeedEnabled);

	eAutoSpeedMode = stiAUTO_SPEED_MODE_DEFAULT;

	m_poPM->propertyGet (AUTO_SPEED_MODE, &eAutoSpeedMode);

	//Update AutoSpeedSetting conference parameter
	AutoSpeedSettingsUpdate (nAutoSpeedEnabled, eAutoSpeedMode, &eAutoSpeedSetting);

	//Update max send and receive conference parameters
	MaxSpeedsUpdate();

	AppNotify(estiMSG_AUTO_SPEED_SETTINGS_CHANGED, (size_t)eAutoSpeedSetting);

	return hResult;
}

stiHResult CstiCCI::AutoSpeedMinStartSpeedChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32AutoSpeedMinStartSpeed = stiAUTO_SPEED_MIN_START_SPEED_DEFAULT;
	
	m_poPM->propertyGet (AUTO_SPEED_MIN_START_SPEED, &un32AutoSpeedMinStartSpeed);

	AutoSpeedMinStartSpeedUpdate (un32AutoSpeedMinStartSpeed);
	
	return hResult;
}

stiHResult CstiCCI::SignMailChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nSignMailEnabled = stiSIGNMAIL_ENABLED_DEFAULT;

	// Update the SignMail setting.
	m_poPM->propertyGet (cmSIGNMAIL_ENABLED, &nSignMailEnabled);
	m_bSignMailEnabled = nSignMailEnabled == 1;

	return hResult;
}

stiHResult CstiCCI::DTMFEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;
	int nDTMFEnabled = stiDTMF_ENABLED_DEFAULT;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	PropertyManager::getInstance()->propertyGet (DTMF_ENABLED, &nDTMFEnabled);
	stConferenceParams.bDTMFEnabled = nDTMFEnabled == 1;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiCCI::RealTimeTextEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	int nRealTimeTextEnabled = stiREAL_TIME_TEXT_ENABLED_DEFAULT;
	bool bRealTimeTextEnabled = false;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	PropertyManager::getInstance()->propertyGet (REAL_TIME_TEXT_ENABLED, &nRealTimeTextEnabled);
	bRealTimeTextEnabled = nRealTimeTextEnabled == 1;
	stConferenceParams.stMediaDirection.bInboundText = bRealTimeTextEnabled;
	stConferenceParams.stMediaDirection.bOutboundText = bRealTimeTextEnabled;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiCCI::BlockListEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool blockListEnabled = stiBLOCK_LIST_ENABLED_DEFAULT;

	if (m_eLocalInterfaceMode == estiPORTED_MODE
	 || m_eLocalInterfaceMode == estiPUBLIC_MODE
	 || m_eLocalInterfaceMode == estiINTERPRETER_MODE
	 || m_eLocalInterfaceMode == estiVRI_MODE
	 || m_eLocalInterfaceMode == estiTECH_SUPPORT_MODE)
	{
		blockListEnabled = false;
	}
	else
	{
		m_poPM->propertyGet (BLOCK_LIST_ENABLED, &blockListEnabled);
	}

	m_pBlockListManager->EnabledSet (blockListEnabled);

	return hResult;
}

stiHResult CstiCCI::RingsBeforeGreetingChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRingsBeforeGreeting = stiRINGS_BEFORE_GREETING_DEFAULT;

	m_poPM->propertyGet (cmRINGS_BEFORE_GREETING, &nRingsBeforeGreeting);

	stiLOG_ENTRY_NAME (CstiCCI::RingsBeforeGreetingChanged);

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	stConferenceParams.nRingsBeforeGreeting = nRingsBeforeGreeting;

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();
	
	AppNotify (estiMSG_RINGS_BEFORE_GREETING_CHANGED, 0);

STI_BAIL:

	return (hResult);
}

stiHResult CstiCCI::RingGroupSettingsChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool ringGroupEnabled = stiRING_GROUP_ENABLED_DEFAULT;

	m_poPM->propertyGet (RING_GROUP_ENABLED, &ringGroupEnabled);

	auto  nMode = (int)IstiRingGroupManager::eRING_GROUP_DISABLED;

	if (ringGroupEnabled)
	{
		m_poPM->propertyGet (RING_GROUP_DISPLAY_MODE, &nMode);
	}

	m_pRingGroupManager->ModeSet ((IstiRingGroupManager::ERingGroupDisplayMode)nMode);

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RemoteLoggerTraceLoggingChanged
//
// Description: Call back that handles the remote logging trace property changed event .
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
stiHResult CstiCCI::RemoteLoggerTraceLoggingChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRemoteTraceLogging = stiREMOTE_LOG_TRACE_DEFAULT;

	m_poPM->propertyGet (REMOTE_TRACE_LOGGING, &nRemoteTraceLogging);

	stiLOG_ENTRY_NAME (CstiCCI::RemoteLoggerTraceLoggingChanged);

	hResult = m_pRemoteLoggerService->RemoteLoggerTraceEnable(nRemoteTraceLogging);

	return hResult;

}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RemoteLoggerAssertLoggingChanged
//
// Description: Call back that handles the remote logging assert property changed event .
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
stiHResult CstiCCI::RemoteLoggerAssertLoggingChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRemoteAssertLogging = stiREMOTE_LOG_ASSERT_DEFAULT;

	m_poPM->propertyGet (REMOTE_ASSERT_LOGGING, &nRemoteAssertLogging);

	stiLOG_ENTRY_NAME (CstiCCI::RemoteLoggerAssertLoggingChanged);

	hResult = m_pRemoteLoggerService->RemoteLoggerAssertEnable(nRemoteAssertLogging);

	return hResult;

}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RemoteLoggerCallStatsLoggingChanged
//
// Description: Call back that handles the remote logging CallStats property changed event .
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
stiHResult CstiCCI::RemoteLoggerCallStatsLoggingChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int nVal;
	nVal = stiREMOTE_LOG_CALL_STATS_DEFAULT;
	m_poPM->propertyGet (REMOTE_CALL_STATS_LOGGING, &nVal);
	m_bRemoteLoggingCallStatsEnabled = (nVal == 1);

	stiLOG_ENTRY_NAME (CstiCCI::RemoteLoggerCallStatsLoggingChanged);

	return hResult;

}


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RemoteLoggerEventLoggingChanged
//
// Description: Call back that handles the remote logging event property changed event .
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
stiHResult CstiCCI::RemoteLoggerEventLoggingChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRemoteEventLogging = stiREMOTE_LOG_EVENT_SEND_DEFAULT;

	m_poPM->propertyGet (REMOTE_LOG_EVENT_SEND_LOGGING, &nRemoteEventLogging);

	stiLOG_ENTRY_NAME (CstiCCI::RemoteLoggerEventLoggingChanged);

	hResult = m_pRemoteLoggerService->RemoteLoggerEventEnable(nRemoteEventLogging);

	return hResult;

}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RemoteFlowControlLoggingChanged
//
// Description: Call back that handles the flow control logging property changed event.
//
// Abstract:
//
// Returns:
//	estiOK (Success), estiERROR (Failure)
//
stiHResult CstiCCI::RemoteFlowControlLoggingChanged ()
{
	stiLOG_ENTRY_NAME (CstiCCI::RemoteFlowControlLoggingChanged);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	int nRemoteFlowControlLogging = 0;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();
	
	m_poPM->propertyGet (REMOTE_FLOW_CONTROL_LOGGING, &nRemoteFlowControlLogging);
	stConferenceParams.bRemoteFlowControlLogging = nRemoteFlowControlLogging;
	
	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
	
}


stiHResult CstiCCI::ContactImagesEnableChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifdef stiIMAGE_MANAGER
	int bEnabled = stiCONTACT_PHOTOS_ENABLED_DEFAULT;
	m_poPM->propertyGet (CONTACT_IMAGES_ENABLED, &bEnabled);

	if (!bEnabled)
	{
		 if (m_pContactManager)
		 {
			 m_pContactManager->PurgeAllContactPhotos();
		 }

		 if (m_pImageManager)
		 {
			 m_pImageManager->PurgeAllContactPhotos();
		 }
	}

	if (m_pImageManager)
	{
		m_pImageManager->ContactImagesEnabledSet((EstiBool)bEnabled);
	}

	AppNotify (estiMSG_CONTACT_IMAGES_ENABLED_CHANGED, 0);

#endif

	return hResult;
}

stiHResult CstiCCI::NewCallEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int nVal;
	nVal = stiNEW_CALL_ENABLED_DEFAULT;
	m_poPM->propertyGet (NEW_CALL_ENABLED, &nVal);
	m_bNewCallEnabled = (nVal == 1);
	
	return hResult;
}

stiHResult CstiCCI::signMailUpdateTimeChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifdef stiMESSAGE_MANAGER
	int64_t lastUpdateTime = 0;
	PropertyManager::getInstance()->propertyGet(LAST_SIGNMAIL_UPDATE_TIME, 
												   &lastUpdateTime, PropertyManager::Persistent);
	m_pMessageManager->LastSignMailUpdateSet(lastUpdateTime);
#endif
	return hResult;
}

stiHResult CstiCCI::videoUpdateTimeChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifdef stiMESSAGE_MANAGER
	int64_t lastUpdateTime = 0;
	PropertyManager::getInstance()->propertyGet(LAST_VIDEO_UPDATE_TIME,
												   &lastUpdateTime, PropertyManager::Persistent);
	m_pMessageManager->LastCategoryUpdateSet(lastUpdateTime);
#endif
	return hResult;
}


stiHResult CstiCCI::vrsFocusedRoutingChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string vrsFocusedRouting;
	
	m_poPM->propertyGet (VRS_FOCUSED_ROUTING, &vrsFocusedRouting);
	m_pConferenceManager->vrsFocusedRoutingSet(vrsFocusedRouting);
	
	return hResult;
}


/*!\brief Updates the DHV API URL via conference manager
*
*  \return Success or Failure.
*/
stiHResult CstiCCI::dhvApiUrlChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string dhvApiUrl = stiDHV_API_URL_DEFAULT;
	m_poPM->propertyGet (DHV_API_URL, &dhvApiUrl);
	m_dhvClient.urlSet (dhvApiUrl);
	return hResult;
}


/*!\brief Updates the DHV API key via conference manager
*
*  \return Success or Failure.
*/
stiHResult CstiCCI::dhvApiKeyChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string dhvApiKey = stiDHV_API_KEY_DEFAULT;
	m_poPM->propertyGet (DHV_API_KEY, &dhvApiKey);
	m_dhvClient.apiKeySet(dhvApiKey);

	return hResult;
}


/*!\brief Updates the DHV enabled setting via conference manager
*
*  \return Success or Failure.
*/
stiHResult CstiCCI::dhvEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int dhvEnabled = stiDEAF_HEARING_VIDEO_ENABLED_DEFAULT;
	m_poPM->propertyGet (DEAF_HEARING_VIDEO_ENABLED, &dhvEnabled);
	m_dhvClient.dhvEnabledSet (dhvEnabled);
	
	return hResult;
}


stiHResult CstiCCI::endpointMonitorServerChanged()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string endpointMonitorServer;

	PropertyManager::getInstance()->propertyGet (ENDPOINT_MONITOR_SERVER, &endpointMonitorServer);

	vpe::EndpointMonitor::instanceGet()->urlSet(endpointMonitorServer);

	return hResult;
}


stiHResult CstiCCI::endpointMonitorEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool endpointMonitorEnabled {false};

	PropertyManager::getInstance()->propertyGet (ENDPOINT_MONITOR_ENABLED, &endpointMonitorEnabled);

	vpe::EndpointMonitor::instanceGet ()->enabled (endpointMonitorEnabled);

	return hResult;
}

stiHResult CstiCCI::deviceLocationTypeChanged ()
{
	auto hResult = stiRESULT_SUCCESS;
	auto deviceLocationType = deviceLocationTypeGet ();
	m_disallowOutgoingCalls = deviceLocationType == DeviceLocationType::Update;
	deviceLocationTypeChangedSignalGet ().Emit (deviceLocationType);
	return hResult;
}


void CstiCCI::propertyChangedMethodsInitialize ()
{
	m_propertyChangedMethods =
	{
		{cmPUBLIC_IP_ASSIGN_METHOD, &CstiCCI::PublicIPSettingsChanged},
		{cmPUBLIC_IP_ADDRESS_MANUAL, &CstiCCI::PublicIPSettingsChanged},
		{cmHEARTBEAT_DELAY, &CstiCCI::HeartbeatDelayChanged},
		{cmCORE_SERVICE_URL1, &CstiCCI::CoreServiceUrlChanged},
		{cmCORE_SERVICE_ALT_URL1, &CstiCCI::CoreServiceUrlChanged},
		{cmSTATE_NOTIFY_SERVICE_URL1, &CstiCCI::StateNotifyServiceUrlChanged},
		{cmSTATE_NOTIFY_SERVICE_ALT_URL1, &CstiCCI::StateNotifyServiceUrlChanged},
		{cmMESSAGE_SERVICE_URL1, &CstiCCI::MessageServiceUrlChanged},
		{cmMESSAGE_SERVICE_ALT_URL1, &CstiCCI::MessageServiceUrlChanged},
		{REMOTELOGGER_SERVICE_URL1, &CstiCCI::RemoteLoggerServiceUrlChanged},
		{REMOTELOGGER_SERVICE_ALT_URL1, &CstiCCI::RemoteLoggerServiceUrlChanged},
		{CONFERENCE_SERVICE_URL1, &CstiCCI::ConferenceServiceUrlChanged},
		{CONFERENCE_SERVICE_ALT_URL1, &CstiCCI::ConferenceServiceUrlChanged},
		{CoreServiceSSLFailOver, &CstiCCI::CoreServiceSSLFailOverChanged},
		{StateNotifySSLFailOver, &CstiCCI::StateNotifySSLFailOverChanged},
		{MessageServiceSSLFailOver, &CstiCCI::MessageServiceSSLFailOverChanged},
		{RemoteLoggerServiceSSLFailOver, &CstiCCI::RemoteLoggerServiceSSLFailOverChanged},
		{REMOTE_TRACE_LOGGING, &CstiCCI::RemoteLoggerTraceLoggingChanged},
		{REMOTE_ASSERT_LOGGING, &CstiCCI::RemoteLoggerAssertLoggingChanged},
		{REMOTE_CALL_STATS_LOGGING, &CstiCCI::RemoteLoggerCallStatsLoggingChanged},
		{REMOTE_LOG_EVENT_SEND_LOGGING, &CstiCCI::RemoteLoggerEventLoggingChanged},
		{REMOTE_FLOW_CONTROL_LOGGING, &CstiCCI::RemoteFlowControlLoggingChanged},
		{AltSipListenPortEnabled, &CstiCCI::ConferencePortsSettingsChanged},
		{AltSipListenPort, &CstiCCI::ConferencePortsSettingsChanged},
		{NAT_TRAVERSAL_SIP, &CstiCCI::NATTraversalSIPChanged},
		{SIP_NAT_TRANSPORT, &CstiCCI::SIPNatTransportChanged},
		{IPv6Enabled, &CstiCCI::IPv6EnabledChanged},
		{SECURE_CALL_MODE, &CstiCCI::SecureCallModeChanged},
		{cmCHECK_FOR_AUTH_NUMBER, &CstiCCI::CheckForAuthorizedNumberChanged},
		{cmMAX_SEND_SPEED, &CstiCCI::MaxSpeedsChanged},
		{cmMAX_RECV_SPEED, &CstiCCI::MaxSpeedsChanged},
		{AUTO_SPEED_ENABLED, &CstiCCI::AutoSpeedSettingsChanged},
		{AUTO_SPEED_MODE, &CstiCCI::AutoSpeedSettingsChanged},
		{AUTO_SPEED_MIN_START_SPEED, &CstiCCI::AutoSpeedMinStartSpeedChanged},
		{cmSIGNMAIL_ENABLED, &CstiCCI::SignMailChanged},
		{DTMF_ENABLED, &CstiCCI::DTMFEnabledChanged},
		{REAL_TIME_TEXT_ENABLED, &CstiCCI::RealTimeTextEnabledChanged},
		{BLOCK_LIST_ENABLED, &CstiCCI::BlockListEnabledChanged},
		{cmRINGS_BEFORE_GREETING, &CstiCCI::RingsBeforeGreetingChanged},
		{RING_GROUP_ENABLED, &CstiCCI::RingGroupSettingsChanged},
		{RING_GROUP_DISPLAY_MODE, &CstiCCI::RingGroupSettingsChanged},
		{CONTACT_IMAGES_ENABLED, &CstiCCI::ContactImagesEnableChanged},
		{BLOCK_CALLER_ID, &CstiCCI::BlockCallerIDChanged},
		{BLOCK_CALLER_ID_ENABLED, &CstiCCI::BlockCallerIDEnabledChanged},
		{BLOCK_ANONYMOUS_CALLERS, &CstiCCI::BlockAnonymousCallersChanged},
		{LDAP_ENABLED, &CstiCCI::LDAPChanged},
		{LDAP_SERVER, &CstiCCI::LDAPChanged},
		{LDAP_SERVER_PORT, &CstiCCI::LDAPChanged},
		{LDAP_SERVER_USE_SSL, &CstiCCI::LDAPChanged},
		{LDAP_SERVER_REQUIRES_AUTHENTICATION, &CstiCCI::LDAPChanged},
		{LDAP_DOMAIN_BASE, &CstiCCI::LDAPChanged},
		{LDAP_TELEPHONE_NUMBER_FIELD, &CstiCCI::LDAPChanged},
		{LDAP_HOME_NUMBER_FIELD, &CstiCCI::LDAPChanged},
		{LDAP_MOBILE_NUMBER_FIELD, &CstiCCI::LDAPChanged},
		{LDAP_DIRECTORY_DISPLAY_NAME, &CstiCCI::LDAPChanged},
		{LDAP_SCOPE, &CstiCCI::LDAPChanged},
		{NEW_CALL_ENABLED, &CstiCCI::NewCallEnabledChanged},
		{UNREGISTERED_SIP_RESTART_TIME, &CstiCCI::SipRestartTimeChanged},
		{VRS_FAILOVER_TIMEOUT, &CstiCCI::VRSFailOverTimeoutChanged},
		{VRS_FAILOVER_SERVER, &CstiCCI::VRSFailoverDomainResolve},
		{FIR_FEEDBACK_ENABLED, &CstiCCI::FirFeedbackEnabledChanged},
		{PLI_FEEDBACK_ENABLED, &CstiCCI::PliFeedbackEnabledChanged},
		{AFB_FEEDBACK_ENABLED, &CstiCCI::AfbFeedbackEnabledChanged},
		{TMMBR_FEEDBACK_ENABLED, &CstiCCI::TmmbrFeedbackEnabledChanged},
		{NACK_RTX_FEEDBACK_ENABLED, &CstiCCI::NackRtxFeedbackEnabledChanged},
		{SERVICE_OUTAGE_SERVICE_URL1, &CstiCCI::ServiceOutageServiceUrlChanged},
		{SERVICE_OUTAGE_TIMER_INTERVAL, &CstiCCI::ServiceOutageTimerIntervalChanged},
		{LAST_SIGNMAIL_UPDATE_TIME, &CstiCCI::signMailUpdateTimeChanged},
		{LAST_VIDEO_UPDATE_TIME, &CstiCCI::videoUpdateTimeChanged},
		{VRS_FOCUSED_ROUTING, &CstiCCI::vrsFocusedRoutingChanged},
		{DHV_API_URL, &CstiCCI::dhvApiUrlChanged},
		{DHV_API_KEY, &CstiCCI::dhvApiKeyChanged},
		{DEAF_HEARING_VIDEO_ENABLED, &CstiCCI::dhvEnabledChanged},
		{ENDPOINT_MONITOR_SERVER, &CstiCCI::endpointMonitorServerChanged},
		{ENDPOINT_MONITOR_ENABLED, &CstiCCI::endpointMonitorEnabledChanged},
		{DEVICE_LOCATION_TYPE, &CstiCCI::deviceLocationTypeChanged},
		{cmLOCAL_INTERFACE_MODE, &CstiCCI::interfaceModeChanged}
	};
}

stiHResult CstiCCI::interfaceModeChanged ()
{
	EstiInterfaceMode mode = estiSTANDARD_MODE;
	m_poPM->propertyGet (cmLOCAL_INTERFACE_MODE, &mode);
	return LocalInterfaceModeSet (mode);
}

stiHResult CstiCCI::logInUsingCache (const std::string& phoneNumber, const std::string& pin)
{
	auto hResult = stiRESULT_SUCCESS;
	std::string coreAuthToken;

	stiTESTCOND (!phoneNumber.empty (), stiRESULT_ERROR);
	stiTESTCOND (!pin.empty (), stiRESULT_ERROR);

	m_poPM->propertyGet (CORE_AUTH_TOKEN, &coreAuthToken, PropertyManager::Persistent);
	stiTESTCOND (!coreAuthToken.empty (), stiRESULT_ERROR);

	hResult = m_pCoreService->logInUsingCache (phoneNumber, pin, coreAuthToken);
	stiTESTRESULT ();

	RegistrationConferenceParamsSet ();

	// Always request a heartbeat.  Need to get changes, or make sure we can actually be logged in
	StateNotifyHeartbeatRequest();

STI_BAIL:
	return hResult;
}

ISignal<const ServiceResponseSP<CstiUserAccountInfo>&>& CstiCCI::userAccountInfoReceivedSignalGet ()
{
	return userAccountInfoReceivedSignal;
}

ISignal<const ServiceResponseSP<vpe::Address>&>& CstiCCI::emergencyAddressReceivedSignalGet ()
{
	return emergencyAddressReceivedSignal;
}

ISignal<const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>&>& CstiCCI::emergencyAddressStatusReceivedSignalGet ()
{
	return emergencyAddressStatusReceivedSignal;
}

ISignal<EstiSecureCallMode>& CstiCCI::secureCallModeChangedSignalGet ()
{
	return secureCallModeChangedSignal;
}

ISignal<DeviceLocationType>& CstiCCI::deviceLocationTypeChangedSignalGet ()
{
	return deviceLocationTypeChangedSignal;
}


void CstiCCI::CheckProperty (
	const char *pszName,
	SstiSettingItem::EstiSettingType eType,
	const char *pszValue,
	std::set<PstiPropertyUpdateMethod, classcomp> *pMethods)
{
	//
	// Find the method associated to this property (if any) and add
	// it to the set if it has changed value.
	//
	for (auto &method : m_propertyChangedMethods)
	{
		if (strcmp (pszName, method.pszPropertyName) == 0)
		{
			std::string value;

			int nResult = m_poPM->propertyGet (pszName, &value,
												PropertyManager::Temporary);

			if (nResult == PM_RESULT_NOT_FOUND
				|| value != pszValue)
			{
				pMethods->insert (method.pfnMethod);
			}
		}
	}

	//
	// Save the setting
	//
	switch (eType)
	{
		case SstiSettingItem::estiEnum:
		case SstiSettingItem::estiInt:
		{
			int64_t nValue = 0;
			if (boost::conversion::try_lexical_convert(pszValue, nValue))
			{
				m_poPM->propertySet(
					pszName,
					nValue,
					PropertyManager::Persistent);
			}
			else
			{
				m_poPM->removeProperty(pszName, true);
			}
			break;
		}

		case SstiSettingItem::estiBool:
		{
			int nValue = 0;
			if (0 != stiStrICmp(pszValue, "false")
			 && 0 != stiStrICmp(pszValue, "0"))
			{
				nValue = 1;
			} // end if

			m_poPM->propertySet(
				pszName,
				nValue,
				PropertyManager::Persistent);

			break;
		}

		case SstiSettingItem::estiString:
		{
			m_poPM->propertySet(
				pszName,
				pszValue,
				PropertyManager::Persistent);
		}
	}
}


void CstiCCI::CallPropertyUpdateMethods (
	const std::set<PstiPropertyUpdateMethod, classcomp> *pMethods)
{
	//
	// Iterate through the set, calling each method
	//
	std::set<PstiPropertyUpdateMethod, classcomp>::const_iterator k;

	for (k = pMethods->begin (); k != pMethods->end (); k++)
	{
		(this->*((PstiPropertyUpdateMethod)(*k)))();
	}
}


stiHResult CstiCCI::PropertiesSet (
	const CstiVRCLServer::PropertyList &list)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiSettingItem::EstiSettingType eType;
	CstiVRCLServer::PropertyList::const_iterator i;
	std::set<PstiPropertyUpdateMethod, classcomp> Methods;

	for (auto &item: list)
	{
		if (item.Type == "String")
		{
			eType = SstiSettingItem::estiString;
		}
		else if (item.Type == "Number")
		{
			eType = SstiSettingItem::estiInt;
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}

		CheckProperty (item.Name.c_str (), eType, item.Value.c_str (), &Methods);
	}

	m_poPM->saveToPersistentStorage();
	CallPropertyUpdateMethods (&Methods);

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::SettingsGetProcess
//
// Description:
//
// Abstract:
//
// Returns:
//
void CstiCCI::SettingsGetProcess (
	CstiCoreResponse * poResponse)
{
	auto settingList = poResponse->SettingListGet ();

	if (!settingList.empty ())
	{
		std::set<PstiPropertyUpdateMethod, classcomp> Methods;

		for (auto &setting: settingList)
		{
			CheckProperty (setting.Name.c_str (), setting.eType, setting.Value.c_str (), &Methods);
		}

		m_poPM->saveToPersistentStorage();
		CallPropertyUpdateMethods (&Methods);
	}

	//
	// The missing list is a list of properties that were requested from
	// core but core did not have.  For some properties, since core did not have them,
	// we will send up a "setting set" request.
	//
	std::list <std::string> missingSettingsList;

	poResponse->MissingSettingsListGet (&missingSettingsList);

	if (!missingSettingsList.empty ())
	{
		for (auto &missingSetting: missingSettingsList)
		{
			for (auto &setting : m_PhoneSettings)
			{
				if (missingSetting == setting.name && setting.sendDefault)
				{
					m_poPM->PropertySend(setting.name, estiPTypePhone);
				}
			}

			for (auto &setting : m_UserSettings)
			{
				if (missingSetting == setting.name && setting.sendDefault)
				{
					m_poPM->PropertySend(setting.name, estiPTypeUser);
				}
			}
		}
	}
} // end CstiCoreService::SettingsGetProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::RequestMessageUploadGUID
//
// Description:
//
// Abstract:
//
// Returns:
//
//
stiHResult CstiCCI::RequestMessageUploadGUID ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string dialString;
	CstiMessageRequest *pMessageRequest = nullptr;
	SstiMessageInfo stMessageInfo;
	const SstiUserPhoneNumbers *pstPhoneNumbers = nullptr;
	EstiBool bBlockCallerId = estiFALSE;
	EstiMessageType messageType = estiMT_SIGN_MAIL;


	// If we have already requested an ID and not received it then cancel
	// the request before we request it again.  Also cancel the GUID request timer.
	if (m_unVideoMessageGUIDRequestId != 0)
	{
		MessageRequestCancel (m_unVideoMessageGUIDRequestId);
		m_unVideoMessageGUIDRequestId = 0;
		m_uploadGUIDRequestTimer.stop ();
	}

	pMessageRequest = new CstiMessageRequest;
	stiTESTCOND (pMessageRequest, stiRESULT_ERROR);

	// We need to add the TerpNet data pieces and can't call this until we
	// update the fileplayer.
	{
		std::lock_guard<std::recursive_mutex> lock(m_LeaveMessageMutex);

		if (m_spLeaveMessageCall)
		{
			//Check if this is an Direct SignMail
			if (m_spLeaveMessageCall->directSignMailGet())
			{
				messageType = estiMT_DIRECT_SIGNMAIL;
			}

			// Check if we have a number to use instead of the remote dial string for SignMailUploadURLGet.
			m_spLeaveMessageCall->VideoMailServerNumberGet(&dialString);

			if (dialString.empty())
			{
				m_spLeaveMessageCall->RemoteDialStringGet(&dialString);
			}

			hResult = m_spLeaveMessageCall->MessageInfoGet(&stMessageInfo);

			auto poCallInfo = (CstiCallInfo*)m_spLeaveMessageCall->LocalCallInfoGet();
			pstPhoneNumbers = poCallInfo->UserPhoneNumbersGet();

			if (m_spLeaveMessageCall->LocalCallerIdBlockedGet() &&
				m_pCoreService->APIMajorVersionGet() >= 8) // Only supported on Core 8.0 Release 2 and newer
			{
				bBlockCallerId = estiTRUE;
			}
		}
	}

	pMessageRequest->SignMailUploadURLGet (dialString.c_str(),
											messageType,
											pstPhoneNumbers ? pstPhoneNumbers->szHearingPhoneNumber : nullptr,
											bBlockCallerId,
											stMessageInfo.hearingName ? stMessageInfo.hearingName.value ().c_str () : nullptr,
											stMessageInfo.TerpId.length () == 0 ? nullptr : stMessageInfo.TerpId.c_str (),
											stMessageInfo.CallId.length () == 0 ? nullptr : stMessageInfo.CallId.c_str ());

	pMessageRequest->RemoveUponCommunicationError () = estiTRUE;

	MessageRequestSend (pMessageRequest, &m_unVideoMessageGUIDRequestId);

	// Set a timer that will close the call if we don't get an upload GUID back in a defined time.
	m_uploadGUIDRequestTimer.restart ();

	pMessageRequest = nullptr;

STI_BAIL:

	if (pMessageRequest)
	{
		delete pMessageRequest;
		pMessageRequest = nullptr;
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::videoMessageGUIDRequest
//
// Description:
//
// Abstract:
//
// Returns:
//
//
void CstiCCI::videoMessageGUIDRequest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pMessageRequest = new CstiMessageRequest;
	stiTESTCOND (pMessageRequest, stiRESULT_ERROR);

	// If GetItemId() returns invalid then we are probably viewing a Greeting
	// and had an error so don't try to get a new GUID.
	stiTESTCOND (m_pFilePlay->GetItemId ().IsValid (), stiRESULT_ERROR);

	pMessageRequest->MessageDownloadURLListGet (m_pFilePlay->GetItemId());

	pMessageRequest->RemoveUponCommunicationError () = estiTRUE;

	hResult = MessageRequestSend (pMessageRequest, &m_unVideoMessageGUIDRequestId);
	stiTESTRESULT();

	pMessageRequest = nullptr;

STI_BAIL:

	if (pMessageRequest)
	{
		delete pMessageRequest;
		pMessageRequest = nullptr;
	}
}


/*!
 * \brief Creates a ConferenceService request to get the Conference Room Status
 *
 * \param call - The call for which we want the status
 *
 * \return Success or failure result
 */
stiHResult CstiCCI::ConferenceRoomStatusGet (
	CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string Guid;
	CstiConferenceRequest *poRequest = nullptr;

	stiTESTCOND (call, stiRESULT_ERROR);

	// Does this call object have a GVC Guid?  If not, it isn't in a GVC room and does
	// not need to have this call made.
	call->ConferencePublicIdGet (&Guid);
	if (0 < Guid.length ())
	{
		// Create a conference request object
		poRequest = new CstiConferenceRequest;
		stiTESTCOND (poRequest, stiRESULT_ERROR);

		poRequest->ConferenceRoomStatusGet (Guid.c_str ());

		// Send the request
		hResult = ConferenceRequestSendEx (poRequest, nullptr);
		stiTESTRESULT ();

		// Store the request ID in the call object as AppData
		auto requestId = poRequest->requestIdGet ();
		call->AppDataSet (requestId);

		// set poRequest to NULL so that it doesn't get freed at the bottom of this method
		// in the success case.
		poRequest = nullptr;
	}

STI_BAIL:

	// If we have poRequest handle, delete it since there must have been an error
	if (stiIS_ERROR (hResult) && poRequest)
	{
		delete poRequest;
		poRequest = nullptr;
	}

return hResult;
}


/*!
 * \brief Requests the Greeting's URL, whether it is enabled and record time
 *
 * \return Success or failure result
 */
stiHResult CstiCCI::GreetingInfoGet()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = nullptr;

	// Reqeust the Upload GUID
	pMessageRequest = new CstiMessageRequest;
	if (pMessageRequest)
	{
		stiDEBUG_TOOL (g_stiUiEventDebug,
			stiTrace ("Send GreetingInfoGet request\n");
		);

		pMessageRequest->GreetingInfoGet();

		pMessageRequest->RemoveUponCommunicationError () = estiTRUE;
		hResult = MessageRequestSendEx (pMessageRequest, &m_unGreetingInfoRequestId);
		stiTESTRESULT();

		pMessageRequest = nullptr;
	}

STI_BAIL:
	if (pMessageRequest)
	{
		delete pMessageRequest;
		pMessageRequest = nullptr;
	}

	return hResult;
}


/*!\brief Gets the currernt preferences for a Personal SignMail Greeting.
 *		  The preferences are stored in the property table.
 *
 *  \retval stiRESULT_SUCCESS
 */
stiHResult CstiCCI::GreetingPreferencesGet(SstiPersonalGreetingPreferences *pGreetingPreferences) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Get the greeting type.
	int nGreetingData = 0;
	m_poPM->propertyGet(GREETING_PREFERENCE,
						   &nGreetingData,
						   PropertyManager::Persistent);
	pGreetingPreferences->eGreetingType = (eSignMailGreetingType)nGreetingData;

	// Check to see if the greeting feature has been enabled.
	m_poPM->propertyGet(PERSONAL_GREETING_ENABLED,
						   &nGreetingData,
						   PropertyManager::Persistent);

	pGreetingPreferences->bGreetingEnabled = nGreetingData == 1 ? estiTRUE : estiFALSE;

	// Get the personal greeting type.
	m_poPM->propertyGet(PERSONAL_GREETING_TYPE,
						   &nGreetingData,
						   PropertyManager::Persistent);
	pGreetingPreferences->ePersonalType = (eSignMailGreetingType)nGreetingData;

	// Get the stored text for a greeting.
	std::string storedText;
	m_poPM->propertyGet(GREETING_TEXT,
							  &storedText,
							  PropertyManager::Persistent);
	pGreetingPreferences->szGreetingText.clear();
	pGreetingPreferences->szGreetingText.append(storedText);

	return (hResult);
}

/*!\brief Stores the supplied properties for a Personal SignMail Greeting in the property table.
 *		  Changed properties are sent to core.
 *
 *  \retval stiRESULT_SUCCESS If changed properties were sent successfully.
 *  \retval stiERROR Otherwise.
 */
stiHResult CstiCCI::GreetingPreferencesSet(const SstiPersonalGreetingPreferences *pGreetingPreferences)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nGreetingType = 0;

	// Check to see if we need to update the greeting type.
	m_poPM->propertyGet(GREETING_PREFERENCE,
						   &nGreetingType,
						   PropertyManager::Persistent);
	if (pGreetingPreferences->eGreetingType != nGreetingType)
	{
		m_poPM->propertySet(GREETING_PREFERENCE,
							   pGreetingPreferences->eGreetingType,
							   PropertyManager::Persistent);
		m_poPM->PropertySend(GREETING_PREFERENCE, estiPTypeUser);
	}

	// Check to see if we need to update the personal greeting type.
	nGreetingType = 0;
	m_poPM->propertyGet(PERSONAL_GREETING_TYPE,
						   &nGreetingType,
						   PropertyManager::Persistent);
	if (pGreetingPreferences->ePersonalType != nGreetingType)
	{
		m_poPM->propertySet(PERSONAL_GREETING_TYPE,
							   pGreetingPreferences->ePersonalType,
							   PropertyManager::Persistent);
		m_poPM->PropertySend(PERSONAL_GREETING_TYPE, estiPTypeUser);
	}

	// Check to see if we need to update the Greeting Text.
	std::string storedText;
	m_poPM->propertyGet(GREETING_TEXT,
							  &storedText,
							  PropertyManager::Persistent);
	if (pGreetingPreferences->szGreetingText != storedText)
	{
		m_poPM->propertySet(GREETING_TEXT,
								  pGreetingPreferences->szGreetingText,
								  PropertyManager::Persistent);
		m_poPM->PropertySend(GREETING_TEXT, estiPTypeUser);
	}

	return (hResult);
}


stiHResult CstiCCI::GreetingUpload()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = nullptr;

	// Reqeust the Upload GUID
	pMessageRequest = new CstiMessageRequest;
	if (pMessageRequest)
	{
		stiDEBUG_TOOL (g_stiUiEventDebug,
			stiTrace ("Send GreetingUploadURL request\n");
		);

		pMessageRequest->GreetingUploadURLGet();

		pMessageRequest->RemoveUponCommunicationError () = estiTRUE;
		hResult = MessageRequestSendEx (pMessageRequest, &m_unGreetingInfoRequestId);
		stiTESTRESULT();

		pMessageRequest = nullptr;
	}

STI_BAIL:
	if (pMessageRequest)
	{
		delete pMessageRequest;
		pMessageRequest = nullptr;
	}

	if (stiIS_ERROR(hResult))
	{
		AppNotify(estiMSG_GREETING_INFO_REQUEST_FAILED, 0);
	}

	return hResult;
}

/*! \brief Gets the Local Interface Mode flag.
*
* It is recommended that this function not be called until the conference
* engine has finished initializing.  This function may be called at any time,
* but there is no guarantee that the setting is initialized or valid until the
* conference engine has reached the WAITING state.
*
* \return The interface mode currently in use.
* \retval estiSTANDARD_MODE - Can make all types of calls.
* \retval estiPUBLIC_MODE - Can make all types of calls, not allowed to access settings.
* \retval estiKIOSK_MODE - Restricted to VRS calls, not allowed to access settings.
* \retval estiINTERPRETER_MODE - Same as "Standard Mode" + Logged In as an interpreter,
*      changes what is sent as "Display Name" and how missed calls are handled.
* \retval estiVRI_MODE - Same as "Interpreter Mode" + allowed to make a second call.
* \retval estiPORTED_MODE - The videophone has been ported away from use of Core Services
*		to an alternate VRS provider.
*/
EstiInterfaceMode CstiCCI::LocalInterfaceModeGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::LocalInterfaceModeGet);

	return (m_eLocalInterfaceMode);
} // end CstiCCI::LocalInterfaceModeGet


/*! \brief Gets the current public IP address assignment method.
*
*  It is recommended that this function not be called until the conference
*  engine has finished initializing.  This function may be called at any time,
*  but there is no gaurantee that the setting is initialized or valid until the
*  conference engine has reached the WAITING state.
*
*  \return The current public IP assignment method.
*  \retval estiIP_ASSIGN_AUTO If the system is using auto-detection to assign
*      the public IP.
*  \retval estiIP_ASSIGN_MANUAL If an user-entered IP address is assigned as
*      the public IP.
*  \retval estiIP_ASSIGN_SYSTEM If the system's IP address is also used as the
*      public IP.
*/
EstiPublicIPAssignMethod CstiCCI::PublicIPAssignMethodGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::PublicIPAssignMethodGet);

	EstiPublicIPAssignMethod value{estiIP_ASSIGN_AUTO};

	m_poPM->propertyGet (cmPUBLIC_IP_ASSIGN_METHOD, &value);

	return value;
} // end CstiCCI::PublicIPAssignMethodGet


/*! \brief Gets whether to use VCO by default.
*
*  It is recommended that this function not be called until the conference
*  engine has finished initializing.  This function may be called at any time,
*  but there is no gaurantee that the setting is initialized or valid until the
*  conference engine has reached the WAITING state.
*
*  \retval estiTRUE If using VCO be default.
*  \retval estiFALSE Otherwise.
*/
bool CstiCCI::VCOUseGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::VRSUseVCOGet);

	return m_bUseVCO;
} // end CstiCCI::VRSUseVCOGet

/*! \brief Gets the current VRS VCO Callback number.
*
*  \return A constant string containing the VRS Voice Carry-Over Callback Number.
*/
stiHResult CstiCCI::VCOCallbackNumberGet (
	std::string *vcoCallbackNumber) const
{
	stiLOG_ENTRY_NAME (CstiCCI::VRSVCOCallbackNumberGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	*vcoCallbackNumber = m_vcoNumber;

	return (hResult);
} // end CstiCCI::VRSVCOCallbackNumberGet


IUserAccountManager* CstiCCI::userAccountManagerGet ()
{
	return m_userAccountManager.get ();
}

/*!
* \brief Obtain a copy of the list of relay languages supported.
*
*/
stiHResult CstiCCI::RelayLanguageListGet (
	std::vector <std::string> *pList) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::vector <CstiCoreResponse::SRelayLanguage>::const_iterator i;

	stiTESTCOND_NOLOG (pList, stiRESULT_ERROR);

	for (i = m_RelayLanguageList.begin (); i != m_RelayLanguageList.end (); i++)
	{
		pList->push_back ((*i).Name);
	}

STI_BAIL:

return hResult;
}

void CstiCCI::RelayLanguageSet (
	IstiCall *call,
	OptString language)
{
	// If the language passed in is NULL, set it to the default language.
	if (!language || language.value ().empty ())
	{
		language = CstiCall::g_szDEFAULT_RELAY_LANGUAGE;
	}

	for (auto &lang : m_RelayLanguageList)
	{
		if (lang.Name == language)
		{
			auto callObj = m_pConferenceManager->CallObjectLookup (call);
			callObj->LocalPreferredLanguageSet (lang.Name, lang.nId);
			break;
		}
	}
}


// Forces SIP registrations to a specific server to work around SMX sending 481 errors
// when using iOS Push Notifications. This function is only meant to be used with ENS
// using Push Notifications and does not prevent the SIP domain it is setting
// from being overwritten such as by a REGISTRATION_INFO_CHANGED state notify response.
// TODO: When SMX is changed test if this code is still needed.
stiHResult CstiCCI::SipProxyAddressSet (
	const std::string &sipProxyAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiConferenceParams stConferenceParams;
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	// If we're on an IPv6-only network and we're given an IPv4 address, we need
	// to convert it to an IPv6 address
	if (stConferenceParams.bIPv6Enabled && IPv4AddressValidate(sipProxyAddress.c_str ()))
	{
		std::string ip6Address;
		hResult = stiMapIPv4AddressToIPv6 (sipProxyAddress, &ip6Address);
		stiTESTRESULT ();

		stConferenceParams.SIPRegistrationInfo.PrivateDomain = ip6Address;
	}
	else
	{
		stConferenceParams.SIPRegistrationInfo.PrivateDomain = sipProxyAddress;
	}

	m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);

STI_BAIL:

	return hResult;
}


/*!
 * \brief Performs default handling of messages.
 *
 * \param n32Message The message to handle
 * \param Param Parameter data associated to the message
 */
stiHResult CstiCCI::MessageCleanup (
	int32_t n32Message,
	size_t Param)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = stiMessageCleanup (n32Message, Param);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::NetworkChangeNotify()
//
// Description: Inform the CM task of a Network IP Settings Change
//
// Abstract:
//
// Returns:
//
void CstiCCI::NetworkChangeNotify()
{
	stiLOG_ENTRY_NAME (CstiCCI::NetworkChangeNotify);
	if (m_pConferenceManager)
	{
		if (!TunnelingEnabledGet())
		{
			m_pConferenceManager->NetworkChangedNotify();
		}
		else
		{
			TunnelManagerGet()->NetworkChangeNotify();
		}
	}
	
	m_pRemoteLoggerService->NetworkTypeAndDataSet (
		IstiNetwork::InstanceGet ()->networkTypeGet (),
		IstiNetwork::InstanceGet ()->networkDataGet ());
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::ClientReRegister()
//
// Description: Inform the CM task to re-register
//
// Abstract:
//
// Returns:
//
void CstiCCI::ClientReRegister()
{
	stiLOG_ENTRY_NAME (CstiCCI::ClientReRegister);
	if (m_pConferenceManager)
	{
		m_pConferenceManager->ClientReregister();
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCCI::SipStackRestart()
//
// Description: Restart the SIP stack
//
// Abstract:
//
// Returns:
//
void CstiCCI::SipStackRestart()
{
	stiLOG_ENTRY_NAME (CstiCCI::SipStackRestart);
	if (m_pConferenceManager)
	{
		m_pConferenceManager->SipStackRestart();
	}
}

/*!
 * \brief Sets the HTTP Proxy server for the engine to use with all HTTP(s) communications
 *
 * \param httpProxyAddress The address of the HTTP Proxy.  Set to an empty string to not use a proxy.
 * \param httpProxyPort The port to connect to on the HTTP Proxy.  The port must be non-zero.
 */
stiHResult CstiCCI::HTTPProxyServerSet (
	const std::string &httpProxyAddress,
	uint16_t httpProxyPort)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!httpProxyAddress.empty ())
	{
		std::stringstream ServerAddressAndPort;

		//
		// Makes sure we have both a port.
		//
		stiTESTCOND (httpProxyPort != 0, stiRESULT_ERROR);

		CstiHTTPService::ProxyAddressSet (httpProxyAddress, httpProxyPort);

		//
		// Combine the server address and port together and store the in a single property.
		//
		ServerAddressAndPort << httpProxyAddress << ":" << httpProxyPort;

		m_poPM->propertySet (HTTP_PROXY_ADDRESS, ServerAddressAndPort.str (), PropertyManager::Persistent);
	}
	else
	{
		CstiHTTPService::ProxyAddressSet ("", 0);

		//
		// Store an empty string into the property.
		//
		m_poPM->propertySet (HTTP_PROXY_ADDRESS, "", PropertyManager::Persistent);
	}

	//
	// Send the property to core.
	//
	m_poPM->PropertySend(HTTP_PROXY_ADDRESS, estiPTypePhone);

STI_BAIL:

	return hResult;
}


/*!
 * \brief Gets the HTTP Proxy server that the engine is currently using
 *
 * \param httpProxyAddress The address of the HTTP Proxy.  This will be an empty string if a proxy is not being used.
 * \param httpProxyPort The port of the HTTP Proxy.  This will be zero if a proxy is not being used.
 */
stiHResult CstiCCI::HTTPProxyServerGet (
	std::string *httpProxyAddress,
	uint16_t *httpProxyPort) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND(httpProxyAddress != nullptr && httpProxyPort != nullptr, stiRESULT_ERROR);

	CstiHTTPService::ProxyAddressGet(httpProxyAddress, httpProxyPort);

STI_BAIL:

	return hResult;
}


/*!
 * \brief Set the SIP proxy information
 *  \retval estiOK If the value was successfully stored.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::RegistrationConferenceParamsSet ()
{
	stiLOG_ENTRY_NAME (CstiCCI::RegistrationConferenceParamsSet);
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	SstiRegistrationInfo RegistrationInfo;

	std::string privateDomainOverride;
	std::string privateDomainAltOverride;
	std::string publicDomainOverride;
	std::string publicDomainAltOverride;
	std::string agentDomainOverride;
	std::string agentDomainAltOverride;

	int nRestartProxyMaxLookupTime = CLIENT_REREGISTER_MAX_LOOKUP_TIME_DEFAULT;
	int nRestartProxyMinLookupTime = CLIENT_REREGISTER_MIN_LOOKUP_TIME_DEFAULT;

	// Set the info into the conference params
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);

	stiTESTRESULT ();

	RegistrationInfo = m_pRegistrationManager->SipRegistrationInfoGet ();

	// Core does not return Agent domain information at this time.
	// This is causing the Agent Domain information to not properly persist after receiving a RegistrationInfoGetResult from Core.
	// Copying the existing information from ConferenceParams out below fixes bug #22489.
	if (estiINTERPRETER_MODE == m_eLocalInterfaceMode
		|| estiVRI_MODE == m_eLocalInterfaceMode
		|| estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode)
	{
		RegistrationInfo.AgentDomain = stConferenceParams.SIPRegistrationInfo.AgentDomain;
	}

	stConferenceParams.SIPRegistrationInfo = RegistrationInfo;

	// Private Domain Override: look for SipPrivateDomainOverride tag in Property Table
	m_poPM->propertyGet (SIP_PRIVATE_DOMAIN_OVERRIDE, &privateDomainOverride, PropertyManager::Persistent);

	if (!privateDomainOverride.empty ())
	{
		stiDEBUG_TOOL (g_stiCCIDebug,
			stiTrace ("Overriding SIP Private domain with: %s\n", privateDomainOverride.c_str ());
		);
		stConferenceParams.SIPRegistrationInfo.PrivateDomain = privateDomainOverride;
	}

	// Private Domain Alternate Override: look for SipPrivateDomainAltOverride tag in Property Table
	m_poPM->propertyGet (
		SIP_PRIVATE_DOMAIN_ALT_OVERRIDE,
		&privateDomainAltOverride,
		PropertyManager::Persistent);

	if (!privateDomainAltOverride.empty ())
	{
		stiDEBUG_TOOL (g_stiCCIDebug,
			stiTrace ("Overriding SIP Private domain alternate with: %s\n", privateDomainAltOverride.c_str());
		);
		stConferenceParams.SIPRegistrationInfo.PrivateDomainAlt = privateDomainAltOverride;
	}

	//
	// Public Domain Override: look for SipPublicDomainOverride tag in Property Table
	//
	m_poPM->propertyGet (SIP_PUBLIC_DOMAIN_OVERRIDE, &publicDomainOverride, PropertyManager::Persistent);

	if (!publicDomainOverride.empty ())
	{
		stiDEBUG_TOOL (g_stiCCIDebug,
			stiTrace ("Overriding SIP Public domain with: %s\n", publicDomainOverride.c_str ());
		);
		stConferenceParams.SIPRegistrationInfo.PublicDomain = publicDomainOverride;
	}

	//
	// Public Domain Alternate Override: look for SipPublicDomainAltOverride tag in Property Table
	//
	m_poPM->propertyGet (SIP_PUBLIC_DOMAIN_ALT_OVERRIDE, &publicDomainAltOverride, PropertyManager::Persistent);

	if (!publicDomainAltOverride.empty ())
	{
		stiDEBUG_TOOL (g_stiCCIDebug,
			stiTrace ("Overriding SIP Public domain alternate with: %s\n", publicDomainAltOverride.c_str());
		);
		stConferenceParams.SIPRegistrationInfo.PublicDomainAlt = publicDomainAltOverride;
	}

	if (estiINTERPRETER_MODE == m_eLocalInterfaceMode
		|| estiVRI_MODE == m_eLocalInterfaceMode
		|| estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode)
	{
		//
		// Agent Domain Override: look for SipAgentDomainOverride tag in Property Table
		//
		m_poPM->propertyGet (SIP_AGENT_DOMAIN_OVERRIDE, &agentDomainOverride, PropertyManager::Persistent);

		if (!agentDomainOverride.empty ())
		{
			stiDEBUG_TOOL (g_stiCCIDebug,
				stiTrace ("Overriding SIP Agent domain with: %s\n", agentDomainOverride.c_str ());
			);
			stConferenceParams.SIPRegistrationInfo.AgentDomain = agentDomainOverride;
		}

		//
		// Agent Domain Alternate Override: look for SipAgentDomainAltOverride tag in Property Table
		//
		m_poPM->propertyGet (SIP_AGENT_DOMAIN_ALT_OVERRIDE, &agentDomainAltOverride, PropertyManager::Persistent);

		if (!agentDomainAltOverride.empty ())
		{
			stiDEBUG_TOOL (g_stiCCIDebug,
				stiTrace ("Overriding SIP Agent domain alternate with: %s\n", agentDomainAltOverride.c_str ());
			);
			stConferenceParams.SIPRegistrationInfo.AgentDomainAlt = agentDomainAltOverride;
		}
	}
	else
	{
		stConferenceParams.SIPRegistrationInfo.AgentDomain.clear ();
		stConferenceParams.SIPRegistrationInfo.AgentDomainAlt.clear ();
	}

	//
	// Retrieve and set the number of seconds to restart the proxy lookup.
	//
	m_poPM->propertyGet (CLIENT_REREGISTER_MAX_LOOKUP_TIME, &nRestartProxyMaxLookupTime);
	m_poPM->propertyGet (CLIENT_REREGISTER_MIN_LOOKUP_TIME, &nRestartProxyMinLookupTime);

	stConferenceParams.SIPRegistrationInfo.nRestartProxyMaxLookupTime = nRestartProxyMaxLookupTime;
	stConferenceParams.SIPRegistrationInfo.nRestartProxyMinLookupTime = nRestartProxyMinLookupTime;


	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}


/*!
 * \brief Notifies the MessageManager of the message's new pause point.
 */
void CstiCCI::UpdatePausePoint()
{
#ifdef stiMESSAGE_MANAGER
	CstiMessageInfo messageInfo;
	messageInfo.ItemIdSet(m_pFilePlay->GetItemId());

	uint32_t un32PausePoint = m_pFilePlay->StopPositionGet();

	m_pMessageManager->Lock();

	m_pMessageManager->MessageInfoGet(&messageInfo);

	// Only log the the pause point in splunk for non-SignMail videos.
	if (messageInfo.MessageTypeIdGet() != estiMT_SIGN_MAIL &&
		messageInfo.MessageTypeIdGet() != estiMT_DIRECT_SIGNMAIL && 
		messageInfo.MessageTypeIdGet() != estiMT_P2P_SIGNMAIL &&
		messageInfo.MessageTypeIdGet() != estiMT_THIRD_PARTY_VIDEO_MAIL)
	{
		// Log a splunk event to track how much of a video has been viewed.
		stiRemoteLogEventSend ("EventType=VideoViewed Title=\"%s\" PausePoint=%d VideoLength=%d", messageInfo.NameGet(), un32PausePoint, messageInfo.MessageLengthGet());
	}

	messageInfo.PausePointSet(un32PausePoint);
	messageInfo.ViewIdSet(m_pFilePlay->GetViewId());
	m_pMessageManager->MessageInfoSet(&messageInfo);

	m_pMessageManager->Unlock();
#endif
}


void CstiCCI::userIdsSet (
	const std::string &userId,
	const std::string &groupId)
{
	// Make sure the user ID set. The group ID may or may not be set.
	if (!userId.empty ())
	{
		m_pConferenceManager->UserIdsSet (userId, groupId);
		m_pRemoteLoggerService->CoreUserIdsSet (userId, groupId);

		// Check the existing UserId and GroupUserId in PM to see if the one we received is different
		std::string phoneUserId;

		m_poPM->propertyGet (
			PHONE_USER_ID,
			&phoneUserId);

		std::string phoneGroupUserId;

		m_poPM->propertyGet (
			GROUP_USER_ID,
			&phoneGroupUserId);

		// Has it changed?
		bool bUserIdChanged = (phoneUserId != userId);
		bool bGroupIdChanged = (phoneGroupUserId != groupId);

		if (bUserIdChanged || bGroupIdChanged)
		{
			// yes, clear the user settings
			//  (currently only the Ring group enabled and display mode settings)
			PropertiesRemove (m_UserSettings);
			RingGroupSettingsChanged ();
			RealTimeTextEnabledChanged ();

			if (m_pContactManager)
			{
				m_pContactManager->purgeContacts ();
			}

			if (m_pBlockListManager)
			{
				m_pBlockListManager->PurgeItems ();
			}

			// save the property to the property table
			m_poPM->propertySet (
				PHONE_USER_ID,
				userId,
				PropertyManager::Persistent);
			// save the property to the property table
			m_poPM->propertySet (
				GROUP_USER_ID,
				groupId,
				PropertyManager::Persistent);
	#ifdef stiCALL_HISTORY_MANAGER
			// Only purge call history cache when the user ID changes.  No need to do this when joining/leaving ring groups.
			if (bUserIdChanged)
			{
				CallHistoryManagerGet ()->CachePurge ();
			}
	#endif
		}
	}
}


void CstiCCI::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
{
	if (clientAuthenticateResult->responseSuccessful)
	{
		stiRemoteLogEventSend ("EventType=CoreLogin Reason=LoginSucceeded");

		m_pConferenceManager->LoggedInSet (true);

		userIdsSet (clientAuthenticateResult->content.userId, clientAuthenticateResult->content.groupUserId);

		//
		// We have been authenticated by core.  If we are not
		// currently setup to do SIP NAT Traversal then signal
		// that we are ready for conferencing.
		//
		int nNatTraversalSIP = stiNAT_TRAVERSAL_SIP_DEFAULT;
		PropertyManager::getInstance ()->propertyGet (NAT_TRAVERSAL_SIP,
			(int*)&nNatTraversalSIP);

		if (nNatTraversalSIP == estiSIPNATDisabled)
		{
			ConferencingReadyStateSet (estiConferencingStateReady);
		}

		if (clientAuthenticateResult->origin == vpe::ResponseOrigin::VPServices)
		{
			m_poPM->propertySet (CORE_AUTH_TOKEN, clientAuthenticateResult->content.authToken, PropertyManager::Persistent);

			// Request the user interface group that we should be using.
			auto  poCoreRequest = new CstiCoreRequest ();
			unsigned int unCoreRequestId;
			poCoreRequest->userInterfaceGroupGet ();

			IstiRingGroupManager::ERingGroupDisplayMode nMode = RingGroupManagerGet ()->ModeGet ();
			//
			// Request the ring group information
			//
			if (nMode != IstiRingGroupManager::eRING_GROUP_DISABLED)
			{
				// Get RingGroup Information (if it exists)
				poCoreRequest->ringGroupInfoGet ();
			}


			// Request the registration information for SIP
			poCoreRequest->registrationInfoGet ();

			// Request the user account information
			poCoreRequest->userAccountInfoGet ();

			auto hResult = CoreRequestSend (poCoreRequest, &unCoreRequestId);

			if (!stiIS_ERROR (hResult))
			{
				m_unRegistrationInfoRequestId = unCoreRequestId;
				m_unUserAccountInfoRequestId = unCoreRequestId;
				m_unUserInterfaceGroupRequestId = unCoreRequestId;
			}

			PropertiesCheck (m_UserSettings, estiPTypeUser, nullptr);
		}

		m_poPM->PropertySend (cmFIRMWARE_VERSION, estiPTypePhone);

		m_poPM->PropertySend (cmPUBLIC_IP_ASSIGN_METHOD, estiPTypePhone);
		m_poPM->PropertySend (cmPUBLIC_IP_ADDRESS_MANUAL, estiPTypePhone);

		m_poPM->PropertySend (BOOT_COUNT, estiPTypePhone);

		//
		// Send the current ports being used to Core Services
		//
		SendCurrentPortsToCore ();
		SendPortSettingsToCore ();
	}
	else
	{
		// Clear the porting back status.
		m_portingBack = false;

		stiRemoteLogEventSend ("EventType=CoreLogin Reason=LoginFailed Error=%d", clientAuthenticateResult->coreErrorNumber);
	}
	clientAuthenticateSignal.Emit (clientAuthenticateResult);
}


void CstiCCI::userAccountInfoGetHandle (const ServiceResponseSP<CstiUserAccountInfo>& userAccountInfoResponse)
{
	if (userAccountInfoResponse->requestId == m_unUserAccountInfoRequestId)
	{
		if (userAccountInfoResponse->responseSuccessful)
		{
			userAccountInfoProcess (&userAccountInfoResponse->content);
		}
		m_unUserAccountInfoRequestId = 0;
	}
	else
	{
		userAccountInfoProcess (&userAccountInfoResponse->content);

		// The password has (or may have) changed, therefore we need to re-request credentials
		auto  poCoreRequest = new CstiCoreRequest ();
		poCoreRequest->registrationInfoGet ();
		CoreRequestSend (poCoreRequest, &m_unRegistrationInfoRequestId);
	}
	userAccountInfoReceivedSignal.Emit (userAccountInfoResponse);
}


/*!
 * \brief Determines if the dial string attempted is a member of this endpoint's ring group.
 */
EstiBool CstiCCI::IsDialStringInRingGroup(
	const char *pszDialString)
{
	EstiBool bIsInGroup = estiFALSE;

	if (nullptr == pszDialString || '\0' == pszDialString[0])
	{
		bIsInGroup = estiFALSE;
	}
	else if (0 == strcmp(m_sUserPhoneNumbers.szRingGroupLocalPhoneNumber, pszDialString)
	 || 0 == strcmp(m_sUserPhoneNumbers.szRingGroupTollFreePhoneNumber, pszDialString))
	{
		bIsInGroup = estiTRUE;
	}
	else
	{
		std::string Description;
		IstiRingGroupManager::ERingGroupMemberState eState;

		if (m_pRingGroupManager->ItemGetByNumber (pszDialString, &Description, &eState))
		{
			bIsInGroup = estiTRUE;
		}
	}

	return bIsInGroup;
}

#ifdef stiTUNNEL_MANAGER
IstiTunnelManager* CstiCCI::TunnelManagerGet ()
{
	return m_pConferenceManager->TunnelManagerGet();
}
#endif

void CstiCCI::AllowIncomingCallsModeSet (bool bAllow)
{
	stiLOG_ENTRY_NAME (CstiCCI::AllowIncomingCallsModeSet);

	// Update the AllowIncomingCall Mode
	m_pConferenceManager->AllowIncomingCallsModeSet (bAllow);
}


stiHResult CstiCCI::DiagnosticsSend ()
{
	// Obtain and send Sip Stack staistics
	SstiRvSipStats stRvSipStats;
	m_pConferenceManager->RvSipStatsGet (&stRvSipStats);
	std::stringstream SipStats;
	SipStats << stRvSipStats;
//	stiTrace ("\n\nRadvision SIP Stats\n%s\n", SipStats.str().c_str ());
	m_pRemoteLoggerService->SipStackStatsSend (SipStats.str().c_str ());
//	m_pRemoteLoggerService->EndPointDiagnosticsSend (SipStats.str().c_str ());
	return stiRESULT_SUCCESS;
}


#ifdef stiCALL_HISTORY_MANAGER
void CstiCCI::AddCallToCallHistory(
	CstiCallSP call)
{
	// Check whether this has already been added to call list
	if (estiFALSE == call->AddedToCallListGet())
	{
		CstiCallList::EType eListType = CstiCallList::eTYPE_UNKNOWN;
		EstiDialMethod eMethod = estiDM_UNKNOWN;
		std::string CallId;
		std::string IntendedDialString;

		// Get the CallId from the Call object
		call->CallIdGet(&CallId);
		
		if (call->directSignMailGet())
		{
			//
			// Direct SignMails should not go into a call list
			//
			eListType = CstiCallList::eTYPE_UNKNOWN;
		}
		else if (call->DirectionGet() == estiOUTGOING)
		{
			//
			// Add to the dialed call list.
			//
			eListType = CstiCallList::eDIALED;

		}
		else if (call->DirectionGet() == estiINCOMING
				&& call->ConferencedGet ())
		{
			//
			// Add to received call list.
			//
			eListType = CstiCallList::eANSWERED;

			// Get the IntendedDialString from the Call object
			call->IntendedDialStringGet(&IntendedDialString);
		}

		eMethod = call->DialMethodGet ();

		if (eListType != CstiCallList::eTYPE_UNKNOWN)
		{
			CstiCallHistoryItemSharedPtr spCallItem = std::make_shared<CstiCallHistoryItem> ();

			if (spCallItem)
			{
				bool bBlockedCallerID = false;

				spCallItem->CallTimeSet (call->StartTimeGet());

				std::string Name;
				call->RemoteNameGet(&Name);
				spCallItem->NameSet(Name);

				std::string DialString;
				//Get the DialString using DialStringGet
				call->DialStringGet (&eMethod, &DialString);
				spCallItem->DialMethodSet (eMethod);
				//If DialStringGet is empty, try and retrieve the number from the preferred phone number
				if(DialString.empty())
				{
					const SstiUserPhoneNumbers *psUserPhoneNumbers = call->RemoteCallInfoGet()->UserPhoneNumbersGet ();
					if (0 != strlen (psUserPhoneNumbers->szPreferredPhoneNumber))
					{
						DialString = call->RemoteCallInfoGet()->UserPhoneNumbersGet()->szPreferredPhoneNumber;
					}

					// Determine if remote caller id is blocked
					if (call->RemoteCallerIdBlockedGet())
					{
						bBlockedCallerID = true;
					}
				}

				//Add the Dial String to the Call Item
				spCallItem->DialStringSet (DialString);

				if (call->IsInContactsGet())
				{
					spCallItem->InSpeedDialSet (estiTRUE);
				}

				if (!CallId.empty())
				{
					spCallItem->CallPublicIdSet (CallId);
				}

				spCallItem->callIndexSet(call->CallIndexGet());

				if (!IntendedDialString.empty())
				{
					spCallItem->IntendedDialStringSet (IntendedDialString);
				}

				//
				// Add it to the core services list
				//
				CallHistoryManagerGet()->ItemAdd(eListType, spCallItem, bBlockedCallerID, nullptr);

				call->AddedToCallListSet(estiTRUE);
			}
		}
	}
}
#endif


/*!\brief Determine if the Group Video Chat is allowed
*
* \param call - The call object being considered for Group Video Chat
* \param pbLackOfResources - A return parameter that if passed in, will be loaded
* 		with true if this call object is in a GVC room and the MCU hasn't enough resources
* 		to add another participant.
*
*
* \return estiTRUE if Group Video Chat is allowed for this call object
* \return estiFALSE otherwise
*/
bool CstiCCI::GroupVideoChatAllowed (
	CstiCallSP call,
	bool *pbLackOfResources) const
{
	bool bAllowed = false;

	stiDEBUG_TOOL (g_stiGVC_Debug,
		stiTrace ("<CCI::GroupVideoChatAllowed 2> Connected MCU:%s, Transferrable:%d, RemoteInterface:%s\n",
			call->ConnectedWithMcuGet (estiMCU_NONE) ? "None" :
			call->ConnectedWithMcuGet (estiMCU_GENERIC) ? "Non GVC MCU" :
			call->ConnectedWithMcuGet (estiMCU_GVC) ? "GVC MCU" : "???",
			call->IsTransferableGet (),
			estiSTANDARD_MODE == call->RemoteInterfaceModeGet () ? "Standard" :
			estiPUBLIC_MODE == call->RemoteInterfaceModeGet () ? "Public" :
			estiINTERPRETER_MODE == call->RemoteInterfaceModeGet () ? "Terp" :
			estiVRI_MODE == call->RemoteInterfaceModeGet () ? "VRI Terp" :
			estiTECH_SUPPORT_MODE == call->RemoteInterfaceModeGet () ? "Tech Support" : "???");

		stiTrace ("\tRemoteDeviceType:%s, LocalInterfaceMode:%s\n",
			call->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER) ? "HoldServer" :
			call->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_VIDEOPHONE) ? "SVRS Videophone" : "???",
			estiSTANDARD_MODE == m_eLocalInterfaceMode ? "Standard" :
			estiPUBLIC_MODE == m_eLocalInterfaceMode ? "Public" :
			estiINTERPRETER_MODE == m_eLocalInterfaceMode ? "Terp" :
			estiVRI_MODE == m_eLocalInterfaceMode ? "VRI Terp" :
			estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode ? "Tech Support" : "???");

		if (call->ConnectedWithMcuGet (estiMCU_GVC))
		{
			stiTrace ("\tThere is%s room for another participant\n",
				call->ConferenceRoomParticipantAddAllowed (nullptr) ? "" : " not");
		}
	);

	if (
		// If we are "adding" a person to an existent room, does the MCU have room and resources?
		(call->ConnectedWithMcuGet (estiMCU_GVC) &&
				call->ConferenceRoomParticipantAddAllowed (pbLackOfResources)) ||

		// Not connected with a GroupVideoChat MCU.  Test various other things.
		// Does this call support "Refer"?
		(call->IsTransferableGet () &&

		// Remote connection can't be in Interpreter or VRI or Ported interface modes
		estiINTERPRETER_MODE != call->RemoteInterfaceModeGet () &&	// Doesn't seem that we need to check this.  REFER isn't present in this case.
		estiVRI_MODE != call->RemoteInterfaceModeGet () &&
		estiPORTED_MODE != call->RemoteInterfaceModeGet () &&
		estiTECH_SUPPORT_MODE != call->RemoteInterfaceModeGet () &&

		// Remote connection can't be a Hold Server
		! call->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER) &&  // Doesn't seem that we need to check this; REFER isn't present in this case.

		// The local device cannot be in Interpreter or Public interface mode
		estiINTERPRETER_MODE != m_eLocalInterfaceMode &&	// Doesn't seem that we need to check this.  REFER is UI isn't allowing us access.
		estiPUBLIC_MODE != m_eLocalInterfaceMode))		// Doesn't seem that we need to check this.  REFER is UI isn't allowing us access.
	{
		bAllowed = true;
	}

	stiDEBUG_TOOL (g_stiGVC_Debug,
			stiTrace ("<CCI::GroupVideoChatAllowed 2> Returning %d\n", bAllowed);
	);

	return bAllowed;
}


//#define stiTEST_DIALSTRINGS
/*!\brief Determine if the Group Video Chat is allowed
*
* \param call1 - The 1st call object being considered for Group Video Chat
* \param call2 - The 2nd call object being considered for Group Video Chat
* \param pbLackOfResources - A return parameter that if passed in, will be loaded
* 		with true if this call object is in a GVC room and the MCU hasn't enough resources
* 		to add another participant.
*
* \NOTE: The return parameters should be passed in as NULL unless being called by the
*		method that is joining the calls into a GVC (CstiCCI::GroupVideoChatJoin).
*
* \return estiTRUE if Group Video Chat is allowed for the combination of these two call objects
* \return estiFALSE otherwise
*/
bool CstiCCI::GroupVideoChatAllowed (
	CstiCallSP call1,
	CstiCallSP call2,
	bool *pbLackOfResources) const
{
	bool bAllowed = false;

#ifdef stiTEST_DIALSTRINGS
	std::string DialString1;
	std::string DialString2;
#endif

	ICallInfo *pCallInfo1 = nullptr;
	ICallInfo *pCallInfo2 = nullptr;

	std::string userId1;
	std::string userId2;
	bool bHaveUserIds = false;
	bool bGvcMcuInUse = false;
	bool bCall1ConnectedWithGVC = call1->ConnectedWithMcuGet (estiMCU_GVC);
	bool bCall2ConnectedWithGVC = call2->ConnectedWithMcuGet (estiMCU_GVC);

	bool bCall1OnHoldRemotely = call1->StateValidate(esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT);
	bool bCall2OnHoldRemotely = call2->StateValidate(esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT);
	if (call1->StateGet() == esmiCS_CONNECTED && call1->SubstateValidate(estiSUBSTATE_NEGOTIATING_RMT_HOLD))
	{
		bCall1OnHoldRemotely = true;
	}
	if (call2->StateGet() == esmiCS_CONNECTED && call2->SubstateValidate(estiSUBSTATE_NEGOTIATING_RMT_HOLD))
	{
		bCall2OnHoldRemotely = true;
	}

	// If we received return parameters, initialize them now
	if (pbLackOfResources)
	{
		*pbLackOfResources = false;
	}

#ifdef stiTEST_DIALSTRINGS
	// Get the remote dial strings
	call1->RemoteDialStringGet (&DialString1);
	call2->RemoteDialStringGet (&DialString2);
#endif

	// Is one of the nodes connected to a GVC MCU?
	if (bCall1ConnectedWithGVC || bCall2ConnectedWithGVC)
	{
		bGvcMcuInUse = true;
	}

	else
	{
		// We will need to look at either the remote call info or the callIDs
		pCallInfo1 = call1->RemoteCallInfoGet ();
		pCallInfo2 = call2->RemoteCallInfoGet ();

		pCallInfo1->UserIDGet (&userId1);
		pCallInfo2->UserIDGet (&userId2);
		bHaveUserIds = !userId1.empty () && !userId2.empty ();
	}

	stiDEBUG_TOOL (g_stiGVC_Debug,
		stiTrace ("<CCI::GroupVideoChatAllowed 1> \n\tEnabled = %d, Call1 MCU Connection: %s, Call2 MCU Connection: %s, Call1 remote hold: %s, Call2 remote hold: %s\n",
			GroupVideoChatEnabled (),
			bCall1ConnectedWithGVC ? "GVC MCU" :
			call1->ConnectedWithMcuGet (estiMCU_NONE) ? "None" :
			call1->ConnectedWithMcuGet (estiMCU_GENERIC) ? "Non GVC MCU" : "???",
			bCall2ConnectedWithGVC ? "GVC MCU" :
			call2->ConnectedWithMcuGet (estiMCU_NONE) ? "None" :
			call2->ConnectedWithMcuGet (estiMCU_GENERIC) ? "Non GVC MCU" : "???",
			bCall1OnHoldRemotely ? "True" : "False",
			bCall2OnHoldRemotely ? "True" : "False");

		if (bHaveUserIds)
		{
			stiTrace ("\tUserId1 = %s, UserId2 = %s\n", userId1.c_str (), userId2.c_str ());
		}
	);

	#ifdef stiTEST_DIALSTRINGS
	stiDEBUG_TOOL(g_stiGVC_Debug,
				stiTrace ("\tDialStrings are Call1: %s, Call2: %s\n", DialString1.c_str (), DialString2.c_str ());
		);
	#endif

	// Test that both call objects are valid
	if ((call1 && call2) &&

		// Is the feature enabled
		(GroupVideoChatEnabled ()) &&

		// Test that at most, one of the call objects is connected with a GroupVideoChat MCU already
		(!bCall1ConnectedWithGVC || !bCall2ConnectedWithGVC) &&

		// Check each call object for additional rules
		GroupVideoChatAllowed (call1, pbLackOfResources) &&
		GroupVideoChatAllowed (call2, pbLackOfResources) &&

		// Don't try and join someone if they have put you on hold
		!bCall1OnHoldRemotely &&
		!bCall2OnHoldRemotely &&

		// Do the two call legs to be joined originate from the same phone?  If one
		// is connected with a GVC MCU, we don't need to test.
		// NOTE:  We cannot test against phone numbers since two phones may be from the
		// same myPhone group and therefore have the same number.

#ifndef stiTEST_DIALSTRINGS // Don't test phone numbers
		(bGvcMcuInUse || !bHaveUserIds || userId1 != userId2))

#else // Test against phone numbers
		// Doing the following two tests fail when two separate phones that are part of the
		// same myPhone group are being tested (one in the GVC and the other connected
		// to a second call leg of this device).
		(bGvcMcuInUse || ((bHaveUserIds && userId1 != userId2) ||
			(!bHaveUserIds && DialString1 != DialString2))) &&

		// If one of the call legs is already connected with the GVC, the other
		// call leg cannot be from the same device as one of the participants
		// already in the GVC room.
		(!bGvcMcuInUse ||
				((bCall1ConnectedWithGVC &&
				  !call1->ConferenceRoomParticipantListHas (&DialString2))) ||

				((bCall2ConnectedWithGVC &&
				  !call2->ConferenceRoomParticipantListHas (&DialString1)))))
#endif
	{
		bAllowed = true;
	}

	stiDEBUG_TOOL (g_stiGVC_Debug,
			stiTrace ("<CCI::GroupVideoChatAllowed 1> Returning %d\n", bAllowed);
	);

	return bAllowed;
}


/*!\brief Determine if the Group Video Chat is allowed
*
* \param poCall1 - The 1st call object being considered for Group Video Chat
* \param poCall2 - The 2nd call object being considered for Group Video Chat
*
* \return estiTRUE if Group Video Chat is allowed for the combination of these two call objects
* \return estiFALSE otherwise
*/
bool CstiCCI::GroupVideoChatAllowed (
		IstiCall *poCall1,
		IstiCall *poCall2) const
{
	// Lookup the shared_ptr call objects
	auto call1 = m_pConferenceManager->CallObjectLookup (poCall1);
	auto call2 = m_pConferenceManager->CallObjectLookup (poCall2);

	return GroupVideoChatAllowed (call1, call2, nullptr);
}


/*!\brief Attempt to join the two calls into a Group Video Chat room.
 * 	If one of the calls is already in a GVC room, a new room will not be created
 * 	but instead, if allowed, we will join the other call to that room.
 *
 * \param poCall1 - The 1st call object being considered for Group Video Chat
 * \param poCall2 - The 2nd call object being considered for Group Video Chat
 *
 * \return an stiHResult indicating success or failure of this function.
 */
stiHResult CstiCCI::GroupVideoChatJoin (
		IstiCall *poCall1,
		IstiCall *poCall2)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiConferenceRequest * poRequest = nullptr;
	bool bLackOfResources = false;

	// Lookup the shared_ptr call objects
	auto call1 = m_pConferenceManager->CallObjectLookup (poCall1);
	auto call2 = m_pConferenceManager->CallObjectLookup (poCall2);

	// Test that issuing the join is allowed.
	bool bAllowed = GroupVideoChatAllowed (call1, call2, &bLackOfResources);

	if (bAllowed)
	{

		//
		// If either of the calls is already connected with an MCU, just refer the other call
		//
		if (call1->ConnectedWithMcuGet(estiMCU_GVC))
		{
			//
			// Refer the non-conferenced call to the conference room
			//
			call2->TransferToAddress (call1->RoutingAddressGet ().UriGet ());
		}

		else if (call2->ConnectedWithMcuGet(estiMCU_GVC))
		{
			//
			// Refer the non-conferenced call to the conference room
			//
			call1->TransferToAddress (call2->RoutingAddressGet ().UriGet ());
		}

		else
		{
			bool bEncrypted = false;

			// Create a conference request object
			poRequest = new CstiConferenceRequest;
			stiTESTCOND (poRequest, stiRESULT_ERROR);

			poRequest->ConferenceRoomCreate (bEncrypted, CstiConferenceRequest::ConferenceRoomType::Generic);

			// Send the request
			hResult = ConferenceRequestSendEx (poRequest, &m_unConferenceRoomCreateRequestId);
			stiTESTRESULT ();

			// Set this to NULL so it is not freed in the clean up step.
			poRequest = nullptr;

			// Add the requestId to the endpoints so we can join them to the
			// conference room upon return.
			call1->AppDataSet ((size_t)m_unConferenceRoomCreateRequestId);
			call2->AppDataSet ((size_t)m_unConferenceRoomCreateRequestId);

			// Start a timer.  If it expires before we get the response, we will abandon the request
			// and alert the UI of the failure.
			m_groupVideoChatRoomCreateTimer.restart ();
		}
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		if (!bAllowed && bLackOfResources)
		{
			CstiCallSP call = (call1 ? call1 : call2);

			// Notify the application of the lack of resources.
			AppNotify (estiMSG_CONFERENCE_SERVICE_INSUFFICIENT_RESOURCES, 0);

			// Log a message with Splunk
			std::string ConferenceId;
			const ICallInfo *pCallInfo = nullptr;
			std::string userID;

			if (call)
			{
				pCallInfo = call->LocalCallInfoGet ();
				pCallInfo->UserIDGet (&userID);
				call->ConferencePublicIdGet (&ConferenceId);
			}

			stiRemoteLogEventSend ("EventType=GVC Reason=\"AddFailed-Resources\" GUID=%s CoreId=%s",
					ConferenceId.c_str (),
					userID.c_str ());
		}

		if (poRequest)
		{
			if (m_unConferenceRoomCreateRequestId != 0)
			{
				m_pConferenceService->RequestCancel (m_unConferenceRoomCreateRequestId);
				m_unConferenceRoomCreateRequestId = 0;
			}

			delete poRequest;
			poRequest = nullptr;
		}
	}

	return hResult;
}


/*!\brief Determine if the Group Video Chat feature is enabled
*
* \return true - if Group Video Chat feature is enabled
* \return false - otherwise
*/
bool CstiCCI::GroupVideoChatEnabled () const
{
	int nGroupVideoChatEnabled = 0;
	PropertyManager::getInstance()->propertyGet(
		GROUP_VIDEO_CHAT_ENABLED,
		&nGroupVideoChatEnabled,
		PropertyManager::Persistent);
	return (bool)nGroupVideoChatEnabled;
}


/*!\brief Sends a confrence room create request for the purpose of DHVI
*
*/
stiHResult CstiCCI::dhviCreate (IstiCall *call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	bool bEncrypted = false;
	
	// Lookup the shared_ptr call objects
	auto call1 = m_pConferenceManager->CallObjectLookup (call);
	std::string callID;
	
	call->CallIdGet(&callID);
	
	stiRemoteLogEventSend ("EventType=DHVI Reason=StartDHVI CallID=%s", callID.c_str ());
	
	// Create a conference request object
	auto request = new CstiConferenceRequest;
	stiTESTCOND (request, stiRESULT_ERROR);
	
	request->ConferenceRoomCreate (bEncrypted, CstiConferenceRequest::ConferenceRoomType::Dhvi);
	
	// Send the request
	hResult = ConferenceRequestSendEx (request, &m_dhviCreateRequestId);
	stiTESTRESULT ();
	
	
	call1->dhviStateSet (IstiCall::DhviState::Connecting);
	
	// Set this to NULL so it is not freed in the clean up step.
	request = nullptr;
	
	// Add the requestId to the endpoints so we can join them to the
	// conference room upon return.
	call1->AppDataSet ((size_t)m_dhviCreateRequestId);
	
	m_dhviRoomCreateTimer.start ();
	
STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		if (request)
		{
			if (m_dhviCreateRequestId != 0)
			{
				m_pConferenceService->RequestCancel (m_dhviCreateRequestId);
				m_dhviCreateRequestId = 0;
				call1->dhviStateSet (IstiCall::DhviState::Failed);
			}
			
			delete request;
			request = nullptr;
		}
	}
	
	return hResult;
}


/*! \brief Gets the enabled status of the block caller id feature.
 *
 *  \retval estiON If the block caller ID featureis enabled.
 *  \retval estiOFF If it is turned off.
 */
EstiSwitch CstiCCI::BlockCallerIDEnabledGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::BlockCallerIDEnabledGet);

	EstiSwitch eFeatureEnabled = estiOFF;

	int nBlockCallerIDEnabled = 0;	
	if (estiSTANDARD_MODE == m_eLocalInterfaceMode 
		|| estiHEARING_MODE == m_eLocalInterfaceMode
		|| estiPUBLIC_MODE == m_eLocalInterfaceMode)
	{
		m_poPM->propertyGet (BLOCK_CALLER_ID_ENABLED, &nBlockCallerIDEnabled);
	}
	
	eFeatureEnabled = nBlockCallerIDEnabled ? estiON : estiOFF;

	return eFeatureEnabled;
}


/*! \brief Gets the current block caller ID setting (on/off).
 *
 *  \retval estiON If block caller ID is turned on.
 *  \retval estiOFF If it is turned off.
 */
EstiSwitch CstiCCI::BlockCallerIDGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::BlockCallerIDGet);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiSwitch eBlock = estiOFF;

	SstiConferenceParams stConferenceParams;

	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();

	eBlock = stConferenceParams.bBlockCallerID ? estiON : estiOFF;

STI_BAIL:

	return eBlock;
}

/*!\brief Sets the block caller ID switch (on/off)
 *
 *  \retval estiOK If in an initialized state and the block caller ID set message was
 *      sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::BlockCallerIDSet (
	EstiSwitch eBlock)  //!< Block caller ID switch - estiON or estiOFF.
{
	stiLOG_ENTRY_NAME (CstiCCI::BlockCallerIDSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;

	int nBlockCallerIDEnabled = 0;
	if (estiSTANDARD_MODE == m_eLocalInterfaceMode 
		|| estiHEARING_MODE == m_eLocalInterfaceMode 
		|| estiPUBLIC_MODE == m_eLocalInterfaceMode)
	{
		m_poPM->propertyGet (BLOCK_CALLER_ID_ENABLED, &nBlockCallerIDEnabled);
		if (nBlockCallerIDEnabled)
		{
			hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
			stiTESTRESULT ();

			stConferenceParams.bBlockCallerID = (eBlock == estiON);

			hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
			stiTESTRESULT ();
		}
	}

	m_poPM->propertySet (BLOCK_CALLER_ID, stConferenceParams.bBlockCallerID ? 1 : 0, PropertyManager::Persistent);
	PropertyManager::getInstance ()->PropertySend(BLOCK_CALLER_ID, estiPTypeUser);

STI_BAIL:

	return (hResult);
}

/*! \brief Gets the current block anonymous callers setting (on/off).
 *
 *  \retval estiON If block anonymous callers is turned on.
 *  \retval estiOFF If it is turned off.
 */
EstiSwitch CstiCCI::BlockAnonymousCallersGet () const
{
	stiLOG_ENTRY_NAME (CstiCCI::BlockAnonymousCallersGet);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiSwitch eBlock = estiOFF;
	
	SstiConferenceParams stConferenceParams;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();
	
	eBlock = stConferenceParams.bBlockAnonymousCallers ? estiON : estiOFF;
	
STI_BAIL:
	
	return eBlock;
}

/*!\brief Sets the block anonymous callers switch (on/off)
 *
 *  \retval estiOK If in an initialized state and the block anonymous callers set message was
 *      sent successfully.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCCI::BlockAnonymousCallersSet (
	EstiSwitch eBlock)  //!< Block anonymous callers switch - estiON or estiOFF.
{
	stiLOG_ENTRY_NAME (CstiCCI::BlockAnonymousCallersSet);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	
	if (estiSTANDARD_MODE == m_eLocalInterfaceMode 
		|| estiHEARING_MODE == m_eLocalInterfaceMode)
	{
		hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
		stiTESTRESULT ();
		
		stConferenceParams.bBlockAnonymousCallers = (eBlock == estiON);
		
		hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
		stiTESTRESULT ();
	}
	
	m_poPM->propertySet (BLOCK_ANONYMOUS_CALLERS, stConferenceParams.bBlockAnonymousCallers ? 1 : 0, PropertyManager::Persistent);
	PropertyManager::getInstance ()->PropertySend (BLOCK_ANONYMOUS_CALLERS, estiPTypeUser);
	
STI_BAIL:
	
	return (hResult);
}

stiHResult CstiCCI::LocalAlternateNameGet (
	std::string *pAltName)
{
	return (m_pConferenceManager->localAlternateNameGet (pAltName));
}

bool CstiCCI::IPv6EnabledGet ()
{
	stiLOG_ENTRY_NAME (CstiCCI::IPv6EnabledGet);

	return m_bIPv6Enabled;
}

stiHResult CstiCCI::IPv6EnabledSet (bool bEnable)
{
	stiLOG_ENTRY_NAME (CstiCCI::IPv6EnabledSet);

	m_poPM->propertySet(IPv6Enabled, bEnable ? 1 : 0, PropertyManager::Persistent);
	IPv6EnabledChanged();

	return stiRESULT_SUCCESS;
}

// Checks if the current connected call is eligible to spawn a new relay call.
bool CstiCCI::NewRelayCallAllowed ()
{
	bool bNewRelayCallAllowed = false;
	
	if (m_bNewCallEnabled && !isOffline())
	{
		auto call = m_pConferenceManager->CallObjectGet(esmiCS_CONNECTED);
		if (call)
		{
			if (call->DirectionGet() == estiOUTGOING)
			{
				bNewRelayCallAllowed = call->CanSpawnNewRelayCall ();
			}
		}
	}
	
	return bNewRelayCallAllowed;
}

// Places a connected call on hold. "Connected" does not mean the call is in the
// esmiCS_CONNECTED state, the call may be on hold remotely (i.e. esmiCS_HOLD_RMT)
stiHResult CstiCCI::ConnectedCallHold ()
{
	// Check for remotely held call, fixes bug #21505 where engine would fail
	// silently because remotely held call wasn't being put on hold locally.
	auto call = m_pConferenceManager->CallObjectGet(esmiCS_CONNECTED | esmiCS_HOLD_RMT);
	stiHResult hResult = stiRESULT_ERROR;
	
	if (call)
	{
		hResult = call->Hold();
	}
	
	return hResult;
}

// Sends a phone number to the remote endpoint via ContactShare.
// This should be called if ShouldPlaceNewCall returned false.
stiHResult CstiCCI::SpawnNewCall (
	const std::string &dialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto call = m_pConferenceManager->CallObjectGet(esmiCS_CONNECTED);
	SstiSharedContact contact;
	
	stiTESTCOND(call, stiRESULT_ERROR);

	if (dialString.length() > 0)
	{
		contact.DialString = dialString;
		call->SpawnCallNumberSet("");
		call->SpawnCall(contact);
	}

STI_BAIL:
	return hResult;
}

EstiHearingCallStatus CstiCCI::HearingStatusGet ()
{
	return m_eHearingStatus;
}

void CstiCCI::HearingStatusSet (EstiHearingCallStatus eHearingStatus)
{
	m_eHearingStatus = eHearingStatus;
}

stiHResult CstiCCI::SipRestartTimeChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_poPM->propertyGet (UNREGISTERED_SIP_RESTART_TIME, &m_nStackRestartDelta);
	
	return hResult;
}


stiHResult CstiCCI::FirFeedbackEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams conferenceParams;
	bool firFeedbackEnabled = false;
	int value = 0;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&conferenceParams);
	stiTESTRESULT ();
	
	m_poPM->propertyGet(FIR_FEEDBACK_ENABLED, &value);
	firFeedbackEnabled = (value == 1);
	
	if (conferenceParams.rtcpFeedbackFirEnabled != firFeedbackEnabled)
	{
		conferenceParams.rtcpFeedbackFirEnabled = firFeedbackEnabled;
		hResult = m_pConferenceManager->ConferenceParamsSet(&conferenceParams);
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}


/*!
 * \brief Handler for when PLI feedback enable value changes
 */
stiHResult CstiCCI::PliFeedbackEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams conferenceParams;
	bool pliFeedbackEnabled = false;
	int value = 0;

	hResult = m_pConferenceManager->ConferenceParamsGet (&conferenceParams);
	stiTESTRESULT ();

	m_poPM->propertyGet(PLI_FEEDBACK_ENABLED, &value);
	pliFeedbackEnabled = (value == 1);

	if (conferenceParams.rtcpFeedbackPliEnabled != pliFeedbackEnabled)
	{
		conferenceParams.rtcpFeedbackPliEnabled = pliFeedbackEnabled;
		hResult = m_pConferenceManager->ConferenceParamsSet(&conferenceParams);
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}


/*!
 * \brief Handler for when AFB feedback enable value changes
 */
stiHResult CstiCCI::AfbFeedbackEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams conferenceParams;
	bool afbFeedbackEnabled = false;
	int value = 0;

	hResult = m_pConferenceManager->ConferenceParamsGet (&conferenceParams);
	stiTESTRESULT ();

	m_poPM->propertyGet(AFB_FEEDBACK_ENABLED, &value);
	afbFeedbackEnabled = (value == 1);

	if (conferenceParams.rtcpFeedbackAfbEnabled != afbFeedbackEnabled)
	{
		conferenceParams.rtcpFeedbackAfbEnabled = afbFeedbackEnabled;
		hResult = m_pConferenceManager->ConferenceParamsSet(&conferenceParams);
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}



/*!
 * \brief Handler for when TMMBR feedback enable value changes
 */
stiHResult CstiCCI::TmmbrFeedbackEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams conferenceParams;
	SignalingSupport tmmbrFeedbackEnabled = SignalingSupport::Disabled;
	int value = 0;

	hResult = m_pConferenceManager->ConferenceParamsGet (&conferenceParams);
	stiTESTRESULT ();

	m_poPM->propertyGet(TMMBR_FEEDBACK_ENABLED, &value);
	tmmbrFeedbackEnabled = static_cast<SignalingSupport>(value);

	if (conferenceParams.rtcpFeedbackTmmbrEnabled != tmmbrFeedbackEnabled)
	{
		conferenceParams.rtcpFeedbackTmmbrEnabled = tmmbrFeedbackEnabled;
		hResult = m_pConferenceManager->ConferenceParamsSet(&conferenceParams);
		stiTESTRESULT ();
	}

STI_BAIL:
	return hResult;
}


/*!
 * \brief Handler for when NACK/RTX feedback enable value changes
 */
stiHResult CstiCCI::NackRtxFeedbackEnabledChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams conferenceParams;
	SignalingSupport nackRtxFeedbackSupport = SignalingSupport::Disabled;
	int value = 0;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&conferenceParams);
	stiTESTRESULT ();
	
	m_poPM->propertyGet(NACK_RTX_FEEDBACK_ENABLED, &value);
	nackRtxFeedbackSupport = static_cast<SignalingSupport>(value);
	
	if (conferenceParams.rtcpFeedbackNackRtxSupport != nackRtxFeedbackSupport)
	{
		conferenceParams.rtcpFeedbackNackRtxSupport = nackRtxFeedbackSupport;
		hResult = m_pConferenceManager->ConferenceParamsSet(&conferenceParams);
		stiTESTRESULT ();
	}
	
STI_BAIL:
	return hResult;
}


/*!\brief Updates the vrsFailoverTimeout value in conference params.
 *
 *  \return Success or Failure.
 */
stiHResult CstiCCI::VRSFailOverTimeoutChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	SstiConferenceParams stConferenceParams;
	
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();
	
	m_poPM->propertyGet (VRS_FAILOVER_TIMEOUT,
							(int*)&stConferenceParams.vrsFailoverTimeout);
	
	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}


/*!\brief Updates the service outage client with a newly received URL
*
*  \return Success or Failure.
*/
stiHResult CstiCCI::ServiceOutageServiceUrlChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string serviceOutageUrl = (char *)stiSERVICE_OUTAGE_SERVER_DEFAULT;
	m_poPM->propertyGet (SERVICE_OUTAGE_SERVICE_URL1, &serviceOutageUrl);

	m_serviceOutageClient.urlSet (serviceOutageUrl);

	return hResult;
}


/*!\brief Updates the service outage client with a newly received timer interval
*
*  \return Success or Failure.
*/
stiHResult CstiCCI::ServiceOutageTimerIntervalChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int serviceOutageTimerInterval = vpe::ServiceOutageClient::DefaultTimerInterval;
	m_poPM->propertyGet (SERVICE_OUTAGE_TIMER_INTERVAL, &serviceOutageTimerInterval);

	m_serviceOutageClient.timerIntervalSet (serviceOutageTimerInterval);

	return hResult;
}


/*!\brief Performs a DNS lookup for the vrs failover server.
 *
 *  \return Success or Failure.
 */
stiHResult CstiCCI::VRSFailoverDomainResolve ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string failOverDomain;
	
	m_poPM->propertyGet (VRS_FAILOVER_SERVER,
							   &failOverDomain,
							   PropertyManager::Persistent);
	
	hResult = stiDNSGetHostByName (failOverDomain.c_str (), nullptr, &m_vrsFailoverIP);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}


std::shared_ptr<IstiContact> CstiCCI::findContact(std::string dialString)
{
	auto contactCopy = m_pContactManager->contactByPhoneNumberGet(dialString);

#ifdef stiLDAP_CONTACT_LIST
	if (!contactCopy && m_pLDAPDirectoryContactManager->EnabledGet())
	{
		// Not in the phonebook, so try to find the number in the LDAP directory
		contactCopy = m_pLDAPDirectoryContactManager->contactByPhoneNumberGet(dialString);
	}
#endif

	return contactCopy;
}

std::shared_ptr<IstiContact> CstiCCI::findContact(const CstiItemId &contactId)
{
	auto contactCopy = m_pContactManager->contactByIDGet(contactId);

#ifdef stiLDAP_CONTACT_LIST
	if (!contactCopy && m_pLDAPDirectoryContactManager->EnabledGet())
	{
		// Not in the phonebook, so try to find the number in the LDAP directory
		contactCopy = m_pLDAPDirectoryContactManager->contactByIDGet(contactId);
	}
#endif

	return contactCopy;
}

/*!
* \brief Tells us if we can reach Core or not
*
* \return True if we cannot reach Core, False otherwise
*/
bool CstiCCI::isOffline()
{
	bool offline = false;

	if (m_pCoreService)
	{
		offline = m_pCoreService->isOfflineGet();
	}

	return offline;
}

/*!
* \brief Compares dial string against know customer service numbers
*
* \return True if number is customer service, False otherwise
*/
bool CstiCCI::isCustomerServiceNumber(std::string dialString)
{
	return CstiPhoneNumberValidator::InstanceGet()->PhoneNumberIsSupport (dialString);
}

// End file CstiCCI.cpp
